#include "SystemInfo.h"
#include <iostream>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#include <intrin.h>
#include <VersionHelpers.h>
#include <dxgi1_6.h>
#include <atlbase.h>
#pragma comment(lib, "dxgi.lib")

namespace {
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
std::string readFileContent(const std::string& path) {
    std::ifstream file(path);
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}
#endif

namespace {
    double BytesToGB(uint64_t bytes) {
        return static_cast<double>(bytes) / (1024 * 1024 * 1024);
    }
}

// Get the name of the OS
std::string SystemInfo::getOSName() {
#ifdef _WIN32
    if (IsWindows10OrGreater()) {
        HKEY hKey;
        LONG lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey);
        if (lRes == ERROR_SUCCESS) {
            wchar_t value[256];
            DWORD valueSize = sizeof(value);
            LONG queryRes = RegQueryValueExW(hKey, L"ProductName", NULL, NULL, (LPBYTE)value, &valueSize);
            RegCloseKey(hKey);
            if (queryRes == ERROR_SUCCESS) {
                return WStringToString(value);
            }
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
std::string SystemInfo::getCPUName() {
#ifdef _WIN32
    int cpuInfo[4] = { -1 };
    char cpuBrandString[0x40];
    __cpuid(cpuInfo, 0x80000000);
    unsigned int nExIds = cpuInfo[0];
    memset(cpuBrandString, 0, sizeof(cpuBrandString));

    for (unsigned int i = 0x80000000; i <= nExIds; ++i) {
        __cpuid(cpuInfo, i);
        if (i == 0x80000002)
            memcpy(cpuBrandString, cpuInfo, sizeof(cpuInfo));
        else if (i == 0x80000003)
            memcpy(cpuBrandString + 16, cpuInfo, sizeof(cpuInfo));
        else if (i == 0x80000004)
            memcpy(cpuBrandString + 32, cpuInfo, sizeof(cpuInfo));
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
double SystemInfo::getAvailableRAM() {
#ifdef _WIN32
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    return BytesToGB(statex.ullAvailPhys);
#else
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    return BytesToGB(memInfo.freeram * memInfo.mem_unit);
#endif
}

// Get usable RAM in GB
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
std::string SystemInfo::getGPUName() {
#ifdef _WIN32
    CComPtr<IDXGIFactory6> pFactory;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory6), (void**)&pFactory);
    if (FAILED(hr)) return "Failed to create DXGI Factory.";

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
double SystemInfo::getGPUVRAM() {
#ifdef _WIN32
    CComPtr<IDXGIFactory6> pFactory;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory6), (void**)&pFactory);
    if (FAILED(hr)) return 0.0;

    CComPtr<IDXGIAdapter1> pAdapter;
    hr = pFactory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&pAdapter));
    if (FAILED(hr)) return 0.0;

    DXGI_ADAPTER_DESC1 desc;
    hr = pAdapter->GetDesc1(&desc);
    if (FAILED(hr)) return 0.0;

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