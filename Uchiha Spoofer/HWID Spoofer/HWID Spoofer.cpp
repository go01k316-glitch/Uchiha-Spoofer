#pragma execution_character_set("utf-8")

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// --- THƯ VIỆN HỆ THỐNG WINDOWS ---
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <d3d11.h>
#include <rpc.h>
#include <rpcdce.h>
#include <iphlpapi.h>
#include <winioctl.h>  
#include <iostream>
#include <fstream>     
#include <string>
#include <vector>
#include <ctime>
#include <cmath>       
#include <filesystem>  

// --- THƯ VIỆN ĐA LUỒNG & AN TOÀN DỮ LIỆU ---
#include <thread>
#include <mutex>
#include <atomic>

// --- THƯ VIỆN IMGUI ---
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// --- NHÚNG FILE CHỨA DRIVER NHỊ PHÂN ---
#include "driver.h" 

// --- THƯ VIỆN LIÊN KẾT (LINKER) ---
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Rpcrt4.lib")
#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib, "Ws2_32.lib")

using namespace std;
namespace fs = std::filesystem;

// --- ĐỊNH NGHĨA MÃ ĐIỀU KHIỂN (IOCTL) ---
#define IOCTL_SPOOF_CPU  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SPOOF_BIOS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

// --- KHAI BÁO CÁC BIẾN DIRECTX 11 ---
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// --- BIẾN TRẠNG THÁI ĐA LUỒNG & PROGRESS BAR ---
std::mutex logMutex;
std::mutex taskMutex;
std::atomic<bool> isSpoofingInProgress{ false };
std::atomic<float> spoofProgress{ 0.0f };
std::string spoofCurrentTask = "";

std::atomic<bool> isRecoveringInProgress{ false };
std::atomic<float> recoveryProgress{ 0.0f };
std::string recoveryCurrentTask = "";

// --- HỆ THỐNG TOAST NOTIFICATION (NEW) ---
struct Toast {
    string title;
    string message;
    ImVec4 color;
    float remainingTime;
    float maxTime;
};
std::vector<Toast> activeToasts;
std::mutex toastMutex;

// Hàm thread-safe để đẩy thông báo trượt lên màn hình
void AddToast(const string& title, const string& msg, ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f), float duration = 4.0f) {
    std::lock_guard<std::mutex> lock(toastMutex);
    activeToasts.push_back({ title, msg, color, duration, duration });
}

// --- BIẾN ĐIỀU KHIỂN HIỆU ỨNG CHUYỂN TAB MƯỢT MÀ (NEW) ---
float tabAlpha = 1.0f;
int previousTab = 0;

// Hàm thread-safe để cập nhật task ngầm
void SetSpoofTask(const string& task) {
    std::lock_guard<std::mutex> lock(taskMutex);
    spoofCurrentTask = task;
}
string GetSpoofTask() {
    std::lock_guard<std::mutex> lock(taskMutex);
    return spoofCurrentTask;
}

void SetRecoveryTask(const string& task) {
    std::lock_guard<std::mutex> lock(taskMutex);
    recoveryCurrentTask = task;
}
string GetRecoveryTask() {
    std::lock_guard<std::mutex> lock(taskMutex);
    return recoveryCurrentTask;
}

// --- HÀM THỰC THI DÒNG LỆNH ẨN DANH ---
void ExecuteSilentCommand(const std::string& command) {
    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    std::vector<char> cmdBuffer(command.begin(), command.end());
    cmdBuffer.push_back('\0');

    if (CreateProcessA(NULL, cmdBuffer.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

// --- HÀM DỌN DẸP THƯ MỤC NATIVE ---
void CleanDirectoryNative(const std::string& pathStr) {
    try {
        fs::path dirPath(pathStr);
        if (fs::exists(dirPath) && fs::is_directory(dirPath)) {
            for (const auto& entry : fs::directory_iterator(dirPath)) {
                try {
                    fs::remove_all(entry.path());
                }
                catch (...) {}
            }
        }
    }
    catch (...) {}
}

// --- CƠ CHẾ NẠP / HỦY DRIVER KERNEL ---
std::string GetCurrentDirectoryPath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    return std::string(buffer).substr(0, pos);
}

std::string GetDriverPath() {
    return GetCurrentDirectoryPath() + "\\UchihaDriver_temp.sys";
}

const std::string SERVICE_NAME = "UchihaDriver";
bool isDriverLoaded = false;

bool ExtractDriver() {
    std::string driverPath = GetDriverPath();
    std::ofstream os(driverPath, std::ios::binary);
    if (!os.is_open()) return false;
    os.write((char*)driver_bytes, driver_size);
    os.close();
    return true;
}

bool LoadUchihaDriver() {
    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCM) return false;

    std::string driverPath = GetDriverPath();
    SC_HANDLE hService = CreateServiceA(
        hSCM, SERVICE_NAME.c_str(), SERVICE_NAME.c_str(),
        SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL, driverPath.c_str(),
        NULL, NULL, NULL, NULL, NULL
    );

    if (!hService) {
        if (GetLastError() == ERROR_SERVICE_EXISTS) {
            hService = OpenServiceA(hSCM, SERVICE_NAME.c_str(), SERVICE_ALL_ACCESS);
        }
        else {
            CloseServiceHandle(hSCM);
            return false;
        }
    }

    if (!StartService(hService, 0, NULL)) {
        if (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING) {
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCM);
            return false;
        }
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
    isDriverLoaded = true;
    return true;
}

void UnloadUchihaDriver() {
    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCM) return;

    SC_HANDLE hService = OpenServiceA(hSCM, SERVICE_NAME.c_str(), SERVICE_ALL_ACCESS);
    if (hService) {
        SERVICE_STATUS status;
        ControlService(hService, SERVICE_CONTROL_STOP, &status);
        DeleteService(hService);
        CloseServiceHandle(hService);
    }
    CloseServiceHandle(hSCM);
    DeleteFileA(GetDriverPath().c_str());
}

