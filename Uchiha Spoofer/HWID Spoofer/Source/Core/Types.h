#pragma once

#include <string>
#include <cstdint>

namespace UchihaSpoofer::Core {

// Định nghĩa kiểu dữ liệu chung cho toàn ứng d��ng
struct Identity {
    std::string macAddress;
    std::string registryGuid;
    std::string hwProfileGuid;
    std::string pcName;
    std::string productId;

    std::string volCSerialStr;
    std::string volDSerialStr;
    uint32_t volC_Raw;
    uint32_t volD_Raw;

    Identity() 
        : macAddress("00:1A:2B:3C:4D:5E")
        , registryGuid("")
        , hwProfileGuid("")
        , pcName("DESKTOP-UNKNOWN")
        , productId("")
        , volCSerialStr("XXXX-XXXX")
        , volDSerialStr("XXXX-XXXX")
        , volC_Raw(0)
        , volD_Raw(0) {}
};

// Cấu hình Spoofing
struct SpoofConfig {
    bool selectMAC = true;
    bool selectGUID = true;
    bool selectHWProfile = true;
    bool selectPCName = true;
    bool selectProductID = true;
    bool selectVolC = true;
    bool selectVolD = true;
    bool selectCPU = false;
    bool selectBIOS = false;
};

// Trạng thái tiến trình
struct OperationProgress {
    float progress = 0.0f;           // 0.0 - 1.0
    std::string currentTask = "";
    bool isRunning = false;
};

} // namespace UchihaSpoofer::Core
