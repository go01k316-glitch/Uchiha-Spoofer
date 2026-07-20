#pragma once

#include "../Core/Exception.h"
#include "../Utils/SystemUtils.h"
#include <string>
#include <vector>

namespace UchihaSpoofer::Spoofing {

class DeepCleaner {
public:
    static DeepCleaner& GetInstance() {
        static DeepCleaner instance;
        return instance;
    }

    // Callback cho progress update
    using CleanCallback = std::function<void(float progress, const std::string& task)>;

    // Bắt đầu deep clean (chạy trong thread riêng)
    void StartDeepClean(CleanCallback onProgress = nullptr);

    // Kiểm tra trạng thái
    bool IsCleaningInProgress() const { return cleaningInProgress_; }

    // Hủy bỏ operation
    void CancelCleaning() { cancelRequested_ = true; }

    ~DeepCleaner() = default;

private:
    DeepCleaner() = default;
    DeepCleaner(const DeepCleaner&) = delete;
    DeepCleaner& operator=(const DeepCleaner&) = delete;

    // Worker thread
    void CleaningWorkerThread(CleanCallback onProgress);

    // Các công việc dọn dẹp riêng biệt
    void CleanTempFiles();
    void CleanWindowsLogs();
    void CleanGameRegistries();
    void CleanGameCaches();
    void CleanNetworkCache();
    void CleanEventLogs();

    std::atomic<bool> cleaningInProgress_{false};
    std::atomic<bool> cancelRequested_{false};
};

} // namespace UchihaSpoofer::Spoofing
