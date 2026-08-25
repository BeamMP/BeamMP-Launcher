/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/


#include "Logger.h"
#include "Startup.h"
#include "Utils.h"
#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>
#include <iostream>
#include "Options.h"
#include <mutex>
#include <queue>
#include <condition_variable>

std::mutex logMutex;
std::condition_variable logCV;
std::queue<beammp_fs_string> logQueue;
bool logThreadRunning = false;
std::thread logThread;

void logThreadFunc() {
#ifdef _WIN32
    std::wofstream LFS;
#else
    std::ofstream LFS;
#endif
    LFS.open(GetEP() + beammp_wide("Launcher.log"), std::ios_base::out);
    if (!LFS.is_open()) {
        std::cerr << "Failed to open Launcher.log: " << std::strerror(errno) << std::endl;
        return;
    }

    while (logThreadRunning || !logQueue.empty()) {
        std::unique_lock<std::mutex> lock(logMutex);
        logCV.wait(lock, [] { return !logQueue.empty() || !logThreadRunning; });

        while (!logQueue.empty()) {
            beammp_fs_string line = logQueue.front();
            logQueue.pop();
            lock.unlock();

            LFS << line;
            LFS.flush();

            lock.lock();
        }
    }
    LFS.close();
}

std::string getDate() {
    time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm local_tm = *localtime(&tt);
    std::stringstream date;
    int S = local_tm.tm_sec;
    int M = local_tm.tm_min;
    int H = local_tm.tm_hour;
    std::string Secs = (S > 9 ? std::to_string(S) : "0" + std::to_string(S));
    std::string Min = (M > 9 ? std::to_string(M) : "0" + std::to_string(M));
    std::string Hour = (H > 9 ? std::to_string(H) : "0" + std::to_string(H));
    date
        << "["
        << local_tm.tm_mday << "/"
        << local_tm.tm_mon + 1 << "/"
        << local_tm.tm_year + 1900 << " "
        << Hour << ":"
        << Min << ":"
        << Secs
        << "] ";
    return date.str();
}
void InitLog() {
    logThreadRunning = true;
    logThread = std::thread(logThreadFunc);
}
void CloseLog() {
    if (logThreadRunning) {
        logThreadRunning = false;
        logCV.notify_one();
        if (logThread.joinable()) {
            logThread.join();
        }
    }
}
void addToLog(const std::string& Line) {
    {
        std::lock_guard<std::mutex> lock(logMutex);
#ifdef _WIN32
        logQueue.push(Utils::ToWString(Line));
#else
        logQueue.push(Line);
#endif
    }
    logCV.notify_one();
}
void addToLog(const std::wstring& Line) {
    {
        std::lock_guard<std::mutex> lock(logMutex);
#ifdef _WIN32
        logQueue.push(Line);
#else
        logQueue.push(std::string(Line.begin(), Line.end()));
#endif
    }
    logCV.notify_one();
}
void info(const std::string& toPrint) {
    std::string Print = getDate() + "[INFO] " + toPrint + "\n";
    beammp_stdout << Utils::ToWString(Print);
    addToLog(Print);
}
void debug(const std::string& toPrint) {
    std::string Print = getDate() + "[DEBUG] " + toPrint + "\n";
    if (options.verbose) {
        beammp_stdout << Utils::ToWString(Print);
    }
    addToLog(Print);
}
void warn(const std::string& toPrint) {
    std::string Print = getDate() + "[WARN] " + toPrint + "\n";
    beammp_stdout << Utils::ToWString(Print);
    addToLog(Print);
}
void error(const std::string& toPrint) {
    std::string Print = getDate() + "[ERROR] " + toPrint + "\n";
    beammp_stdout << Utils::ToWString(Print);
    addToLog(Print);
}
void fatal(const std::string& toPrint) {
    std::string Print = getDate() + "[FATAL] " + toPrint + "\n";
    beammp_stdout << Utils::ToWString(Print);
    addToLog(Print);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::exit(1);
}
void except(const std::string& toPrint) {
    std::string Print = getDate() + "[EXCEP] " + toPrint + "\n";
    beammp_stdout << Utils::ToWString(Print);
    addToLog(Print);
}


#ifdef _WIN32
void info(const std::wstring& toPrint) {
    std::wstring Print = Utils::ToWString(getDate()) + L"[INFO] " + toPrint + L"\n";
    std::wcout << Print;
    addToLog(Print);
}
void debug(const std::wstring& toPrint) {
    std::wstring Print = Utils::ToWString(getDate()) + L"[DEBUG] " + toPrint + L"\n";
    if (options.verbose) {
        std::wcout << Print;
    }
    addToLog(Print);
}
void warn(const std::wstring& toPrint) {
    std::wstring Print = Utils::ToWString(getDate()) + L"[WARN] " + toPrint + L"\n";
    std::wcout << Print;
    addToLog(Print);
}
void error(const std::wstring& toPrint) {
    std::wstring Print = Utils::ToWString(getDate()) + L"[ERROR] " + toPrint + L"\n";
    std::wcout << Print;
    addToLog(Print);
}
void fatal(const std::wstring& toPrint) {
    std::wstring Print = Utils::ToWString(getDate()) + L"[FATAL] " + toPrint + L"\n";
    std::wcout << Print;
    addToLog(Print);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::exit(1);
}
void except(const std::wstring& toPrint) {
    std::wstring Print = Utils::ToWString(getDate()) + L"[EXCEP] " + toPrint + L"\n";
    std::wcout << Print;
    addToLog(Print);
}
#endif