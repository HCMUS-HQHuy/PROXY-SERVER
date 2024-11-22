#ifndef REQUEST_H
#define REQUEST_H

#include "./Setting.h"
#include "./HttpHandler.h"


class RequestHandler: public HttpHandler{
private:
    void modifyRequest(std::string& requestData);
public:
    RequestHandler(SocketHandler* socketHandler);
    bool handleRequest();
};



#endif