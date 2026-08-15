/*
 Copyright (C) 2026 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#include "Network/network.hpp"
#include <stdexcept>

#if defined(_WIN32)
#include <ws2tcpip.h>
#elif defined(__linux__)
#include "linuxfixes.h"
#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

#include "Logger.h"
#include <array>
#include <string>

SOCKET DVSock = -1;
sockaddr_in ToVehicle;
std::unordered_set<std::string> activeVehicles;
std::unordered_map<std::string, int> vehiclePortMap;

void DVSend(std::string_view Data, int Port) {
    if (DVSock == -1)
        return;
    ToVehicle.sin_port = htons(Port);
    int sendOk = sendto(DVSock, Data.data(), int(Data.size()), 0, (sockaddr*)&ToVehicle, sizeof(ToVehicle));
    if (sendOk == SOCKET_ERROR)
        error("(Direct VE) Failed to send data. Error Code : " + std::to_string(WSAGetLastError()));
}

void DVRcv() {
    sockaddr_in FromVehicle;
    socklen_t size = sizeof(FromVehicle);
    ZeroMemory(&FromVehicle, size);
    static thread_local std::array<char, 10240> Ret {};
    if (DVSock == -1)
        return;
    int32_t Rcv = recvfrom(DVSock, Ret.data(), Ret.size() - 1, 0, (sockaddr*)&FromVehicle, &size);
    if (Rcv == SOCKET_ERROR)
        return;
    Ret[Rcv] = 0;

    std::string Data = std::string(Ret.data(), Rcv);
    size_t first = Data.find(':');
    if (first == std::string::npos) {
        debug("(Direct VE) Failed to parse serverVehicleID from data: " + Data);
        return;
    }
    first += 1;
    size_t len = Data.find(':', first);
    if (len != std::string::npos) {
        len -= first;
    }
    std::string serverVehicleID = Data.substr(first, len);
    if (activeVehicles.contains(serverVehicleID)) {
        int port = ntohs(FromVehicle.sin_port);
        auto portIter = vehiclePortMap.find(serverVehicleID);
        if (portIter != vehiclePortMap.end()) {
            if (portIter->second == port) {
                ServerSend(Data, false);
            } else {
                debug("(Direct VE) Received data for vehicle " + serverVehicleID + " from wrong port: " + std::to_string(port) + " != " + std::to_string(portIter->second));
            }
        } else {
            debug("(Direct VE) Registering port for vehicle " + serverVehicleID + ": " + std::to_string(port));
            vehiclePortMap.insert({ serverVehicleID, port });

            ServerSend(Data, false);
        }
    } else {
        debug("(Direct VE) Received data from unregistered vehicle: " + serverVehicleID);
    }
}

void DVClientMain(const std::string& IP, int Port) {
    debug("(Direct VE) Starting direct vehicle client on adress " + IP + ":" + std::to_string(Port));

#ifdef _WIN32
    WSADATA data;
    if (WSAStartup(514, &data)) {
        error("(Direct VE) Can't start Winsock!");
        return;
    }
#endif
    sockaddr_in DVListenAddr;
    ZeroMemory(&DVListenAddr, sizeof(DVListenAddr));
    DVListenAddr.sin_family = AF_INET;
    DVListenAddr.sin_port = htons(Port);
    inet_pton(AF_INET, IP.c_str(), &DVListenAddr.sin_addr);

    ZeroMemory(&ToVehicle, sizeof(ToVehicle));
    ToVehicle.sin_family = AF_INET;
    ToVehicle.sin_addr = DVListenAddr.sin_addr;

    DVSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (DVSock == -1) {
        error("(Direct VE) Socket creation failed with error: " + std::to_string(WSAGetLastError()));
        KillSocket(DVSock);
        WSACleanup();
        return;
    }
    if (bind(DVSock, (const sockaddr*)&DVListenAddr, sizeof(DVListenAddr)) == SOCKET_ERROR) {
        error("(Direct VE) Socket bind failed with error: " + std::to_string(WSAGetLastError()));
        KillSocket(DVSock);
        WSACleanup();
        return;
    }
    debug("(Direct VE) Starting direct vehicle receive loop");
    while (!TCPTerminate) {
        DVRcv();
    }
    debug("(Direct VE) Direct vehicle receive loop done");
    KillSocket(DVSock);
    WSACleanup();
    activeVehicles.clear();
    vehiclePortMap.clear();
}
