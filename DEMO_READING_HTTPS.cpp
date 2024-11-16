#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/applink.c>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509v3.h>
#include <openssl/rand.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

#define HTTPS_PORT 443
#define HTTP_PORT 80

bool getHostFromRequest(const std::string request, std::string &hostname, int &port) {
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

// Hàm tạo serial number ngẫu nhiên để tránh bị trùng lặp
ASN1_INTEGER* generateSerialNumber() {
    ASN1_INTEGER* serial = ASN1_INTEGER_new();
    if (!serial) {
        std::cerr << "Error creating serial number.\n";
        return nullptr;
    }

    // Sinh số ngẫu nhiên
    unsigned char randBytes[16];
    if (RAND_bytes(randBytes, sizeof(randBytes)) != 1) {
        std::cerr << "Error generating random serial number.\n";
        ASN1_INTEGER_free(serial);
        return nullptr;
    }

    // Đặt số ngẫu nhiên vào serial number
    BIGNUM* bn = BN_bin2bn(randBytes, sizeof(randBytes), nullptr);
    BN_to_ASN1_INTEGER(bn, serial);
    BN_free(bn);
    return serial;
}

std::vector<std::string> generateSANs(const std::string& host) {
    std::vector<std::string> sanList;

    // Thêm host chính
    sanList.push_back(host);

    // Nếu host bắt đầu bằng "www.", thêm phiên bản không "www."
    if (host.find("www.") == 0) {
        sanList.push_back(host.substr(4)); // Loại bỏ "www."
    } else {
        // Nếu host không bắt đầu bằng "www.", thêm phiên bản có "www."
        sanList.push_back("www." + host);
    }

    // Thêm các subdomain phổ biến
    const std::vector<std::string> commonSubdomains = {"api", "cdn", "mail"};
    for (const auto& subdomain : commonSubdomains) {
        size_t pos = host.find(".");
        if (pos != std::string::npos) {
            std::string baseDomain = host.substr(pos + 1);
            sanList.push_back(subdomain + "." + baseDomain);
        }
    }

    return sanList;
}

X509_EXTENSION* addSAN(const std::string& host) {
    std::vector<std::string> sanList = generateSANs(host);
    std::string sanEntry = "";
    for (size_t i = 0; i < sanList.size(); ++i) {
        if (i > 0) {
            sanEntry += ", ";
        }
        sanEntry += "DNS:" + sanList[i];
    }
    std::cerr << sanEntry << '\n';
    STACK_OF(CONF_VALUE)* sk = nullptr;
    CONF_VALUE* value = nullptr;

    sk = sk_CONF_VALUE_new_null();
    value = (CONF_VALUE*)OPENSSL_malloc(sizeof(CONF_VALUE));
    value->name = nullptr;
    value->value = strdup(sanEntry.c_str());
    sk_CONF_VALUE_push(sk, value);

    X509_EXTENSION* ext = X509V3_EXT_conf_nid(nullptr, nullptr, NID_subject_alt_name, sanEntry.c_str());
    return ext;
}

bool generateCertificate(const std::string& host,
                         const std::string& outputCertPath,
                         const std::string& outputKeyPath,
                         const std::string& rootKeyPath,
                         const std::string& rootCertPath) {
    const int keyBits = 2048; // RSA key length
    const int validityDays = 365; // Certificate validity in days

    // Generate RSA private key
    RSA* rsaKey = RSA_generate_key(keyBits, RSA_F4, nullptr, nullptr);
    EVP_PKEY* privateKey = EVP_PKEY_new();
    EVP_PKEY_assign_RSA(privateKey, rsaKey);

    // Create a new certificate
    X509* cert = X509_new();
    X509_set_version(cert, 2);
    ASN1_INTEGER* serial = generateSerialNumber();
    if (!serial) {
        std::cerr << "Error generating serial number.\n";
        return false;
    }
    X509_set_serialNumber(cert, serial);
    ASN1_INTEGER_free(serial);
    
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), validityDays * 24 * 60 * 60);

    // Set certificate subject name
    X509_NAME* name = X509_get_subject_name(cert);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (const unsigned char*)host.c_str(), -1, -1, 0);
    X509_set_subject_name(cert, name);

    // Set public key
    X509_set_pubkey(cert, privateKey);

    // Add SAN extension
    X509_EXTENSION* sanExtension = addSAN(host);
    X509_add_ext(cert, sanExtension, -1);

    // Load root key and root certificate
    FILE* rootKeyFile = fopen(rootKeyPath.c_str(), "r");
    FILE* rootCertFile = fopen(rootCertPath.c_str(), "r");
    if (!rootKeyFile || !rootCertFile) {
        std::cerr << "Cannot open root key or root certificate files.\n";
        return false;
    }
    EVP_PKEY* rootKey = PEM_read_PrivateKey(rootKeyFile, nullptr, nullptr, nullptr);
    X509* rootCert = PEM_read_X509(rootCertFile, nullptr, nullptr, nullptr);
    fclose(rootKeyFile);
    fclose(rootCertFile);

    if (!rootKey || !rootCert) {
        std::cerr << "Error loading root key or root certificate.\n";
        return false;
    }

    // Set issuer name to root certificate's subject name
    X509_set_issuer_name(cert, X509_get_subject_name(rootCert));

    // Sign the certificate using the root key
    if (!X509_sign(cert, rootKey, EVP_sha256())) {
        std::cerr << "Error signing certificate.\n";
        return false;
    }

    // Save the generated certificate and private key
    FILE* certFile = fopen(outputCertPath.c_str(), "w");
    FILE* keyFile = fopen(outputKeyPath.c_str(), "w");
    if (!certFile || !keyFile) {
        std::cerr << "Cannot open output files for writing.\n";
        return false;
    }
    PEM_write_X509(certFile, cert);
    PEM_write_PrivateKey(keyFile, privateKey, nullptr, nullptr, 0, nullptr, nullptr);
    fclose(certFile);
    fclose(keyFile);

    // Free resources
    EVP_PKEY_free(privateKey);
    X509_free(cert);
    EVP_PKEY_free(rootKey);
    X509_free(rootCert);

    std::cout << "Certificate generated successfully for " << host << ".\n";
    return true;
}

