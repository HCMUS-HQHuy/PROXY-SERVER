#ifndef SOCKETHANDLER_H
#define SOCKETHANDLER_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>

enum Protocol { HTTP, HTTPS };

class SocketHandler {
    
private:
    std::string host;

    SSL_CTX* remoteCtx;
    SSL_CTX* browserCtx;

    SOCKET connectToServer();
    bool setSSLbrowser();
    bool setSSLserver();

public:
    Protocol protocol;

    SOCKET remoteSocket;
    SOCKET browserSocket;

    SSL* remoteSSL;
    SSL* browserSSL;

    SocketHandler(SOCKET browser);

    ~SocketHandler();

    bool isValid();
    bool setSSLContexts();
};

#endif