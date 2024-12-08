#ifndef UNICODE
#define UNICODE
#endif
#include "./../HEADER/Setting.hpp"
#include "./../HEADER/ProxyServer.hpp"
#include "./../HEADER/FrontEnd.hpp"

std::atomic<bool> ServerRunning{true};

// int main() {
//     ProxyServer proxyServer(LOCAL_PORT);

//     std::signal(SIGINT, ProxyServer::stop); 
//     proxyServer.start();

//     std::cout << "Cleaning up resources...\n";
//     return 0;
// }

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Đăng ký lớp cửa sổ
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MainWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    RegisterClass(&wc);

    // Tạo cửa sổ chính
    HWND hwnd = CreateWindowEx(
        0,
        L"MainWindow",
        L"Demo Application",
        (WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME) & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);

    // Vòng lặp xử lý thông điệp
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
