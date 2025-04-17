#include "./../../HEADER/ThreadPool.hpp"
#include "./../../HEADER/ProxyServer.hpp"
#include "./../../HEADER/ClientHandler.hpp"
#include "./../../HEADER/Logger.hpp"
#include <fstream>

std::atomic<bool> ServerRunning;

ProxyServer::ProxyServer(Proxy t, int p) {
    type = t; port = p;
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;  
    serverAddr.sin_port = htons(port);     

    if (bind(localSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        logger.logError(-41);
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        return;
    }
    std::cerr << "Server initialized successfully!\n";
}

void ProxyServer::start() {
    ServerRunning = true;
    waitingClient();

    while (ServerRunning) {
        SOCKET client = acceptClient();
        if (client != INVALID_SOCKET) {
            requestHandlerPool.enqueue(static_cast<std::function<void()>>([client, this]() mutable{
                ClientHandler h(this->type == MITM);
                if (h.handleConnection(client))
                    h.handleRequest();
            }));

            // ClientHandler h(client);
            // if (h.handleConnection(client)) 
            //     h.handleRequest();
        }
    }
    logger.logError(-42);
}

void ProxyServer::stop(int signum) {
    logger.logError(-43);
    std::cout << "Interrupt signal (" << signum << ") received. Stopping server..." << std::endl;
    ServerRunning = false; // Dừng server
}

void ProxyServer::waitingClient() {
    if (listen(localSocket, SOMAXCONN) == SOCKET_ERROR) {
        logger.logError(-44);
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        return;
    }
    std::cout << "Server is listenning on IPv4: " << IPv4 << " PORT: " <<  port << "...\n";
    std::cout << "(local) use loopback IPaddress : 127.0.0.1 PORT: " <<  port << "...\n";
}

SOCKET ProxyServer::acceptClient() {
    SOCKET clientSocket = accept(localSocket, nullptr, nullptr);
    if (clientSocket == INVALID_SOCKET) {
        logger.logError(-45);
        return INVALID_SOCKET;
    }
    return clientSocket;
}

Proxy ProxyServer::getType() {
    return type;
}