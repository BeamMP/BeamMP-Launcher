///
/// Created by Anonymous275 on 6/17/2020
///
#include "Network/network.h"
#include "Security/Game.h"
#include "Security/Enc.h"
#include "Memory.hpp"
#include "Startup.h"
#include <iostream>
#include <thread>

Memory Game;
std::string GameVer(HANDLE processHandle, long long Address){
    //lib_Beam
    Address += 0x1B0688;
    std::vector<int> Off = {0x0,0xBF0};
    return Game.ReadPointerText(processHandle,Address,Off);
}
std::string LoadedMap(HANDLE processHandle, long long Address){
    //lib_Beam
    //History : 0x1B0688
    Address += 0x1A1668;
    std::vector<int> Off = {0x140,0x0};
    return Game.ReadPointerText(processHandle,Address,Off);
}

void MemoryInit(){
    if(Dev)return;
    Game.PID = GamePID;
    if(Game.PID == 0)exit(4);
    HANDLE processHandle;
    long long ExeBase; //BeamNG.drive.x64.exe
    long long Lib1 = 0x180000000; //libbeamng.x64.dll Contains Vehicle Data
    Game.GetDebugPrivileges();
    processHandle = OpenProcess(PROCESS_ALL_ACCESS, false, Game.PID);
    ExeBase = Game.GetModuleBase(processHandle, "BeamNG.drive.x64.exe");

    std::string Map,Temp;
    while(true){
        Map = LoadedMap(processHandle,Lib1);
        if(!Map.empty() && Map != "-1" && Map.find("/info.json") != std::string::npos && Map != Temp){
            if(MStatus.find(Map) == std::string::npos)exit(5);
            Temp = Map;
        }
        Map.clear();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}