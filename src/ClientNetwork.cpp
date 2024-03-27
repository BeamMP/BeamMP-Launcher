#include "ClientNetwork.h"
#include "ClientPacket.h"
#include "ClientTransport.h"
#include "Http.h"
#include "Identity.h"
#include "Launcher.h"
#include "Transport.h"
#include "Util.h"

#include <boost/asio/socket_base.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

void ClientNetwork::run() {
    ip::tcp::endpoint listen_ep(ip::address::from_string("0.0.0.0"), static_cast<uint16_t>(m_listen_port));
    ip::tcp::socket listener(m_io);
    boost::system::error_code ec;
    listener.open(listen_ep.protocol(), ec);
    if (ec) {
        spdlog::error("Failed to open m_game_socket: {}", ec.message());
        return;
    }
    ip::tcp::socket::linger linger_opt {};
    linger_opt.enabled(false);
    listener.set_option(linger_opt, ec);
    if (ec) {
        spdlog::error("Failed to set up listening m_game_socket to not linger / reuse address. "
                      "This may cause the m_game_socket to refuse to bind(). spdlog::error: {}",
            ec.message());
    }

    m_acceptor = ip::tcp::acceptor(m_io, listen_ep);
    m_acceptor.listen(ip::tcp::socket::max_listen_connections, ec);
    if (ec) {
        spdlog::error("listen() failed, which is needed for the launcher to operate. "
                      "Shutting down. spdlog::error: {}",
            ec.message());
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::exit(1);
    }
    start_accept();
    m_io.run();
}

ClientNetwork::ClientNetwork(Launcher& launcher, uint16_t port)
    : m_listen_port(port)
    , launcher(launcher) {
    spdlog::debug("Client network created");
}

ClientNetwork::~ClientNetwork() {
    spdlog::debug("Client network destroyed");
}

void ClientNetwork::start_read() {
    client_tcp_read([this](auto&& packet) {
        handle_packet(packet);
        start_read();
    });
}

void ClientNetwork::handle_connection() {
    // immediately send launcher info (first step of client identification)
    m_client_state = bmp::ClientState::ClientIdentification;

    client_tcp_write(bmp::ClientPacket {
        .purpose = bmp::ClientPurpose::LauncherInfo,
        .raw_data = json_to_vec(
            {
                { "implementation", "Official BeamMP Launcher" },
                { "version", { PRJ_VERSION_MAJOR, PRJ_VERSION_MINOR, PRJ_VERSION_PATCH } },
                { "mod_cache_path", "/idk sorry/" }, // TODO: mod_cache_path in LauncherInfo
            }) });

    start_read();
}

void ClientNetwork::handle_packet(bmp::ClientPacket& packet) {
    spdlog::debug("Got client packet: purpose: 0x{:x}, flags: 0x{:x}, pid: {}, vid: {}, size: {}",
        uint16_t(packet.purpose),
        uint8_t(packet.flags),
        packet.pid, packet.vid,
        packet.get_readable_data().size());
    spdlog::debug("Client State: 0x{:x}", int(m_client_state));

    switch (m_client_state) {
    case bmp::ClientIdentification:
        handle_client_identification(packet);
        break;
    case bmp::Login:
        handle_login(packet);
        break;
    case bmp::QuickJoin:
        handle_quick_join(packet);
        break;
    case bmp::Browsing:
        handle_browsing(packet);
        break;
    case bmp::ServerIdentification:
        handle_server_identification(packet);
        break;
    case bmp::ServerAuthentication:
        handle_server_authentication(packet);
        break;
    case bmp::ServerModDownload:
        handle_server_mod_download(packet);
        break;
    case bmp::ServerSessionSetup:
        handle_server_session_setup(packet);
        break;
    case bmp::ServerPlaying:
        handle_server_playing(packet);
        break;
    case bmp::ServerLeaving:
        handle_server_leaving(packet);
        break;
    }
}

