#pragma execution_character_set("utf-8")

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <ctime>
#include <thread>

// Core modules
#include "Source/Core/Exception.h"
#include "Source/Core/Types.h"

// Utils modules
#include "Source/Utils/Logger.h"
#include "Source/Utils/SystemUtils.h"
#include "Source/Utils/StringUtils.h"
#include "Source/Utils/FileUtils.h"
#include "Source/Utils/CacheManager.h"
#include "Source/Utils/AsyncWorker.h"

// Driver module
#include "Source/Driver/DriverManager.h"

// Spoofing modules
#include "Source/Spoofing/IdentityManager.h"
#include "Source/Spoofing/Spoofer.h"
#include "Source/Spoofing/DeepCleaner.h"
#include "Source/Spoofing/ConfigManager.h"
#include "Source/Spoofing/HWIDPresets.h"
#include "Source/Spoofing/BackupManager.h"

// UI modules
#include "Source/UI/UIManager.h"
#include "Source/UI/UIStyles.h"
#include "Source/UI/Components.h"

using namespace UchihaSpoofer;

// Forward declarations
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Global state
struct AppState {
    int activeTab = 0;
    int previousTab = 0;
    float tabAlpha = 1.0f;
    bool isSpoofed = false;
    Core::SpoofConfig spoofConfig;
    Core::OperationProgress spoofProgress;
    Core::OperationProgress recoveryProgress;
    Core::OperationProgress cleanProgress;
    
    // Profile management
    std::vector<std::string> profileList;
    int selectedProfileIdx = 0;
    char customHWIDInput[256] = {0};
    int selectedPresetIdx = 0;
    int selectedBackupIdx = 0;
};

static AppState g_appState;
static HWND g_hwnd = nullptr;
static bool g_done = false;

