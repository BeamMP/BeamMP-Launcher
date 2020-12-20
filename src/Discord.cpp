// Copyright (c) 2020 Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/16/2020
///
#include "Discord/discord_rpc.h"
#include "Logger.h"
#include <iostream>
#include <thread>
#include <ctime>

struct DInfo{
    std::string Name;
    std::string Tag;
    std::string DID;
};
DInfo* DiscordInfo = nullptr;
int64_t StartTime;
void updateDiscordPresence(){
    //if (SendPresence) {
    //char buffer[256];
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));
    std::string P = "Playing with friends!"; ///to be revisited
    discordPresence.state = P.c_str();
    //sprintf(buffer, "Frustration level: %d", FrustrationLevel);
    //discordPresence.details = buffer;
    discordPresence.startTimestamp = StartTime;
    //discordPresence.endTimestamp = time(0) + 5 * 60;

    discordPresence.largeImageKey = "mainlogo";
    //discordPresence.smallImageKey = "logo";
    //discordPresence.partyId = "party1234";
    //discordPresence.partySize = 1;
    //discordPresence.partyMax = 6;
    //discordPresence.matchSecret = "xyzzy";
    //discordPresence.joinSecret = "join";
    //discordPresence.spectateSecret = "look";
    //discordPresence.instance = 0;
    Discord_UpdatePresence(&discordPresence);
    //}
    //else {
    // Discord_ClearPresence();
    //}
}
void handleDiscordReady(const DiscordUser* User){
    DiscordInfo = new DInfo{
        User->username,
        User->discriminator,
        User->userId
    };
}
void discordInit(){
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    handlers.ready = handleDiscordReady;
    /*handlers.disconnected = handleDiscordDisconnected;
    handlers.errored = handleDiscordError;
    handlers.joinGame = handleDiscordJoin;
    handlers.spectateGame = handleDiscordSpectate;
    handlers.joinRequest = handleDiscordJoinRequest;*/
    Discord_Initialize("629743237988352010", &handlers, 1,nullptr);
}
[[noreturn]] void Loop(){
    StartTime = time(nullptr);
    while (true) {
        updateDiscordPresence();
        #ifdef DISCORD_DISABLE_IO_THREAD
            Discord_UpdateConnection();
        #endif
        Discord_RunCallbacks();
        if(DiscordInfo == nullptr){
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }else std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void DMain(){
    discordInit();
    Loop();
}
std::string GetDName(){
    return DiscordInfo->Name;
}
std::string GetDTag(){
    return DiscordInfo->Tag;
}
std::string GetDID(){
    return DiscordInfo->DID;
}
void DAboard(){
    DiscordInfo = nullptr;
}
void ErrorAboard(){
    error("Discord timeout! please start the discord app and try again after 30 secs");
    std::this_thread::sleep_for(std::chrono::seconds(5));
    exit(6);
}
void Discord_Main(){
    std::thread t1(DMain);
    t1.detach();
    /*info("Connecting to discord client...");
    int C = 0;
    while(DiscordInfo == nullptr && C < 80){
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        C++;
    }
    if(DiscordInfo == nullptr)ErrorAboard();*/
}
