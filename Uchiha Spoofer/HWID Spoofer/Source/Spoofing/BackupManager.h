#pragma once

#include <string>
#include "../Core/Exception.h"

namespace UchihaSpoofer::Spoofing {

class BackupManager {
public:
    static BackupManager& GetInstance() {
        static BackupManager instance;
        return instance;
    }

    // Tạo backup của system HWID hiện tại
    bool CreateBackup(const std::string& backupName = "");

    // Restore từ backup
    bool RestoreBackup(const std::string& backupName);

    // List tất cả backups
    std::vector<std::string> GetBackupList() const;

    // Delete backup
    bool DeleteBackup(const std::string& backupName);

    // Auto backup trước khi spoof
    bool AutoBackup();

    // Lấy latest backup
    std::string GetLatestBackup() const;

private:
    BackupManager() = default;
    ~BackupManager() = default;

    BackupManager(const BackupManager&) = delete;
    BackupManager& operator=(const BackupManager&) = delete;

    std::string GetBackupDir() const;
    std::string GetBackupFilePath(const std::string& backupName) const;
    int GetBackupCount() const;
};

} // namespace UchihaSpoofer::Spoofing
