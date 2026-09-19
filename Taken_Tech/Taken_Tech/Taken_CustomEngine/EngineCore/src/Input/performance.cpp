/**
* @file     performance.cpp
* @author   Lim Zhi Jie
* @email    zhijie.lim ,t.weiliangterril
* @co-author Tan Wei Liang Terril
* @date     2025-09-29
*
* @brief Definitions for the functions in PerformanceProfiler
* This class measures the time taken by each system during a single
* frame, and reports their percentage of the total frame time.
* 
* @usage:
*  profile.BeginFrame();
*  profile.ProfileSystem("Physics", insert_physics_update_here);
*  profile.EndFrame();
*  profile.Report();
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "Input/Performance.h"
#include "Input/DebugConsole.hpp"
#include <iostream>

/**
    * @brief Marks the beginning of a profiling frame.
    *
    * Clears previous results and records the current timestamp, which is
    * later compared in EndFrame() to calculate total frame duration.
*/
void PerformanceProfiler::BeginFrame() {
    results.clear();
    frameStart = std::chrono::high_resolution_clock::now();
}

/**
    * @brief Marks the end of a profiling frame.
    *
    * Computes the total frame time in milliseconds using the high-resolution
    * clock. This value is used when calculating system time percentages.
*/
void PerformanceProfiler::EndFrame() {
    auto frameEnd = std::chrono::high_resolution_clock::now();
    totalFrameTimeMs = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();
}

/**
    * @brief Profiles the execution time of a specific system function.
    *
    * Measures the duration of a system update function using a high-precision
    * timer, then stores the result for inclusion in the final report.
    *
    * @param name Name of the system (e.g. "Physics", "AI", "Rendering").
    * @param func Function pointer to the system update routine.
    *
    * @note The system function must take no arguments and return void.
*/
void PerformanceProfiler::ProfileSystem(const std::string& name, void (*func)()) {
    auto start = std::chrono::high_resolution_clock::now();

    func(); // call the function pointer

    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end - start).count();

    results.push_back({ name, duration });
}

/**
    * @brief Outputs profiling results to the DebugConsole.
    *
    * For each profiled system, prints:
    *  - Percentage of total frame time
    *  - Execution time in milliseconds
    *
    * Finally prints the total frame time for the entire frame.
*/
void PerformanceProfiler::Report() const {
    DebugConsole::Get().Info("Frame Profiling Results\n");
    for (const auto& r : results) {
        double percent = (totalFrameTimeMs > 0.0) ? (r.timeMs / totalFrameTimeMs) * 100.0 : 0.0;
        DebugConsole::Get().AddFormattedMessage(LogLevel::Info, r.name + ": " + std::to_string(percent) + "% (" + std::to_string(r.timeMs) + " ms)\n");
    }
    DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "Total Frame Time: ", std::to_string(totalFrameTimeMs), " ms\n\n");
}