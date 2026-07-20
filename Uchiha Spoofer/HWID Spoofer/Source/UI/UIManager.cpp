#pragma execution_character_set("utf-8")
#include "UIManager.h"
#include "UIStyles.h"
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

pragma comment(lib, "d3d11.lib")
pragma comment(lib, "dxgi.lib")

namespace UchihaSpoofer::UI {

bool UIManager::Initialize(HWND hwnd) {
    try {
        UCHIHA_LOG_INFO("[UI] Khởi tạo UIManager...");

        if (!CreateDeviceD3D(hwnd)) {
            throw Core::UchihaException(Core::ErrorCode::DirectXInitializationFailed,
                "Failed to create D3D device");
        }

        // Khởi tạo ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

        // Load font Vietnamese
        char winFolder[512];
        GetWindowsDirectoryA(winFolder, sizeof(winFolder));
        std::string fontPath = std::string(winFolder) + "\\Fonts\\segoeui.ttf";
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 16.0f, NULL,
            io.Fonts->GetGlyphRangesVietnamese());

        // Áp dụng Uchiha style
        UIStyles::ApplyUchihaStyle();

        UCHIHA_LOG_INFO("[UI] UIManager khởi tạo thành công!");
        return true;
    }
    catch (const Core::UchihaException& e) {
        UCHIHA_LOG_ERROR(std::string("[UI] ") + e.what());
        return false;
    }
}

bool UIManager::CreateDeviceD3D(HWND hWnd) {
    try {
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 2;
        sd.BufferDesc.Width = 0;
        sd.BufferDesc.Height = 0;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        UINT createDeviceFlags = 0;
        D3D_FEATURE_LEVEL featureLevel;
        const D3D_FEATURE_LEVEL featureLevelArray[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};

        HRESULT res = D3D11CreateDeviceAndSwapChain(
            NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags,
            featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
            &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

        if (res == DXGI_ERROR_UNSUPPORTED) {
            res = D3D11CreateDeviceAndSwapChain(
                NULL, D3D_DRIVER_TYPE_WARP, NULL, createDeviceFlags,
                featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
                &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
        }

        if (res != S_OK) {
            throw Core::UchihaException(Core::ErrorCode::DirectXInitializationFailed,
                "D3D11CreateDeviceAndSwapChain failed");
        }

        CreateRenderTarget();
        return true;
    }
    catch (const std::exception& e) {
        UCHIHA_LOG_ERROR(std::string("[UI] CreateDeviceD3D error: ") + e.what());
        return false;
    }
}

void UIManager::BeginFrame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void UIManager::EndFrame() {
    ImGui::Render();
    const float clear_color_with_alpha[4] = {0.06f, 0.06f, 0.07f, 1.00f};
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_pSwapChain->Present(1, 0);
}

void UIManager::Render() {
    // Main UI rendering logic sẽ được implement trong main app
}

void UIManager::CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = NULL;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

void UIManager::CleanupRenderTarget() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = NULL;
    }
}

void UIManager::CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) {
        g_pSwapChain->Release();
        g_pSwapChain = NULL;
    }
    if (g_pd3dDeviceContext) {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = NULL;
    }
    if (g_pd3dDevice) {
        g_pd3dDevice->Release();
        g_pd3dDevice = NULL;
    }
}

void UIManager::Shutdown() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    UCHIHA_LOG_INFO("[UI] UIManager đã shutdown");
}

UIManager::~UIManager() {
    // Không gọi Shutdown ở đây để tránh double-free
}

} // namespace UchihaSpoofer::UI
