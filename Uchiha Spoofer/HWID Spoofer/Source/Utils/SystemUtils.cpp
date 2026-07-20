#pragma execution_character_set("utf-8")
#include "SystemUtils.h"
#include "Logger.h"
#include <windows.h>
#include <iphlpapi.h>
#include <winioctl.h>
#include <process.h>

pragma comment(lib, "IPHLPAPI.lib")
pragma comment(lib, "Ws2_32.lib")

namespace UchihaSpoofer::Utils {

std::string SystemUtils::ReadRegistry(const std::string& path, const std::string& key) {
    HKEY hKey;
    char val[256] = {0};
    DWORD sz = sizeof(val);

    LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        throw Core::UchihaException(Core::ErrorCode::RegistryOpenFailed,
            "Path: " + path);
    }

    result = RegQueryValueExA(hKey, key.c_str(), NULL, NULL, (LPBYTE)val, &sz);
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) {
        throw Core::UchihaException(Core::ErrorCode::RegistryReadFailed,
            "Key: " + key);
    }

    return std::string(val);
}

void SystemUtils::WriteRegistry(const std::string& path, const std::string& key, const std::string& value) {
    HKEY hKey;
    LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_SET_VALUE, &hKey);

    if (result != ERROR_SUCCESS) {
        throw Core::UchihaException(Core::ErrorCode::RegistryOpenFailed,
            "Path: " + path);
    }

    result = RegSetValueExA(hKey, key.c_str(), 0, REG_SZ,
        (const BYTE*)value.c_str(), (DWORD)(value.length() + 1));
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) {
        throw Core::UchihaException(Core::ErrorCode::RegistryWriteFailed,
            "Key: " + key);
    }
}

void SystemUtils::DeleteRegistry(const std::string& path, const std::string& key) {
    HKEY hKey;
    LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_SET_VALUE, &hKey);

    if (result != ERROR_SUCCESS) {
        throw Core::UchihaException(Core::ErrorCode::RegistryOpenFailed,
            "Path: " + path);
    }

    result = RegDeleteValueA(hKey, key.c_str());
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) {
        throw Core::UchihaException(Core::ErrorCode::RegistryDeleteFailed,
            "Key: " + key);
    }
}

std::string SystemUtils::GetMachineGUID() {
    return ReadRegistry(
        "SOFTWARE\\Microsoft\\Cryptography",
        "MachineGuid");
}

std::string SystemUtils::GetHWProfileGUID() {
    return ReadRegistry(
        "SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\0001",
        "HwProfileGuid");
}

std::string SystemUtils::GetComputerName() {
    char buf[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD sz = sizeof(buf);
    if (!GetComputerNameA(buf, &sz)) {
        throw Core::UchihaException(Core::ErrorCode::InvalidParameter,
            "Failed to get computer name");
    }
    return std::string(buf);
}

std::string SystemUtils::GetProductID() {
    return ReadRegistry(
        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        "ProductId");
}

void SystemUtils::SetComputerName(const std::string& newName) {
    if (!SetComputerNameExA(ComputerNamePhysicalDnsHostname, newName.c_str())) {
        throw Core::UchihaException(Core::ErrorCode::ComputerNameChangeFailed,
            "NewName: " + newName);
    }
}

std::string SystemUtils::GetMACAddress() {
    ULONG outBufLen = sizeof(IP_ADAPTER_ADDRESSES);
    PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(outBufLen);

    if (!pAddresses) {
        throw Core::UchihaException(Core::ErrorCode::MemoryAllocationFailed);
    }

    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAddresses);
        pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(outBufLen);
        if (!pAddresses) {
            throw Core::UchihaException(Core::ErrorCode::MemoryAllocationFailed);
        }
    }

    std::string macStr = "00:1A:2B:3C:4D:5E";

    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &outBufLen) == NO_ERROR) {
        PIP_ADAPTER_ADDRESSES curr = pAddresses;
        while (curr) {
            if ((curr->IfType == IF_TYPE_ETHERNET_CSMACD || curr->IfType == IF_TYPE_IEEE80211) &&
                curr->OperStatus == IfOperStatusUp) {
                if (curr->PhysicalAddressLength > 0) {
                    char buf[30];
                    sprintf_s(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
                        curr->PhysicalAddress[0], curr->PhysicalAddress[1], curr->PhysicalAddress[2],
                        curr->PhysicalAddress[3], curr->PhysicalAddress[4], curr->PhysicalAddress[5]);
                    macStr = buf;
                    break;
                }
            }
            curr = curr->Next;
        }
    }

    free(pAddresses);
    return macStr;
}

