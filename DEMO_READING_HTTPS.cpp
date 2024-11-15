#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/applink.c>
#include <regex>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

#define HTTPS_PORT 443
#define HTTP_PORT 80

bool getHostFromRequest(const std::string request, std::string &hostname, int &port) {
    port = (request.find("CONNECT") == 0 ? HTTPS_PORT : HTTP_PORT);
    size_t hostPos = request.find("Host: ");
    if (hostPos == std::string::npos) {
        std::cerr << "Host header not found in request." << std::endl;
        return false;
    }

    std::string domain = request.substr(hostPos + 6);
    size_t endPos = domain.find('\n');
    if (endPos != std::string::npos) {
        domain = domain.substr(0, endPos);
    }

    if (port == HTTP_PORT) {
        hostname = domain;
        while (!hostname.empty() && !std::isalpha(hostname.back())) {
            hostname.pop_back();
        }
        if (hostname.empty()) {
            std::cerr << "Host not found in request." << std::endl;
            return false;
        }
        return true;
    }

    size_t colonPos = domain.find(':');
    if (colonPos != std::string::npos) {
        hostname = domain.substr(0, colonPos);
        try {
            port = std::stoi(domain.substr(colonPos + 1));
        } catch (const std::invalid_argument &e) {
            std::cerr << "Invalid port number." << std::endl;
            return false;
        } catch (const std::out_of_range &e) {
            std::cerr << "Port number out of range." << std::endl;
            return false;
        }
    } else {
        hostname = domain;
    }

    if (hostname.empty()) {
        std::cerr << "Host not found in request." << std::endl;
        return false;
    }
    return true;
}

// Initialization functions

void InitWinsock();
void InitOpenSSL();
void clientHandler(SOCKET);
void HTTPHandler(SOCKET, char*, int);
void HTTPSHandler(SOCKET, char*, int);
void relayData(SSL* clientSSL, SSL* serverSSL);
SSL_CTX* CreateSSLContext();

int main() {
    InitWinsock();
    InitOpenSSL();

    SSL_CTX* ctx = CreateSSLContext();
    if (!ctx) {
        std::cerr << "SSL context creation failed." << std::endl;
        return -1;
    }

    // Set up server socket
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(listenSocket, SOMAXCONN);

    std::cout << "MITM Proxy listening on port 8080..." << std::endl;

    while (true) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Failed to accept client connection." << std::endl;
            continue;
        }
        clientHandler(clientSocket);
    }

    SSL_CTX_free(ctx);
    WSACleanup();
    return 0;
}

void InitWinsock() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

void InitOpenSSL() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

SSL_CTX* CreateSSLContext() {
    const SSL_METHOD* method = SSLv23_server_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return nullptr;
    }
    if (SSL_CTX_use_certificate_file(ctx, "./CERTIFICATE/root.crt", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "./CERTIFICATE/root.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return nullptr;
    }
    return ctx;
}


void clientHandler(SOCKET clientSocket) {
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        closesocket(clientSocket);
        return;
    }
    buffer[bytesReceived] = '\0';

    std::string request(buffer, bytesReceived);

    std::string host; int port; getHostFromRequest(request, host, port);
    hostent* hostInfo = gethostbyname(host.c_str());
    // Resolve the host name to an IP address
    if (hostInfo == nullptr) {
        std::cerr << "Failed to resolve host name." << std::endl;
        closesocket(clientSocket);
        return;
    }
    // Kiểm tra nếu là yêu cầu CONNECT (HTTPS)
    if (port == HTTPS_PORT) {
        HTTPSHandler(clientSocket, buffer, bytesReceived);
    } else {
        HTTPHandler(clientSocket, buffer, bytesReceived);
    }
}

