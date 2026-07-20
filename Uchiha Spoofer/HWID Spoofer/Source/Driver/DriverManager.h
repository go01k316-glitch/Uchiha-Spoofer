#pragma once

#include <string>
#include "../Core/Exception.h"
#include "../Core/Types.h"

namespace UchihaSpoofer::Driver {

class DriverManager {
public:
    static DriverManager& GetInstance() {
        static DriverManager instance;
        return instance;
    }

    // Khởi tạo và load driver
    bool Initialize();

    // Kiểm tra driver đã load chưa
    bool IsLoaded() const { return isLoaded_; }

    // Gửi lệnh IOCTL đến driver
    bool SendIOControl(unsigned int ioctlCode);

    // Spoof CPU & BIOS
    bool SpoofCPU();
    bool SpoofBIOS();

    // Hủy driver
    void Shutdown();

    ~DriverManager() {
        if (isLoaded_) {
            Shutdown();
        }
    }

private:
    DriverManager() = default;
    DriverManager(const DriverManager&) = delete;
    DriverManager& operator=(const DriverManager&) = delete;

    // Khóa giải phóng driver
    bool ExtractDriver();
    bool LoadDriver();
    bool UnloadDriver();

    // Lấy đường dẫn driver
    std::string GetDriverPath() const;

    bool isLoaded_ = false;
    std::string driverPath_;
};

} // namespace UchihaSpoofer::Driver
