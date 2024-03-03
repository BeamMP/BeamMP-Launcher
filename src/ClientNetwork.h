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
    bmp::ClientPacket client_tcp_read();
    void client_tcp_write(bmp::ClientPacket& packet);

    void handle_packet(bmp::ClientPacket& packet);
    void handle_client_identification(bmp::ClientPacket& packet);
    void handle_login(bmp::ClientPacket& packet);
    void handle_quick_join(bmp::ClientPacket& packet);
    void handle_browsing(bmp::ClientPacket& packet);
    void handle_server_identification(bmp::ClientPacket& packet);
    void handle_server_authentication(bmp::ClientPacket& packet);
    void handle_server_mod_download(bmp::ClientPacket& packet);
    void handle_server_session_setup(bmp::ClientPacket& packet);
    void handle_server_playing(bmp::ClientPacket& packet);
    void handle_server_leaving(bmp::ClientPacket& packet);

    void disconnect(const std::string& reason);
    void start_login();

    static std::vector<uint8_t> json_to_vec(const nlohmann::json& json);
    static nlohmann::json vec_to_json(const std::vector<uint8_t>& vec);

    Version m_mod_version;
    Version m_game_version;

    ident::Identity m_identity {};

    uint16_t m_listen_port {};
    io_context m_io {};
    ip::tcp::socket m_game_socket { m_io };
    Sync<bool> m_shutdown { false };
    bmp::ClientState m_client_state;
};
