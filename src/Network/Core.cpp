// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/20/2020
///
#include "Http.h"
#include "Network/network.hpp"
#include "Security/Init.h"
#include <cstdlib>
#include <regex>
#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined(__linux__)
#include <cstring>
#include <errno.h>
#include <netdb.h>
#include <spawn.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "Logger.h"
#include "Startup.h"
#include <charconv>
#include <nlohmann/json.hpp>
#include <set>
#include <thread>

extern int TraceBack;
std::set<std::string>* ConfList = nullptr;
bool TCPTerminate = false;
int DEFAULT_PORT = 4444;
bool Terminate = false;
bool LoginAuth = false;
std::string Username = "";
std::string UserRole = "";
std::string UlStatus;
std::string MStatus;
bool ModLoaded;
int ping = -1;

void StartSync(const std::string& Data) {
    std::string IP = GetAddr(Data.substr(1, Data.find(':') - 1));
    if (IP.find('.') == -1) {
        if (IP == "DNS")
            UlStatus = "UlConnection Failed! (DNS Lookup Failed)";
        else
            UlStatus = "UlConnection Failed! (WSA failed to start)";
        ListOfMods = "-";
        Terminate = true;
        return;
    }
    CheckLocalKey();
    UlStatus = "UlLoading...";
    TCPTerminate = false;
    Terminate = false;
    ConfList->clear();
    ping = -1;
    std::thread GS(TCPGameServer, IP, std::stoi(Data.substr(Data.find(':') + 1)));
    GS.detach();
    info("Connecting to server");
}

bool IsAllowedLink(const std::string& Link) {
    std::regex link_pattern(R"(https:\/\/(?:\w+)?(?:\.)?(?:beammp\.com|discord\.gg))");
    std::smatch link_match;
    return std::regex_search(Link, link_match, link_pattern) && link_match.position() == 0;
}

