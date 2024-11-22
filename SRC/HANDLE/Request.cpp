#include "./../../HEADER/Request.h"
#include <vector>
#include <sstream>
#include <string>
#include <unordered_map>

RequestHandler::RequestHandler(SocketHandler* _socketHandler){
    socketHandler = _socketHandler;
    HttpHandler();
}

bool RequestHandler::handleRequest() {
    char buffer[10000];
    while (isEndMessage() == false) {
        int bytesReceived = receiveMessage(browser);
        if (bytesReceived <= 0) break;
        int bytesSent = sendMessage(server, bytesReceived);
    }
    printHeader();
    return true;
}


void RequestHandler::modifyRequest(std::string& requestData) {
    std::vector<std::string> lines;
    std::istringstream stream(requestData);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    // Kiểm tra request có hợp lệ
    if (lines.empty() || lines[0].find("GET") != 0 && lines[0].find("POST") != 0 &&
        lines[0].find("PUT") != 0 && lines[0].find("DELETE") != 0) {
        std::cerr << "Invalid HTTP request format.\n";
        std::cerr << requestData;
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
    // headers["Accept-Encoding"] = "gzip, deflate"; // Chỉ hỗ trợ định dạng nén đơn giản
    // headers.erase("Proxy-Connection"); // Xóa nếu có Proxy-Connection (không cần thiết cho server)

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


// bool RequestHandler::handleRequest() {
//     char buffer[BUFFER_SIZE];
//     int bytesReceived = 0;
//     Protocol protocol = socketHandler->protocol;
//     if (protocol == HTTP) {
//         bytesReceived = recv(socketHandler->socketID[browser], buffer, BUFFER_SIZE, 0);
//     } else if (protocol == HTTPS) {
//         bytesReceived = SSL_read(socketHandler->sslID[browser], buffer, BUFFER_SIZE);
//     }
//     if (bytesReceived > 0) {
//         std::cerr << "bytesRequest: " << bytesReceived << '\n';
//         if (protocol == HTTP) {
//             send(socketHandler->socketID[server], buffer, bytesReceived, 0);
//         } else if (protocol == HTTPS) {
//             std::string requestData(buffer, bytesReceived);
//             std::cerr << "REQUEST DATA:\n" << requestData << '\n';
//             // modifyRequest(requestData); // Hàm chỉnh sửa request 
//             // std::cerr << "REQUEST DATA MODIFIRED:\n";
//             // std::cerr << requestData << '\n';
//             std::cerr << "NUMBYTE SENT: " << SSL_write(socketHandler->sslID[server], requestData.c_str(), requestData.size()) << "\n";
//         }
//         return true;
//     } 
//     return false;
// }