#ifndef RESPONSE
#define RESPONSE

#include "./Setting.h"
#include "./HttpHandler.h"

class ResponseHandler : public HttpHandler{
private:
public:
    ResponseHandler(SocketHandler* _socketHandler);
    bool handleResponse();
};


#endif