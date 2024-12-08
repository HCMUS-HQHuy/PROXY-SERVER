#include "./../../HEADER/ThreadPool.hpp"
#include "./../../HEADER/ProxyServer.hpp"
#include "./../../HEADER/ClientHandler.hpp"
#include "./../../HEADER/Logger.hpp"
#include <fstream>

ProxyServer::ProxyServer(Proxy t, int p) {
    type = t; port = p;
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;  
    serverAddr.sin_port = htons(port);     

    if (bind(localSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        Logger::errorStatus(-41);
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        return;
    }
    std::cerr << "Server initialized successfully!\n";
}

void ProxyServer::start() {
    waitingClient();

    while (ServerRunning) {
        SOCKET client = acceptClient();
        if (client != INVALID_SOCKET) {
            std::shared_ptr<ClientHandler> h = std::make_shared<ClientHandler>(type == MITM);
            if (h->handleConnection(client))
                requestHandlerPool.enqueue(std::move(h));
            // ClientHandler h(client);
            // if (h.handleConnection(client)) 
            //     h.handleRequest();
        }
    }
    Logger::errorStatus(-42);
}

void ProxyServer::stop(int signum) {
    Logger::errorStatus(-43);
    std::cout << "Interrupt signal (" << signum << ") received. Stopping server..." << std::endl;
    ServerRunning = false; // Dừng server
}

void ProxyServer::waitingClient() {
    if (listen(localSocket, SOMAXCONN) == SOCKET_ERROR) {
        Logger::errorStatus(-44);
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        return;
    }
    std::cout << "Server is listenning on IPv4: " << IPv4 << " PORT: " <<  port << "...\n";
    std::cout << "(local) use loopback IPaddress : 127.0.0.1 PORT: " <<  port << "...\n";
}

SOCKET ProxyServer::acceptClient() {
    SOCKET clientSocket = accept(localSocket, nullptr, nullptr);
    if (clientSocket == INVALID_SOCKET) {
        Logger::errorStatus(-45);
        return INVALID_SOCKET;
    }
    return clientSocket;
}
