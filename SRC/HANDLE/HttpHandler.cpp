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
    chunkEnd = 0;
    isChunked = headersParsed = false;
    header.clear(); chunkBuffer.clear(); body.clear();
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
    // std::cerr << "STEP: " << ++STEP << '\n';
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
            // debugerString("HEADER IN HANDLERMESAAGE", header);
            size_t contentLengthPos = header.find("content-length: ");
            if (contentLengthPos == std::string::npos) contentLengthPos=header.find("Content-Length: ");
            if (contentLengthPos == std::string::npos) contentLengthPos=header.find("CONTENT-LENGTH: ");
            if (contentLengthPos != std::string::npos) {
                int p = contentLengthPos + string("content-length: ").size();
                contentLength = 0;
                while (isdigit(header[p])) contentLength = contentLength * 10 + (header[p] - '0'), p++;
                // debugerString("BODY REMAIN: ", body);
                // std::cout << "CURRENT: " << sz(body) << "/" << contentLength << '\n';
                if (sz(body) >= contentLength) onFlagEnd();
            } else {
                // std::cout << "DON'T HAVE CONTENT LENGTH\n"; 
                if (header.find("Transfer-Encoding: chunked") != std::string::npos) {
                    isChunked = true;
                    chunkBuffer = body;
                }
                else onFlagEnd();
            }
        }
        if (size(chunkBuffer) == 0) return;
        // else debugerString("HEADER REMAIN IN CHUNK BUFFER", chunkBuffer);
    } 

    if (isChunked) {
        // std::cerr << "=> endHeader: " << headersParsed << "->" << sz(body) << '\n';
        if (body.empty()) { 
            chunkBuffer.append(buffer, bytesReceived); 
        }
        else body.clear();
        // debugerString("CHECK CHUNKBUFFER", chunkBuffer);
        while (true) {
            if (isEndChunk()) {
                size_t chunkSizeEnd = chunkBuffer.find("\r\n");
                if (chunkSizeEnd == std::string::npos) {
                    // std::cerr << "Chunk size missed data.\n";
                    return; // Chưa nhận đủ để đọc chunk size
                } 
                // else std::cerr << "full!!\n";
                // size_t chunkStart = chunkSizeEnd + 2; // Vị trí sau "\r\n"
    
                std::string chunkSizeHex = chunkBuffer.substr(0, chunkSizeEnd);
                // std::cerr << "CHUNK HEX: " << chunkSizeHex << '\n';
                int chunkSize = std::stoi(chunkSizeHex, nullptr, 16);
                // std::cerr << "Chunk size (hex): " << chunkSizeHex << ", (int): " << chunkSize << "\n";
                // Xử lý chunk cuối cùng
                if (chunkSize == 0) {
                    // size_t trailerStart = chunkSizeEnd + 2; // Vị trí sau "\r\n"
                    // size_t trailerEnd = chunkBuffer.find("\r\n\r\n", trailerStart);
                    // if (trailerEnd == std::string::npos) {
                    //     std::cerr << "Trailer chưa đủ, chờ thêm dữ liệu.\n";
                    //     return; // Chờ thêm dữ liệu cho trailer
                    // }

                    // Gọi hàm xử lý hoàn thành và xóa trailer khỏi buffer
                    onFlagEnd();
                    // std::cerr << "End chunked transfer (have trailer).\n";
                    // chunkBuffer.erase(0, trailerEnd + 4);
                    return;
                }
                chunkEnd = chunkSizeEnd + 2 + chunkSize;
            }
                // Kiểm tra đủ dữ liệu cho chunk
            if (chunkBuffer.size() < chunkEnd + 2) { // +2 cho "\r\n" sau chunk
                // std::cerr << "Chunk is not full, waitting...\n";
                return;
            }
            // std::string chunkData = chunkBuffer.substr(chunkStart, chunkSize);
            chunkBuffer.erase(0, chunkEnd + 2); // Xóa chunk + "\r\n"
            chunkEnd = 0;
        }
        return;
    }

    body.append(buffer, bytesReceived);
    // std::cerr << "body size: " << sz(body) << "/" << contentLength << '\n';
    if (contentLength >= 0 && sz(body) >= contentLength) {
        std::cerr << "received: " << sz(body) << "/" << contentLength << '\n';
        onFlagEnd();
        return;
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