#include "../HEADER/setting.h"
#include "../HEADER/supportFunction.h"

#pragma comment(lib, "ws2_32.lib")

WSADATA wsaData; 
SOCKET localSocket, remoteSocket;

// Function to parse and get the host name from the HTTP request
bool getHostFromRequest(const std::string request, std::string &hostname, int &port) {
    port = 80;
    std::string doman = request.substr(request.find("Host: ") + 6);
    size_t colonPos = doman.find(':');  
    if (colonPos != std::string::npos) {
        hostname = doman.substr(0, colonPos);           // Lấy phần tên miền
        port = std::stoi(doman.substr(colonPos + 1));   // Lấy phần cổng
    } else {
        hostname = doman;  // Nếu không có cổng, dùng tên miền như đã nhập
    }
    if (hostname.empty()) {
        std::cerr << "Host not found in request." << std::endl;
        return false;
    }  
    return true;
}

void closeAll();
bool isInitializedWinsock();
bool isInitializedSocket(SOCKET &socket);
bool bindSocketToAdressPort(ADDRESS_FAMILY, u_long, int);
bool handleClient();
 
int runServerProxy() {
    WSADATA wsaData; if (not isInitializedWinsock()) return EXISTS_ERORRS;
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

    // while (true) {
    //     if (!handleClient())
    //         break;
    // }

    handleClient();

    closeAll();
    return 0;
}

void closeAll() {
    closesocket(remoteSocket);
    closesocket(localSocket);
    WSACleanup();
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
        closeAll();
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
        closeAll();
        return false;
    }
    return true;
}

bool handleClient() {
        // Accept a connection from a client
    SOCKET clientSocket = accept(localSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Error accepting connection from client." << std::endl;
        closeAll();
        return false;
    }

    // Receive request from client
    char buffer[9000];
    int recvSize = 10;
    recvSize = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (recvSize == SOCKET_ERROR) {
        std::cerr << "Error receiving request from client." << std::endl;
        closesocket(clientSocket);
        closeAll();
        return false;
    }

    // Convert request to string and get the host name
    std::string request(buffer, recvSize);
    std::cerr << request << "\n";
    std::string host; int port; getHostFromRequest(request, host, port);
    hostent* hostInfo = gethostbyname(host.c_str());
    // Resolve the host name to an IP address
    if (hostInfo == NULL) {
        std::cerr << "Failed to resolve host name." << std::endl;
        closesocket(clientSocket);
        closeAll();
        return false;

    }
    const char* connectResponse = "HTTP/1.1 200 Connection Established\r\n\r\n";
    send(clientSocket, connectResponse, strlen(connectResponse), 0);
    // Create a socket to connect to the remote server
    if (not isInitializedSocket(remoteSocket)) return EXISTS_ERORRS;
    // Set the remote server address
    sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(port); 
    remoteAddr.sin_addr.s_addr = *((unsigned long*)hostInfo->h_addr_list[0]);

    // Connect to the remote server
    if (connect(remoteSocket, (sockaddr*)&remoteAddr, sizeof(remoteAddr)) == SOCKET_ERROR) {
        std::cerr << "Error connecting to remote server." << std::endl;
        closesocket(clientSocket);
        closeAll();
        return false;
    }

    fd_set readfds;
    while (true) {
        const int bufferSize = 4096;
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);
        FD_SET(remoteSocket, &readfds);

        int activity = select(0, &readfds, nullptr, nullptr, nullptr);
        if (activity <= 0) break;

        if (FD_ISSET(clientSocket, &readfds)) {
            int bytesReceived = recv(clientSocket, buffer, bufferSize, 0);
            if (bytesReceived <= 0) break;
            send(remoteSocket, buffer, bytesReceived, 0);
        }

        if (FD_ISSET(remoteSocket, &readfds)) {
            int bytesReceived = recv(remoteSocket, buffer, bufferSize, 0);
            if (bytesReceived <= 0) break;
            send(clientSocket, buffer, bytesReceived, 0);
        }
        std::cerr << "RUNNING...\n";
    }
    closesocket(clientSocket);
    return true;
}