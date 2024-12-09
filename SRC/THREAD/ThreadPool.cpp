#include "./../../HEADER/ThreadPool.hpp"
#include "./../../HEADER/Setting.hpp"
#include "./../../HEADER/Logger.hpp"

ThreadPool requestHandlerPool(std::thread::hardware_concurrency() * 30);
// ThreadPool requestHandlerPool(1);
// Khởi tạo pool với số luồng cố định
ThreadPool::ThreadPool(size_t numThreads) : stop(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this] {
            while (true) {
                std::shared_ptr<ClientHandler> task;
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex);
                    this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty() || !ServerRunning; });
                    // [this] được gọi khi bị notetify. 
                    // Nếu trả về true thì hoạt động lại và lock
                    // Nếu trả về false thì chờ để  notetify và unlock

                    if ((this->stop && this->tasks.empty())  || !ServerRunning) return;
                    
                    if (!this->tasks.empty()) {
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                }
                
                if (task) {
                    try {
                        task->handleRequest();
                    } catch (const std::exception& e) {
                        Logger::errorStatus(-46);
                        std::cerr << "Exception in task: " << e.what() << std::endl;
                    } catch (...) {
                        Logger::errorStatus(-47);
                    }
                }
            }
        });
        workers[i].detach();
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
    for (std::thread& worker : workers) if (worker.joinable()) worker.join();
}