void HTTPHandler(SOCKET clientSocket, char* buffer, int bytesReceived) {
    // Xử lý yêu cầu HTTP không mã hóa

    // Parse và kết nối đến server đích cho HTTP
    std::string request(buffer, bytesReceived);
    std::string host; int port; getHostFromRequest(request, host, port);
    hostent* hostInfo = gethostbyname(host.c_str());
    SOCKET remoteSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in targetAddr;
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port = htons(80);
    targetAddr.sin_addr.s_addr = *((unsigned long*)hostInfo->h_addr_list[0]);

    if (connect(remoteSocket, (sockaddr*)&targetAddr, sizeof(targetAddr)) == SOCKET_ERROR) {
        closesocket(remoteSocket);
        closesocket(clientSocket);
        return;
    }

    fd_set readfds;
    while (true) {
        const int bufferSize = 512;
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);
        FD_SET(remoteSocket, &readfds);
        TIMEVAL delay; delay.tv_sec = 2; delay.tv_usec=0;
        int activity = select(0, &readfds, nullptr, nullptr, &delay);
        if (activity <= 0) break;
        if (FD_ISSET(clientSocket, &readfds)) {
            int bytesReceived = bufferSize;
            if (recv(clientSocket, buffer, bytesReceived, 0) <= 0) break;
            if (send(remoteSocket, buffer, bytesReceived, 0) <= 0) break;
        }
        if (FD_ISSET(remoteSocket, &readfds)) { 
            int bytesReceived = bufferSize;
            if (recv(remoteSocket, buffer, bytesReceived, 0) <= 0) break; 
            if (send(clientSocket, buffer, bytesReceived, 0) <= 0) break;
        }
    }
    closesocket(remoteSocket);
    closesocket(clientSocket);
}

SSL_CTX* createSSLContext(const char* certFile, const char* keyFile) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!SSL_CTX_use_certificate_file(ctx, certFile, SSL_FILETYPE_PEM) ||
        !SSL_CTX_use_PrivateKey_file(ctx, keyFile, SSL_FILETYPE_PEM)) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return nullptr;
    }
    return ctx;
}

// Kết nối HTTPS đến server đích qua OpenSSL
SSL* connectToTargetServer(const std::string& host, int port) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        std::cerr << "CANNOT create SSL for server.\n";
        return nullptr;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        SSL_CTX_free(ctx); // Đảm bảo giải phóng ctx khi gặp lỗi
        return nullptr;
    }
    // Địa chỉ server
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    hostent* hostInfo = gethostbyname(host.c_str());
    memcpy(&serverAddr.sin_addr, hostInfo->h_addr_list[0], hostInfo->h_length);

    if (connect(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "CANNOT connect to server.\n";
        closesocket(serverSocket);
        return nullptr;
    }
    std::cerr << "CONNECTED SERVER: " << host << " " << port << '\n';

    SSL* serverSSL = SSL_new(ctx);
    SSL_set_fd(serverSSL, serverSocket);

    if (SSL_connect(serverSSL) <= 0) {
        int err = SSL_get_error(serverSSL, -1);
        std::cerr << "SSL_connect failed with error code: " << err << '\n';
        ERR_print_errors_fp(stderr);
        closesocket(serverSocket);
        SSL_free(serverSSL);
        SSL_CTX_free(ctx);
        return nullptr;
    }
    
    return serverSSL;
}

void HTTPSHandler(SOCKET clientSocket, char* buffer, int bytesReceived) {
    std::string request(buffer);
    std::string host; int port; getHostFromRequest(request, host, port);
    std::cerr << host << " " << port << '\n';

    const char* response = "HTTP/1.1 200 Connection Established\r\n\r\n";
    int byteSent = send(clientSocket, response, strlen(response), 0);

    SSL_CTX* clientCtx = createSSLContext("./CERTIFICATE/wikipedia.crt", "./CERTIFICATE/wikipedia.key");
    if (!clientCtx) return;

    SSL* clientSSL = SSL_new(clientCtx);
    SSL_set_fd(clientSSL, clientSocket);

    if (SSL_get_verify_result(clientSSL) != X509_V_OK) {
        std::cerr << "SSL certificate verification failed\n";
        return;
    }


    if (SSL_accept(clientSSL) <= 0) {
        std::cerr << "CANNOT ACCEPT clientSSL\n";
        ERR_print_errors_fp(stderr);
        SSL_free(clientSSL);
        SSL_CTX_free(clientCtx);
        closesocket(clientSocket);
        return;
    }


    // Kết nối đến server thực qua HTTPS
    SSL* serverSSL = connectToTargetServer(host, port);
    if (!serverSSL) {
        std::cerr << "Cannot connect to server.\n";
        SSL_shutdown(clientSSL);
        SSL_free(clientSSL);
        SSL_CTX_free(clientCtx);
        closesocket(clientSocket);
        return;
    }

    std::cout << "SSL connection established with client." << std::endl;
    relayData(clientSSL, serverSSL);

    SSL_shutdown(clientSSL);
    SSL_free(clientSSL);
    closesocket(clientSocket);
}

