#include "Logger.h"
#include <filesystem>

Logger::Logger() {
    std::filesystem::create_directory("logs");
    std::string filename = "logs/VulkanGrid_" + getTimestamp() + ".log";
    logFile.open(filename, std::ios::out | std::ios::app);
    if (!logFile.is_open()) {
        throw std::runtime_error("Failed to open log file.");
    }
    log("Logger initialized.");
}

Logger::~Logger() {
    log("Logger shutting down.");
    if (logFile.is_open()) {
        logFile.close();
    }
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::log(const std::string& message) {
    writeLog("INFO", message);
}

void Logger::logError(const std::string& message) {
    writeLog("ERROR", message);
}

std::string Logger::getTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    std::stringstream ss;
    ss << (localTime->tm_year + 1900) << "-"
       << (localTime->tm_mon + 1) << "-"
       << localTime->tm_mday << "_"
       << localTime->tm_hour << "-"
       << localTime->tm_min << "-"
       << localTime->tm_sec;
    return ss.str();
}

void Logger::writeLog(const std::string& level, const std::string& message) {
    std::lock_guard<std::mutex> guard(logMutex);
    logFile << "[" << getTimestamp() << "] [" << level << "] " << message << std::endl;
}
