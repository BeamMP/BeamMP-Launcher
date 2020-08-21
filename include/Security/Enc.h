///
/// Created by Anonymous275 on 7/16/2020
///
#pragma once
#include <string>
#include "Xor.h"
std::string RSA_D(const std::string& Data,int d, int n);
std::string RSA_E(const std::string& Data,int e, int n);
std::string LocalEnc(const std::string& Data);
std::string LocalDec(const std::string& Data);
std::string Encrypt(std::string msg);
std::string Decrypt(std::string msg);