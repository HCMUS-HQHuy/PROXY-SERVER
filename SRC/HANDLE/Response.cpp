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



void ResponseHandler::modifyResponse(std::string& responseData) {
    
}
// bool ResponseHandler::handleResponse() {
//     char buffer[BUFFER_SIZE];
//     memset(buffer, 0, sizeof buffer);
//     int bytesReceived = 0;
//     Protocol protocol = socketHandler->protocol;
//     if (protocol == HTTP) {
//         bytesReceived = recv(socketHandler->remoteSocket, buffer, BUFFER_SIZE, 0);
//     } else if (socketHandler->protocol == HTTPS) {
//         bytesReceived = SSL_read(socketHandler->remoteSSL, buffer, BUFFER_SIZE);
//     }

//     if (bytesReceived > 0) {
//         std::cerr << "bytesResponse: " << bytesReceived << '\n';
//         if (protocol == HTTP) {
//             send(socketHandler->browserSocket, buffer, bytesReceived, 0);
//         } else if (protocol == HTTPS) {
//             std::cerr << "RESPONSE FROM SERVER: \n" << buffer << '\n';
//             SSL_write(socketHandler->browserSSL, buffer, bytesReceived);
//         }
//         return true;
//     }
//     return false;
// }