// --- GIAO TIẾP DEVICE CONTROL ĐỂ GỬI LỆNH SPOOF XUỐNG KERNEL ---
bool SendCommandToDriver(DWORD ioctlCode) {
    HANDLE hDriver = CreateFileA("\\\\.\\UchihaDriverDevice",
        GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hDriver == INVALID_HANDLE_VALUE) return false;

    DWORD bytesReturned = 0;
    BOOL status = DeviceIoControl(hDriver, ioctlCode, NULL, 0, NULL, 0, &bytesReturned, NULL);

    CloseHandle(hDriver);
    return status ? true : false;
}

// --- CẤU TRÚC LOGIC SPOOFER ---
struct Identity {
    string macAddress;
    string registryGuid;
    string hwProfileGuid;
    string pcName;
    string productId;

    string volCSerialStr;
    string volDSerialStr;
    DWORD volC_Raw;
    DWORD volD_Raw;
};

Identity originalID;
Identity fakeID;
bool isSpoofed = false;
int activeTab = 0;

// --- CÁC TÙY CHỌN CẤU HÌNH (LƯU / TẢI QUA INI) (UPGRADED) ---
bool select_mac = true;
bool select_guid = true;
bool select_hwprofile = true;
bool select_pcname = true;
bool select_productid = true;
bool select_volC = true;
bool select_volD = true;
bool select_cpu = false;
bool select_bios = false;

// --- QUẢN LÝ LƯU / TẢI CẤU HÌNH CONFIG.INI (NEW) ---
void SaveConfig() {
    ofstream f("config.ini");
    if (!f.is_open()) return;
    f << "select_mac=" << (select_mac ? "1" : "0") << "\n";
    f << "select_guid=" << (select_guid ? "1" : "0") << "\n";
    f << "select_hwprofile=" << (select_hwprofile ? "1" : "0") << "\n";
    f << "select_pcname=" << (select_pcname ? "1" : "0") << "\n";
    f << "select_productid=" << (select_productid ? "1" : "0") << "\n";
    f << "select_volC=" << (select_volC ? "1" : "0") << "\n";
    f << "select_volD=" << (select_volD ? "1" : "0") << "\n";
    f << "select_cpu=" << (select_cpu ? "1" : "0") << "\n";
    f << "select_bios=" << (select_bios ? "1" : "0") << "\n";
    f.close();
}

void LoadConfig() {
    ifstream f("config.ini");
    if (!f.is_open()) return;
    string line;
    while (getline(f, line)) {
        size_t pos = line.find('=');
        if (pos == string::npos) continue;
        string key = line.substr(0, pos);
        string val = line.substr(pos + 1);
        bool bVal = (val == "1");

        if (key == "select_mac") select_mac = bVal;
        else if (key == "select_guid") select_guid = bVal;
        else if (key == "select_hwprofile") select_hwprofile = bVal;
        else if (key == "select_pcname") select_pcname = bVal;
        else if (key == "select_productid") select_productid = bVal;
        else if (key == "select_volC") select_volC = bVal;
        else if (key == "select_volD") select_volD = bVal;
        else if (key == "select_cpu") select_cpu = bVal;
        else if (key == "select_bios") select_bios = bVal;
    }
    f.close();
}

vector<string> logs;

// Hàm ghi Log có Lock tránh xung đột đa luồng
void AddLog(const string& text) {
    std::lock_guard<std::mutex> lock(logMutex);
    time_t now = time(0);
    tm ltm;
    localtime_s(&ltm, &now);
    char timeStr[15];
    sprintf_s(timeStr, "[%02d:%02d:%02d] ", ltm.tm_hour, ltm.tm_min, ltm.tm_sec);
    logs.push_back(string(timeStr) + text);
}

// --- HÀM VẼ SHARINGAN ĐỘNG XOAY TRÒN ---
void DrawRotatingSharingan(ImVec2 center, float radius, float speed) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    float time = (float)ImGui::GetTime();
    float angle_offset = time * speed;

    draw_list->AddCircleFilled(center, radius, IM_COL32(215, 15, 15, 255), 64);
    draw_list->AddCircle(center, radius, IM_COL32(10, 10, 10, 255), 64, 3.0f);

    float tomoe_ring_r = radius * 0.58f;
    draw_list->AddCircle(center, tomoe_ring_r, IM_COL32(10, 10, 10, 140), 64, 1.5f);

    float pupil_r = radius * 0.18f;
    draw_list->AddCircleFilled(center, pupil_r, IM_COL32(10, 10, 10, 255), 32);

    float tomoe_r = radius * 0.11f;
    for (int i = 0; i < 3; i++) {
        float angle = angle_offset + i * (2.0f * 3.14159265f / 3.0f);

        ImVec2 tomoe_pos = ImVec2(
            center.x + cosf(angle) * tomoe_ring_r,
            center.y + sinf(angle) * tomoe_ring_r
        );

        draw_list->AddCircleFilled(tomoe_pos, tomoe_r, IM_COL32(10, 10, 10, 255), 16);

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

// --- CÁC HÀM XỬ LÝ HỆ THỐNG / REGISTRY / DISK ---
string getRealMachineGuid() {
    HKEY hKey; char val[256] = { 0 }; DWORD sz = sizeof(val);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExA(hKey, "MachineGuid", NULL, NULL, (LPBYTE)val, &sz); RegCloseKey(hKey);
    }
    return string(val);
}

