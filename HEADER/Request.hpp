#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "./Setting.hpp"
#include "./HttpHandler.hpp"


class RequestHandler: public HttpHandler{
private:
public:
    RequestHandler();
    RequestHandler(SocketHandler* socketHandler);
    void handleRequest();
    bool receiveRequest(SOCKET sock);
    bool sendRequest(SOCKET sock);
};



#endif