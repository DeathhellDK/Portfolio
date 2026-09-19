/**
 * @file     engine.cpp
 * @author   Jethro Sung
 * @email    sung.h
 * @co-author Woh Kye Le, Lim Zhi Jie
 * @email     w.kyele, zhijie.lim
 * @date     2025-09-11
 *
 * @brief    Implementation of core engine functions: window creation,
 *           per-frame updates, delta time calculation, input handling,
 *           and shutdown. Acts as the backbone of the game loop.
 *
 * This module wraps GLFW and custom Window/Input classes into a simple
 * set of global functions under the `eng` namespace. It is used by the
 * GameApp and main loop to control frame progression.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#include "Core/engine.hpp"
#include "Core/window.h"
#include "Input/input.h"
#include "Audio/audio.h"
#include "Input/DebugConsole.hpp"
#include <memory>

namespace eng {

    static std::unique_ptr<Window> gWindow;
    static std::unique_ptr<Input>  gInput;
    static double gLast = 0.0, gDt = 0.0;

    static double gFixedDt = 1.0 / 60.0;
    static double gAccumulator = 0.0;
    static double gAlpha = 0.0;         // leftover fraction in [0,1)
    static int    gMaxSteps = 8;        // guard per frame
    static bool gPaused = false;
    bool eng::isPaused() { return gPaused; }
    void eng::_setPaused(bool p) { gPaused = p; }
    /**
    * @brief Initialize the engine by creating the window and input systems.
    *
    * Builds a Window descriptor from the provided configuration, creates
    * the OpenGL context, sets up the input system, and registers scroll callbacks.
    *
    * @param cfg Engine configuration (window size, title, vsync, etc.).
    * @return true if initialization succeeded, false otherwise.
    */
    bool init(const Config& cfg) {
        // Build window descriptor from Config
        Window::Desc desc;
        desc.w = cfg.width;
        desc.h = cfg.height;
        desc.title = cfg.title;
        desc.vsync = cfg.vsync;
        desc.fullscreen = cfg.fullscreen;

        gWindow = std::make_unique<Window>(desc);
        if (!gWindow->create()) return false;

        gInput = std::make_unique<Input>(gWindow->handle());

        // Register Input scroll callback (Window already uses user pointer for resize)
        glfwSetScrollCallback(gWindow->handle(), Input::scrollCallback);

        GLFWwindow* win = gWindow->handle();

        glfwSetWindowFocusCallback(win, [](GLFWwindow* win, int focused)
            {
                if (!focused)
                {
                    // Window lost focus (ALT+TAB, CTRL+ALT+DEL, Windows key)
                    eng::_setPaused(true);

                    Audio::PauseAll();

                    glfwIconifyWindow(win);   // minimize the window
                }
                else
                {
                    // Window regained focus
                    eng::_setPaused(false);

                    Audio::ResumeAll();
                }
            });

        gLast = glfwGetTime();
        gDt = 0.0;
        return true;
    }
    /**
    * @brief Shut down the engine and release resources.
    *
    * Destroys the input system, window, and any associated GLFW state.
    */
    void shutdown() {
        gInput.reset();
        if (gWindow) { gWindow->destroy(); gWindow.reset(); }
    }

    /**
     * @brief Begin a new frame: poll window events, update input, and compute delta time.
     *
     * Should be called at the start of every frame in the main loop.
     */
    void beginFrame() {
        gWindow->poll();
        gInput->beginFrame();
        const double now = glfwGetTime();
        gDt = now - gLast; gLast = now;
    }

    void endFrame() { gWindow->swap(); }

    bool   shouldClose() { return gWindow->ShouldClose(); }
    double deltaTime() { return gDt; }
    GLFWwindow* windowHandle() { return gWindow ? gWindow->handle() : nullptr; }
    Input& input() { return *gInput; }

    /**
    * @brief Get the current framebuffer size in pixels.
    *
    * @param[out] w Width of the framebuffer.
    * @param[out] h Height of the framebuffer.
    */
    void framebufferSize(int& w, int& h) {
        if (auto* win = windowHandle()) {
            glfwGetFramebufferSize(win, &w, &h);
        }
        else {
            w = h = 0;
        }
    }

    /**
    * @brief Set the fixed simulation timestep used by the fixed-step driver
    * @param seconds Fixed delta time in seconds
    */
    void setFixedDelta(double seconds) {
        gFixedDt = (seconds > 0.0) ? seconds : (1.0 / 60.0);
    }

    /**
    * @brief Get the current fixed simulation timestep.
    * @return Fixed delta time in seconds.  
    */
    double fixedDelta() { return gFixedDt; }

    /**
    * @brief Limit how many fixed steps may be executed in a single rendered frame
    * @param n Maximum number of fixed steps per frame (min: 1).
    */
    void setMaxFixedStepsPerFrame(int n) {
        gMaxSteps = (n > 0) ? n : 1;
    }

    /**
    * @brief Run the fixed simulation steps required for this rendered frame.
    *
    * Accumulates variable dt time (from @c beginFrame/@c deltaTime), then invokes
    * once per fixed slice (size = @c fixedDelta), up to the maximum number, it
    * Also computes the interpolation alpha for render-time smoothing.
    *
    * @param stepFn Callback invoked once per fixed tick with @p fdt seconds.
    */
    void forEachFixedStep(const std::function<void(double)>& stepFn) {
        // Accumulate the variable dt from this frame
        gAccumulator += gDt;

        int steps = 0;
        // Run up to gMaxSteps fixed ticks to catch up
        while (gAccumulator + 1e-12 >= gFixedDt && steps < gMaxSteps) {
            stepFn(gFixedDt);
            gAccumulator -= gFixedDt;
            ++steps;
        }

        // If we hit the cap, drop any runaway debt to avoid spiral of death
        if (steps == gMaxSteps && gAccumulator > gFixedDt) {
            gAccumulator = std::fmod(gAccumulator, gFixedDt);
        }

        // Compute interpolation fraction for render
        gAlpha = (gFixedDt > 0.0) ? (gAccumulator / gFixedDt) : 0.0;
        if (gAlpha < 0.0) gAlpha = 0.0;
        if (gAlpha >= 1.0) gAlpha = std::nextafter(1.0, 0.0); // clamp to [0,1)
    }

    /**
    * @brief Fractional remainder of time in the accumulator after running fixed steps.
    * @return Alpha in [0,1)
    */
    double interpolationAlpha() { return gAlpha; }

    bool isFullscreen() {
        return gWindow && gWindow->isFullscreen();
    }

    void toggleFullscreen(int windowWidth, int windowHeight) {
        if (!gWindow) return;

        bool nowFullscreen = !gWindow->isFullscreen();
        gWindow->setFullscreen(nowFullscreen, windowWidth, windowHeight);

        DebugConsole::Get().Info("[Window] Switched to "
            + std::string(nowFullscreen ? "fullscreen\n" : "windowed\n"));
    }

} // namespace eng
