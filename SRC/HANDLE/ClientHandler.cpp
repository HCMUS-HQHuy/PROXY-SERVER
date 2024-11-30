#include <chrono>

#include "./../../HEADER/ClientHandler.hpp"
#include "./../../HEADER/BlackList.hpp"

bool ClientHandler::parseHostAndPort(std::string request, std::string& hostname, int& port) {
    if (request.find("\r\n\r\n") == std::string::npos) {
        std::cerr << "MISSING HEADER HTTP DATA.\n";
        std::cerr << request << '\n';
        return false;
    }

    port = (request.find("CONNECT") == 0 ? HTTPS_PORT : HTTP_PORT);

    size_t hostPos = request.find("Host: ");
    if (hostPos == std::string::npos) {
        std::cerr << "HOST HEADER NOT FOUND IN REQUEST.\n";
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
        hostname = domain.substr(0, colonPos);
        try {
            port = std::stoi(domain.substr(colonPos + 1));
        } catch (const std::invalid_argument&) {
            std::cerr << "INVALID PORT NUMBER." << std::endl;
            return false;
        } catch (const std::out_of_range&) {
            std::cerr << "PORT NUMBER OUT OF RANGE." << std::endl;
            return false;
        }
    } else {
        // Không có port, sử dụng port mặc định
        hostname = domain;
    }

    // Loại bỏ các ký tự không hợp lệ ở cuối hostname (nếu có)
    while (!hostname.empty() && !std::isalnum(hostname.back())) {
        hostname.pop_back();
    }

    if (hostname.empty()) {
        std::cerr << "HOST NOT FOUND IN REQUEST.\n";
        return false;
    }

    return true;
}

ClientHandler::ClientHandler(SOCKET sock) {
    // std::cerr << "Create new client Handler\n";
    char buffer[BUFFER_SIZE]; 
    int bytesRecv = recv(sock, buffer, BUFFER_SIZE, 0);

    SOCKET remote = SOCKET_ERROR;
    if (parseHostAndPort(std::string(buffer, bytesRecv), host, port)) {
        if (blackList.isMember(host)) host = "example.com";
        else {
            remote = connectToServer();
            if (port == HTTPS_PORT) {
                const char* response = "HTTP/1.1 200 Connection Established\r\n\r\n";
                size_t byteSent = send(sock, response, strlen(response), 0);
                if (byteSent != strlen(response)) {
                    closesocket(remote);
                    remote = SOCKET_ERROR;
                }
            }
        }
    }

    socketHandler = new SocketHandler(sock, remote, port==HTTPS_PORT);
}

ClientHandler::~ClientHandler() {
    delete socketHandler;
}

SOCKET ClientHandler::connectToServer() {
    hostent* hostInfo = gethostbyname(host.c_str());
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

    if (connect(remoteSocket, (sockaddr*)&remoteAddr, sizeof(remoteAddr)) == SOCKET_ERROR) {
        std::cerr << "Error connecting to remote server." << std::endl;
        closesocket(remoteSocket);
        return SOCKET_ERROR;
    }
    std::cout << "HOST: " << host << " " << port << '\n';
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

    while (true) {
        int ret = WSAPoll(fds, 2, TIMEOUT);
        if (ret < 0) {
            std::cerr << "WSAPoll ERROR!\n";
            break;
        } 
        else if (ret == 0) {
            // Kiểm tra idle timeout
            const auto& currentTime = std::chrono::steady_clock::now();
            const auto& idleDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastActivity).count(); 
            if (idleDuration > MAX_IDLE_TIME) {
                std::cerr << "TIMEOUT\n";
                break;
            }
            continue;
        }
        
        for (int t = 0; t < 2; ++t)
            if (fds[t].revents & (POLLERR | POLLHUP)) {
                std::cerr << (t == 0 ? "BROWSER" : "REMOTE") << " HAVE SOME PROBLEM!!\n";
                return;
            }

        lastActivity = std::chrono::steady_clock::now();

        if (fds[0].revents & POLLIN) {
            // std::cerr << "IN browser\n";
            RequestHandler request(socketHandler);
            if (request.handleRequest() == false) break;
        }
        if (fds[1].revents & POLLIN) {
            // std::cerr << "IN server\n";
            ResponseHandler response(socketHandler);
            if (response.handleResponse() == false) break;
        }
    }
}
