///
/// Created by Anonymous275 on 7/16/2020
///
#pragma once
#include <string>
#include "Xor.h"
struct RSA{
    int n = 0;
    int e = 0;
    int d = 0;
};
std::string RSA_D(const std::string& Data,int d, int n);
std::string RSA_E(const std::string& Data,int e, int n);
std::string LocalEnc(const std::string& Data);
std::string LocalDec(const std::string& Data);
RSA* GenKey();