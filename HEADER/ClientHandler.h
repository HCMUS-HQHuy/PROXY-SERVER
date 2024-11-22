#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include "./setting.h"
#include "./SocketHandler.h"
#include "./Request.h"
#include "./Response.h"
#include "./ThreadManager.h"

class ClientHandler {
private:
    SocketHandler* socketHandler;
public:
    ClientHandler(SOCKET sock);
    ~ClientHandler();
    void handleRequest();
};

#endif