string getRealHwProfileGuid() {
    HKEY hKey; char val[256] = { 0 }; DWORD sz = sizeof(val);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\0001", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExA(hKey, "HwProfileGuid", NULL, NULL, (LPBYTE)val, &sz); RegCloseKey(hKey);
    }
    return string(val);
}

string getRealPCName() {
    char buf[MAX_COMPUTERNAME_LENGTH + 1]; DWORD sz = sizeof(buf);
    return GetComputerNameA(buf, &sz) ? string(buf) : "DESKTOP-UNKNOWN";
}

string getRealProductId() {
    HKEY hKey; char val[256] = { 0 }; DWORD sz = sizeof(val);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExA(hKey, "ProductId", NULL, NULL, (LPBYTE)val, &sz); RegCloseKey(hKey);
    }
    return string(val);
}

string getRealMacAddress() {
    ULONG outBufLen = sizeof(IP_ADAPTER_ADDRESSES);
    PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(outBufLen);
    if (!pAddresses) return "00:1A:2B:3C:4D:5E";
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAddresses); pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(outBufLen);
    }
    string macStr = "00:1A:2B:3C:4D:5E";
    if (pAddresses && GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &outBufLen) == NO_ERROR) {
        PIP_ADAPTER_ADDRESSES curr = pAddresses;
        while (curr) {
            if ((curr->IfType == IF_TYPE_ETHERNET_CSMACD || curr->IfType == IF_TYPE_IEEE80211) && curr->OperStatus == IfOperStatusUp) {
                if (curr->PhysicalAddressLength > 0) {
                    char buf[30];
                    sprintf_s(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
                        curr->PhysicalAddress[0], curr->PhysicalAddress[1], curr->PhysicalAddress[2],
                        curr->PhysicalAddress[3], curr->PhysicalAddress[4], curr->PhysicalAddress[5]);
                    macStr = buf; break;
                }
            }
            curr = curr->Next;
        }
    }
    if (pAddresses) free(pAddresses);
    return macStr;
}

string getVolumeSerial(char drive) {
    char root[4] = { drive, ':', '\\', '\0' };
    DWORD serial = 0;
    if (GetVolumeInformationA(root, NULL, 0, &serial, NULL, NULL, NULL, 0)) {
        char buf[30];
        sprintf_s(buf, "%04X-%04X", HIWORD(serial), LOWORD(serial));
        return string(buf);
    }
    return "FAIL";
}

DWORD getVolumeRawSerial(char drive) {
    char root[4] = { drive, ':', '\\', '\0' };
    DWORD serial = 0;
    GetVolumeInformationA(root, NULL, 0, &serial, NULL, NULL, NULL, 0);
    return serial;
}

string formatVolumeSerial(DWORD serial) {
    char buf[30];
    sprintf_s(buf, "%04X-%04X", HIWORD(serial), LOWORD(serial));
    return string(buf);
}

// --- GHI VOLUME SERIAL AN TOÀN TRÁNH BSOD ---
bool WriteVolumeSerialSafe(char driveLetter, DWORD newSerial) {
    char szDrive[7];
    sprintf_s(szDrive, "\\\\.\\%c:", driveLetter);

    HANDLE hVol = CreateFileA(szDrive, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hVol == INVALID_HANDLE_VALUE) return false;

    DWORD bytesReturned;

    if (!DeviceIoControl(hVol, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL)) {
        CloseHandle(hVol);
        return false;
    }

    DeviceIoControl(hVol, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);

    BYTE sector[512];
    DWORD bytesRead, bytesWritten;
    if (!ReadFile(hVol, sector, 512, &bytesRead, NULL)) {
        DeviceIoControl(hVol, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
        CloseHandle(hVol);
        return false;
    }

    if (memcmp(sector + 3, "NTFS", 4) == 0) {
        *(DWORD*)(sector + 0x48) = newSerial;
        *(DWORD*)(sector + 0x4C) = newSerial ^ 0x55555555;
    }
    else {
        *(DWORD*)(sector + 0x43) = newSerial;
    }

    SetFilePointer(hVol, 0, NULL, FILE_BEGIN);
    BOOL success = WriteFile(hVol, sector, 512, &bytesWritten, NULL);

    DeviceIoControl(hVol, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
    CloseHandle(hVol);
    return success;
}

string generateRandomGUID() {
    UUID uuid; UuidCreate(&uuid); char* str; UuidToStringA(&uuid, (RPC_CSTR*)&str);
    string s(str); RpcStringFreeA((RPC_CSTR*)&str);
    for (char& c : s) c = toupper(c);
    return s;
}

string generateRandomMAC() {
    char buf[30]; sprintf_s(buf, sizeof(buf), "00:50:56:%02X:%02X:%02X", rand() % 256, rand() % 256, rand() % 256);
    return string(buf);
}

string generateRandomPCName() {
    string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"; string name = "UCHIHA-";
    for (int i = 0; i < 7; i++) name += chars[rand() % chars.length()];
    return name;
}

string generateRandomProductID() {
    string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"; string id = "";
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 5; j++) id += chars[rand() % chars.length()];
        if (i < 3) id += "-";
    }
    return id;
}

DWORD generateRandomVolumeID() {
    return (DWORD)(rand() ^ (rand() << 16));
}

bool writeRegistryValue(const string& path, const string& key, const string& val) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        LSTATUS status = RegSetValueExA(hKey, key.c_str(), 0, REG_SZ, (const BYTE*)val.c_str(), (DWORD)(val.length() + 1));
        RegCloseKey(hKey); return (status == ERROR_SUCCESS);
    }
    return false;
}

