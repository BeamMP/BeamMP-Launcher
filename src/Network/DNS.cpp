/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/


#include <string>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined(__linux__)
#include "linuxfixes.h"
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include "Logger.h"

std::string GetAddr(const std::string& IP) {
    struct addrinfo* res = nullptr;
    struct addrinfo hints {0};
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(514, &wsaData) != 0) {
        error("WSA Startup Failed!");
        WSACleanup();
        return "";
    }
#endif

    hints.ai_family = AF_INET6;
    hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;
    if (getaddrinfo(IP.c_str(), NULL, &hints, &res) != 0) {
        error("DNS lookup failed! on " + IP);
        WSACleanup();
        return "DNS";
    }
    char ipstr[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr, ipstr, sizeof(ipstr));
    std::string Ret = ipstr;
    freeaddrinfo(res);
    WSACleanup();
    return Ret;
}
