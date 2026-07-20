#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <functional>

namespace UchihaSpoofer::Utils {

class Logger {
public:
    enum class LogLevel {
        Debug,
        Info,
        Warning,
        Error,
        Critical
    };

    static Logger& GetInstance() {
        static Logger instance;
        return instance;
    }

    // Ghi log với mức độ
    void Log(LogLevel level, const std::string& message);
    void Debug(const std::string& message) { Log(LogLevel::Debug, message); }
    void Info(const std::string& message) { Log(LogLevel::Info, message); }
    void Warning(const std::string& message) { Log(LogLevel::Warning, message); }
    void Error(const std::string& message) { Log(LogLevel::Error, message); }
    void Critical(const std::string& message) { Log(LogLevel::Critical, message); }

    // Lấy toàn bộ logs
    std::vector<std::string> GetAllLogs() const;

    // Lấy logs gần đây nhất
    std::vector<std::string> GetRecentLogs(size_t count) const;

    // Xóa logs
    void ClearLogs();

    // Callback khi có log mới
    using LogCallback = std::function<void(LogLevel, const std::string&)>;
    void SetCallback(LogCallback callback) { callback_ = callback; }

    // Lưu logs vào file
    void SaveToFile(const std::string& filepath) const;

private:
    Logger() = default;
    ~Logger() = default;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    mutable std::mutex mutex_;
    std::vector<std::string> logs_;
    LogCallback callback_;

    std::string FormatLogMessage(LogLevel level, const std::string& message) const;
};

// Macro helpers
#define UCHIHA_LOG_DEBUG(msg) UchihaSpoofer::Utils::Logger::GetInstance().Debug(msg)
#define UCHIHA_LOG_INFO(msg) UchihaSpoofer::Utils::Logger::GetInstance().Info(msg)
#define UCHIHA_LOG_WARNING(msg) UchihaSpoofer::Utils::Logger::GetInstance().Warning(msg)
#define UCHIHA_LOG_ERROR(msg) UchihaSpoofer::Utils::Logger::GetInstance().Error(msg)
#define UCHIHA_LOG_CRITICAL(msg) UchihaSpoofer::Utils::Logger::GetInstance().Critical(msg)

} // namespace UchihaSpoofer::Utils
