#pragma once

#include "../Core/Types.h"
#include "../Core/Exception.h"
#include <functional>
#include <atomic>
#include <memory>

namespace UchihaSpoofer::Spoofing {

class Spoofer {
public:
    static Spoofer& GetInstance() {
        static Spoofer instance;
        return instance;
    }

    // Callback cho progress update
    using ProgressCallback = std::function<void(float progress, const std::string& task)>;

    // Bắt đầu spoof (chạy trong thread riêng)
    void StartSpoofing(ProgressCallback onProgress = nullptr);

    // Khôi phục danh tính gốc
    void RecoverOriginal(ProgressCallback onProgress = nullptr);

    // Kiểm tra trạng thái
    bool IsSpoofed() const { return isSpoofed_; }
    bool IsOperationInProgress() const { return operationInProgress_; }

    // Hủy bỏ operatio
    void CancelOperation() { cancelRequested_ = true; }

    // Lấy cấu hình hiện tại
    const Core::SpoofConfig& GetConfig() const { return config_; }
    void SetConfig(const Core::SpoofConfig& config) { config_ = config; }

    ~Spoofer() = default;

private:
    Spoofer() = default;
    Spoofer(const Spoofer&) = delete;
    Spoofer& operator=(const Spoofer&) = delete;

    // Worker threads
    void SpoofingWorkerThread(ProgressCallback onProgress);
    void RecoveryWorkerThread(ProgressCallback onProgress);

    // Công việc thực tế
    void DoSpoof();
    void DoRecover();

    bool isSpoofed_ = false;
    std::atomic<bool> operationInProgress_{false};
    std::atomic<bool> cancelRequested_{false};
    Core::SpoofConfig config_;
};

} // namespace UchihaSpoofer::Spoofing
