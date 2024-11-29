#ifndef SOCKETHANDLER_H
#define SOCKETHANDLER_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>

enum Protocol { HTTP, HTTPS };
enum Socket {browser, server};

class SocketHandler {
    
private:
    std::string host;

    SSL_CTX* ctxID[2];

    SOCKET connectToServer();
    bool setSSLbrowser();
    bool setSSLserver();
    
public:
    Protocol protocol;

    SOCKET socketID[2];

    SSL* sslID[2];

    SocketHandler(SOCKET browser);
    ~SocketHandler();
    bool setSSLContexts();
    bool isValid();
};

#endif