#include "NetworkHelpers.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <stdexcept>

using asio::ip::tcp;

static uint32_t RecvHeader(tcp::socket& socket) {
    std::array<uint8_t, sizeof(uint32_t)> header_buffer {};
    asio::error_code ec;
    auto n = asio::read(socket, asio::buffer(header_buffer), ec);
    if (ec) {
        throw std::runtime_error(std::string("recv() of header failed: ") + ec.message());
    }
    if (n == 0) {
        throw std::runtime_error("Game disconnected");
    }
    return *reinterpret_cast<uint32_t*>(header_buffer.data());
}

/// Throws!!!
void ReceiveFromGame(tcp::socket& socket, std::vector<char>& out_data) {
    auto header = RecvHeader(socket);
    out_data.resize(header);
    asio::error_code ec;
    auto n = asio::read(socket, asio::buffer(out_data), ec);
    if (ec) {
        throw std::runtime_error(std::string("recv() of data failed: ") + ec.message());
    }
    if (n == 0) {
        throw std::runtime_error("Game disconnected");
    }
}
