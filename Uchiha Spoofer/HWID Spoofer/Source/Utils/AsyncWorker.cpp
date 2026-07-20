#pragma execution_character_set("utf-8")
#include "AsyncWorker.h"
#include "Logger.h"

namespace UchihaSpoofer::Utils {

AsyncWorker::AsyncWorker() {
    worker_ = std::thread(&AsyncWorker::WorkerThread, this);
}

AsyncWorker::~AsyncWorker() {
    Shutdown();
}

void AsyncWorker::WorkerThread() {
    while (running_) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            cv_.wait(lock, [this] { return !tasks_.empty() || !running_; });

            if (!running_ && tasks_.empty()) break;
            if (tasks_.empty()) continue;

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        try {
            if (task) {
                task();
            }
        }
        catch (const std::exception& e) {
            Logger::GetInstance().Error(std::string("[AsyncWorker] Task error: ") + e.what());
        }
    }
}

void AsyncWorker::Enqueue(Task task) {
    if (!running_) {
        Logger::GetInstance().Warning("[AsyncWorker] Worker đã shutdown");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        tasks_.push(task);
    }
    cv_.notify_one();
}

void AsyncWorker::WaitAll() {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            if (tasks_.empty()) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

size_t AsyncWorker::PendingTaskCount() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return tasks_.size();
}

void AsyncWorker::Shutdown() {
    running_ = false;
    cv_.notify_one();
    if (worker_.joinable()) {
        worker_.join();
    }
}

} // namespace UchihaSpoofer::Utils
