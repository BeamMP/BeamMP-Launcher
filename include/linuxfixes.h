/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#ifndef _LINUXFIXES_H
#define _LINUXFIXES_H

#include <stdint.h>

// Translate windows sockets stuff to linux sockets
#define SOCKET uint64_t
#define SOCKADDR sockaddr
#define SOCKADDR_IN sockaddr_in
#define WSAGetLastError() errno
#define closesocket close
#define SD_BOTH SHUT_RDWR
// We dont need wsacleanup
#define WSACleanup()
#define SOCKET_ERROR -1

#define ZeroMemory(mem, len) memset(mem, 0, len)

#endif