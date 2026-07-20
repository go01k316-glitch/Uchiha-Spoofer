#pragma execution_character_set("utf-8")
#include "DriverManager.h"
#include "DriverIOControl.h"
#include "../Utils/Logger.h"
#include "../Utils/SystemUtils.h"
#include "driver.h"
#include <windows.h>
#include <fstream>

namespace UchihaSpoofer::Driver {

bool DriverManager::Initialize() {
    try {
        UCHIHA_LOG_INFO("[Driver] Khởi tạo Driver Manager...");

        if (!ExtractDriver()) {
            UCHIHA_LOG_ERROR("[Driver] Không thể giải nén file driver");
            return false;
        }

        if (!LoadDriver()) {
            UCHIHA_LOG_ERROR("[Driver] Không thể nạp driver vào kernel");
            return false;
        }

        isLoaded_ = true;
        UCHIHA_LOG_INFO("[Driver] Driver đã nạp thành công!");
        return true;
    }
    catch (const Core::UchihaException& e) {
        UCHIHA_LOG_ERROR(std::string(e.what()));
        return false;
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_ERROR(std::string("[Driver] Exception: ") + e.what());
        return false;
    }
}

bool DriverManager::ExtractDriver() {
    try {
        driverPath_ = GetDriverPath();
        std::ofstream os(driverPath_, std::ios::binary);

        if (!os.is_open()) {
            throw Core::UchihaException(Core::ErrorCode::DriverExtractionFailed,
                "Cannot create driver file: " + driverPath_);
        }

        os.write((char*)driver_bytes, driver_size);
        os.close();

        return true;
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_ERROR(std::string("[Driver] Extract failed: ") + e.what());
        return false;
    }
}

bool DriverManager::LoadDriver() {
    try {
        SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (!hSCM) {
            throw Core::UchihaException(Core::ErrorCode::DriverServiceCreationFailed,
                "OpenSCManager failed");
        }

        SC_HANDLE hService = CreateServiceA(
            hSCM, DRIVER_SERVICE_NAME, DRIVER_SERVICE_NAME,
            SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
            SERVICE_ERROR_NORMAL, driverPath_.c_str(),
            NULL, NULL, NULL, NULL, NULL);

        if (!hService) {
            if (GetLastError() == ERROR_SERVICE_EXISTS) {
                hService = OpenServiceA(hSCM, DRIVER_SERVICE_NAME, SERVICE_ALL_ACCESS);
                if (!hService) {
                    CloseServiceHandle(hSCM);
                    throw Core::UchihaException(Core::ErrorCode::DriverServiceCreationFailed,
                        "Cannot open existing service");
                }
            }
            else {
                CloseServiceHandle(hSCM);
                throw Core::UchihaException(Core::ErrorCode::DriverServiceCreationFailed,
                    "CreateService failed");
            }
        }

        if (!StartService(hService, 0, NULL)) {
            if (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING) {
                CloseServiceHandle(hService);
                CloseServiceHandle(hSCM);
                throw Core::UchihaException(Core::ErrorCode::DriverServiceStartFailed,
                    "StartService failed");
            }
        }

        CloseServiceHandle(hService);
        CloseServiceHandle(hSCM);
        return true;
    }
    catch (const Core::UchihaException& e) {
        UCHIHA_LOG_ERROR(std::string(e.what()));
        return false;
    }
}

bool DriverManager::UnloadDriver() {
    try {
        SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (!hSCM) {
            return false;
        }

        SC_HANDLE hService = OpenServiceA(hSCM, DRIVER_SERVICE_NAME, SERVICE_ALL_ACCESS);
        if (hService) {
            SERVICE_STATUS status;
            ControlService(hService, SERVICE_CONTROL_STOP, &status);
            DeleteService(hService);
            CloseServiceHandle(hService);
        }

        CloseServiceHandle(hSCM);

        // Xóa file driver tạm
        DeleteFileA(driverPath_.c_str());
        return true;
    }
    catch (...) {
        return false;
    }
}

bool DriverManager::SendIOControl(unsigned int ioctlCode) {
    try {
        HANDLE hDriver = CreateFileA(DRIVER_DEVICE_NAME,
            GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, NULL);

        if (hDriver == INVALID_HANDLE_VALUE) {
            throw Core::UchihaException(Core::ErrorCode::DriverCommunicationFailed,
                "Cannot open driver device");
        }

        DWORD bytesReturned = 0;
        BOOL status = DeviceIoControl(hDriver, ioctlCode, NULL, 0, NULL, 0,
            &bytesReturned, NULL);

        CloseHandle(hDriver);

        if (!status) {
            throw Core::UchihaException(Core::ErrorCode::DriverIOControlFailed,
                "DeviceIoControl failed");
        }

        return true;
    }
    catch (const Core::UchihaException& e) {
        UCHIHA_LOG_ERROR(std::string(e.what()));
        return false;
    }
}

bool DriverManager::SpoofCPU() {
    if (!isLoaded_) {
        UCHIHA_LOG_ERROR("[Driver] Driver chưa được nạp");
        return false;
    }
    return SendIOControl(IOCTL_SPOOF_CPU);
}

bool DriverManager::SpoofBIOS() {
    if (!isLoaded_) {
        UCHIHA_LOG_ERROR("[Driver] Driver chưa được nạp");
        return false;
    }
    return SendIOControl(IOCTL_SPOOF_BIOS);
}

void DriverManager::Shutdown() {
    try {
        UnloadDriver();
        isLoaded_ = false;
        UCHIHA_LOG_INFO("[Driver] Driver đã hủy thành công");
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_ERROR(std::string("[Driver] Shutdown error: ") + e.what());
    }
}

std::string DriverManager::GetDriverPath() const {
    return Utils::SystemUtils::GetCurrentExecutablePath() + "\\" + DRIVER_TEMP_FILE;
}

} // namespace UchihaSpoofer::Driver