void SystemUtils::RestartNetworkAdapters() {
    ExecuteSilentCommand("powershell.exe -WindowStyle Hidden -Command \"Get-NetAdapter | Restart-NetAdapter -Confirm:$false\"");
}

std::string SystemUtils::GetVolumeSerial(char drive) {
    char root[4] = {drive, ':', '\\', '\0'};
    DWORD serial = 0;
    if (!GetVolumeInformationA(root, NULL, 0, &serial, NULL, NULL, NULL, 0)) {
        throw Core::UchihaException(Core::ErrorCode::VolumeSerialReadFailed);
    }
    char buf[30];
    sprintf_s(buf, "%04X-%04X", HIWORD(serial), LOWORD(serial));
    return std::string(buf);
}

uint32_t SystemUtils::GetVolumeRawSerial(char drive) {
    char root[4] = {drive, ':', '\\', '\0'};
    DWORD serial = 0;
    GetVolumeInformationA(root, NULL, 0, &serial, NULL, NULL, NULL, 0);
    return serial;
}

bool SystemUtils::WriteVolumeSerial(char drive, uint32_t newSerial) {
    char szDrive[7];
    sprintf_s(szDrive, "\\\\.\\%c:", drive);

    HANDLE hVol = CreateFileA(szDrive, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (hVol == INVALID_HANDLE_VALUE) {
        throw Core::UchihaException(Core::ErrorCode::VolumeLockFailed);
    }

    DWORD bytesReturned;
    if (!DeviceIoControl(hVol, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL)) {
        CloseHandle(hVol);
        throw Core::UchihaException(Core::ErrorCode::VolumeLockFailed);
    }

    DeviceIoControl(hVol, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);

    BYTE sector[512];
    DWORD bytesRead, bytesWritten;
    if (!ReadFile(hVol, sector, 512, &bytesRead, NULL)) {
        DeviceIoControl(hVol, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
        CloseHandle(hVol);
        throw Core::UchihaException(Core::ErrorCode::VolumeReadFailed);
    }

    if (memcmp(sector + 3, "NTFS", 4) == 0) {
        *(DWORD*)(sector + 0x48) = newSerial;
        *(DWORD*)(sector + 0x4C) = newSerial ^ 0x55555555;
    } else {
        *(DWORD*)(sector + 0x43) = newSerial;
    }

    SetFilePointer(hVol, 0, NULL, FILE_BEGIN);
    BOOL success = WriteFile(hVol, sector, 512, &bytesWritten, NULL);

    DeviceIoControl(hVol, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
    CloseHandle(hVol);

    if (!success) {
        throw Core::UchihaException(Core::ErrorCode::VolumeWriteFailed);
    }

    return true;
}

void SystemUtils::ExecuteSilentCommand(const std::string& command) {
    STARTUPINFOA si = {sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    std::vector<char> cmdBuffer(command.begin(), command.end());
    cmdBuffer.push_back('\0');

    if (CreateProcessA(NULL, cmdBuffer.data(), NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        throw Core::UchihaException(Core::ErrorCode::InvalidParameter,
            "Failed to execute: " + command);
    }
}

void SystemUtils::ExecuteCommand(const std::string& command, bool showWindow) {
    STARTUPINFOA si = {sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = showWindow ? SW_SHOW : SW_HIDE;

    std::vector<char> cmdBuffer(command.begin(), command.end());
    cmdBuffer.push_back('\0');

    if (!CreateProcessA(NULL, cmdBuffer.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        throw Core::UchihaException(Core::ErrorCode::InvalidParameter,
            "Failed to execute: " + command);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

std::string SystemUtils::GetCurrentExecutablePath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    return std::string(buffer).substr(0, pos);
}

std::string SystemUtils::GetWindowsPath() {
    char winFolder[512];
    GetWindowsDirectoryA(winFolder, sizeof(winFolder));
    return std::string(winFolder);
}

} // namespace UchihaSpoofer::Utils
