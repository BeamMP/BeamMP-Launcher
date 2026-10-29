/*
 Copyright (C) 2026 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#include "Utils.h"
#include "Jobs/Jobs.h"
#include "Jobs/ProxyMesh.h"
#include "Jobs/VehicleFiles.h"
#include "Logger.h"
#include "Network/network.hpp"
#include "Security/Init.h"
#include "Startup.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

using nlohmann::json;
namespace fs = std::filesystem;

namespace Jobs {

namespace {
    // UTF-8 string -> path (std::filesystem::u8path is deprecated in C++20)
    fs::path Utf8Path(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }
}

namespace {
    constexpr size_t MaxQueued = 256;

    struct Job {
        json request;
        uint64_t generation;
    };

    std::mutex queueMutex;
    std::condition_variable queueCv;
    std::deque<Job> queue;
    std::atomic<uint64_t> generation { 0 };
    std::once_flag startOnce;
    unsigned workerCount = 1;

    void Send(const json& reply, uint64_t gen) {
        if (gen != generation.load())
            return; // the game that asked is gone
        CoreSend("J" + reply.dump(-1, ' ', false, json::error_handler_t::replace));
    }

    json Reply(const json& req, const char* status) {
        return { { "id", req.contains("id") ? req["id"] : json(nullptr) }, { "status", status } };
    }

    json Failed(const json& req, const std::string& reason) {
        auto r = Reply(req, "failed");
        r["reason"] = reason;
        return r;
    }

    json Capabilities() {
        return { { "protocol", ProtocolVersion }, { "jobs", { "ping", "proxy_mesh" } }, { "workers", workerCount }, { "launcher", GetVer() + GetPatch() } };
    }

    fs::path GameDir(const json& req, bool allowOverride) {
        if (allowOverride && req.contains("gameDir") && req["gameDir"].is_string())
            return Utf8Path(req["gameDir"].get<std::string>());
        return fs::path(GetGameDir());
    }
    fs::path UserDir(const json& req, bool allowOverride) {
        if (allowOverride && req.contains("userDir") && req["userDir"].is_string())
            return Utf8Path(req["userDir"].get<std::string>());
        return GetGamePath();
    }

    json Run(const json& req, const ProgressFn& progress, bool allowPathOverride) {
        const auto type = req.value("type", std::string());
        try {
            if (type == "ping") {
                auto r = Reply(req, "done");
                r["result"] = Capabilities();
                return r;
            }
            if (type == "proxy_mesh") {
                auto files = VehicleFiles::Get(GameDir(req, allowPathOverride), UserDir(req, allowPathOverride));
                auto r = Reply(req, "done");
                r["result"] = BuildProxyMesh(req, *files, progress);
                return r;
            }
            return Failed(req, "unknown job type: " + type);
        } catch (const std::exception& e) {
            return Failed(req, e.what());
        }
    }

    void Worker() {
#if defined(_WIN32)
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL); // the game comes first
#endif
        while (true) {
            Job job;
            {
                std::unique_lock lock(queueMutex);
                queueCv.wait(lock, [] { return !queue.empty(); });
                job = std::move(queue.front());
                queue.pop_front();
            }
            if (job.generation != generation.load())
                continue;
            const auto gen = job.generation;
            auto last = std::chrono::steady_clock::now() - std::chrono::seconds(1);
            ProgressFn progress = [&](const std::string& stage, size_t done, size_t total) {
                auto now = std::chrono::steady_clock::now();
                if (now - last < std::chrono::milliseconds(250))
                    return; // a few progress packets per second at most
                last = now;
                auto r = Reply(job.request, "progress");
                r["stage"] = stage;
                r["done"] = done;
                r["total"] = total;
                Send(r, gen);
            };
            auto reply = Run(job.request, progress, false);
            if (reply["status"] == "failed")
                debug("(Jobs) " + job.request.value("type", std::string("?")) + " failed: " + reply.value("reason", std::string()));
            Send(reply, gen);
        }
    }

    void StartWorkers() {
        // a small pool: mesh jobs are disk and memory bound, and the game needs the CPU more
        unsigned hw = std::max(1u, std::thread::hardware_concurrency());
        workerCount = std::clamp(hw / 4, 1u, 2u);
        for (unsigned i = 0; i < workerCount; i++)
            std::thread(Worker).detach();
        debug("(Jobs) Started " + std::to_string(workerCount) + " job worker(s)");
    }
}

void OnGameConnected() {
    generation++;
    std::lock_guard lock(queueMutex);
    queue.clear();
}

void Handle(const std::string& payload) {
    auto req = json::parse(payload, nullptr, false);
    if (!req.is_object()) {
        CoreSend("J" + json { { "id", nullptr }, { "status", "failed" }, { "reason", "bad job request" } }.dump());
        return;
    }
    const auto gen = generation.load();
    if (req.value("type", std::string()) == "ping") { // answered straight away, also tells the mod the pool size
        std::call_once(startOnce, StartWorkers);
        Send(Run(req, nullptr, false), gen);
        return;
    }
    std::call_once(startOnce, StartWorkers);
    {
        std::lock_guard lock(queueMutex);
        if (queue.size() >= MaxQueued) {
            Send(Failed(req, "queue full"), gen);
            return;
        }
        queue.push_back({ std::move(req), gen });
    }
    queueCv.notify_one();
}

json RunNow(const json& request) {
    return Run(request, [](const std::string& stage, size_t done, size_t total) {
        std::cout << "  " << stage << " " << done << "/" << total << "\n";
    }, true);
}

int RunCli(const char* jsonFile) {
    std::ifstream f(Utf8Path(jsonFile), std::ios::binary);
    if (!f) {
        std::cerr << "cannot read " << jsonFile << "\n";
        return 1;
    }
    std::stringstream ss;
    ss << f.rdbuf();
    auto req = json::parse(ss.str(), nullptr, false);
    if (!req.is_object()) {
        std::cerr << "not a JSON object: " << jsonFile << "\n";
        return 1;
    }
    auto reply = RunNow(req);
    std::cout << reply.dump(2) << std::endl;
    return reply["status"] == "done" ? 0 : 1;
}

}
