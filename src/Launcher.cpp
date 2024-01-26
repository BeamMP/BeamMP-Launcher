#include "Launcher.h"
#include "Compression.h"
#include "Hashing.h"
#include "Http.h"
#include "Platform.h"
#include "Version.h"
#include <boost/asio.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/process.hpp>
#include <chrono>
#include <filesystem>
#include <fmt/format.h>
#include <httplib.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <vector>
#include <charconv>

using namespace boost::asio;

namespace fs = std::filesystem;

Launcher::Launcher()
    : m_game_socket(m_io)
    , m_core_socket(m_io)
    , m_tcp_socket(m_io)
    , m_udp_socket(m_io) {
    spdlog::debug("Launcher startup");
    m_config = Config {};
    if (!m_config->is_valid) {
        spdlog::error("Launcher config invalid!");
    }
}

/// Sets shared headers for all backend proxy messages
static void proxy_set_headers(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Request-Method", "POST, OPTIONS, GET");
    res.set_header("Access-Control-Request-Headers", "X-API-Version");
}

void Launcher::proxy_main() {
    httplib::Server HTTPProxy;
    httplib::Headers headers = {
        { "User-Agent", fmt::format("BeamMP-Launcher/{}.{}.{}", PRJ_VERSION_MAJOR, PRJ_VERSION_MINOR, PRJ_VERSION_PATCH) },
        { "Accept", "*/*" }
    };
    std::string pattern = "/:any1";
    for (int i = 2; i <= 4; i++) {
        HTTPProxy.Get(pattern, [&](const httplib::Request& req, httplib::Response& res) {
            httplib::Client cli("https://backend.beammp.com");
            proxy_set_headers(res);
            if (req.has_header("X-BMP-Authentication")) {
                headers.emplace("X-BMP-Authentication", m_identity->PrivateKey);
            }
            if (req.has_header("X-API-Version")) {
                headers.emplace("X-API-Version", req.get_header_value("X-API-Version"));
            }
            if (auto cli_res = cli.Get(req.path, headers); cli_res) {
                res.set_content(cli_res->body, cli_res->get_header_value("Content-Type"));
            } else {
                res.set_content(to_string(cli_res.error()), "text/plain");
            }
        });

        HTTPProxy.Post(pattern, [&](const httplib::Request& req, httplib::Response& res) {
            httplib::Client cli("https://backend.beammp.com");
            proxy_set_headers(res);
            if (req.has_header("X-BMP-Authentication")) {
                headers.emplace("X-BMP-Authentication", m_identity->PrivateKey);
            }
            if (req.has_header("X-API-Version")) {
                headers.emplace("X-API-Version", req.get_header_value("X-API-Version"));
            }
            if (auto cli_res = cli.Post(req.path, headers, req.body,
                    req.get_header_value("Content-Type"));
                cli_res) {
                res.set_content(cli_res->body, cli_res->get_header_value("Content-Type"));
            } else {
                res.set_content(to_string(cli_res.error()), "text/plain");
            }
        });
        pattern += "/:any" + std::to_string(i);
    }
    m_proxy_port = HTTPProxy.bind_to_any_port("0.0.0.0");
    spdlog::debug("http proxy started on port {}", m_proxy_port.get());
    HTTPProxy.listen_after_bind();
}

Launcher::~Launcher() {
    m_proxy_thread.interrupt();
    m_game_thread.detach();
}

void Launcher::parse_config() {
}

void Launcher::set_port(int p) {
    spdlog::warn("Using custom port {}", p);
    m_config->port = p;
}

