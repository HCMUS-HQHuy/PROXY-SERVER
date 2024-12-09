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

    bool stop;
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();
    void enqueue(std::shared_ptr<ClientHandler>&& p);
};

extern ThreadPool requestHandlerPool;

#endif 
