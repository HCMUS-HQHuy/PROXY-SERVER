#include "./../../HEADER/Response.hpp"

ResponseHandler::ResponseHandler(SocketHandler* _socketHandler){
    socketHandler = _socketHandler;
    HttpHandler();
}

bool ResponseHandler::handleResponse() {
    // std::cerr << "IN RESPONSE\n";
    while (ServerRunning && isEndMessage() == false) {
        int bytesReceived = receiveMessage(server);
        // std::cerr << "byteReceived: " << bytesReceived << "->" << isEndMessage() << "\n";
        int bytesSent = sendMessage(browser, bytesReceived);
    }
    // std::cerr << "END RESPONSE\n";
    return true;
}
