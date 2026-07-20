#pragma once

#include <string>
#include "../Core/Exception.h"

namespace UchihaSpoofer::Utils {

class FileUtils {
public:
    // Thao tác file
    static bool FileExists(const std::string& path);
    static bool DirectoryExists(const std::string& path);
    static bool CreateDirectory(const std::string& path);
    static bool DeleteFile(const std::string& path);
    static bool DeleteDirectory(const std::string& path);
    static std::string ReadFileContent(const std::string& path);
    static void WriteFileContent(const std::string& path, const std::string& content);

    // Xóa thư mục và nội dung bên trong
    static void CleanDirectoryRecursive(const std::string& path);

    // Lấy kích thước file
    static uint64_t GetFileSize(const std::string& path);
};

} // namespace UchihaSpoofer::Utils
