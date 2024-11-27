#ifndef REQUEST_H
#define REQUEST_H

#include "./Setting.h"
#include "./HttpHandler.h"


class RequestHandler: public HttpHandler{
private:
public:
    RequestHandler(SocketHandler* socketHandler);
    bool handleRequest();
};



#endif