#include "ServerNetwork.h"
#include "ClientInfo.h"
#include "ImplementationInfo.h"
#include "Launcher.h"
#include "ProtocolVersion.h"
#include "ServerInfo.h"
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
    switch (m_state) {
    case bmp::State::None:
        m_state = bmp::State::Identification;
        [[fallthrough]];
    case bmp::State::Identification:
        handle_identification(packet);
        break;
    case bmp::State::Authentication:
        break;
    case bmp::State::ModDownload:
        break;
    case bmp::State::SessionSetup:
        break;
    case bmp::State::Playing:
        break;
    case bmp::State::Leaving:
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