void Launcher::check_for_updates(int argc, char** argv) {
    std::string LatestHash = HTTP::Get(fmt::format("https://backend.beammp.com/sha/launcher?branch={}&pk={}", m_config->branch, m_identity->PublicKey));
    std::string LatestVersion = HTTP::Get(fmt::format("https://backend.beammp.com/version/launcher?branch={}&pk={}", m_config->branch, m_identity->PublicKey));

    std::string DownloadURL = fmt::format("https://backend.beammp.com/builds/launcher?download=true"
                                          "&pk={}"
                                          "&branch={}",
        m_identity->PublicKey, m_config->branch);

    spdlog::debug("Latest hash: {}", LatestHash);
    spdlog::debug("Latest version: {}", LatestVersion);

    transform(LatestHash.begin(), LatestHash.end(), LatestHash.begin(), ::tolower);
    std::string EP = (m_exe_path.get() / m_exe_name.get()).generic_string();
    std::string Back = (m_exe_path.get() / "BeamMP-Launcher.back").generic_string();

    std::string FileHash = sha256_file(EP);

    if (FileHash != LatestHash
        && Version(PRJ_VERSION_MAJOR, PRJ_VERSION_MINOR, PRJ_VERSION_PATCH).is_outdated(Version(LatestVersion))) {
        spdlog::info("Launcher update found!");
        fs::remove(Back);
        fs::rename(EP, Back);
        spdlog::info("Downloading Launcher update " + LatestHash);
        HTTP::Download(DownloadURL, EP);
        plat::URelaunch(argc, argv);
    } else {
        spdlog::info("Launcher version is up to date");
    }
}

void Launcher::find_game() {
    // try to find the game by multiple means

    spdlog::info("Locating game");
    // 0. config!
    if (!m_config->game_dir.empty()
        && std::filesystem::exists(std::filesystem::path(m_config->game_dir) / "BeamNG.drive.exe")) {
        spdlog::debug("Found game directory in config: '{}'", m_config->game_dir);
        return;
    }

    // 1. magic
    auto game_dir = plat::get_game_dir_magically();
    if (!game_dir.empty()) {
        m_config->game_dir = game_dir;
        m_config->write_to_file();
        return;
    } else {
        spdlog::debug("Couldn't magically find game directory");
    }

    // 2. ask
    m_config->game_dir = plat::ask_for_folder();

    spdlog::debug("Located game at '{}'", m_config->game_dir);
    m_config->write_to_file();
}

static std::string check_game_version(const std::filesystem::path& dir) {
    std::string temp;
    std::string Path = (dir / "integrity.json").generic_string();
    boost::iostreams::mapped_file file(Path);
    auto json = nlohmann::json::parse(file.const_begin(), file.const_end());
    return json["version"].is_string() ? json["version"].get<std::string>() : "";
}

void Launcher::pre_game() {
    std::string GameVer = check_game_version(m_config->game_dir);
    if (GameVer.empty()) {
        spdlog::error("Game version is empty!");
    }
    spdlog::info("Game Version: " + GameVer);

    check_mp((std::filesystem::path(m_config->game_dir) / "mods/multiplayer").generic_string());

    std::string LatestHash = HTTP::Get(fmt::format("https://backend.beammp.com/sha/mod?branch={}&pk={}", m_config->branch, m_identity->PublicKey));
    transform(LatestHash.begin(), LatestHash.end(), LatestHash.begin(), ::tolower);
    LatestHash.erase(std::remove_if(LatestHash.begin(), LatestHash.end(),
                         [](auto const& c) -> bool { return !std::isalnum(c); }),
        LatestHash.end());

    try {
        if (!fs::exists(std::filesystem::path(m_config->game_dir) / "mods/multiplayer")) {
            fs::create_directories(std::filesystem::path(m_config->game_dir) / "mods/multiplayer");
        }
        enable_mp();
    } catch (std::exception& e) {
        spdlog::error("Fatal: {}", e.what());
        std::exit(1);
    }

    auto ZipPath(std::filesystem::path(m_config->game_dir) / "mods/multiplayer/BeamMP.zip");

    std::string FileHash = sha256_file(ZipPath.generic_string());

    if (FileHash != LatestHash) {
        spdlog::info("Downloading BeamMP Update " + LatestHash);
        HTTP::Download(fmt::format("https://backend.beammp.com/builds/client?download=true&pk={}&branch={}", m_identity->PublicKey, m_config->branch), ZipPath.generic_string());
    }

    auto Target = std::filesystem::path(m_config->game_dir) / "mods/unpacked/beammp";
    if (fs::is_directory(Target)) {
        fs::remove_all(Target);
    }
}

static size_t count_items_in_dir(const std::filesystem::path& path) {
    return static_cast<size_t>(std::distance(std::filesystem::directory_iterator { path }, std::filesystem::directory_iterator {}));
}

