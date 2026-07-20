#pragma execution_character_set("utf-8")
#include "ConfigManager.h"
#include "../Utils/Logger.h"
#include "../Utils/FileUtils.h"
#include <fstream>
#include <filesystem>
#include <ctime>

namespace fs = std::filesystem;

namespace UchihaSpoofer::Spoofing {

json SpoofProfile::ToJson() const {
    json j;
    j["name"] = name;
    j["timestamp"] = timestamp;
    j["identity"]["macAddress"] = identity.macAddress;
    j["identity"]["registryGuid"] = identity.registryGuid;
    j["identity"]["hwProfileGuid"] = identity.hwProfileGuid;
    j["identity"]["pcName"] = identity.pcName;
    j["identity"]["productId"] = identity.productId;
    j["identity"]["volCSerialStr"] = identity.volCSerialStr;
    j["identity"]["volDSerialStr"] = identity.volDSerialStr;
    j["config"]["selectMAC"] = config.selectMAC;
    j["config"]["selectGUID"] = config.selectGUID;
    j["config"]["selectHWProfile"] = config.selectHWProfile;
    j["config"]["selectPCName"] = config.selectPCName;
    j["config"]["selectProductID"] = config.selectProductID;
    j["config"]["selectVolC"] = config.selectVolC;
    j["config"]["selectVolD"] = config.selectVolD;
    j["config"]["selectCPU"] = config.selectCPU;
    j["config"]["selectBIOS"] = config.selectBIOS;
    return j;
}

SpoofProfile SpoofProfile::FromJson(const json& j) {
    SpoofProfile profile;
    profile.name = j.value("name", "Unknown");
    profile.timestamp = j.value("timestamp", "");
    
    if (j.contains("identity")) {
        const auto& id = j["identity"];
        profile.identity.macAddress = id.value("macAddress", "");
        profile.identity.registryGuid = id.value("registryGuid", "");
        profile.identity.hwProfileGuid = id.value("hwProfileGuid", "");
        profile.identity.pcName = id.value("pcName", "");
        profile.identity.productId = id.value("productId", "");
        profile.identity.volCSerialStr = id.value("volCSerialStr", "");
        profile.identity.volDSerialStr = id.value("volDSerialStr", "");
    }

    if (j.contains("config")) {
        const auto& cfg = j["config"];
        profile.config.selectMAC = cfg.value("selectMAC", true);
        profile.config.selectGUID = cfg.value("selectGUID", true);
        profile.config.selectHWProfile = cfg.value("selectHWProfile", true);
        profile.config.selectPCName = cfg.value("selectPCName", true);
        profile.config.selectProductID = cfg.value("selectProductID", true);
        profile.config.selectVolC = cfg.value("selectVolC", true);
        profile.config.selectVolD = cfg.value("selectVolD", true);
        profile.config.selectCPU = cfg.value("selectCPU", false);
        profile.config.selectBIOS = cfg.value("selectBIOS", false);
    }

    return profile;
}

bool ConfigManager::SaveProfile(const std::string& profileName, const SpoofProfile& profile) {
    try {
        SpoofProfile p = profile;
        p.name = profileName;
        
        // Lấy timestamp hiện tại
        auto now = std::time(nullptr);
        tm ltm;
        localtime_s(&ltm, &now);
        char timeStr[100];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &ltm);
        p.timestamp = timeStr;

        profiles_.push_back(p);
        SaveAllProfiles();
        
        UCHIHA_LOG_INFO("[Config] Đã lưu profile: " + profileName);
        return true;
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_ERROR(std::string("[Config] Save error: ") + e.what());
        return false;
    }
}

bool ConfigManager::LoadProfile(const std::string& profileName, SpoofProfile& profile) {
    try {
        for (const auto& p : profiles_) {
            if (p.name == profileName) {
                profile = p;
                UCHIHA_LOG_INFO("[Config] Đã tải profile: " + profileName);
                return true;
            }
        }
        UCHIHA_LOG_WARNING("[Config] Profile không tìm thấy: " + profileName);
        return false;
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_ERROR(std::string("[Config] Load error: ") + e.what());
        return false;
    }
}

bool ConfigManager::DeleteProfile(const std::string& profileName) {
    try {
        auto it = std::find_if(profiles_.begin(), profiles_.end(),
            [&](const SpoofProfile& p) { return p.name == profileName; });
        
        if (it != profiles_.end()) {
            profiles_.erase(it);
            SaveAllProfiles();
            UCHIHA_LOG_INFO("[Config] Đã xóa profile: " + profileName);
            return true;
        }
        return false;
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_ERROR(std::string("[Config] Delete error: ") + e.what());
        return false;
    }
}

std::vector<std::string> ConfigManager::GetProfileList() const {
    std::vector<std::string> names;
    for (const auto& p : profiles_) {
        names.push_back(p.name);
    }
    return names;
}

void ConfigManager::SaveAllProfiles() {
    try {
        json profilesJson = json::array();
        for (const auto& p : profiles_) {
            profilesJson.push_back(p.ToJson());
        }

        std::string configPath = GetConfigFilePath();
        std::ofstream file(configPath);
        if (file.is_open()) {
            file << profilesJson.dump(4);
            file.close();
        }
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_ERROR(std::string("[Config] SaveAll error: ") + e.what());
    }
}

void ConfigManager::LoadAllProfiles() {
    try {
        std::string configPath = GetConfigFilePath();
        if (!fs::exists(configPath)) {
            UCHIHA_LOG_INFO("[Config] Config file chưa tồn tại, khởi tạo mới");
            return;
        }

        std::ifstream file(configPath);
        if (!file.is_open()) return;

        json profilesJson;
        file >> profilesJson;
        file.close();

        profiles_.clear();
        if (profilesJson.is_array()) {
            for (const auto& j : profilesJson) {
                profiles_.push_back(SpoofProfile::FromJson(j));
            }
        }

        UCHIHA_LOG_INFO("[Config] Đã tải " + std::to_string(profiles_.size()) + " profiles");
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_ERROR(std::string("[Config] LoadAll error: ") + e.what());
    }
}

std::string ConfigManager::GetConfigFilePath() const {
    return "uchiha_profiles.json";
}

std::string ConfigManager::GetProfilePath() const {
    return "profiles/";
}

} // namespace UchihaSpoofer::Spoofing
