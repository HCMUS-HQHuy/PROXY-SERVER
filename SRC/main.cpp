#include "./../HEADER/Setting.hpp"
#include "./../HEADER/ProxyServer.hpp"

std::atomic<bool> ServerRunning{true};

int main() {
    ProxyServer proxyServer(MITM, LOCAL_PORT);

    std::signal(SIGINT, ProxyServer::stop); 
    
    proxyServer.start();

    std::cout << "Cleaning up resources...\n";
    return 0;
}
