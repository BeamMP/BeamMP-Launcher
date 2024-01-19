#include "Platform.h"
#if defined(PLATFORM_LINUX)

#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <filesystem>

void plat::ReLaunch(int argc, char* args[]) {
    spdlog::warn("Not implemented: {}", __func__);
}
void plat::URelaunch(int argc, char** argv) {
    spdlog::warn("Not implemented: {}", __func__);
}
void plat::set_console_title(const std::string& title) {
    //fmt::print("\x1b]2;{}\0", title);
}
void plat::clear_screen() {
    // unwanted on linux, due to the ability to run this as a command in an existing console.
}
std::string plat::get_game_dir_magically() {
    spdlog::warn("Not implemented: {}", __func__);
    for (const auto& path : { "" }) {
        if (std::filesystem::exists(std::filesystem::path(path) / "BeamNG.drive.exe")) {
            return path;
        }
    }
    return "";
}

#include <iostream>
#include <filesystem>

std::string plat::ask_for_folder() {
    spdlog::info("Asking user for path");
    fmt::print("====\n"
               "Couldn't find game directory, please enter the path to the game (the folder that contains 'BeamNG.drive.exe')\n"
               "Path: ");
    std::string folder;
    std::getline(std::cin, folder);
    while (!std::filesystem::exists(std::filesystem::path(folder) / "BeamNG.drive.exe")) {
        fmt::print("This folder ('{}') doesn't contain 'BeamNG.drive.exe', please try again.\n", folder);
        fmt::print("Path: ");
        std::getline(std::cin, folder);
    }
    fmt::print("Thank you!\n====\n");
    spdlog::info("Game path found");
    return folder;
}
#endif
