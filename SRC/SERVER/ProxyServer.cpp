#include "./../../HEADER/ProxyServer.h"
#include "./../../HEADER/ClientHandler.h"
#include <thread>
#include <bits/shared_ptr.h>

ProxyServer::ProxyServer(int p) {
    port = p;
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;  
    serverAddr.sin_port = htons(port);     

    if (bind(localSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        return;
    }
    std::cerr << "Server initialized successfully!\n";
}

void ProxyServer::start() {
    waitingClient();
    while (true) {
        SOCKET client = acceptClient();
        ClientHandler handler(client);
        handler.handleRequest();
    } 
    // while (true) {
    //     SOCKET client = acceptClient();
    //     if (client != INVALID_SOCKET) {
    //         auto handler = std::make_shared<ClientHandler>(client);  
    //         std::thread clientThread(&ClientHandler::handleRequest, handler);
    //         clientThread.detach();
    //     }
    // }

}

void ProxyServer::stop() {
}

void ProxyServer::waitingClient() {
    if (listen(localSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        return;
    }
    std::cerr << "Server is listenning on IPv4: " << IPv4 << " PORT: " <<  port << "...\n";
}

SOCKET ProxyServer::acceptClient() {
    SOCKET clientSocket = accept(localSocket, nullptr, nullptr);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Error accepting connection from client." << std::endl;
        return INVALID_SOCKET;
    }
    return clientSocket;
}