void clientHandler(SOCKET);
void HTTPHandler(SOCKET, char*, int);
void HTTPSHandler(SOCKET, char*, int);
void relayData(SSL* clientSSL, SSL* serverSSL);

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    // Set up server socket
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(listenSocket, SOMAXCONN);

    std::cout << "MITM Proxy listening on port 8080..." << std::endl;

    while (true) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Failed to accept client connection." << std::endl;
            continue;
        }
        clientHandler(clientSocket);
    }

    WSACleanup();
    return 0;
}

void clientHandler(SOCKET clientSocket) {
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        closesocket(clientSocket);
        return;
    }
    buffer[bytesReceived] = '\0';

    std::string request(buffer, bytesReceived);

    std::string host; int port; getHostFromRequest(request, host, port);
    hostent* hostInfo = gethostbyname(host.c_str());
    // Resolve the host name to an IP address
    if (hostInfo == nullptr) {
        std::cerr << "Failed to resolve host name." << std::endl;
        closesocket(clientSocket);
        return;
    }
    // Kiểm tra nếu là yêu cầu CONNECT (HTTPS)
    if (port == HTTPS_PORT) {
        HTTPSHandler(clientSocket, buffer, bytesReceived);
    } else {
        HTTPHandler(clientSocket, buffer, bytesReceived);
    }
}

