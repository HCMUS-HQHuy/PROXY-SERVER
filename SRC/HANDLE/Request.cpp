#include "./../../HEADER/Request.h"
#include <vector>
#include <sstream>
#include <string>
#include <unordered_map>

RequestHandler::RequestHandler(SocketHandler* _socketHandler) {
    socketHandler = _socketHandler;
}

bool RequestHandler::handleRequest() {
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    int bytesReceived = 0;
    Protocol protocol = socketHandler->protocol;
    std::string requestBuffer;

    while (true) {
        if (protocol == HTTP) {
            bytesReceived = recv(socketHandler->browserSocket, buffer, BUFFER_SIZE, 0);
        } else if (protocol == HTTPS) {
            bytesReceived = SSL_read(socketHandler->browserSSL, buffer, BUFFER_SIZE);
        }

        if (bytesReceived > 0) {
            requestBuffer.append(buffer, bytesReceived);
            if (protocol == HTTP) {
                int bytesSent = send(socketHandler->remoteSocket, buffer, bytesReceived, 0);
                if (bytesSent <= 0) {
                    std::cerr << "Error sending data to server (HTTP)\n";
                    return false;
                }
                // std::cerr << "Chunk sent to server (HTTP): " << bytesSent << " bytes\n";
            } else if (protocol == HTTPS) {
                int bytesSent = SSL_write(socketHandler->remoteSSL, buffer, bytesReceived);
                if (bytesSent <= 0) {
                    std::cerr << "Error sending data to server (HTTPS)\n";
                    return false;
                }
                // std::cerr << "Chunk sent to server (HTTPS): " << bytesSent << " bytes\n";
            }
        } else if (bytesReceived == 0) {
            break;
        } else {
            std::cerr << "Error receiving data from browser\n";
            return false;
        }

        // Kiểm tra nếu HTTP request đã hoàn chỉnh
        if (protocol == HTTPS && requestBuffer.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }
    std::cerr << requestBuffer << '\n';
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
//         bytesReceived = recv(socketHandler->browserSocket, buffer, BUFFER_SIZE, 0);
//     } else if (protocol == HTTPS) {
//         bytesReceived = SSL_read(socketHandler->browserSSL, buffer, BUFFER_SIZE);
//     }
//     if (bytesReceived > 0) {
//         std::cerr << "bytesRequest: " << bytesReceived << '\n';
//         if (protocol == HTTP) {
//             send(socketHandler->remoteSocket, buffer, bytesReceived, 0);
//         } else if (protocol == HTTPS) {
//             std::string requestData(buffer, bytesReceived);
//             std::cerr << "REQUEST DATA:\n" << requestData << '\n';
//             // modifyRequest(requestData); // Hàm chỉnh sửa request 
//             // std::cerr << "REQUEST DATA MODIFIRED:\n";
//             // std::cerr << requestData << '\n';
//             std::cerr << "NUMBYTE SENT: " << SSL_write(socketHandler->remoteSSL, requestData.c_str(), requestData.size()) << "\n";
//         }
//         return true;
//     } 
//     return false;
// }