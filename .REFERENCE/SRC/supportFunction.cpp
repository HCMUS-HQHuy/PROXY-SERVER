#include "../HEADER/setting.h"
#include <vector>

bool receiveMSG(SOCKET socket, char *buffer, int &len) {
    int bytesReceived = recv(socket, buffer, len, 0);

    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Error receiving data: " << WSAGetLastError() << std::endl;
        return false;
    }

    if (bytesReceived == 0) {
        std::cerr << "Connection closed by peer." << std::endl;
        return false;
    }
    len = bytesReceived;
    // std::cerr << "Received message: " << buffer << std::endl;
    return true;
}

bool sendMSG(SOCKET socket, char *buffer, int &len) {
    int bytesSent = send(socket, buffer, len, 0);

    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "Error sending data: " << WSAGetLastError() << std::endl;
        return false;
    }
    len = bytesSent;
    // std::cerr << "Sent " << bytesSent << " bytes." << std::endl;
    return true;
}

const int BUFFER_SIZE = 4096; // Kích thước buffer để nhận dữ liệu

// Hàm nhận dữ liệu lớn
bool receiveLargeData(SOCKET socket, std::vector<char>& data) {
    char buffer[BUFFER_SIZE];
    int bytesReceived = 0;
    int totalBytesReceived = 0;

    while (true) {
        bytesReceived = recv(socket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0) {
            // Thêm dữ liệu nhận được vào vector
            data.insert(data.end(), buffer, buffer + bytesReceived);
            totalBytesReceived += bytesReceived;
        } else if (bytesReceived == 0) {
            // Kết thúc kết nối
            break;
        } else {
            std::cerr << "Error receiving data: " << WSAGetLastError() << std::endl;
            return false;
        }
    }

    std::cout << "Total bytes received: " << totalBytesReceived << std::endl;
    return true;
}

// Hàm gửi dữ liệu lớn
bool sendLargeData(SOCKET socket, const std::vector<char>& data) {
    int totalBytesSent = 0;
    int bytesToSend = data.size();
    int bytesSent = 0;

    while (totalBytesSent < bytesToSend) {
        bytesSent = send(socket, data.data() + totalBytesSent, bytesToSend - totalBytesSent, 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "Error sending data: " << WSAGetLastError() << std::endl;
            return false;
        }
        totalBytesSent += bytesSent;
    }

    std::cout << "Total bytes sent: " << totalBytesSent << std::endl;
    return true;
}