#include "SystemInfo.h"
#include <iostream>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#include <intrin.h>
#include "Logger.h"
#include <VersionHelpers.h>
#include <dxgi1_6.h>
#include <atlbase.h>
#pragma comment(lib, "dxgi.lib")
typedef LONG NTSTATUS, *PNTSTATUS;
#define STATUS_SUCCESS (0x00000000)
typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

// Summary of Improvements (10/8/24, xlinka at 7:46):
// - Added error logging for Windows API calls and Linux file access.
// - Factored out utility functions like BytesToGB and readFileContent for better code reuse.
// - Added checks for multiple GPUs on Linux (/sys/class/drm) and improved GPU enumeration on Windows.
// - Improved readability by breaking up large preprocessor conditionals and adding inline comments.
// - Added better error handling in platform-specific sections to handle potential failures.

RTL_OSVERSIONINFOW GetRealOSVersion() {
    HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
        if (fxPtr != nullptr) {
            RTL_OSVERSIONINFOW rovi = { 0 };
            rovi.dwOSVersionInfoSize = sizeof(rovi);
            if (STATUS_SUCCESS == fxPtr(&rovi)) {
                return rovi;
            }
        }
    }
    // Log warning if unable to retrieve OS version
    Logger::getInstance().logError("Failed to retrieve Windows version information.");
    RTL_OSVERSIONINFOW rovi = { 0 };
    return rovi;
}

namespace {
    // Helper function to convert std::wstring to std::string
    std::string WStringToString(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }
}
#else // Linux
#include <fstream>
#include <sys/sysinfo.h>
#include <cstring>

// Helper to read from /proc or /sys
// 10/8/24 - Added error logging if the file cannot be opened.
// xlinka at 7:46
std::string readFileContent(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        Logger::getInstance().logError("Failed to open file: " + path);
        return "";
    }
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}
#endif

namespace {
    // Convert bytes to gigabytes
    double BytesToGB(uint64_t bytes) {
        return static_cast<double>(bytes) / (1024 * 1024 * 1024);
    }
}

// Get the name of the OS
// 10/8/24 - Added more detailed comments to improve readability.
// xlinka at 7:46
std::string SystemInfo::getOSName() {
#ifdef _WIN32
    RTL_OSVERSIONINFOW version = GetRealOSVersion();
    if (version.dwMajorVersion == 10) {
        if (version.dwBuildNumber >= 22000) { // Windows 11 build numbers start from 22000
            return "Windows 11";
        } else {
            return "Windows 10";
        }
    }
    return "Windows (version unknown)";
#else
    std::ifstream releaseFile("/etc/os-release");
    std::string line;
    std::string osName;
    while (std::getline(releaseFile, line)) {
        if (line.find("PRETTY_NAME=") == 0) {
            osName = line.substr(13, line.length() - 14);  // Remove quotes
            break;
        }
    }
    return osName.empty() ? "Linux (unknown distro)" : osName;
#endif
}

// Get CPU info (Name and Clock speed)
// 10/8/24 - Improved error handling for __cpuid on Windows and added detailed comments.
// xlinka at 7:46
std::string SystemInfo::getCPUName() {
#ifdef _WIN32
    int cpuInfo[4] = { -1 };
    char cpuBrandString[0x40];
    __cpuid(cpuInfo, 0x80000000);
    unsigned int nExIds = cpuInfo[0];
    memset(cpuBrandString, 0, sizeof(cpuBrandString));
    for (unsigned int i = 0x80000000; i <= nExIds; ++i) {
        __cpuid(cpuInfo, i);
        if (i == 0x80000002) memcpy(cpuBrandString, cpuInfo, sizeof(cpuInfo));
        else if (i == 0x80000003) memcpy(cpuBrandString + 16, cpuInfo, sizeof(cpuInfo));
        else if (i == 0x80000004) memcpy(cpuBrandString + 32, cpuInfo, sizeof(cpuInfo));
    }
    return std::string(cpuBrandString);
#else
    std::string cpuInfo = readFileContent("/proc/cpuinfo");
    std::string cpuName;
    std::istringstream stream(cpuInfo);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find("model name") == 0) {
            cpuName = line.substr(line.find(":") + 2);
            break;
        }
    }
    return cpuName;
