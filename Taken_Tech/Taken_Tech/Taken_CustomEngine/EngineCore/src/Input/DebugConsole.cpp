/**
* @file DebugConsole.cpp
* @author Tan Wei Liang Terril
* @email t.weiliangterril
* @date 2025-11-29
*
* @brief The DebugConsole provides a centralized, thread-safe logging system used throughout the engine and editor to display runtime diagnostic information. 
* It maintains an internal message buffer that stores text entries along with their severity level (Info, Success, Warning, Error), allowing the UI layer to render a live scrolling console 
* for debugging. All message writes are protected by a mutex to support safe concurrent logging from multiple subsystems—gameplay logic, rendering, physics, or editor scripts.
* 
* @version 1.0
*
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
#include "Input/DebugConsole.hpp"

/**
    * @brief Adds a new formatted debug message to the console buffer.
    *
    * The DebugConsole acts as a thread-safe message sink for runtime diagnostics
    * displayed inside the engine/editor. This function acquires a mutex to ensure
    * safe concurrent writes from multiple threads, then pushes a message+level
    * pair into the internal message list.
    *
    * Effects:
    * - Stores the message and its severity (Info, Success, Warning, Error).
    * - Sets `scrollToBottom` to false, allowing the UI layer to decide when
    *   to auto-scroll the message list.
    *
    * @param msg   Text to record in the console buffer.
    * @param level Log severity classification (Info / Success / Warning / Error).
*/
void DebugConsole::AddMessage(const std::string& msg, LogLevel level)
{
    std::lock_guard<std::mutex> lock(mutex_); // thread safety
    messages.push_back({ msg, level });
    scrollToBottom = false;
}
