///
/// Created by Anonymous275 on 1/26/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include <string>

class IPC {
public:
    IPC() = delete;
    IPC(const char* MemID, const char* SemID, const char* SemID2, size_t Size) noexcept;
    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] char* c_str() const noexcept;
    void send(const std::string& msg) noexcept;
    [[nodiscard]] void* raw() const noexcept;
    const std::string& msg() noexcept;
    void confirm_receive() noexcept;
    void receive();
    ~IPC() noexcept;
private:
    void* SemConfHandle_;
    void* MemoryHandle_;
    void* SemHandle_;
    std::string Msg_;
    size_t Size_;
    char* Data_;
};
