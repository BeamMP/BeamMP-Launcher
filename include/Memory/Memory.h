///
/// Created by Anonymous275 on 1/21/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include <string>

class Memory{
public:
    static uint64_t FindByPattern(const char* module, const char* Pattern, const char* Mask);
    static uint64_t GetModuleBase(const char* Name);
    static void Print(const std::string& msg);
    static void Inject(uint32_t PID);
    static uint32_t GetBeamNGPID();
    static uint32_t EntryPoint();
    static uint32_t GetPID();
};
