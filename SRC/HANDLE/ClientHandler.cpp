#include <chrono>

#include "./../../HEADER/Request.hpp"
#include "./../../HEADER/Response.hpp"
#include "./../../HEADER/ThreadManager.hpp"
#include "./../../HEADER/ClientHandler.hpp"
#include "./../../HEADER/BlackList.hpp"
#include "./../../HEADER/Logger.hpp"

bool ClientHandler::parseHostAndPort(std::string request, std::string& hostname, int& port) {
    // std::cerr << request << '\n';
    if (request.find("\r\n\r\n") == std::string::npos) {
        Logger::errorStatus(-1);
        std::cerr << "MISSING HEADER HTTP DATA.\n";
        std::cerr << request << '\n';
        return false;
    }

    port = (request.find("CONNECT") == 0 ? HTTPS_PORT : HTTP_PORT);

    size_t hostPos = request.find("Host: ");
    if (hostPos == std::string::npos) {
        Logger::errorStatus(-2);
        return false;
    }

    size_t hostStart = hostPos + 6; while (isspace(request[hostStart])) hostStart++;
    size_t hostEnd = request.find("\r\n", hostStart);

    std::string domain = request.substr(hostStart, hostEnd - hostStart);
    while (!domain.empty() && isspace(domain.back())) domain.pop_back();
    // Kiểm tra xem có chứa port hay không
    size_t colonPos = domain.find(':');
    if (colonPos != std::string::npos) {
        // Lấy hostname và port từ domain
        try {
            port = std::stoi(domain.substr(colonPos + 1));
        } catch (const std::invalid_argument&) {
            Logger::errorStatus(-3);
            return false;
        } catch (const std::out_of_range&) {
            Logger::errorStatus(-4);
            return false;
        }
        hostname = domain.substr(0, colonPos);
    } else {
        // Không có port, sử dụng port mặc định
        hostname = domain;
        port = HTTP_PORT; 
    }

    // Loại bỏ các ký tự không hợp lệ ở cuối hostname (nếu có)
    while (!hostname.empty() && !std::isalnum(hostname.back())) {
        hostname.pop_back();
    }

    if (hostname.empty()) {
        Logger::errorStatus(-5);
        return false;
    }

    return true;
}

ClientHandler::ClientHandler(SOCKET sock) {
    // std::cerr << "Create new client Handler\n";
    char buffer[BUFFER_SIZE]; 
    int bytesRecv = recv(sock, buffer, BUFFER_SIZE, 0);
    host.clear(); port = -1;
    parseHostAndPort(std::string(buffer, bytesRecv), host, port);
    if (port == HTTP_PORT) {
        SOCKET remote = connectToServer();
        send(remote, buffer, bytesRecv, 0);
        bytesRecv = recv(remote, buffer, BUFFER_SIZE, 0);
        send(sock, buffer, bytesRecv, 0);
        std::cerr << "HTTP REQUEST SUCCESFUL!! " << host << " port:" << port << '\n';
        host.clear(); closesocket(remote);
    }
    socketHandler = nullptr;
}

bool ClientHandler::handleConnection(SOCKET sock) {
    if (host.empty()) {
        Logger::errorStatus(-6);
        return false;
    }
    SOCKET remote = SOCKET_ERROR;
    if (blackList.isMember(host)) {
        Logger::errorStatus(-7);
        std::cerr << "BLOCKED! -> host:" << host << " port:" << port << '\n';
        const char* response = "HTTP/1.1 403 Forbidden\r\n\r\n";
        size_t byteSent = send(sock, response, strlen(response), 0);
        if (byteSent != strlen(response)) {
            Logger::errorStatus(-8);
        }
        host.clear();
        closesocket(sock);
        return false;
    }
    else {
        std::cerr << "ALLOWED! -> host:" << host << " port:" << port << '\n';
        remote = connectToServer();
        if (port == HTTPS_PORT) {
            const char* response = "HTTP/1.1 200 Connection Established\r\n\r\n";
            size_t byteSent = send(sock, response, strlen(response), 0);
            if (byteSent != strlen(response)) {
                closesocket(remote);
                remote = SOCKET_ERROR;
                Logger::errorStatus(-9);
                return false;
            }
        }
    }
    socketHandler = new SocketHandler(sock, remote, port==HTTPS_PORT);
    // socketHandler = new SocketHandler(sock, remote, false);
    return true;
}

ClientHandler::~ClientHandler() {
    if (socketHandler != nullptr) 
        delete socketHandler;
}

SOCKET ClientHandler::connectToServer() {
    hostent* hostInfo = gethostbyname(host.c_str());
    if (hostInfo == nullptr) {
        Logger::errorStatus(-10);
        return SOCKET_ERROR;
    }
    // Create a socket to listen for client connections
    SOCKET remoteSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (remoteSocket == INVALID_SOCKET) {
        Logger::errorStatus(-11);
        return SOCKET_ERROR;
    }

    // Set the remote server address
    sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(port); 
    remoteAddr.sin_addr.s_addr = *((unsigned long*)hostInfo->h_addr_list[0]);

    if (connect(remoteSocket, (sockaddr*)&remoteAddr, sizeof(remoteAddr)) == SOCKET_ERROR) {
        Logger::errorStatus(-12);
        closesocket(remoteSocket);
        return SOCKET_ERROR;
    }
    return remoteSocket;
} 

void ClientHandler::handleRequest() {
    if (socketHandler->isValid() == false) return;
    if (socketHandler->setSSLContexts(host) == false) return;

    struct pollfd fds[2];
    for (int i: {0, 1}) {
        fds[i].fd = socketHandler->socketID[i];
        fds[i].events = POLLIN;  
    }

    #define TIMEOUT 100
    #define MAX_IDLE_TIME 5000

    auto lastActivity = std::chrono::steady_clock::now();

    while (ServerRunning) {
        int ret = WSAPoll(fds, 2, TIMEOUT);
        if (ret < 0) {
            Logger::errorStatus(-13);
            break;
        }
        else if (ret == 0) {
            const auto& currentTime = std::chrono::steady_clock::now();
            const auto& idleDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastActivity).count(); 
            if (idleDuration > MAX_IDLE_TIME) {
                // std::cerr << "TIMEOUT\n";
                break;
            }
            continue;
        }
        
        for (int t = 0; t < 2; ++t)
            if (fds[t].revents & (POLLERR | POLLHUP)) {
                Logger::errorStatus(-14);
                std::cerr << (t == 0 ? "BROWSER" : "REMOTE") << " HAVE SOME PROBLEM!!\n";
                return;
            }

        lastActivity = std::chrono::steady_clock::now();

        if (fds[0].revents & POLLIN) {
            RequestHandler request(socketHandler);
            if (request.handleRequest() == false) break;
        }
        if (fds[1].revents & POLLIN) {
            ResponseHandler response(socketHandler);
            if (response.handleResponse() == false) break;
        }
    }
    // std::cerr << "in client Handler END!!\n";
}
