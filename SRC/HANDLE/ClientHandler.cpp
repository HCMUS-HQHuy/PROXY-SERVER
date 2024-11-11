#include "./../../HEADER/ClientHandler.h"
#include <chrono>

ClientHandler::ClientHandler(SOCKET sock) {
    clientSocket = sock;
    remoteSocket = connectToServer();
}

ClientHandler::~ClientHandler() {
    closesocket(clientSocket);
    closesocket(remoteSocket);
}

void ClientHandler::handleRequest() {
    if (remoteSocket == (SOCKET)SOCKET_ERROR) return;

    struct pollfd fds[2];
    
    fds[0].fd = clientSocket;
    fds[0].events = POLLIN;  

    fds[1].fd = remoteSocket;
    fds[1].events = POLLIN;  

    #define TIMEOUT 1000 
    #define BUFFER_SIZE 1024
    #define MAX_IDLE_TIME 5000
    char buffer[BUFFER_SIZE];
    auto lastActivity = std::chrono::steady_clock::now();
    while (true) {
        int ret = WSAPoll(fds, 2, TIMEOUT);

        if (ret < 0) {
            std::cerr << "WSAPoll ERROR!\n";
            break;
        } else if (ret == 0) {
            // Kiểm tra idle timeout
            const auto& currentTime = std::chrono::steady_clock::now();
            const auto& idleDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastActivity).count();
            if (idleDuration > MAX_IDLE_TIME) {
                std::cerr << "Idle timeout reached. Closing connection.\n";
                break;
            }
            std::cerr << "Timeout - no data activity\n";
            continue;
        }

        // Reset reconnect khi có hoạt động
        lastActivity = std::chrono::steady_clock::now();

        // Kiểm tra lỗi hoặc ngắt kết nối cho cả hai socket
        for (int t = 0; t < 2; ++t) {
            if (fds[t].revents & (POLLHUP | POLLERR | POLLNVAL)) {
                std::cerr << "Socket from " << (t == 0 ? "client" : "server") << " closed.\n";
                return;
            }
        }

        if (fds[0].revents & POLLIN) {
            int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            if (bytesReceived <= 0) {
                std::cerr << "Client closed connection.\n";
                break;
            }
            send(remoteSocket, buffer, bytesReceived, 0);
        }
        if (fds[1].revents & POLLIN) {
            int bytesReceived = recv(remoteSocket, buffer, BUFFER_SIZE, 0);
            if (bytesReceived <= 0) {
                std::cerr << "Server closed connection.\n";
                break;
            }
            send(clientSocket, buffer, bytesReceived, 0);
        }
    }
}

SOCKET ClientHandler::connectToServer() {
    Message message; message.receiveMessage(clientSocket);
    string host; int port; 
    message.getHostFromRequest(host, port);
    hostent* hostInfo = gethostbyname(host.c_str());
    std::cerr << host << " " << port <<'\n';
    if (hostInfo == nullptr) {
        std::cerr << "Failed to resolve host name." << std::endl;
        return SOCKET_ERROR;
    }
    // Create a socket to listen for client connections
    SOCKET remoteSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (remoteSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket." << std::endl;
        return SOCKET_ERROR;
    }

    // Set the remote server address
    sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(port); 
    remoteAddr.sin_addr.s_addr = *((unsigned long*)hostInfo->h_addr_list[0]);

    // Connect to the remote server
    if (connect(remoteSocket, (sockaddr*)&remoteAddr, sizeof(remoteAddr)) == SOCKET_ERROR) {
        std::cerr << "Error connecting to remote server." << std::endl;
        closesocket(remoteSocket);
        return SOCKET_ERROR;
    }

    if (port == HTTPS_PORT) {
        Message connectMessage("HTTP/1.1 200 Connection Established\r\n\r\n");
        if (connectMessage.sendMessage(clientSocket) == SOCKET_ERROR) {
            closesocket(remoteSocket);
            return SOCKET_ERROR;
        }
    }
    return remoteSocket;
}