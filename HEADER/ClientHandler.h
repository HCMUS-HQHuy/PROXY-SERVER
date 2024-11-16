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