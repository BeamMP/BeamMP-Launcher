///
/// Created by Anonymous275 on 1/18/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///
#ifndef DEBUG
#include <discord_rpc.h>
#include "Launcher.h"
#include "Logger.h"
#include <ctime>


void handleReady(const DiscordUser* u) {}
void handleDisconnected(int errcode, const char* message) {}
void handleError(int errcode, const char* message) {
    LOG(ERROR) << "Discord error: " << message;
}

void Launcher::UpdatePresence() {
    auto currentTime = std::time(nullptr);
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));
    discordPresence.state = DiscordMessage.c_str();
    discordPresence.largeImageKey = "mainlogo";
    discordPresence.startTimestamp = currentTime - (currentTime - DiscordTime);
    discordPresence.endTimestamp = 0;
    DiscordTime = currentTime;
    Discord_UpdatePresence(&discordPresence);
}

void Launcher::setDiscordMessage(const std::string& message) {
    DiscordMessage = message;
    UpdatePresence();
}

void Launcher::RichPresence() {
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    handlers.ready = handleReady;
    handlers.errored = handleError;
    handlers.disconnected = handleDisconnected;
    Discord_Initialize("629743237988352010", &handlers, 1, nullptr);
    UpdatePresence();
    while(!Shutdown.load()) {
        Discord_RunCallbacks();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    Discord_ClearPresence();
    Discord_Shutdown();
}

void Launcher::RunDiscordRPC() {
    DiscordRPC = std::thread(&Launcher::RichPresence, this);
}
#else
#include "Launcher.h"
void Launcher::setDiscordMessage(const std::string& message) {
    DiscordMessage = message;
}
void Launcher::RunDiscordRPC() {
    DiscordRPC = std::thread(&Launcher::RichPresence, this);
}
void Launcher::RichPresence() {};
void Launcher::UpdatePresence() {};
#endif