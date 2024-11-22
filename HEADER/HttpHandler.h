#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include "./setting.h"
#include "./SocketHandler.h"

#define BUFFER_SIZE 1024
#define sz(s) (int)s.size()

class HttpHandler {
private:
    string header; 
    string body;

    char *buffer;

    bool flag;
    int contentLength;
    int isChunked;
    bool headersParsed;

    void handleMessage(int bytesReceived);

protected: 
    SocketHandler* socketHandler;
    void printHeader();
public:
    HttpHandler();
    ~HttpHandler();
    int sendMessage(Socket id, int size = BUFFER_SIZE);
    int receiveMessage(Socket id, int size = BUFFER_SIZE);
    bool isEndMessage();
};

#endif