///
/// Created by Anonymous275 on 4/11/2020
///

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

std::vector<std::string> Split(const std::string& String,const std::string& delimiter){
    std::vector<std::string> Val;
    size_t pos = 0;
    std::string token,s = String;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        Val.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    Val.push_back(s);
    return Val;
}

void SyncResources(const std::string&IP,int Port){
    std::cout << "Called" << std::endl;
    std::string FileList;
    struct stat info{};
    if(stat( "Resources", &info) != 0){
        _wmkdir(L"Resources");
    }

    WSADATA wsaData;
    SOCKET SendingSocket;
    SOCKADDR_IN ServerAddr, ThisSenderInfo;
    int RetCode;
    int BytesSent, nlen;

    WSAStartup(MAKEWORD(2,2), &wsaData);


    SendingSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(SendingSocket == INVALID_SOCKET)
    {
        printf("Client: socket() failed! Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    // Set up a SOCKADDR_IN structure that will be used to connect

    // to a listening server on port 5150. For demonstration

    // purposes, let's assume our server's IP address is 127.0.0.1 or localhost

    // IPv4

    ServerAddr.sin_family = AF_INET;

    // Port no.

    ServerAddr.sin_port = htons(Port);

    // The IP address

    ServerAddr.sin_addr.s_addr = inet_addr(IP.c_str());

    // Make a connection to the server with socket SendingSocket.

    RetCode = connect(SendingSocket, (SOCKADDR *) &ServerAddr, sizeof(ServerAddr));

    if(RetCode != 0)
    {
        printf("Client: connect() failed! Error code: %ld\n", WSAGetLastError());
        // Close the socket
        closesocket(SendingSocket);
        // Do the clean up
        WSACleanup();
        // Exit with error
        return;
    }

    // At this point you can start sending or receiving data on
    // the socket SendingSocket.

    // Some info on the receiver side...

    getsockname(SendingSocket, (SOCKADDR *)&ServerAddr, (int *)sizeof(ServerAddr));

    /*printf("Client: Receiver IP(s) used: %s\n", inet_ntoa(ServerAddr.sin_addr));

    printf("Client: Receiver port used: %d\n", htons(ServerAddr.sin_port));*/

    BytesSent = send(SendingSocket, "a", 1, 0);


    int iResult;
    char recvbuf[65535];
    int recvbuflen = 65535;

    do {
        iResult = recv(SendingSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            //printf("Bytes received: %d\n", iResult);
            std::string Data = recvbuf,Response,toSend;
            Data.resize(iResult);
            std::vector<std::string> list = Split(Data,";");
            Data.clear();
            Data.resize(recvbuflen);
            for(const std::string& a : list){
                if(!a.empty() && stat(a.c_str(), &info) != 0){
                    std::ofstream LFS;
                    LFS.open(a.c_str());
                    LFS.close();
                    toSend = "b"+a;
                    std::cout << a << std::endl;
                    send(SendingSocket, toSend.c_str(), toSend.length(), 0);
                    do{
                        iResult = recv(SendingSocket, &Data[0], recvbuflen, 0);
                        if(iResult < 0)break;
                        Data.resize(iResult);
                        if(Data.find("End of file") != std::string::npos){
                            if(Data.length() > 11){
                                LFS.open (a.c_str(), std::ios_base::app | std::ios::binary);
                                LFS << Data.substr(0,Data.length()-11);
                                LFS.close();
                            }
                            std::cout << "File Done\n";
                            break;
                        }
                        LFS.open (a.c_str(), std::ios_base::app | std::ios::binary);
                        LFS << Data;
                        LFS.close();
                    }while(Data != "End of file");
                }
            }
            break;
            // Echo the buffer back to the sender
            /*iSendResult = send(Client, Response.c_str(), Response.length(), 0);
            if (iSendResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(Client);
                return;
            }
            printf("Bytes sent: %d\n", iSendResult);*/
        }
        else if (iResult == 0)
            printf("Connection closing...\n");
        else  {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(SendingSocket);
            break;
        }
    } while (iResult > 0);


    if(BytesSent == SOCKET_ERROR)
        printf("Client: send() error %ld.\n", WSAGetLastError());



    if( shutdown(SendingSocket, SD_SEND) != 0)
        printf("Client: Well, there is something wrong with the shutdown() The error code: %ld\n", WSAGetLastError());

    // When you are finished sending and receiving data on socket SendingSocket,

    // you should close the socket using the closesocket API. We will

    // describe socket closure later in the chapter.

    if(closesocket(SendingSocket) != 0)
        printf("Client: Cannot close \"SendingSocket\" socket. Error code: %ld\n", WSAGetLastError());




    // When your application is finished handling the connection, call WSACleanup.

    if(WSACleanup() != 0)
        printf("Client: WSACleanup() failed!...\n");
}