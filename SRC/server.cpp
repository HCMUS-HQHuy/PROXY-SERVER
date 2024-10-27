#include "../HEADER/setting.h"
#include "../HEADER/supportFunction.h"

#pragma comment(lib, "ws2_32.lib")

WSADATA wsaData; 
SOCKET localSocket, remoteSocket;

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
            return tmp;
        }
    }
    return "";
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
    char buffer[4096];
    int recvSize;
    recvSize = recv(clientSocket, buffer, sizeof(buffer), 0);
    std::cerr << buffer << '\n';
    if (recvSize == SOCKET_ERROR || recvSize == 0) {
        std::cerr << "Error receiving request from client." << std::endl;
        closesocket(clientSocket);
        closeAll();
        return false;
    }

    // Convert request to string and get the host name
    std::string request(buffer, recvSize);
    std::string host = getHostFromRequest(request);
    if (host.empty()) {
        std::cerr << "Host not found in request." << std::endl;
        closesocket(clientSocket);
        closeAll();
        return false;
    }

    // Create a socket to connect to the remote server
    if (not isInitializedSocket(remoteSocket)) return EXISTS_ERORRS;

    // Resolve the host name to an IP address
    std::cerr << host << '\n';
    hostent* hostInfo = gethostbyname(host.c_str());
    if (hostInfo == NULL) {
        std::cerr << "Failed to resolve host name." << std::endl;
        closesocket(clientSocket);
        closeAll();
        return false;
    }

    // Set the remote server address
    sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(80); // Assume HTTP connection on port 80
    remoteAddr.sin_addr.s_addr = *((unsigned long*)hostInfo->h_addr);

    // Connect to the remote server
    if (connect(remoteSocket, (sockaddr*)&remoteAddr, sizeof(remoteAddr)) == SOCKET_ERROR) {
        std::cerr << "Error connecting to remote server." << std::endl;
        closesocket(clientSocket);
        closeAll();
        return false;
    }

    std::cout << "Connected to remote server: " << host << std::endl;

    // Send the request to the remote server
    send(remoteSocket, request.c_str(), request.length(), 0);

    // Receive and forward response to the client
    while ((recvSize = recv(remoteSocket, buffer, sizeof(buffer), 0)) > 0) {
        std::cerr << "get\n";
        send(clientSocket, buffer, recvSize, 0);
        std::cerr << "Finish get\n";
    }
    closesocket(clientSocket);
    return true;
}