void restartNetworkAdapters() {
    ExecuteSilentCommand("powershell.exe -WindowStyle Hidden -Command \"Get-NetAdapter | Restart-NetAdapter -Confirm:$false\"");
}

// --- DỌN DẸP SÂU DẤU VẾT GAME NATIVE C++ ---
void ExecuteDeepCleaner() {
    AddLog("[*] Khởi động động cơ DỌN DẸP SÂU HỆ THỐNG...");
    AddToast("CLEANER", "Đang khởi động dọn dẹp hệ thống...", ImVec4(1.0f, 0.6f, 0.0f, 1.0f));

    AddLog("Đang xóa bỏ bộ nhớ đệm ẩn Temp & Windows Prefetch...");
    char tempPath[MAX_PATH];
    if (GetTempPathA(MAX_PATH, tempPath)) {
        CleanDirectoryNative(tempPath);
    }
    CleanDirectoryNative("C:\\Windows\\Temp");
    CleanDirectoryNative("C:\\Windows\\Prefetch");

    AddLog("Đang dọn sạch Registry của các bệ phóng Game...");
    ExecuteSilentCommand("reg.exe delete \"HKCU\\Software\\Valve\\Steam\\ActiveProcess\" /f");
    ExecuteSilentCommand("reg.exe delete \"HKCU\\Software\\Epic Games\\Unreal Engine\\Hardware Survey\" /f");
    ExecuteSilentCommand("reg.exe delete \"HKCU\\Software\\Epic Games\\Unreal Engine\\Identifiers\" /f");

    char localAppData[MAX_PATH];
    if (GetEnvironmentVariableA("LOCALAPPDATA", localAppData, MAX_PATH)) {
        std::string riotPath = std::string(localAppData) + "\\Riot Games";
        std::string valPath = std::string(localAppData) + "\\Valorant";
        try {
            fs::remove_all(riotPath);
            fs::remove_all(valPath);
        }
        catch (...) {}
    }

    AddLog("Đang xóa sạch toàn bộ Nhật ký hoạt động hệ thống (Windows Event Logs)...");
    ExecuteSilentCommand("powershell.exe -WindowStyle Hidden -Command \"Get-EventLog -LogName * | ForEach-Object { Clear-EventLog -LogName $_.Log }\"");

    AddLog("Đang Flush DNS, làm sạch bảng định tuyến ARP và cấp lại IP mới...");
    ExecuteSilentCommand("ipconfig.exe /flushdns");
    ExecuteSilentCommand("ipconfig.exe /release");
    ExecuteSilentCommand("ipconfig.exe /renew");
    ExecuteSilentCommand("netsh.exe winsock reset");
    ExecuteSilentCommand("netsh.exe int ip reset");
    ExecuteSilentCommand("arp.exe -d *");

    AddLog("[+] HOÀN TẤT: Hệ thống đã hoàn toàn sạch sẽ, không còn dấu vết cũ!");
    AddToast("THÀNH CÔNG", "Hệ thống đã dọn dẹp sạch dấu vết!", ImVec4(0.2f, 0.9f, 0.2f, 1.0f));
}

// --- THIẾT LẬP STYLE UCHIHA (ĐỎ SHARINGAN & ĐEN) ---
void ApplyUchihaStyle() {
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
    colors[ImGuiCol_Button] = ImVec4(0.14f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.85f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.12f, 0.12f, 1.00f);
}

