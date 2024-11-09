#include "./../../HEADER/ClientHandler.h"


ClientHandler::ClientHandler(SOCKET sock) {
    clientSocket = sock;
}

ClientHandler::~ClientHandler() {
    closesocket(clientSocket);
}

void ClientHandler::handleRequest() {
    Message message; message.receiveMessage(clientSocket);
    string host; int port; 
    message.getHostFromRequest(host, port);
    hostent* hostInfo = gethostbyname(host.c_str());
    message.print();
    std::cerr << host << " " << port <<'\n';
    if (hostInfo == nullptr) {
        std::cerr << "Failed to resolve host name." << std::endl;
        return;
    }
    
    // Create a socket to listen for client connections
    SOCKET remoteSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (remoteSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket." << std::endl;
        return;
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
        return;
    }

    if (port == HTTPS_PORT) {
        Message connectMessage("HTTP/1.1 200 Connection Established\r\n\r\n");
        if (connectMessage.sendMessage(clientSocket) == SOCKET_ERROR) {
            closesocket(remoteSocket);
            return;
        }
    }

    fd_set readfds;
    int step = 0;
    while (true) {
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);
        FD_SET(remoteSocket, &readfds);
        TIMEVAL delay; delay.tv_sec = 5; delay.tv_usec=0;
        int activity = select(0, &readfds, nullptr, nullptr, &delay);
        if (activity <= 0) break;
        if (FD_ISSET(clientSocket, &readfds)) {
            Message request; 
            int byteReceive = request.receiveMessage(clientSocket);
            if (byteReceive <= 0) break;
            if (request.sendMessage(remoteSocket, byteReceive) <= 0) break;
        }
        if (FD_ISSET(remoteSocket, &readfds)) {
            Message response;
            int byteReceive = response.receiveMessage(remoteSocket);
            if (byteReceive <= 0) break;
            if (response.sendMessage(clientSocket, byteReceive) <= 0) break;
        }
        std::cerr << ++step << "\n";
    }
    closesocket(remoteSocket);
}

