#pragma once

#include <imgui.h>
#include <string>

namespace UchihaSpoofer::UI {

// Toast notification structure
struct Toast {
    std::string title;
    std::string message;
    ImVec4 color;
    float remainingTime;
    float maxTime;
};

class UIComponents {
public:
    // Vẽ rotating Sharingan
    static void DrawRotatingSharingan(ImVec2 center, float radius, float speed);

    // Hiển thị progress bar
    static void ShowProgressBar(float progress, const std::string& task);

    // Hiển thị status badge
    static void ShowStatusBadge(const std::string& text, ImVec4 color, bool pulse = false);

    // Hiển thị section header
    static void ShowSectionHeader(const std::string& title, ImVec4 color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f));

    // Checkbox với auto-save config
    static bool CheckboxWithConfig(const std::string& label, bool& value, const std::string& configKey = "");
};

} // namespace UchihaSpoofer::UI