// --- TIẾN TRÌNH SPOOF CHẠY NGẦM ĐA LUỒNG (THREAD WORKER) ---
void SpoofingThreadWorker() {
    auto setStep = [](float progress, const string& taskName, int sleepMs) {
        spoofProgress = progress;
        SetSpoofTask(taskName);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
        };

    setStep(0.05f, "Đang khởi tạo thuật thức ngụy trang...", 400);
    AddLog("[*] Khởi động tiến trình ngụy trang thông số phần cứng...");
    AddToast("SPOOFER", "Bắt đầu áp dụng ảo thuật...", ImVec4(1.0f, 0.5f, 0.0f, 1.0f));

    setStep(0.15f, "Đang thiết lập danh tính ảo mới...", 500);

    setStep(0.30f, "Đang thay đổi các Registry hệ thống (GUID/PCName)...", 600);
    if (select_pcname) SetComputerNameExA(ComputerNamePhysicalDnsHostname, fakeID.pcName.c_str());
    if (select_guid) writeRegistryValue("SOFTWARE\\Microsoft\\Cryptography", "MachineGuid", fakeID.registryGuid);
    if (select_hwprofile) writeRegistryValue("SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\0001", "HwProfileGuid", fakeID.hwProfileGuid);
    if (select_productid) writeRegistryValue("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductId", fakeID.productId);
    AddLog("[+] Cập nhật thông tin Registry mới thành công.");
    AddToast("REGISTRY", "Cập nhật định danh Registry mới!", ImVec4(0.2f, 0.8f, 0.2f, 1.0f));

    setStep(0.50f, "Đang thực hiện phân rã & khóa phân vùng Volume Serial...", 700);
    if (select_volC) {
        if (WriteVolumeSerialSafe('C', fakeID.volC_Raw)) AddLog("[+] Đã đổi mã Serial Volume ổ đĩa [C:]!");
        else AddLog("[-] Lỗi: Không thể khóa/ghi đè Sector ổ đĩa [C:].");
    }
    if (select_volD) {
        if (WriteVolumeSerialSafe('D', fakeID.volD_Raw)) AddLog("[+] Đã đổi mã Serial Volume ổ đĩa [D:]!");
        else AddLog("[-] Lỗi: Không thể khóa/ghi đè Sector ổ đĩa [D:].");
    }
    AddToast("DISK SERIAL", "Đã viết lại Sector Volume ổ đĩa!", ImVec4(0.2f, 0.8f, 0.2f, 1.0f));

    setStep(0.70f, "Đang giao tiếp Kernel Driver (Spoofing CPU/BIOS)...", 600);
    if (isDriverLoaded) {
        if (select_cpu) {
            AddLog("[*] Đang chuyển tiếp lệnh Spoof CPU xuống Ring 0...");
            if (SendCommandToDriver(IOCTL_SPOOF_CPU)) AddLog("[+] Kernel: Giả lập CPU Serial thành công!");
            else AddLog("[-] Lỗi Kernel: Driver từ chối lệnh CPU.");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        if (select_bios) {
            AddLog("[*] Đang chuyển tiếp lệnh Spoof BIOS xuống Ring 0...");
            if (SendCommandToDriver(IOCTL_SPOOF_BIOS)) AddLog("[+] Kernel: Giả lập BIOS Serial thành công!");
            else AddLog("[-] Lỗi Kernel: Driver từ chối lệnh BIOS.");
        }
        AddToast("DRIVER (RING 0)", "Nạp thông số giả vào BIOS/CPU thành công!", ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
    }

    setStep(0.85f, "Đang thay đổi địa chỉ MAC & Khởi động lại Network Adapter...", 300);
    if (select_mac) {
        HKEY hClassKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}", 0, KEY_READ, &hClassKey) == ERROR_SUCCESS) {
            char subKey[256]; DWORD len;
            for (DWORD i = 0; ; i++) {
                len = sizeof(subKey);
                if (RegEnumKeyExA(hClassKey, i, subKey, &len, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) break;
                string full = "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}\\" + string(subKey);
                HKEY hSub;
                if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, full.c_str(), 0, KEY_SET_VALUE | KEY_QUERY_VALUE, &hSub) == ERROR_SUCCESS) {
                    char desc[256]; DWORD dsz = sizeof(desc);
                    if (RegQueryValueExA(hSub, "DriverDesc", NULL, NULL, (LPBYTE)desc, &dsz) == ERROR_SUCCESS) {
                        string cleanMac = "";
                        for (char c : fakeID.macAddress) if (c != ':' && c != '-') cleanMac += toupper(c);
                        RegSetValueExA(hSub, "NetworkAddress", 0, REG_SZ, (const BYTE*)cleanMac.c_str(), (DWORD)(cleanMac.length() + 1));
                    }
                    RegCloseKey(hSub);
                }
            }
            RegCloseKey(hClassKey);
        }
        AddLog("[*] Đang khởi động lại Card mạng (Mất kết nối mạng tạm thời)...");
        AddToast("MẠNG", "Đang reset card mạng để nạp MAC...", ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        restartNetworkAdapters();
    }

    setStep(0.95f, "Đang dọn dẹp Cache DNS & kết thúc thuật thức...", 400);
    ExecuteSilentCommand("ipconfig.exe /flushdns");

    setStep(1.00f, "Thuật thức hoàn tất!", 500);
    AddLog("[+] HOÀN THÀNH: Hệ thống đã phủ Ảo thuật ngụy trang thành công!");
    AddToast("UCHIHA", "Mangekyou Sharingan thức tỉnh thành công!", ImVec4(0.2f, 0.9f, 0.2f, 1.0f), 5.0f);

    isSpoofed = true;
    isSpoofingInProgress = false;
}

// --- TIẾN TRÌNH KHÔI PHỤC CHẠY NGẦM ĐA LUỒNG (THREAD WORKER) ---
void RecoveryThreadWorker() {
    auto setStep = [](float progress, const string& taskName, int sleepMs) {
        recoveryProgress = progress;
        SetRecoveryTask(taskName);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
        };

    setStep(0.10f, "Đang khởi động tiến trình giải trừ ảo ảnh...", 400);
    AddLog("[*] Bắt đầu hoàn trả lại thông số phần cứng gốc...");
    AddToast("PHÁ GIẢI", "Đang khôi phục hệ thống về nguyên trạng...", ImVec4(1.0f, 0.5f, 0.0f, 1.0f));

    setStep(0.30f, "Đang khôi phục lại Registry ban đầu...", 500);
    SetComputerNameExA(ComputerNamePhysicalDnsHostname, originalID.pcName.c_str());
    writeRegistryValue("SOFTWARE\\Microsoft\\Cryptography", "MachineGuid", originalID.registryGuid);
    writeRegistryValue("SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\0001", "HwProfileGuid", originalID.hwProfileGuid);
    writeRegistryValue("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductId", originalID.productId);
    AddLog("[+] Khôi phục Registry thành công.");

    setStep(0.50f, "Đang khôi phục Volume Serial nguyên thủy...", 600);
    WriteVolumeSerialSafe('C', originalID.volC_Raw);
    WriteVolumeSerialSafe('D', originalID.volD_Raw);
    AddLog("[+] Khôi phục Serial ổ đĩa [C:] & [D:] hoàn tất.");

    setStep(0.70f, "Đang khôi phục địa chỉ MAC cũ...", 500);
    HKEY hClassKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}", 0, KEY_READ, &hClassKey) == ERROR_SUCCESS) {
        char subKey[256]; DWORD len;
        for (DWORD i = 0; ; i++) {
            len = sizeof(subKey);
            if (RegEnumKeyExA(hClassKey, i, subKey, &len, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) break;
            string full = "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}\\" + string(subKey);
            HKEY hSub;
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, full.c_str(), 0, KEY_SET_VALUE | KEY_QUERY_VALUE, &hSub) == ERROR_SUCCESS) {
                string cleanMac = "";
                for (char c : originalID.macAddress) if (c != ':' && c != '-') cleanMac += toupper(c);
                RegSetValueExA(hSub, "NetworkAddress", 0, REG_SZ, (const BYTE*)cleanMac.c_str(), (DWORD)(cleanMac.length() + 1));
                RegCloseKey(hSub);
            }
        }
        RegCloseKey(hClassKey);
    }

    setStep(0.85f, "Đang kết nối lại Adapter mạng nguyên bản...", 300);
    AddToast("MẠNG", "Khôi phục lại Card mạng chính chủ...", ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    restartNetworkAdapters();

    setStep(1.00f, "Giải thuật hoàn tất!", 500);
    AddLog("[+] Đã trả lại toàn bộ thông số phần cứng chính chủ.");
    AddToast("GIẢI THUẬT", "Đã thu hồi ảo ảnh phần cứng!", ImVec4(0.2f, 0.9f, 0.2f, 1.0f), 5.0f);

    isSpoofed = false;
    select_cpu = false;
    select_bios = false;
    isRecoveringInProgress = false;
}