void Parse(std::string Data, SOCKET CSocket) {
    char Code = Data.at(0), SubCode = 0;
    if (Data.length() > 1)
        SubCode = Data.at(1);
    switch (Code) {
    case 'A':
        Data = Data.substr(0, 1);
        break;
    case 'B':
        NetReset();
        Terminate = true;
        TCPTerminate = true;
        Data = Code + HTTP::Get("https://backend.beammp.com/servers-info");
        break;
    case 'C':
        ListOfMods.clear();
        StartSync(Data);
        while (ListOfMods.empty() && !Terminate) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (ListOfMods == "-")
            Data = "L";
        else
            Data = "L" + ListOfMods;
        break;
    case 'O': // open default browser with URL
        if (IsAllowedLink(Data.substr(1))) {
#if defined(__linux)
            if (char* browser = getenv("BROWSER"); browser != nullptr && !std::string_view(browser).empty()) {
                pid_t pid;
                auto arg = Data.substr(1);
                char* argv[] = { browser, arg.data() };
                auto status = posix_spawn(&pid, browser, nullptr, nullptr, argv, environ);
                if (status == 0) {
                    debug("Browser PID: " + std::to_string(pid));
                    // we don't wait for it to exit, because we just don't care.
                    // typically, you'd waitpid() here.
                } else {
                    error("Failed to open the following link in the browser (error follows below): " + arg);
                    error(std::string("posix_spawn: ") + strerror(status));
                }
            } else {
                error("Failed to open the following link in the browser because the $BROWSER environment variable is not set: " + Data.substr(1));
            }
#elif defined(WIN32)
            ShellExecuteA(nullptr, "open", Data.substr(1).c_str(), nullptr, nullptr, SW_SHOW); /// TODO: Look at when working on linux port
#endif

            info("Opening Link \"" + Data.substr(1) + "\"");
        }
        Data.clear();
        break;
    case 'P':
        Data = Code + std::to_string(ProxyPort);
        break;
    case 'U':
        if (SubCode == 'l')
            Data = UlStatus;
        if (SubCode == 'p') {
            if (ping > 800) {
                Data = "Up-2";
            } else
                Data = "Up" + std::to_string(ping);
        }
        if (!SubCode) {
            std::string Ping;
            if (ping > 800)
                Ping = "-2";
            else
                Ping = std::to_string(ping);
            Data = std::string(UlStatus) + "\n" + "Up" + Ping;
        }
        break;
    case 'M':
        Data = MStatus;
        break;
    case 'Q':
        if (SubCode == 'S') {
            NetReset();
            Terminate = true;
            TCPTerminate = true;
            ping = -1;
        }
        if (SubCode == 'G')
            exit(2);
        Data.clear();
        break;
    case 'R': // will send mod name
        if (ConfList->find(Data) == ConfList->end()) {
            ConfList->insert(Data);
            ModLoaded = true;
        }
        Data.clear();
        break;
    case 'Z':
        Data = "Z" + GetVer();
        break;
    case 'N':
        if (SubCode == 'c') {
            nlohmann::json Auth = {
                { "Auth", LoginAuth ? 1 : 0 },
            };
            if (!Username.empty()) {
                Auth["username"] = Username;
            }
            if (!UserRole.empty()) {
                Auth["role"] = UserRole;
            }
            Data = "N" + Auth.dump();
        } else {
            Data = "N" + Login(Data.substr(Data.find(':') + 1));
        }
        break;
    default:
        Data.clear();
        break;
    }
    if (!Data.empty() && CSocket != -1) {
        int res = send(CSocket, (Data + "\n").c_str(), int(Data.size()) + 1, 0);
        if (res < 0) {
            debug("(Core) send failed with error: " + std::to_string(WSAGetLastError()));
        }
    }
}
void GameHandler(SOCKET Client) {

    int32_t Size, Temp, Rcv;
    char Header[10] = { 0 };
    do {
        Rcv = 0;
        do {
            Temp = recv(Client, &Header[Rcv], 1, 0);
            if (Temp < 1)
                break;
            if (!isdigit(Header[Rcv]) && Header[Rcv] != '>') {
                error("(Core) Invalid lua communication");
                KillSocket(Client);
                return;
            }
        } while (Header[Rcv++] != '>');
        if (Temp < 1)
            break;
        if (std::from_chars(Header, &Header[Rcv], Size).ptr[0] != '>') {
            debug("(Core) Invalid lua Header -> " + std::string(Header, Rcv));
            break;
        }
        std::string Ret(Size, 0);
        Rcv = 0;

        do {
            Temp = recv(Client, &Ret[Rcv], Size - Rcv, 0);
            if (Temp < 1)
                break;
            Rcv += Temp;
        } while (Rcv < Size);
        if (Temp < 1)
            break;

        Parse(Ret, Client);
    } while (Temp > 0);
    if (Temp == 0) {
        debug("(Core) Connection closing");
    } else {
        debug("(Core) recv failed with error: " + std::to_string(WSAGetLastError()));
    }
    NetReset();
    KillSocket(Client);
}
void localRes() {
    MStatus = " ";
    UlStatus = "Ulstart";
    if (ConfList != nullptr) {
        ConfList->clear();
        delete ConfList;
        ConfList = nullptr;
    }
    ConfList = new std::set<std::string>;
}
void CoreMain() {
    debug("Core Network on start!");
    SOCKET LSocket, CSocket;
    struct addrinfo* res = nullptr;
    struct addrinfo hints { };
    int iRes;
#ifdef _WIN32
    WSADATA wsaData;
    iRes = WSAStartup(514, &wsaData); // 2.2
    if (iRes)
        debug("WSAStartup failed with error: " + std::to_string(iRes));
#endif

    ZeroMemory(&hints, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    iRes = getaddrinfo(nullptr, std::to_string(DEFAULT_PORT).c_str(), &hints, &res);
    if (iRes) {
        debug("(Core) addr info failed with error: " + std::to_string(iRes));
        WSACleanup();
        return;
    }
    LSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (LSocket == -1) {
        debug("(Core) socket failed with error: " + std::to_string(WSAGetLastError()));
        freeaddrinfo(res);
        WSACleanup();
        return;
    }
    iRes = bind(LSocket, res->ai_addr, int(res->ai_addrlen));
    if (iRes == SOCKET_ERROR) {
        error("(Core) bind failed with error: " + std::to_string(WSAGetLastError()));
        freeaddrinfo(res);
        KillSocket(LSocket);
        WSACleanup();
        return;
    }
    iRes = listen(LSocket, SOMAXCONN);
    if (iRes == SOCKET_ERROR) {
        debug("(Core) listen failed with error: " + std::to_string(WSAGetLastError()));
        freeaddrinfo(res);
        KillSocket(LSocket);
        WSACleanup();
        return;
    }
    do {
        CSocket = accept(LSocket, nullptr, nullptr);
        if (CSocket == -1) {
            error("(Core) accept failed with error: " + std::to_string(WSAGetLastError()));
            continue;
        }
        localRes();
        info("Game Connected!");
        GameHandler(CSocket);
        warn("Game Reconnecting...");
    } while (CSocket);
    KillSocket(LSocket);
    WSACleanup();
}

#if defined(_WIN32)
int Handle(EXCEPTION_POINTERS* ep) {
    char* hex = new char[100];
    sprintf_s(hex, 100, "%lX", ep->ExceptionRecord->ExceptionCode);
    except("(Core) Code : " + std::string(hex));
    delete[] hex;
    return 1;
}
#endif

[[noreturn]] void CoreNetwork() {
    while (true) {
#if not defined(__MINGW32__)
        __try {
#endif

            CoreMain();

#if not defined(__MINGW32__) and not defined(__linux__)
        } __except (Handle(GetExceptionInformation())) { }
#elif not defined(__MINGW32__) and defined(__linux__)
    }
    catch (...) {
        except("(Core) Code : " + std::string(strerror(errno)));
    }
#endif

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
