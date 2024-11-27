#include <chrono>

#include "./../../HEADER/ClientHandler.h"


ClientHandler::ClientHandler(SOCKET sock) {
    // std::cerr << "Create new client Handler\n";
    // activeThreads++;
    // std::cout << "Thread started. Active threads: " << activeThreads.load() << std::endl;
    socketHandler = new SocketHandler(sock);
}

ClientHandler::~ClientHandler() {
    delete socketHandler;
    // activeThreads--;
    // std::cout << "Thread finished. Active threads: " << activeThreads.load() << std::endl;
    // std::cerr << "Destructure client Handler\n";
}

void ClientHandler::handleRequest() {
    if (socketHandler->isValid() == false) return;
    if (socketHandler->setSSLContexts() == false) return;

    struct pollfd fds[2];
    for (int i: {0, 1}) {
        fds[i].fd = socketHandler->socketID[i];
        fds[i].events = POLLIN;  
    }

    #define TIMEOUT 100
    #define MAX_IDLE_TIME 5000

    auto lastActivity = std::chrono::steady_clock::now();
    // int STEP = 0;
    while (true) {
        int ret = WSAPoll(fds, 2, TIMEOUT);

        for (int t = 0; t < 2; ++t)
            if (fds[t].revents & (POLLERR | POLLHUP)) {
                std::cerr << (t == 0 ? "BROWSER" : "REMOTE") << " HAVE SOME PROBLEM!!\n";
                return;
            }

        if (ret < 0) {
            std::cerr << "WSAPoll ERROR!\n";
            break;
        } 
        else if (ret == 0) {
            // Kiểm tra idle timeout
            const auto& currentTime = std::chrono::steady_clock::now();
            const auto& idleDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastActivity).count(); 
            if (idleDuration > MAX_IDLE_TIME) {
                std::cerr << "TIMEOUT\n";
                break;
            }
            // std::cerr <<"NO activity IN " << idleDuration << '\n';
            continue;
        }
        // bool ok = false;
        // if (!ok) {
        //     std::cerr << "NO SOCKET!!\n";
        //     break;
        // }
        lastActivity = std::chrono::steady_clock::now();

        if (fds[0].revents & POLLIN) {
            std::cerr << "IN browser\n";
            RequestHandler request(socketHandler);
            if (request.handleRequest() == false) break;
        }
        if (fds[1].revents & POLLIN) {
            std::cerr << "IN server\n";
            ResponseHandler response(socketHandler);
            if (response.handleResponse() == false) break;
        }
    }
}
