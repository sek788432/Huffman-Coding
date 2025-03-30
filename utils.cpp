#include "utils.h"

// Initialize static members
std::ofstream Logger::logFile;
bool Logger::initialized = false;
std::string Logger::logFileName;

void Logger::setLogFileName(const std::string& filename) {
    logFileName = filename;
}

void Logger::initialize() {
    if (!initialized) {
        logFile.open(logFileName, std::ios::app);
        if (!logFile) {
            std::cerr << "Failed to open log file" << std::endl;
        }
        initialized = true;
    }
}

void Logger::log(Level level, const std::string& message) {
    initialize();
    std::time_t now = std::time(nullptr);
    std::string timeStr = std::ctime(&now);
    timeStr.pop_back();  // Remove newline

    std::string levelStr;
    switch (level) {
        case Level::DEBUG:
            levelStr = "DEBUG";
            break;
        case Level::INFO:
            levelStr = "INFO";
            break;
        case Level::ERROR:
            levelStr = "ERROR";
            break;
        case Level::CRITICAL:
            levelStr = "CRITICAL";
            break;
    }

    std::string logMessage =
        "[" + timeStr + "] [" + levelStr + "] " + message + "\n";
    std::cout << logMessage;
    if (logFile.is_open()) {
        logFile << logMessage;
        logFile.flush();
    }
}

void Logger::info(const char* message) { log(Level::INFO, message); }

void Logger::error(const char* message) { log(Level::ERROR, message); }

void Logger::critical(const char* message) { log(Level::CRITICAL, message); }

void Logger::close() {
    if (logFile.is_open()) {
        logFile.close();
    }
}