std::string processHTTPRequest(const std::string& request) {
    std::stringstream input(request);
    std::stringstream output;

    std::string line;
    bool isFirstLine = true;

    while (std::getline(input, line) && !line.empty()) {
        if (line.back() == '\r') line.pop_back();

        if (isFirstLine) {
            // Dòng đầu tiên (method và URL)
            std::string method, url, version;
            std::istringstream firstLine(line);
            firstLine >> method >> url >> version;

            if (url.find("http://") == 0 || url.find("https://") == 0) {
                size_t pathStart = url.find('/', url.find("//") + 2);
                url = (pathStart != std::string::npos) ? url.substr(pathStart) : "/";
            }

            output << method << " " << url << " " << version << "\r\n";
            isFirstLine = false;
        } else if (line.find("Proxy-Connection:") == 0) {
            // Bỏ Proxy-Connection
            continue;
        } else if (line.find("Connection:") == 0) {
            output << "Connection: close\r\n";
        } else if (line.find("Accept-Encoding:") == 0) {
            output << "Accept-Encoding: gzip\r\n";
        } else if (line.find("Sec-F") == 0 || line.find("Upgrade-Insecure-Requests:") == 0) {
            continue;
        } else {
            output << line << "\r\n";
        }
    }

    // Thêm dòng trống cuối header
    output << "\r\n";

    return output.str();
}


void relayData(SSL* clientSSL, SSL* serverSSL) {
    std::cerr << "HELLO\n\n\n";
    fd_set readfds;
    const int bufferSize = 512;
    char buffer[bufferSize];
    int step = 0;
    while (true) {
        std::cerr << "STEP: " << ++step << '\n';
        FD_ZERO(&readfds);
        int clientFd = SSL_get_fd(clientSSL);
        int serverFd = SSL_get_fd(serverSSL);

        FD_SET(clientFd, &readfds);
        FD_SET(serverFd, &readfds);

        TIMEVAL timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        int maxFd = std::max(clientFd, serverFd) + 1; // Lấy giá trị lớn nhất của các file descriptors
        int activity = select(maxFd, &readfds, nullptr, nullptr, &timeout);

        if (activity == -1) {
            int errorCode = WSAGetLastError();
            std::cerr << "Error in select(): " << errorCode << ". Exiting...\n";
            break;
        }
        if (activity == 0) {
            break;
        }
        // Xử lý dữ liệu từ client đến server
        if (FD_ISSET(clientFd, &readfds)) {
            std::cerr << "IN CLIENT\n";
            int bytesReceived = SSL_read(clientSSL, buffer, bufferSize);
            if (bytesReceived <= 0) {
                std::cerr << "Client disconnected or error reading from client.\n";
                break;
            }
            std::cerr << "Data from client: \n" << std::string(buffer, bytesReceived) << '\n';

            std::string tmp = std::string(buffer, bytesReceived);
            tmp = processHTTPRequest(tmp);
            std::cerr << tmp << '\n';
            int bytesSent = SSL_write(serverSSL, tmp.c_str(), tmp.size());
            if (bytesSent <= 0) {
                std::cerr << "Error writing to server.\n";
                break;
            }
        }

        // Xử lý dữ liệu từ server đến client
        if (FD_ISSET(serverFd, &readfds)) {
            std::cerr << "IN SERVER\n";
            int bytesReceived = SSL_read(serverSSL, buffer, bufferSize);
            if (bytesReceived <= 0) {
                std::cerr << "Server disconnected or error reading from server.\n";
                break;
            }
            std::cerr << "Data from server: \n" << std::string(buffer, bytesReceived) << '\n';
            std::cerr << "SENT\n";
            int bytesSent = SSL_write(clientSSL, buffer, bytesReceived);
            if (bytesSent <= 0) {
                std::cerr << "Error writing to client.\n";
                break;
            }
        }
    }
}