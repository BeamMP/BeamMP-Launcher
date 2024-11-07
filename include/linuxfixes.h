#ifndef _LINUXFIXES_H
#define _LINUXFIXES_H

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <netinet/tcp.h>

// Translate windows sockets stuff to linux sockets
#define SOCKET uint64_t
#define SOCKADDR sockaddr
#define SOCKADDR_IN sockaddr_in
#define WSAGetLastError() errno
#define closesocket close
#define SD_BOTH SHUT_RDWR
#define WSACleanup()
#define SOCKET_ERROR -1
#define INVALID_SOCKET (-1)
#define ZeroMemory(mem, len) memset(mem, 0, len)

// Socket constants that need to be defined
#ifndef AF_INET
#define AF_INET PF_INET
#endif

#ifndef SOCK_STREAM 
#define SOCK_STREAM 1
#endif

#ifndef IPPROTO_TCP
#define IPPROTO_TCP 6
#endif

#ifndef AI_PASSIVE
#define AI_PASSIVE 0x00000001
#endif

#ifdef __APPLE__
// MacOS specific includes already covered above
// Just need to redefine SOCKET as int for MacOS
#undef SOCKET
#define SOCKET int
#endif

#endif
