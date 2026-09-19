#pragma once
/**
* @file		  ImGuiHost.h
* @author     Woh Kye Le
* @email      w.kyele
* @date		  2026 - 01 - 23
*
* @brief Owns and manages ImGui context + GLFW/OpenGL3 backends.
*
* Goal:
* - Avoid multiple CreateContext()/Shutdown() calls across systems.
* - Centralize backend init / new frame / render / shutdown.
*
*
* @version 1.0
* @copyright
* Copyright (C) 2026 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the
* prior written consent of DigiPen Institute of Technology is prohibited.
*/


#include <cstdint>

struct GLFWwindow;

/**
 * @class ImGuiHost
 * @brief Singleton class that manages ImGui context and GLFW/OpenGL3 backend lifecycle
 *
 * This class provides centralized management of Dear ImGui initialization, frame rendering,
 * and shutdown. It uses reference counting to allow multiple systems to share the same
 * ImGui context safely without duplicate initialization or premature shutdown.
 */
class ImGuiHost{
public:
   
    /**
     * @brief Singleton access for the ImGuiHost manager.
     * @return Reference to the global ImGuiHost instance.
     */
    static ImGuiHost& Get();

    /**
     * @brief Initialize or acquire reference to the ImGui backend
     *
     * Initializes the ImGui context, GLFW backend, and OpenGL3 renderer if not already
     * initialized. If already initialized, increments the reference count.
     *
     * @param window Pointer to the GLFW window for backend initialization
     * @param glslVersion GLSL version string (default: "#version 330")
     * @return true if initialization successful or already ready
     * @return false if initialization failed (e.g., invalid window)
     */
    bool Acquire(GLFWwindow* window, const char* glslVersion = "#version 330");

    /**
     * @brief Release reference to the ImGui backend
     *
     * Decrements the reference count. When reference count reaches zero,
     * shuts down the ImGui backends and destroys the context.
     */
    void Release();

    /**
     * @brief Prepares ImGui state for the beginning of a new frame.
     */
    void BeginFrame() const;

    /**
     * @brief Finalizes ImGui rendering and dispatches draw data to the GPU.
     */
    void EndFrame() const;

    /**
     * @brief Check if ImGuiHost is ready for use
     * @return true if backend is initialized and ready
     * @return false if not initialized or shutdown
     */
    bool IsReady() const { return ready_; }

    /**
     * @brief Begin a global ImGui frame for the entire application.
     *
     */
    void BeginFrameGlobal() const { BeginFrame(); }

    /**
     * @brief End the global ImGui frame and render for the entire application.
     *
     */
    void EndFrameGlobal()   const { EndFrame(); }

    /**
     * @brief Check if ImGui context has been created
     * @return true if ImGui context exists
     * @return false if no context created
     */
    bool HasContext() const { return contextCreated_; }

private:
    ImGuiHost() = default;
    ~ImGuiHost() = default;

    // Prevent copying
    ImGuiHost(const ImGuiHost&) = delete;
    ImGuiHost& operator=(const ImGuiHost&) = delete;

private:
    bool ready_ = false;
    bool contextCreated_ = false;
    int  refCount_ = 0;

    // store to detect invalid window changes
    GLFWwindow* window_ = nullptr;
};
