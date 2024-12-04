#include "./../../HEADER/Request.hpp"
#include <vector>
#include <sstream>
#include <string>
#include <unordered_map>

#include "./../../HEADER/Logger.hpp"

RequestHandler::RequestHandler(SocketHandler* _socketHandler){
    socketHandler = _socketHandler;
    HttpHandler();
}

RequestHandler::RequestHandler(){
    socketHandler = nullptr;
}

int sendLargeData(SOCKET sock, const char* data, const int dataSize) {
    int totalSent = 0;
    while (totalSent < dataSize) {
        int remaining = dataSize - totalSent;
        int bytesToSend = (remaining < BUFFER_SIZE) ? remaining : BUFFER_SIZE;
        int sent = send(sock, data + totalSent, bytesToSend, 0);
        if (sent == SOCKET_ERROR) {
            int errorCode = WSAGetLastError();
            std::cerr << "Error sending data: " << errorCode << std::endl;
            return SOCKET_ERROR;
        }
        totalSent += sent;
    }
    return totalSent; 
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

bool RequestHandler::receiveRequest(SOCKET sock) {
    while (ServerRunning && isEndMessage() == false) {
        receiveMessage(sock);
    }
    return true;
}

bool RequestHandler::sendRequest(SOCKET sock) {
    sendLargeData(sock, getHeader().c_str(), getHeader().size());
    sendLargeData(sock, getBody().c_str(), getBody().size());
    return true;
}

