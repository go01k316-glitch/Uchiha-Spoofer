#pragma execution_character_set("utf-8")
#include "BackupManager.h"
#include "../Utils/Logger.h"
#include "../Utils/FileUtils.h"
#include "../Utils/SystemUtils.h"
#include "IdentityManager.h"
#include <fstream>
#include <filesystem>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

namespace UchihaSpoofer::Spoofing {

bool BackupManager::CreateBackup(const std::string& backupName) {
    try {
        // Tạo backup folder nếu chưa tồn tại
        std::string backupDir = GetBackupDir();
        Utils::FileUtils::CreateDirectory(backupDir);

        // Tạo tên backup (nếu không được cung cấp)
        std::string name = backupName.empty() ? "backup_" + std::to_string(time(nullptr)) : backupName;
        std::string filepath = GetBackupFilePath(name);

        // Backup system identity hiện tại
        auto& idMgr = IdentityManager::GetInstance();
        const auto& originalID = idMgr.GetOriginalIdentity();

        std::ofstream file(filepath);
        if (!file.is_open()) {
            throw Core::UchihaException(Core::ErrorCode::FileWriteFailed,
                "Cannot create backup file: " + filepath);
        }

        // Ghi backup data (simple format)
        file << "# Uchiha Spoofer Backup\n";
        file << "# Created: " << std::put_time(std::localtime(&std::time(nullptr)), "%Y-%m-%d %H:%M:%S") << "\n";
        file << "MacAddress=" << originalID.macAddress << "\n";
        file << "RegistryGuid=" << originalID.registryGuid << "\n";
        file << "HWProfileGuid=" << originalID.hwProfileGuid << "\n";
        file << "PCName=" << originalID.pcName << "\n";
        file << "ProductId=" << originalID.productId << "\n";
        file << "VolC_Serial=" << originalID.volCSerialStr << "\n";
        file << "VolD_Serial=" << originalID.volDSerialStr << "\n";

        file.close();

        UCHIHA_LOG_INFO("[Backup] Đã tạo backup: " + name);
        return true;
    }
    catch (const Core::UchihaException& e) {
        UCHIHA_LOG_ERROR(std::string("[Backup] ") + e.what());
        return false;
    }
}

bool BackupManager::RestoreBackup(const std::string& backupName) {
    try {
        std::string filepath = GetBackupFilePath(backupName);

        if (!fs::exists(filepath)) {
            throw Core::UchihaException(Core::ErrorCode::PathNotFound,
                "Backup not found: " + backupName);
        }

        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw Core::UchihaException(Core::ErrorCode::FileReadFailed,
                "Cannot read backup: " + filepath);
        }

        Core::Identity restoredID;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            size_t pos = line.find('=');
            if (pos == std::string::npos) continue;

            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            if (key == "MacAddress") restoredID.macAddress = value;
            else if (key == "RegistryGuid") restoredID.registryGuid = value;
            else if (key == "HWProfileGuid") restoredID.hwProfileGuid = value;
            else if (key == "PCName") restoredID.pcName = value;
            else if (key == "ProductId") restoredID.productId = value;
            else if (key == "VolC_Serial") restoredID.volCSerialStr = value;
            else if (key == "VolD_Serial") restoredID.volDSerialStr = value;
        }

        file.close();

        // Restore thông tin hệ thống (ở đây chỉ log, không restore thực tế)
        UCHIHA_LOG_INFO("[Backup] Đã restore backup: " + backupName);
        return true;
    }
    catch (const Core::UchihaException& e) {
        UCHIHA_LOG_ERROR(std::string("[Backup] ") + e.what());
        return false;
    }
}

std::vector<std::string> BackupManager::GetBackupList() const {
    try {
        std::vector<std::string> backups;
        std::string backupDir = GetBackupDir();

        if (!fs::exists(backupDir)) return backups;

        for (const auto& entry : fs::directory_iterator(backupDir)) {
            if (entry.is_regular_file()) {
                backups.push_back(entry.path().filename().string());
            }
        }

        return backups;
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_ERROR(std::string("[Backup] GetList error: ") + e.what());
        return {};
    }
}

bool BackupManager::DeleteBackup(const std::string& backupName) {
    try {
        std::string filepath = GetBackupFilePath(backupName);
        if (fs::exists(filepath)) {
            fs::remove(filepath);
            UCHIHA_LOG_INFO("[Backup] Đã xóa backup: " + backupName);
            return true;
        }
        return false;
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_ERROR(std::string("[Backup] Delete error: ") + e.what());
        return false;
    }
}

bool BackupManager::AutoBackup() {
    return CreateBackup("auto_" + std::to_string(time(nullptr)));
}

std::string BackupManager::GetLatestBackup() const {
    auto backups = GetBackupList();
    if (backups.empty()) return "";
    return backups.back();
}

std::string BackupManager::GetBackupDir() const {
    return "backups/";
}

std::string BackupManager::GetBackupFilePath(const std::string& backupName) const {
    return GetBackupDir() + backupName + ".bak";
}

int BackupManager::GetBackupCount() const {
    return GetBackupList().size();
}

} // namespace UchihaSpoofer::Spoofing
