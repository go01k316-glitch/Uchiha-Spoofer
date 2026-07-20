#pragma once

// ============================================
// VERSION INFORMATION
// ============================================
#define APP_VERSION_MAJOR 3
#define APP_VERSION_MINOR 1
#define APP_VERSION_PATCH 0

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define APP_VERSION_STR TOSTRING(APP_VERSION_MAJOR) "." TOSTRING(APP_VERSION_MINOR) "." TOSTRING(APP_VERSION_PATCH)
#define APP_BUILD_TIMESTAMP __DATE__ " " __TIME__
#define APP_BUILD_CONFIG
#ifdef _DEBUG
    #define APP_BUILD_CONFIG "Debug"
#else
    #define APP_BUILD_CONFIG "Release"
#endif

// ============================================
// FEATURE FLAGS
// ============================================
#define ENABLE_LOGGING_TO_FILE 1
#define ENABLE_BACKUP_SYSTEM 1
#define ENABLE_ADMIN_CHECK 1
#define ENABLE_COMMAND_WHITELIST 1
#define ENABLE_REGISTRY_VALIDATION 1
#define ENABLE_THREAD_POOL 1

namespace Uchiha {
    class VersionInfo {
    public:
        static const char* GetVersionString() {
            return APP_VERSION_STR;
        }

        static const char* GetBuildInfo() {
            return "Build: " APP_BUILD_CONFIG " | " APP_BUILD_TIMESTAMP;
        }

        static int GetMajorVersion() { return APP_VERSION_MAJOR; }
        static int GetMinorVersion() { return APP_VERSION_MINOR; }
        static int GetPatchVersion() { return APP_VERSION_PATCH; }
    };
}
