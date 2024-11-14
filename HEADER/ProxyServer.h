#ifndef PROXYSERVER_H
#define PROXYSERVER_H

#include "./Setting.h"
#include "./NetworkManager.h"
#include "ThreadPool.h"

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