void Launcher::check_mp(const std::string& path) {
    if (!fs::exists(path))
        return;
    size_t c = count_items_in_dir(fs::path(path));
    try {
        for (auto& p : fs::directory_iterator(path)) {
            if (p.exists() && !p.is_directory()) {
                std::string name = p.path().filename().string();
                for (char& Ch : name)
                    Ch = char(tolower(Ch));
                if (name != "beammp.zip")
                    fs::remove(p.path());
            }
        }
    } catch (...) {
        spdlog::error("We were unable to clean the multiplayer mods folder! Is the game still running or do you have something open in that folder?");
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::exit(1);
    }
}

void Launcher::enable_mp() {
    std::string File = (std::filesystem::path(m_config->game_dir) / "mods/db.json").generic_string();
    if (!fs::exists(File))
        return;
    auto Size = fs::file_size(File);
    if (Size < 2)
        return;
    std::ifstream db(File);
    if (db.is_open()) {
        std::string Data(Size, 0);
        db.read(&Data[0], Size);
        db.close();
        nlohmann::json d = nlohmann::json::parse(Data, nullptr, false);
        if (Data.at(0) != '{' || d.is_discarded()) {
            // spdlog::error("Failed to parse " + File); //TODO illegal formatting
            return;
        }
        if (d.contains("mods") && d["mods"].contains("multiplayerbeammp")) {
            d["mods"]["multiplayerbeammp"]["active"] = true;
            std::ofstream ofs(File);
            if (ofs.is_open()) {
                ofs << d.dump();
                ofs.close();
            } else {
                spdlog::error("Failed to write " + File);
            }
        }
    }
}
void Launcher::game_main() {
    auto path = std::filesystem::current_path();
    std::filesystem::current_path(m_config->game_dir);
#if defined(PLATFORM_LINUX)
    auto game_path = (std::filesystem::path(m_config->game_dir) / "BinLinux/BeamNG.drive.x64").generic_string();
#elif defined(PLATFORM_WINDOWS)
    auto game_path = (std::filesystem::path(m_config->game_dir) / "Bin64/BeamNG.drive.x64.exe").generic_string();
#endif
    boost::process::child game(game_path, boost::process::std_out > boost::process::null);
    std::filesystem::current_path(path);
    if (game.running()) {
        spdlog::info("Game launched!");
        game.wait();
        spdlog::info("Game closed! Launcher closing soon");
    } else {
        spdlog::error("Failed to launch the game! Please start the game manually.");
    }
}

