#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#if defined(__linux__)
#include <sys/socket.h>
#include "linuxfixes.h"
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace Utils {
    inline std::vector<std::string> Split(const std::string& String, const std::string& delimiter) {
        std::vector<std::string> Val;
        size_t pos;
        std::string token, s = String;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            token = s.substr(0, pos);
            if (!token.empty())
                Val.push_back(token);
            s.erase(0, pos + delimiter.length());
        }
        if (!s.empty())
            Val.push_back(s);
        return Val;
    };

    template<typename T>
    inline std::vector<char> PrependHeader(const T& data) {
        std::vector<char> size_buffer(4);
        uint32_t len = data.size();
        std::memcpy(size_buffer.data(), &len, 4);
        std::vector<char> buffer;
        buffer.reserve(size_buffer.size() + data.size());
        buffer.insert(buffer.begin(), size_buffer.begin(), size_buffer.end());
        buffer.insert(buffer.end(), data.begin(), data.end());
        return buffer;
    }

    inline uint32_t RecvHeader(SOCKET socket) {
        std::array<uint8_t, sizeof(uint32_t)> header_buffer {};
        auto n = recv(socket, reinterpret_cast<char*>(header_buffer.data()), header_buffer.size(), MSG_WAITALL);
        if (n < 0) {
            throw std::runtime_error(std::string("recv() of header failed: ") + std::strerror(errno));
        } else if (n == 0) {
            throw std::runtime_error("Game disconnected");
        }
        return *reinterpret_cast<uint32_t*>(header_buffer.data());
    }

    /// Throws!!!
    inline void ReceiveFromGame(SOCKET socket, std::vector<char>& out_data) {
        auto header = RecvHeader(socket);
        out_data.resize(header);
        auto n = recv(socket, reinterpret_cast<char*>(out_data.data()), out_data.size(), MSG_WAITALL);
        if (n < 0) {
            throw std::runtime_error(std::string("recv() of data failed: ") + std::strerror(errno));
        } else if (n == 0) {
            throw std::runtime_error("Game disconnected");
        }
    }
};
