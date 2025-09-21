#pragma once
#include <Windows.h>
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <sstream>

namespace EvasionLogger {
    
    enum LogLevel {
        LOG_SUCCESS = 0,
        LOG_WARNING = 1,
        LOG_ERROR = 2,
        LOG_DEBUG = 3
    };
    
    class Logger {
    public:
        // Initialize logger (console + file)
        static bool Initialize(bool enableConsole = true, bool enableFile = false);
        
        // Log with different levels
        static void LogSuccess(const std::string& technique, const std::string& message);
        static void LogWarning(const std::string& technique, const std::string& message);
        static void LogError(const std::string& technique, const std::string& message);
        static void LogDebug(const std::string& technique, const std::string& message);
        
        // Log with custom formatting
        static void Log(LogLevel level, const std::string& technique, const std::string& message);
        
        // Get evasion status summary
        static std::string GetStatusSummary();
        
        // Cleanup
        static void Cleanup();
        
    private:
        static std::string GetTimestamp();
        static std::string GetLevelString(LogLevel level);
        static std::string GetLevelColor(LogLevel level);
        
        static bool g_consoleEnabled;
        static bool g_fileEnabled;
        static std::ofstream g_logFile;
        static int g_successCount;
        static int g_warningCount;
        static int g_errorCount;
    };
    
    // Convenience macros for easy logging
    #define EVASION_LOG_SUCCESS(technique, msg) EvasionLogger::Logger::LogSuccess(technique, msg)
    #define EVASION_LOG_WARNING(technique, msg) EvasionLogger::Logger::LogWarning(technique, msg)
    #define EVASION_LOG_ERROR(technique, msg) EvasionLogger::Logger::LogError(technique, msg)
    #define EVASION_LOG_DEBUG(technique, msg) EvasionLogger::Logger::LogDebug(technique, msg)
}
