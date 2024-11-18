#include "./../../HEADER/SocketHandler.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/rand.h>
#include <openssl/applink.c>

const int HTTP_PORT = 80;
const int HTTPS_PORT = 443;

bool parseHostAndPort(std::string request, std::string& hostname, int& port) {

    // Xác định giao thức dựa trên dòng đầu tiên
    port = (request.find("CONNECT") == 0 ? HTTPS_PORT : HTTP_PORT);

    // Tìm vị trí của header "Host: "
    size_t hostPos = request.find("Host: ");
    if (hostPos == std::string::npos) {
        std::cerr << "Host header not found in request." << std::endl;
        return false;
    }

    // Lấy nội dung sau "Host: "
    size_t hostStart = hostPos + 6;  // Độ dài của "Host: "
    size_t hostEnd = request.find('\r', hostStart);
    if (hostEnd == std::string::npos) {
        hostEnd = request.find('\n', hostStart); // Trong trường hợp không có '\r'
    }
    if (hostEnd == std::string::npos) {
        std::cerr << "Host header is malformed." << std::endl;
        return false;
    }

    std::string domain = request.substr(hostStart, hostEnd - hostStart);
    domain.erase(std::remove_if(domain.begin(), domain.end(), ::isspace), domain.end()); // Xóa khoảng trắng

    // Kiểm tra xem có chứa port hay không
    size_t colonPos = domain.find(':');
    if (colonPos != std::string::npos) {
        // Lấy hostname và port từ domain
        hostname = domain.substr(0, colonPos);
        try {
            port = std::stoi(domain.substr(colonPos + 1));
        } catch (const std::invalid_argument&) {
            std::cerr << "Invalid port number." << std::endl;
            return false;
        } catch (const std::out_of_range&) {
            std::cerr << "Port number out of range." << std::endl;
            return false;
        }
    } else {
        // Không có port, sử dụng port mặc định
        hostname = domain;
    }

    // Loại bỏ các ký tự không hợp lệ ở cuối hostname (nếu có)
    while (!hostname.empty() && !std::isalnum(hostname.back())) {
        hostname.pop_back();
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
    // const std::vector<std::string> commonSubdomains = {"api", "cdn", "mail"};
    // for (const auto& subdomain : commonSubdomains) {
    //     size_t pos = host.find(".");
    //     if (pos != std::string::npos) {
    //         std::string baseDomain = host.substr(pos + 1);
    //         sanList.push_back(subdomain + "." + baseDomain);
    //     }
    // }

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

SocketHandler::SocketHandler(SOCKET browser) {
    browserSocket = browser;
    remoteSocket = connectToServer();
    remoteSSL = browserSSL = nullptr;
}

SocketHandler::~SocketHandler() {
    if (protocol == HTTPS) {
        SSL_CTX_free(remoteCtx);
        SSL_CTX_free(browserCtx);
        if (remoteSSL) SSL_free(remoteSSL);
        if (browserSSL) SSL_free(browserSSL);
    }
    closesocket(browserSocket);
    closesocket(remoteSocket);
}

bool SocketHandler::setSSLContexts() {
    if (protocol == HTTP) return true;
    return setSSLbrowser() && setSSLserver();
}

bool SocketHandler::setSSLbrowser() {
    const std::string rootKeyPath = "./CERTIFICATE/root.key";
    const std::string rootCertPath = "./CERTIFICATE/root.crt";
    const std::string outputKeyPath = "./CERTIFICATE/generated.key";
    const std::string outputCertPath = "./CERTIFICATE/generated.crt";
    if (!generateCertificate(host, outputCertPath, outputKeyPath, rootKeyPath, rootCertPath)) {
        std::cerr << "Failed to generate certificate.\n";
        return false;
    }
    std::cout << "Certificate and key saved to: " << outputCertPath << ", " << outputKeyPath << "\n";
    browserCtx = createSSLContext(outputCertPath.c_str(), outputKeyPath.c_str());
    if (!browserCtx) return false;

    browserSSL = SSL_new(browserCtx);
    SSL_set_fd(browserSSL, browserSocket);

    if (SSL_get_verify_result(browserSSL) != X509_V_OK) {
        std::cerr << "SSL certificate verification failed\n";
        return false;
    }

    if (SSL_accept(browserSSL) <= 0) {
        std::cerr << "CANNOT ACCEPT clientSSL\n";
        ERR_print_errors_fp(stderr);
        return false;
    }
    return true;
}

bool SocketHandler::setSSLserver() {
    remoteCtx = SSL_CTX_new(TLS_client_method());
    if (!remoteCtx) {
        std::cerr << "CANNOT create SSL for server.\n";
        return false;
    }
    
    remoteSSL = SSL_new(remoteCtx);
    SSL_set_fd(remoteSSL, remoteSocket);
    if (SSL_connect(remoteSSL) <= 0) {
        int err = SSL_get_error(remoteSSL, -1);
        std::cerr << "SSL_connect failed with error code: " << err << '\n';
        ERR_print_errors_fp(stderr);
        return false;
    }
    return true;
}

bool SocketHandler::isValid() {
    return remoteSocket != INVALID_SOCKET && remoteSocket != SOCKET_ERROR
        && browserSocket != INVALID_SOCKET && browserSocket != SOCKET_ERROR;
}

SOCKET SocketHandler::connectToServer() {
    // Message message; message.receiveMessage(clientSocket);
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE]; int bytesRecv = recv(browserSocket, buffer, BUFFER_SIZE, 0);

    int port; 
    parseHostAndPort(std::string(buffer, bytesRecv), host, port);
    hostent* hostInfo = gethostbyname(host.c_str());
    if (hostInfo == nullptr) {
        std::cerr << "Failed to resolve host name." << std::endl;
        return SOCKET_ERROR;
    }
    // Create a socket to listen for client connections
    SOCKET remoteSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (remoteSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket." << std::endl;
        return SOCKET_ERROR;
    }

    // Set the remote server address
    sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(port); 
    remoteAddr.sin_addr.s_addr = *((unsigned long*)hostInfo->h_addr_list[0]);

    // Connect to the remote server
    if (connect(remoteSocket, (sockaddr*)&remoteAddr, sizeof(remoteAddr)) == SOCKET_ERROR) {
        std::cerr << "Error connecting to remote server." << std::endl;
        closesocket(remoteSocket);
        return SOCKET_ERROR;
    }

    if (port == HTTPS_PORT) {
        const char* response = "HTTP/1.1 200 Connection Established\r\n\r\n";
        int byteSent = send(browserSocket, response, strlen(response), 0);
        if (byteSent != strlen(response)) {
            closesocket(remoteSocket);
            return SOCKET_ERROR;
        }
        protocol = HTTPS;
    }
    return remoteSocket;
}