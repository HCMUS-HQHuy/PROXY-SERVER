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
    std::string directoryName = "./GENERATED_CER";
    if (GetFileAttributesA(directoryName.c_str()) == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryA(directoryName.c_str(), NULL))
            return 0;
    }

    Window.init(WindowProc, hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    Window.start();

    if (!RemoveDirectoryA(directoryName.c_str())) {
        std::cerr << "Directory deleted UNsuccessfully." << std::endl;
    }
    return 0;
}
