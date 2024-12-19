#include <chrono>

#include "./../../HEADER/UI.hpp"
#include "./../../HEADER/Request.hpp"
#include "./../../HEADER/Response.hpp"
#include "./../../HEADER/ThreadPool.hpp"
#include "./../../HEADER/ClientHandler.hpp"
#include "./../../HEADER/BlackList.hpp"
#include "./../../HEADER/Logger.hpp"


bool ClientHandler::parseHostAndPort(std::string request, std::string& hostname, int& port) {
    if (request.find("\r\n\r\n") == std::string::npos) {
        logger.logError(-1); // Thiếu phần kết thúc header HTTP
        return false;
    }

    size_t firstLineEnd = request.find("\r\n");
    if (firstLineEnd == std::string::npos) {
        logger.logError(-1);
        return false;
    }

    std::string firstLine = request.substr(0, firstLineEnd);
    size_t connectPos = firstLine.find("CONNECT");
    if (connectPos == 0) {
        size_t hostStart = 8; // Sau "CONNECT "
        while (hostStart < firstLine.size() && isspace(firstLine[hostStart])) hostStart++;

        size_t hostEnd = firstLine.find(' ', hostStart);
        if (hostEnd == std::string::npos) {
            logger.logError(-5); // Không tìm thấy hostname hợp lệ
            return false;
        }

        std::string domain = firstLine.substr(hostStart, hostEnd - hostStart);
        size_t colonPos = domain.find(':');
        if (colonPos != std::string::npos) {
            try {
                port = std::stoi(domain.substr(colonPos + 1));
                hostname = domain.substr(0, colonPos);
            } catch (const std::invalid_argument&) {
                logger.logError(-3); // Port không hợp lệ
                return false;
            } catch (const std::out_of_range&) {
                logger.logError(-4); // Port ngoài phạm vi
                return false;
            }
        } else {
            hostname = domain;
            port = HTTPS_PORT; // Mặc định cho CONNECT
        }

        if (hostname.empty()) {
            logger.logError(-5); // Hostname rỗng
            return false;
        }

        return true;
    }

    size_t hostPos = request.find("Host: ");
    if (hostPos == std::string::npos) {
        logger.logError(-2); // Không tìm thấy header Host
        return false;
    }

    size_t hostStart = hostPos + 6;
    while (isspace(request[hostStart])) hostStart++;
    size_t hostEnd = request.find("\r\n", hostStart);

    std::string domain = request.substr(hostStart, hostEnd - hostStart);
    while (!domain.empty() && isspace(domain.back())) domain.pop_back();

    size_t colonPos = domain.find(':');
    if (colonPos != std::string::npos) {
        try {
            port = std::stoi(domain.substr(colonPos + 1));
        } catch (const std::invalid_argument&) {
            logger.logError(-3);
            return false;
        } catch (const std::out_of_range&) {
            logger.logError(-4);
            return false;
        }
        hostname = domain.substr(0, colonPos);
    } else {
        hostname = domain;
        port = HTTP_PORT; // Mặc định cho HTTP
    }

    while (!hostname.empty() && !std::isalnum(hostname.back())) {
        hostname.pop_back();
    }

    if (hostname.empty()) {
        logger.logError(-5);
        return false;
    }

    return true;
}

ClientHandler::ClientHandler(bool type) {
    host.clear(); port = -1;
    socketHandler = nullptr;
    isMITM = type;
}