void Launcher::start_game() {
    m_game_thread = boost::scoped_thread<>(&Launcher::game_main, this);
}
void Launcher::start_network() {
    while (true) {
        try {
            net_core_main();
        } catch (const std::exception& e) {
            spdlog::error("Error: {}", e.what());
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void Launcher::reset_status() {
    *m_m_status = " ";
    *m_ul_status = "Ulstart";
    m_conf_list->clear();
    *m_ping = -1;
}

void Launcher::net_core_main() {
    ip::tcp::endpoint listen_ep(ip::address::from_string("0.0.0.0"), static_cast<uint16_t>(m_config->port));
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
            m_game_socket = acceptor.accept(game_ep, ec);
            if (ec) {
                spdlog::error("Failed to accept: {}", ec.message());
            }
            reset_status();
            spdlog::info("Game connected!");
            spdlog::debug("Game: [{}]:{}", game_ep.address().to_string(), game_ep.port());
            game_loop();
            spdlog::warn("Game disconnected!");
        } catch (const std::exception& e) {
            spdlog::error("Fatal error in core network: {}", e.what());
        }
    } while (!*m_shutdown);
}

void Launcher::game_loop() {
    size_t Size;
    size_t Temp;
    size_t Rcv;
    char Header[10] = { 0 };
    boost::system::error_code ec;
    do {
        Rcv = 0;
        do {
            Temp = boost::asio::read(m_game_socket, buffer(&Header[Rcv], 1), ec);
            if (ec) {
                spdlog::error("(Core) Failed to receive from game: {}", ec.message());
                break;
            }
            if (Temp < 1)
                break;
            if (!isdigit(Header[Rcv]) && Header[Rcv] != '>') {
                spdlog::error("(Core) Invalid lua communication: '{}'", std::string(Header, Rcv));
                break;
            }
        } while (Header[Rcv++] != '>');
        if (Temp < 1)
            break;
        if (std::from_chars(Header, &Header[Rcv], Size).ptr[0] != '>') {
            spdlog::debug("(Core) Invalid lua Header: '{}'", std::string(Header, Rcv));
            break;
        }
        std::vector<char> Ret(Size, 0);
        Rcv = 0;

        Temp = boost::asio::read(m_game_socket, buffer(Ret, Size), ec);
        if (ec) {
            spdlog::error("(Core) Failed to receive from game: {}", ec.message());
            break;
        }
        handle_core_packet(Ret);
    } while (Temp > 0);
    if (Temp == 0) {
        spdlog::debug("(Core) Connection closing");
    }
    // TODO: NetReset
}

static bool IsAllowedLink(const std::string& Link) {
    std::regex link_pattern(R"(https:\/\/(?:\w+)?(?:\.)?(?:beammp\.com|discord\.gg))");
    std::smatch link_match;
    return std::regex_search(Link, link_match, link_pattern) && link_match.position() == 0;
}

void Launcher::handle_core_packet(const std::vector<char>& RawData) {
    std::string Data(RawData.begin(), RawData.end());

    char Code = Data.at(0), SubCode = 0;
    if (Data.length() > 1)
        SubCode = Data.at(1);
    switch (Code) {
    case 'A':
        Data = Data.substr(0, 1);
        break;
    case 'B':
        Data = Code + HTTP::Get("https://backend.beammp.com/servers-info");
        break;
    case 'C':
        m_list_of_mods->clear();
        if (!start_sync(Data)) {
            // TODO: Handle
            spdlog::error("start_sync failed, spdlog::error case not implemented");
        }
        while (m_list_of_mods->empty() && !*m_shutdown) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (m_list_of_mods.get() == "-")
            Data = "L";
        else
            Data = "L" + m_list_of_mods.get();
        break;
    case 'O': // open default browser with URL
        if (IsAllowedLink(Data.substr(1))) {
            spdlog::info("Opening Link \"" + Data.substr(1) + "\"");
            boost::process::child c(std::string("open '") + Data.substr(1) + "'", boost::process::std_out > boost::process::null);
            c.detach();
        }
        Data.clear();
        break;
    case 'P':
        Data = Code + std::to_string(*m_proxy_port);
        break;
    case 'U':
        if (SubCode == 'l')
            Data = *m_ul_status;
        if (SubCode == 'p') {
            if (*m_ping > 800) {
                Data = "Up-2";
            } else
                Data = "Up" + std::to_string(*m_ping);
        }
        if (!SubCode) {
            std::string Ping;
            if (*m_ping > 800)
                Ping = "-2";
            else
                Ping = std::to_string(*m_ping);
            Data = std::string(*m_ul_status) + "\n" + "Up" + Ping;
        }
        break;
    case 'M':
        Data = *m_m_status;
        break;
    case 'Q':
        if (SubCode == 'S') {
            spdlog::debug("Shutting down via QS");
            *m_shutdown = true;
            *m_ping = -1;
        }
        if (SubCode == 'G') {
            spdlog::debug("Shutting down via QG");
            *m_shutdown = true;
        }
        Data.clear();
        break;
    case 'R': // will send mod name
        if (m_conf_list->find(Data) == m_conf_list->end()) {
            m_conf_list->insert(Data);
            m_mod_loaded = true;
        }
        Data.clear();
        break;
    case 'Z':
        Data = fmt::format("Z{}.{}", PRJ_VERSION_MAJOR, PRJ_VERSION_MINOR);
        break;
    case 'N':
        if (SubCode == 'c') {
            Data = "N{\"Auth\":" + std::to_string(m_identity->LoginAuth) + "}";
        } else {
            Data = "N" + m_identity->login(Data.substr(Data.find(':') + 1));
        }
        break;
    default:
        Data.clear();
        break;
    }
    if (!Data.empty() && m_game_socket.is_open()) {
        boost::system::error_code ec;
        boost::asio::write(m_game_socket, buffer((Data + "\n").c_str(), Data.size() + 1), ec);
        if (ec) {
            spdlog::error("(Core) send failed with error: {}", ec.message());
        }
    }
}

static void compress_properly(std::vector<uint8_t>& Data) {
    constexpr std::string_view ABG = "ABG:";
    auto CombinedData = std::vector<uint8_t>(ABG.begin(), ABG.end());
    auto CompData = Comp(Data);
    CombinedData.resize(ABG.size() + CompData.size());
    std::copy(CompData.begin(), CompData.end(), CombinedData.begin() + ABG.size());
    Data = CombinedData;
}

void Launcher::send_large(const std::string& Data) {
    std::vector<uint8_t> vec(Data.data(), Data.data() + Data.size());
    compress_properly(vec);
    tcp_send(vec);
}

void Launcher::server_send(const std::string& Data, bool Rel) {
    if (Data.empty())
        return;
    if (Data.find("Zp") != std::string::npos && Data.size() > 500) {
        abort();
    }
    char C = 0;
    bool Ack = false;
    int DLen = int(Data.length());
    if (DLen > 3) {
        C = Data.at(0);
    }
    if (C == 'O' || C == 'T') {
        Ack = true;
    }
    if (C == 'N' || C == 'W' || C == 'Y' || C == 'V' || C == 'E' || C == 'C') {
        Rel = true;
    }
    if (Ack || Rel) {
        if (Ack || DLen > 1000) {
            send_large(Data);
        } else {
            tcp_send(Data);
        }
    } else {
        udp_send(Data);
    }

    if (DLen > 1000) {
        spdlog::debug("(Launcher->Server) Bytes sent: " + std::to_string(Data.length()) + " : "
            + Data.substr(0, 10)
            + Data.substr(Data.length() - 10));
    } else if (C == 'Z') {
        // spdlog::debug("(Game->Launcher) : " + Data);
    }
}

void Launcher::tcp_game_main() {
    spdlog::debug("Game server starting on port {}", m_config->port + 1);
    ip::tcp::endpoint listen_ep(ip::address::from_string("0.0.0.0"), static_cast<uint16_t>(m_config->port + 1));
    ip::tcp::socket g_listener(m_io);
    g_listener.open(listen_ep.protocol());
    socket_base::linger linger_opt {};
    linger_opt.enabled(false);
    g_listener.set_option(linger_opt);
    ip::tcp::acceptor g_acceptor(m_io, listen_ep);
    g_acceptor.listen(socket_base::max_listen_connections);
    spdlog::debug("Game server listening");

    while (!*m_shutdown) {
        spdlog::debug("Main loop");

        // socket is connected at this point, spawn thread
        m_client_thread = boost::scoped_thread<>(&Launcher::tcp_client_main, this);
        spdlog::debug("Client thread spawned");

        m_core_socket = g_acceptor.accept();
        spdlog::debug("Game connected (tcp game)!");

        spdlog::info("Successfully connected");

        m_ping_thread = boost::scoped_thread<>(&Launcher::auto_ping, this);
        m_udp_thread = boost::scoped_thread<>(&Launcher::udp_main, this);

        int32_t Temp = 0;
        do {
            boost::system::error_code ec;
            int32_t Rcv = 0;
            int32_t Size = 0;
            char Header[10] = { 0 };
            do {
                Temp = boost::asio::read(m_core_socket, buffer(&Header[Rcv], 1), ec);
                if (ec) {
                    spdlog::error("(Game) Failed to receive from game: {}", ec.message());
                    break;
                }
                if (Temp < 1)
                    break;
                if (!isdigit(Header[Rcv]) && Header[Rcv] != '>') {
                    spdlog::error("(Game) Invalid lua communication");
                    break;
                }
            } while (Header[Rcv++] != '>');
            if (Temp < 1)
                break;
            if (std::from_chars(Header, &Header[Rcv], Size).ptr[0] != '>') {
                spdlog::debug("(Game) Invalid lua Header -> " + std::string(Header, Rcv));
                break;
            }
            std::vector<char> Ret(Size, 0);
            Rcv = 0;

            Temp = boost::asio::read(m_core_socket, buffer(Ret, Size), ec);
            if (ec) {
                spdlog::error("(Game) Failed to receive from game: {}", ec.message());
                break;
            }
            spdlog::debug("Got {} from the game, sending to server", Ret[0]);
            server_send(std::string(Ret.begin(), Ret.end()), false);
        } while (Temp > 0);
        if (Temp == 0) {
            spdlog::debug("(Proxy) Connection closing");
        } else {
            spdlog::debug("(Proxy) Recv failed");
        }
    }
    spdlog::debug("Game server exiting");
}

bool Launcher::start_sync(const std::string& Data) {
    ip::tcp::resolver resolver(m_io);
    const auto sep = Data.find_last_of(':');
    if (sep == std::string::npos || sep == Data.length() - 1) {
        spdlog::error("Invalid host:port string: '{}'", Data);
        return false;
    }
    const auto host = Data.substr(1, sep - 1);
    const auto service = Data.substr(sep + 1);
    boost::system::error_code ec;
    auto resolved = resolver.resolve(host, service, ec);
    if (ec) {
        spdlog::error("Failed to resolve '{}': {}", Data.substr(1), ec.message());
        return false;
    }
    bool connected = false;
    for (const auto& addr : resolved) {
        m_tcp_socket.connect(addr.endpoint(), ec);
        if (!ec) {
            spdlog::info("Resolved and connected to '[{}]:{}'",
                addr.endpoint().address().to_string(),
                addr.endpoint().port());
            connected = true;
            if (addr.endpoint().address().is_v4()) {
                m_udp_socket.open(ip::udp::v4());
            } else {
                m_udp_socket.open(ip::udp::v6());
            }
            m_udp_endpoint = ip::udp::endpoint(m_tcp_socket.remote_endpoint().address(), m_tcp_socket.remote_endpoint().port());
            break;
        }
    }
    if (!connected) {
        spdlog::error("Failed to connect to '{}'", Data);
        return false;
    }
    reset_status();
    m_identity->check_local_key();
    *m_ul_status = "UlLoading...";

    auto thread = boost::scoped_thread<>(&Launcher::tcp_game_main, this);
    m_tcp_game_thread.swap(thread);

    return true;
}

void Launcher::tcp_send(const std::vector<uint8_t>& data) {
    const auto Size = int32_t(data.size());
    std::vector<uint8_t> ToSend;
    ToSend.resize(data.size() + sizeof(Size));
    std::memcpy(ToSend.data(), &Size, sizeof(Size));
    std::memcpy(ToSend.data() + sizeof(Size), data.data(), data.size());
    boost::system::error_code ec;
    spdlog::debug("tcp sending {} bytes to the server", data.size());
    boost::asio::write(m_tcp_socket, buffer(ToSend), ec);
    if (ec) {
        spdlog::debug("write(): {}", ec.message());
        throw std::runtime_error(fmt::format("write() failed: {}", ec.message()));
    }
    spdlog::debug("tcp sent {} bytes to the server", data.size());
}
void Launcher::tcp_send(const std::string& data) {
    tcp_send(std::vector<uint8_t>(data.begin(), data.end()));
}

std::string Launcher::tcp_recv() {
    int32_t Header {};

    boost::system::error_code ec;
    std::array<uint8_t, sizeof(Header)> HeaderData {};
    boost::asio::read(m_tcp_socket, buffer(HeaderData), ec);
    if (ec) {
        throw std::runtime_error(fmt::format("read() failed: {}", ec.message()));
    }
    Header = *reinterpret_cast<int32_t*>(HeaderData.data());

    if (Header < 0) {
        throw std::runtime_error("read() failed: Negative TCP header");
    }

    std::vector<uint8_t> Data;
    // 100 MiB is super arbitrary but what can you do.
    if (Header < int32_t(100 * (1024 * 1024))) {
        Data.resize(Header);
    } else {
        throw std::runtime_error("Header size limit exceeded");
    }
    auto N = boost::asio::read(m_tcp_socket, buffer(Data), ec);
    if (ec) {
        throw std::runtime_error(fmt::format("read() failed: {}", ec.message()));
    }

    if (N != Header) {
        throw std::runtime_error(fmt::format("read() failed: Expected {} byte(s), got {} byte(s) instead", Header, N));
    }

    constexpr std::string_view ABG = "ABG:";
    if (Data.size() >= ABG.size() && std::equal(Data.begin(), Data.begin() + ABG.size(), ABG.begin(), ABG.end())) {
        Data.erase(Data.begin(), Data.begin() + ABG.size());
        Data = DeComp(Data);
    }
    return { reinterpret_cast<const char*>(Data.data()), Data.size() };
}

void Launcher::game_send(const std::string& data) {
    auto to_send = data + "\n";
    boost::asio::write(m_core_socket, buffer(reinterpret_cast<const uint8_t*>(to_send.data()), to_send.size()));
}

void Launcher::tcp_client_main() {
    spdlog::debug("Client starting");

    boost::system::error_code ec;

    try {
        {
            // send C to say hi
            boost::asio::write(m_tcp_socket, buffer("C", 1), ec);

            // client version
            tcp_send(fmt::format("VC{}.{}", PRJ_VERSION_MAJOR, PRJ_VERSION_MINOR));

            // auth
            auto res = tcp_recv();
            if (res.empty() || res[0] == 'E') {
                throw std::runtime_error("Kicked!");
            } else if (res[0] == 'K') {
                if (res.size() > 1) {
                    throw std::runtime_error(fmt::format("Kicked: {}", res.substr(1)));
                } else {
                    throw std::runtime_error("Kicked!");
                }
            }

            tcp_send(m_identity->PublicKey);

            res = tcp_recv();
            if (res.empty() || res[0] != 'P') {
                throw std::runtime_error("Expected 'P'");
            }

            if (res.size() > 1 && std::all_of(res.begin() + 1, res.end(), [](char c) { return std::isdigit(c); })) {
                *m_client_id = std::stoi(res.substr(1));
            } else {
                // auth failed
                throw std::runtime_error("Authentication failed");
            }

            tcp_send("SR");

            res = tcp_recv();
            if (res.empty() || res[0] == 'E') {
                throw std::runtime_error("Kicked!");
            } else if (res[0] == 'K') {
                if (res.size() > 1) {
                    throw std::runtime_error(fmt::format("Kicked: {}", res.substr(1)));
                } else {
                    throw std::runtime_error("Kicked!");
                }
            }

            if (res == "-") {
                spdlog::info("Didn't receive any mods");
                *m_list_of_mods = "-";
                tcp_send("Done");
                spdlog::info("Done!");
            }
            // auth done!
        }

        if (m_list_of_mods.get() != "-") {
            spdlog::info("Checking resources");
            if (!std::filesystem::exists("Resources")) {
                std::filesystem::create_directories("Resources");
            }
            throw std::runtime_error("Mod loading not yet implemented");
        }

        std::string res;
        while (!*m_shutdown) {
            res = tcp_recv();
            server_parse(res);
        }

        game_send("T");

    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        *m_shutdown = true;
    }
    spdlog::debug("Client stopping");
}

void Launcher::server_parse(const std::string& data) {
    if (data.empty()) {
        return;
    }
    switch (data[0]) {
    case 'p':
        *m_ping_end = std::chrono::high_resolution_clock::now();
        if (m_ping_start.get() > m_ping_end.get()) {
            *m_ping = 0;
        } else {
            *m_ping = int(std::chrono::duration_cast<std::chrono::milliseconds>(m_ping_end.get() - m_ping_start.get()).count());
        }
        break;
    case 'M':
        *m_m_status = data;
        *m_ul_status = "Uldone";
        break;
    default:
        game_send(data);
    }
}

std::string Launcher::udp_recv() {
    // the theoretical maximum udp message is 64 KiB, so we save one buffer per thread for it
    static thread_local std::vector<char> s_local_buf(size_t(64u * 1024u));
    auto n = m_udp_socket.receive_from(buffer(s_local_buf), m_udp_endpoint);
    return { s_local_buf.data(), n };
}

void Launcher::udp_main() {
    spdlog::info("UDP starting");

    try {
        game_send(std::string("P") + std::to_string(*m_client_id));
        tcp_send("H");
        udp_send("p");

        while (!*m_shutdown) {
            auto msg = udp_recv();
            if (!msg.empty() && msg.length() > 4 && msg.substr(0, 4) == "ABG:") {
                server_parse(std::string(DeComp(msg)));
            } else {
                server_parse(std::string(msg));
            }
        }

    } catch (const std::exception& e) {
        spdlog::error("Error in udp_main: {}", e.what());
    }

    spdlog::info("UDP stopping");
}

void Launcher::auto_ping() {
    spdlog::debug("Ping thread started");
    while (!*m_shutdown) {
        udp_send("p");
        *m_ping_start = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    spdlog::debug("Ping thread stopped");
}

void Launcher::udp_send(const std::string& data) {
    auto vec = std::vector<uint8_t>(data.begin(), data.end());
    if (data.length() > 400) {
        compress_properly(vec);
    }
    std::vector<uint8_t> to_send = { uint8_t(*m_client_id), uint8_t(':') };
    to_send.insert(to_send.end(), vec.begin(), vec.end());
    m_udp_socket.send_to(buffer(to_send.data(), to_send.size()), m_udp_endpoint);
}
std::string Launcher::get_public_key() {
    return m_identity->PublicKey;
}

