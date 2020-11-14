///
/// Created by Anonymous275 on 7/18/2020
///
#pragma once
#include <string>
void PreGame(int argc, char* argv[],const std::string& GamePath);
void InitGame(const std::string& Dir,const std::string&Current);
std::string CheckVer(const std::string &path);
void FindDLL(const std::string& Name);
void SecurityCheck(char* argv[]);
std::string GetGameDir();
void LegitimacyCheck();