bool ClientHandler::handleConnection(SOCKET sock) {
    RequestHandler request; request.receiveRequest(sock);
    parseHostAndPort(request.getHeader(), host, port);
    if (port == -1) {
        // std::cerr << "SKIP\n";
        // Logger::errorStatus(-6);
        return false;
    }
    SOCKET remote = SOCKET_ERROR;
    if ((!isMITM || port == HTTP_PORT) && blackList.isMember(host)) {
        logger.logError(-7);
        // std::cerr << "BLOCKED! -> host:" << host << " port:" << port << '\n';
        // Window.AppendList(L"Block", std::wstring(host.begin(), host.end()), std::to_wstring(port));
        request.parseHttpHeader(sock, false);
        host.clear(); closesocket(sock);
        return false;
    }

    if (!blackList.isMember(host)) {
        // std::cerr << "ALLOWED! -> host:" << host << " port:" << port << '\n';
        request.parseHttpHeader(sock, true);
        // Window.AppendList(L"Allow", std::wstring(host.begin(), host.end()), std::to_wstring(port));
    } else request.parseHttpHeader(sock, false);

    remote = connectToServer();
    if (port == HTTPS_PORT) {
        const char* response = "HTTP/1.1 200 Connection Established\r\n\r\n";
        size_t byteSent = send(sock, response, strlen(response), 0);
        if (byteSent != strlen(response)) { 
            closesocket(remote); closesocket(sock);
            remote = SOCKET_ERROR; 
            logger.logError(-9);
            return false;
        }
    }
    else request.sendRequest(remote);
    if (isMITM) socketHandler = new SocketHandler(sock, remote, port==HTTPS_PORT);
    else socketHandler = new SocketHandler(sock, remote, false);
    return true;
}

ClientHandler::~ClientHandler() {
    if (socketHandler != nullptr) 
        delete socketHandler;
}

SOCKET ClientHandler::connectToServer() {
    hostent* hostInfo = gethostbyname(host.c_str());
    if (hostInfo == nullptr) {
        logger.logError(-10);
        return SOCKET_ERROR;
    }
    // Create a socket to listen for client connections
    SOCKET remoteSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (remoteSocket == INVALID_SOCKET) {
        logger.logError(-11);
        return SOCKET_ERROR;
    }

    // Set the remote server address
    sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(port); 
    remoteAddr.sin_addr.s_addr = *((unsigned long*)hostInfo->h_addr_list[0]);

    if (connect(remoteSocket, (sockaddr*)&remoteAddr, sizeof(remoteAddr)) == SOCKET_ERROR) {
        logger.logError(-12);
        closesocket(remoteSocket);
        return SOCKET_ERROR;
    }
    return remoteSocket;
}

#define TIMEOUT 100
#define MAX_IDLE_TIME 2000

void ClientHandler::handleRequest() {
    if (isMITM) handleMITM();
    else handleRelayData();
}

void ClientHandler::handleMITM() {
    if (socketHandler->isValid() == false) return;
    if (socketHandler->setSSLContexts(host) == false) return;

    if (blackList.isMember(host)) {
        // std::string response = 
        //     "HTTP/1.1 302 Found\r\n"
        //     "Location: https://example.com\r\n"
        //     "Content-Length: 0\r\n"
        //     "Connection: close\r\n\r\n";

        std::string blockMessage = 
            "<!DOCTYPE html>"
            "<html>"
            "<head><title>Website Blocked</title></head>"
            "<body>"
            "<h1>Access Denied</h1>"
            "<p>The website you are trying to access (" + host + ") is blocked by the proxy server.</p>"
            "<p>If you believe this is a mistake, please contact the administrator.</p>"
            "</body>"
            "</html>";

        std::string response = 
            "HTTP/1.1 403 Forbidden\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(blockMessage.size()) + "\r\n"
            "Connection: close\r\n\r\n" + 
            blockMessage;

        SSL_write(socketHandler->sslID[browser], response.c_str(), response.size());
        return;
    }

    struct pollfd fds[2];
    for (int i: {0, 1}) {
        fds[i].fd = socketHandler->socketID[i];
        fds[i].events = POLLIN;  
    }

    auto lastActivity = std::chrono::steady_clock::now();

    while (ServerRunning) {
        int ret = WSAPoll(fds, 2, TIMEOUT);
        if (ret < 0) {
            logger.logError(-13);
            break;
        } else if (ret == 0) {
            const auto& currentTime = std::chrono::steady_clock::now();
            const auto& idleDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastActivity).count(); 
            if (idleDuration > MAX_IDLE_TIME) {
                break;
            }
            continue;
        }

        for (int t = 0; t < 2; ++t)
            if (fds[t].revents & (POLLERR | POLLHUP)) {
                logger.logError(-14);
                return;
            }
        

        std::shared_ptr<std::future<void>> futures[2] = {nullptr, nullptr};
        if (fds[0].revents & POLLIN) {
            std::promise<void>* promise = new std::promise<void>();

            requestHandlerPool.enqueue([this, promise]() mutable {
                try {
                    RequestHandler request(this->socketHandler);
                    request.handleRequest();
                    request.parseHttpHeader(this->socketHandler->socketID[browser], true);
                } catch (...) {
                    // Xử lý lỗi nếu cần
                }
                promise->set_value(); // Báo task đã hoàn thành
            });
            futures[0] = std::make_shared<std::future<void>>(promise->get_future()); // Lưu future
        }

        if (fds[1].revents & POLLIN) {
            std::promise<void>* promise = new std::promise<void>();

            requestHandlerPool.enqueue([this, promise]() mutable {
                try {
                    ResponseHandler response(this->socketHandler);
                    response.handleResponse();
                } catch (...) {
                    // Xử lý lỗi nếu cần
                }
                promise->set_value(); // Báo task đã hoàn thành
            });
            futures[1] = std::make_shared<std::future<void>>(promise->get_future()); // Lưu future
        }
        for (int i = 0; i < 2; i++) 
            if (futures[i] != nullptr) futures[i]->wait();
        lastActivity = std::chrono::steady_clock::now();
    }
}