void ClientNetwork::handle_client_identification(bmp::ClientPacket& packet) {
    switch (packet.purpose) {
    case bmp::ClientPurpose::GameInfo: {
        try {
            auto game_info = vec_to_json(packet.get_readable_data());
            std::string impl = game_info.at("implementation");
            std::vector<int> mod_version = game_info.at("mod_version");
            std::vector<int> game_version = game_info.at("game_version");
            std::vector<int> protocol_version = game_info.at("protocol_version");
            if (protocol_version.at(0) != 1) {
                disconnect(fmt::format("Incompatible protocol version, expected v{}", "1.x.x"));
                return;
            }
            *launcher.mod_version = Version { uint8_t(mod_version.at(0)), uint8_t(mod_version.at(1)), uint8_t(mod_version.at(2)) };
            *launcher.game_version = Version { uint8_t(game_version.at(0)), uint8_t(game_version.at(1)), uint8_t(game_version.at(2)) };
            spdlog::info("Connected to {} (mod v{}, game v{}, protocol v{}.{}.{})",
                impl,
                launcher.mod_version->to_string(),
                launcher.game_version->to_string(),
                protocol_version.at(0),
                protocol_version.at(1), protocol_version.at(2));
        } catch (const std::exception& e) {
            spdlog::error("Failed to read json for purpose 0x{:x}: {}", uint16_t(packet.purpose), e.what());
            disconnect(fmt::format("Invalid json in purpose 0x{:x}, see launcher logs for more info", uint16_t(packet.purpose)));
        }
        start_login();
        break;
    }
    default:
        disconnect(fmt::format("Invalid packet purpose in state 0x{:x}: 0x{:x}", uint16_t(m_client_state), uint16_t(packet.purpose)));
        break;
    }
}

void ClientNetwork::start_login() {
    client_tcp_write(bmp::ClientPacket {
        .purpose = bmp::ClientPurpose::StateChangeLogin,
    });

    m_client_state = bmp::ClientState::Login;

    if (ident::is_login_cached()) {
        auto login = ident::login_cached();
        // case in which the login is cached and login was successful:
        // we just send the login result and continue to the next state.
        if (login.has_value()) {
            *m_identity = login.value();
            client_tcp_write(bmp::ClientPacket {
                .purpose = bmp::ClientPurpose::LoginResult,
                .raw_data = json_to_vec({
                    { "success", true },
                    { "message", m_identity->Message },
                    { "username", m_identity->Username },
                    { "role", m_identity->Role },
                }),
            });
            // move on to quick join right away
            start_quick_join();
            return; // done
        } else {
            spdlog::warn("Failed to automatically login with cached/saved login details: {}", login.error());
            spdlog::info("Trying normal login");
        }
        // fallthrough to normal login
    }

    // first packet in login
    // TODO: send LoginResult if already logged in.
    client_tcp_write(bmp::ClientPacket {
        .purpose = bmp::ClientPurpose::AskForCredentials,
    });
    // wait for response, so return
}

void ClientNetwork::disconnect(const std::string& reason) {
    spdlog::debug("Disconnecting game: {}", reason);
    client_tcp_write(bmp::ClientPacket {
                         .purpose = bmp::ClientPurpose::Error,
                         .raw_data = json_to_vec({ "message", reason }) },
        [this](auto ec) {
            // ignore any error and just shut down
            // we pass `ec` to both of these and ignore their errors as well
            // because we dont care if they fail
            m_game_socket.shutdown(boost::asio::socket_base::shutdown_both, ec);
            m_game_socket.close(ec);
            start_accept();
        });
}

