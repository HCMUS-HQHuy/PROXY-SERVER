#include "./../../HEADER/NetworkManager.hpp"
#include <iphlpapi.h>

#include "./../../HEADER/Logger.hpp"

NetworkManager::NetworkManager() {
    IPv4 = getIPv4();
    int error = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (error != 0) {
        Logger::errorStatus(-33);
        std::cerr << "WSAStartup failed: " << error << '\n';
    }
    localSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (localSocket == INVALID_SOCKET) {
        Logger::errorStatus(-34);
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
    }
    std::cerr << "NetworkManager initialized successfully!\n";
}

NetworkManager::~NetworkManager() {
    delete[] IPv4;
    closesocket(localSocket);
    WSACleanup();
    std::cerr << "NetworkManager destructed successfully!\n";
}
int NetworkManager::sendMessage(SOCKET &Socket, char *message, int sizeMessage) {
    int bytesSent = send(Socket, message, sizeMessage, 0);
    if (bytesSent == SOCKET_ERROR) {
        Logger::errorStatus(-35);
        std::cerr << "Sent message faild! Code error: " << WSAGetLastError() << std::endl;
    }
    return bytesSent;
}

int NetworkManager::receiveMessage(SOCKET &Socket, char* message, int sizeMessage) {
    int byteReceived = recv(Socket, message, sizeMessage, 0);
    if (byteReceived == SOCKET_ERROR) {
        Logger::errorStatus(-36);
        std::cerr << "Error receiving data: " << WSAGetLastError() << std::endl;
    }

    if (byteReceived == 0) {
        Logger::errorStatus(-37);
        std::cerr << "Connection interrupted" << std::endl;
    }
    return byteReceived;
}


int NetworkManager::sendLargeData(SOCKET sock, char* data, int dataSize) {
    const int chunkSize = 2048; 
    int totalSent = 0;
    while (totalSent < dataSize) {
        int remaining = dataSize - totalSent;
        int bytesToSend = (remaining < chunkSize) ? remaining : chunkSize;
        int sent = send(sock, data + totalSent, bytesToSend, 0);
        if (sent == SOCKET_ERROR) {
            int errorCode = WSAGetLastError();
            Logger::errorStatus(-38);
            std::cerr << "Error sending data: " << errorCode << std::endl;
            return SOCKET_ERROR;
        }
        totalSent += sent;
    }

    return totalSent; 
}


int NetworkManager::receiveLargeData(SOCKET sock, char* buffer, int dataSize) {
    const int chunkSize = 2048; 
    int totalReceived = 0;
    while (totalReceived < dataSize) {
        int remaining = dataSize - totalReceived;
        int bytesToReceive = (remaining < chunkSize) ? remaining : chunkSize;
        int received = recv(sock, buffer + totalReceived, bytesToReceive, 0);

        if (received == SOCKET_ERROR) {
            int errorCode = WSAGetLastError();
            Logger::errorStatus(-36);
            std::cerr << "Error receiving data: " << errorCode 
                        << " at totalReceived = " << totalReceived << '\n';
            return SOCKET_ERROR;
        }

        if (received == 0) {
            Logger::errorStatus(-39);
            break;
        }
        totalReceived += received;
    }
    return totalReceived;
}


char* NetworkManager::getIPv4() {
    char *ipAddress = new char[INET_ADDRSTRLEN]();
    ULONG bufferSize = 15000;
    PIP_ADAPTER_ADDRESSES adapterAddresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);

    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, adapterAddresses, &bufferSize) == NO_ERROR) {
        PIP_ADAPTER_ADDRESSES currentAdapter = adapterAddresses;

        while (currentAdapter) {
            if (currentAdapter->OperStatus == IfOperStatusUp) {
                PIP_ADAPTER_UNICAST_ADDRESS unicast = currentAdapter->FirstUnicastAddress;
                while (unicast) {
                    if (unicast->Address.lpSockaddr->sa_family == AF_INET) {
                        sockaddr_in *sa_in = (sockaddr_in*)unicast->Address.lpSockaddr;
                        inet_ntop(AF_INET, &(sa_in->sin_addr), ipAddress, INET_ADDRSTRLEN);

                        if (strncmp(ipAddress, "169.254", 7) != 0) {
                            break; 
                        }
                    }
                    unicast = unicast->Next;
                }
            }
            if (ipAddress[0] != 0) break;
            currentAdapter = currentAdapter->Next;
        }
    } else {
        Logger::errorStatus(-40);
    }
    free(adapterAddresses);
    return ipAddress;
}