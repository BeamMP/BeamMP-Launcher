#include "ServerNetwork.h"
#include "ClientInfo.h"
#include "Identity.h"
#include "ImplementationInfo.h"
#include "Launcher.h"
#include "ProtocolVersion.h"
#include "ServerInfo.h"
#include "Transport.h"
#include "Util.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

ServerNetwork::ServerNetwork(Launcher& launcher, const ip::tcp::endpoint& ep)
    : m_launcher(launcher)
    , m_tcp_ep(ep) {
    spdlog::debug("Server network created");
}

ServerNetwork::~ServerNetwork() {
    spdlog::debug("Server network destroyed");
}

void ServerNetwork::run() {
    boost::system::error_code ec;
    m_tcp_socket.connect(m_tcp_ep, ec);
    if (ec) {
        spdlog::error("Failed to connect to [{}]:{}: {}", m_tcp_ep.address().to_string(), m_tcp_ep.port(), ec.message());
        throw std::runtime_error(ec.message());
    }
    // start the connection by going to the identification state and sending
    // the first packet in the identification protocol (protocol version)
    m_state = bmp::State::Identification;
    bmp::Packet version_packet {
        .purpose = bmp::Purpose::ProtocolVersion,
        .raw_data = std::vector<uint8_t>(6),
    };
    spdlog::trace("Protocol version: v{}.{}.{}", 1, 0, 0);
    struct bmp::ProtocolVersion version {
        .version = {
            .major = 1,
            .minor = 0,
            .patch = 0,
        },
    };
    version.serialize_to(version_packet.raw_data);
    tcp_write(version_packet);
    // main tcp recv loop
    while (true) {
        auto packet = tcp_read();
        handle_packet(packet);
    }
}

void ServerNetwork::handle_packet(const bmp::Packet& packet) {
    // handle ping immediately
    if (m_state > bmp::State::Identification && packet.purpose == bmp::Purpose::Ping) {
        bmp::Packet pong {
            .purpose = bmp::Purpose::Ping,
        };
        tcp_write(pong);
        return;
    }
    switch (m_state) {
    case bmp::State::None:
        m_state = bmp::State::Identification;
        [[fallthrough]];
    case bmp::State::Identification:
        handle_identification(packet);
        break;
    case bmp::State::Authentication:
        handle_authentication(packet);
        break;
    case bmp::State::ModDownload:
        handle_mod_download(packet);
        break;
    case bmp::State::SessionSetup:
        handle_session_setup(packet);
        break;
    case bmp::State::Playing:
        break;
    case bmp::State::Leaving:
        break;
    }
}

void ServerNetwork::handle_mod_download(const bmp::Packet& packet) {
    switch (packet.purpose) {
    case bmp::Purpose::ModsInfo: {
        auto data = packet.get_readable_data();
        auto mods_info = nlohmann::json::parse(data.begin(), data.end());
        spdlog::info("Got info about {} mod(s)", mods_info.size());
        for (const auto& mod : mods_info) {
            spdlog::warn("Mod download not implemented, but data is: {}", mod.dump(4));
        }
        // TODO: implement mod download
        // for now we just pretend we're all good!
        bmp::Packet ok {
            .purpose = bmp::Purpose::ModsSyncDone,
        };
        spdlog::info("Done syncing mods");
        tcp_write(ok);
        break;
    }
    case bmp::Purpose::MapInfo: {
        auto data = packet.get_readable_data();
        std::string map(data.begin(), data.end());
        // TODO: Send to game
        spdlog::debug("Map: '{}'", map);
        break;
    }
    case bmp::Purpose::StateChangeSessionSetup: {
        spdlog::info("Starting session setup");
        m_state = bmp::State::SessionSetup;
        break;
    }
    default:
        spdlog::error("Got 0x{:x} in state {}. This is not allowed. Disconnecting", uint16_t(packet.purpose), int(m_state));
        // todo: disconnect gracefully
        break;
    }
}

void ServerNetwork::handle_authentication(const bmp::Packet& packet) {
    switch (packet.purpose) {
    case bmp::Purpose::AuthOk: {
        spdlog::info("Authentication succeeded");
        uint32_t player_id;
        bmp::deserialize(player_id, packet.get_readable_data());
        spdlog::debug("Player id: {}", player_id);
        break;
    }
    case bmp::Purpose::AuthFailed: {
        auto data = packet.get_readable_data();
        spdlog::error("Authentication failed: {}", std::string(data.begin(), data.end()));
        break;
    }
    case bmp::Purpose::PlayerRejected: {
        auto data = packet.get_readable_data();
        spdlog::error("Server rejected me: {}", std::string(data.begin(), data.end()));
        break;
    }
    case bmp::Purpose::StartUDP: {
        bmp::deserialize(m_udp_magic, packet.get_readable_data());
        spdlog::debug("UDP start, got magic 0x{:x}", m_udp_magic);
        m_udp_ep = ip::udp::endpoint(m_tcp_ep.address(), m_tcp_ep.port());
        m_udp_socket.open(m_tcp_ep.address().is_v4() ? ip::udp::v4() : ip::udp::v6());
        auto copy = bmp::Packet {
            .purpose = bmp::Purpose::StartUDP,
            .raw_data = packet.get_readable_data(),
        };
        udp_write(copy);
        break;
    }
    case bmp::Purpose::StateChangeModDownload: {
        spdlog::info("Starting mod sync");
        m_state = bmp::State::ModDownload;
        break;
    }
    default:
        spdlog::error("Got 0x{:x} in state {}. This is not allowed. Disconnecting", uint16_t(packet.purpose), int(m_state));
        // todo: disconnect gracefully
        break;
    }
}

