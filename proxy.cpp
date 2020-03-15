////
//// Created by Anonymous275 on 3/3/2020.
////

#define ENET_IMPLEMENTATION
#include "enet.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <cstdio>
#include <iostream>
#include <thread>
#define DEFAULT_BUFLEN 64000
#define DEFAULT_PORT "4444"
typedef struct {
    ENetHost *host;
    ENetPeer *peer;
} Client;

std::string RUDPData;
std::string RUDPToSend;


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void TCPServerThread(){
    do{

    WSADATA wsaData;
    int iResult;
    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
       std::cout <<"WSAStartup failed with error: " << iResult << std::endl;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        std::cout << "getaddrinfo failed with error: " << iResult << std::endl;
        WSACleanup();
    }

    // Create a socket for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::cout << "bind failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        std::cout << "listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
    }
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        std::cout << "accept failed with error: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
    }
    closesocket(ListenSocket);

    do {
        if(!RUDPData.empty()){
            iSendResult = send( ClientSocket, RUDPData.c_str(), int(RUDPData.length())+1, 0);
            if (iSendResult == SOCKET_ERROR) {
                std::cout << "send failed with error: " << WSAGetLastError() << std::endl;
                closesocket(ClientSocket);
                WSACleanup();
            }else{
                RUDPData.clear();
                std::cout << "Bytes sent: " << iSendResult << std::endl;
            }
        }

        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            RUDPToSend = recvbuf;
            RUDPToSend.resize(iResult-1);
            std::cout << "size : " << RUDPToSend.size() << std::endl;
            std::cout << "Data : " << RUDPToSend.c_str() << std::endl;
        }

        else if (iResult == 0)
           std::cout << "Connection closing...\n";
        else  {
            std::cout << "recv failed with error: " << WSAGetLastError() << std::endl;
            closesocket(ClientSocket);
            WSACleanup();
        }

    } while (iResult > 0);

    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        std::cout << "shutdown failed with error: " << WSAGetLastError() << std::endl;
        closesocket(ClientSocket);
        WSACleanup();
    }

    closesocket(ClientSocket);
    WSACleanup();
    }while (true);
}
#pragma clang diagnostic pop
#pragma clang diagnostic pop
void HandleEvent(ENetEvent event,Client client){
    switch (event.type){
        case ENET_EVENT_TYPE_CONNECT:
            printf("Client Connected port : %u.\n",event.peer->address.port);
            //Name Of the Server
            event.peer->data = (void *)"Connected Server";
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            printf("Received: %s\n",event.packet->data);
            RUDPData = reinterpret_cast<const char *const>(event.packet->data);
            enet_packet_destroy (event.packet);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            printf ("%s disconnected.\n", (char *)event.peer->data);
            // Reset the peer's client information.
            event.peer->data = NULL;
            break;

        case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
            printf ("%s timeout.\n", (char *)event.peer->data);
            event.peer->data = NULL;
            break;

        case ENET_EVENT_TYPE_NONE: break;
    }
}
void RUDPClientThread(){
    if (enet_initialize() != 0) {
       std::cout << "An error occurred while initializing ENet.\n";
    }


    Client client;
    ENetAddress address = {0};

    address.host = ENET_HOST_ANY;
    address.port = 30814;


    std::cout << "starting client...\n";

    enet_address_set_host(&address, "localhost");
    client.host = enet_host_create(NULL, 1, 2, 0, 0);
    client.peer = enet_host_connect(client.host, &address, 2, 0);
    if (client.peer == NULL) {
        std::cout << "could not connect\n";
    }

    do {

        ENetEvent event;
        enet_host_service(client.host, &event, 0);
        HandleEvent(event,client); //Handles the Events
        if(!RUDPToSend.empty()){
            ENetPacket* packet = enet_packet_create (RUDPToSend.c_str(),
                                                     RUDPToSend.size()+1,
                                                     ENET_PACKET_FLAG_RELIABLE); //Create A reliable packet using the data
            enet_peer_send(client.peer, 0, packet);
            RUDPToSend.clear();
        }
        Sleep(50);
    } while (true);

}


void Start(){
    std::thread t1(TCPServerThread);
    std::thread t2(RUDPClientThread);
    t2.join();
}