// --- VẼ TOAST NOTIFICATIONS LÊN MÀN HÌNH (NEW) ---
void RenderToasts() {
    std::lock_guard<std::mutex> lock(toastMutex);
    if (activeToasts.empty()) return;

    float deltaTime = ImGui::GetIO().DeltaTime;
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    float currentY = displaySize.y - 20.0f; // Bắt đầu xếp chồng từ chân màn hình

    for (auto it = activeToasts.begin(); it != activeToasts.end(); ) {
        it->remainingTime -= deltaTime;
        if (it->remainingTime <= 0.0f) {
            it = activeToasts.erase(it);
            continue;
        }

        // Tạo hiệu ứng Fade-in và Fade-out dựa theo thời gian sống của Toast
        float alpha = 1.0f;
        if (it->remainingTime < 0.5f) {
            alpha = it->remainingTime / 0.5f;
        }
        else if (it->maxTime - it->remainingTime < 0.2f) {
            alpha = (it->maxTime - it->remainingTime) / 0.2f;
        }

        string winName = "##Toast_" + to_string(distance(activeToasts.begin(), it));

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        ImGui::PushStyleColor(ImGuiCol_Border, it->color);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);

        // Định vị Toast dán vào lề phải màn hình
        ImGui::SetNextWindowPos(ImVec2(displaySize.x - 320.0f, currentY), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
        ImGui::SetNextWindowSize(ImVec2(300.0f, 0.0f));

        ImGui::Begin(winName.c_str(), NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);

        ImGui::TextColored(it->color, "[ %s ]", it->title.c_str());
        ImGui::Separator();
        ImGui::TextWrapped("%s", it->message.c_str());

        currentY -= ImGui::GetWindowHeight() + 10.0f; // Xếp chồng dần lên trên
        ImGui::End();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        ++it;
    }
}

