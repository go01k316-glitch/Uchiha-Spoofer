#pragma execution_character_set("utf-8")
#include "UIStyles.h"
#include <imgui.h>

namespace UchihaSpoofer::UI {

void UIStyles::ApplyUchihaStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.ScrollbarRounding = 5.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.09f, 0.11f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.18f, 0.11f, 0.11f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.09f, 0.09f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.14f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.85f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.90f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.85f, 0.10f, 0.10f, 1.00f);
}

void UIStyles::ResetStyle() {
    ImGui::StyleColorsDark();
}

} // namespace UchihaSpoofer::UI
