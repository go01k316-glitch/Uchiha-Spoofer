#pragma once

#include <exception>
#include <string>
#include <sstream>
#include "ErrorCode.h"

namespace UchihaSpoofer::Core {

// Exception custom cho Uchiha Spoofer
class UchihaException : public std::exception {
private:
    ErrorCode errorCode_;
    std::string message_;
    std::string fullMessage_;

public:
    explicit UchihaException(ErrorCode code, const std::string& message = "")
        : errorCode_(code), message_(message) {
        std::ostringstream oss;
        oss << "[0x" << std::hex << static_cast<uint32_t>(code) << "] "
            << ErrorCodeToString(code);
        if (!message.empty()) {
            oss << " - " << message;
        }
        fullMessage_ = oss.str();
    }

    const char* what() const noexcept override {
        return fullMessage_.c_str();
    }

    ErrorCode GetErrorCode() const noexcept {
        return errorCode_;
    }

    const std::string& GetMessage() const noexcept {
        return message_;
    }
};

// Macro helper để throw exception với HRESULT
#define THROW_IF_FAILED(hr, code, msg) \
    if (FAILED(hr)) { \
        throw UchihaException(code, msg); \
    }

// Macro helper để throw exception với condition
#define THROW_IF(condition, code, msg) \
    if (condition) { \
        throw UchihaException(code, msg); \
    }

} // namespace UchihaSpoofer::Core
