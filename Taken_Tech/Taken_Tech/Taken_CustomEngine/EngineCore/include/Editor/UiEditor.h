#pragma once
/**
* @file		  UiEditor.h
* @author     Woh Kye Le
* @email      w.kyele
* @date		  2026 - 03 - 09
*
* @brief ImGui-based UI editor for creating and editing GUI layouts
*
* The UiEditor provides a user interface for designing UI layouts using
* a docking system. It allows for live editing of UI elements by toggling
* an editing mode that disables normal game UI updates.
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

#include "imgui.h"
#include <functional>
#include <string>

class UiEditor {
public:
    // Callbacks provided by GAME layer
    using SetUiUpdateEnabledFn = std::function<void(bool enabled)>;

    /**
    * @brief Initialize the UI editor
    */
    void Init();

    /**
    * @brief Update the editor state
    * @param dt Delta time in seconds
    */
    void Update(float dt);

    /**
    * @brief Draw the editor UI
    */
    void Draw();

    /**
    * @brief Set whether the editor is visible
    */
    void SetVisible(bool visible) { 
        if (visible && !visible_) requestDockRebuild_ = true;
        visible_ = visible; 
    }

    /**
    * @brief Check if the editor is visible
    */
    bool IsVisible() const { return visible_; }

    /**
    * @brief Toggle the editor visibility
    */
    void ToggleVisible() { 
        visible_ = !visible_; 
        if (visible_) requestDockRebuild_ = true;
    }

    /**
    * @brief Set the callback for enabling/disabling game UI updates
    */
    void SetUiUpdateEnabledCallback(SetUiUpdateEnabledFn cb) { setUiUpdateEnabled_ = cb; }

    /**
    * @brief Check if we are currently in editing mode
    */
    bool IsEditing() const { return isEditing_; }

    /**
     * @brief Set the scene texture to display in the viewport.
     * @param texId OpenGL texture ID of the scene
     * @param w Width of the scene texture
     * @param h Height of the scene texture
     */
    void SetSceneTexture(unsigned int texId, int w, int h);

    /**
     * @brief Get the viewport image position and size in window space
     * @param pos Output position
     * @param size Output size
     */
    void GetViewportRect(float& x, float& y, float& w, float& h) const {
        x = viewportImagePos_.x;
        y = viewportImagePos_.y;
        w = viewportImageSize_.x;
        h = viewportImageSize_.y;
    }

private:
    // ---------------- Drawing various ImGui panels ----------------
    /** @brief Draws the main editor control panel and settings. */
    void DrawEditorControlsWindow_();
    
    /** @brief Draws the toggle button and properties for Edit Mode. */
    void DrawEditModeWindow_();
    
    /** @brief Draws the central viewport window containing the scene texture. */
    void DrawViewportWindow_();

    // ---------------- Docking helpers ----------------
    bool BeginDockHost_();
    void EndDockHost_();
    void BuildDockLayoutIfNeeded_();
    void UpdateDockRebuildRequests_();

    // ---------------- State ----------------
    bool visible_ = false;
    bool isEditing_ = false;
    
    // Docking state
    ImGuiID dockspaceId_ = 0;
    bool requestDockRebuild_ = true;
    bool firstDockBuild_ = true;
    ImGuiID centerDockId_ = 0;

    // Callbacks
    SetUiUpdateEnabledFn setUiUpdateEnabled_;

    // Viewport texture
    unsigned int sceneTextureId_ = 0;
    int sceneTexW_ = 0;
    int sceneTexH_ = 0;

    // Last known image pos/size in window space
    ImVec2 viewportImagePos_ = { 0, 0 };
    ImVec2 viewportImageSize_ = { 0, 0 };
};

#endif // ENABLE_EDITOR
