#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "./Setting.hpp"
#include "./HttpHandler.hpp"


class RequestHandler: public HttpHandler{
private:
public:
    RequestHandler(SocketHandler* socketHandler);
    bool handleRequest();
};



#endif