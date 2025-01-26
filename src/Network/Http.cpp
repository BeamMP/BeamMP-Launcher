/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/


#include "Http.h"
#include <Logger.h>
#include <Network/network.hpp>
#include <Startup.h>
#include <Utils.h>
#include <cmath>
#include <curl/curl.h>
#include <curl/easy.h>
#include <filesystem>
#include <fstream>
#include <httplib.h>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>

void WriteHttpDebug(const httplib::Client& client, const std::string& method, const std::string& target, const httplib::Result& result) try {
    const std::filesystem::path folder = ".https_debug";
    std::filesystem::create_directories(folder);
    if (!std::filesystem::exists(folder / "WHAT IS THIS FOLDER.txt")) {
        std::ofstream ignore { folder / "WHAT IS THIS FOLDER.txt" };
        ignore << "This folder exists to help debug current issues with the backend. Do not share this folder with anyone but BeamMP staff. It contains detailed logs of any failed http requests." << std::endl;
    }
    const auto file = folder / (method + ".json");
    // 1 MB limit
    if (std::filesystem::exists(file) && std::filesystem::file_size(file) > 1'000'000) {
        std::filesystem::rename(file, file.generic_string() + ".bak");
    }

    std::ofstream of { file, std::ios::app };
    nlohmann::json js {
        { "utc", std::chrono::system_clock::now().time_since_epoch().count() },
        { "target", target },
        { "client_info", {
                             { "openssl_verify_result", client.get_openssl_verify_result() },
                             { "host", client.host() },
                             { "port", client.port() },
                             { "socket_open", client.is_socket_open() },
                             { "valid", client.is_valid() },
                         } },
    };
    if (result) {
        auto value = result.value();
        js["result"] = {};
        js["result"]["body"] = value.body;
        js["result"]["status"] = value.status;
        js["result"]["headers"] = value.headers;
        js["result"]["version"] = value.version;
        js["result"]["location"] = value.location;
        js["result"]["reason"] = value.reason;
    }
    of << js.dump();
} catch (const std::exception& e) {
    error(e.what());
}

static size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    std::string* Result = reinterpret_cast<std::string*>(userp);
    std::string NewContents(reinterpret_cast<char*>(contents), size * nmemb);
    *Result += NewContents;
    return size * nmemb;
}

bool HTTP::isDownload = false;
std::string HTTP::Get(const std::string& IP) {
    std::string Ret;
    static thread_local CURL* curl = curl_easy_init();
    if (curl) {
        CURLcode res;
        char errbuf[CURL_ERROR_SIZE];
        curl_easy_setopt(curl, CURLOPT_URL, IP.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&Ret);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 120); // seconds
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
        errbuf[0] = 0;
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            error("GET to " + IP + " failed: " + std::string(curl_easy_strerror(res)));
            error("Curl error: " + std::string(errbuf));
            return "";
        }
    } else {
        error("Curl easy init failed");
        return "";
    }
    return Ret;
}

std::string HTTP::Post(const std::string& IP, const std::string& Fields) {
    std::string Ret;
    static thread_local CURL* curl = curl_easy_init();
    if (curl) {
        CURLcode res;
        char errbuf[CURL_ERROR_SIZE];
        curl_easy_setopt(curl, CURLOPT_URL, IP.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&Ret);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, Fields.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, Fields.size());
        struct curl_slist* list = nullptr;
        list = curl_slist_append(list, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 120); // seconds
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
        errbuf[0] = 0;
        res = curl_easy_perform(curl);
        curl_slist_free_all(list);
        if (res != CURLE_OK) {
            error("POST to " + IP + " failed: " + std::string(curl_easy_strerror(res)));
            error("Curl error: " + std::string(errbuf));
            return "";
        }
    } else {
        error("Curl easy init failed");
        return "";
    }
    return Ret;
}

