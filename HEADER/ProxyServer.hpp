#ifndef PROXYSERVER_HPP
#define PROXYSERVER_HPP

#include "./Setting.hpp"
#include "./NetworkManager.hpp"
#include "./ThreadPool.hpp"

class ProxyServer : public NetworkManager{
private:
    int port;
    void waitingClient();
    SOCKET acceptClient();
public:
    ProxyServer(int port);
    void start();
    void stop();
} ;

#endif