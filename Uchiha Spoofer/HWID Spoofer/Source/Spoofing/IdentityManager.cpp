#pragma execution_character_set("utf-8")
#include "IdentityManager.h"
#include "../Utils/SystemUtils.h"
#include "../Utils/StringUtils.h"
#include "../Utils/Logger.h"
#include <fstream>
#include <sstream>

namespace UchihaSpoofer::Spoofing {

void IdentityManager::LoadOriginalIdentity() {
    try {
        UCHIHA_LOG_INFO("[Identity] Đang tải danh tính gốc từ hệ thống...");

        originalIdentity_.macAddress = Utils::SystemUtils::GetMACAddress();
        originalIdentity_.registryGuid = Utils::SystemUtils::GetMachineGUID();
        originalIdentity_.hwProfileGuid = Utils::SystemUtils::GetHWProfileGUID();
        originalIdentity_.pcName = Utils::SystemUtils::GetComputerName();
        originalIdentity_.productId = Utils::SystemUtils::GetProductID();
        originalIdentity_.volC_Raw = Utils::SystemUtils::GetVolumeRawSerial('C');
        originalIdentity_.volD_Raw = Utils::SystemUtils::GetVolumeRawSerial('D');
        originalIdentity_.volCSerialStr = Utils::SystemUtils::GetVolumeSerial('C');
        originalIdentity_.volDSerialStr = Utils::SystemUtils::GetVolumeSerial('D');

        UCHIHA_LOG_INFO("[Identity] Danh tính gốc đã tải thành công");
    }
    catch (const Core::UchihaException& e) {
        UCHIHA_LOG_ERROR(std::string("[Identity] ") + e.what());
    }
}

void IdentityManager::GenerateFakeIdentity() {
    UCHIHA_LOG_INFO("[Identity] Tạo danh tính giả ngẫu nhiên...");

    fakeIdentity_.macAddress = Utils::StringUtils::GenerateRandomMAC();
    fakeIdentity_.registryGuid = Utils::StringUtils::GenerateRandomGUID();
    fakeIdentity_.hwProfileGuid = Utils::StringUtils::GenerateRandomGUID_Bracketed();
    fakeIdentity_.pcName = Utils::StringUtils::GenerateRandomPCName();
    fakeIdentity_.productId = Utils::StringUtils::GenerateRandomProductID();
    fakeIdentity_.volC_Raw = Utils::StringUtils::GenerateRandomVolumeID();
    fakeIdentity_.volD_Raw = Utils::StringUtils::GenerateRandomVolumeID();
    fakeIdentity_.volCSerialStr = Utils::StringUtils::FormatVolumeSerial(fakeIdentity_.volC_Raw);
    fakeIdentity_.volDSerialStr = Utils::StringUtils::FormatVolumeSerial(fakeIdentity_.volD_Raw);

    UCHIHA_LOG_INFO("[Identity] Danh tính giả đã tạo thành công");
}

void IdentityManager::UpdateFakeIdentity(const Core::Identity& identity) {
    fakeIdentity_ = identity;
    UCHIHA_LOG_INFO("[Identity] Danh tính giả đã cập nhật");
}

void IdentityManager::SetCustomIdentity(const Core::Identity& custom) {
    fakeIdentity_ = custom;
    UCHIHA_LOG_INFO("[Identity] Đã set custom identity");
}

bool IdentityManager::ValidateIdentity(const Core::Identity& identity) const {
    // Validate MAC address format (XX:XX:XX:XX:XX:XX)
    if (identity.macAddress.length() != 17) {
        UCHIHA_LOG_ERROR("[Identity] Invalid MAC format");
        return false;
    }

    // Validate GUID format
    if (identity.registryGuid.length() < 36) {
        UCHIHA_LOG_ERROR("[Identity] Invalid GUID format");
        return false;
    }

    // Validate PC name (not empty)
    if (identity.pcName.empty() || identity.pcName.length() > 15) {
        UCHIHA_LOG_ERROR("[Identity] Invalid PC name");
        return false;
    }

    return true;
}

void IdentityManager::SaveConfiguration() {
    try {
        std::ofstream file("config.ini");
        if (!file.is_open()) {
            throw Core::UchihaException(Core::ErrorCode::FileWriteFailed,
                "Cannot open config.ini for writing");
        }

        file << "# Uchiha Spoofer Configuration\n";
        file << "LastMAC=" << fakeIdentity_.macAddress << "\n";
        file << "LastGUID=" << fakeIdentity_.registryGuid << "\n";
        file << "LastPCName=" << fakeIdentity_.pcName << "\n";
        file.close();

        UCHIHA_LOG_INFO("[Config] Đã lưu cấu hình");
    }
    catch (const Core::UchihaException& e) {
        UCHIHA_LOG_ERROR(std::string("[Config] ") + e.what());
    }
}

void IdentityManager::LoadConfiguration() {
    try {
        std::ifstream file("config.ini");
        if (!file.is_open()) {
            UCHIHA_LOG_INFO("[Config] Config file không tồn tại, sử dụng mặc định");
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            // Parse config lines if needed
        }

        file.close();
        UCHIHA_LOG_INFO("[Config] Đã tải cấu hình");
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_WARNING(std::string("[Config] Load error: ") + e.what());
    }
}

} // namespace UchihaSpoofer::Spoofing
