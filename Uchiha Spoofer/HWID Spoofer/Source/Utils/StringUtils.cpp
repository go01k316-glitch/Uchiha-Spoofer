#pragma execution_character_set("utf-8")
#include "StringUtils.h"
#include <rpc.h>
#include <rpcdce.h>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <random>

pragma comment(lib, "Rpcrt4.lib")

namespace UchihaSpoofer::Utils {

std::string StringUtils::GenerateRandomGUID() {
    UUID uuid;
    UuidCreate(&uuid);
    char* str = nullptr;
    UuidToStringA(&uuid, reinterpret_cast<RPC_CSTR*>(&str));
    std::string result(str ? str : "");
    if (str) RpcStringFreeA(reinterpret_cast<RPC_CSTR*>(&str));
    return ToUpper(result);
}

std::string StringUtils::GenerateRandomGUID_Bracketed() {
    return "{" + GenerateRandomGUID() + "}";
}

std::string StringUtils::GenerateRandomMAC() {
    char buf[30];
    sprintf_s(buf, sizeof(buf), "00:50:56:%02X:%02X:%02X",
        rand() % 256, rand() % 256, rand() % 256);
    return std::string(buf);
}

std::string StringUtils::CleanMACAddress(const std::string& mac) {
    std::string clean;
    for (char c : mac) {
        if (c != ':' && c != '-') {
            clean += std::toupper(c);
        }
    }
    return clean;
}

std::string StringUtils::GenerateRandomPCName() {
    static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string name = "UCHIHA-";
    for (int i = 0; i < 7; i++) {
        name += chars[rand() % chars.length()];
    }
    return name;
}

std::string StringUtils::GenerateRandomProductID() {
    static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string id;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 5; j++) {
            id += chars[rand() % chars.length()];
        }
        if (i < 3) id += "-";
    }
    return id;
}

uint32_t StringUtils::GenerateRandomVolumeID() {
    return (uint32_t)(rand() ^ (rand() << 16));
}

std::string StringUtils::FormatVolumeSerial(uint32_t serial) {
    char buf[30];
    sprintf_s(buf, sizeof(buf), "%04X-%04X",
        (serial >> 16) & 0xFFFF, serial & 0xFFFF);
    return std::string(buf);
}

std::string StringUtils::ToUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::toupper(c); });
    return result;
}

std::string StringUtils::ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::vector<std::string> StringUtils::Split(const std::string& str, char delimiter) {
    std::vector<std::string> parts;
    std::string part;
    for (char c : str) {
        if (c == delimiter) {
            if (!part.empty()) {
                parts.push_back(part);
                part.clear();
            }
        } else {
            part += c;
        }
    }
    if (!part.empty()) {
        parts.push_back(part);
    }
    return parts;
}

std::string StringUtils::Join(const std::vector<std::string>& parts, const std::string& delimiter) {
    std::string result;
    for (size_t i = 0; i < parts.size(); i++) {
        result += parts[i];
        if (i < parts.size() - 1) {
            result += delimiter;
        }
    }
    return result;
}

bool StringUtils::StartsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() &&
        str.compare(0, prefix.size(), prefix) == 0;
}

bool StringUtils::EndsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
        str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string StringUtils::Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

} // namespace UchihaSpoofer::Utils
