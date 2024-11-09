#include "./setting.h"

const int bufferSize = 32000;

class Message {
private:
    char *buffer;
public:
    Message();
    Message(const char* MSG);
    ~Message();
    int receiveMessage(SOCKET socket, int byteReceive = bufferSize);
    int sendMessage(SOCKET socket, int byteSent = bufferSize);
    bool getHostFromRequest(string &hostname, int &port);
    char* get();
    void print();
};

