///
/// Created by Anonymous275 on 1/26/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Memory/IPC.h"

IPC::IPC(uint32_t ID, size_t Size) noexcept : Size_(Size) {
    std::string Sem{"MP_S" + std::to_string(ID)},
    SemConf{"MP_SC" + std::to_string(ID)},
    Mem{"MP_IO" + std::to_string(ID)};

    SemHandle_ = OpenSemaphoreA(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, Sem.c_str());
    if(SemHandle_ == nullptr) {
        SemHandle_ = CreateSemaphoreA(nullptr, 0, 1, Sem.c_str());
    }
    SemConfHandle_ = OpenSemaphoreA(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, SemConf.c_str());
    if(SemConfHandle_ == nullptr) {
        SemConfHandle_ = CreateSemaphoreA(nullptr, 0, 1, SemConf.c_str());
    }
    MemoryHandle_ = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, Mem.c_str());
    if(MemoryHandle_ == nullptr) {
        MemoryHandle_ = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, DWORD(Size), Mem.c_str());
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

bool IPC::mem_used(uint32_t MemID) noexcept {
    std::string Mem{"MP_IO" + std::to_string(MemID)};
    HANDLE MEM = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, Mem.c_str());
    bool used = MEM != nullptr;
    UnmapViewOfFile(MEM);
    return used;
}


