#pragma once

#include <imgui.h>

namespace UchihaSpoofer::UI {

class UIStyles {
public:
    // Áp dụng Uchiha theme (đỏ Sharingan & đen)
    static void ApplyUchihaStyle();

    // Reset style về mặc định ImGui
    static void ResetStyle();

    // Các color presets
    static ImVec4 GetColorRed() { return ImVec4(1.0f, 0.2f, 0.2f, 1.0f); }
    static ImVec4 GetColorGreen() { return ImVec4(0.2f, 0.9f, 0.2f, 1.0f); }
    static ImVec4 GetColorYellow() { return ImVec4(1.0f, 0.8f, 0.0f, 1.0f); }
    static ImVec4 GetColorOrange() { return ImVec4(1.0f, 0.6f, 0.0f, 1.0f); }
    static ImVec4 GetColorDarkRed() { return ImVec4(0.85f, 0.10f, 0.10f, 1.0f); }
};

} // namespace UchihaSpoofer::UI
