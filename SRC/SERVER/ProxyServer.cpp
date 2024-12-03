#include "./../../HEADER/ThreadPool.hpp"
#include "./../../HEADER/ProxyServer.hpp"
#include "./../../HEADER/ClientHandler.hpp"
#include <fstream>

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
    std::signal(SIGINT, stop);
    waitingClient();

    while (ServerRunning) {
        SOCKET client = acceptClient();
        if (client != INVALID_SOCKET) {
            std::shared_ptr<ClientHandler> h = std::make_shared<ClientHandler>(client);
            if (h->handleConnection(client))
                requestHandlerPool.enqueue(std::move(h));
            else {
                std::cerr << "CANNOT CONNECT TO BROWSER\n";
            }
            // ClientHandler h(client);
            // if (h.connectToBrowser(client)) 
            //     h.handleRequest();
        }
    }
    std::cerr << "in proxy server END!!\n";
}

void ProxyServer::stop(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received. Stopping server..." << std::endl;
    ServerRunning = false; // Dừng server
}

void ProxyServer::waitingClient() {
    if (listen(localSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        return;
    }
    std::cout << "Server is listenning on IPv4: " << IPv4 << " PORT: " <<  port << "...\n";
    std::cout << "(local) use loopback IPaddress : 127.0.0.1 PORT: " <<  port << "...\n";
}

SOCKET ProxyServer::acceptClient() {
    SOCKET clientSocket = accept(localSocket, nullptr, nullptr);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Error accepting connection from client." << std::endl;
        return INVALID_SOCKET;
    }
    return clientSocket;
}
