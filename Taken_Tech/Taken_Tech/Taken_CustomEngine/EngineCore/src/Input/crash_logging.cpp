/**
* @file     crash_logging.cpp
* @author   Lim Zhi Jie
* @email    zhijie.lim
* @co-author
* @email
* @date     2025-09-29
*
* @brief Defines the LogError function which appends error messages to the
* "crash_log.txt" file. This system is designed for global use across
* the game engine, ensuring all modules can record failures consistently.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "Input/crash_logging.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

/**
    * @brief Appends an error message to the global crash log file.
    *
    * This function provides a lightweight, engine-wide error logging mechanism.
    * It writes the supplied message to "crash_log.txt", creating the file if
    * needed, and appending new entries at the end.
    *
    * Usage scenario:
    * - Wrap high-risk operations inside a try/catch block and call LogError()
    *   inside the catch clause to ensure the failure reason is always recorded.
    *
    * Limitations:
    * - Logging is fire-and-forget; errors during writing are silently ignored.
    * - Not thread-safe; if multiple threads call LogError() concurrently,
    *   log order is not guaranteed.
    *
    * @param message Human-readable error string to append to the crash log.
*/
void LogError(const std::string& message) {
    std::ofstream log("crash_log.txt", std::ios::app);
    if (log.is_open()) {
        log << "ERROR: " << message << "\n";
    }
}