void ClientNetwork::handle_login(bmp::ClientPacket& packet) {
    switch (packet.purpose) {
    case bmp::ClientPurpose::Credentials:
        try {
            auto creds = vec_to_json(packet.get_readable_data());
            std::string username = creds.at("username");
            std::string password = creds.at("password");
            bool remember = creds.at("remember");
            spdlog::debug("Got credentials username: '{}', password: ({} chars) (remember: {})", username, password.size(), remember ? "yes" : "no");
            // login!
            auto result = ident::login(username, password, remember);
            if (result.has_error()) {
                ;
                client_tcp_write(bmp::ClientPacket {
                    .purpose = bmp::ClientPurpose::LoginResult,
                    .raw_data = json_to_vec({
                        { "success", false },
                        { "message", result.error() },
                    }),
                });
                client_tcp_write(bmp::ClientPacket {
                    .purpose = bmp::ClientPurpose::AskForCredentials,
                });
                return;
            }
            *m_identity = result.value();
            client_tcp_write(bmp::ClientPacket {
                .purpose = bmp::ClientPurpose::LoginResult,
                .raw_data = json_to_vec({
                    { "success", true },
                    { "message", m_identity->Message },
                    { "username", m_identity->Username },
                    { "role", m_identity->Role },
                }),
            });

            start_quick_join();
        } catch (const std::exception& e) {
            spdlog::error("Failed to read json for purpose 0x{:x}: {}", uint16_t(packet.purpose), e.what());
            disconnect(fmt::format("Invalid json in purpose 0x{:x}, see launcher logs for more info", uint16_t(packet.purpose)));
        }
        break;
    default:
        disconnect(fmt::format("Invalid packet purpose in state 0x{:x}: 0x{:x}", uint16_t(m_client_state), uint16_t(packet.purpose)));
        break;
    }
}

void ClientNetwork::handle_quick_join(bmp::ClientPacket& packet) {
    switch (packet.purpose) {
    default:
        disconnect(fmt::format("Invalid packet purpose in state 0x{:x}: 0x{:x}", uint16_t(m_client_state), uint16_t(packet.purpose)));
        break;
    }
}

void ClientNetwork::handle_browsing(bmp::ClientPacket& packet) {
    switch (packet.purpose) {
    case bmp::ClientPurpose::ServerListRequest: {
        post(m_io, [&, this] {
            auto list = load_server_list();
            if (list.has_value()) {
                client_tcp_write(bmp::ClientPacket {
                    .purpose = bmp::ClientPurpose::ServerListResponse,
                    .raw_data = list.value(),
                });
            } else {
                spdlog::error("Failed to load server list: {}", list.error());
                client_tcp_write(bmp::ClientPacket {
                    .purpose = bmp::ClientPurpose::Error,
                    .raw_data = json_to_vec({ "message", list.error() }),
                });
            }
        });
    } break;
    case bmp::ClientPurpose::Logout: {
        spdlog::error("Logout is not yet implemented");
    } break;
    case bmp::ClientPurpose::Connect: {
        try {
            auto details = json::parse(packet.get_readable_data());
            std::string host = details.at("host");
            uint16_t port = details.at("port");
            spdlog::info("Game requesting to connect to server [{}]:{}", host, port);
            auto connect_result = launcher.start_server_network(host, port);
            if (connect_result.has_error()) {
                spdlog::error("Failed to connect to server: {}", connect_result.error());
                client_tcp_write(bmp::ClientPacket {
                    .purpose = bmp::ClientPurpose::ConnectError,
                    .raw_data = json_to_vec({ "message", connect_result.error() }),
                });
            } else {
                spdlog::info("Connected to server!");
                start_server_identification();
            }
        } catch (const std::exception& e) {
            spdlog::error("Failed to read json for purpose 0x{:x}: {}", uint16_t(packet.purpose), e.what());
            disconnect(fmt::format("Invalid json in purpose 0x{:x}, see launcher logs for more info", uint16_t(packet.purpose)));
        }
    } break;
    default:
        disconnect(fmt::format("Invalid packet purpose in state 0x{:x}: 0x{:x}", uint16_t(m_client_state), uint16_t(packet.purpose)));
        break;
    }
}

void ClientNetwork::handle_server_identification(bmp::ClientPacket& packet) {
    switch (packet.purpose) {
    default:
        disconnect(fmt::format("Invalid packet purpose in state 0x{:x}: 0x{:x}", uint16_t(m_client_state), uint16_t(packet.purpose)));
        break;
    }
}

void ClientNetwork::handle_server_authentication(bmp::ClientPacket& packet) {
    switch (packet.purpose) {
    default:
        disconnect(fmt::format("Invalid packet purpose in state 0x{:x}: 0x{:x}", uint16_t(m_client_state), uint16_t(packet.purpose)));
        break;
    }
}

