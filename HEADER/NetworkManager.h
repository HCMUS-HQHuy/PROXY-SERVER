#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include "./Setting.h"

class NetworkManager {
private:
    WSAData wsaData;
    char* getIPv4();
protected: 
    char* IPv4;
    SOCKET localSocket;
public:
    NetworkManager();
    ~NetworkManager();
    int sendMessage(SOCKET& Socket, char* msg, int length);
    int receiveMessage(SOCKET& Socket, char* msg, int length);
    int sendLargeData(SOCKET sock, char* data, int dataSize);
    int receiveLargeData(SOCKET sock, char* buffer, int dataSize);
};

#endif