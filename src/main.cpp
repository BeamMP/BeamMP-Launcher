#include "Launcher.h"
#include "Platform.h"
#include "ServerNetwork.h"
#include <boost/system/detail/errc.hpp>
#include <boost/system/detail/error_category.hpp>
#include <iostream>
#include <thread>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

/// Sets up a file- and console logger and makes it the default spdlog logger.
static void setup_logger(bool debug = false);
static std::shared_ptr<spdlog::logger> default_logger {};

int main(int argc, char** argv) {
    bool enable_debug = false;
    bool enable_dev = false;
    int custom_port = 0;
    std::string_view invalid_arg;
    std::string all_args = std::string { argv[0] } + " ";
    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        all_args += "'" + std::string(arg) + "' ";
        std::string_view next(i + 1 < argc ? argv[i + 1] : "");
        // --debug flag enables debug printing in console
        if (arg == "--debug") {
            enable_debug = true;
            // --dev enables developer mode (game is not started)
        } else if (arg == "--dev") {
            enable_dev = true;
        } else if (arg == "0" && next == "0") {
            enable_dev = true;
            ++i;
            // an argument that is all digits and not 0 is a custom port, backwards-compat
        } else if (std::all_of(arg.begin(), arg.end(), [](char c) { return std::isdigit(c); })) {
            custom_port = std::stoi(arg.data());
        } else {
            invalid_arg = arg;
        }
    }
    setup_logger(enable_debug || enable_dev);

    spdlog::trace("BeamMP Launcher invoked as: {}", all_args);

    if (enable_debug) {
        spdlog::debug("Debug mode enabled");
    }
    if (enable_dev) {
        spdlog::debug("Development mode enabled");
    }
    if (custom_port != 0) {
        spdlog::debug("Custom port set: {}", custom_port);
    }

    if (!invalid_arg.empty()) {
        spdlog::warn("One or more invalid argument(s) passed via commandline switches, last one: '{}'. This argument was ignored.", invalid_arg);
    }

    plat::clear_screen();
    plat::set_console_title(fmt::format("BeamMP Launcher v{}.{}.{}", PRJ_VERSION_MAJOR, PRJ_VERSION_MINOR, PRJ_VERSION_PATCH));
    spdlog::trace("BeamMP Launcher v{}.{}.{}", PRJ_VERSION_MAJOR, PRJ_VERSION_MINOR, PRJ_VERSION_PATCH);

    spdlog::info("BeamMP Launcher v{}.{}.{} is a PRE-RELEASE build. Please report any errors immediately at https://github.com/BeamMP/BeamMP-Launcher.",
        PRJ_VERSION_MAJOR, PRJ_VERSION_MINOR, PRJ_VERSION_PATCH);

    
    Launcher launcher {};

    std::filesystem::path arg0(argv[0]);
    launcher.set_exe_name(arg0.filename().generic_string());
    launcher.set_exe_path(arg0.parent_path());

    if (custom_port > 0) {
        launcher.set_port(custom_port);
    }

    if (!enable_dev) {
        //&launcher.check_for_updates(argc, argv);
    } else {
        spdlog::debug("Skipping update check due to dev mode");
    }

    launcher.find_game();

    if (!enable_dev) {
        launcher.pre_game();
        launcher.start_game();
    }

    launcher.start_network();
    /*
    Launcher launcher {};

    std::filesystem::path arg0(argv[0]);
    launcher.set_exe_name(arg0.filename().generic_string());
    launcher.set_exe_path(arg0.parent_path());

    try {
        ServerNetwork sn(launcher, ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 30814));
        sn.run();
    } catch (const std::exception& e) {
        spdlog::error("Connection to server closed: {}", e.what());
    }
    spdlog::info("Shutting down.");
    */
}

void setup_logger(bool debug) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%H:%M:%S] [%^%l%$] %v");
    if (debug) {
        console_sink->set_level(spdlog::level::debug);
    } else {
        console_sink->set_level(spdlog::level::info);
    }

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Launcher.log", true);
    file_sink->set_level(spdlog::level::trace);
    file_sink->set_pattern("[%H:%M:%S.%e] [%t] [%L] %v");

    default_logger = std::make_shared<spdlog::logger>(spdlog::logger("default", { console_sink, file_sink }));

    default_logger->set_level(spdlog::level::trace);
    default_logger->flush_on(spdlog::level::trace);

    spdlog::set_default_logger(default_logger);

    spdlog::debug("Logger initialized");
}

/*
int oldmain(int argc, char* argv[]) {
#ifdef DEBUG
    std::thread th(flush);
    th.detach();
#endif
    GetEP(argv[0]);

    InitLauncher(argc, argv);

    try {
        LegitimacyCheck();
    } catch (std::exception& e) {
        fatal("Main 1 : " + std::string(e.what()));
    }

    StartProxy();
    PreGame(GetGameDir());
    InitGame(GetGameDir());
    CoreNetwork();

    /// TODO: make sure to use argv[0] for everything that should be in the same dir (mod down ect...)
}
*/
