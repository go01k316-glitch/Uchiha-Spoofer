#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace UchihaSpoofer::Utils {

// Task wrapper
using Task = std::function<void()>;

// Async worker cho background operations
class AsyncWorker {
public:
    static AsyncWorker& GetInstance() {
        static AsyncWorker instance;
        return instance;
    }

    // Enqueue task để chạy async
    void Enqueue(Task task);

    // Chờ tất cả tasks hoàn tất
    void WaitAll();

    // Shutdown worker
    void Shutdown();

    // Số tasks còn chờ xử lý
    size_t PendingTaskCount() const;

    ~AsyncWorker();

private:
    AsyncWorker();
    AsyncWorker(const AsyncWorker&) = delete;
    AsyncWorker& operator=(const AsyncWorker&) = delete;

    void WorkerThread();

    std::queue<Task> tasks_;
    mutable std::mutex queueMutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_{true};
    std::thread worker_;
};

} // namespace UchihaSpoofer::Utils
