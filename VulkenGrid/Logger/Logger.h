#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <ctime>
#include <mutex>

class Logger {
public:
    static Logger& getInstance();
    
    void log(const std::string& message);
    void logError(const std::string& message);

private:
    Logger();
    ~Logger();

    std::ofstream logFile;
    std::mutex logMutex;

    std::string getTimestamp();
    void writeLog(const std::string& level, const std::string& message);
};
