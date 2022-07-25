///
/// Created by Anonymous275 on 1/26/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include <string>

class IPC {
public:
    IPC() = default;
    IPC(uint32_t ID, size_t Size) noexcept;
    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] char* c_str() const noexcept;
    void send(const std::string& msg) noexcept;
    [[nodiscard]] void* raw() const noexcept;
    [[nodiscard]] bool receive_timed_out() const noexcept;
    [[nodiscard]] bool send_timed_out() const noexcept;
    const std::string& msg() noexcept;
    void confirm_receive() noexcept;
    void try_receive() noexcept;
    void receive() noexcept;
    ~IPC() noexcept;
    static bool mem_used(uint32_t MemID) noexcept;
private:
    void* SemConfHandle_;
    void* MemoryHandle_;
    void* SemHandle_;
    std::string Msg_;
    bool SendTimeout;
    bool RcvTimeout;
    size_t Size_;
    char* Data_;
};
