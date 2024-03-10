#include "ServerNetwork.h"
#include "ClientInfo.h"
#include "ClientNetwork.h"
#include "ClientPacket.h"
#include "ClientTransport.h"
#include "Identity.h"
#include "ImplementationInfo.h"
#include "Launcher.h"
#include "Packet.h"
#include "ProtocolVersion.h"
#include "ServerInfo.h"
#include "Transport.h"
#include "Util.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

ServerNetwork::ServerNetwork(Launcher& launcher, const ip::tcp::endpoint& ep)
    : m_tcp_ep(ep)
    , launcher(launcher) {
    spdlog::debug("Server network created");
    boost::system::error_code ec;
    m_tcp_socket.connect(m_tcp_ep, ec);
    if (ec) {
        throw std::runtime_error(ec.message());
    }
}

ServerNetwork::~ServerNetwork() {
    spdlog::debug("Server network destroyed");
}

void ServerNetwork::run() {
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
    tcp_write(std::move(version_packet));

    start_tcp_read();

    m_io.run();
}

void ServerNetwork::handle_packet(bmp::Packet&& packet) {
    // handle ping immediately
    if (m_state > bmp::State::Identification && packet.purpose == bmp::Purpose::Ping) {
        tcp_write(bmp::Packet {
            .purpose = bmp::Purpose::Ping,
        });
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
        handle_playing(packet);
        break;
    case bmp::State::Leaving:
        break;
    }

    launcher.client_network->handle_server_packet(std::move(packet));
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
        tcp_write(bmp::Packet {
            .purpose = bmp::Purpose::ModsSyncDone,
        });
        spdlog::info("Done syncing mods");
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
        struct bmp::ClientInfo ci {
            .program_version = { .major = PRJ_VERSION_MAJOR, .minor = PRJ_VERSION_MINOR, .patch = PRJ_VERSION_PATCH },
            .game_version = {
                .major = launcher.game_version->major,
                .minor = launcher.game_version->minor,
                .patch = launcher.game_version->patch,
            },
            .mod_version = {
                .major = launcher.mod_version->major,
                .minor = launcher.mod_version->minor,
                .patch = launcher.mod_version->patch,
            },
            .implementation = bmp::ImplementationInfo {
                .value = "Official BeamMP Launcher",
            },
        };
        auto sz = ci.serialize_to(ci_packet.raw_data);
        ci_packet.raw_data.resize(sz);
        tcp_write(std::move(ci_packet));
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
        auto ident = launcher.identity.synchronize();
        tcp_write(bmp::Packet {
            .purpose = bmp::Purpose::PlayerPublicKey,
            .raw_data = std::vector<uint8_t>(ident->PublicKey.begin(), ident->PublicKey.end()) });
        break;
    }
    default:
        spdlog::error("Got 0x{:x} in state {}. This is not allowed. Disconnecting", uint16_t(packet.purpose), int(m_state));
        // todo: disconnect gracefully
        break;
    }
}

void ServerNetwork::tcp_read(std::function<void(bmp::Packet&&)> handler) {
    m_tmp_header_buffer.resize(bmp::Header::SERIALIZED_SIZE);
    boost::asio::async_read(m_tcp_socket, buffer(m_tmp_header_buffer),
        [this, handler](auto ec, auto) {
            if (ec) {
                spdlog::error("Failed to read from server: {}", ec.message());
            } else {
                bmp::Header hdr {};
                hdr.deserialize_from(m_tmp_header_buffer);
                // vector eaten up by now, recv again
                m_tmp_packet.raw_data.resize(hdr.size);
                m_tmp_packet.purpose = hdr.purpose;
                m_tmp_packet.flags = hdr.flags;
                boost::asio::async_read(m_tcp_socket, buffer(m_tmp_packet.raw_data),
                    [handler, this](auto ec, auto) {
                        if (ec) {
                            spdlog::error("Failed to read from server: {}", ec.message());
                        } else {
                            // ok!
                            handler(std::move(m_tmp_packet));
                        }
                    });
            }
        });
}

