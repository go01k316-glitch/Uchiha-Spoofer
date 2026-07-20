#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "../Core/Types.h"

using json = nlohmann::json;

namespace UchihaSpoofer::Spoofing {

// Spoof profile structure
struct SpoofProfile {
    std::string name;
    Core::Identity identity;
    Core::SpoofConfig config;
    std::string timestamp;

    json ToJson() const;
    static SpoofProfile FromJson(const json& j);
};

// Config Manager - Save/Load profiles
class ConfigManager {
public:
    static ConfigManager& GetInstance() {
        static ConfigManager instance;
        return instance;
    }

    // Profile operations
    bool SaveProfile(const std::string& profileName, const SpoofProfile& profile);
    bool LoadProfile(const std::string& profileName, SpoofProfile& profile);
    bool DeleteProfile(const std::string& profileName);
    std::vector<std::string> GetProfileList() const;

    // Config file operations
    void SaveAllProfiles();
    void LoadAllProfiles();

    // Get profiles
    const std::vector<SpoofProfile>& GetProfiles() const { return profiles_; }

private:
    ConfigManager() = default;
    ~ConfigManager() = default;

    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    std::vector<SpoofProfile> profiles_;
    std::string GetProfilePath() const;
    std::string GetConfigFilePath() const;
};

} // namespace UchihaSpoofer::Spoofing
