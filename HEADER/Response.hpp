#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "./Setting.hpp"
#include "./HttpHandler.hpp"

class ResponseHandler : public HttpHandler{
private:
public:
    ResponseHandler(SocketHandler* _socketHandler);
    void handleResponse();
};


#endif