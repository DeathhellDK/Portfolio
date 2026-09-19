#ifndef DEBUG_CONSOLE_HPP
#define DEBUG_CONSOLE_HPP

/**
* @file DebugConsole.hpp
* @author Tan Wei Liang Terril
* @email t.weiliangterril
* @date 2025-11-29
*
* @brief Defines a lightweight, thread-safe logging system used across the engine and editor for capturing runtime diagnostics. It exposes a singleton DebugConsole that stores 
* time-stamped messages (Info, Warning, Error, Success) in an internal buffer that other systems can query or render inside the editor UI. 
* The console supports simple and variadic formatted logging, automatically attaches log level tags and timestamps, and ensures concurrency safety through internal mutex locking.
*
* @version 1.0
*
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include <vector>
#include <string>
#include <sstream>
#include <mutex>
#include <chrono>
#include <iomanip>
//#include "imgui.h"

enum class LogLevel {
    Info,
    Warning,
    Error,
    Success
};

struct LogEntry {
    std::string message;
    LogLevel level;
};

class DebugConsole
{
public:
    // Singleton access
    /**
     * @brief Singleton access for the DebugConsole.
     * @return Reference to the global DebugConsole instance.
     */
    static DebugConsole& Get() {
        static DebugConsole instance;
        return instance;
    }

    // Draws the ImGui window
    //void Draw();

    // Basic message adding
    void AddMessage(const std::string& msg, LogLevel level = LogLevel::Info);

    // Variadic template for multiple strings
    template<typename... Args>
    void AddFormattedMessage(LogLevel level, const Args&... args) {
        std::ostringstream oss;
        (oss << ... << args); // fold expression
        std::string typeStr;
        switch (level)
        {
        case LogLevel::Info:    typeStr = "INFO"; break;
        case LogLevel::Warning: typeStr = "WARNING"; break;
        case LogLevel::Error:   typeStr = "ERROR"; break;
        case LogLevel::Success:   typeStr = "SUCCESS"; break;
        }

        std::string finalMsg = FormatWithTime(typeStr, oss.str());
        AddMessage(finalMsg, level);
    }

    // Helper functions for levels
    void Info(const std::string& msg) { AddMessage(FormatWithTime("INFO", msg), LogLevel::Info); }
    void Warning(const std::string& msg) { AddMessage(FormatWithTime("WARNING", msg), LogLevel::Warning); }
    void Error(const std::string& msg) { AddMessage(FormatWithTime("ERROR", msg), LogLevel::Error); }
    void Success(const std::string& msg) { AddMessage(FormatWithTime("SUCCESS", msg), LogLevel::Success); }

    // Access messages (for EditorOverlay draw)
    const std::vector<LogEntry>& GetMessages() const { return messages; }

    // Clear console
    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<LogEntry>().swap(messages);  // free our msg storage
        scrollToBottom = true;
        //messages.clear();
    }

    bool IsScrollToBottom() const { return scrollToBottom; }

private:
    DebugConsole() = default;
    ~DebugConsole() = default;

    std::vector<LogEntry> messages;
    bool scrollToBottom = true;
    mutable std::mutex mutex_;

    std::string FormatWithTime(const std::string& type, const std::string& msg) {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream oss;
        oss << "[" << std::put_time(&tm, "%H:%M:%S") << "] [" << type << "] " << msg;
        return oss.str();
    }
};
#endif // !DEBUG_CONSOLE_HPP