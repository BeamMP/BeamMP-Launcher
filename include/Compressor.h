///
/// Created by Anonymous275 on 1/30/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include <string>

class Zlib {
public:
    static std::string DeComp(std::string Compressed);
    static std::string Comp(std::string Data);
};