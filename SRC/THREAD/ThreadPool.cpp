#include "./../../HEADER/ThreadPool.hpp"
#include "./../../HEADER/Logger.hpp"

ThreadPool requestHandlerPool(std::thread::hardware_concurrency() * 10);
// ThreadPool requestHandlerPool(6);
// Khởi tạo pool với số luồng cố định
ThreadPool::ThreadPool(size_t numThreads) : stop(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex);
                    this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                    // [this] được gọi khi bị notetify. 
                    // Nếu trả về true thì hoạt động lại và lock
                    // Nếu trả về false thì chờ để  notetify và unlock

                    if (this->stop && this->tasks.empty()) return;
                    
                    if (!this->tasks.empty()) {
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                }
                
                if (task) {
                    try {
                        task();
                    } catch (const std::exception& e) {
                        logger.logError(-46);
                        std::cerr << "Exception in task: " << e.what() << std::endl;
                    } catch (...) {
                        logger.logError(-47);
                    }
                }
            }
        });
    }
}

// Đưa task vào hàng đợi

void ThreadPool::enqueue(std::function<void()>&& task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        tasks.emplace(std::forward<std::function<void()>>(task));
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
    for (std::thread& worker : workers) 
        if (worker.joinable()) worker.join();
}