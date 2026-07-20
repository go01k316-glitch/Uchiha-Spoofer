#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "../Core/Exception.h"

namespace UchihaSpoofer::Utils {

class StringUtils {
public:
    // GUID & UUID
    static std::string GenerateRandomGUID();
    static std::string GenerateRandomGUID_Bracketed(); // {GUID}

    // MAC Address
    static std::string GenerateRandomMAC();
    static std::string CleanMACAddress(const std::string& mac);

    // PC Name
    static std::string GenerateRandomPCName();

    // Product ID
    static std::string GenerateRandomProductID();

    // Volume Serial
    static uint32_t GenerateRandomVolumeID();
    static std::string FormatVolumeSerial(uint32_t serial);

    // String utils
    static std::string ToUpper(const std::string& str);
    static std::string ToLower(const std::string& str);
    static std::vector<std::string> Split(const std::string& str, char delimiter);
    static std::string Join(const std::vector<std::string>& parts, const std::string& delimiter);
    static bool StartsWith(const std::string& str, const std::string& prefix);
    static bool EndsWith(const std::string& str, const std::string& suffix);
    static std::string Trim(const std::string& str);
};

} // namespace UchihaSpoofer::Utils
