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
    /// Reads a single packet from the TCP stream. Blocks all other reads (not writes).
    bmp::Packet tcp_read();
    /// Writes the packet to the TCP stream. Blocks all other writes.
    void tcp_write(bmp::Packet& packet);

    /// Reads a packet from the given UDP socket, returning the client's endpoint as an out-argument.
    bmp::Packet udp_read(ip::udp::endpoint& out_ep);
    /// Sends a packet to the specified UDP endpoint via the UDP socket.
    void udp_write(bmp::Packet& packet);

    void handle_packet(const bmp::Packet& packet);
    void handle_identification(const bmp::Packet& packet);
    void handle_authentication(const bmp::Packet& packet);
    void handle_mod_download(const bmp::Packet& packet);

    io_context m_io {};
    ip::tcp::socket m_tcp_socket { m_io };
    ip::udp::socket m_udp_socket { m_io };

    bmp::State m_state {};

    uint64_t m_udp_magic {};

    Launcher& m_launcher;

    ip::tcp::endpoint m_tcp_ep;
    ip::udp::endpoint m_udp_ep;
};
