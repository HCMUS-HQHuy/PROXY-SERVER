#include "./../../HEADER/Request.h"
#include <vector>
#include <sstream>
#include <string>
#include <unordered_map>

RequestHandler::RequestHandler(SocketHandler* _socketHandler) {
    socketHandler = _socketHandler;
}

bool RequestHandler::handleRequest() {
    const int BUFFER_SIZE = 512;
    char buffer[BUFFER_SIZE];
    int bytesReceived = 0;
    Protocol protocol = socketHandler->protocol;

    if (protocol == HTTP) {
        bytesReceived = recv(socketHandler->browserSocket, buffer, BUFFER_SIZE, 0);
    } else if (protocol == HTTPS) {
        bytesReceived = SSL_read(socketHandler->browserSSL, buffer, BUFFER_SIZE);
    }

    if (bytesReceived > 0) {
        if (protocol == HTTP) {
            send(socketHandler->remoteSocket, buffer, bytesReceived, 0);
        } else if (protocol == HTTPS) {
            std::string requestData(buffer, bytesReceived);
            modifyRequest(requestData); // Hàm chỉnh sửa request
            SSL_write(socketHandler->remoteSSL, requestData.c_str(), requestData.size());
        }
        return true;
    } 
    return false;
}

std::vector<std::string> splitLines(const std::string& request) {
    std::vector<std::string> lines;
    std::istringstream stream(request);
    std::string line;
    while (std::getline(stream, line)) {
        // Loại bỏ ký tự xuống dòng cuối cùng nếu có
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

void RequestHandler::modifyRequest(std::string& requestData) {
    auto lines = splitLines(requestData);
    // Kiểm tra request có hợp lệ
    if (lines.empty() || lines[0].find("GET") != 0 && lines[0].find("POST") != 0 &&
        lines[0].find("PUT") != 0 && lines[0].find("DELETE") != 0) {
        std::cerr << "Invalid HTTP request format.\n";
        requestData = "";
        return;
    }

    // Request line: "GET /path HTTP/1.1"
    std::string requestLine = lines[0];
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    bool isBody = false;

    // Tách headers và body
    for (size_t i = 1; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        if (line.empty()) {
            isBody = true;
            continue;
        }
        if (isBody) {
            body += line + "\r\n";
        } else {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string key = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);
                while (!value.empty() && value.front() == ' ') value.erase(value.begin());
                headers[key] = value;
            }
        }
    }

    // Bắt buộc chỉnh sửa một số headers
    headers["Connection"] = "close"; // Đóng kết nối sau request
    headers["Accept-Encoding"] = "gzip, deflate"; // Chỉ hỗ trợ định dạng nén đơn giản
    headers.erase("Proxy-Connection"); // Xóa nếu có Proxy-Connection (không cần thiết cho server)

    // Build lại request
    std::ostringstream modifiedRequest;
    modifiedRequest << requestLine << "\r\n";
    for (const auto& header : headers) {
        modifiedRequest << header.first << ": " << header.second << "\r\n";
    }
    modifiedRequest << "\r\n"; // Dòng trống phân cách header và body
    modifiedRequest << body;

    requestData = modifiedRequest.str();
    return;
}