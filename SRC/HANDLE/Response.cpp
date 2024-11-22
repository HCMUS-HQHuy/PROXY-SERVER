#include "./../../HEADER/Response.h"

ResponseHandler::ResponseHandler(SocketHandler* _socketHandler){
    socketHandler = _socketHandler;
}

bool ResponseHandler::handleResponse() {
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    Protocol protocol = socketHandler->protocol;
    int totalBytesReceived = 0;  // Theo dõi tổng số byte nhận được
    int contentLength = MAXINT;       // Dùng để theo dõi Content-Length (nếu có)
    bool isChunked = false;         // Đánh dấu nếu dữ liệu dùng chunked encoding
    bool headersParsed = false;    // Xác định xem header đã được xử lý chưa

    std::string header;

    while (true) {
        int bytesReceived = 0;
        memset(buffer, 0, sizeof(buffer));  // Xóa buffer trước khi đọc

        // Đọc dữ liệu từ server
        if (protocol == HTTP) {
            bytesReceived = recv(socketHandler->remoteSocket, buffer, BUFFER_SIZE, 0);
        } else if (protocol == HTTPS) {
            bytesReceived = SSL_read(socketHandler->remoteSSL, buffer, BUFFER_SIZE);
        }

        if (bytesReceived > 0) {
            if (!headersParsed) {
                header.append(buffer, bytesReceived);
                std::cout<< header << '\n';
                size_t headerEnd = header.find("\r\n\r\n");
                totalBytesReceived = (int)header.size() - (int)headerEnd - (int)std::string("\r\n\r\n").size();
                if (headerEnd != std::string::npos) {
                    headersParsed = true;
                    // Phân tích header để lấy Content-Length hoặc Transfer-Encoding
                    header = header.substr(0, headerEnd);
                    size_t contentLengthPos = header.find("content-length: ");
                    if (contentLengthPos != std::string::npos) {
                        int p = contentLengthPos + string("content-length: ").size();
                        contentLength = 0;
                        while (isdigit(header[p])) contentLength = contentLength * 10 + (header[p] - '0'), p++;
                        std::cerr << header << "\n";
                        std::cout << "CONTENT LENGTH: " << contentLength << '\n';
                        
                    }
                    else std::cout << "DON'T HAVE CONTENT LENGTH\n";
                    if (header.find("Transfer-Encoding: chunked") != std::string::npos) {
                        isChunked = true;
                    }
                }
            }
            else totalBytesReceived += bytesReceived;
            if (protocol == HTTP) {
                int bytesSent = send(socketHandler->browserSocket, buffer, bytesReceived, 0);
                if (bytesSent <= 0) {
                    std::cerr << "Error sending data to browser (HTTP)\n";
                    return false;
                }
                // std::cerr << "Chunk sent to browser (HTTP): " << bytesSent << " bytes\n";
            } else if (protocol == HTTPS) {
                int bytesSent = SSL_write(socketHandler->browserSSL, buffer, bytesReceived);
                if (bytesSent <= 0) {
                    std::cerr << "Error sending data to browser (HTTPS)\n";
                    return false;
                }
                // std::cerr << "Chunk sent to browser (HTTPS): " << bytesSent << " bytes\n";
            }

            // Kiểm tra nếu đã nhận đủ Content-Length (nếu có)
            if (totalBytesReceived >= contentLength) {
                std::cerr << "received: " << totalBytesReceived << "/" << contentLength << '\n';
                break;
            }

            // TODO: Thêm xử lý dữ liệu chunked nếu cần thiết
        } else if (bytesReceived == 0) {
            // Server đóng kết nối
            std::cerr << "Server closed the connection\n";
            break;
        } else {
            // Lỗi khi nhận dữ liệu
            std::cerr << "Error receiving data from server\n";
            return false;
        }

        // Giới hạn thời gian chờ (timeout)
        // Thêm logic timeout nếu không có dữ liệu mới trong khoảng thời gian cố định
    }

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