#pragma once

#include <asio.hpp>
#include <vector>

void ReceiveFromGame(asio::ip::tcp::socket& socket, std::vector<char>& out_data);
