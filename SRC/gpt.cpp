#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 4096

// Hàm chuyển tiếp dữ liệu giữa client và server
void transferData(SOCKET srcSocket, SOCKET destSocket) {
    char buffer[BUFFER_SIZE];
    int bytesReceived;

    while ((bytesReceived = recv(srcSocket, buffer, BUFFER_SIZE, 0)) > 0) {
        send(destSocket, buffer, bytesReceived, 0);
    }
}

// Hàm xử lý yêu cầu từ client
void handleClient(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    int bytesReceived;

    // Đọc yêu cầu CONNECT từ client
    bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesReceived <= 0) {
        closesocket(clientSocket);
        return;
    }

    // Parse yêu cầu CONNECT để lấy hostname và port
    std::string request(buffer, bytesReceived);
    std::cout << "Request: " << request << std::endl;

    size_t hostPos = request.find("CONNECT ");
    size_t portPos = request.find(':', hostPos);
    size_t endPos = request.find(" ", portPos);

    std::string host = request.substr(hostPos + 8, portPos - hostPos - 8);
    std::string port = request.substr(portPos + 1, endPos - portPos - 1);

    // Kết nối đến server đích
    struct addrinfo hints, *result;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &result) != 0) {
        closesocket(clientSocket);
        return;
    }

    SOCKET serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (connect(serverSocket, result->ai_addr, (int)result->ai_addrlen) != 0) {
        closesocket(clientSocket);
        freeaddrinfo(result);
        return;
    }

    freeaddrinfo(result);

    // Gửi phản hồi thành công cho client
    const char* successResponse = "HTTP/1.1 200 Connection Established\r\n\r\n";
    send(clientSocket, successResponse, strlen(successResponse), 0);

    // Tạo luồng chuyển tiếp dữ liệu giữa client và server
    std::thread clientToServer(transferData, clientSocket, serverSocket);
    std::thread serverToClient(transferData, serverSocket, clientSocket);

    // Đợi cả hai luồng kết thúc
    clientToServer.join();
    serverToClient.join();

    // Đóng kết nối
    closesocket(clientSocket);
    closesocket(serverSocket);
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in proxyAddr;
    proxyAddr.sin_family = AF_INET;
    proxyAddr.sin_addr.s_addr = INADDR_ANY;
    proxyAddr.sin_port = htons(8080);

    bind(listenSocket, (sockaddr*)&proxyAddr, sizeof(proxyAddr));
    listen(listenSocket, SOMAXCONN);

    std::cout << "Proxy server is running on port 8080..." << std::endl;

    while (true) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        std::thread(handleClient, clientSocket).detach();
    }

    closesocket(listenSocket);
    WSACleanup();

    return 0;
}