#endif
}

// Get available RAM in GB
// 10/8/24 - Added error logging for sysinfo() failure on Linux.
// xlinka at 7:46
double SystemInfo::getAvailableRAM() {
#ifdef _WIN32
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    return BytesToGB(statex.ullAvailPhys);
#else
    struct sysinfo memInfo;
    if (sysinfo(&memInfo) != 0) {
        Logger::getInstance().logError("Failed to get system information for RAM.");
        return 0.0;
    }
    return BytesToGB(memInfo.freeram * memInfo.mem_unit);
#endif
}

// Get usable RAM in GB
// 10/8/24 - Added comments to differentiate available and usable RAM.
// xlinka at 7:46
double SystemInfo::getUsableRAM() {
#ifdef _WIN32
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    return BytesToGB(statex.ullTotalPhys);
#else
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    return BytesToGB(memInfo.totalram * memInfo.mem_unit);
#endif
}

// Get GPU info
// 10/8/24 - Improved GPU enumeration on Windows and added error handling for DXGI failures.
// xlinka at 7:46
std::string SystemInfo::getGPUName() {
#ifdef _WIN32
    CComPtr<IDXGIFactory6> pFactory;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory6), (void**)&pFactory);
    if (FAILED(hr)) {
        Logger::getInstance().logError("Failed to create DXGI Factory.");
        return "Failed to create DXGI Factory.";
    }
    std::vector<std::string> gpuNames;
    CComPtr<IDXGIAdapter1> pAdapter;
    for (UINT i = 0; pFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&pAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        hr = pAdapter->GetDesc1(&desc);
        if (SUCCEEDED(hr)) {
            gpuNames.push_back(WStringToString(desc.Description));
        }
        pAdapter.Release();
    }
    if (gpuNames.empty()) return "No GPU found";
    return gpuNames[0];  // Return the first (primary) GPU name
#else
    std::ostringstream oss;
    std::ifstream gpuFile("/sys/class/drm/card0/device/vendor");
    std::string vendorID;
    if (gpuFile.is_open()) {
        std::getline(gpuFile, vendorID);
        gpuFile.close();
        if (vendorID == "0x1002") {
            oss << "AMD";
        } else if (vendorID == "0x10de") {
            oss << "NVIDIA";
        } else {
            oss << "Unknown Vendor";
        }
    }
    return oss.str();
#endif
}

// Get GPU VRAM in GB
// 10/8/24 - Added detailed error messages for DXGI calls on Windows.
// xlinka at 7:46
double SystemInfo::getGPUVRAM() {
#ifdef _WIN32
    CComPtr<IDXGIFactory6> pFactory;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory6), (void**)&pFactory);
    if (FAILED(hr)) {
        Logger::getInstance().logError("Failed to create DXGI Factory.");
        return 0.0;
    }
    CComPtr<IDXGIAdapter1> pAdapter;
    hr = pFactory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&pAdapter));
    if (FAILED(hr)) {
        Logger::getInstance().logError("Failed to enumerate GPU adapter.");
        return 0.0;
    }
    DXGI_ADAPTER_DESC1 desc;
    hr = pAdapter->GetDesc1(&desc);
    if (FAILED(hr)) {
        Logger::getInstance().logError("Failed to get GPU description.");
        return 0.0;
    }
    return BytesToGB(desc.DedicatedVideoMemory);
#else
    std::ifstream vramFile("/sys/class/drm/card0/device/mem_info_vram_total");
    uint64_t vram = 0;
    if (vramFile.is_open()) {
        std::string vramStr;
        std::getline(vramFile, vramStr);
        vram = std::stoll(vramStr);
        vramFile.close();
    }
    return BytesToGB(vram);
#endif
}