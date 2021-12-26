///
/// Created by Anonymous275 on 12/26/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include "Launcher.h"
#include <Logger.h>

Launcher::Launcher(int argc, char **argv) : DirPath(argv[0]) {
    DirPath = DirPath.substr(0, DirPath.find_last_of("\\/") + 1);
    Log::Init(argc, argv);
    WindowsInit();
}

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
void Launcher::WindowsInit() {
    system("cls");
    SetConsoleTitleA(("BeamMP Launcher v" + FullVersion).c_str());
}
#else //WIN32
void Launcher::WindowsInit() {}
#endif //WIN32