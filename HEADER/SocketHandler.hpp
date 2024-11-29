#ifndef SOCKETHANDLER_HPP
#define SOCKETHANDLER_HPP

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>

enum Protocol { HTTP, HTTPS };
enum Socket {browser, server};

class SocketHandler {
    
private:
    SSL_CTX* ctxID[2];

    bool setSSLbrowser(const std::string& host);
    bool setSSLserver(const std::string& host);
    
public:
    Protocol protocol;

    SOCKET socketID[2];

    SSL* sslID[2];

    SocketHandler(SOCKET browser, SOCKET remote, bool isHTTPS);
    ~SocketHandler();
    bool setSSLContexts(const std::string& host);
    bool isValid();
};

#endif