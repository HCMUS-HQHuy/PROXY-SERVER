#include "../HEADER/setting.h"
#include "../HEADER/supportFunction.h"

#pragma comment(lib, "ws2_32.lib")

WSADATA wsaData; 
SOCKET localSocket;

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

bool isInitializedWinsock();
bool isInitializedSocket(SOCKET &socket);
bool bindSocketToAdressPort(ADDRESS_FAMILY, u_long, int);
bool handleClient();
 
int runServerProxy() {
    if (not isInitializedWinsock()) return EXISTS_ERORRS;
    if (not isInitializedSocket(localSocket)) return EXISTS_ERORRS;

    if (not bindSocketToAdressPort(AF_INET, INADDR_ANY, LOCAL_PORT)) return EXISTS_ERORRS;

    // Start listening for connections
    if (listen(localSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Error listening on socket." << std::endl;
        closesocket(localSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Proxy is listening on port " << LOCAL_PORT << "..." << std::endl;

    while (true) {
        if (handleClient() == false) 
            std::cerr << "HAVE SOME PROBLEMS\n"; 
        std::cerr << "END CLIENT!\n";
        // std::thread t(handleClient);
        // t.detach();
    }
    closesocket(localSocket);
    WSACleanup();
    return 0;
}

bool isInitializedWinsock() {
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error initializing Winsock." << std::endl;
        return false;
    }
    return true;
}

bool isInitializedSocket(SOCKET &sock) {
    // Create a socket to listen for client connections
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Error creating socket." << std::endl;
        closesocket(localSocket);
        WSACleanup();
        return false;
    }
    return true;
}

bool bindSocketToAdressPort(ADDRESS_FAMILY sin_family, u_long s_address, int port) {
    sockaddr_in localAddr;
    // Set address and port for the listening socket
    localAddr.sin_family = sin_family;
    localAddr.sin_addr.s_addr = s_address;
    localAddr.sin_port = htons(port);

    // Bind the listening socket to the address and port
    if (bind(localSocket, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
        std::cerr << "ERROR binding socket!!!!" << std::endl;
        closesocket(localSocket);
        WSACleanup();
        return false;
    }
    return true;
}

bool handleClient() {
    SOCKET clientSocket = accept(localSocket, nullptr, nullptr);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Error accepting connection from client." << std::endl;
        return false;
    }
    // Receive request from client
    const int bufferSize = 512;
    char buffer[bufferSize]; int recvSize = bufferSize;
    memset(buffer, 0, sizeof buffer);
    if (not receiveMSG(clientSocket, buffer, recvSize)) {
        closesocket(clientSocket);
        return false;
    }

    // Convert request to string and get the host name
    std::string request(buffer, recvSize);
    // std::cerr << "SIZE\n" << recvSize << "\n" << request << '\n';
    std::string host; int port; getHostFromRequest(request, host, port);
    hostent* hostInfo = gethostbyname(host.c_str());
    // Resolve the host name to an IP address
    if (hostInfo == nullptr) {
        std::cerr << "Failed to resolve host name." << std::endl;
        closesocket(clientSocket);
        return false;
    }
    SOCKET remoteSocket;
    if (not isInitializedSocket(remoteSocket)) {
        closesocket(clientSocket);
        return EXISTS_ERORRS;
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
        closesocket(clientSocket);
        return false;
    }

    if (port == HTTPS_PORT) {
        char connectResponse[] = "HTTP/1.1 200 Connection Established\r\n\r\n"; int len = strlen(connectResponse);
        if (not sendMSG(clientSocket, connectResponse, len)) {
            closesocket(remoteSocket);
            closesocket(clientSocket);
            return false;
        }
    }

    // std::cerr << request << "\n";

    fd_set readfds;
    while (true) {
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);
        FD_SET(remoteSocket, &readfds);
        TIMEVAL delay; delay.tv_sec = 2; delay.tv_usec=0;
        int activity = select(0, &readfds, nullptr, nullptr, &delay);
        if (activity <= 0) break;
        if (FD_ISSET(clientSocket, &readfds)) {
            int bytesReceived = bufferSize;
            if (not receiveMSG(clientSocket, buffer, bytesReceived)) {
                std::cerr << "FROM clientSocket\n";
                break;
            }
            if (not sendMSG(remoteSocket, buffer, bytesReceived)) break;
        }
        if (FD_ISSET(remoteSocket, &readfds)) { 
            int bytesReceived = bufferSize;
            if (not receiveMSG(remoteSocket, buffer, bytesReceived)) {
                std::cerr << "FROM remoteSocket\n";
                break; 
            }
            if (not sendMSG(clientSocket, buffer, bytesReceived)) break;
        }
    }
    closesocket(remoteSocket);
    closesocket(clientSocket);
    return true;
}