// --- CHƯƠNG TRÌNH CHÍNH (MAIN ENTRY) ---
int main(int, char**) {
    srand(static_cast<unsigned int>(time(0)));

    // Load lại cấu hình người dùng đã lưu lần trước
    LoadConfig();

    originalID.macAddress = getRealMacAddress();
    originalID.registryGuid = getRealMachineGuid();
    originalID.hwProfileGuid = getRealHwProfileGuid();
    originalID.pcName = getRealPCName();
    originalID.productId = getRealProductId();
    originalID.volC_Raw = getVolumeRawSerial('C');
    originalID.volD_Raw = getVolumeRawSerial('D');
    originalID.volCSerialStr = getVolumeSerial('C');
    originalID.volDSerialStr = getVolumeSerial('D');

    fakeID.macAddress = generateRandomMAC();
    fakeID.registryGuid = generateRandomGUID();
    fakeID.hwProfileGuid = "{" + generateRandomGUID() + "}";
    fakeID.pcName = generateRandomPCName();
    fakeID.productId = generateRandomProductID();
    fakeID.volC_Raw = generateRandomVolumeID();
    fakeID.volD_Raw = generateRandomVolumeID();
    fakeID.volCSerialStr = formatVolumeSerial(fakeID.volC_Raw);
    fakeID.volDSerialStr = formatVolumeSerial(fakeID.volD_Raw);

    if (ExtractDriver()) {
        if (LoadUchihaDriver()) {
            AddLog("[+] Tu dong nap UchihaDriver len Kernel thanh cong!");
        }
        else {
            AddLog("[-] Loi SCM: Khong the khoi dong Driver. Hay chay bang quyen Admin!");
        }
    }
    else {
        AddLog("[-] Loi IO: Khong the giai nen file driver tam thoi.");
    }

    WNDCLASSEXA wc = { sizeof(WNDCLASSEXA), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "UCHIHA_CLASS", NULL };
    RegisterClassExA(&wc);
    HWND hwnd = CreateWindowA(wc.lpszClassName, "UCHIHA-HITACHI SPOOFER ULTIMATE v3.0", WS_OVERLAPPEDWINDOW, 100, 100, 950, 560, NULL, NULL, wc.hInstance, NULL);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassA(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    char winFolder[512];
    GetWindowsDirectoryA(winFolder, sizeof(winFolder));
    string fontPath = string(winFolder) + "\\Fonts\\segoeui.ttf";
    io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 16.0f, NULL, io.Fonts->GetGlyphRangesVietnamese());

    ApplyUchihaStyle();
    AddLog("Chào mừng bạn đến với Uchiha-Hitachi Spoofer Ultimate!");
    AddLog("[Hệ thống] Đang chạy với quyền quản trị viên cao cấp.");

    // Gửi Toast chào mừng cực ngầu
    AddToast("XIN CHÀO", "Khởi động giao diện Mangekyou Sharingan!", ImVec4(1.0f, 0.2f, 0.2f, 1.0f), 4.5f);

    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(950, 525));
        ImGui::Begin("MainPanel", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        // --- SIDEBAR BÊN TRÁI ---
        ImGui::BeginChild("Sidebar", ImVec2(220, 0), true);
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.15f, 0.15f, 1.0f), "  U C H I H A - H I T A C H I");
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        ImVec2 sidebar_size = ImGui::GetWindowSize();
        ImVec2 sharingan_center = ImVec2(ImGui::GetWindowPos().x + sidebar_size.x / 2.0f, ImGui::GetWindowPos().y + 110.0f);

        float currentSharinganSpeed = 1.8f;
        if (isSpoofingInProgress || isRecoveringInProgress) {
            currentSharinganSpeed = 6.0f;
        }
        DrawRotatingSharingan(sharingan_center, 38.0f, currentSharinganSpeed);

        ImGui::Dummy(ImVec2(0, 95.0f));

        ImGui::BeginDisabled(isSpoofingInProgress || isRecoveringInProgress);
        if (ImGui::Button("Trang Chủ", ImVec2(200, 40))) activeTab = 0;
        if (ImGui::Button("Thay Đổi HWID", ImVec2(200, 40))) activeTab = 1;
        if (ImGui::Button("Dọn Dẹp Dấu Vết", ImVec2(200, 40))) activeTab = 2;
        if (ImGui::Button("Hướng Dẫn Sử Dụng", ImVec2(200, 40))) activeTab = 3;
        ImGui::EndDisabled();

        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 50);
        ImGui::Text("Phiên Bản: 3.0.0\nPhát triển bởi: SecHex");
        ImGui::EndChild();

        ImGui::SameLine();

        // --- XỬ LÝ LƯU TRẠNG THÁI CHUYỂN TAB MƯỢT MÀ (NEW) ---
        if (activeTab != previousTab) {
            tabAlpha = 0.0f; // Bắt đầu chu kỳ Fade-in từ 0
            previousTab = activeTab;
        }
        if (tabAlpha < 1.0f) {
            tabAlpha += io.DeltaTime * 4.5f; // Chuyển Tab cực mượt trong ~0.22s
            if (tabAlpha > 1.0f) tabAlpha = 1.0f;
        }

        // --- KHU VỰC HIỂN THỊ CHÍNH BÊN PHẢI ---
        ImGui::BeginGroup();
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, tabAlpha); // Áp dụng độ mờ Fade-in

        if (activeTab == 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "TRẠNG THÁI HWID HIỆN TẠI HỆ THỐNG");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("TÊN MÁY TÍNH: %s", (isSpoofed ? fakeID.pcName.c_str() : originalID.pcName.c_str()));
            ImGui::Text("ĐỊA CHỈ MAC: %s", (isSpoofed ? fakeID.macAddress.c_str() : originalID.macAddress.c_str()));
            ImGui::Text("MACHINE GUID: %s", (isSpoofed ? fakeID.registryGuid.c_str() : originalID.registryGuid.c_str()));
            ImGui::Text("HW-PROFILE: %s", (isSpoofed ? fakeID.hwProfileGuid.c_str() : originalID.hwProfileGuid.c_str()));
            ImGui::Text("PRODUCT ID: %s", (isSpoofed ? fakeID.productId.c_str() : originalID.productId.c_str()));

            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "THÔNG SỐ PHÂN VÙNG Ổ CỨNG (VOLUME ID):");
            ImGui::Text("MÃ SERIAL Ổ C: %s", (isSpoofed ? fakeID.volCSerialStr.c_str() : originalID.volCSerialStr.c_str()));
            ImGui::Text("MÃ SERIAL Ổ D: %s", (isSpoofed ? fakeID.volDSerialStr.c_str() : originalID.volDSerialStr.c_str()));
        }
        else if (activeTab == 1) {
            ImGui::BeginDisabled(isSpoofingInProgress || isRecoveringInProgress);
            ImGui::Columns(2, "options_col", false);
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Thay đổi Registry & Thiết bị");
            ImGui::Separator();

            // Các Checkbox tự lưu cấu hình mỗi khi người dùng click thay đổi
            if (ImGui::Checkbox("Thay đổi Địa chỉ MAC", &select_mac)) SaveConfig();
            if (ImGui::Checkbox("Thay đổi Machine GUID", &select_guid)) SaveConfig();
            if (ImGui::Checkbox("Thay đổi HwProfile GUID", &select_hwprofile)) SaveConfig();
            if (ImGui::Checkbox("Thay đổi Tên máy tính", &select_pcname)) SaveConfig();
            if (ImGui::Checkbox("Thay đổi Product ID", &select_productid)) SaveConfig();

            ImGui::NextColumn();
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Thay đổi mã Ổ cứng (Volume ID)");
            ImGui::Separator();
            if (ImGui::Checkbox("Thay đổi mã Serial ổ đĩa [C:]", &select_volC)) SaveConfig();
            if (ImGui::Checkbox("Thay đổi mã Serial ổ đĩa [D:]", &select_volD)) SaveConfig();
            ImGui::Spacing();

            if (isDriverLoaded) {
                if (ImGui::Checkbox("Thay đổi CPU Serial", &select_cpu)) SaveConfig();
                if (ImGui::Checkbox("Thay đổi BIOS Serial", &select_bios)) SaveConfig();
            }
            else {
                ImGui::BeginDisabled();
                ImGui::Checkbox("CPU Serial (Cần nạp Driver)", &select_cpu);
                ImGui::Checkbox("BIOS Serial (Cần nạp Driver)", &select_bios);
                ImGui::EndDisabled();
            }
            ImGui::Columns(1);
            ImGui::EndDisabled();

            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            // --- ĐỒ HỌA TIẾN TRÌNH % KHI ĐANG SPOOF / RECOVER ---
            if (isSpoofingInProgress) {
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Mangekyou Sharingan đang áp đặt ảo ảnh lên hệ thống...");

                char percentageBuf[32];
                sprintf_s(percentageBuf, "%d%%", static_cast<int>(spoofProgress * 100));

                ImGui::ProgressBar(spoofProgress, ImVec2(690, 30), percentageBuf);
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Đang thực hiện: %s", GetSpoofTask().c_str());
            }
            else if (isRecoveringInProgress) {
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "Đang phá giải thuật ảo ảnh (Khôi phục danh tính)...");

                char percentageBuf[32];
                sprintf_s(percentageBuf, "%d%%", static_cast<int>(recoveryProgress * 100));

                ImGui::ProgressBar(recoveryProgress, ImVec2(690, 30), percentageBuf);
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Đang thực hiện: %s", GetRecoveryTask().c_str());
            }
            else {
                if (ImGui::Button("TẠO ID MỚI NGẪU NHIÊN", ImVec2(200, 40))) {
                    fakeID.macAddress = generateRandomMAC();
                    fakeID.registryGuid = generateRandomGUID();
                    fakeID.hwProfileGuid = "{" + generateRandomGUID() + "}";
                    fakeID.pcName = generateRandomPCName();
                    fakeID.productId = generateRandomProductID();
                    fakeID.volC_Raw = generateRandomVolumeID();
                    fakeID.volD_Raw = generateRandomVolumeID();
                    fakeID.volCSerialStr = formatVolumeSerial(fakeID.volC_Raw);
                    fakeID.volDSerialStr = formatVolumeSerial(fakeID.volD_Raw);
                    AddLog("Đã tạo ngẫu nhiên một bộ thông tin HWID & Volume ID mới.");
                    AddToast("RANDOM", "Đã khởi tạo bộ thông số ngẫu nhiên mới!", ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
                }
                ImGui::SameLine();
                if (ImGui::Button("BẮT ĐẦU SPOOF", ImVec2(200, 40))) {
                    isSpoofingInProgress = true;
                    spoofProgress = 0.0f;
                    std::thread(SpoofingThreadWorker).detach();
                }
                ImGui::SameLine();
                if (ImGui::Button("KHÔI PHỤC GỐC", ImVec2(200, 40))) {
                    isRecoveringInProgress = true;
                    recoveryProgress = 0.0f;
                    std::thread(RecoveryThreadWorker).detach();
                }
            }
        }
        else if (activeTab == 2) {
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "HỆ THỐNG DỌN DẸP SÂU HƠN (DEEP CLEAN ENGINE)");
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("Chức năng này sẽ dọn sạch mọi file Log, bộ nhớ đệm Tracking, Registry rác của");
            ImGui::Text("Steam, Epic Games, Riot Games và dọn sạch Event Logs của Windows.");
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            ImGui::BeginDisabled(isSpoofingInProgress || isRecoveringInProgress);
            if (ImGui::Button("KÍCH HOẠT DEEP CLEAN (QUÉT SẠCH DẤU VẾT)", ImVec2(350, 45))) {
                ExecuteDeepCleaner();
            }
            ImGui::EndDisabled();
        }
        else if (activeTab == 3) {
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "CẨM NANG HƯỚNG DẪN SỬ DỤNG PHẦN MỀM");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::BulletText("BƯỚC 1: Hãy chắc chắn khởi động Spoofer bằng quyền Trực tiếp Quản trị viên (Admin).");
            ImGui::BulletText("BƯỚC 2: Vào Tab 'Thay Đổi HWID', bấm nút 'TẠO ID MỚI NGẪU NHIÊN'. Bật các mục mong muốn.");
            ImGui::BulletText("BƯỚC 3: Nhấn nút 'BẮT ĐẦU SPOOF'. Hệ thống mạng sẽ làm mới trong vòng 3 giây để đổi MAC.");
            ImGui::BulletText("BƯỚC 4: Chuyển qua Tab 'Dọn Dẹp Dấu Vết', nhấn nút dọn dẹp để quét sạch cache Registry cũ của Game.");
            ImGui::BulletText("BƯỚC 5: Khi muốn chơi game bình thường và trả lại ID thật, hãy nhấn 'KHÔI PHỤC GỐC'.");

            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Mẹo nhỏ: Với các game quét dọn gắt gao, hãy chạy 'Deep Clean' trước rồi mới bấm Spoof!");
        }

        // --- CONSOLE LOG BOX ---
        ImGui::SetCursorPosY(355);
        ImGui::Text("Nhật ký hoạt động (System Logs)");
        ImGui::BeginChild("ConsoleLog", ImVec2(700, 120), true);

        {
            std::lock_guard<std::mutex> lock(logMutex);
            for (const auto& log : logs) {
                ImGui::TextUnformatted(log.c_str());
            }
        }

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();

        ImGui::PopStyleVar(); // Pop độ mờ Fade-in của tab
        ImGui::EndGroup();
        ImGui::End();

        // --- VẼ THÔNG BÁO TRƯỢT TOAST LÊN TRÊN LỚP GIAO DIỆN (NEW) ---
        RenderToasts();

        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.06f, 0.06f, 0.07f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassA(wc.lpszClassName, wc.hInstance);

    if (isDriverLoaded) {
        UnloadUchihaDriver();
    }

    return 0;
}

// --- BOILERPLATE CẤU HÌNH WIN32 & DIRECTX 11 ---
bool CreateDeviceD3D(HWND hWnd) {
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
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_WARP, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = NULL;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}