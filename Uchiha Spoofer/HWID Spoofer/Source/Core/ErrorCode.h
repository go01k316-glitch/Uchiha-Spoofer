#pragma once

#include <cstdint>
#include <string>

namespace UchihaSpoofer::Core {

// Mã lỗi centralized cho toàn bộ ứng dụng
enum class ErrorCode : uint32_t {
    // Success
    Success = 0x00000000,

    // System & Registry Errors (0x1xxx)
    RegistryOpenFailed = 0x10001,
    RegistryReadFailed = 0x10002,
    RegistryWriteFailed = 0x10003,
    RegistryDeleteFailed = 0x10004,
    ComputerNameChangeFailed = 0x10005,

    // Driver Errors (0x2xxx)
    DriverExtractionFailed = 0x20001,
    DriverLoadFailed = 0x20002,
    DriverServiceCreationFailed = 0x20003,
    DriverServiceStartFailed = 0x20004,
    DriverCommunicationFailed = 0x20005,
    DriverIOControlFailed = 0x20006,
    DriverNotLoaded = 0x20007,

    // Disk & Volume Errors (0x3xxx)
    VolumeAccessFailed = 0x30001,
    VolumeLockFailed = 0x30002,
    VolumeReadFailed = 0x30003,
    VolumeWriteFailed = 0x30004,
    VolumeSerialReadFailed = 0x30005,

    // Network Errors (0x4xxx)
    NetworkAdapterEnumerationFailed = 0x40001,
    NetworkAdapterRestartFailed = 0x40002,
    MACAddressRetrievalFailed = 0x40003,

    // File & IO Errors (0x5xxx)
    FileOpenFailed = 0x50001,
    FileReadFailed = 0x50002,
    FileWriteFailed = 0x50003,
    DirectoryCreationFailed = 0x50004,
    PathNotFound = 0x50005,

    // UI & Graphics Errors (0x6xxx)
    DirectXInitializationFailed = 0x60001,
    WindowCreationFailed = 0x60002,
    ImGuiInitializationFailed = 0x60003,
    RenderTargetCreationFailed = 0x60004,

    // General Errors (0x7xxx)
    MemoryAllocationFailed = 0x70001,
    InvalidParameter = 0x70002,
    OperationCancelled = 0x70003,
    UnknownError = 0x7FFFFFFF
};

// Chuyển mã lỗi thành string
inline std::string ErrorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::Success: return "Success";
        case ErrorCode::RegistryOpenFailed: return "Registry: Failed to open key";
        case ErrorCode::RegistryReadFailed: return "Registry: Failed to read value";
        case ErrorCode::RegistryWriteFailed: return "Registry: Failed to write value";
        case ErrorCode::DriverLoadFailed: return "Driver: Failed to load driver";
        case ErrorCode::DriverNotLoaded: return "Driver: Driver not loaded";
        case ErrorCode::VolumeLockFailed: return "Volume: Failed to lock volume";
        case ErrorCode::VolumeWriteFailed: return "Volume: Failed to write serial";
        case ErrorCode::FileOpenFailed: return "File: Failed to open file";
        case ErrorCode::InvalidParameter: return "Invalid parameter provided";
        default: return "Unknown error (0x" + std::to_string(static_cast<uint32_t>(code)) + ")";
    }
}

} // namespace UchihaSpoofer::Core