void ClientNetwork::handle_server_mod_download(bmp::ClientPacket& packet) {
    switch (packet.purpose) {
    default:
        disconnect(fmt::format("Invalid packet purpose in state 0x{:x}: 0x{:x}", uint16_t(m_client_state), uint16_t(packet.purpose)));
        break;
    }
}

void ClientNetwork::handle_server_session_setup(bmp::ClientPacket& packet) {
    switch (packet.purpose) {
    default:
        disconnect(fmt::format("Invalid packet purpose in state 0x{:x}: 0x{:x}", uint16_t(m_client_state), uint16_t(packet.purpose)));
        break;
    }
}

void ClientNetwork::handle_server_playing(bmp::ClientPacket& packet) {
    switch (packet.purpose) {
    default:
        disconnect(fmt::format("Invalid packet purpose in state 0x{:x}: 0x{:x}", uint16_t(m_client_state), uint16_t(packet.purpose)));
        break;
    }
}

void ClientNetwork::handle_server_leaving(bmp::ClientPacket& packet) {
    switch (packet.purpose) {
    default:
        disconnect(fmt::format("Invalid packet purpose in state 0x{:x}: 0x{:x}", uint16_t(m_client_state), uint16_t(packet.purpose)));
        break;
    }
}

void ClientNetwork::client_tcp_read(std::function<void(bmp::ClientPacket&&)> handler) {
    m_tmp_header_buffer.resize(bmp::ClientHeader::SERIALIZED_SIZE);
    boost::asio::async_read(m_game_socket, buffer(m_tmp_header_buffer),
        bind_executor(m_read_strand, [this, handler](auto ec, auto) {
            if (ec) {
                disconnect(fmt::format("Failed to read from game: {}", ec.message()));
            } else {
                bmp::ClientHeader hdr {};
                hdr.deserialize_from(m_tmp_header_buffer);
                // vector eaten up by now, recv again
                m_tmp_packet.raw_data.resize(hdr.data_size);
                m_tmp_packet.purpose = hdr.purpose;
                m_tmp_packet.flags = hdr.flags;
                spdlog::trace("Got header: purpose: 0x{:x}, flags: 0x{:x}, pid: {}, vid: {}, size: {}",
                    uint16_t(m_tmp_packet.purpose),
                    uint8_t(m_tmp_packet.flags),
                    m_tmp_packet.pid, m_tmp_packet.vid,
                    m_tmp_packet.get_readable_data().size());
                boost::asio::async_read(m_game_socket, buffer(m_tmp_packet.raw_data),
                    bind_executor(m_read_strand, [handler, this](auto ec, auto) {
                        if (ec) {
                            disconnect(fmt::format("Failed to read from game: {}", ec.message()));
                        } else {
                            // ok!
                            handler(std::move(m_tmp_packet));
                        }
                    }));
            }
        }));
}

void ClientNetwork::client_tcp_write(bmp::ClientPacket&& packet, std::function<void(boost::system::error_code)> handler) {
    boost::asio::post(m_write_strand, [this, packet = std::move(packet), handler = std::move(handler)] {
        auto purpose = packet.purpose;
        m_outbox.emplace_back(OutPacket { std::move(packet), handler });
        if (m_outbox.size() > 1) {
            // outstanding async_write
            spdlog::debug("Outstanding write, not writing 0x{:x} now", int(purpose));
            return;
        }
        client_tcp_write_impl();
    });
}

