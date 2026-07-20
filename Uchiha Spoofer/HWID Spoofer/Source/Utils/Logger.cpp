#include "Logger.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>

namespace UchihaSpoofer::Utils {

void Logger::Log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string formatted = FormatLogMessage(level, message);
    logs_.push_back(formatted);

    if (callback_) {
        callback_(level, formatted);
    }
}

std::vector<std::string> Logger::GetAllLogs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return logs_;
}

std::vector<std::string> Logger::GetRecentLogs(size_t count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> recent;
    if (logs_.empty()) return recent;

    size_t start = (logs_.size() > count) ? logs_.size() - count : 0;
    recent.insert(recent.end(), logs_.begin() + start, logs_.end());
    return recent;
}

void Logger::ClearLogs() {
    std::lock_guard<std::mutex> lock(mutex_);
    logs_.clear();
}

void Logger::SaveToFile(const std::string& filepath) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream file(filepath, std::ios::app);
    if (!file.is_open()) return;

    for (const auto& log : logs_) {
        file << log << "\n";
    }
    file.close();
}

std::string Logger::FormatLogMessage(LogLevel level, const std::string& message) const {
    // Lấy thời gian hiện tại
    auto now = std::time(nullptr);
    tm ltm;
    localtime_s(&ltm, &now);

    std::ostringstream oss;
    oss << "[" << std::setfill('0')
        << std::setw(2) << ltm.tm_hour << ":"
        << std::setw(2) << ltm.tm_min << ":"
        << std::setw(2) << ltm.tm_sec << "] ";

    // Thêm level prefix
    switch (level) {
        case LogLevel::Debug: oss << "[DEBUG] "; break;
        case LogLevel::Info: oss << "[INFO] "; break;
        case LogLevel::Warning: oss << "[WARN] "; break;
        case LogLevel::Error: oss << "[ERROR] "; break;
        case LogLevel::Critical: oss << "[CRITICAL] "; break;
    }

    oss << message;
    return oss.str();
}

} // namespace UchihaSpoofer::Utils
