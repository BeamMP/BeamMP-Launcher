#include "ClientNetwork.h"
#include "ClientPacket.h"
#include "ClientTransport.h"
#include "Http.h"
#include "Launcher.h"
#include "Identity.h"

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

    ip::tcp::acceptor acceptor(m_io, listen_ep);
    acceptor.listen(ip::tcp::socket::max_listen_connections, ec);
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

ClientNetwork::ClientNetwork(Launcher& launcher, uint16_t port)
    : m_listen_port(port)
    , launcher(launcher) {
    spdlog::debug("Client network created");
}

ClientNetwork::~ClientNetwork() {
    spdlog::debug("Client network destroyed");
}

void ClientNetwork::handle_connection(ip::tcp::socket&& socket) {
    m_game_socket = std::move(socket);
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
    client_tcp_write(info_packet);

    // packet read and respond loop
    while (!*m_shutdown) {
        try {
            auto packet = client_tcp_read();
            handle_packet(packet);
        } catch (const std::exception& e) {
            spdlog::error("Unhandled exception in connection handler, connection closing.");
            spdlog::debug("Exception: {}", e.what());
            m_game_socket.close();
        }
    }
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
            spdlog::info("Connected to {} (mod v{}, game v{}, protocol v{}.{}.{}",
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
    bmp::ClientPacket state_change {
        .purpose = bmp::ClientPurpose::StateChangeLogin,
    };
    client_tcp_write(state_change);

    m_client_state = bmp::ClientState::Login;

    if (ident::is_login_cached()) {
        auto login = ident::login_cached();
        // case in which the login is cached and login was successful:
        // we just send the login result and continue to the next state.
        if (login.has_value()) {
            *m_identity = login.value();
            bmp::ClientPacket login_result {
                .purpose = bmp::ClientPurpose::LoginResult,
                .raw_data = json_to_vec({
                    { "success", true },
                    { "message", m_identity->Message },
                    { "username", m_identity->Username },
                    { "role", m_identity->Role },
                }),
            };
            client_tcp_write(login_result);
            bmp::ClientPacket change_to_quickjoin {
                .purpose = bmp::ClientPurpose::StateChangeQuickJoin,
            };
            client_tcp_write(change_to_quickjoin);
            m_client_state = bmp::ClientState::QuickJoin;
            return; // done
        } else {
            spdlog::warn("Failed to automatically login with cached/saved login details: {}", login.error());
            spdlog::info("Trying normal login");
        }
        // fallthrough to normal login
    }

    // first packet in login
    // TODO: send LoginResult if already logged in.
    bmp::ClientPacket ask_for_creds {
        .purpose = bmp::ClientPurpose::AskForCredentials,
    };
    client_tcp_write(ask_for_creds);
    // wait for response, so return
}

void ClientNetwork::disconnect(const std::string& reason) {
    bmp::ClientPacket error {
        .purpose = bmp::ClientPurpose::Error,
        .raw_data = json_to_vec({ "message", reason })
    };
    client_tcp_write(error);
    m_game_socket.close();
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
                bmp::ClientPacket login_fail {
                    .purpose = bmp::ClientPurpose::LoginResult,
                    .raw_data = json_to_vec({
                        { "success", false },
                        { "message", result.error() },
                    }),
                };
                client_tcp_write(login_fail);
                bmp::ClientPacket retry {
                    .purpose = bmp::ClientPurpose::AskForCredentials,
                };
                client_tcp_write(retry);
                return;
            }
            *m_identity = result.value();
            bmp::ClientPacket login_result {
                .purpose = bmp::ClientPurpose::LoginResult,
                .raw_data = json_to_vec({
                    { "success", true },
                    { "message", m_identity->Message },
                    { "username", m_identity->Username },
                    { "role", m_identity->Role },
                }),
            };
            client_tcp_write(login_result);

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
        auto list = load_server_list();
        if (list.has_value()) {
            bmp::ClientPacket response {
                .purpose = bmp::ClientPurpose::ServerListResponse,
                .raw_data = json_to_vec(list.value()),
            };
            client_tcp_write(response);
        } else {
            spdlog::error("Failed to load server list: {}", list.error());
            bmp::ClientPacket err {
                .purpose = bmp::ClientPurpose::Error,
                .raw_data = json_to_vec({ "message", list.error() }),
            };
            client_tcp_write(err);
        }
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

bmp::ClientPacket ClientNetwork::client_tcp_read() {
    bmp::ClientPacket packet {};
    std::vector<uint8_t> header_buffer(bmp::ClientHeader::SERIALIZED_SIZE);
    read(m_game_socket, buffer(header_buffer));
    bmp::ClientHeader hdr {};
    hdr.deserialize_from(header_buffer);
    // vector eaten up by now, recv again
    packet.raw_data.resize(hdr.data_size);
    read(m_game_socket, buffer(packet.raw_data));
    packet.purpose = hdr.purpose;
    packet.flags = hdr.flags;
    return packet;
}

void ClientNetwork::client_tcp_write(bmp::ClientPacket& packet) {
    // finalize the packet (compress etc) and produce header
    auto header = packet.finalize();
    // serialize header
    std::vector<uint8_t> header_data(bmp::ClientHeader::SERIALIZED_SIZE);
    header.serialize_to(header_data);
    // write header and packet data
    write(m_game_socket, buffer(header_data));
    write(m_game_socket, buffer(packet.raw_data));
}
std::vector<uint8_t> ClientNetwork::json_to_vec(const nlohmann::json& value) {
    auto str = value.dump();
    return std::vector<uint8_t>(str.begin(), str.end());
}
nlohmann::json ClientNetwork::vec_to_json(const std::vector<uint8_t>& vec) {
    return json::parse(std::string(vec.begin(), vec.end()));
}
void ClientNetwork::start_quick_join() {
    bmp::ClientPacket change_to_quickjoin {
        .purpose = bmp::ClientPurpose::StateChangeQuickJoin,
    };
    client_tcp_write(change_to_quickjoin);
    m_client_state = bmp::ClientState::QuickJoin;

    // TODO: Implement DoJoin, etc

    start_browsing();
}

void ClientNetwork::start_browsing() {
    bmp::ClientPacket change_to_browsing {
        .purpose = bmp::ClientPurpose::StateChangeBrowsing,
    };
    client_tcp_write(change_to_browsing);
    m_client_state = bmp::ClientState::Browsing;
}

Result<nlohmann::json, std::string> ClientNetwork::load_server_list() noexcept {
    try {
        auto list = HTTP::Get("https://backend.beammp.com/servers-list");
        if (list == "-1") {
            return outcome::failure("Failed to fetch server list, see launcher log for more information.");
        }
        json result = json::parse(list);
        return outcome::success(result);
    } catch (const std::exception& e) {
        return outcome::failure(fmt::format("Failed to fetch server list from backend: {}", e.what()));
    }
}
