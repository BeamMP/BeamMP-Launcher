#pragma once

#if defined(__linux__)
#include "linuxfixes.h"
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include <vector>

void ReceiveFromGame(SOCKET socket, std::vector<char>& out_data);
