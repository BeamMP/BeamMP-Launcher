///
/// Created by Anonymous275 on 12/26/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include "Launcher.h"
#include "Logger.h"

int main(int argc, char* argv[]) {
    try {
        Launcher launcher(argc, argv);
        launcher.RunDiscordRPC();
        launcher.LoadConfig();
        launcher.CheckKey();
        launcher.QueryRegistry();
        //UI call
        launcher.SetupMOD();
        launcher.LaunchGame();
        launcher.WaitForGame();
        LOG(INFO) << "Launcher shutting down";
    } catch (const ShutdownException& e) {
        LOG(INFO) << "Launcher shutting down with reason: " << e.what();
    } catch (const std::exception& e) {
        LOG(FATAL) << e.what();
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    Launcher::setExit(true);
    return 0;
}
