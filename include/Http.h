///
/// Created by Anonymous275 on 1/17/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include <string>

class HTTP {
public:
    static bool Download(const std::string &IP, const std::string &Path);
    static std::string Post(const std::string& IP, const std::string& Fields);
    static std::string Get(const std::string &IP);
    static bool ProgressBar(size_t c, size_t t);
public:
    static bool isDownload;
};