void ServerNetwork::tcp_write(bmp::Packet&& packet, std::function<void(boost::system::error_code)> handler) {
    // finalize the packet (compress etc) and produce header
    auto header = packet.finalize();

    auto owned_packet = std::make_shared<bmp::Packet>(std::move(packet));
    // serialize header
    auto header_data = std::make_shared<std::vector<uint8_t>>(bmp::Header::SERIALIZED_SIZE);
    header.serialize_to(*header_data);
    std::array<const_buffer, 2> buffers = {
        buffer(*header_data),
        buffer(owned_packet->raw_data)
    };
    boost::asio::async_write(m_tcp_socket, buffers,
        [this, header_data, owned_packet, handler](auto ec, auto size) {
            spdlog::trace("Wrote {} bytes for 0x{:x} to server", size, int(owned_packet->purpose));
            if (handler) {
                handler(ec);
            } else {
                if (ec) {
                    spdlog::error("Failed to send packet of type 0x{:x} to server", int(owned_packet->purpose));
                } else {
                    // ok!
                    spdlog::trace("Sent packet of type 0x{:x} to server", int(owned_packet->purpose));
                }
            }
        });
}

void ServerNetwork::udp_read(std::function<void(ip::udp::endpoint&&, bmp::Packet&&)> handler) {
    // maximum we can ever expect from udp
    auto ep = std::make_shared<ip::udp::endpoint>();
    m_udp_socket.async_receive_from(buffer(m_tmp_udp_buffer), *ep, [this, ep, handler](auto ec, auto) {
        if (ec) {
            spdlog::error("Failed to receive UDP from server: {}", ec.message());
        } else {
            bmp::Packet packet;
            bmp::Header header {};
            auto offset = header.deserialize_from(m_tmp_udp_buffer);
            packet.raw_data.resize(header.size);
            std::copy(m_tmp_udp_buffer.begin() + long(offset), m_tmp_udp_buffer.begin() + long(offset) + header.size, packet.raw_data.begin());
            handler(std::move(*ep), std::move(packet));
        }
    });
}

void ServerNetwork::udp_write(bmp::Packet& packet) {
    auto header = packet.finalize();
    auto data = std::make_shared<std::vector<uint8_t>>(header.size + bmp::Header::SERIALIZED_SIZE);
    auto offset = header.serialize_to(*data);
    std::copy(packet.raw_data.begin(), packet.raw_data.end(), data->begin() + static_cast<long>(offset));
    m_udp_socket.async_send_to(buffer(*data), m_udp_ep, [data](auto ec, auto size) {
        if (ec) {
            spdlog::error("Failed to UDP write to server: {}", ec.message());
        } else {
            spdlog::trace("Wrote {} bytes to server via UDP", size);
        }
    });
}
void ServerNetwork::handle_session_setup(const bmp::Packet& packet) {
    switch (packet.purpose) {
    case bmp::Purpose::PlayersVehiclesInfo: {
        spdlog::debug("Players and vehicles info: {} bytes ({} bytes on arrival)", packet.get_readable_data().size(), packet.raw_data.size());
        // TODO: Send to game
        tcp_write(bmp::Packet {
            .purpose = bmp::Purpose::SessionReady,
        });
        break;
    }
    case bmp::Purpose::StateChangePlaying: {
        spdlog::info("Playing!");
        break;
    }
    default:
        spdlog::error("Got 0x{:x} in state {}. This is not allowed. Disconnecting", uint16_t(packet.purpose), int(m_state));
        // todo: disconnect gracefully
        break;
    }
}

void ServerNetwork::handle_playing(const bmp::Packet& packet) {
    switch (packet.purpose) {
    default:
        spdlog::error("Got 0x{:x} in state {}. This is not allowed. Disconnecting", uint16_t(packet.purpose), int(m_state));
        // todo: disconnect gracefully
        break;
    }
}

void ServerNetwork::start_tcp_read() {
    tcp_read([this](auto&& packet) {
        spdlog::debug("Got packet 0x{:x} from server", int(packet.purpose));
        handle_packet(std::move(packet));
        start_tcp_read();
    });
}

void ServerNetwork::start_udp_read() {
    udp_read([this](auto&& ep, auto&& packet) {
        spdlog::debug("Got packet 0x{:x} from server via UDP", int(packet.purpose));
        handle_packet(std::move(packet));
        start_udp_read();
    });
}
