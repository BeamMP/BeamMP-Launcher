///
/// Created by Anonymous275 on 9/25/2020
///

#include <string>
#include <winsock.h>
#include "Logger.h"
#include "Security/Enc.h"
std::string GetAddr(const std::string&IP){
    if(IP.find_first_not_of("0123456789.") == -1)return IP;
    WSADATA wsaData;
    hostent *host;
    if(WSAStartup(514, &wsaData) != 0){
        error(Sec("WSA Startup Failed!"));
        WSACleanup();
        return "";
    }
    host = gethostbyname(IP.c_str());
    if(!host){
        error(Sec("DNS lookup failed! on ") + IP);
        WSACleanup();
        return "DNS";
    }
    std::string Ret = inet_ntoa(*((struct in_addr *)host->h_addr));
    WSACleanup();
    return Ret;
}