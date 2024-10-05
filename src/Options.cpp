#include "Options.h"

#include "Logger.h"
#include <cstdlib>

void InitOptions(int argc, char *argv[], Options &options) {
    int i = 1;

    options.argc = argc;
    options.argv = argv;

    if (argc > 2)
        if (std::string(argv[1]) == "0" && std::string(argv[2]) == "0") {
            options.verbose = true;
            options.no_download = true;
            options.no_launch = true;
            options.no_update = true;
            warn("You are using deprecated commandline arguments, please use --dev instead");
            return;
        }

    options.executable_name = std::string(argv[0]);

    while (i < argc) {
        std::string argument(argv[i]);
        if (argument == "-p" || argument == "--port") {
            if (argc > i) {
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
        } else if (argument == "--") {
            options.game_arguments = &argv[i + 1];
            options.game_arguments_length = argc - i - 1;
            break;
        } else {
            warn("Unknown option: " + argument);
        }

        i++;
    }
}
