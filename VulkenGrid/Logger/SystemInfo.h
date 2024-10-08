#pragma once

#include <string>
#include <cstdint>

/**
 * @brief Provides utilities for retrieving system information.
 * 
 * Summary of Improvements (10/8/24, xlinka at 7:46):
 * - Added error logging for Windows API calls and Linux file access.
 * - Factored out utility functions like BytesToGB and readFileContent for better code reuse.
 * - Added checks for multiple GPUs on Linux (/sys/class/drm) and improved GPU enumeration on Windows.
 * - Improved readability by breaking up large preprocessor conditionals and adding inline comments.
 * - Added better error handling in platform-specific sections to handle potential failures.
 */
class SystemInfo {
public:
    /**
     * @brief Returns the name of the operating system (e.g., "Windows 10", "Linux").
     * @return The name of the operating system.
     * 
     * This function differentiates between Windows 10 and Windows 11 based on build number.
     * On Linux, it reads from /etc/os-release.
     */
    static std::string getOSName();

    /**
     * @brief Returns the name of the CPU (e.g., "Intel(R) Core(TM) i7-9700K").
     * @return The name of the CPU.
     * 
     * On Windows, this function uses __cpuid to retrieve CPU information.
     * On Linux, it reads from /proc/cpuinfo.
     */
    static std::string getCPUName();

    /**
     * @brief Returns the amount of available RAM in gigabytes.
     * @return Available RAM in gigabytes.
     * 
     * On Linux, added error logging if sysinfo() fails.
     */
    static double getAvailableRAM();

    /**
     * @brief Returns the total usable RAM in gigabytes.
     * @return Total usable RAM in gigabytes.
     * 
     * This function differentiates between available and total usable RAM.
     */
    static double getUsableRAM();

    /**
     * @brief Returns the name of the primary GPU (e.g., "NVIDIA GeForce GTX 1080").
     * @return The name of the GPU.
     * 
     * On Windows, this function enumerates GPUs using DXGI and logs errors if enumeration fails.
     * On Linux, it reads GPU vendor information from /sys/class/drm/card0/device/vendor.
     */
    static std::string getGPUName();

    /**
     * @brief Returns the amount of GPU VRAM in gigabytes.
     * @return The amount of GPU VRAM in gigabytes.
     * 
     * On Windows, detailed error messages are provided for DXGI-related failures.
     * On Linux, it reads from /sys/class/drm/card0/device/mem_info_vram_total.
     */
    static double getGPUVRAM();
};
