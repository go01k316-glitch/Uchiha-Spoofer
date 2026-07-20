#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <memory>
#include "../Core/Exception.h"
#include "../Utils/Logger.h"

namespace UchihaSpoofer::UI {

class UIManager {
public:
    static UIManager& GetInstance() {
        static UIManager instance;
        return instance;
    }

    // Khởi tạo DirectX 11 & ImGui
    bool Initialize(HWND hwnd);

    // Bắt đầu frame mới
    void BeginFrame();

    // Kết thúc frame và render
    void EndFrame();

    // Vẽ main UI
    void Render();

    // Cleanup
    void Shutdown();

    // Kiểm tra device D3D
    ID3D11Device* GetDevice() { return g_pd3dDevice; }
    ID3D11DeviceContext* GetDeviceContext() { return g_pd3dDeviceContext; }

    ~UIManager();

private:
    UIManager() = default;
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;

    // Khởi tạo DirectX 11
    bool CreateDeviceD3D(HWND hWnd);
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();

    // DirectX 11 objects
    ID3D11Device* g_pd3dDevice = nullptr;
    ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
    IDXGISwapChain* g_pSwapChain = nullptr;
    ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
};

} // namespace UchihaSpoofer::UI
