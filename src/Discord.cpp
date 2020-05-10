///
/// Created by Anonymous275 on 3/25/2020
///

#define _CRT_SECURE_NO_WARNINGS
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include "include/discord_rpc.h"
#include <chrono>
#include <thread>
#include <vector>
extern bool MPDEV;
static const char* APPLICATION_ID = "629743237988352010";
static int64_t StartTime;
static int SendPresence = 1;
static std::vector<std::string> LocalInfo;

std::vector<std::string> GetDiscordInfo(){
    return LocalInfo;
}

static void updateDiscordPresence()
{
    if (SendPresence) {
        char buffer[256];
        DiscordRichPresence discordPresence;
        memset(&discordPresence, 0, sizeof(discordPresence));
        discordPresence.state = "Playing with friends!";
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
    }
    else {
        Discord_ClearPresence();
    }
}

static void handleDiscordReady(const DiscordUser* connectedUser)
{
    /*printf("\nDiscord: connected to user %s#%s - %s\n",
           connectedUser->username,
           connectedUser->discriminator,
           connectedUser->userId);*/
    LocalInfo.emplace_back(connectedUser->username);
    LocalInfo.emplace_back(connectedUser->discriminator);
    LocalInfo.emplace_back(connectedUser->userId);
}

static void handleDiscordDisconnected(int errcode, const char* message)
{
    if(MPDEV)printf("\nDiscord: disconnected (%d: %s)\n", errcode, message);
}

static void handleDiscordError(int errcode, const char* message)
{
    if(MPDEV)printf("\nDiscord: error (%d: %s)\n", errcode, message);
}

static void handleDiscordJoin(const char* secret)
{
    if(MPDEV)printf("\nDiscord: join (%s)\n", secret);
}

static void handleDiscordSpectate(const char* secret)
{
    if(MPDEV)printf("\nDiscord: spectate (%s)\n", secret);
}

static void handleDiscordJoinRequest(const DiscordUser* request)
{
    /* int response = -1;
     char yn[4];
     printf("\nDiscord: join request from %s#%s - %s\n",
            request->username,
            request->discriminator,
            request->userId);
     do {
         printf("Accept? (y/n)");
         if (!prompt(yn, sizeof(yn))) {
             break;
         }

         if (!yn[0]) {
             continue;
         }

         if (yn[0] == 'y') {
             response = DISCORD_REPLY_YES;
             break;
         }

         if (yn[0] == 'n') {
             response = DISCORD_REPLY_NO;
             break;
         }
     } while (1);
     if (response != -1) {
         Discord_Respond(request->userId, response);
     }*/
}

static void discordInit()
{
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    handlers.ready = handleDiscordReady;
    handlers.disconnected = handleDiscordDisconnected;
    handlers.errored = handleDiscordError;
    handlers.joinGame = handleDiscordJoin;
    handlers.spectateGame = handleDiscordSpectate;
    handlers.joinRequest = handleDiscordJoinRequest;
    Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);
}

[[noreturn]] static void Loop()
{
    char line[512];
    char* space;

    StartTime = time(0);

    while (true) {
        updateDiscordPresence();

#ifdef DISCORD_DISABLE_IO_THREAD
        Discord_UpdateConnection();
#endif
        Discord_RunCallbacks();
        if(LocalInfo.empty()){
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }else std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
}

void Discord_Main()
{
    discordInit();
    Loop();
    Discord_Shutdown();
}

