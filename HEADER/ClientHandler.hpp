#ifndef CLIENTHANDLER_HPP
#define CLIENTHANDLER_HPP

#include "./setting.hpp"
#include "./SocketHandler.hpp"
#include "./Request.hpp"
#include "./Response.hpp"
#include "./ThreadManager.hpp"

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