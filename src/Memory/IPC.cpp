///
/// Created by Anonymous275 on 1/26/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Memory/IPC.h"

IPC::IPC(const char* MemID, const char* SemID, const char* SemID2, size_t Size) noexcept : Size_(Size) {
    SemHandle_ = OpenSemaphoreA(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, SemID);
    if(SemHandle_ == nullptr) {
        SemHandle_ = CreateSemaphoreA(nullptr, 0, 2, SemID);
    }
    SemConfHandle_ = OpenSemaphoreA(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, SemID2);
    if(SemConfHandle_ == nullptr) {
        SemConfHandle_ = CreateSemaphoreA(nullptr, 0, 2, SemID2);
    }
    MemoryHandle_ = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, MemID);
    if(MemoryHandle_ == nullptr) {
        MemoryHandle_ = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, DWORD(Size), MemID);
    }
    Data_ = (char*)MapViewOfFile(MemoryHandle_, FILE_MAP_ALL_ACCESS, 0, 0, Size);
}

void IPC::confirm_receive() noexcept {
    ReleaseSemaphore(SemConfHandle_, 1, nullptr);
}

void IPC::send(const std::string& msg) noexcept {
    size_t Size = msg.size();
    memcpy(Data_, &Size, sizeof(size_t));
    memcpy(Data_ + sizeof(size_t), msg.c_str(), Size);
    memset(Data_ + sizeof(size_t) + Size, 0, 3);
    ReleaseSemaphore(SemHandle_, 1, nullptr);
    SendTimeout = WaitForSingleObject(SemConfHandle_, 5000) == WAIT_TIMEOUT;
}

void IPC::receive() noexcept {
    RcvTimeout = WaitForSingleObject(SemHandle_, 5000) == WAIT_TIMEOUT;
}

void IPC::try_receive() noexcept {
    RcvTimeout = WaitForSingleObject(SemHandle_, 0) == WAIT_TIMEOUT;
}

size_t IPC::size() const noexcept {
    return Size_;
}

char* IPC::c_str() const noexcept {
    return Data_ + sizeof(size_t);
}

void* IPC::raw() const noexcept {
    return Data_ + sizeof(size_t);
}

const std::string& IPC::msg() noexcept {
    size_t Size;
    memcpy(&Size, Data_, sizeof(size_t));
    Msg_ = std::string(c_str(), Size);
    return Msg_;
}

bool IPC::receive_timed_out() const noexcept {
    return RcvTimeout;
}

bool IPC::send_timed_out() const noexcept {
    return SendTimeout;
}

IPC::~IPC() noexcept {
    UnmapViewOfFile(Data_);
    CloseHandle(SemHandle_);
    CloseHandle(MemoryHandle_);
}


