#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <iostream>
#include <thread>
#include <queue>
#include <vector>
#include <condition_variable>
#include <bits/shared_ptr.h>
#include "./ClientHandler.hpp"
#include <unordered_map>

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::shared_ptr<ClientHandler>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;

    const int MAX_CONNECTIONS_PER_HOST = 3; // Giới hạn kết nối đồng thời trên mỗi host
    std::mutex hostMutex;
    std::unordered_map<std::string, int> hostConnections;

    bool stop;
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();
    void enqueue(std::shared_ptr<ClientHandler>&& p);
    
    bool canProcessRequest(const std::string& host);
    void finishProcessingRequest(const std::string& host);
};


extern ThreadPool requestHandlerPool;


#endif 
