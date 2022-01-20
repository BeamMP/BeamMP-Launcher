///
/// Created by Anonymous275 on 12/26/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#define WIN32_LEAN_AND_MEAN
#include "Launcher.h"
#include "Logger.h"
#include "BeamNG.h"
#include "Http.h"
#include <csignal>
#include <windows.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <comutil.h>

LONG WINAPI CrashHandler(EXCEPTION_POINTERS* p) {
    LOG(ERROR) << "CAUGHT EXCEPTION! Code " << p->ExceptionRecord->ExceptionCode;
    return EXCEPTION_EXECUTE_HANDLER;
}

std::atomic<bool> Launcher::Shutdown{false}, Launcher::Exit{false};
Launcher::Launcher(int argc, char* argv[]) : CurrentPath(std::filesystem::path(argv[0])), DiscordMessage("Just launched") {
    Launcher::StaticAbort(this);
    DiscordTime = std::time(nullptr);
    Log::Init();
    WindowsInit();
    SetUnhandledExceptionFilter(CrashHandler);
    LOG(INFO) << "Starting Launcher V" << FullVersion;
    UpdateCheck();
}

void Launcher::Abort() {
    Shutdown.store(true);
    if(DiscordRPC.joinable()) {
        DiscordRPC.join();
    }
    if(!MPUserPath.empty()) {
        ResetMods();
    }
    if(GamePID != 0) {
        auto Handle = OpenProcess(PROCESS_TERMINATE, false, DWORD(GamePID));
        TerminateProcess(Handle, 0);
        CloseHandle(Handle);
    }
}

Launcher::~Launcher() {
    if(!Shutdown.load()) {
        Abort();
    }
}

void ShutdownHandler(int sig) {
    Launcher::StaticAbort();
    while(HTTP::isDownload) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    LOG(INFO) << "Got termination signal (" << sig << ")";
    while(!Launcher::getExit()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void Launcher::StaticAbort(Launcher* Instance) {
    static Launcher* Address;
    if(Instance) {
        Address = Instance;
        return;
    }
    Address->Abort();
}

void Launcher::WindowsInit() {
    system("cls");
    SetConsoleTitleA(("BeamMP Launcher v" + FullVersion).c_str());
    signal(SIGINT, ShutdownHandler);
    signal(SIGTERM, ShutdownHandler);
    signal(SIGABRT, ShutdownHandler);
    signal(SIGBREAK, ShutdownHandler);
}

void Launcher::LaunchGame() {
    if(BeamNG::GetProcessID() != 0) {
        LOG(FATAL) << "Game is already running, please close it and try again!";
        throw ShutdownException("Fatal Error");
    }
    VersionParser GameVersion(BeamVersion);
    if(GameVersion.data[0] > SupportedVersion.data[0]) {
        LOG(FATAL) << "BeamNG V" << BeamVersion << " not yet supported, please wait until we update BeamMP!";
        throw ShutdownException("Fatal Error");
    } else if(GameVersion.data[0] < SupportedVersion.data[0]) {
        LOG(FATAL) << "BeamNG V" << BeamVersion << " not supported, please update and launch the new update!";
        throw ShutdownException("Fatal Error");
    } else if(GameVersion > SupportedVersion) {
        LOG(WARNING) << "BeamNG V" << BeamVersion << " is slightly newer than recommended, this might cause issues!";
    } else if(GameVersion < SupportedVersion) {
        LOG(WARNING) << "BeamNG V" << BeamVersion << " is slightly older than recommended, this might cause issues!";
    }

    ShellExecuteA(nullptr, nullptr, "steam://rungameid/284160", nullptr, nullptr, SW_SHOWNORMAL);
    //ShowWindow(GetConsoleWindow(), HIDE_WINDOW);
}

void Launcher::WaitForGame() {
    LOG(INFO) << "Waiting for the game, please start BeamNG manually in case of steam issues";
    do{
        GamePID = BeamNG::GetProcessID();
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }while(GamePID == 0 && !Shutdown.load());
    if(Shutdown.load())return;
    if(GamePID == 0) {
        LOG(FATAL) << "Game process not found! aborting";
        throw ShutdownException("Fatal Error");
    }
    LOG(INFO) << "Game found! PID " << GamePID;
    setDiscordMessage("In menus");
    //TODO: Inject then start IPC
    while(!Shutdown.load() && BeamNG::GetProcessID() != 0) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    LOG(INFO) << "Game process was lost";
}

std::string QueryValue(HKEY& hKey, const char* Name) {
    DWORD keySize;
    BYTE buffer[16384];
    if(RegQueryValueExA(hKey, Name, nullptr, nullptr, buffer, &keySize) == ERROR_SUCCESS) {
        return {(char*)buffer, keySize-1};
    }
    return {};
}
std::string Launcher::GetLocalAppdata() {
    PWSTR folderPath = nullptr;

    HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &folderPath);
    if (!SUCCEEDED(hr))return {};

    _bstr_t bstrPath(folderPath);
    std::string Path((char*)bstrPath);
    CoTaskMemFree(folderPath);

    if(!Path.empty()) {
        Path += "\\BeamNG.drive\\";
        VersionParser GameVer(BeamVersion);
        Path += GameVer.split[0] + '.' + GameVer.split[1] + '\\';
        return Path;
    }
    return {};
}
void Launcher::QueryRegistry() {
    HKEY BeamNG;
    LONG RegRes = RegOpenKeyExA(HKEY_CURRENT_USER, R"(Software\BeamNG\BeamNG.drive)", 0, KEY_READ, &BeamNG);
    if(RegRes == ERROR_SUCCESS) {
        BeamRoot = QueryValue(BeamNG, "rootpath");
        BeamVersion = QueryValue(BeamNG, "version");
        BeamUserPath = QueryValue(BeamNG, "userpath_override");
        RegCloseKey(BeamNG);
        if(BeamUserPath.empty() && !BeamVersion.empty()) {
            BeamUserPath = GetLocalAppdata();
        } else if(!BeamUserPath.empty() && !BeamVersion.empty()) {
            VersionParser GameVer(BeamVersion);
            BeamUserPath += GameVer.split[0] + '.' + GameVer.split[1] + '\\';
        }
        if(!BeamUserPath.empty()) {
            MPUserPath = BeamUserPath + "mods\\multiplayer";
        }
        if(!BeamRoot.empty() && !BeamVersion.empty() && !BeamUserPath.empty())return;
    }
    LOG(FATAL) << "Please launch the game at least once, failed to read registry key Software\\BeamNG\\BeamNG.drive";
    throw ShutdownException("Fatal Error");
}

void Launcher::AdminRelaunch() {
    system("cls");
    ShellExecuteA(nullptr, "runas", CurrentPath.string().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    ShowWindow(GetConsoleWindow(),0);
    throw ShutdownException("Relaunching");
}

void Launcher::Relaunch() {
    ShellExecuteA(nullptr, "open", CurrentPath.string().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    ShowWindow(GetConsoleWindow(),0);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    throw ShutdownException("Relaunching");
}

const std::string& Launcher::getFullVersion() {
    return FullVersion;
}

const std::string &Launcher::getVersion() {
    return Version;
}

const std::string& Launcher::getUserRole() {
    return UserRole;
}

bool Launcher::Terminated() noexcept {
    return Shutdown.load();
}

bool Launcher::getExit() noexcept {
    return Exit.load();
}

void Launcher::setExit(bool exit) noexcept {
    Exit.store(exit);
}
