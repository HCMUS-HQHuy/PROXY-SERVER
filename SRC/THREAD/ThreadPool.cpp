#include "./../../HEADER/ThreadPool.h"

ThreadPool requestHandlerPool(std::thread::hardware_concurrency() * 50);
// Khởi tạo pool với số luồng cố định
ThreadPool::ThreadPool(size_t numThreads) : stop(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this] {
            while (true) {
                std::shared_ptr<ClientHandler> task;
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex);
                    this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                    if (this->stop && this->tasks.empty()) return;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task->handleRequest();
            }
        });
    }
}

// Đưa task vào hàng đợi
void ThreadPool::enqueue(std::shared_ptr<ClientHandler>&& p) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        tasks.emplace(std::forward<std::shared_ptr<ClientHandler>>(p));
    }
    condition.notify_one();
}

// Hủy pool
ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread& worker : workers) worker.join();
}