void ClientNetwork::client_tcp_write_impl() {
    OutPacket out_packet = std::move(m_outbox.front());

    spdlog::debug("Writing 0x{:x} now", int(out_packet.packet.purpose));
    // don't pop just yet

    auto header = out_packet.packet.finalize();
    // copy packet
    auto owned_packet = std::make_shared<bmp::ClientPacket>(std::move(out_packet.packet));
    // serialize header
    auto header_data = std::make_shared<std::vector<uint8_t>>(bmp::ClientHeader::SERIALIZED_SIZE);
    header.serialize_to(*header_data);
    std::array<const_buffer, 2> buffers = {
        buffer(*header_data),
        buffer(owned_packet->raw_data)
    };
    boost::asio::async_write(m_game_socket, buffers,
        bind_executor(m_write_strand, [this, header_data, owned_packet, handler = std::move(out_packet.handler)](auto ec, auto size) {
            // only now pop the packet we just sent from the outbox
            spdlog::trace("Wrote {} bytes for 0x{:x}", size, int(owned_packet->purpose));
            if (handler) {
                handler(ec);
            } else {
                if (ec) {
                    disconnect(fmt::format("Failed to send packet to game: {}", ec.message()));
                } else {
                    // ok!
                    spdlog::debug("Sent packet of type 0x{:x}", int(owned_packet->purpose));
                }
            }
            m_outbox.pop_front();
            // trigger the next send if there's more to send
            if (!m_outbox.empty()) {
                // async so this is not recursion!
                client_tcp_write_impl();
            }
        }));
}

std::vector<uint8_t> ClientNetwork::json_to_vec(const nlohmann::json& value) {
    auto str = value.dump();
    return std::vector<uint8_t>(str.begin(), str.end());
}
nlohmann::json ClientNetwork::vec_to_json(const std::vector<uint8_t>& vec) {
    return json::parse(std::string(vec.begin(), vec.end()));
}
void ClientNetwork::start_quick_join() {
    client_tcp_write(bmp::ClientPacket {
        .purpose = bmp::ClientPurpose::StateChangeQuickJoin,
    });
    m_client_state = bmp::ClientState::QuickJoin;

    // TODO: Implement DoJoin, etc

    start_browsing();
}

void ClientNetwork::start_browsing() {
    client_tcp_write(bmp::ClientPacket {
        .purpose = bmp::ClientPurpose::StateChangeBrowsing,
    });
    m_client_state = bmp::ClientState::Browsing;
}

Result<std::vector<uint8_t>, std::string> ClientNetwork::load_server_list() noexcept {
    try {
        auto list = HTTP::Get("https://backend.beammp.com/servers-info/");
        if (list == "-1") {
            return outcome::failure("Failed to fetch server list, see launcher log for more information.");
        }
        return outcome::success(std::vector<uint8_t>(list.begin(), list.end()));
    } catch (const std::exception& e) {
        return outcome::failure(fmt::format("Failed to fetch server list from backend: {}", e.what()));
    }
}
void ClientNetwork::handle_server_packet(bmp::Packet&& packet) {
    post(m_io, [packet, this] {
        switch (packet.purpose) {
        case bmp::ProtocolVersionBad:
            client_tcp_write(bmp::ClientPacket {
                .purpose = bmp::ClientPurpose::ConnectError,
                .raw_data = json_to_vec({ "message", "Server is outdated or too new - protocol is incompatible. Please try a different server and make sure your Launcher and Mod are up-to-date." }),
            });
            start_browsing();
            break;
        case bmp::ProtocolVersionOk:
            start_server_authentication();
            break;
        case bmp::AuthOk:
            uint32_t player_id;
            bmp::deserialize(player_id, packet.get_readable_data());
            client_tcp_write(bmp::ClientPacket {
                .purpose = bmp::ClientPurpose::AuthenticationOk,
                .raw_data = json_to_vec({ "pid", player_id }),
            });
            start_server_mod_download();
            break;
        case bmp::AuthFailed:
            client_tcp_write(bmp::ClientPacket {
                .purpose = bmp::ClientPurpose::AuthenticationError,
                .raw_data = json_to_vec(
                    { "message",
                        std::string(
                            packet.get_readable_data().begin(),
                            packet.get_readable_data().end()) }),
            });
            start_browsing();
            break;
        case bmp::PlayerRejected:
            client_tcp_write(bmp::ClientPacket {
                .purpose = bmp::ClientPurpose::PlayerRejected,
                .raw_data = json_to_vec(
                    { "message",
                        std::string(
                            packet.get_readable_data().begin(),
                            packet.get_readable_data().end()) }),
            });
            start_browsing();
            break;
        case bmp::ModsInfo:
            client_tcp_write(bmp::ClientPacket {
                .purpose = bmp::ClientPurpose::ModSyncStatus,
                .raw_data = json_to_vec(json::array()), // TODO: send more here i guess
            });
            spdlog::debug("Mod stuff not implemented, skipping to session setup");
            break;
        case bmp::MapInfo: {
            auto data = packet.get_readable_data();
            auto map = std::string(data.begin(), data.end());
            spdlog::debug("Map: '{}'", map);
            client_tcp_write(bmp::ClientPacket {
                .purpose = bmp::ClientPurpose::MapInfo,
                .raw_data = json_to_vec({ "map", map }),
            });
            start_server_session_setup();
        } break;
        case bmp::PlayersVehiclesInfo:
            client_tcp_write(bmp::ClientPacket {
                .purpose = bmp::ClientPurpose::PlayersAndVehiclesInfo,
                .raw_data = packet.get_readable_data(),
            });
            start_server_playing();
            break;
        case bmp::ClientInfo:
        case bmp::ServerInfo:
        case bmp::PlayerPublicKey:
        case bmp::StartUDP:
        case bmp::ModRequest:
        case bmp::ModResponse:
        case bmp::ModRequestInvalid:
        case bmp::ModsSyncDone:
        case bmp::SessionReady:
        case bmp::Ping:
        case bmp::VehicleSpawn:
        case bmp::VehicleDelete:
        case bmp::VehicleReset:
        case bmp::VehicleEdited:
        case bmp::VehicleCouplerChanged:
        case bmp::SpectatorSwitched:
        case bmp::ApplyInput:
        case bmp::ApplyElectrics:
        case bmp::ApplyNodes:
        case bmp::ApplyBreakgroups:
        case bmp::ApplyPowertrain:
        case bmp::ApplyPosition:
        case bmp::ChatMessage:
        case bmp::Event:
        case bmp::PlayerJoined:
        case bmp::PlayerLeft:
        case bmp::PlayerPingUpdate:
        case bmp::Notification:
        case bmp::Kicked:
        case bmp::StateChangeIdentification:
        case bmp::StateChangeAuthentication:
        case bmp::StateChangeModDownload:
        case bmp::StateChangeSessionSetup:
        case bmp::StateChangePlaying:
        case bmp::StateChangeLeaving:
            break;
        case bmp::Invalid:
        case bmp::ProtocolVersion:
            break;
        }
    });
}

