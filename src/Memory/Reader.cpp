///
/// Created by Anonymous275 on 6/17/2020
///
#include "Memory.hpp"
#include <iostream>
#include <thread>
extern bool MPDEV;
Memory Game;
std::string GameVer(HANDLE processHandle, long long Address){
    //lib_Beam
    Address += 0x1B0688;
    std::vector<int> Off = {0x0,0xBF0};
    return Game.ReadPointerText(processHandle,Address,Off);
}
std::string LoadedMap(HANDLE processHandle, long long Address){
    //lib_Beam
    Address += 0x1B0688;
    std::vector<int> Off = {0x2F8,0x0};
    return Game.ReadPointerText(processHandle,Address,Off);
}
void SetPID(DWORD PID){
    Game.PID = PID;
}
[[noreturn]] void MemoryInit(){
    if(Game.PID == 0 && !MPDEV)exit(4);
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
            std::cout << "You just loaded: " << Map << std::endl;
            Temp = Map;
        }
        Map.clear();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}