#include "./../../HEADER/Request.hpp"
#include <vector>
#include <sstream>
#include <string>
#include <unordered_map>

RequestHandler::RequestHandler(SocketHandler* _socketHandler){
    socketHandler = _socketHandler;
    HttpHandler();
}

bool RequestHandler::handleRequest() {
    while (ServerRunning && isEndMessage() == false) {
        int bytesReceived = receiveMessage(browser);
        // std::cerr << "RECEIVED : " << bytesReceived << '\n';
        // printHeader();
        sendMessage(server, bytesReceived);
    }
    // printHeader();
    return true;
}