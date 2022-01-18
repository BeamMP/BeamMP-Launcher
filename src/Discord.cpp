///
/// Created by Anonymous275 on 1/18/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include <discord-rpc/include/discord_rpc.h>
#include "Launcher.h"
#include "Logger.h"

void Launcher::richPresence() {
    Discord_Initialize("629743237988352010", nullptr, 1,nullptr);
    int64_t Start{};
    while(!Shutdown) {
        DiscordRichPresence discordPresence;
        memset(&discordPresence, 0, sizeof(discordPresence));
        discordPresence.state = DiscordMessage.c_str();
        discordPresence.startTimestamp = Start;
        discordPresence.largeImageKey = "mainlogo";
        Discord_UpdatePresence(&discordPresence);
        Discord_RunCallbacks();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    Discord_ClearPresence();
}

void Launcher::runDiscordRPC() {
    DiscordRPC = std::thread(&Launcher::richPresence, this);
}