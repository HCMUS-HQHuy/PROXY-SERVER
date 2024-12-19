#include "./../../HEADER/UI.hpp"
#include "./../../HEADER/HttpHandler.hpp"
#include "./../../HEADER/Logger.hpp"

#include <sstream>
#include <map>

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
    if (bytesSent < 0) logger.logError(-15);
    if (bytesSent == 0) logger.logError(-16);
    if (bytesSent <= 0) onFlagEnd();
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
    if (bytesReceived < 0) logger.logError(-17);
    if (bytesReceived == 0) logger.logError(-16);
    if (bytesReceived <= 0) {
        onFlagEnd();
        return bytesReceived;
    }

    handleMessage(bytesReceived);
    return bytesReceived;
}

int HttpHandler::receiveMessage(SOCKET sock, int size) {
    int bytesReceived = recv(sock, buffer, size, 0);
    if (bytesReceived < 0) logger.logError(-17);
    if (bytesReceived == 0) logger.logError(-16);
    if (bytesReceived <= 0) {
        onFlagEnd();
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
        while (ServerRunning) {
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
            // std::cerr << "received: " << sz(body) << "/" << contentLength << '\n';
            onFlagEnd();
            return;
        }
    }
    // std::cerr << "HANDLER STEP END AFTER ADD INTO BODY: " << STEP << '\n';
}

bool HttpHandler::isEndMessage() {
    return flagEndMessage;
}

string HttpHandler::getHeader() {
    return header;
}

string HttpHandler::getBody() {
    return body;
}

std::wstring convert(string &s) {
    if (s.empty()) return L"NA";
    return std::wstring(s.begin(), s.end());
}

// Hàm phân tích HTTP GET request
void HttpHandler::parseHttpHeader(SOCKET sock, bool isAllow) {
    std::string requestLine, headerLine, body;
    std::istringstream stream(header);

    // Biến lưu thông tin
    std::string method, uri, httpVersion, cookie, host;
    std::map<std::string, std::string> headers;

    // Đọc dòng đầu tiên (Request Line)
    std::getline(stream, requestLine);
    std::istringstream requestLineStream(requestLine);
    requestLineStream >> method >> uri >> httpVersion;
    if (method == "CONNECT") uri.clear();
    size_t p = uri.find(":");
    if (p != std::string::npos) {
        uri.erase(p);
    }

    // Đọc headers
    while (std::getline(stream, headerLine) && headerLine != "\r") {
        size_t colonPos = headerLine.find(":");
        if (colonPos != std::string::npos) {
            std::string headerName = headerLine.substr(0, colonPos);
            std::string headerValue = headerLine.substr(colonPos + 2); // Bỏ qua ": "
            headers[headerName] = headerValue;
        }
    }
    // Lưu riêng cookie (nếu có)
    if (headers.find("Cookie") != headers.end()) {
        cookie = headers["Cookie"];
    }
    if (headers.find("Host") != headers.end())
        host = headers["Host"];

    p = host.find(":");
    if (p != std::string::npos) {
        host.erase(p);
    }

    std::string source;

    sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);
    if (getpeername(sock, (sockaddr*)&clientAddr, &addrLen) == 0) {
        // Lấy địa chỉ IP của client
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        // Lấy cổng của client
        int clientPort = ntohs(clientAddr.sin_port);
        source = std::string(clientIP) + ":" + std::to_string(clientPort);
    }
    Window.AppendList((isAllow ? L"Allow" :L"Block"), convert(httpVersion), convert(method), convert(source), convert(host), convert(uri), convert(cookie));
}