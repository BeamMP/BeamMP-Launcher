// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/18/2020
///
#pragma once
#include <evpp/httpc/response.h>
#include <evpp/httpc/request.h>
#include <string>
#include "Logger.h"
class HTTP{
public:
    static void Response(const std::shared_ptr<evpp::httpc::Response>& response, evpp::httpc::Request* request);
    static bool Download(const std::string &IP, const std::string &Path);
    static std::string Post(const std::string& IP, const std::string& Fields);
    static std::string Get(const std::string &IP);
    static void Init(char* argv[]){
        google::InitGoogleLogging(argv[0]);
        google::SetStderrLogging(google::GLOG_ERROR);
        evpp::httpc::SET_SSL_VERIFY_MODE(SSL_VERIFY_NONE);
    }
private:
    static std::string Res_;
};