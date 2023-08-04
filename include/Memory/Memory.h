///
/// Created by Anonymous275 on 1/21/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include <set>
#include <string>

class Memory {
   public:
   static uint64_t FindPattern(const char* module, const char* Pattern[]);
   static uint32_t GetBeamNGPID(const std::set<uint32_t>& BL);
   static uint32_t GetLauncherPID(const std::set<uint32_t>& BL);
   static uint64_t GetModuleBase(const char* Name);
   static void Print(const std::string& msg);
   static std::string GetHex(uint64_t num);
   static inline bool DebugMode = false;
   static void Inject(uint32_t PID);
   static uint32_t GetTickCount();
   static uint32_t EntryPoint();
   static uint32_t GetPID();
};
