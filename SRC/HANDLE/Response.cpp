#include "./../../HEADER/Response.h"

ResponseHandler::ResponseHandler(SocketHandler* _socketHandler){
    socketHandler = _socketHandler;
}

bool ResponseHandler::handleResponse() {
    const int BUFFER_SIZE = 512;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof buffer);
    int bytesReceived = 0;
    Protocol protocol = socketHandler->protocol;
    if (protocol == HTTP) {
        bytesReceived = recv(socketHandler->remoteSocket, buffer, BUFFER_SIZE, 0);
    } else if (socketHandler->protocol == HTTPS) {
        bytesReceived = SSL_read(socketHandler->remoteSSL, buffer, BUFFER_SIZE);
    }

    if (bytesReceived > 0) {
        if (protocol == HTTP) {
            send(socketHandler->browserSocket, buffer, bytesReceived, 0);
        } else if (protocol == HTTPS) {
            // std::cerr << buffer << '\n';
            SSL_write(socketHandler->browserSSL, buffer, bytesReceived);
        }
        return true;
    }
    return false;
}

void ResponseHandler::modifyResponse(std::string& responseData) {
    
}