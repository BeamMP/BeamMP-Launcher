#pragma once

#include "ClientPacket.h"
#include "ClientState.h"
#include "Launcher.h"
#include "Packet.h"
#include "Sync.h"
#include "Version.h"

#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

using namespace boost::asio;

class ClientNetwork {
public:
    ClientNetwork(Launcher& launcher, uint16_t port);

    ~ClientNetwork();

    void run();

    void handle_server_packet(bmp::Packet&& packet);

private:
    void start_accept();
    void handle_accept(boost::system::error_code ec);

    void start_read();

    void handle_connection();
    void client_tcp_read(std::function<void(bmp::ClientPacket&&)> handler);
    void client_tcp_write(bmp::ClientPacket&& packet, std::function<void(boost::system::error_code)> handler = nullptr);

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
    void start_quick_join();
    void start_browsing();

    Result<std::vector<uint8_t>, std::string> load_server_list() noexcept;

    static std::vector<uint8_t> json_to_vec(const nlohmann::json& json);
    static nlohmann::json vec_to_json(const std::vector<uint8_t>& vec);

    Sync<ident::Identity> m_identity {};

    uint16_t m_listen_port {};
    io_context m_io {};
    ip::tcp::socket m_game_socket { m_io };
    Sync<bool> m_shutdown { false };
    bmp::ClientState m_client_state;

    ip::tcp::acceptor m_acceptor { m_io };
    boost::asio::strand<ip::tcp::socket::executor_type> m_strand { m_game_socket.get_executor() };

    // temporary packet and header buffer for async reads
    bmp::ClientPacket m_tmp_packet {};
    std::vector<uint8_t> m_tmp_header_buffer {};

    Launcher& launcher;
};
