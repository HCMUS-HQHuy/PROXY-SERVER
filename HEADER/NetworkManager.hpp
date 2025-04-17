#ifndef NETWORKMANAGER_HPP
#define NETWORKMANAGER_HPP

#include "./Setting.hpp"

class NetworkManager {
private:
    WSAData wsaData;
protected: 
    string IPv4;
    SOCKET localSocket;
public:
    NetworkManager();
    ~NetworkManager();

    static string getIPv4();

    int sendMessage(SOCKET& Socket, char* msg, int length);
    int receiveMessage(SOCKET& Socket, char* msg, int length);
    int sendLargeData(SOCKET sock, char* data, int dataSize);
    int receiveLargeData(SOCKET sock, char* buffer, int dataSize);
};

#endif