#pragma once

#include <string>
#include <cstdint>

class SystemInfo {
public:
    // Returns the name of the operating system (e.g., "Windows", "Linux").
    static std::string getOSName();

    // Returns the name of the CPU (e.g., "Intel(R) Core(TM) i7-9700K").
    static std::string getCPUName();

    // Returns the amount of available RAM in gigabytes.
    static double getAvailableRAM();

    // Returns the total usable RAM in gigabytes.
    static double getUsableRAM();

    // Returns the name of the GPU (e.g., "NVIDIA GeForce GTX 1080").
    static std::string getGPUName();

    // Returns the amount of GPU VRAM in gigabytes.
    static double getGPUVRAM();
};