void ClientHandler::handleRelayData() {
    if (socketHandler->isValid() == false) return;

    struct pollfd fds[2];
    for (int i: {0, 1}) {
        fds[i].fd = socketHandler->socketID[i];
        fds[i].events = POLLIN;  
    }

    auto lastActivity = std::chrono::steady_clock::now();

    while (ServerRunning) {
        int ret = WSAPoll(fds, 2, TIMEOUT);
        if (ret < 0) {
            logger.logError(-13);
            break;
        }
        else if (ret == 0) {
            const auto& currentTime = std::chrono::steady_clock::now();
            const auto& idleDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastActivity).count(); 
            if (idleDuration > MAX_IDLE_TIME) {
                break;
            }
            continue;
        }
        
        for (int t = 0; t < 2; ++t)
            if (fds[t].revents & (POLLERR | POLLHUP)) {
                logger.logError(-14);
                return;
            }


        std::shared_ptr<std::future<void>> futures[2] = {nullptr, nullptr};
        if (fds[0].revents & POLLIN) {
            std::promise<void>* promise = new std::promise<void>();

            requestHandlerPool.enqueue([this, promise]() mutable {
                try {
                    char buffer[BUFFER_SIZE];
                    int bytesReceived = recv(socketHandler->socketID[browser], buffer, BUFFER_SIZE, 0);
                    if (bytesReceived <= 0) {
                        logger.logError(-37);
                        return;
                    }
                    send(socketHandler->socketID[server], buffer, bytesReceived, 0);
                } catch (...) {
                    // Xử lý lỗi nếu cần
                }
                promise->set_value(); // Báo task đã hoàn thành
            });
            futures[0] = std::make_shared<std::future<void>>(promise->get_future()); // Lưu future
        }

        if (fds[1].revents & POLLIN) {
            std::promise<void>* promise = new std::promise<void>();

            requestHandlerPool.enqueue([this, promise]() mutable {
                try {
                    char buffer[BUFFER_SIZE];
                    int bytesReceived = recv(socketHandler->socketID[server], buffer, BUFFER_SIZE, 0);
                    if (bytesReceived <= 0) {
                        logger.logError(-37);
                        return;
                    }
                    send(socketHandler->socketID[browser], buffer, bytesReceived, 0);
                } catch (...) {
                    // Xử lý lỗi nếu cần
                }
                promise->set_value(); // Báo task đã hoàn thành
            });
            futures[1] = std::make_shared<std::future<void>>(promise->get_future()); // Lưu future
        }

        lastActivity = std::chrono::steady_clock::now();

        for (int i = 0; i < 2; i++) 
            if (futures[i] != nullptr) futures[i]->wait();
    }
}