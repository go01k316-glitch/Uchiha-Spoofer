#pragma execution_character_set("utf-8")
#include "FileUtils.h"
#include <windows.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace UchihaSpoofer::Utils {

bool FileUtils::FileExists(const std::string& path) {
    try {
        return fs::exists(path) && fs::is_regular_file(path);
    }
    catch (...) {
        return false;
    }
}

bool FileUtils::DirectoryExists(const std::string& path) {
    try {
        return fs::exists(path) && fs::is_directory(path);
    }
    catch (...) {
        return false;
    }
}

bool FileUtils::CreateDirectory(const std::string& path) {
    try {
        if (!DirectoryExists(path)) {
            fs::create_directories(path);
        }
        return true;
    }
    catch (const std::exception& e) {
        throw Core::UchihaException(Core::ErrorCode::DirectoryCreationFailed,
            std::string("Path: ") + path + " - " + e.what());
    }
}

bool FileUtils::DeleteFile(const std::string& path) {
    try {
        if (FileExists(path)) {
            fs::remove(path);
        }
        return true;
    }
    catch (const std::exception& e) {
        throw Core::UchihaException(Core::ErrorCode::FileOpenFailed,
            "Cannot delete file: " + path);
    }
}

bool FileUtils::DeleteDirectory(const std::string& path) {
    try {
        if (DirectoryExists(path)) {
            fs::remove_all(path);
        }
        return true;
    }
    catch (...) {
        return false;
    }
}

std::string FileUtils::ReadFileContent(const std::string& path) {
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            throw Core::UchihaException(Core::ErrorCode::FileOpenFailed,
                "Cannot open: " + path);
        }
        std::string content((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
        file.close();
        return content;
    }
    catch (const Core::UchihaException&) {
        throw;
    }
    catch (const std::exception& e) {
        throw Core::UchihaException(Core::ErrorCode::FileReadFailed,
            std::string(e.what()));
    }
}

void FileUtils::WriteFileContent(const std::string& path, const std::string& content) {
    try {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            throw Core::UchihaException(Core::ErrorCode::FileOpenFailed,
                "Cannot open: " + path);
        }
        file.write(content.c_str(), content.size());
        file.close();
    }
    catch (const Core::UchihaException&) {
        throw;
    }
    catch (const std::exception& e) {
        throw Core::UchihaException(Core::ErrorCode::FileWriteFailed,
            std::string(e.what()));
    }
}

void FileUtils::CleanDirectoryRecursive(const std::string& path) {
    try {
        if (DirectoryExists(path)) {
            for (const auto& entry : fs::directory_iterator(path)) {
                try {
                    if (fs::is_directory(entry)) {
                        fs::remove_all(entry.path());
                    }
                    else {
                        fs::remove(entry.path());
                    }
                }
                catch (...) {
                    // Bỏ qua file không thể xóa
                    continue;
                }
            }
        }
    }
    catch (...) {
        // Bỏ qua lỗi
    }
}

uint64_t FileUtils::GetFileSize(const std::string& path) {
    try {
        if (FileExists(path)) {
            return fs::file_size(path);
        }
        return 0;
    }
    catch (...) {
        return 0;
    }
}

} // namespace UchihaSpoofer::Utils
