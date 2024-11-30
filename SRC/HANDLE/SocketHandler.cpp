#include "./../../HEADER/SocketHandler.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/rand.h>
#include <openssl/applink.c>

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

X509_EXTENSION* addSAN(const std::string& host) {
    std::vector<std::string> sanList;
    sanList.push_back(host);
    // Nếu host bắt đầu bằng "www.", thêm phiên bản không "www."
    if (host.find("www.") == 0) {
        sanList.push_back(host.substr(4)); // Loại bỏ "www."
    } else {
        // Nếu host không bắt đầu bằng "www.", thêm phiên bản có "www."
        sanList.push_back("www." + host);
    }

    std::string sanEntry = "";
    for (size_t i = 0; i < sanList.size(); ++i) {
        if (i > 0) {
            sanEntry += ", ";
        }
        sanEntry += "DNS:" + sanList[i];
    }
    // std::cerr << sanEntry << '\n';
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
    if (fopen(outputCertPath.c_str(), "r")) return true;
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

    // std::cout << "Certificate generated successfully for " << host << ".\n";
    return true;
}

SSL_CTX* createSSLContext(const char* certFile, const char* keyFile) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_SERVER);
    if (!SSL_CTX_use_certificate_file(ctx, certFile, SSL_FILETYPE_PEM) ||
        !SSL_CTX_use_PrivateKey_file(ctx, keyFile, SSL_FILETYPE_PEM)) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return nullptr;
    }
    return ctx;
}

SocketHandler::SocketHandler(SOCKET fromBrowser, SOCKET fromRemote, bool isHTTPS) {
    sslID[browser] = sslID[server] = nullptr;
    ctxID[browser] = ctxID[server] = nullptr; // Cần giải thích vì sao nêu không có cái này thì việc giải phóng gặp lỗi.
    socketID[browser] = fromBrowser;
    socketID[server] = fromRemote;
    protocol = (isHTTPS ? HTTPS : HTTP);
}

SocketHandler::~SocketHandler() {
    for (int i: {0, 1}) {
        if (protocol == HTTPS) {
            SSL_shutdown(sslID[i]);
            SSL_CTX_free(ctxID[i]);
            SSL_free(sslID[i]);
            sslID[i] = nullptr; 
            ctxID[i] = nullptr;
        }
        closesocket(socketID[i]); socketID[i] = -1;
    }
}

bool SocketHandler::setSSLContexts(const std::string &host) {
    if (protocol == HTTP) return true;
    SSL_library_init();      // Khởi tạo thư viện OpenSSL
    SSL_load_error_strings(); // Tải chuỗi lỗi của OpenSSL
    OpenSSL_add_all_algorithms(); // Thêm các thuật toán mã hóa (optional)
    if (setSSLbrowser(host) == false) return false;
    if (setSSLserver(host) == false) return false;
    return true;
}

bool SocketHandler::setSSLbrowser(const std::string& host) {
    std::string directoryName = "./CERTIFICATE/GENERATED";

    if (GetFileAttributesA(directoryName.c_str()) == INVALID_FILE_ATTRIBUTES) {
        if (CreateDirectoryA(directoryName.c_str(), NULL)) {
            std::cout << "Directory '" << directoryName << "' created successfully.\n";
        } else {
            std::cerr << "Failed to create directory '" << directoryName << "'.\n";
            return false;
        }
    }

    const std::string rootKeyPath = "./CERTIFICATE/root.key";
    const std::string rootCertPath = "./CERTIFICATE/root.crt";
    const std::string outputKeyPath = "./CERTIFICATE/GENERATED/" + host + ".key";
    const std::string outputCertPath = "./CERTIFICATE/GENERATED/" + host + ".crt";

    if (!generateCertificate(host, outputCertPath, outputKeyPath, rootKeyPath, rootCertPath)) {
        std::cerr << "Failed to generate certificate.\n";
        return false;
    }

    ctxID[browser] = createSSLContext(outputCertPath.c_str(), outputKeyPath.c_str());
    if (!ctxID[browser]) return false;

    sslID[browser] = SSL_new(ctxID[browser]);
    SSL_set_fd(sslID[browser], socketID[browser]);

    if (SSL_get_verify_result(sslID[browser]) != X509_V_OK) {
        std::cerr << "SSL certificate verification failed\n";
        return false;
    }

    if (SSL_accept(sslID[browser]) <= 0) {
        std::cerr << "CANNOT ACCEPT clientSSL\n";
        ERR_print_errors_fp(stderr);
        return false;
    }
    return true;
}

bool SocketHandler::setSSLserver(const std::string& host) {
    // Tạo SSL_CTX mới cho kết nối đến server
    ctxID[server] = SSL_CTX_new(TLS_client_method());
    if (!ctxID[server]) {
        std::cerr << "CANNOT create SSL for server.\n";
        return false;
    }
    
    // Tạo đối tượng SSL từ SSL_CTX
    sslID[server] = SSL_new(ctxID[server]);
    if (!sslID[server]) {
        std::cerr << "CANNOT create SSL object.\n";
        SSL_CTX_free(ctxID[server]);
        return false;
    }

    // Gắn socket vào SSL
    SSL_set_fd(sslID[server], socketID[server]);

    // Thiết lập SNI (Server Name Indication)
    if (!SSL_set_tlsext_host_name(sslID[server], host.c_str())) {
        std::cerr << "Failed to set SNI (Server Name Indication).\n";
        SSL_free(sslID[server]);
        SSL_CTX_free(ctxID[server]);
        return false;
    }

    // Thực hiện kết nối SSL
    if (SSL_connect(sslID[server]) <= 0) {
        int err = SSL_get_error(sslID[server], -1);
        std::cerr << "SSL_connect failed with error code: " << err << '\n';
        ERR_print_errors_fp(stderr);

        SSL_free(sslID[server]);
        SSL_CTX_free(ctxID[server]);
        return false;
    }

    // std::cout << "SSL connection to " << host << " established successfully.\n";
    return true;
}

bool SocketHandler::isValid() {
    for (int i:{0, 1}) {
        if (socketID[i] == INVALID_SOCKET || socketID[i] == SOCKET_ERROR)
            return false;
    }
    return true;
}
