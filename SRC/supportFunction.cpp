#include "../HEADER/supportFunction.h"
#include <iostream>


bool receiveMSG(SOCKET socket, char *buffer, int len) {
    int bytesReceived = recv(socket, buffer, len, 0);

    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Error receiving data: " << WSAGetLastError() << std::endl;
        return false;
    }

    if (bytesReceived == 0) {
        std::cout << "Connection closed by peer." << std::endl;
        return false;
    }

    // Xử lý dữ liệu nhận được
    buffer[bytesReceived] = '\0'; // Thêm ký tự kết thúc chuỗi
    std::cout << "Received message: " << buffer << std::endl;

    return true;
}

