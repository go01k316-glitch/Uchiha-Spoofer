#pragma once

#include <string>
#include "../Core/Types.h"

namespace UchihaSpoofer::Spoofing {

// HWID Presets
class HWIDPresets {
public:
    // Lấy preset theo tên
    static Core::Identity GetPreset(const std::string& presetName);
    
    // Danh sách available presets
    static std::vector<std::string> GetAvailablePresets();

private:
    // Preset functions
    static Core::Identity GenerateRandomPreset();
    static Core::Identity GenerateVietnamPreset();
    static Core::Identity GenerateUSAPreset();
    static Core::Identity GenerateEuropePreset();
    static Core::Identity GenerateAsiaPreset();
};

} // namespace UchihaSpoofer::Spoofing
