#ifndef SETTING_HPP
#define SETTING_HPP

#include <iostream>
#include <string>
#include <vector>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <csignal>
#include <atomic>

using std::string;
extern std::atomic<bool> ServerRunning;

#define HTTPS_PORT 443
#define HTTP_PORT 80
#define LOCAL_PORT 8080
#define BUFFER_SIZE 1024
#endif