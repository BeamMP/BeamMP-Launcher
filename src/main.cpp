// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/16/2020
///
#include "Http.h"
#include "Logger.h"
#include "Network/network.hpp"
#include "Security/Init.h"
#include "Startup.h"
#include <iostream>
#include <thread>

[[noreturn]] void flush() {
    while (true) {
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main(int argc, char** argv) try {
#ifdef DEBUG
    std::thread th(flush);
    th.detach();
#endif

    GetEP(argv[0]);

    for (int i = 0; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--skip-ssl-verify") {
            info("SSL verification skip enabled");
            HTTP::SkipSslVerify = true;
        }
    }

    InitLauncher(argc, argv);

    try {
        LegitimacyCheck();
    } catch (std::exception& e) {
        error("Failure in LegitimacyCheck: " + std::string(e.what()));
        throw;
    }

    try {
        HTTP::StartProxy();
    } catch (const std::exception& e) {
        error(std::string("Failed to start HTTP proxy: Some in-game functions may not work. Error: ") + e.what());
    }
    PreGame(GetGameDir());
    InitGame(GetGameDir());
    CoreNetwork();
} catch (const std::exception& e) {
    error(std::string("Exception in main(): ") + e.what());
    info("Closing in 5 seconds");
    std::this_thread::sleep_for(std::chrono::seconds(5));
}
