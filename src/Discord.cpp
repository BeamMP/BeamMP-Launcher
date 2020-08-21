///
/// Created by Anonymous275 on 7/16/2020
///
#include "Discord/discord_rpc.h"
#include "Security/Enc.h"
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
void DASM();
int64_t StartTime;
void updateDiscordPresence(){
    //if (SendPresence) {
    //char buffer[256];
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));
    std::string P = Sec("Playing with friends!");
    discordPresence.state = P.c_str();
    //sprintf(buffer, "Frustration level: %d", FrustrationLevel);
    //discordPresence.details = buffer;
    discordPresence.startTimestamp = StartTime;
    //discordPresence.endTimestamp = time(0) + 5 * 60;
    std::string L = Sec("mainlogo");
    discordPresence.largeImageKey = L.c_str();
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
        LocalEnc(User->username),
        LocalEnc(User->discriminator),
        LocalEnc(User->userId)
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
    std::string a = Sec("629743237988352010");
    Discord_Initialize(a.c_str(), &handlers, 1,nullptr);
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
[[noreturn]] void SecurityLoop(){
    std::string t,t1,t2;
    while(true){
        if(DiscordInfo != nullptr){
            if(t.empty()){
                t = LocalDec(DiscordInfo->Name);
                t1 = LocalDec(DiscordInfo->Tag);
                t2 = LocalDec(DiscordInfo->DID);
            }else if(t2 != LocalDec(DiscordInfo->DID) ||
            t != LocalDec(DiscordInfo->Name) || t1 != LocalDec(DiscordInfo->Tag))DiscordInfo = nullptr;
        }else if(!t.empty())DiscordInfo->DID.clear();
        DASM();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
void DMain(){
    auto*S = new std::thread(SecurityLoop);
    S->detach();
    delete S;
    discordInit();
    Loop();
}
std::string GetDName(){
    return LocalDec(DiscordInfo->Name);
}
std::string GetDTag(){
    return LocalDec(DiscordInfo->Tag);
}
std::string GetDID(){
    return LocalDec(DiscordInfo->DID);
}
void DAboard(){
    DiscordInfo = nullptr;
}
void ErrorAboard(){
    error(Sec("Discord timeout! please start the discord app and try again after 30 secs"));
    std::this_thread::sleep_for(std::chrono::seconds(5));
    exit(6);
}
void Discord_Main(){
    std::thread t1(DMain);
    t1.detach();
    info(Sec("Connecting to discord client..."));
    int C = 0;
    while(DiscordInfo == nullptr && C < 80){
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        C++;
    }
    if(DiscordInfo == nullptr)ErrorAboard();
}
