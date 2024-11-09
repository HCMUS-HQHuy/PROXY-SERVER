#ifndef CLIENTHANDLE_H
#define CLIENTHANDLE_H

#include "./setting.h"
#include "./Message.h"

class ClientHandler {
private:
    SOCKET clientSocket;
    // Request request;
    // Response response:
    bool connectToServer();
public:
    ClientHandler(SOCKET sock);
    ~ClientHandler();
    void handleRequest();
};

#endif