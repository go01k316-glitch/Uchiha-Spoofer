#pragma execution_character_set("utf-8")
#include "Components.h"
#include <imgui.h>
#include <cmath>

namespace UchihaSpoofer::UI {

void UIComponents::DrawRotatingSharingan(ImVec2 center, float radius, float speed) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    float time = (float)ImGui::GetTime();
    float angle_offset = time * speed;

    // Vòng tròn chính đỏ
    draw_list->AddCircleFilled(center, radius, IM_COL32(215, 15, 15, 255), 64);
    draw_list->AddCircle(center, radius, IM_COL32(10, 10, 10, 255), 64, 3.0f);

    // Vòng tròn Tomoe
    float tomoe_ring_r = radius * 0.58f;
    draw_list->AddCircle(center, tomoe_ring_r, IM_COL32(10, 10, 10, 140), 64, 1.5f);

    // Con ng瞳 (pupil)
    float pupil_r = radius * 0.18f;
    draw_list->AddCircleFilled(center, pupil_r, IM_COL32(10, 10, 10, 255), 32);

    // Vẽ 3 Tomoe xoay
    float tomoe_r = radius * 0.11f;
    for (int i = 0; i < 3; i++) {
        float angle = angle_offset + i * (2.0f * 3.14159265f / 3.0f);

        ImVec2 tomoe_pos = ImVec2(
            center.x + cosf(angle) * tomoe_ring_r,
            center.y + sinf(angle) * tomoe_ring_r
        );

        draw_list->AddCircleFilled(tomoe_pos, tomoe_r, IM_COL32(10, 10, 10, 255), 16);

        // Vẽ đuôi Tomoe
        float tail_angle_offset = -0.32f;
        for (int j = 1; j <= 5; j++) {
            float t_angle = angle + (tail_angle_offset * (j / 5.0f));
            float t_dist = tomoe_ring_r - (radius * 0.05f * sinf((float)j / 5.0f * 1.57f));

            ImVec2 tail_pos = ImVec2(
                center.x + cosf(t_angle) * t_dist,
                center.y + sinf(t_angle) * t_dist
            );
            float tail_r = tomoe_r * (1.0f - (float)j * 0.16f);
            if (tail_r > 0.5f) {
                draw_list->AddCircleFilled(tail_pos, tail_r, IM_COL32(10, 10, 10, 255), 8);
            }
        }
    }
}

void UIComponents::ShowProgressBar(float progress, const std::string& task) {
    char percentageBuf[32];
    sprintf_s(percentageBuf, "%d%%", static_cast<int>(progress * 100));

    ImGui::ProgressBar(progress, ImVec2(690, 30), percentageBuf);
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Đang thực hiện: %s", task.c_str());
}

void UIComponents::ShowStatusBadge(const std::string& text, ImVec4 color, bool pulse) {
    if (pulse) {
        float alpha = 0.5f + 0.5f * sinf(ImGui::GetTime() * 3.0f);
        ImVec4 pulseColor = color;
        pulseColor.w = alpha;
        ImGui::TextColored(pulseColor, "● %s", text.c_str());
    }
    else {
        ImGui::TextColored(color, "● %s", text.c_str());
    }
}

void UIComponents::ShowSectionHeader(const std::string& title, ImVec4 color) {
    ImGui::TextColored(color, "%s", title.c_str());
    ImGui::Separator();
    ImGui::Spacing();
}

bool UIComponents::CheckboxWithConfig(const std::string& label, bool& value, const std::string& configKey) {
    bool changed = ImGui::Checkbox(label.c_str(), &value);
    if (changed && !configKey.empty()) {
        // Callback để lưu config - sẽ được implement trong UIManager
    }
    return changed;
}

} // namespace UchihaSpoofer::UI
