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
#include <errno.h>
#include <netdb.h>
#endif

#include "Logger.h"
#include "Network/network.hpp"

static bool IsNumericAddress(const std::string& IP) {
    struct in_addr Addr4 {};
    struct in6_addr Addr6 {};
    return inet_pton(AF_INET, IP.c_str(), &Addr4) == 1
        || inet_pton(AF_INET6, IP.c_str(), &Addr6) == 1;
}

// Splits "host:port" or "[host]:port" (needed for IPv6 literals) into Host and Port.
// Returns false when there is no usable port.
bool SplitHostPort(const std::string& In, std::string& Host, int& Port) {
    Port = -1;
    if (In.empty())
        return false;
    std::string HostPart;
    std::string PortPart;
    if (In.front() == '[') {
        auto Close = In.find(']');
        if (Close == std::string::npos || Close + 1 >= In.size() || In[Close + 1] != ':')
            return false;
        HostPart = In.substr(1, Close - 1);
        PortPart = In.substr(Close + 2);
    } else {
        auto Colon = In.find(':');
        if (Colon == std::string::npos || In.find(':', Colon + 1) != std::string::npos)
            return false;
        HostPart = In.substr(0, Colon);
        PortPart = In.substr(Colon + 1);
    }
    if (HostPart.empty() || PortPart.empty())
        return false;
    try {
        size_t Pos = 0;
        int Value = std::stoi(PortPart, &Pos);
        if (Pos != PortPart.size() || Value < 1 || Value > 65535)
            return false;
        Port = Value;
    } catch (const std::exception&) {
        return false;
    }
    Host = HostPart;
    return true;
}

std::string GetAddr(const std::string& IP) {
    if (IsNumericAddress(IP))
        return IP;
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(514, &wsaData) != 0) {
        error("WSA Startup Failed!");
        WSACleanup();
        return "";
    }
#endif

    struct addrinfo Hints {};
    Hints.ai_family = AF_UNSPEC;
    Hints.ai_socktype = SOCK_STREAM;
    struct addrinfo* Result = nullptr;
    int Res = getaddrinfo(IP.c_str(), nullptr, &Hints, &Result);
    if (Res != 0 || !Result) {
        error("DNS lookup failed! on " + IP);
        if (Result)
            freeaddrinfo(Result);
        WSACleanup();
        return "DNS";
    }
    char Host[NI_MAXHOST] = { 0 };
    Res = getnameinfo(Result->ai_addr, Result->ai_addrlen, Host, sizeof(Host), nullptr, 0, NI_NUMERICHOST);
    freeaddrinfo(Result);
    WSACleanup();
    if (Res != 0) {
        error("DNS numeric conversion failed! on " + IP);
        return "DNS";
    }
    return std::string(Host);
}

// Connects a TCP socket to Host:Port, trying every resolved address (IPv6 and
// IPv4) in order. Returns the connected socket, or -1 on failure. On Windows,
// WSA is left initialized on success so the returned socket stays usable.
uint64_t ConnectTCP(const std::string& Host, int Port) {
    if (Host.empty() || Port < 1 || Port > 65535)
        return -1;
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(514, &wsaData) != 0) {
        error("WSA Startup Failed!");
        return -1;
    }
#endif

    struct addrinfo Hints {};
    Hints.ai_family = AF_UNSPEC;
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_protocol = IPPROTO_TCP;
    struct addrinfo* Result = nullptr;
    int Res = getaddrinfo(Host.c_str(), std::to_string(Port).c_str(), &Hints, &Result);
    if (Res != 0 || !Result) {
        debug("Failed to resolve " + Host + " (" + std::to_string(Res) + ")");
        if (Result)
            freeaddrinfo(Result);
        WSACleanup();
        return -1;
    }
    SOCKET Sock = -1;
    for (struct addrinfo* Ptr = Result; Ptr != nullptr; Ptr = Ptr->ai_next) {
        Sock = socket(Ptr->ai_family, Ptr->ai_socktype, Ptr->ai_protocol);
        if (Sock == -1)
            continue;
        if (connect(Sock, Ptr->ai_addr, int(Ptr->ai_addrlen)) == 0)
            break;
        KillSocket(Sock);
        Sock = -1;
    }
    freeaddrinfo(Result);
    if (Sock == -1) {
        debug("Connection to " + Host + " failed with error: " + std::to_string(WSAGetLastError()));
        WSACleanup();
    }
    return Sock;
}
