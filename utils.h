#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <string_view>
#include <vector>

// Logger class declaration
class Logger {
   private:
    static std::ofstream logFile;
    static bool initialized;
    static std::string logFileName;

    static void initialize();

   public:
    enum class Level { DEBUG, INFO, ERROR, CRITICAL };

    static void log(Level level, const std::string& message);
    static void info(const char* message);
    static void error(const char* message);
    static void critical(const char* message);
    static void close();
    static void setLogFileName(const std::string& filename);

    template <typename... Args>
    static void info(const char* format, Args... args) {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), format, args...);
        log(Level::INFO, buffer);
    }

    template <typename... Args>
    static void error(const char* format, Args... args) {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), format, args...);
        log(Level::ERROR, buffer);
    }

    template <typename... Args>
    static void critical(const char* format, Args... args) {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), format, args...);
        log(Level::CRITICAL, buffer);
    }
};

// Network constants
constexpr std::string_view SERVER_IP = "127.0.0.1";
constexpr int SERVER_PORT = 5200;
constexpr int BUFFER_SIZE = 10240;

// File constants
constexpr std::string_view INPUT_FILE = "test.jpg";
constexpr std::string_view COMPRESSED_FILE = "compressed.jpeg";
constexpr std::string_view CODE_FILE = "code.txt";
constexpr std::string_view FOR_DECOMPRESSED_FILE = "for_decompressed.txt";
constexpr std::string_view DECODED_FILE = "decoded.jpeg";