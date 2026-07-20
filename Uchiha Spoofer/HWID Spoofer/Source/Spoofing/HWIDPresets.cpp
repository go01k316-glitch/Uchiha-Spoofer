#pragma execution_character_set("utf-8")
#include "HWIDPresets.h"
#include "../Utils/StringUtils.h"

namespace UchihaSpoofer::Spoofing {

Core::Identity HWIDPresets::GetPreset(const std::string& presetName) {
    if (presetName == "Random") return GenerateRandomPreset();
    if (presetName == "Vietnam") return GenerateVietnamPreset();
    if (presetName == "USA") return GenerateUSAPreset();
    if (presetName == "Europe") return GenerateEuropePreset();
    if (presetName == "Asia") return GenerateAsiaPreset();
    return GenerateRandomPreset();
}

std::vector<std::string> HWIDPresets::GetAvailablePresets() {
    return {"Random", "Vietnam", "USA", "Europe", "Asia"};
}

Core::Identity HWIDPresets::GenerateRandomPreset() {
    Core::Identity id;
    id.macAddress = Utils::StringUtils::GenerateRandomMAC();
    id.registryGuid = Utils::StringUtils::GenerateRandomGUID();
    id.hwProfileGuid = Utils::StringUtils::GenerateRandomGUID_Bracketed();
    id.pcName = Utils::StringUtils::GenerateRandomPCName();
    id.productId = Utils::StringUtils::GenerateRandomProductID();
    id.volC_Raw = Utils::StringUtils::GenerateRandomVolumeID();
    id.volD_Raw = Utils::StringUtils::GenerateRandomVolumeID();
    id.volCSerialStr = Utils::StringUtils::FormatVolumeSerial(id.volC_Raw);
    id.volDSerialStr = Utils::StringUtils::FormatVolumeSerial(id.volD_Raw);
    return id;
}

Core::Identity HWIDPresets::GenerateVietnamPreset() {
    // Prefix MAC cho Vietnam (Viettel, Mobifone, etc)
    Core::Identity id = GenerateRandomPreset();
    id.macAddress = "00:50:F2:" + id.macAddress.substr(8); // Viettel prefix
    id.pcName = "VIETNAM-" + Utils::StringUtils::GenerateRandomPCName().substr(7);
    return id;
}

Core::Identity HWIDPresets::GenerateUSAPreset() {
    // Prefix MAC cho USA (Intel, Broadcom, etc)
    Core::Identity id = GenerateRandomPreset();
    id.macAddress = "00:22:68:" + id.macAddress.substr(8); // Intel prefix
    id.pcName = "USA-" + Utils::StringUtils::GenerateRandomPCName().substr(7);
    return id;
}

Core::Identity HWIDPresets::GenerateEuropePreset() {
    // Prefix MAC cho Europe
    Core::Identity id = GenerateRandomPreset();
    id.macAddress = "00:1A:2B:" + id.macAddress.substr(8); // Generic prefix
    id.pcName = "EUROPE-" + Utils::StringUtils::GenerateRandomPCName().substr(7);
    return id;
}

Core::Identity HWIDPresets::GenerateAsiaPreset() {
    // Prefix MAC cho Asia
    Core::Identity id = GenerateRandomPreset();
    id.macAddress = "00:30:BD:" + id.macAddress.substr(8); // Asia prefix
    id.pcName = "ASIA-" + Utils::StringUtils::GenerateRandomPCName().substr(7);
    return id;
}

} // namespace UchihaSpoofer::Spoofing
