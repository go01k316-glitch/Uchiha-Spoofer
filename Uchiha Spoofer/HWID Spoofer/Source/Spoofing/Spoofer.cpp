#pragma execution_character_set("utf-8")
#include "Spoofer.h"
#include "IdentityManager.h"
#include "../Driver/DriverManager.h"
#include "../Utils/SystemUtils.h"
#include "../Utils/Logger.h"
#include <thread>
#include <chrono>

namespace UchihaSpoofer::Spoofing {

void Spoofer::StartSpoofing(ProgressCallback onProgress) {
    if (operationInProgress_) {
        UCHIHA_LOG_WARNING("[Spoof] Một thao tác khác đang chạy");
        return;
    }

    cancelRequested_ = false;
    std::thread worker(&Spoofer::SpoofingWorkerThread, this, onProgress);
    worker.detach();
}

void Spoofer::RecoverOriginal(ProgressCallback onProgress) {
    if (operationInProgress_) {
        UCHIHA_LOG_WARNING("[Recover] Một thao tác khác đang chạy");
        return;
    }

    cancelRequested_ = false;
    std::thread worker(&Spoofer::RecoveryWorkerThread, this, onProgress);
    worker.detach();
}

void Spoofer::SpoofingWorkerThread(ProgressCallback onProgress) {
    operationInProgress_ = true;

    try {
        auto updateProgress = [&](float progress, const std::string& task) {
            if (onProgress) onProgress(progress, task);
            if (cancelRequested_) {
                throw Core::UchihaException(Core::ErrorCode::OperationCancelled);
            }
        };

        updateProgress(0.05f, "Đang khởi tạo thuật thức ngụy trang...");
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        UCHIHA_LOG_INFO("[Spoof] Bắt đầu tiến trình spoof");

        updateProgress(0.15f, "Đang thiết lập danh tính ảo mới...");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        DoSpoof();

        updateProgress(1.0f, "Thuật thức hoàn tất!");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        isSpoofed_ = true;
        UCHIHA_LOG_INFO("[Spoof] Spoof hoàn tất thành công!");
    }
    catch (const Core::UchihaException& e) {
        UCHIHA_LOG_ERROR(std::string("[Spoof] ") + e.what());
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_ERROR(std::string("[Spoof] Exception: ") + e.what());
    }

    operationInProgress_ = false;
}

void Spoofer::RecoveryWorkerThread(ProgressCallback onProgress) {
    operationInProgress_ = true;

    try {
        auto updateProgress = [&](float progress, const std::string& task) {
            if (onProgress) onProgress(progress, task);
            if (cancelRequested_) {
                throw Core::UchihaException(Core::ErrorCode::OperationCancelled);
            }
        };

        updateProgress(0.10f, "Đang khởi động tiến trình giải trừ ảo ảnh...");
        UCHIHA_LOG_INFO("[Recover] Bắt đầu khôi phục danh tính gốc");

        DoRecover();

        updateProgress(1.0f, "Giải thuật hoàn tất!");
        isSpoofed_ = false;
        UCHIHA_LOG_INFO("[Recover] Khôi phục hoàn tất thành công!");
    }
    catch (const Core::UchihaException& e) {
        UCHIHA_LOG_ERROR(std::string("[Recover] ") + e.what());
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_ERROR(std::string("[Recover] Exception: ") + e.what());
    }

    operationInProgress_ = false;
}

void Spoofer::DoSpoof() {
    auto& idManager = IdentityManager::GetInstance();
    const auto& fakeID = idManager.GetFakeIdentity();

    try {
        // Thay đổi Registry
        if (config_.selectPCName) {
            Utils::SystemUtils::SetComputerName(fakeID.pcName);
        }
        if (config_.selectGUID) {
            Utils::SystemUtils::WriteRegistry(
                "SOFTWARE\\Microsoft\\Cryptography", "MachineGuid", fakeID.registryGuid);
        }
        // ... thêm các thay đổi khác

        UCHIHA_LOG_INFO("[Spoof] Spoof logic đã thực hiện");
    }
    catch (const Core::UchihaException& e) {
        throw;
    }
}

void Spoofer::DoRecover() {
    auto& idManager = IdentityManager::GetInstance();
    const auto& originalID = idManager.GetOriginalIdentity();

    try {
        // Khôi phục Registry
        Utils::SystemUtils::SetComputerName(originalID.pcName);
        Utils::SystemUtils::WriteRegistry(
            "SOFTWARE\\Microsoft\\Cryptography", "MachineGuid", originalID.registryGuid);
        // ... thêm các khôi phục khác

        UCHIHA_LOG_INFO("[Recover] Recovery logic đã thực hiện");
    }
    catch (const Core::UchihaException& e) {
        throw;
    }
}

} // namespace UchihaSpoofer::Spoofing
