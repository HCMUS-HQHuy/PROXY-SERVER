#ifndef SUPPORT_FUNCTION_H
#define SUPPORT_FUNCTION_H 1

#include <winsock2.h>

bool receiveMSG(SOCKET socket, char *buffer, int len);
bool sendMSG(SOCKET socket, char *buffer, int len);

#endif 