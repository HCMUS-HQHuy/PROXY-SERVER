#include "./../../HEADER/HttpHandler.hpp"

void debugerString(const string &name, const string &buffer) {
    std::cerr << "\n-----------------------------------------------\n";
    std::cerr << name << " -> START BUFFFER \n";
    std::cerr << "-----------------------------------------------\n";
    std::cerr << buffer << '\n';
    std::cerr << "-----------------------------------------------\n";
    std::cerr << name << " -> END BUFFER\n";
    std::cerr << "\n-----------------------------------------------\n";
}

HttpHandler::HttpHandler() { 
    buffer = new char[BUFFER_SIZE]();
    flagEndMessage = false; contentLength = -1;
    curChunkID = chunkEnd = 0;
    isChunked = headersParsed = false;
    header.clear(); body.clear();
    STEP = 0;
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
    if (bytesSent == 0) {
        std::cerr << "CONNECTION CLOSED!\n";
        onFlagEnd();
    }
    return bytesSent;
}

int HttpHandler::receiveMessage(Socket id, int size) {
    STEP++;
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
        onFlagEnd();
        return bytesReceived;
    }
    if (protocol == HTTPS) handleMessage(bytesReceived);
    else onFlagEnd();
    return bytesReceived;
}

void HttpHandler::handleMessage(int bytesReceived) {
    // std::cerr << "HANDLER STEP: " << STEP << '\n';
    if (headersParsed == false) {
        header.append(buffer, bytesReceived);
        size_t headerEnd = header.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            headerEnd += 4;
            headersParsed = true; 

            body = header.substr(headerEnd);
            header = header.substr(0, headerEnd);

            auto findContentLength = [&]() {
                const std::string patterns[] = {"content-length: ", "Content-Length: ", "CONTENT-LENGTH: "};
                for (const auto& pattern : patterns) {
                    size_t pos = header.find(pattern);
                    if (pos != std::string::npos) {
                        return pos;
                    }
                }
                return std::string::npos; // Không tìm thấy
            };

            size_t contentLengthPos = findContentLength();
            if (contentLengthPos != std::string::npos) {
                int p = contentLengthPos + string("content-length: ").size();
                contentLength = 0;
                while (isdigit(header[p])) contentLength = contentLength * 10 + (header[p] - '0'), p++;
                if (sz(body) >= contentLength) onFlagEnd();
            } else { 
                if (header.find("Transfer-Encoding: chunked") != std::string::npos) {
                    isChunked = true;
                }
                else onFlagEnd();
            }
        }
    } else body.append(buffer, bytesReceived);

    if (isChunked) {
        while (true) {
            if (isEndChunk()) {
                size_t chunkSizeEnd = body.find("\r\n", curChunkID);
                if (chunkSizeEnd == std::string::npos) {
                    // std::cerr << "Chunk size missed data.\n";
                    return;
                } 

                std::string chunkSizeHex = body.substr(curChunkID, chunkSizeEnd - curChunkID);
                // std::cerr << "CHUNK HEX: " << chunkSizeHex << '\n';
                int chunkSize = std::stoi(chunkSizeHex, nullptr, 16);
                // std::cerr << "Chunk size (hex): " << chunkSizeHex << ", (int): " << chunkSize << "\n";
                // Xử lý chunk cuối cùng
                if (chunkSize == 0) {
                    onFlagEnd();
                    return;
                }
                chunkEnd = chunkSizeEnd + 2 + chunkSize;
            }
            if (body.size() < chunkEnd + 2) { // +2 cho "\r\n" sau chunk
                // std::cerr << "Chunk is not full, waitting...\n";
                return;
            }
            curChunkID = chunkEnd + 2;
        }
    }
    else {
        if (contentLength >= 0 && sz(body) >= contentLength) {
            std::cerr << "received: " << sz(body) << "/" << contentLength << '\n';
            onFlagEnd();
            return;
        }
    }
    // std::cerr << "HANDLER STEP END AFTER ADD INTO BODY: " << STEP << '\n';
}

bool HttpHandler::isEndMessage() {
    return flagEndMessage;
}

void HttpHandler::printHeader() {
    std::cerr << "HEADER\n";
    std::cerr << header << '\n';
}