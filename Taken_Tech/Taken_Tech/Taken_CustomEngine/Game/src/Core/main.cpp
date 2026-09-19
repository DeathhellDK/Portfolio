// Game.cpp : Entry point for the game application.
/**
* @file     main.cpp
* @author   Jethro Sung, Woh Kye Le, Lim Zhi Jie,Tan Wei Liang Terril
* @email    sung.h, w.kyele, zhijie.lim, t.weiliangterril
* @date     2025-09-11
*
* @brief    Entry point for the game application.
*
* This file initializes the engine, sets up the renderer, game application,
* and ImGui, then runs the main game loop. It handles per-frame updates,
* rendering, and UI drawing, and performs proper shutdown of subsystems
* when the application exits.
*
* @version 1.0
* @copyright
* Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the
* prior written consent of DigiPen Institute of Technology is prohibited.
*/
#include "Core/engine.hpp"
#include <glad/glad.h>
#include <iostream>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "Graphics/renderer.h"
#include "Graphics/shader.h"
#include "Graphics/mesh2d.h"
#include "Graphics/vertex2d.h"
#include "Graphics/camera2d.h"
#include "Core/gameobj.h"
#include "Core/transform.h"
#include "Graphics/meshrenderer.h"
#include <vector>
#include <cmath>
#include "Core/gameApp.h"
#ifdef _WIN32
#if defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#undef APIENTRY
#include <crtdbg.h>
#endif
#endif

#if ENABLE_EDITOR
#include "Editor/ImGuiHost.h"
#endif


//#include "imgui.h"
//#include "backends/imgui_impl_glfw.h"
//#include "backends/imgui_impl_opengl3.h"


/**
* @brief Program entry point.
*
* Initializes the engine and rendering system, creates the GameApp, and
* sets up ImGui for runtime debug UI. Runs the main game loop until the
* window is closed, updating the game state and rendering each frame.
* On exit, it properly shuts down all systems and releases resources.
*
* @return Exit code. Returns 0 on success, non-zero on failure.
*/
int main() {


    #ifdef _WIN32
    #if defined(_DEBUG)
        // Enable debug heap + automatic leak dump on exit
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
        //_CrtSetBreakAlloc(6040);

        // Send CRT output to Visual Studio Output window
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
        _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    #endif
    #endif

    // Init engine
    if (!eng::init({ 1400, 700, "Game", true, true })) return -1;

    glfwSwapInterval(0);

    #if ENABLE_EDITOR
        if (GLFWwindow* win = glfwGetCurrentContext()) {
            ImGuiHost::Get().Acquire(win, "#version 330");
        }
    #endif

    // Enable transparency blending (for particles, sprites, etc.)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Setup renderer
    Renderer renderer;


    // Game application
    GameApp game;

    if (!game.Initialize(renderer, static_cast<float>(eng::deltaTime()))) {
        DebugConsole::Get().Error("GameApp initialization failed");
    #if ENABLE_EDITOR
            ImGuiHost::Get().Release();   
    #endif
        eng::shutdown();
        return -1;
    }

    eng::setFixedDelta(1.0 / 60.0);    // 60 Hz simulation
    eng::setMaxFixedStepsPerFrame(8);  // max no of updates per frame

    while (!eng::shouldClose()) {
        eng::beginFrame();

        // --- Make sure the default framebuffer viewport matches the actual window ---
        int fbw = 0, fbh = 0;
        if (GLFWwindow* win = glfwGetCurrentContext()) {
            glfwGetFramebufferSize(win, &fbw, &fbh);
            if (fbw > 0 && fbh > 0) {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, fbw, fbh);
            }
        }
        if (!eng::isPaused()) {
            eng::forEachFixedStep([&](double fdt) {
                game.FixedUpdate(fdt);
                });

            game.Update(eng::deltaTime());
        }

        renderer.Clear(0.1f, 0.1f, 0.15f, 1.0f);

        game.Draw(renderer);    // replaces raw game.draw()

        eng::endFrame();
    }

    game.Shutdown();

    #if ENABLE_EDITOR
        ImGuiHost::Get().Release();  
    #endif

    eng::shutdown();


    return 0;
}