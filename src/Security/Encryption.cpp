#include "Security/Enc.h"
#include <iostream>
#include <sstream>

int LocalKeys[] = {7406809,6967,4810803}; //n e d

int log_power(int n,unsigned int p, int mod){
    int result = 1;
    for (; p; p >>= 1u){
        if (p & 1u)result = int((1LL * result * n) % mod);
        n = int((1LL * n * n) % mod);
    }
    return result;
}

int Enc(int value,int e,int n){
    return log_power(value, e, n);
}

int Dec(int value,int d,int n){
    return log_power(value, d, n);
}
std::string LocalEnc(const std::string& Data){
    std::stringstream stream;
    for(const char&c : Data){
        stream << std::hex << Enc(uint8_t(c),LocalKeys[1],LocalKeys[0]) << "g";
    }
    return stream.str();
}
std::string LocalDec(const std::string& Data){
    std::stringstream ss(Data);
    std::string token,ret;
    while (std::getline(ss, token, 'g')) {
        if(token.find_first_not_of(Sec("0123456789abcdef")) != std::string::npos)return "";
        int c = std::stoi(token, nullptr, 16);
        ret += char(Dec(c,LocalKeys[2],LocalKeys[0]));
    }
    return ret;
}
#include <random>
int Rand(){
    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_int_distribution<int> uniform_dist(1, 200);
    return uniform_dist(e1);
}
std::string Encrypt(std::string msg){
    if(msg.size() < 2)return msg;
    int R = (Rand()+Rand())/2,T = R;
    for(char&c : msg){
        if(R > 30)c = char(int(c) + (R-=3));
        else c = char(int(c) - (R+=4));
    }
    return char(T) + msg;
}
std::string Decrypt(std::string msg){
    if(msg.size() < 2)return "";
    int R = uint8_t(msg.at(0));
    if(R > 200 || R < 1)return "";
    msg = msg.substr(1);
    for(char&c : msg){
        if(R > 30)c = char(int(c) - (R-=3));
        else c = char(int(c) + (R+=4));
    }
    return msg;
}
std::string RSA_E(const std::string& Data,int e, int n){
    if(e < 10 || n < 10)return "";
    std::stringstream stream;
    for(const char&c : Data){
        stream << std::hex << Enc(uint8_t(c),e,n) << "g";
    }
    return stream.str();
}

std::string RSA_D(const std::string& Data, int d, int n){
    std::stringstream ss(Data);
    std::string token,ret;
    while (std::getline(ss, token, 'g')) {
        if(token.find_first_not_of(Sec("0123456789abcdef")) != std::string::npos)return "";
        int c = std::stoi(token, nullptr, 16);
        ret += char(Dec(c,d,n));
    }
    return ret;
}