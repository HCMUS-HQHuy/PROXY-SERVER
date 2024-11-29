#include "./../HEADER/Setting.hpp"
#include "./../HEADER/ProxyServer.hpp"

int main() {
    ProxyServer proxyServer(LOCAL_PORT);
    proxyServer.start();
    proxyServer.stop();
    return 0;
}