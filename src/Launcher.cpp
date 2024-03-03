#include "Launcher.h"
#include "Compression.h"
#include "Hashing.h"
#include "Http.h"
#include "Identity.h"
#include "Platform.h"
#include "Version.h"
#include <boost/asio.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/process.hpp>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <fmt/format.h>
#include <httplib.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <vector>

using namespace boost::asio;

namespace fs = std::filesystem;

Launcher::Launcher() {
    spdlog::debug("Launcher startup");
    m_config = Config {};
    if (!m_config->is_valid) {
        spdlog::error("Launcher config invalid!");
    }
    // try logging in immediately, for later update requests and such stuff
    try_auto_login();
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
                headers.emplace("X-BMP-Authentication", identity->PrivateKey);
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
                headers.emplace("X-BMP-Authentication", identity->PrivateKey);
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
    HTTPProxy.listen_after_bind();
}

Launcher::~Launcher() {
    m_proxy_thread.interrupt();
    m_game_thread.detach();
}

void Launcher::parse_config() {
}

void Launcher::check_for_updates(int argc, char** argv) {
    std::string LatestHash = HTTP::Get(fmt::format("https://backend.beammp.com/sha/launcher?branch={}&pk={}", m_config->branch, identity->PublicKey));
    std::string LatestVersion = HTTP::Get(fmt::format("https://backend.beammp.com/version/launcher?branch={}&pk={}", m_config->branch,identity->PublicKey));
    std::string DownloadURL = fmt::format("https://backend.beammp.com/builds/launcher?download=true"
                                          "&pk={}"
                                          "&branch={}",
        identity->PublicKey, m_config->branch);

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

    std::string LatestHash = HTTP::Get(fmt::format("https://backend.beammp.com/sha/mod?branch={}&pk={}", m_config->branch, identity->PublicKey));
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
        HTTP::Download(fmt::format("https://backend.beammp.com/builds/client?download=true&pk={}&branch={}", identity->PublicKey, m_config->branch), ZipPath.generic_string());
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
void Launcher::try_auto_login() {
    if (identity->PublicKey.empty() && ident::is_login_cached()) {
        auto login = ident::login_cached();
        if (login.has_value()) {
            *identity = login.value();
        }
    }
}

