// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 9/25/2020
///

#include <string>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined(__linux__)
#include "linuxfixes.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#endif

#include "Logger.h"

/**
 * Resolve IPs of host, prefered IPv6, and return IPv4 otherwise.
 */
std::string resolveHost(const std::string& hostStr) {
    struct addrinfo* addresses = nullptr;
    struct addrinfo hints {};
    memset(&hints, 0, sizeof(hints));

    std::string resolved = "";

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(514, &wsaData) != 0) {
        error("WSA Startup Failed!");
        WSACleanup();
        return resolved;
    }
#endif
    // UNSPEC to resolve both ip stack (IPv4 & IPv6)
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int res = getaddrinfo(hostStr.c_str(), nullptr, &hints, &addresses);
    if (res != 0)
    {
        std::cerr << "getaddrinfo failed: " << gai_strerror(res) << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        freeaddrinfo(addresses);
        return resolved;
    }

    //Loop all and return it by prefeence ipv6
    for (struct addrinfo* ptr = addresses; ptr != nullptr; ptr = ptr->ai_next) {
        char ipstr[INET6_ADDRSTRLEN] = { 0 };

        if (ptr->ai_family == AF_INET6) {
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)ptr->ai_addr;
            inet_ntop(AF_INET6, &(ipv6->sin6_addr), ipstr, sizeof(ipstr));
            resolved = ipstr;
            break; //Break if IPv6 finded
        }
        else if (ptr->ai_family == AF_INET) {
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)ptr->ai_addr;
            inet_ntop(AF_INET, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
            resolved = ipstr;
        }
    }

    freeaddrinfo(addresses);
    WSACleanup();
    return resolved;
}