// Window procedure
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (msg == WM_SIZE && wParam != SIZE_MINIMIZED) {
            auto& ui = UI::UIManager::GetInstance();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        g_done = true;
        return 0;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

// Render main interface
void RenderMainUI() {
    auto& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(1050, 650));
    ImGui::Begin("MainPanel", NULL,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    // --- SIDEBAR BÊN TRÁI ---
    ImGui::BeginChild("Sidebar", ImVec2(240, 0), true);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 0.15f, 0.15f, 1.0f), "  U C H I H A - H I T A C H I");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Vẽ Sharingan động
    ImVec2 sidebar_size = ImGui::GetWindowSize();
    ImVec2 sharingan_center = ImVec2(
        ImGui::GetWindowPos().x + sidebar_size.x / 2.0f,
        ImGui::GetWindowPos().y + 110.0f
    );

    float currentSharinganSpeed = 1.8f;
    if (g_appState.spoofProgress.isRunning || g_appState.recoveryProgress.isRunning ||
        g_appState.cleanProgress.isRunning) {
        currentSharinganSpeed = 6.0f;
    }
    UI::UIComponents::DrawRotatingSharingan(sharingan_center, 38.0f, currentSharinganSpeed);

    ImGui::Dummy(ImVec2(0, 95.0f));

    // Buttons menu
    bool operationInProgress = g_appState.spoofProgress.isRunning ||
                               g_appState.recoveryProgress.isRunning ||
                               g_appState.cleanProgress.isRunning;

    ImGui::BeginDisabled(operationInProgress);
    if (ImGui::Button("Trang Chủ", ImVec2(220, 40))) g_appState.activeTab = 0;
    if (ImGui::Button("Thay Đổi HWID", ImVec2(220, 40))) g_appState.activeTab = 1;
    if (ImGui::Button("Quản Lý Profile", ImVec2(220, 40))) g_appState.activeTab = 2;
    if (ImGui::Button("Dọn Dẹp Dấu Vết", ImVec2(220, 40))) g_appState.activeTab = 3;
    if (ImGui::Button("Quản Lý Backup", ImVec2(220, 40))) g_appState.activeTab = 4;
    if (ImGui::Button("Hướng Dẫn Sử Dụng", ImVec2(220, 40))) g_appState.activeTab = 5;
    ImGui::EndDisabled();

    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 50);
    ImGui::Text("Phiên Bản: 4.0.0\nOptimized Edition");
    ImGui::EndChild();

    ImGui::SameLine();

    // --- XỬ LÝ FADE-IN CHUYỂN TAB MỊU MÀ ---
    if (g_appState.activeTab != g_appState.previousTab) {
        g_appState.tabAlpha = 0.0f;
        g_appState.previousTab = g_appState.activeTab;
    }
    if (g_appState.tabAlpha < 1.0f) {
        g_appState.tabAlpha += io.DeltaTime * 4.5f;
        if (g_appState.tabAlpha > 1.0f) g_appState.tabAlpha = 1.0f;
    }

    // --- KHU VỰC HIỂN THỊ CHÍNH ---
    ImGui::BeginGroup();
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_appState.tabAlpha);

    auto& idManager = Spoofing::IdentityManager::GetInstance();
    const auto& originalID = idManager.GetOriginalIdentity();
    const auto& fakeID = idManager.GetFakeIdentity();

    if (g_appState.activeTab == 0) {
        // Tab 0: Trang Chủ
        UI::UIComponents::ShowSectionHeader("TRẠNG THÁI HWID HIỆN TẠI HỆ THỐNG",
            UI::UIStyles::GetColorRed());

        ImGui::Text("TÊN MÁY TÍNH: %s",
            (g_appState.isSpoofed ? fakeID.pcName.c_str() : originalID.pcName.c_str()));
        ImGui::Text("ĐỊA CHỈ MAC: %s",
            (g_appState.isSpoofed ? fakeID.macAddress.c_str() : originalID.macAddress.c_str()));
        ImGui::Text("MACHINE GUID: %s",
            (g_appState.isSpoofed ? fakeID.registryGuid.c_str() : originalID.registryGuid.c_str()));
        ImGui::Text("HW-PROFILE: %s",
            (g_appState.isSpoofed ? fakeID.hwProfileGuid.c_str() : originalID.hwProfileGuid.c_str()));
        ImGui::Text("PRODUCT ID: %s",
            (g_appState.isSpoofed ? fakeID.productId.c_str() : originalID.productId.c_str()));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        UI::UIComponents::ShowSectionHeader("THÔNG SỐ PHÂN VÙNG Ổ CỨNG (VOLUME ID)",
            UI::UIStyles::GetColorRed());
        ImGui::Text("MÃ SERIAL Ổ C: %s",
            (g_appState.isSpoofed ? fakeID.volCSerialStr.c_str() : originalID.volCSerialStr.c_str()));
        ImGui::Text("MÃ SERIAL Ổ D: %s",
            (g_appState.isSpoofed ? fakeID.volDSerialStr.c_str() : originalID.volDSerialStr.c_str()));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Status indicator
        if (g_appState.isSpoofed) {
            UI::UIComponents::ShowStatusBadge("HỆ THỐNG ĐÃ ĐƯỢC NGỤ TRANG", UI::UIStyles::GetColorRed(), true);
        } else {
            UI::UIComponents::ShowStatusBadge("HỆ THỐNG TRẠNG THÁI BÌNH THƯỜNG", UI::UIStyles::GetColorGreen(), false);
        }
    }
    else if (g_appState.activeTab == 1) {
        // Tab 1: Thay Đổi HWID
        ImGui::BeginDisabled(operationInProgress);
        ImGui::Columns(2, "options_col", false);

        UI::UIComponents::ShowSectionHeader("Thay đổi Registry & Thiết bị");

        ImGui::Checkbox("Thay đổi Địa chỉ MAC", &g_appState.spoofConfig.selectMAC);
        ImGui::Checkbox("Thay đổi Machine GUID", &g_appState.spoofConfig.selectGUID);
        ImGui::Checkbox("Thay đổi HwProfile GUID", &g_appState.spoofConfig.selectHWProfile);
        ImGui::Checkbox("Thay đổi Tên máy tính", &g_appState.spoofConfig.selectPCName);
        ImGui::Checkbox("Thay đổi Product ID", &g_appState.spoofConfig.selectProductID);

        ImGui::NextColumn();

        UI::UIComponents::ShowSectionHeader("Thay đổi mã Ổ cứng (Volume ID)");
        ImGui::Checkbox("Thay đổi mã Serial ổ đĩa [C:]", &g_appState.spoofConfig.selectVolC);
        ImGui::Checkbox("Thay đổi mã Serial ổ đĩa [D:]", &g_appState.spoofConfig.selectVolD);
        ImGui::Spacing();

        auto& driverMgr = Driver::DriverManager::GetInstance();
        if (driverMgr.IsLoaded()) {
            ImGui::Checkbox("Thay đổi CPU Serial", &g_appState.spoofConfig.selectCPU);
            ImGui::Checkbox("Thay đổi BIOS Serial", &g_appState.spoofConfig.selectBIOS);
        }
        else {
            ImGui::BeginDisabled();
            ImGui::Checkbox("CPU Serial (Cần nạp Driver)", &g_appState.spoofConfig.selectCPU);
            ImGui::Checkbox("BIOS Serial (Cần nạp Driver)", &g_appState.spoofConfig.selectBIOS);
            ImGui::EndDisabled();
        }

        ImGui::Columns(1);
        ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Presets
        auto presets = Spoofing::HWIDPresets::GetAvailablePresets();
        if (ImGui::Combo("Quick Presets", &g_appState.selectedPresetIdx, 
                         [](void* data, int idx, const char** out_text) {
                             auto* presets_ptr = (std::vector<std::string>*)data;
                             if (idx < 0 || idx >= (int)presets_ptr->size()) return false;
                             *out_text = (*presets_ptr)[idx].c_str();
                             return true;
                         }, 
                         (void*)&presets, presets.size())) {
            if (g_appState.selectedPresetIdx < (int)presets.size()) {
                auto preset = Spoofing::HWIDPresets::GetPreset(presets[g_appState.selectedPresetIdx]);
                idManager.SetCustomIdentity(preset);
                UCHIHA_LOG_INFO("[UI] Applied preset: " + presets[g_appState.selectedPresetIdx]);
            }
        }

        ImGui::Spacing();

        // Progress bar hoặc buttons
        if (g_appState.spoofProgress.isRunning) {
            ImGui::TextColored(UI::UIStyles::GetColorRed(),
                "Mangekyou Sharingan đang áp đặt ảo ảnh lên hệ thống...");
            UI::UIComponents::ShowProgressBar(
                g_appState.spoofProgress.progress,
                g_appState.spoofProgress.currentTask);
        }
        else if (g_appState.recoveryProgress.isRunning) {
            ImGui::TextColored(UI::UIStyles::GetColorGreen(),
                "Đang phá giải thuật ảo ảnh (Khôi phục danh tính)...");
            UI::UIComponents::ShowProgressBar(
                g_appState.recoveryProgress.progress,
                g_appState.recoveryProgress.currentTask);
        }
        else {
            if (ImGui::Button("TẠO ID MỚI NGẪU NHIÊN", ImVec2(200, 40))) {
                idManager.GenerateFakeIdentity();
                Utils::Logger::GetInstance().Info("[UI] Tạo ID mới ngẫu nhiên");
            }
            ImGui::SameLine();
            if (ImGui::Button("BẮT ĐẦU SPOOF", ImVec2(200, 40))) {
                auto& spoofer = Spoofing::Spoofer::GetInstance();
                spoofer.SetConfig(g_appState.spoofConfig);
                spoofer.StartSpoofing([](float progress, const std::string& task) {
                    g_appState.spoofProgress.progress = progress;
                    g_appState.spoofProgress.currentTask = task;
                    g_appState.spoofProgress.isRunning = progress < 1.0f;
                    if (progress >= 1.0f) {
                        g_appState.isSpoofed = true;
                    }
                });
                g_appState.spoofProgress.isRunning = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("KHÔI PHỤC GỐC", ImVec2(200, 40))) {
                auto& spoofer = Spoofing::Spoofer::GetInstance();
                spoofer.RecoverOriginal([](float progress, const std::string& task) {
                    g_appState.recoveryProgress.progress = progress;
                    g_appState.recoveryProgress.currentTask = task;
                    g_appState.recoveryProgress.isRunning = progress < 1.0f;
                    if (progress >= 1.0f) {
                        g_appState.isSpoofed = false;
                    }
                });
                g_appState.recoveryProgress.isRunning = true;
            }
        }
    }
    else if (g_appState.activeTab == 2) {
        // Tab 2: Quản Lý Profile
        UI::UIComponents::ShowSectionHeader("QUẢN LÝ CẤU HÌNH SPOOF (PROFILES)");

        auto& configMgr = Spoofing::ConfigManager::GetInstance();
        g_appState.profileList = configMgr.GetProfileList();

        ImGui::Text("Các Profile đã lưu (%zu):", g_appState.profileList.size());
        ImGui::ListBoxHeader("##profiles", ImVec2(400, 150));
        for (size_t i = 0; i < g_appState.profileList.size(); i++) {
            if (ImGui::Selectable(g_appState.profileList[i].c_str(), g_appState.selectedProfileIdx == (int)i)) {
                g_appState.selectedProfileIdx = i;
            }
        }
        ImGui::ListBoxFooter();

        ImGui::Spacing();

        ImGui::BeginDisabled(operationInProgress || g_appState.selectedProfileIdx >= (int)g_appState.profileList.size());
        if (ImGui::Button("Load Profile", ImVec2(100, 30))) {
            if (g_appState.selectedProfileIdx < (int)g_appState.profileList.size()) {
                Spoofing::SpoofProfile profile;
                if (configMgr.LoadProfile(g_appState.profileList[g_appState.selectedProfileIdx], profile)) {
                    idManager.SetCustomIdentity(profile.identity);
                    g_appState.spoofConfig = profile.config;
                    UCHIHA_LOG_INFO("[UI] Loaded profile: " + g_appState.profileList[g_appState.selectedProfileIdx]);
                }
            }
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Delete Profile", ImVec2(100, 30))) {
            if (g_appState.selectedProfileIdx < (int)g_appState.profileList.size()) {
                configMgr.DeleteProfile(g_appState.profileList[g_appState.selectedProfileIdx]);
                g_appState.selectedProfileIdx = 0;
            }
        }

        ImGui::SameLine();
        static char profileName[64] = {0};
        ImGui::InputText("Profile Name", profileName, sizeof(profileName));

        ImGui::SameLine();
        if (ImGui::Button("Save as Profile", ImVec2(120, 30))) {
            if (strlen(profileName) > 0) {
                Spoofing::SpoofProfile newProfile;
                newProfile.name = profileName;
                newProfile.identity = idManager.GetFakeIdentity();
                newProfile.config = g_appState.spoofConfig;
                configMgr.SaveProfile(profileName, newProfile);
                memset(profileName, 0, sizeof(profileName));
            }
        }
    }
    else if (g_appState.activeTab == 3) {
        // Tab 3: Dọn Dẹp Dấu Vết
        UI::UIComponents::ShowSectionHeader("HỆ THỐNG DỌN DẸP SÂU HƠN (DEEP CLEAN ENGINE)");

        ImGui::Text("Chức năng này sẽ dọn sạch mọi file Log, bộ nhớ đệm Tracking, Registry rác của");
        ImGui::Text("Steam, Epic Games, Riot Games và dọn sạch Event Logs của Windows.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (g_appState.cleanProgress.isRunning) {
            ImGui::TextColored(UI::UIStyles::GetColorOrange(),
                "Deep Cleaner đang quét sạch dấu vết...");
            UI::UIComponents::ShowProgressBar(
                g_appState.cleanProgress.progress,
                g_appState.cleanProgress.currentTask);
        }
        else {
            ImGui::BeginDisabled(operationInProgress);
            if (ImGui::Button("KÍCH HOẠT DEEP CLEAN (QUÉT SẠCH DẤU VẾT)", ImVec2(450, 45))) {
                auto& cleaner = Spoofing::DeepCleaner::GetInstance();
                cleaner.StartDeepClean([](float progress, const std::string& task) {
                    g_appState.cleanProgress.progress = progress;
                    g_appState.cleanProgress.currentTask = task;
                    g_appState.cleanProgress.isRunning = progress < 1.0f;
                });
                g_appState.cleanProgress.isRunning = true;
            }
            ImGui::EndDisabled();
        }
    }
    else if (g_appState.activeTab == 4) {
        // Tab 4: Quản Lý Backup
        UI::UIComponents::ShowSectionHeader("QUẢN LÝ BACKUP & KHÔI PHỤC");

        auto& backupMgr = Spoofing::BackupManager::GetInstance();
        auto backupList = backupMgr.GetBackupList();

        ImGui::Text("Các Backup đã lưu (%zu):", backupList.size());
        ImGui::ListBoxHeader("##backups", ImVec2(400, 150));
        for (size_t i = 0; i < backupList.size(); i++) {
            if (ImGui::Selectable(backupList[i].c_str(), g_appState.selectedBackupIdx == (int)i)) {
                g_appState.selectedBackupIdx = i;
            }
        }
        ImGui::ListBoxFooter();

        ImGui::Spacing();

        ImGui::BeginDisabled(operationInProgress || g_appState.selectedBackupIdx >= (int)backupList.size());
        if (ImGui::Button("Restore Backup", ImVec2(150, 30))) {
            if (g_appState.selectedBackupIdx < (int)backupList.size()) {
                backupMgr.RestoreBackup(backupList[g_appState.selectedBackupIdx]);
                UCHIHA_LOG_INFO("[UI] Restored backup: " + backupList[g_appState.selectedBackupIdx]);
            }
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Delete Backup", ImVec2(150, 30))) {
            if (g_appState.selectedBackupIdx < (int)backupList.size()) {
                backupMgr.DeleteBackup(backupList[g_appState.selectedBackupIdx]);
                g_appState.selectedBackupIdx = 0;
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Manual Backup Now", ImVec2(150, 30))) {
            backupMgr.CreateBackup();
            UCHIHA_LOG_INFO("[UI] Manual backup created");
        }
    }
    else if (g_appState.activeTab == 5) {
        // Tab 5: Hướng Dẫn Sử Dụng
        UI::UIComponents::ShowSectionHeader("CẨM NANG HƯỚNG DẪN SỬ DỤNG PHẦN MỀM");

        ImGui::BulletText("BƯỚC 1: Hãy chắc chắn khởi động Spoofer bằng quyền Trực tiếp Quản trị viên (Admin).");
        ImGui::BulletText("BƯỚC 2: Vào Tab 'Thay Đổi HWID', bấm nút 'TẠO ID MỚI NGẪU NHIÊN'. Bật các mục mong muốn.");
        ImGui::BulletText("BƯỚC 3: Nhấn nút 'BẮT ĐẦU SPOOF'. Hệ thống mạng sẽ làm mới trong vòng 3 giây để đổi MAC.");
        ImGui::BulletText("BƯỚC 4: Chuyển qua Tab 'Dọn Dẹp Dấu Vết', nhấn nút dọn dẹp để quét sạch cache Registry cũ của Game.");
        ImGui::BulletText("BƯỚC 5: Khi muốn chơi game bình thường và trả lại ID thật, hãy nhấn 'KHÔI PHỤC GỐC'.");
        ImGui::BulletText("\nBỏ qua: Sử dụng Tab 'Quản Lý Profile' để lưu cấu hình và tái sử dụng nhanh chóng.");
        ImGui::BulletText("\nQuản Lý Backup: Tab 'Quản Lý Backup' cho phép bạn tạo & khôi phục từ backup tự động.");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(UI::UIStyles::GetColorOrange(),
            "Mẹo nhỏ: Với các game quét dọc gặt gao, hãy chạy 'Deep Clean' trước rồi mới bấm Spoof!");
        
        ImGui::Spacing();
        ImGui::TextColored(UI::UIStyles::GetColorGreen(),
            "🎉 Chúc bạn sử dụng vui vẻ với Uchiha Spoofer!");
    }

    // --- CONSOLE LOG BOX ---
    ImGui::SetCursorPosY(450);
    ImGui::Text("Nhật ký hoạt động (System Logs)");
    ImGui::BeginChild("ConsoleLog", ImVec2(750, 150), true);

    auto logs = Utils::Logger::GetInstance().GetRecentLogs(50);
    for (const auto& log : logs) {
        ImGui::TextUnformatted(log.c_str());
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    ImGui::PopStyleVar();
    ImGui::EndGroup();
    ImGui::End();
}

int main(int, char**) {
    try {
        srand(static_cast<unsigned int>(time(0)));

        // Khởi tạo Logger
        auto& logger = Utils::Logger::GetInstance();
        logger.Info("[MAIN] Uchiha Spoofer v4.0.0 - Optimized Edition");
        logger.Info("[MAIN] Khởi động ứng dụng...");

        // Khởi tạo Identity Manager và load original identity
        auto& idManager = Spoofing::IdentityManager::GetInstance();
        idManager.LoadOriginalIdentity();
        idManager.GenerateFakeIdentity();
        g_appState.spoofConfig = Core::SpoofConfig();

        // Khởi tạo Config Manager
        auto& configMgr = Spoofing::ConfigManager::GetInstance();
        configMgr.LoadAllProfiles();

        // Khởi tạo Driver Manager
        auto& driverMgr = Driver::DriverManager::GetInstance();
        if (!driverMgr.Initialize()) {
            logger.Warning("[MAIN] Driver không thể khởi tạo (không critical)");
        }

        // Tạo window
        WNDCLASSEXA wc = {sizeof(WNDCLASSEXA), CS_CLASSDC, WndProc, 0L, 0L,
                         GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                         "UCHIHA_CLASS", NULL};
        RegisterClassExA(&wc);

        g_hwnd = CreateWindowA(wc.lpszClassName,
                              "UCHIHA-HITACHI SPOOFER ULTIMATE v4.0 (Optimized)",
                              WS_OVERLAPPEDWINDOW, 100, 100, 1050, 670,
                              NULL, NULL, wc.hInstance, NULL);

        if (!g_hwnd) {
            throw Core::UchihaException(Core::ErrorCode::WindowCreationFailed,
                "Cannot create window");
        }

        // Khởi tạo UI Manager
        auto& uiManager = UI::UIManager::GetInstance();
        if (!uiManager.Initialize(g_hwnd)) {
            throw Core::UchihaException(Core::ErrorCode::ImGuiInitializationFailed,
                "Cannot initialize ImGui");
        }

        ShowWindow(g_hwnd, SW_SHOWDEFAULT);
        UpdateWindow(g_hwnd);

        logger.Info("[MAIN] Giao diện khởi động thành công!");
        logger.Info("[MAIN] Chào mừng bạn đến với Uchiha Spoofer v4.0!");

        // Main game loop
        while (!g_done) {
            MSG msg;
            while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                if (msg.message == WM_QUIT)
                    g_done = true;
            }
            if (g_done) break;

            // Render frame
            uiManager.BeginFrame();
            RenderMainUI();
            uiManager.EndFrame();
        }

        // Cleanup
        logger.Info("[MAIN] Đang shutdown...");
        uiManager.Shutdown();
        driverMgr.Shutdown();
        DestroyWindow(g_hwnd);
        UnregisterClassA(wc.lpszClassName, wc.hInstance);

        logger.Info("[MAIN] Uchiha Spoofer shutdown hoàn tất!");
        logger.SaveToFile("uchiha_session.log");

        return 0;
    }
    catch (const Core::UchihaException& e) {
        Utils::Logger::GetInstance().Critical(std::string("[MAIN] Exception: ") + e.what());
        MessageBoxA(NULL, e.what(), "Uchiha Spoofer - Fatal Error", MB_ICONERROR);
        return 1;
    }
    catch (const std::exception& e) {
        std::string msg = std::string("[MAIN] Std Exception: ") + e.what();
        Utils::Logger::GetInstance().Critical(msg);
        MessageBoxA(NULL, msg.c_str(), "Uchiha Spoofer - Fatal Error", MB_ICONERROR);
        return 1;
    }
    catch (...) {
        Utils::Logger::GetInstance().Critical("[MAIN] Unknown exception!");
        MessageBoxA(NULL, "Unknown exception occurred!", "Uchiha Spoofer - Fatal Error", MB_ICONERROR);
        return 1;
    }
}
