#ifndef RESPONSE
#define RESPONSE

#include "./Setting.h"
#include "./SocketHandler.h"

class ResponseHandler{
private:
    SocketHandler* socketHandler;
    void modifyResponse(std::string& responseData);
    
public:
    ResponseHandler(SocketHandler* socketHandler);
    bool handleResponse();
};


#endif