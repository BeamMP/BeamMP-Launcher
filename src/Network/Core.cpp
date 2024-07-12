// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/20/2020
///
#include "Http.h"
#include "Network/network.hpp"
#include "NetworkHelpers.h"
#include "Security/Init.h"
#include <asio/io_context.hpp>
#include <cstdlib>
#include <optional>
#include <regex>
#if defined(__linux__)
#include <cstring>
#include <errno.h>
#include <netdb.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "Logger.h"
#include "Startup.h"
#include <charconv>
#include <fmt/format.h>
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

asio::io_context io {};

static asio::ip::tcp::socket ResolveAndConnect(const std::string& host_port_string) {

    using namespace asio;
    ip::tcp::resolver resolver(io);
    asio::error_code ec;
    auto port = host_port_string.substr(host_port_string.find_last_of(':') + 1);
    auto host = host_port_string.substr(0, host_port_string.find_last_of(':'));
    auto resolved = resolver.resolve(host, port, ec);
    if (ec) {
        ::error(fmt::format("Failed to resolve '[{}]:{}': {}", host, port, ec.message()));
        throw std::runtime_error(fmt::format("Failed to resolve '{}': {}", host_port_string, ec.message()));
    }
    bool connected = false;

    UlStatus = "UlLoading...";
    
    for (const auto& addr : resolved) {
        try {
            info(fmt::format("Resolved and connected to '[{}]:{}'",
                addr.endpoint().address().to_string(),
                addr.endpoint().port()));
            ip::tcp::socket socket(io);
            socket.connect(addr);
            // done, connected fine
            return socket;
        } catch (...) {
            // ignore
        }
    }
    throw std::runtime_error(fmt::format("Failed to connect to '{}'; connection refused", host_port_string));
}

void StartSync(const std::string& Data) {
    try {
        auto Socket = ResolveAndConnect(Data.substr(1));
        ListOfMods = "-";
        CheckLocalKey();
        TCPTerminate = false;
        Terminate = false;
        ConfList->clear();
        ping = -1;
        std::thread GS(TCPGameServer, std::move(Socket));
        GS.detach();
        info("Connecting to server");
    } catch (const std::exception& e) {
        UlStatus = "UlConnection Failed!";
        error(fmt::format("Client: connect failed! Error: {}", e.what()));
        WSACleanup();
        Terminate = true;
    }
}

bool IsAllowedLink(const std::string& Link) {
    std::vector<std::string> allowed_links = {
        R"(patreon\.com\/beammp$)",
        R"(discord\.gg\/beammp$)",
        R"(forum\.beammp\.com$)",
        R"(beammp\.com$)",
        R"(patreon\.com\/beammp\/$)",
        R"(discord\.gg\/beammp\/$)",
        R"(forum\.beammp\.com\/$)",
        R"(beammp\.com\/$)",
        R"(docs\.beammp\.com$)",
        R"(wiki\.beammp\.com$)",
        R"(docs\.beammp\.com\/$)",
        R"(wiki\.beammp\.com\/$)",
        R"(docs\.beammp\.com\/.*$)",
        R"(wiki\.beammp\.com\/.*$)",
    };
    for (const auto& allowed_link : allowed_links) {
        if (std::regex_match(Link, std::regex(std::string(R"(^http(s)?:\/\/)") + allowed_link))) {
            return true;
        }
    }
    return false;
}

