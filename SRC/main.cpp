#ifndef UNICODE
#define UNICODE
#endif
#include "./../HEADER/ProxyServer.hpp"
#include "./../HEADER/UI.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    FreeConsole();
    std::string directoryName = "./GENERATED_CER";
    if (GetFileAttributesA(directoryName.c_str()) == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryA(directoryName.c_str(), NULL))
            return 0;
    }

    Window.init(WindowProc, hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    Window.start();
    return 0;
}
