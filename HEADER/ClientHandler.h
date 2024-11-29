#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include "./setting.h"
#include "./SocketHandler.h"
#include "./Request.h"
#include "./Response.h"
#include "./ThreadManager.h"

class ClientHandler {
private:
    std::string host; int port;
    SocketHandler* socketHandler;

    SOCKET connectToServer();
    bool parseHostAndPort(std::string request, std::string& hostname, int& port);
public:
    ClientHandler(SOCKET sock);
    ~ClientHandler();
    void handleRequest();
};

#endif