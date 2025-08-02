/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#include "Http.h"
#include "Logger.h"
#include "Network/network.hpp"
#include "Security/Init.h"
#include "Startup.h"
#include "Utils.h"
#include <curl/curl.h>
#include <iostream>
#include <thread>
#include "Options.h"

Options options;

[[noreturn]] void flush() {
    while (true) {
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main(int argc, const char** argv) try {
#if defined(_WIN32)
    system("cls");
#elif defined(__linux__)
    system("clear");
#endif

#ifdef DEBUG
    std::thread th(flush);
    th.detach();
#endif

    curl_global_init(CURL_GLOBAL_ALL);

    GetEP(Utils::ToWString(std::string(argv[0])).c_str());

    InitLog();
    ConfigInit();
    InitOptions(argc, argv, options);
    InitLauncher();

    info("IMPORTANT: You MUST keep this window open to play BeamMP!");

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
    info("If this keeps happening, contact us on either: Forum: https://forum.beammp.com, Discord: https://discord.gg/beammp");
    std::this_thread::sleep_for(std::chrono::seconds(5));
}
