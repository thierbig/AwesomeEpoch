#include "EvasionLogger.h"
#include <iomanip>
#include <ctime>

namespace EvasionLogger {
    
    // Static member initialization
    bool Logger::g_consoleEnabled = false;
    bool Logger::g_fileEnabled = false;
    std::ofstream Logger::g_logFile;
    int Logger::g_successCount = 0;
    int Logger::g_warningCount = 0;
    int Logger::g_errorCount = 0;
    
    bool Logger::Initialize(bool enableConsole, bool enableFile) {
        g_consoleEnabled = enableConsole;
        g_fileEnabled = enableFile;
        
        if (enableConsole) {
            // Allocate console if not present
            if (!GetConsoleWindow()) {
                AllocConsole();
                freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
                freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
                freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
            }
            
            // Set console colors
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            
            std::cout << "\n=== EVASION TECHNIQUE LOGGER INITIALIZED ===" << std::endl;
        }
        
        if (enableFile) {
            // Create log file in temp directory
            char tempPath[MAX_PATH];
            GetTempPathA(MAX_PATH, tempPath);
            std::string logPath = std::string(tempPath) + "evasion_log.txt";
            
            g_logFile.open(logPath, std::ios::app);
            if (!g_logFile.is_open()) {
                if (enableConsole) {
                    std::cerr << "[ERROR] Failed to open log file: " << logPath << std::endl;
                }
                g_fileEnabled = false;
                return false;
            }
            
            g_logFile << "\n=== NEW SESSION " << GetTimestamp() << " ===" << std::endl;
        }
        
        return true;
    }
    
    void Logger::LogSuccess(const std::string& technique, const std::string& message) {
        Log(LOG_SUCCESS, technique, message);
        g_successCount++;
    }
    
    void Logger::LogWarning(const std::string& technique, const std::string& message) {
        Log(LOG_WARNING, technique, message);
        g_warningCount++;
    }
    
    void Logger::LogError(const std::string& technique, const std::string& message) {
        Log(LOG_ERROR, technique, message);
        g_errorCount++;
    }
    
    void Logger::LogDebug(const std::string& technique, const std::string& message) {
        Log(LOG_DEBUG, technique, message);
    }
    
    void Logger::Log(LogLevel level, const std::string& technique, const std::string& message) {
        std::string timestamp = GetTimestamp();
        std::string levelStr = GetLevelString(level);
        
        // Format: [TIMESTAMP] [LEVEL] [TECHNIQUE] Message
        std::ostringstream logEntry;
        logEntry << "[" << timestamp << "] [" << levelStr << "] [" << technique << "] " << message;
        
        if (g_consoleEnabled) {
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            
            // Set color based on level
            switch (level) {
                case LOG_SUCCESS:
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    break;
                case LOG_WARNING:
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    break;
                case LOG_ERROR:
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    break;
                case LOG_DEBUG:
                    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    break;
            }
            
            std::cout << logEntry.str() << std::endl;
            
            // Reset color
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }
        
        if (g_fileEnabled && g_logFile.is_open()) {
            g_logFile << logEntry.str() << std::endl;
            g_logFile.flush(); // Ensure immediate write
        }
    }
    
    std::string Logger::GetStatusSummary() {
        std::ostringstream summary;
        summary << "\n=== EVASION TECHNIQUE STATUS SUMMARY ===\n";
        summary << "✅ Successful: " << g_successCount << " techniques\n";
        summary << "⚠️  Warnings:   " << g_warningCount << " techniques\n";
        summary << "❌ Failed:     " << g_errorCount << " techniques\n";
        
        int total = g_successCount + g_warningCount + g_errorCount;
        if (total > 0) {
            double successRate = ((double)g_successCount / total) * 100.0;
            summary << "📊 Success Rate: " << std::fixed << std::setprecision(1) << successRate << "%\n";
            
            if (successRate >= 90) {
                summary << "🎯 Status: ELITE STEALTH\n";
            } else if (successRate >= 75) {
                summary << "🛡️  Status: HIGH STEALTH\n";
            } else if (successRate >= 50) {
                summary << "⚡ Status: MODERATE STEALTH\n";
            } else {
                summary << "⚠️  Status: LOW STEALTH\n";
            }
        }
        summary << "========================================";
        
        return summary.str();
    }
    
    void Logger::Cleanup() {
        if (g_consoleEnabled) {
            std::cout << GetStatusSummary() << std::endl;
            std::cout << "\nPress any key to continue..." << std::endl;
            std::cin.get();
        }
        
        if (g_fileEnabled && g_logFile.is_open()) {
            g_logFile << GetStatusSummary() << std::endl;
            g_logFile.close();
        }
        
        // Reset counters
        g_successCount = 0;
        g_warningCount = 0;
        g_errorCount = 0;
    }
    
    std::string Logger::GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }
    
    std::string Logger::GetLevelString(LogLevel level) {
        switch (level) {
            case LOG_SUCCESS: return "SUCCESS";
            case LOG_WARNING: return "WARNING";
            case LOG_ERROR:   return "ERROR";
            case LOG_DEBUG:   return "DEBUG";
            default:          return "UNKNOWN";
        }
    }
    
    std::string Logger::GetLevelColor(LogLevel level) {
        switch (level) {
            case LOG_SUCCESS: return "\033[32m"; // Green
            case LOG_WARNING: return "\033[33m"; // Yellow
            case LOG_ERROR:   return "\033[31m"; // Red
            case LOG_DEBUG:   return "\033[34m"; // Blue
            default:          return "\033[0m";  // Reset
        }
    }
}
