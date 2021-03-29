// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/18/2020
///

#include <evpp/event_loop_thread.h>
#include <evpp/httpc/conn_pool.h>
#include <iostream>
#include <Logger.h>
#include <fstream>
#include "http.h"
#include <mutex>
#include <cmath>
#include <evhttp.h>

#include "winmain-inl.h"

static bool responded;
std::string HTTP::Res_{};
void HTTP::Response(const std::shared_ptr<evpp::httpc::Response>& response, evpp::httpc::Request* request){
    if(response->http_code() == 200){
        Res_ = std::move(response->body().ToString());
    } else {
        std::cout << "\n";
        error("Failed request! http code " + std::to_string(response->http_code()));
    }
    responded = true;
    assert(request == response->request());
    delete request;
}

std::string HTTP::Get(const std::string &IP) {
    static std::mutex Lock;
    std::scoped_lock Guard(Lock);
    responded = false;
    Res_.clear();
    evpp::EventLoopThread t;
    t.Start(true);

    auto pos = IP.find('/');
    std::shared_ptr<evpp::httpc::ConnPool> pool = std::make_shared<evpp::httpc::ConnPool>(IP.substr(0, pos), 443, true,
                                                                                          evpp::Duration(10.0));
    auto *r = new evpp::httpc::Request(pool.get(), t.loop(), IP.substr(pos), "");
    r->Execute(Response);

    while (!responded) {
        usleep(1);
    }

    pool->Clear();
    pool.reset();
    t.Stop(true);
    return Res_;
}

std::string HTTP::Post(const std::string& IP, const std::string& Fields) {
    static std::mutex Lock;
    std::scoped_lock Guard(Lock);
    responded = false;
    Res_.clear();
    evpp::EventLoopThread t;
    t.Start(true);


    auto pos = IP.find('/');
    std::shared_ptr<evpp::httpc::ConnPool> pool = std::make_shared<evpp::httpc::ConnPool>(IP.substr(0, pos), 443,
                                                                                          true,
                                                                                          evpp::Duration(10.0));
    auto *r = new evpp::httpc::Request(pool.get(), t.loop(), IP.substr(pos), Fields);
    if(!Fields.empty()) {
        r->AddHeader("Content-Type", "application/json");
    }
    r->set_request_type(EVHTTP_REQ_POST);
    r->Execute(Response);

    while (!responded) {
        usleep(1);
    }

    pool->Clear();
    pool.reset();
    t.Stop(true);
    if(Res_.empty())return "-1";
    else return Res_;
}

void ProgressBar(size_t d, size_t t){
    static double last_progress, progress_bar_adv;
    progress_bar_adv = round(d/double(t)*25);
    std::cout << "\r";
    std::cout << "Progress : [ ";
    std::cout << round(d/double(t)*100);
    std::cout << "% ] [";
    int i;
    for(i = 0; i <= progress_bar_adv; i++)std::cout<<"#";
    for(i = 0; i < 25 - progress_bar_adv; i++)std::cout<<".";
    std::cout << "]";
    last_progress = round(d/double(t)*100);
}

bool HTTP::Download(const std::string &IP, const std::string &Path) {
    static std::mutex Lock;
    std::scoped_lock Guard(Lock);
    responded = false;
    Res_.clear();
    evpp::EventLoopThread t;
    t.Start(true);
    auto pos = IP.find('/');
    std::shared_ptr<evpp::httpc::ConnPool> pool = std::make_shared<evpp::httpc::ConnPool>(IP.substr(0,pos), 443, true, evpp::Duration(10.0));
    auto* r = new evpp::httpc::Request(pool.get(), t.loop(), IP.substr(pos), "");
    r->set_progress_callback(ProgressBar);
    r->Execute(Response);

    while (!responded) {
        usleep(1);
    }

    pool->Clear();
    pool.reset();
    t.Stop(true);

    if(Res_.empty())return false;

    std::ofstream File(Path, std::ios::binary);
    if(File.is_open()) {
        File << Res_;
        File.close();
        std::cout << "\n";
    }else{
        error("Failed to open file directory: " + Path);
        return false;
    }

    return true;
}
