#ifndef UNICODE
#define UNICODE
#endif
#include "./../HEADER/Setting.hpp"
#include "./../HEADER/ProxyServer.hpp"
#include "./../HEADER/UI.hpp"

// int main() {
//     ProxyServer proxyServer(LOCAL_PORT);

//     std::signal(SIGINT, ProxyServer::stop); 
//     proxyServer.start();

//     std::cout << "Cleaning up resources...\n";
//     return 0;
// }

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Window.init(WindowProc, hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    Window.start();
    return 0;
}