void HTTPHandler(SOCKET clientSocket, char* buffer, int bytesReceived) {
    // Xử lý yêu cầu HTTP không mã hóa

    // Parse và kết nối đến server đích cho HTTP
    std::string request(buffer, bytesReceived);
    std::string host; int port; getHostFromRequest(request, host, port);
    hostent* hostInfo = gethostbyname(host.c_str());
    SOCKET remoteSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in targetAddr;
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port = htons(80);
    targetAddr.sin_addr.s_addr = *((unsigned long*)hostInfo->h_addr_list[0]);

    if (connect(remoteSocket, (sockaddr*)&targetAddr, sizeof(targetAddr)) == SOCKET_ERROR) {
        closesocket(remoteSocket);
        closesocket(clientSocket);
        return;
    }

    fd_set readfds;
    while (true) {
        const int bufferSize = 512;
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);
        FD_SET(remoteSocket, &readfds);
        TIMEVAL delay; delay.tv_sec = 2; delay.tv_usec=0;
        int activity = select(0, &readfds, nullptr, nullptr, &delay);
        if (activity <= 0) break;
        if (FD_ISSET(clientSocket, &readfds)) {
            int bytesReceived = bufferSize;
            if (recv(clientSocket, buffer, bytesReceived, 0) <= 0) break;
            if (send(remoteSocket, buffer, bytesReceived, 0) <= 0) break;
        }
        if (FD_ISSET(remoteSocket, &readfds)) { 
            int bytesReceived = bufferSize;
            if (recv(remoteSocket, buffer, bytesReceived, 0) <= 0) break; 
            if (send(clientSocket, buffer, bytesReceived, 0) <= 0) break;
        }
    }
    closesocket(remoteSocket);
    closesocket(clientSocket);
}

SSL_CTX* createSSLContext(const char* certFile, const char* keyFile) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!SSL_CTX_use_certificate_file(ctx, certFile, SSL_FILETYPE_PEM) ||
        !SSL_CTX_use_PrivateKey_file(ctx, keyFile, SSL_FILETYPE_PEM)) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return nullptr;
    }
    return ctx;
}

// Kết nối HTTPS đến server đích qua OpenSSL
SSL* connectToTargetServer(const std::string& host, int port, SOCKET serverSocket) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        std::cerr << "CANNOT create SSL for server.\n";
        return nullptr;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        SSL_CTX_free(ctx); // Đảm bảo giải phóng ctx khi gặp lỗi
        return nullptr;
    }
    // Địa chỉ server
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    hostent* hostInfo = gethostbyname(host.c_str());
    memcpy(&serverAddr.sin_addr, hostInfo->h_addr_list[0], hostInfo->h_length);

    if (connect(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "CANNOT connect to server.\n";
        closesocket(serverSocket);
        return nullptr;
    }
    std::cerr << "CONNECTED SERVER: " << host << " " << port << '\n';

    SSL* serverSSL = SSL_new(ctx);
    SSL_set_fd(serverSSL, serverSocket);

    if (SSL_connect(serverSSL) <= 0) {
        int err = SSL_get_error(serverSSL, -1);
        std::cerr << "SSL_connect failed with error code: " << err << '\n';
        ERR_print_errors_fp(stderr);
        closesocket(serverSocket);
        SSL_free(serverSSL);
        SSL_CTX_free(ctx);
        return nullptr;
    }
    
    return serverSSL;
}

void HTTPSHandler(SOCKET clientSocket, char* buffer, int bytesReceived) {
    std::string request(buffer);
    std::string host; int port; getHostFromRequest(request, host, port);
    std::cerr << host << " " << port << '\n';

    const char* response = "HTTP/1.1 200 Connection Established\r\n\r\n";
    int byteSent = send(clientSocket, response, strlen(response), 0);

    const std::string rootKeyPath = "./CERTIFICATE/root.key";
    const std::string rootCertPath = "./CERTIFICATE/root.crt";
    const std::string outputKeyPath = "./CERTIFICATE/generated.key";
    const std::string outputCertPath = "./CERTIFICATE/generated.crt";
    
    if (!generateCertificate(host, outputCertPath, outputKeyPath, rootKeyPath, rootCertPath)) {
        std::cerr << "Failed to generate certificate.\n";
        return;
    }

    std::cout << "Certificate and key saved to: " << outputCertPath << ", " << outputKeyPath << "\n";
    

    SSL_CTX* clientCtx = createSSLContext(outputCertPath.c_str(), outputKeyPath.c_str());
    if (!clientCtx) return;

    SSL* clientSSL = SSL_new(clientCtx);
    SSL_set_fd(clientSSL, clientSocket);

    if (SSL_get_verify_result(clientSSL) != X509_V_OK) {
        std::cerr << "SSL certificate verification failed\n";
        return;
    }

    if (SSL_accept(clientSSL) <= 0) {
        std::cerr << "CANNOT ACCEPT clientSSL\n";
        ERR_print_errors_fp(stderr);
        SSL_free(clientSSL);
        SSL_CTX_free(clientCtx);
        closesocket(clientSocket);
        return;
    }

    // Kết nối đến server thực qua HTTPS
    SOCKET remoteSocket;
    SSL* serverSSL = connectToTargetServer(host, port, remoteSocket);
    if (!serverSSL) {
        std::cerr << "Cannot connect to server.\n";
        SSL_shutdown(clientSSL);
        SSL_free(clientSSL);
        SSL_CTX_free(clientCtx);
        closesocket(clientSocket);
        return;
    }

    std::cout << "SSL connection established with client." << std::endl;
    relayData(clientSSL, serverSSL);

    SSL_shutdown(clientSSL);
    SSL_free(clientSSL);
    closesocket(clientSocket);
    closesocket(remoteSocket);
}