bool HTTP::Download(const std::string& IP, const beammp_fs_string& Path) {
    static std::mutex Lock;
    std::scoped_lock Guard(Lock);

    info("Downloading an update (this may take a while)");
    std::string Ret = Get(IP);

    if (Ret.empty()) {
        error("Download failed");
        return false;
    }

    std::ofstream File(Path, std::ios::binary);
    if (File.is_open()) {
        File << Ret;
        File.close();
        info("Download Complete!");
    } else {
        error(beammp_wide("Failed to open file directory: ") + Path);
        return false;
    }

    return true;
}

void set_headers(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Request-Method", "POST, OPTIONS, GET");
    res.set_header("Access-Control-Request-Headers", "X-API-Version");
}

void HTTP::StartProxy() {
    std::thread proxy([&]() {
        httplib::Server HTTPProxy;
        httplib::Headers headers = {
            { "User-Agent", "BeamMP-Launcher/" + GetVer() + GetPatch() },
            { "Accept", "*/*" }
        };
        httplib::Client backend("https://backend.beammp.com");
        httplib::Client forum("https://forum.beammp.com");

        const std::string pattern = ".*";

        auto handle_request = [&](const httplib::Request& req, httplib::Response& res) {
            set_headers(res);
            if (req.has_header("X-BMP-Authentication")) {
                headers.emplace("X-BMP-Authentication", PrivateKey);
            }
            if (req.has_header("X-API-Version")) {
                headers.emplace("X-API-Version", req.get_header_value("X-API-Version"));
            }

            const std::vector<std::string> path = Utils::Split(req.path, "/");

            httplib::Result cli_res;
            const std::string method = req.method;
            std::string host = "";

            if (!path.empty())
                host = path[0];

            if (host == "backend") {
                std::string remaining_path = req.path.substr(std::strlen("/backend"));

                if (method == "GET")
                    cli_res = backend.Get(remaining_path, headers);
                else if (method == "POST")
                    cli_res = backend.Post(remaining_path, headers);

            } else if (host == "avatar") {
                bool error = false;
                std::string username;
                std::string avatar_size = "100";

                if (path.size() > 1) {
                    username = path[1];
                } else {
                    error = true;
                }

                if (path.size() > 2) {
                    try {
                        if (std::stoi(path[2]) > 0)
                            avatar_size = path[2];

                    } catch (std::exception&) { }
                }

                httplib::Result summary_res;

                if (!error) {
                    summary_res = forum.Get("/u/" + username + ".json", headers);

                    if (!summary_res || summary_res->status != 200) {
                        error = true;
                    }
                }

                if (!error) {
                    try {
                        nlohmann::json d = nlohmann::json::parse(summary_res->body, nullptr, false); // can fail with parse_error

                        auto user = d.at("user"); // can fail with out_of_range
                        auto avatar_link_json = user.at("avatar_template"); // can fail with out_of_range

                        auto avatar_link = avatar_link_json.get<std::string>();
                        size_t start_pos = avatar_link.find("{size}");
                        if (start_pos != std::string::npos)
                            avatar_link.replace(start_pos, std::strlen("{size}"), avatar_size);

                        cli_res = forum.Get(avatar_link, headers);

                    } catch (std::exception&) {
                        error = true;
                    }
                }

                if (error) {
                    cli_res = forum.Get("/user_avatar/forum.beammp.com/user/0/0.png", headers);
                }

            } else {
                res.set_content("Host not found", "text/plain");
                return;
            }

            if (cli_res) {
                res.set_content(cli_res->body, cli_res->get_header_value("Content-Type"));
            } else {
                res.set_content(to_string(cli_res.error()), "text/plain");
            }
        };

        HTTPProxy.Get(pattern, [&](const httplib::Request& req, httplib::Response& res) {
            handle_request(req, res);
        });

        HTTPProxy.Post(pattern, [&](const httplib::Request& req, httplib::Response& res) {
            handle_request(req, res);
        });

        ProxyPort = HTTPProxy.bind_to_any_port("127.0.0.1");
        debug("HTTP Proxy listening on port " + std::to_string(ProxyPort));
        HTTPProxy.listen_after_bind();
    });
    proxy.detach();
}
