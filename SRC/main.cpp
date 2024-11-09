#include "./../HEADER/Setting.h"
#include "./../HEADER/ProxyServer.h"

int main() {
    ProxyServer proxyServer(LOCAL_PORT);
    proxyServer.start();
    proxyServer.stop();
    return 0;
}