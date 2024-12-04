#ifndef PROXYSERVER_HPP
#define PROXYSERVER_HPP

#include "./Setting.hpp"
#include "./NetworkManager.hpp"

enum Proxy{MITM, Transparent};

class ProxyServer : public NetworkManager{
private:
    int port; Proxy type;
    void waitingClient();
    SOCKET acceptClient();
public:
    ProxyServer(Proxy type, int port);
    void start();
    static void stop(int signum);
} ;

#endif