#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <thread>
#include <atomic>
#include <queue>
#include <vector>
#include <functional>
#include <condition_variable>
#include <future>

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();
    template <class F> void enqueue(F&& f);
};


#endif 
