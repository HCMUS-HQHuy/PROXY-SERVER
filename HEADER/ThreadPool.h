#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <thread>
#include <queue>
#include <vector>
#include <condition_variable>
#include <bits/shared_ptr.h>
#include "ClientHandler.h"

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
