#pragma once

#include <string>
#include <vector>
#include <windows.h>

namespace Uchiha {
namespace Security {

// ============================================
// ADMIN PRIVILEGE CHECK
// ============================================
bool IsUserAdmin() {
    BOOL isAdmin = FALSE;
    HANDLE hToken = NULL;
    
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        
        if (GetTokenInformation(hToken, TokenElevation, &elevation, cbSize, &cbSize)) {
            isAdmin = elevation.TokenIsElevated;
        }
        
        CloseHandle(hToken);
    }
    
    return isAdmin ? true : false;
}

// ============================================
// COMMAND WHITELIST & VALIDATION
// ============================================
struct CommandValidator {
    static const std::vector<std::string> ALLOWED_COMMANDS;
    
    static bool IsCommandAllowed(const std::string& command) {
        for (const auto& allowed : ALLOWED_COMMANDS) {
            if (command.find(allowed) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
    
    static bool ExecuteSafeCommand(const std::string& command) {
        if (!IsCommandAllowed(command)) {
            return false;  // Command not in whitelist
        }
        
        STARTUPINFOA si = { sizeof(STARTUPINFOA) };
        PROCESS_INFORMATION pi = {};
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        
        std::vector<char> cmdBuffer(command.begin(), command.end());
        cmdBuffer.push_back('\0');
        
        if (CreateProcessA(NULL, cmdBuffer.data(), NULL, NULL, FALSE, 
                          CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return true;
        }
        
        return false;
    }
};

const std::vector<std::string> CommandValidator::ALLOWED_COMMANDS = {
    "powershell.exe /WindowStyle Hidden /Command",
    "ipconfig.exe /flushdns",
    "ipconfig.exe /release",
    "ipconfig.exe /renew",
    "netsh.exe winsock reset",
    "netsh.exe int ip reset",
    "arp.exe -d",
    "reg.exe delete",
    "Get-EventLog",
    "Clear-EventLog",
    "Get-NetAdapter",
    "Restart-NetAdapter"
};

// ============================================
// REGISTRY PATH VALIDATION
// ============================================
struct RegistryPathValidator {
    static const std::vector<std::string> ALLOWED_REGISTRY_PATHS;
    
    static bool IsPathAllowed(const std::string& path) {
        for (const auto& allowed : ALLOWED_REGISTRY_PATHS) {
            if (path == allowed) {
                return true;
            }
        }
        return false;
    }
};

const std::vector<std::string> RegistryPathValidator::ALLOWED_REGISTRY_PATHS = {
    "SOFTWARE\\Microsoft\\Cryptography",
    "SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\0001",
    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
    "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}"
};

} // namespace Security
} // namespace Uchiha
