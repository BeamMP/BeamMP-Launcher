#pragma once

#include "Config.h"
#include "Identity.h"
#include "Sync.h"
#include "Version.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <filesystem>
#include <set>
#include <string>
#include <thread>

class ClientNetwork;
class ServerNetwork;

class Launcher {
public:
    Launcher();
    ~Launcher();

    void check_for_updates(int argc, char** argv);

    void set_exe_name(const std::string& name) { m_exe_name = name; }
    void set_exe_path(const std::filesystem::path& path) { m_exe_path = path; }

    void find_game();

    void pre_game();

    void start_game();

    std::string get_public_key();

    Sync<ident::Identity> identity {};

    Result<void, std::string> start_server_network(const std::string& host, uint16_t port);

    Sync<Version> mod_version {};
    Sync<Version> game_version {};

    std::unique_ptr<ClientNetwork> client_network {};
    std::unique_ptr<ServerNetwork> server_network {};

private:
    /// Thread main function for the http(s) proxy thread.
    void proxy_main();

    /// Thread main function for the game thread.
    void game_main();

    void parse_config();

    static void check_mp(const std::string& path);

    void enable_mp();

    void try_auto_login();

    Sync<bool> m_mod_loaded { false };

    Sync<Config> m_config;

    boost::scoped_thread<> m_proxy_thread { &Launcher::proxy_main, this };
    boost::scoped_thread<> m_game_thread;
    boost::scoped_thread<> m_client_network_thread;
    boost::scoped_thread<> m_server_network_thread;

    Sync<std::string> m_exe_name;
    Sync<std::filesystem::path> m_exe_path;
    boost::asio::io_context m_io {};
    Sync<bool> m_shutdown { false };
};
