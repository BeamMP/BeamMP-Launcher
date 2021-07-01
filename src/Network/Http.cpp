// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/18/2020
///
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <iostream>
#include <Logger.h>
#include <fstream>
#include "Http.h"
#include <mutex>
#include <cmath>
#include <httplib.h>

std::string HTTP::Codes_[] =
{
 "Success","Unknown","Connection","BindIPAddress",
 "Read","Write","ExceedRedirectCount","Canceled",
 "SSLConnection","SSLLoadingCerts","SSLServerVerification",
 "UnsupportedMultipartBoundaryChars","Compression"
};
bool HTTP::isDownload = false;
std::string HTTP::Get(const std::string &IP) {
    static std::mutex Lock;
    std::scoped_lock Guard(Lock);

    auto pos = IP.find('/',10);

    httplib::Client cli(IP.substr(0, pos).c_str());
    cli.set_connection_timeout(std::chrono::seconds(10));
    auto res = cli.Get(IP.substr(pos).c_str(), ProgressBar);
    std::string Ret;

    if(res.error() == 0){
        if(res->status == 200){
            Ret = res->body;
        }else error(res->reason);

    }else{
        if(isDownload) {
            std::cout << "\n";
        }
        error("HTTP Get failed on " + Codes_[res.error()]);
    }

    return Ret;
}

std::string HTTP::Post(const std::string& IP, const std::string& Fields) {
    static std::mutex Lock;
    std::scoped_lock Guard(Lock);

    auto pos = IP.find('/',10);

    httplib::Client cli(IP.substr(0, pos).c_str());
    cli.set_connection_timeout(std::chrono::seconds(10));
    std::string Ret;

    if(!Fields.empty()) {
        httplib::Result res = cli.Post(IP.substr(pos).c_str(), Fields, "application/json");

        if(res.error() == 0) {
            if (res->status != 200) {
                error(res->reason);
            }
            Ret = res->body;
        }else{
            error("HTTP Post failed on " + Codes_[res.error()]);
        }
    }else{
        httplib::Result res = cli.Post(IP.substr(pos).c_str());
        if(res.error() == 0) {
            if (res->status != 200) {
                error(res->reason);
            }
            Ret = res->body;
        }else{
            error("HTTP Post failed on " + Codes_[res.error()]);
        }
    }

    if(Ret.empty())return "-1";
    else return Ret;
}

bool HTTP::ProgressBar(size_t c, size_t t){
    if(isDownload) {
        static double last_progress, progress_bar_adv;
        progress_bar_adv = round(c / double(t) * 25);
        std::cout << "\r";
        std::cout << "Progress : [ ";
        std::cout << round(c / double(t) * 100);
        std::cout << "% ] [";
        int i;
        for (i = 0; i <= progress_bar_adv; i++)std::cout << "#";
        for (i = 0; i < 25 - progress_bar_adv; i++)std::cout << ".";
        std::cout << "]";
        last_progress = round(c / double(t) * 100);
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

    std::ofstream File(Path, std::ios::binary);
    if(File.is_open()) {
        File << Ret;
        File.close();
        std::cout << "\n";
        info("Download Complete!");
    }else{
        error("Failed to open file directory: " + Path);
        return false;
    }

    return true;
}
