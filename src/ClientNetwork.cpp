#include "ClientNetwork.h"
#include "ClientPacket.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

void ClientNetwork::run() {
    ip::tcp::endpoint listen_ep(ip::address::from_string("0.0.0.0"), static_cast<uint16_t>(m_listen_port));
    ip::tcp::socket listener(m_io);
    boost::system::error_code ec;
    listener.open(listen_ep.protocol(), ec);
    if (ec) {
        spdlog::error("Failed to open socket: {}", ec.message());
        return;
    }
    socket_base::linger linger_opt {};
    linger_opt.enabled(false);
    listener.set_option(linger_opt, ec);
    if (ec) {
        spdlog::error("Failed to set up listening socket to not linger / reuse address. "
                      "This may cause the socket to refuse to bind(). spdlog::error: {}",
            ec.message());
    }

    ip::tcp::acceptor acceptor(m_io, listen_ep);
    acceptor.listen(socket_base::max_listen_connections, ec);
    if (ec) {
        spdlog::error("listen() failed, which is needed for the launcher to operate. "
                      "Shutting down. spdlog::error: {}",
            ec.message());
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::exit(1);
    }
    do {
        try {
            spdlog::info("Waiting for the game to connect");
            ip::tcp::endpoint game_ep;
            auto game = acceptor.accept(game_ep, ec);
            if (ec) {
                spdlog::error("Failed to accept: {}", ec.message());
            }
            spdlog::info("Game connected!");
            spdlog::debug("Game: [{}]:{}", game_ep.address().to_string(), game_ep.port());
            handle_connection(std::move(game));
            spdlog::warn("Game disconnected!");
        } catch (const std::exception& e) {
            spdlog::error("Fatal error in core network: {}", e.what());
        }
    } while (!*m_shutdown);
}

ClientNetwork::ClientNetwork(uint16_t port)
    : m_listen_port(port) {
    spdlog::debug("Client network created");
}

ClientNetwork::~ClientNetwork() {
    spdlog::debug("Client network destroyed");
}

void ClientNetwork::handle_connection(ip::tcp::socket&& socket) {
    // immediately send launcher info (first step of client identification)
    m_client_state = bmp::ClientState::ClientIdentification;

    bmp::ClientPacket info_packet {};
    info_packet.purpose = bmp::ClientPurpose::LauncherInfo;
    info_packet.raw_data = json_to_vec(
        {
            { "implementation", "Official BeamMP Launcher" },
            { "version", { PRJ_VERSION_MAJOR, PRJ_VERSION_MINOR, PRJ_VERSION_PATCH } },
            { "mod_cache_path", "/idk sorry/" }, // TODO: mod_cache_path in LauncherInfo
        });
    client_tcp_write(socket, info_packet);

    // packet read and respond loop
    while (!*m_shutdown) {
        auto packet = client_tcp_read(socket);
        handle_packet(socket, packet);
    }
}

void ClientNetwork::handle_packet(ip::tcp::socket& socket, bmp::ClientPacket& packet) {
    spdlog::trace("Got client packet: purpose: 0x{:x}, flags: 0x{:x}, pid: {}, vid: {}, size: {}",
        uint16_t(packet.purpose),
        uint8_t(packet.flags),
        packet.pid, packet.vid,
        packet.get_readable_data().size());
    spdlog::trace("Client State: 0x{:x}", int(m_client_state));

    switch (m_client_state) {
    case bmp::ClientIdentification:
        handle_client_identification(socket, packet);
        break;
    case bmp::Login:
        handle_login(socket, packet);
        break;
    case bmp::QuickJoin:
        handle_quick_join(socket, packet);
        break;
    case bmp::Browsing:
        handle_browsing(socket, packet);
        break;
    case bmp::ServerIdentification:
        handle_server_identification(socket, packet);
        break;
    case bmp::ServerAuthentication:
        handle_server_authentication(socket, packet);
        break;
    case bmp::ServerModDownload:
        handle_server_mod_download(socket, packet);
        break;
    case bmp::ServerSessionSetup:
        handle_server_session_setup(socket, packet);
        break;
    case bmp::ServerPlaying:
        handle_server_playing(socket, packet);
        break;
    case bmp::ServerLeaving:
        handle_server_leaving(socket, packet);
        break;
    }
}

void ClientNetwork::handle_client_identification(ip::tcp::socket& socket, bmp::ClientPacket& packet) {
}

void ClientNetwork::handle_login(ip::tcp::socket& socket, bmp::ClientPacket& packet) {
}

void ClientNetwork::handle_quick_join(ip::tcp::socket& socket, bmp::ClientPacket& packet) {
}

void ClientNetwork::handle_browsing(ip::tcp::socket& socket, bmp::ClientPacket& packet) {
}

void ClientNetwork::handle_server_identification(ip::tcp::socket& socket, bmp::ClientPacket& packet) {
}

void ClientNetwork::handle_server_authentication(ip::tcp::socket& socket, bmp::ClientPacket& packet) {
}

void ClientNetwork::handle_server_mod_download(ip::tcp::socket& socket, bmp::ClientPacket& packet) {
}

void ClientNetwork::handle_server_session_setup(ip::tcp::socket& socket, bmp::ClientPacket& packet) {
}

void ClientNetwork::handle_server_playing(ip::tcp::socket& socket, bmp::ClientPacket& packet) {
}

void ClientNetwork::handle_server_leaving(ip::tcp::socket& socket, bmp::ClientPacket& packet) {
}

bmp::ClientPacket ClientNetwork::client_tcp_read(ip::tcp::socket& socket) {
    bmp::ClientPacket packet {};
    std::vector<uint8_t> header_buffer(bmp::ClientHeader::SERIALIZED_SIZE);
    read(socket, buffer(header_buffer));
    bmp::ClientHeader hdr {};
    hdr.deserialize_from(header_buffer);
    // vector eaten up by now, recv again
    packet.raw_data.resize(hdr.data_size);
    read(socket, buffer(packet.raw_data));
    packet.purpose = hdr.purpose;
    packet.flags = hdr.flags;
    return packet;
}

void ClientNetwork::client_tcp_write(ip::tcp::socket& socket, bmp::ClientPacket& packet) {
    // finalize the packet (compress etc) and produce header
    auto header = packet.finalize();
    // serialize header
    std::vector<uint8_t> header_data(bmp::ClientHeader::SERIALIZED_SIZE);
    header.serialize_to(header_data);
    // write header and packet data
    write(socket, buffer(header_data));
    write(socket, buffer(packet.raw_data));
}
std::vector<uint8_t> ClientNetwork::json_to_vec(const nlohmann::json& value) {
    auto str = value.dump();
    return std::vector<uint8_t>(str.begin(), str.end());
}
