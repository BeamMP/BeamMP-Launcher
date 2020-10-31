///
/// Created by Anonymous275 on 7/16/2020
///
#include "Network/network.h"
#include "Security/Init.h"
#include "Startup.h"
#include <thread>
#include <iostream>

[[noreturn]] void aa(){
    while(true){
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main(int argc, char* argv[]) {
    #ifdef DEBUG
        std::thread gb(aa);
        gb.detach();
    #endif
    InitLauncher(argc,argv);
    CheckDir(argc,argv);
    LegitimacyCheck();
    PreGame(argc,argv,GetGameDir());
    InitGame(GetGameDir(),argv[0]);
    CoreNetwork();
}
