#pragma once

#include "ClientPacket.h"
#include "ClientState.h"
#include "Launcher.h"
#include "Sync.h"
#include "Version.h"

#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

using namespace boost::asio;

class ClientNetwork {
public:
    ClientNetwork(uint16_t port);

    ~ClientNetwork();

    void run();

private:
    void handle_connection(ip::tcp::socket&& socket);
    bmp::ClientPacket client_tcp_read(ip::tcp::socket& socket);
    void client_tcp_write(ip::tcp::socket& socket, bmp::ClientPacket& packet);

    void handle_packet(ip::tcp::socket& socket, bmp::ClientPacket& packet);
    void handle_client_identification(ip::tcp::socket& socket, bmp::ClientPacket& packet);
    void handle_login(ip::tcp::socket& socket, bmp::ClientPacket& packet);
    void handle_quick_join(ip::tcp::socket& socket, bmp::ClientPacket& packet);
    void handle_browsing(ip::tcp::socket& socket, bmp::ClientPacket& packet);
    void handle_server_identification(ip::tcp::socket& socket, bmp::ClientPacket& packet);
    void handle_server_authentication(ip::tcp::socket& socket, bmp::ClientPacket& packet);
    void handle_server_mod_download(ip::tcp::socket& socket, bmp::ClientPacket& packet);
    void handle_server_session_setup(ip::tcp::socket& socket, bmp::ClientPacket& packet);
    void handle_server_playing(ip::tcp::socket& socket, bmp::ClientPacket& packet);
    void handle_server_leaving(ip::tcp::socket& socket, bmp::ClientPacket& packet);

    void disconnect(ip::tcp::socket& socket, const std::string& reason);

    static std::vector<uint8_t> json_to_vec(const nlohmann::json& json);
    static nlohmann::json vec_to_json(const std::vector<uint8_t>& vec);

    Version m_mod_version;
    Version m_game_version;

    uint16_t m_listen_port {};
    io_context m_io {};
    Sync<bool> m_shutdown { false };
    bmp::ClientState m_client_state;
};
