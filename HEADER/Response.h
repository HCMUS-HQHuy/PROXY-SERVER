#ifndef RESPONSE
#define RESPONSE

#include "./Setting.h"
#include "./HttpHandler.h"

class ResponseHandler : public HttpHandler{
private:
    void modifyResponse(std::string& responseData);
public:
    ResponseHandler(SocketHandler* _socketHandler);
    bool handleResponse();
};


#endif