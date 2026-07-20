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

void IdentityManager::SaveConfiguration() {
    try {
        std::ofstream file("config.ini");
        if (!file.is_open()) {
            throw Core::UchihaException(Core::ErrorCode::FileWriteFailed,
                "Cannot open config.ini for writing");
        }

        // Lưu cấu hình spoofing (từ UI state)
        // Đây là nơi bạn sẽ lưu các tùy chọn checkbox
        file << "# Uchiha Spoofer Configuration\n";
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
            // Config file chưa tồn tại, không cần throw error
            UCHIHA_LOG_INFO("[Config] Config file không tồn tại, sử dụng mặc định");
            return;
        }

        // Đọc cấu hình
        // Bạn sẽ parse dòng từ file ở đây

        file.close();
        UCHIHA_LOG_INFO("[Config] Đã tải cấu hình");
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_WARNING(std::string("[Config] Load error: ") + e.what());
    }
}

} // namespace UchihaSpoofer::Spoofing
