#pragma once

#include "Packet.h"
#include "State.h"
#include <boost/asio.hpp>

using namespace boost::asio;

class Launcher;

class ServerNetwork {
public:
    ServerNetwork(Launcher& launcher, const ip::tcp::endpoint& ep);
    ~ServerNetwork();

    /// Starts and runs the connection to the server.
    /// Calls back to the Launcher.
    /// Blocking!
    void run();

private:
    void start_tcp_read();
    void start_udp_read();

    /// Reads a single packet from the TCP stream. Blocks all other reads (not writes).
    void tcp_read(std::function<void(bmp::Packet&&)> handler);
    /// Writes the packet to the TCP stream. Blocks all other writes.
    void tcp_write(bmp::Packet&& packet, std::function<void(boost::system::error_code)> handler = nullptr);

    /// Reads a packet from the given UDP socket, returning the client's endpoint as an out-argument.
    void udp_read(std::function<void(ip::udp::endpoint&&, bmp::Packet&&)> handler);
    /// Sends a packet to the specified UDP endpoint via the UDP socket.
    void udp_write(bmp::Packet& packet);

    void handle_packet(bmp::Packet&& packet);
    void handle_identification(const bmp::Packet& packet);
    void handle_authentication(const bmp::Packet& packet);
    void handle_mod_download(const bmp::Packet& packet);
    void handle_session_setup(const bmp::Packet& packet);
    void handle_playing(const bmp::Packet& packet);

    io_context m_io {};
    ip::tcp::socket m_tcp_socket { m_io };
    ip::udp::socket m_udp_socket { m_io };

    // these two tmp fields are used for temporary reading into by read, don't use anywhere else please
    bmp::Packet m_tmp_packet {};
    std::vector<uint8_t> m_tmp_header_buffer {};
    // this is used by udp read, don't touch
    std::vector<uint8_t> m_tmp_udp_buffer { static_cast<unsigned char>(std::numeric_limits<uint16_t>::max()) };

    bmp::State m_state {};

    uint64_t m_udp_magic {};

    ip::tcp::endpoint m_tcp_ep;
    ip::udp::endpoint m_udp_ep;

    Launcher& launcher;
};
