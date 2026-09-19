/**
* @file     crash_logging.h
* @author   Lim Zhi Jie
* @email    zhijie.lim
* @co-author
* @email
* @date     2025-09-29
*
* @brief Provides a basic crash logging utility for the game engine.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#pragma once
#include <string>

// Log an error message into crash_log.txt
void LogError(const std::string& message);