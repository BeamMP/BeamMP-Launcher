///
/// Created by Anonymous275 on 6/17/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include <cstdint>

class Memory {
public:
    static size_t GetProcessID(const char* PName);
    static size_t GetModuleBase(const char* Name);
};