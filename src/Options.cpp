/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#include "Options.h"

#include "Logger.h"
#include <cstdlib>
#include <filesystem>

void InitOptions(int argc, const char *argv[], Options &options) {
    int i = 1;

    options.argc = argc;
    options.argv = argv;

    std::string AllOptions;
    for (int i = 0; i < argc; ++i) {
        AllOptions += std::string(argv[i]);
        if (i + 1 < argc) {
            AllOptions += " ";
        }
    }
    debug("Launcher was invoked as: '" + AllOptions + "'");


    if (argc > 2) {
        if (std::string(argv[1]) == "0" && std::string(argv[2]) == "0") {
            options.verbose = true;
            options.no_download = true;
            options.no_launch = true;
            options.no_update = true;
            warn("You are using deprecated commandline arguments, please use --dev instead");
            return;
        }
    }

    options.executable_name = std::string(argv[0]);

    while (i < argc) {
        std::string argument(argv[i]);
        if (argument == "-p" || argument == "--port") {
            if (i + 1 >= argc) {
                std::string error_message =
                    "No port specified, resorting to default (";
                error_message += std::to_string(options.port);
                error_message += ")";
                error(error_message);
                i++;
                continue;
            }

            int port = options.port;

            try {
                port = std::stoi(argv[i + 1]);
            } catch (std::exception& e) {
                error("Invalid port specified: " + std::string(argv[i + 1]) + " " + std::string(e.what()));
            }

            if (port <= 0) {
                std::string error_message =
                    "Port invalid, must be a non-zero positive "
                    "integer, resorting to default (";
                error_message += options.port;
                error_message += ")";
                error(error_message);
                i++;
                continue;
            }

            options.port = port;
            i++;
        } else if (argument == "-v" || argument == "--verbose") {
            options.verbose = true;
        } else if (argument == "--no-download") {
            options.no_download = true;
        } else if (argument == "--no-update") {
            options.no_update = true;
        } else if (argument == "--no-launch") {
            options.no_launch = true;
        } else if (argument == "--dev") {
            options.verbose = true;
            options.no_download = true;
            options.no_launch = true;
            options.no_update = true;
        } else if (argument == "--user-path") {
            if (i + 1 >= argc) {
                error("No user path specified after flag");
            }
            options.user_path = argv[i + 1];
            i++;
        } else if (argument == "--" || argument == "--game") {
            options.game_arguments = &argv[i + 1];
            options.game_arguments_length = argc - i - 1;
            break;
        } else if (argument == "--help" || argument == "-h" || argument == "/?") {
            std::cout << "USAGE:\n"
                "\t" + std::filesystem::path(options.executable_name).filename().string() + " [OPTIONS] [-- <GAME ARGS>...]\n"
                "\n"
                "OPTIONS:\n"
                "\t--port <port>    -p  Change the default listen port to <port>. This must be configured ingame, too\n"
                "\t--verbose        -v  Verbose mode, prints debug messages\n"
                "\t--no-download        Skip downloading and installing the BeamMP Lua mod\n"
                "\t--no-update          Skip applying launcher updates (you must update manually)\n"
                "\t--no-launch          Skip launching the game (you must launch the game manually)\n"
                "\t--dev                Developer mode, same as --verbose --no-download --no-launch --no-update\n"
                "\t--user-path <path>   Path to BeamNG's User Path\n"
                "\t--game <args...>     Passes ALL following arguments to the game, see also `--`\n"
                << std::flush;
            exit(0);
        } else {
            warn("Unknown option: " + argument);
        }

        i++;
    }
}
