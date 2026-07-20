#pragma once

#include "../Core/Types.h"
#include <memory>

namespace UchihaSpoofer::Spoofing {

class IdentityManager {
public:
    static IdentityManager& GetInstance() {
        static IdentityManager instance;
        return instance;
    }

    // Lấy danh tính gốc
    const Core::Identity& GetOriginalIdentity() const { return originalIdentity_; }

    // Lấy danh tính giả
    const Core::Identity& GetFakeIdentity() const { return fakeIdentity_; }

    // Tạo danh tính mới
    void LoadOriginalIdentity();
    void GenerateFakeIdentity();

    // Cập nhật danh tính
    void UpdateFakeIdentity(const Core::Identity& identity);

    // Custom HWID input
    void SetCustomIdentity(const Core::Identity& custom);

    // Validate identity
    bool ValidateIdentity(const Core::Identity& identity) const;

    // Lưu/tải cấu hình
    void SaveConfiguration();
    void LoadConfiguration();

private:
    IdentityManager() = default;
    ~IdentityManager() = default;

    IdentityManager(const IdentityManager&) = delete;
    IdentityManager& operator=(const IdentityManager&) = delete;

    Core::Identity originalIdentity_;
    Core::Identity fakeIdentity_;
};

} // namespace UchihaSpoofer::Spoofing
