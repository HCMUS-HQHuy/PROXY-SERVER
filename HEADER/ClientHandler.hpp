#ifndef CLIENTHANDLER_HPP
#define CLIENTHANDLER_HPP

#include "./setting.hpp"
#include "./SocketHandler.hpp"

class ClientHandler {
private:
    std::string host; int port;
    bool isMITM;
    SocketHandler* socketHandler;

    SOCKET connectToServer();
    bool parseHostAndPort(std::string request, std::string& hostname, int& port);

    void handleMITM();
    void handleRelayData();
public:
    ClientHandler(bool isMITM);
    ~ClientHandler();
    bool handleConnection(SOCKET sock);
    void handleRequest();
    std::string getHost() {return host;}
};

#endif