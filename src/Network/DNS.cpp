/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/


#include <string>

#if defined(_WIN32)
#include <winsock2.h>
#elif defined(__linux__)
#include "linuxfixes.h"
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include "Logger.h"

std::string GetAddr(const std::string& IP) {
    if (IP.find_first_not_of("0123456789.") == -1)
        return IP;
    hostent* host;
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(514, &wsaData) != 0) {
        error("WSA Startup Failed!");
        WSACleanup();
        return "";
    }
#endif

    host = gethostbyname(IP.c_str());
    if (!host) {
        error("DNS lookup failed! on " + IP);
        WSACleanup();
        return "DNS";
    }
    std::string Ret = inet_ntoa(*((struct in_addr*)host->h_addr));
    WSACleanup();
    return Ret;
}