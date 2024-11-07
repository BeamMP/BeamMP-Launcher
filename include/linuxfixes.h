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

#ifdef __APPLE__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>

#define SOCKET int
#define SOCKADDR sockaddr
#define SOCKADDR_IN sockaddr_in
#define WSAGetLastError() errno
#define closesocket close
#define SD_BOTH SHUT_RDWR
#define WSACleanup()
#define SOCKET_ERROR -1

#define ZeroMemory(mem, len) memset(mem, 0, len)
#endif

#endif
