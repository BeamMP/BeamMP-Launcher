// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/16/2020
///
#include "Network/network.h"
#include "Security/Init.h"
#include "Startup.h"
#include <iostream>
#include "Logger.h"
#include <thread>


[[noreturn]] void flush(){
    while(true){
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main(int argc, char* argv[]) {
    #ifdef DEBUG
        std::thread th(flush);
        th.detach();
    #endif
    GetEP(argv[0]);
    InitLauncher(argc,argv);
    //CheckDir(argc,argv);
    try {
        LegitimacyCheck();
    }catch (std::exception&e){
        fatal("Main 1 : " + std::string(e.what()));
    }
    PreGame(GetGameDir());
    InitGame(GetGameDir());
    CoreNetwork();


    ///TODO: make sure to use argv[0] for everything that should be in the same dir (mod down ect...)
    ///move to master create branch then make the new config in json
}
