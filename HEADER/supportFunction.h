#ifndef SUPPORT_FUNCTION_H
#define SUPPORT_FUNCTION_H 1

#include "../HEADER/setting.h"

bool receiveMSG(SOCKET socket, char *buffer, int &len);
bool sendMSG(SOCKET socket, char *buffer, int &len);
bool receiveLargeData(SOCKET socket, std::vector<char>& data);
bool sendLargeData(SOCKET socket, const std::vector<char>& data);
#endif 