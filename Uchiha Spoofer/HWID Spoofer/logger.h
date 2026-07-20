#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <ctime>
#include <sstream>

namespace Uchiha {

class Logger {
private:
    std::vector<std::string> logs_memory;
    std::ofstream log_file;
    std::mutex log_mutex;
    bool file_logging_enabled = false;
    
    std::string GetTimestamp() {
        time_t now = time(0);
        tm ltm;
        localtime_s(&ltm, &now);
        
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "[%02d:%02d:%02d]",
                 ltm.tm_hour, ltm.tm_min, ltm.tm_sec);
        return std::string(buffer);
    }
    
public:
    Logger() = default;
    ~Logger() {
        if (log_file.is_open()) {
            log_file.close();
        }
    }
    
    bool InitializeFileLogging(const std::string& filename) {
        std::lock_guard<std::mutex> lock(log_mutex);
        
        log_file.open(filename, std::ios::app);
        if (!log_file.is_open()) {
            return false;
        }
        
        file_logging_enabled = true;
        log_file << "\n======== LOG SESSION STARTED ========\n";
        log_file << "Time: " << GetTimestamp() << "\n";
        log_file << "====================================\n\n";
        log_file.flush();
        
        return true;
    }
    
    void AddLog(const std::string& message) {
        std::lock_guard<std::mutex> lock(log_mutex);
        
        std::string timestamped_msg = GetTimestamp() + " " + message;
        logs_memory.push_back(timestamped_msg);
        
        if (file_logging_enabled && log_file.is_open()) {
            log_file << timestamped_msg << "\n";
            log_file.flush();
        }
    }
    
    const std::vector<std::string>& GetLogs() const {
        return logs_memory;
    }
    
    void ClearLogs() {
        std::lock_guard<std::mutex> lock(log_mutex);
        logs_memory.clear();
    }
    
    size_t GetLogCount() const {
        return logs_memory.size();
    }
    
    std::string GetLastLog() const {
        if (logs_memory.empty()) return "";
        return logs_memory.back();
    }
};

} // namespace Uchiha
