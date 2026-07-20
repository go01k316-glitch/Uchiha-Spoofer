#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "../Core/Exception.h"

namespace UchihaSpoofer::Utils {

class SystemUtils {
public:
    // Registry Operations
    static std::string ReadRegistry(const std::string& path, const std::string& key);
    static void WriteRegistry(const std::string& path, const std::string& key, const std::string& value);
    static void DeleteRegistry(const std::string& path, const std::string& key);

    // Computer Info
    static std::string GetMachineGUID();
    static std::string GetHWProfileGUID();
    static std::string GetComputerName();
    static std::string GetProductID();
    static void SetComputerName(const std::string& newName);

    // Network
    static std::string GetMACAddress();
    static void RestartNetworkAdapters();

    // Disk
    static std::string GetVolumeSerial(char drive);
    static uint32_t GetVolumeRawSerial(char drive);
    static bool WriteVolumeSerial(char drive, uint32_t newSerial);

    // Process
    static void ExecuteSilentCommand(const std::string& command);
    static void ExecuteCommand(const std::string& command, bool showWindow = false);

    // Path
    static std::string GetCurrentExecutablePath();
    static std::string GetWindowsPath();
};

} // namespace UchihaSpoofer::Utils
