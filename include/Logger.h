///
/// Created by Anonymous275 on 12/26/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include <easylogging++.h>
#undef min
#undef max
class Log {
   public:
   static void Init();
   static void ConsoleOutput(bool enable);
   private:
   static inline el::Configurations Conf{};
};
