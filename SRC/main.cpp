#include <iostream>
#include <winsock2.h>
#include <string>
#include <sstream>
#pragma comment(lib, "ws2_32.lib")

#define LOCAL_PORT 8080

// Function to parse and get the host name from the HTTP request
std::string getHostFromRequest(const std::string& request) {
    std::cout << request << '\n';
    std::istringstream stream(request);
    std::string line;

    // Iterate over the header lines to find the "Host" field
    while (std::getline(stream, line)) {
        if (line.find("Host:") != std::string::npos) {
            // Extract the host name from the "Host: ..." line
            std::string tmp = line.substr(6);
            while (tmp.back() != ':') tmp.pop_back();
            tmp.pop_back();
            return tmp; // Skip "Host: "
        }
    }
    return "";
}

int main() {
    WSADATA wsaData;
    SOCKET localSocket, remoteSocket;
    sockaddr_in localAddr;
    char buffer[4096];
    int recvSize;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error initializing Winsock." << std::endl;
        return 1;
    }

    // Create a socket to listen for client connections
    localSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (localSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket." << std::endl;
        WSACleanup();
        return 1;
    }

    // Set address and port for the listening socket
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(LOCAL_PORT);

    // Bind the listening socket to the address and port
    if (bind(localSocket, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
        std::cerr << "Error binding socket." << std::endl;
        closesocket(localSocket);
        WSACleanup();
        return 1;
    }

    // Start listening for connections
    if (listen(localSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Error listening on socket." << std::endl;
        closesocket(localSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Proxy is listening on port " << LOCAL_PORT << "..." << std::endl;

    std::cerr << "HQ\n";
    // Accept a connection from a client
    SOCKET clientSocket = accept(localSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Error accepting connection from client." << std::endl;
        closesocket(localSocket);
        WSACleanup();
        return 1;
    }

    // Receive request from client
    recvSize = recv(clientSocket, buffer, sizeof(buffer), 0);
    std::cerr << buffer << '\n';
    if (recvSize == SOCKET_ERROR || recvSize == 0) {
        std::cerr << "Error receiving request from client." << std::endl;
        closesocket(clientSocket);
        closesocket(localSocket);
        WSACleanup();
        return 1;
    }

    // Convert request to string and get the host name
    std::string request(buffer, recvSize);
    std::string host = getHostFromRequest(request);
    if (host.empty()) {
        std::cerr << "Host not found in request." << std::endl;
        closesocket(clientSocket);
        closesocket(localSocket);
        WSACleanup();
        return 1;
    }

    // Create a socket to connect to the remote server
    remoteSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (remoteSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket to connect to remote server." << std::endl;
        closesocket(clientSocket);
        closesocket(localSocket);
        WSACleanup();
        return 1;
    }

    // Set the remote server address
    sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(80); // Assume HTTP connection on port 80

    // Resolve the host name to an IP address
    std::cerr << host << '\n';
    hostent* hostInfo = gethostbyname(host.c_str());
    if (hostInfo == NULL) {
        std::cerr << "Failed to resolve host name." << std::endl;
        closesocket(remoteSocket);
        closesocket(clientSocket);
        closesocket(localSocket);
        WSACleanup();
        return 1;
    }
    remoteAddr.sin_addr.s_addr = *((unsigned long*)hostInfo->h_addr);

    // Connect to the remote server
    if (connect(remoteSocket, (sockaddr*)&remoteAddr, sizeof(remoteAddr)) == SOCKET_ERROR) {
        std::cerr << "Error connecting to remote server." << std::endl;
        closesocket(remoteSocket);
        closesocket(clientSocket);
        closesocket(localSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to remote server: " << host << std::endl;

    // Send the request to the remote server
    send(remoteSocket, request.c_str(), request.length(), 0);

    // Receive and forward response to the client
    while ((recvSize = recv(remoteSocket, buffer, sizeof(buffer), 0)) > 0) {
        send(clientSocket, buffer, recvSize, 0);
    }

    // Close all sockets
    closesocket(remoteSocket);
    closesocket(clientSocket);
    closesocket(localSocket);
    WSACleanup();
    return 0;
}
