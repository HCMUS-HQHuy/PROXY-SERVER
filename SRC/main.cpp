#ifndef UNICODE
#define UNICODE
#endif
#include "./../HEADER/ProxyServer.hpp"
#include "./../HEADER/UI.hpp"
#include "./../HEADER/ThreadPool.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // FreeConsole();
    std::string directoryName = "./GENERATED_CER";
    if (GetFileAttributesA(directoryName.c_str()) == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryA(directoryName.c_str(), NULL))
            return 0;
    }

    Window.init(WindowProc, hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    Window.start();

    // Xóa thư mục và tất cả nội dung bên trong
    directoryName += '\0'; // Kết thúc chuỗi bằng ký tự null để sử dụng với SHFileOperation
    SHFILEOPSTRUCTA fileOp = {0};
    fileOp.wFunc = FO_DELETE; // Thao tác xóa
    fileOp.pFrom = directoryName.c_str(); // Tên thư mục cần xóa
    fileOp.fFlags = FOF_NO_UI; // Không hiển thị giao diện người dùng

    if (SHFileOperationA(&fileOp) != 0) {
        std::cerr << "Error: Failed to delete directory " << directoryName << "\n";
    } else {
        std::cout << "Directory deleted successfully.\n";
    }
    return 0;
}