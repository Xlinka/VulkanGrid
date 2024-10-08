#pragma once
#include <string>
#include <vector>

class FileUtils {
public:
    static std::vector<char> readFile(const std::string& filepath);
};