void ClientNetwork::start_accept() {
    m_acceptor.async_accept(m_game_socket, [this](const auto& ec) { handle_accept(ec); });
}

void ClientNetwork::handle_accept(boost::system::error_code ec) {
    if (ec) {
        spdlog::error("Failed accepting game connection: {}", ec.message());
    } else {
        spdlog::info("Game connected!");
        auto game_ep = m_game_socket.remote_endpoint();
        spdlog::debug("Game: [{}]:{}", game_ep.address().to_string(), game_ep.port());
        handle_connection();
    }
    // TODO: We should probably accept() again somewhere once the game disconnected
}
void ClientNetwork::start_server_identification() {
    client_tcp_write(bmp::ClientPacket {
        .purpose = bmp::ClientPurpose::StateChangeServerIdentification,
    });
    m_client_state = bmp::ClientState::ServerIdentification;
}

void ClientNetwork::start_server_authentication() {
    client_tcp_write(bmp::ClientPacket {
        .purpose = bmp::ClientPurpose::StateChangeServerAuthentication,
    });
    m_client_state = bmp::ClientState::ServerAuthentication;
}

void ClientNetwork::start_server_mod_download() {
    client_tcp_write(bmp::ClientPacket {
        .purpose = bmp::ClientPurpose::StateChangeServerModDownload,
    });
    m_client_state = bmp::ClientState::ServerModDownload;
}

void ClientNetwork::start_server_session_setup() {
    client_tcp_write(bmp::ClientPacket {
        .purpose = bmp::ClientPurpose::StateChangeServerSessionSetup,
    });
    m_client_state = bmp::ClientState::ServerSessionSetup;
}

void ClientNetwork::start_server_playing() {
    client_tcp_write(bmp::ClientPacket {
        .purpose = bmp::ClientPurpose::StateChangeServerPlaying,
    });
    m_client_state = bmp::ClientState::ServerPlaying;
}
