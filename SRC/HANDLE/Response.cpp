#include "./../../HEADER/Response.hpp"
#include "./../../HEADER/Logger.hpp"

ResponseHandler::ResponseHandler(SocketHandler* _socketHandler){
    socketHandler = _socketHandler;
    HttpHandler();
}

void ResponseHandler::handleResponse() {
    while (ServerRunning && isEndMessage() == false) {
        int bytesReceived = receiveMessage(server);
        sendMessage(browser, bytesReceived);
    }
}
