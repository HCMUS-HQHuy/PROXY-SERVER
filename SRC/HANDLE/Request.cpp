#include "./../../HEADER/Request.h"
#include <vector>
#include <sstream>
#include <string>
#include <unordered_map>

RequestHandler::RequestHandler(SocketHandler* _socketHandler){
    socketHandler = _socketHandler;
    HttpHandler();
}

bool RequestHandler::handleRequest() {
    while (isEndMessage() == false) {
        int bytesReceived = receiveMessage(browser);
        // std::cerr << "RECEIVED : " << bytesReceived << '\n';
        // printHeader();
        int bytesSent = sendMessage(server, bytesReceived);
    }
    // printHeader();
    return true;
}