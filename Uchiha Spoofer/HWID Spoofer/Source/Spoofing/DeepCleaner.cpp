#pragma execution_character_set("utf-8")
#include "DeepCleaner.h"
#include "../Utils/Logger.h"
#include "../Utils/FileUtils.h"
#include <thread>
#include <chrono>
#include <windows.h>
#include <shlobj.h>

pragma comment(lib, "shell32.lib")

namespace UchihaSpoofer::Spoofing {

void DeepCleaner::StartDeepClean(CleanCallback onProgress) {
    if (cleaningInProgress_) {
        UCHIHA_LOG_WARNING("[DeepClean] Một thao tác khác đang chạy");
        return;
    }

    cancelRequested_ = false;
    std::thread worker(&DeepCleaner::CleaningWorkerThread, this, onProgress);
    worker.detach();
}

void DeepCleaner::CleaningWorkerThread(CleanCallback onProgress) {
    cleaningInProgress_ = true;
    UCHIHA_LOG_INFO("[DeepClean] Bắt đầu deep cleaning engine...");

    try {
        auto updateProgress = [&](float progress, const std::string& task) {
            if (onProgress) onProgress(progress, task);
            if (cancelRequested_) {
                throw Core::UchihaException(Core::ErrorCode::OperationCancelled);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        };

        // Dọn Temp files
        updateProgress(0.15f, "Đang xóa Temp files & Prefetch...");
        CleanTempFiles();

        // Dọn Game caches
        updateProgress(0.35f, "Đang xóa caches của Steam, Epic Games...");
        CleanGameCaches();

        // Dọn Game registries
        updateProgress(0.55f, "Đang xóa Registry của Game...");
        CleanGameRegistries();

        // Dọn Event Logs
        updateProgress(0.75f, "Đang xóa Windows Event Logs...");
        CleanEventLogs();

        // Dọn Network cache
        updateProgress(0.90f, "Đang flush DNS & reset network...");
        CleanNetworkCache();

        updateProgress(1.0f, "Deep Clean hoàn tất!");
        UCHIHA_LOG_INFO("[DeepClean] Deep cleaning hoàn tất thành công!");
    }
    catch (const Core::UchihaException& e) {
        UCHIHA_LOG_ERROR(std::string("[DeepClean] ") + e.what());
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_ERROR(std::string("[DeepClean] Exception: ") + e.what());
    }

    cleaningInProgress_ = false;
}

void DeepCleaner::CleanTempFiles() {
    try {
        UCHIHA_LOG_INFO("[DeepClean] Dọn sạch Temp & Prefetch...");

        // Xóa Windows Temp
        char tempPath[MAX_PATH];
        if (GetTempPathA(MAX_PATH, tempPath)) {
            Utils::FileUtils::CleanDirectoryRecursive(tempPath);
        }

        // Xóa C:\Windows\Temp
        Utils::FileUtils::CleanDirectoryRecursive("C:\\Windows\\Temp");

        // Xóa Prefetch
        Utils::FileUtils::CleanDirectoryRecursive("C:\\Windows\\Prefetch");
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_WARNING(std::string("[DeepClean] Clean temp error: ") + e.what());
    }
}

void DeepCleaner::CleanGameCaches() {
    try {
        UCHIHA_LOG_INFO("[DeepClean] Dọn sạch game caches...");

        char localAppData[MAX_PATH];
        if (GetEnvironmentVariableA("LOCALAPPDATA", localAppData, MAX_PATH)) {
            std::string basePath = localAppData;

            // Steam
            Utils::FileUtils::CleanDirectoryRecursive(basePath + "\\Steam");

            // Epic Games
            Utils::FileUtils::CleanDirectoryRecursive(basePath + "\\Epic Games");

            // Riot Games
            Utils::FileUtils::CleanDirectoryRecursive(basePath + "\\Riot Games");

            // Valorant
            Utils::FileUtils::CleanDirectoryRecursive(basePath + "\\Valorant");
        }
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_WARNING(std::string("[DeepClean] Clean game cache error: ") + e.what());
    }
}

void DeepCleaner::CleanGameRegistries() {
    try {
        UCHIHA_LOG_INFO("[DeepClean] Dọn sạch game registries...");

        // Xóa Steam registry
        Utils::SystemUtils::ExecuteSilentCommand(
            "reg.exe delete \"HKCU\\Software\\Valve\\Steam\\ActiveProcess\" /f");

        // Xóa Epic Games registry
        Utils::SystemUtils::ExecuteSilentCommand(
            "reg.exe delete \"HKCU\\Software\\Epic Games\\Unreal Engine\\Hardware Survey\" /f");
        Utils::SystemUtils::ExecuteSilentCommand(
            "reg.exe delete \"HKCU\\Software\\Epic Games\\Unreal Engine\\Identifiers\" /f");
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_WARNING(std::string("[DeepClean] Clean registry error: ") + e.what());
    }
}

void DeepCleaner::CleanNetworkCache() {
    try {
        UCHIHA_LOG_INFO("[DeepClean] Dọn sạch network cache...");

        Utils::SystemUtils::ExecuteSilentCommand("ipconfig.exe /flushdns");
        Utils::SystemUtils::ExecuteSilentCommand("ipconfig.exe /release");
        Utils::SystemUtils::ExecuteSilentCommand("ipconfig.exe /renew");
        Utils::SystemUtils::ExecuteSilentCommand("netsh.exe winsock reset");
        Utils::SystemUtils::ExecuteSilentCommand("netsh.exe int ip reset");
        Utils::SystemUtils::ExecuteSilentCommand("arp.exe -d *");
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_WARNING(std::string("[DeepClean] Clean network error: ") + e.what());
    }
}

void DeepCleaner::CleanEventLogs() {
    try {
        UCHIHA_LOG_INFO("[DeepClean] Dọn sạch Event Logs...");

        Utils::SystemUtils::ExecuteSilentCommand(
            "powershell.exe -WindowStyle Hidden -Command \"Get-EventLog -LogName * | ForEach-Object { Clear-EventLog -LogName $_.Log }\"");
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_WARNING(std::string("[DeepClean] Clean event logs error: ") + e.what());
    }
}

} // namespace UchihaSpoofer::Spoofing
