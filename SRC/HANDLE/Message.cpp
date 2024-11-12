#include "./../../HEADER/Message.h"

Message::Message() {
    buffer = new char[bufferSize]();
}

Message::Message(const char* MSG) {
    buffer = new char[bufferSize]();
    strcpy(buffer, MSG); 
}

Message::~Message() {
    delete[] buffer;
}

bool Message::getHostFromRequest(std::string &hostname, int &port) {
    string request(buffer);
    port = (request.find("CONNECT") == 0 ? HTTPS_PORT : HTTP_PORT);
    size_t hostPos = request.find("Host: ");
    if (hostPos == std::string::npos) {
        std::cerr << "Host header not found in request." << std::endl;
        return false;
    }

    std::string domain = request.substr(hostPos + 6);
    size_t endPos = domain.find('\n');
    if (endPos != std::string::npos) {
        domain = domain.substr(0, endPos);
    }

    if (port == HTTP_PORT) {
        hostname = domain;
        while (!hostname.empty() && !std::isalpha(hostname.back())) {
            hostname.pop_back();
        }
        if (hostname.empty()) {
            std::cerr << "Host not found in request." << std::endl;
            return false;
        }
        return true;
    }

    size_t colonPos = domain.find(':');
    if (colonPos != std::string::npos) {
        hostname = domain.substr(0, colonPos);
        try {
            port = std::stoi(domain.substr(colonPos + 1));
        } catch (const std::invalid_argument &e) {
            std::cerr << "Invalid port number." << std::endl;
            return false;
        } catch (const std::out_of_range &e) {
            std::cerr << "Port number out of range." << std::endl;
            return false;
        }
    } else {
        hostname = domain;
    }

    if (hostname.empty()) {
        std::cerr << "Host not found in request." << std::endl;
        return false;
    }
    return true;
}

int Message::receiveMessage(SOCKET socket, int byteReceive) {
    int bytesReceived = recv(socket, buffer, byteReceive, 0);
    if (bytesReceived == SOCKET_ERROR) std::cerr << "Error receiving data: " << WSAGetLastError() << std::endl;
    if (bytesReceived == 0) std::cerr << "Connection closed by peer." << std::endl;
    return bytesReceived;
}

int Message::sendMessage(SOCKET socket, int byteSent) {
    int bytesSent = send(socket, buffer, byteSent, 0);
    if (bytesSent == SOCKET_ERROR) std::cerr << "Error sending data: " << WSAGetLastError() << std::endl;
    return bytesSent;
}

char* Message::get() {
    return buffer;
}

void Message::print() {
    std::cerr << buffer;
}