#include "./../../HEADER/Response.h"

ResponseHandler::ResponseHandler(SocketHandler* _socketHandler){
    socketHandler = _socketHandler;
    HttpHandler();
}

bool ResponseHandler::handleResponse() {
    while (isEndMessage() == false) {
        std::cerr << "IN RESPONSE\n";
        int bytesReceived = receiveMessage(server);
        // std::cerr << "byteReceived: " << bytesReceived << "->" << isEndMessage() << "\n";
        int bytesSent = sendMessage(browser, bytesReceived);
    }
    std::cerr << "ENDDDDD\n";
    return true;
}
