/**
* @file     performance.h
* @author   Lim Zhi Jie
* @email    zhijie.lim
* @co-author
* @email
* @date     2025-09-29
*
* @brief Header file for PerformanceProfiler class
* This class measures the time taken by each system during a single
* frame, and reports their percentage of the total frame time.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#pragma once
#include <string>
#include <vector>
#include <functional>
#include <chrono>

class PerformanceProfiler {
public:
    struct ProfileResult {
        std::string name;
        double timeMs;
    };

    void BeginFrame();
    void EndFrame();
    void ProfileSystem(const std::string& name, void (*func)());
    void Report() const;

private:
    std::vector<ProfileResult> results;
    std::chrono::high_resolution_clock::time_point frameStart;
    double totalFrameTimeMs = 0.0;
};
