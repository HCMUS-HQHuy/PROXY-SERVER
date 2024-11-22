#include "./../../HEADER/HttpHandler.h"

HttpHandler::HttpHandler() {
    header.clear(); body.clear();
    buffer = new char[BUFFER_SIZE]();
    flag = false; contentLength = -1;
    isChunked = headersParsed = false;
}

HttpHandler::~HttpHandler() {
    delete[] buffer;
}

int HttpHandler::sendMessage(Socket id, int sizeSending) {
    int bytesSent = 0;
    Protocol protocol = socketHandler->protocol;
    if (protocol == HTTP) {
        bytesSent = send(socketHandler->socketID[id], buffer, sizeSending, 0);
    } else if (protocol == HTTPS) {
        bytesSent = SSL_write(socketHandler->sslID[id], buffer, sizeSending);
    }
    if (bytesSent < 0) std::cerr << "SENDING ERRORS!\n";
    if (bytesSent == 0) std::cerr << "CONNECTION CLOSED!\n";
    return bytesSent;
}

int HttpHandler::receiveMessage(Socket id, int size) {
    int bytesReceived = 0; 
    Protocol protocol = socketHandler->protocol;
    if (protocol == HTTP) {
        bytesReceived = recv(socketHandler->socketID[id], buffer, size, 0);
    } else if (protocol == HTTPS) {
        bytesReceived = SSL_read(socketHandler->sslID[id], buffer, size);
    }
    if (bytesReceived < 0) {
        std::cerr << "RECEIVING ERRORS!\n";
        return bytesReceived;
    }
    if (bytesReceived == 0) {
        std::cerr << "CONNECTION CLOSED!\n";
        return bytesReceived;
    }
    handleMessage(bytesReceived);
    return bytesReceived;
}

void HttpHandler::handleMessage(int bytesReceived) {
    if (headersParsed == false) {
        header.append(buffer, bytesReceived);
        size_t headerEnd = header.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            headerEnd += 4;
            headersParsed = true;
            // Phân tích header để lấy Content-Length hoặc Transfer-Encoding
            body = header.substr(headerEnd);
            header = header.substr(0, headerEnd);
            this->printHeader();
            size_t contentLengthPos = header.find("content-length: ");
            if (contentLengthPos != std::string::npos) {
                int p = contentLengthPos + string("content-length: ").size();
                contentLength = 0;
                while (isdigit(header[p])) contentLength = contentLength * 10 + (header[p] - '0'), p++;
                std::cerr << header << "\n";
                std::cout << "CONTENT LENGTH: " << contentLength << '\n';
                
            }
            else {
                std::cout << "DON'T HAVE CONTENT LENGTH\n"; 
                flag = true;
            }
            if (header.find("Transfer-Encoding: chunked") != std::string::npos) {
                isChunked = true;
                flag = false;
            }
        }
    } else body.append(buffer, bytesReceived);

    if (isChunked) {
        
    }

    if (contentLength >= 0 && sz(body) >= contentLength) {
        std::cerr << "received: " << sz(body) << "/" << contentLength << '\n';
        flag = true;
        return;
    }
}

bool HttpHandler::isEndMessage() {
    return flag;
}

void HttpHandler::printHeader() {
    std::cerr << "HEADER\n";
    std::cerr << header << '\n';
}