void ServerNetwork::handle_identification(const bmp::Packet& packet) {
    switch (packet.purpose) {
    case bmp::Purpose::ProtocolVersionOk: {
        spdlog::debug("Protocol version ok");
        bmp::Packet ci_packet {
            .purpose = bmp::Purpose::ClientInfo,
            .raw_data = std::vector<uint8_t>(1024),
        };
        // TODO: Game and mod version
        struct bmp::ClientInfo ci {
            .program_version = { .major = PRJ_VERSION_MAJOR, .minor = PRJ_VERSION_MINOR, .patch = PRJ_VERSION_PATCH },
            .game_version = { .major = 4, .minor = 5, .patch = 6 },
            .mod_version = { .major = 1, .minor = 2, .patch = 3 },
            .implementation = bmp::ImplementationInfo {
                .value = "Official BeamMP Launcher",
            },
        };
        auto sz = ci.serialize_to(ci_packet.raw_data);
        ci_packet.raw_data.resize(sz);
        tcp_write(ci_packet);
        break;
    }
    case bmp::Purpose::ProtocolVersionBad:
        spdlog::error("The server rejected our protocol version!");
        throw std::runtime_error("Protocol version rejected");
        break;
    case bmp::Purpose::ServerInfo: {
        struct bmp::ServerInfo si { };
        si.deserialize_from(packet.get_readable_data());
        spdlog::debug("Connected to server implementation '{}' v{}.{}.{}",
            si.implementation.value,
            si.program_version.major,
            si.program_version.minor,
            si.program_version.patch);
        break;
    }
    case bmp::Purpose::StateChangeAuthentication: {
        spdlog::debug("Starting authentication");
        m_state = bmp::State::Authentication;
        // TODO: make the launcher provide login properly!
        auto pubkey = m_launcher.get_public_key();
        bmp::Packet pubkey_packet {
            .purpose = bmp::Purpose::PlayerPublicKey,
            .raw_data = std::vector<uint8_t>(pubkey.begin(), pubkey.end())
        };
        tcp_write(pubkey_packet);
        break;
    }
    default:
        spdlog::error("Got 0x{:x} in state {}. This is not allowed. Disconnecting", uint16_t(packet.purpose), int(m_state));
        // todo: disconnect gracefully
        break;
    }
}

bmp::Packet ServerNetwork::tcp_read() {
    bmp::Packet packet {};
    std::vector<uint8_t> header_buffer(bmp::Header::SERIALIZED_SIZE);
    read(m_tcp_socket, buffer(header_buffer));
    bmp::Header hdr {};
    hdr.deserialize_from(header_buffer);
    // vector eaten up by now, recv again
    packet.raw_data.resize(hdr.size);
    read(m_tcp_socket, buffer(packet.raw_data));
    packet.purpose = hdr.purpose;
    packet.flags = hdr.flags;
    return packet;
}

void ServerNetwork::tcp_write(bmp::Packet& packet) {
    // finalize the packet (compress etc) and produce header
    auto header = packet.finalize();
    // serialize header
    std::vector<uint8_t> header_data(bmp::Header::SERIALIZED_SIZE);
    header.serialize_to(header_data);
    // write header and packet data
    write(m_tcp_socket, buffer(header_data));
    write(m_tcp_socket, buffer(packet.raw_data));
}

bmp::Packet ServerNetwork::udp_read(ip::udp::endpoint& out_ep) {
    // maximum we can ever expect from udp
    static thread_local std::vector<uint8_t> s_buffer(std::numeric_limits<uint16_t>::max());
    m_udp_socket.receive_from(buffer(s_buffer), out_ep, {});
    bmp::Packet packet;
    bmp::Header header {};
    auto offset = header.deserialize_from(s_buffer);
    packet.raw_data.resize(header.size);
    std::copy(s_buffer.begin() + offset, s_buffer.begin() + offset + header.size, packet.raw_data.begin());
    return packet;
}

void ServerNetwork::udp_write(bmp::Packet& packet) {
    auto header = packet.finalize();
    std::vector<uint8_t> data(header.size + bmp::Header::SERIALIZED_SIZE);
    auto offset = header.serialize_to(data);
    std::copy(packet.raw_data.begin(), packet.raw_data.end(), data.begin() + static_cast<long>(offset));
    m_udp_socket.send_to(buffer(data), m_udp_ep, {});
}
void ServerNetwork::handle_session_setup(const bmp::Packet& packet) {
    switch (packet.purpose) {
    case bmp::Purpose::PlayersVehiclesInfo: {
        spdlog::debug("Players and vehicles info: {} bytes ({} bytes on arrival)", packet.get_readable_data().size(), packet.raw_data.size());
        // TODO: Send to game
        break;
    }
    default:
        spdlog::error("Got 0x{:x} in state {}. This is not allowed. Disconnecting", uint16_t(packet.purpose), int(m_state));
        // todo: disconnect gracefully
        break;
    }
}
