/**
* @file		  ImGuiHost.cpp
* @author     Woh Kye Le
* @email      w.kyele
* @date		  2026 - 01 - 23
*
* @brief Implementation of ImGuiHost singleton for managing ImGui lifecycle
*
* Implements the reference-counted acquisition/release pattern and frame
* management for Dear ImGui with GLFW/OpenGL3 backends.
*
*
* @version 1.0
* @copyright
* Copyright (C) 2026 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the
* prior written consent of DigiPen Institute of Technology is prohibited.
*/

#ifndef ENABLE_EDITOR
#define ENABLE_EDITOR 0
#endif

#if ENABLE_EDITOR
#include "Editor/ImGuiHost.h"
#include "imgui.h"
#include "ImGuizmo.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

/**
 * @brief Get the singleton instance
 * @return Reference to the singleton ImGuiHost instance
 */
ImGuiHost& ImGuiHost::Get(){
    static ImGuiHost s;
    return s;
}

/**
 * @brief Initialize or acquire reference to the ImGui backend
 *
 * @param window Pointer to the GLFW window for backend initialization
 * @param glslVersion GLSL version string
 * @return true if initialization successful or already ready
 * @return false if initialization failed
 */
bool ImGuiHost::Acquire(GLFWwindow* window, const char* glslVersion){
    if (!window)
        return false;

    if (ready_ && window_ == window){
        ++refCount_;
        return true;
    }

    // Create context once.
    const bool createdContextNow = !contextCreated_;
    if (createdContextNow) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        contextCreated_ = true;
    }

    // If window changed across runs, init backends for the new one.
    window_ = window;

    // Init backends
    if (!ImGui_ImplGlfw_InitForOpenGL(window_, true)) {
        if (createdContextNow) {
            ImGui::DestroyContext();
            contextCreated_ = false;
        }
        window_ = nullptr;
        return false;
    }
        

    if (!ImGui_ImplOpenGL3_Init(glslVersion)) {
        ImGui_ImplGlfw_Shutdown();
        if (createdContextNow) {
            ImGui::DestroyContext();
            contextCreated_ = false;
        }
        window_ = nullptr;
        return false;
    }
        

    // Configure io once here 
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // expects the current ImGui context
    ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

    refCount_ = 1;
    ready_ = true;
    return true;
}

/**
 * @brief Release reference to the ImGui backend
 */
void ImGuiHost::Release(){
    if (!ready_)
        return;

    --refCount_;
    if (refCount_ > 0)
        return;

    // Shutdown backends and destroy context
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext();

    ready_ = false;
    contextCreated_ = false;
    window_ = nullptr;
    refCount_ = 0;
}

/**
 * @brief Begin a new ImGui frame
 */
void ImGuiHost::BeginFrame() const{
    if (!ready_)
        return;

    // only run if there's still a valid GLFW context
    if (!glfwGetCurrentContext())
        return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

/**
 * @brief End the current ImGui frame and render
 */
void ImGuiHost::EndFrame() const{
    if (!ready_)
        return;

    if (!glfwGetCurrentContext())
        return;

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
#endif