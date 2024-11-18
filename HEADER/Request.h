#ifndef REQUEST_H
#define REQUEST_H

#include "./Setting.h"
#include "./SocketHandler.h"

class RequestHandler{
private:
    SocketHandler* socketHandler;
    void modifyRequest(std::string& requestData);
public:
    RequestHandler(SocketHandler* socketHandler);
    bool handleRequest();
};



#endif