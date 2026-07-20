#pragma execution_character_set("utf-8")
#include "Spoofer.h"
#include "IdentityManager.h"
#include "BackupManager.h"
#include "../Driver/DriverManager.h"
#include "../Utils/SystemUtils.h"
#include "../Utils/Logger.h"
#include "../Utils/AsyncWorker.h"
#include <thread>
#include <chrono>

namespace UchihaSpoofer::Spoofing {

void Spoofer::StartSpoofing(ProgressCallback onProgress) {
    if (operationInProgress_) {
        UCHIHA_LOG_WARNING("[Spoof] Một thao tác khác đang chạy");
        return;
    }

    // Auto backup trước khi spoof
    UCHIHA_LOG_INFO("[Spoof] Tạo auto backup...");
    BackupManager::GetInstance().AutoBackup();

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
        // Validate identity trước khi spoof
        if (!idManager.ValidateIdentity(fakeID)) {
            throw Core::UchihaException(Core::ErrorCode::InvalidParameter,
                "Invalid identity for spoofing");
        }

        // Thay đổi Registry
        if (config_.selectPCName) {
            Utils::SystemUtils::SetComputerName(fakeID.pcName);
            UCHIHA_LOG_INFO("[Spoof] Changed PC name to: " + fakeID.pcName);
        }
        if (config_.selectGUID) {
            Utils::SystemUtils::WriteRegistry(
                "SOFTWARE\\Microsoft\\Cryptography", "MachineGuid", fakeID.registryGuid);
            UCHIHA_LOG_INFO("[Spoof] Changed Machine GUID");
        }
        if (config_.selectVolC) {
            Utils::SystemUtils::WriteVolumeSerial('C', fakeID.volC_Raw);
            UCHIHA_LOG_INFO("[Spoof] Changed Volume C serial");
        }
        if (config_.selectVolD) {
            Utils::SystemUtils::WriteVolumeSerial('D', fakeID.volD_Raw);
            UCHIHA_LOG_INFO("[Spoof] Changed Volume D serial");
        }

        // CPU/BIOS spoofing nếu driver available
        if (config_.selectCPU || config_.selectBIOS) {
            auto& driverMgr = Driver::DriverManager::GetInstance();
            if (driverMgr.IsLoaded()) {
                if (config_.selectCPU) {
                    driverMgr.SpoofCPU();
                    UCHIHA_LOG_INFO("[Spoof] Spoofed CPU via driver");
                }
                if (config_.selectBIOS) {
                    driverMgr.SpoofBIOS();
                    UCHIHA_LOG_INFO("[Spoof] Spoofed BIOS via driver");
                }
            }
        }

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
        Utils::SystemUtils::WriteVolumeSerial('C', originalID.volC_Raw);
        Utils::SystemUtils::WriteVolumeSerial('D', originalID.volD_Raw);

        UCHIHA_LOG_INFO("[Recover] Recovery logic đã thực hiện");
    }
    catch (const Core::UchihaException& e) {
        throw;
    }
}

} // namespace UchihaSpoofer::Spoofing
