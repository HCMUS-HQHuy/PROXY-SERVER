#ifndef CLIENTHANDLE_H
#define CLIENTHANDLE_H

#include "./setting.h"
#include "./Message.h"

class ClientHandler {
private:
    SOCKET clientSocket;
    SOCKET remoteSocket;
    // Request request;
    // Response response:
    SOCKET connectToServer();
public:
    ClientHandler(SOCKET sock);
    ~ClientHandler();
    void handleRequest();
};

#endif
    // fd_set readfds;
    // int step = 0;
    // while (true) {
    //     FD_ZERO(&readfds);
    //     FD_SET(clientSocket, &readfds);
    //     FD_SET(remoteSocket, &readfds);
    //     TIMEVAL delay; delay.tv_sec = 5; delay.tv_usec=0;
    //     int activity = select(0, &readfds, nullptr, nullptr, &delay);
    //     if (activity <= 0) break;
    //     if (FD_ISSET(clientSocket, &readfds)) {
    //         Message request; 
    //         int byteReceive = request.receiveMessage(clientSocket);
    //         if (byteReceive <= 0) break;
    //         if (request.sendMessage(remoteSocket, byteReceive) <= 0) break;
    //     }
    //     if (FD_ISSET(remoteSocket, &readfds)) {
    //         Message response;
    //         int byteReceive = response.receiveMessage(remoteSocket);
    //         if (byteReceive <= 0) break;
    //         if (response.sendMessage(clientSocket, byteReceive) <= 0) break;
    //     }
    //     std::cerr << ++step << "\n";
    // }