// Hàm tách request thành các dòng
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

// Hàm sửa đổi HTTP request
std::string modifyHTTPRequest(const std::string& request) {
    auto lines = splitLines(request);

    // Kiểm tra request có hợp lệ
    if (lines.empty() || lines[0].find("GET") != 0 && lines[0].find("POST") != 0 &&
        lines[0].find("PUT") != 0 && lines[0].find("DELETE") != 0) {
        std::cerr << "Invalid HTTP request format.\n";
        return "";
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

    return modifiedRequest.str();
}


void relayData(SSL* clientSSL, SSL* serverSSL) {
    std::cerr << "HELLO\n\n\n";
    fd_set readfds;
    const int bufferSize = 512;
    char buffer[bufferSize];
    int step = 0;
    while (true) {
        std::cerr << "STEP: " << ++step << '\n';
        FD_ZERO(&readfds);
        int clientFd = SSL_get_fd(clientSSL);
        int serverFd = SSL_get_fd(serverSSL);

        FD_SET(clientFd, &readfds);
        FD_SET(serverFd, &readfds);

        TIMEVAL timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        int maxFd = std::max(clientFd, serverFd) + 1; // Lấy giá trị lớn nhất của các file descriptors
        int activity = select(maxFd, &readfds, nullptr, nullptr, &timeout);

        if (activity == -1) {
            int errorCode = WSAGetLastError();
            std::cerr << "Error in select(): " << errorCode << ". Exiting...\n";
            break;
        }
        if (activity == 0) {
            break;
        }
        // Xử lý dữ liệu từ client đến server
        if (FD_ISSET(clientFd, &readfds)) {
            std::cerr << "IN CLIENT\n";
            int bytesReceived = SSL_read(clientSSL, buffer, bufferSize);
            if (bytesReceived <= 0) {
                std::cerr << "Client disconnected or error reading from client.\n";
                break;
            }
            // std::cerr << "Data from client: \n" << std::string(buffer, bytesReceived) << '\n';

            std::string tmp = std::string(buffer, bytesReceived);
            tmp = modifyHTTPRequest(tmp);
            std::cerr << tmp << '\n';
            int bytesSent = SSL_write(serverSSL, tmp.c_str(), tmp.size());
            if (bytesSent <= 0) {
                std::cerr << "Error writing to server.\n";
                break;
            }
        }

        // Xử lý dữ liệu từ server đến client
        if (FD_ISSET(serverFd, &readfds)) {
            std::cerr << "IN SERVER\n";
            int bytesReceived = SSL_read(serverSSL, buffer, bufferSize);
            if (bytesReceived <= 0) {
                std::cerr << "Server disconnected or error reading from server.\n";
                break;
            }
            std::cerr << "Data from server: \n" << std::string(buffer, bytesReceived) << '\n';
            std::cerr << "SENT\n";
            int bytesSent = SSL_write(clientSSL, buffer, bytesReceived);
            if (bytesSent <= 0) {
                std::cerr << "Error writing to client.\n";
                break;
            }
        }
    }
}