///
/// Created by Anonymous275 on 1/18/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include <discord_rpc.h>
#include "Launcher.h"
#include "Logger.h"

void Launcher::RichPresence() {
    Discord_Initialize("629743237988352010", nullptr, 1, nullptr);
    while(!Shutdown.load()) {
        DiscordRichPresence discordPresence;
        memset(&discordPresence, 0, sizeof(discordPresence));
        discordPresence.state = DiscordMessage.c_str();
        discordPresence.startTimestamp = DiscordTime;
        discordPresence.largeImageKey = "mainlogo";
        Discord_UpdatePresence(&discordPresence);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    Discord_ClearPresence();
}

void Launcher::RunDiscordRPC() {
    //DiscordRPC = std::thread(&Launcher::RichPresence, this);
}