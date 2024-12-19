#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/crypto.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

#define DEFAULT_PORT 443
#define MAX_BUF_SIZE 4096

void init_openssl() {
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    SSL_library_init();
}

SSL_CTX* create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = TLS_client_method();  // Use TLS for client-side connections
    ctx = SSL_CTX_new(method);

    if (!ctx) {
        ERR_print_errors_fp(stderr);
        abort();
    }

    return ctx;
}

void cleanup_openssl() {
    EVP_cleanup();
}

void init_winsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        exit(EXIT_FAILURE);
    }
}

SOCKET create_socket(const char *hostname, int port) {
    struct sockaddr_in server_addr;
    SOCKET sock;
    struct hostent *host_info;
    unsigned long server_ip;
    
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed\n");
        exit(EXIT_FAILURE);
    }
    
    host_info = gethostbyname(hostname);
    if (host_info == NULL) {
        fprintf(stderr, "Unable to resolve host: %s\n", hostname);
        exit(EXIT_FAILURE);
    }

    server_ip = *(unsigned long *)host_info->h_addr_list[0];
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = server_ip;
    server_addr.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Connection failed\n");
        exit(EXIT_FAILURE);
    }

    return sock;
}

void send_https_request(SOCKET sock, SSL *ssl) {
    // Prepare HTTP GET request
    const char *request = "GET /wiki/Main_Page HTTP/1.1\r\n"
                          "Host: en.wikipedia.org\r\n"
                          "User-Agent: CustomClient/1.0\r\n"
                          "Connection: close\r\n"
                          "\r\n";

    // Send request via SSL connection
    SSL_write(ssl, request, strlen(request));
}

void receive_https_response(SSL *ssl) {
    char buffer[MAX_BUF_SIZE];
    int bytes;

    while ((bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes] = 0;  // Null-terminate string
        printf("%s", buffer);  // Print response to stdout
    }
}

int main() {
    const char *hostname = "en.wikipedia.org";  // Wikipedia domain
    int port = DEFAULT_PORT;  // HTTPS runs on port 443
    SOCKET sock;
    SSL_CTX *ctx;
    SSL *ssl;
    
    // Initialize WinSock and OpenSSL
    init_winsock();
    init_openssl();

    // Create SSL context
    ctx = create_context();

    // Create a socket and connect to the server
    sock = create_socket(hostname, port);

    // Create SSL object
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);

    // Establish SSL/TLS connection
    if (SSL_connect(ssl) == -1) {
        fprintf(stderr, "SSL connection failed\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    // Send HTTPS request
    send_https_request(sock, ssl);

    // Receive and print the response
    receive_https_response(ssl);

    // Clean up
    SSL_shutdown(ssl);
    SSL_free(ssl);
    closesocket(sock);
    SSL_CTX_free(ctx);
    cleanup_openssl();
    WSACleanup();

    return 0;
}
