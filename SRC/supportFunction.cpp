#include "../HEADER/supportFunction.h"
#include <iostream>


bool receiveMSG(SOCKET socket, char *buffer, int len) {
    int bytesReceived = recv(socket, buffer, len, 0);

    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Error receiving data: " << WSAGetLastError() << std::endl;
        return false;
    }

    if (bytesReceived == 0) {
        std::cout << "Connection closed by peer." << std::endl;
        return false;
    }

    std::cout << "Received message: " << buffer << std::endl;

    return true;
}

bool sendMSG(SOCKET socket, char *buffer, int len) {
    int bytesSent = send(socket, buffer, len, 0);

    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "Error sending data: " << WSAGetLastError() << std::endl;
        return false;
    }
    std::cout << "Sent " << bytesSent << " bytes." << std::endl;
    return true;
}

