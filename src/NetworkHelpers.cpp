#include "NetworkHelpers.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#if defined(__linux__)
#include <sys/socket.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

static uint32_t RecvHeader(SOCKET socket) {
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
void ReceiveFromGame(SOCKET socket, std::vector<char>& out_data) {
    auto header = RecvHeader(socket);
    out_data.resize(header);
    auto n = recv(socket, reinterpret_cast<char*>(out_data.data()), out_data.size(), MSG_WAITALL);
    if (n < 0) {
        throw std::runtime_error(std::string("recv() of data failed: ") + std::strerror(errno));
    } else if (n == 0) {
        throw std::runtime_error("Game disconnected");
    }
}