void Parse(std::span<char> InData, asio::ip::tcp::socket& CSocket) {
    std::string OutData;
    char Code = InData[0], SubCode = 0;
    if (InData.size() > 1)
        SubCode = InData[1];
    switch (Code) {
    case 'A':
        OutData = "A";
        break;
    case 'B':
        NetReset();
        Terminate = true;
        TCPTerminate = true;
        OutData = Code + HTTP::Get("https://backend.beammp.com/servers-info");
        break;
    case 'C':
        ListOfMods.clear();
        StartSync(std::string(InData.data(), InData.size()));
        while (ListOfMods.empty() && !Terminate) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (ListOfMods == "-")
            OutData = "L";
        else
            OutData = "L" + ListOfMods;
        break;
    case 'O': // open default browser with URL
        if (IsAllowedLink(bytespan_to_string(InData.subspan(1)))) {
#if defined(__linux)
            if (char* browser = getenv("BROWSER"); browser != nullptr && !std::string_view(browser).empty()) {
                pid_t pid;
                auto arg = bytespan_to_string(InData.subspan(1));
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
                error("Failed to open the following link in the browser because the $BROWSER environment variable is not set: " + bytespan_to_string(InData.subspan(1)));
            }
#elif defined(WIN32)
            ShellExecuteA(nullptr, "open", InData.subspan(1).data(), nullptr, nullptr, SW_SHOW); /// TODO: Look at when working on linux port
#endif

            info("Opening Link \"" + bytespan_to_string(InData.subspan(1)) + "\"");
        }
        OutData.clear();
        break;
    case 'P':
        OutData = Code + std::to_string(ProxyPort);
        break;
    case 'U':
        if (SubCode == 'l')
            OutData = UlStatus;
        if (SubCode == 'p') {
            if (ping > 800) {
                OutData = "Up-2";
            } else
                OutData = "Up" + std::to_string(ping);
        }
        if (!SubCode) {
            std::string Ping;
            if (ping > 800)
                Ping = "-2";
            else
                Ping = std::to_string(ping);
            OutData = std::string(UlStatus) + "\n" + "Up" + Ping;
        }
        break;
    case 'M':
        OutData = MStatus;
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
        OutData.clear();
        break;
    case 'R': // will send mod name
    {
        auto str = bytespan_to_string(InData);
        if (ConfList->find(str) == ConfList->end()) {
            ConfList->insert(str);
            ModLoaded = true;
        }
        OutData.clear();
    } break;
    case 'Z':
        OutData = "Z" + GetVer();
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
            OutData = "N" + Auth.dump();
        } else {
            auto indata_str = bytespan_to_string(InData);
            OutData = "N" + Login(indata_str.substr(indata_str.find(':') + 1));
        }
        break;
    default:
        OutData.clear();
        break;
    }
    if (!OutData.empty() && CSocket.is_open()) {
        uint32_t DataSize = OutData.size();
        std::vector<char> ToSend(sizeof(DataSize) + OutData.size());
        std::copy_n(reinterpret_cast<char*>(&DataSize), sizeof(DataSize), ToSend.begin());
        std::copy_n(OutData.data(), OutData.size(), ToSend.begin() + sizeof(DataSize));
        asio::error_code ec;
        asio::write(CSocket, asio::buffer(ToSend), ec);
        if (ec) {
            debug(fmt::format("(Core) send failed with error: {}", ec.message()));
        }
    }
}
void GameHandler(asio::ip::tcp::socket& Client) {
    std::vector<char> data {};
    do {
        try {
            ReceiveFromGame(Client, data);
            Parse(data, Client);
        } catch (const std::exception& e) {
            error(std::string("Error while receiving from game: ") + e.what());
            break;
        }
    } while (true);
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

    asio::ip::tcp::endpoint listen_ep(asio::ip::address::from_string("0.0.0.0"), static_cast<uint16_t>(DEFAULT_PORT));
    asio::ip::tcp::socket LSocket(io);
    asio::error_code ec;
    LSocket.open(listen_ep.protocol(), ec);
    if (ec) {
        ::error(fmt::format("Failed to open core socket: {}", ec.message()));
        return;
    }
    asio::ip::tcp::socket::linger linger_opt {};
    linger_opt.enabled(false);
    LSocket.set_option(linger_opt, ec);
    if (ec) {
        ::error(fmt::format("Failed to set up listening core socket to not linger / reuse address. "
                            "This may cause the core socket to refuse to bind(). Error: {}",
            ec.message()));
        return;
    }
    asio::ip::tcp::socket::reuse_address reuse_opt { true };
    LSocket.set_option(reuse_opt, ec);
    if (ec) {
        ::error(fmt::format("Failed to set up listening core socket to not linger / reuse address. "
                            "This may cause the core socket to refuse to bind(). Error: {}",
            ec.message()));
        return;
    }

    auto acceptor = asio::ip::tcp::acceptor(io, listen_ep);
    acceptor.listen(asio::ip::tcp::socket::max_listen_connections, ec);
    if (ec) {
        ::error(fmt::format("listen() failed, which is needed for the launcher to operate. Error: {}",
            ec.message()));
        return;
    }

    do {
        auto CSocket = acceptor.accept(ec);
        if (ec) {
            error(fmt::format("(Core) accept failed with error: {}", ec.message()));
            continue;
        }
        localRes();
        info("Game Connected!");
        GameHandler(CSocket);
        warn("Game Reconnecting...");
    } while (LSocket.is_open());
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
