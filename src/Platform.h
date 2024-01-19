#pragma once

/// Platform-specific code
#include <string>
namespace plat {

void clear_screen();
void set_console_title(const std::string& title);
void URelaunch(int argc,char* args[]);
void ReLaunch(int argc,char*args[]);
std::string get_game_dir_magically();
std::string ask_for_folder();

}