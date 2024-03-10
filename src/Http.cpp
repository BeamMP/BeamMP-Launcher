#include "Http.h"
#include <cmath>
#include <fstream>
#include <httplib.h>
#include <iostream>
#include <mutex>
#include <spdlog/spdlog.h>

static httplib::Client& get_client(const std::string& host) {
    static thread_local std::unordered_map<std::string /* host */, httplib::Client> s_clients_cache {};
    if (!s_clients_cache.contains(host)) {
        spdlog::debug("Caching connection to {}", host);
        s_clients_cache.emplace(host, httplib::Client(host));
    } else {
        spdlog::debug("Reusing cached connection to {}", host);
    }
    return s_clients_cache.at(host);
}

bool HTTP::isDownload = false;
std::string HTTP::Get(const std::string& host_and_target) {
    static std::mutex Lock;
    std::scoped_lock Guard(Lock);

    auto pos = host_and_target.find('/', 10);

    auto& cli = get_client(host_and_target.substr(0, pos).c_str());
    cli.set_connection_timeout(std::chrono::seconds(10));
    cli.set_follow_location(true);

    httplib::Headers headers {
        { "Accept-Encoding", "gzip" }
    };

    auto res = cli.Get(host_and_target.substr(pos).c_str(), headers);
    std::string Ret;

    if (res) {
        if (res->status == 200) {
            Ret = res->body;
        } else
            spdlog::error(res->reason);

    } else {
        if (isDownload) {
            std::cout << "\n";
        }
        spdlog::error("HTTP Get failed on " + to_string(res.error()));
    }

    return Ret;
}

std::string HTTP::Post(const std::string& host_and_target, const std::string& Fields) {
    static std::mutex Lock;
    std::scoped_lock Guard(Lock);

    auto pos = host_and_target.find('/', 10);

    auto& cli = get_client(host_and_target.substr(0, pos).c_str());
    cli.set_connection_timeout(std::chrono::seconds(10));
    std::string Ret;

    if (!Fields.empty()) {
        httplib::Result res = cli.Post(host_and_target.substr(pos).c_str(), Fields, "application/json");

        if (res) {
            if (res->status != 200) {
                spdlog::error(res->reason);
            }
            Ret = res->body;
        } else {
            spdlog::error("HTTP Post failed on " + to_string(res.error()));
        }
    } else {
        httplib::Result res = cli.Post(host_and_target.substr(pos).c_str());
        if (res) {
            if (res->status != 200) {
                spdlog::error(res->reason);
            }
            Ret = res->body;
        } else {
            spdlog::error("HTTP Post failed on " + to_string(res.error()));
        }
    }

    if (Ret.empty())
        return "-1";
    else
        return Ret;
}

bool HTTP::Download(const std::string& IP, const std::string& Path) {
    static std::mutex Lock;
    std::scoped_lock Guard(Lock);

    isDownload = true;
    std::string Ret = Get(IP);
    isDownload = false;

    if (Ret.empty())
        return false;

    std::ofstream File(Path, std::ios::binary);
    if (File.is_open()) {
        File << Ret;
        File.close();
        std::cout << "\n";
        spdlog::info("Download Complete!");
    } else {
        spdlog::error("Failed to open file directory: " + Path);
        return false;
    }

    return true;
}
