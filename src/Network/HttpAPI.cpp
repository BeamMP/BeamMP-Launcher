///
/// Created by Anonymous275 on 1/17/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <cpp-httplib/httplib.h>
#include "Launcher.h"
#include "HttpAPI.h"
#include <iostream>
#include "Logger.h"
#include <fstream>
#include <mutex>
#include <cmath>

bool HTTP::isDownload = false;
std::atomic<httplib::Client*> CliRef = nullptr;
std::string HTTP::Get(const std::string &IP) {
    static std::mutex Lock;
    std::scoped_lock Guard(Lock);

    auto pos = IP.find('/',10);

    httplib::Client cli(IP.substr(0, pos));
    CliRef.store(&cli);
    cli.set_connection_timeout(std::chrono::seconds(5));
    auto res = cli.Get(IP.substr(pos).c_str(), ProgressBar);
    std::string Ret;

    if(res.error() == httplib::Error::Success){
        if(res->status == 200){
            Ret = res->body;
        }else LOG(ERROR) << res->reason;

    }else{
        if(isDownload) {
            std::cout << "\n";
        }
        LOG(ERROR) << "HTTP Get failed on " << httplib::to_string(res.error());
    }
    CliRef.store(nullptr);
    return Ret;
}

std::string HTTP::Post(const std::string& IP, const std::string& Fields) {
    static std::mutex Lock;
    std::scoped_lock Guard(Lock);

    auto pos = IP.find('/',10);

    httplib::Client cli(IP.substr(0, pos));
    CliRef.store(&cli);
    cli.set_connection_timeout(std::chrono::seconds(5));
    std::string Ret;

    if(!Fields.empty()) {
        httplib::Result res = cli.Post(IP.substr(pos).c_str(), Fields, "application/json");

        if(res.error() == httplib::Error::Success) {
            if(res->status != 200) {
                LOG(ERROR) << res->reason;
            }
            Ret = res->body;
        } else{
            LOG(ERROR) << "HTTP Post failed on " << httplib::to_string(res.error());
        }
    } else {
        httplib::Result res = cli.Post(IP.substr(pos).c_str());
        if(res.error() == httplib::Error::Success) {
            if (res->status != 200) {
                LOG(ERROR) << res->reason;
            }
            Ret = res->body;
        } else {
            LOG(ERROR) << "HTTP Post failed on " << httplib::to_string(res.error());
        }
    }
    CliRef.store(nullptr);
    if(Ret.empty())return "-1";
    else return Ret;
}

bool HTTP::ProgressBar(size_t c, size_t t){
    if(isDownload) {
        static double progress_bar_adv;
        progress_bar_adv = round(double(c) / double(t) * 25);
        std::cout << "\r";
        std::cout << "Progress: [ ";
        std::cout << round(double(c) / double(t) * 100);
        std::cout << "% ] [";
        int i;
        for (i = 0; i <= progress_bar_adv; i++)std::cout << "#";
        for (i = 0; i < 25 - progress_bar_adv; i++)std::cout << ".";
        std::cout << "]";
    }
    if(Launcher::Terminated()) {
        CliRef.load()->stop();
        std::cout << '\n';
        isDownload = false;
        throw ShutdownException("Interrupted");
    }
    return true;
}

bool HTTP::Download(const std::string &IP, const std::string &Path) {
    static std::mutex Lock;
    std::scoped_lock Guard(Lock);

    isDownload = true;
    std::string Ret = Get(IP);
    isDownload = false;

    if(Ret.empty())return false;
    std::cout << "\n";
    std::ofstream File(Path, std::ios::binary);
    if(File.is_open()) {
        File << Ret;
        File.close();
        LOG(INFO) << "Download complete!";
    } else {
        LOG(ERROR) << "Failed to open file directory: " << Path;
        return false;
    }
    return true;
}
