/**
 * @file      UiEditor.cpp
 * @author    Woh Kye Le
 * @email     w.kyele
 * @date      2026-03-09
 *
 * @brief Implements the UI Editor for designing and managing in-game GUI elements.
 *
 * The UI Editor provides a visual interface for editing GUI layouts, handling
 * viewport scaling, and managing edit-mode interactions. It uses ImGui for
 * its layout and docking system while interacting with the game's custom GuiSystem.
 *
 * @version 1.1
 * @copyright
 * Copyright (C) 2026 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#ifndef ENABLE_EDITOR
#define ENABLE_EDITOR 0
#endif

#if ENABLE_EDITOR

#include "Editor/UiEditor.h"
#include "imgui_internal.h"
#include <iostream>

/**
 * @brief Initializes the UI Editor state.
 */
void UiEditor::Init() {
    visible_ = false;
    isEditing_ = false;
    requestDockRebuild_ = true;
    firstDockBuild_ = true;
    sceneTextureId_ = 0;
    sceneTexW_ = 0;
    sceneTexH_ = 0;
}

/**
 * @brief Sets the texture to be displayed in the editor viewport.
 * @param texId The OpenGL texture ID.
 * @param w Texture width.
 * @param h Texture height.
 */
void UiEditor::SetSceneTexture(unsigned int texId, int w, int h) {
    sceneTextureId_ = texId;
    sceneTexW_ = w;
    sceneTexH_ = h;
}

/**
 * @brief Per-frame update logic for the editor.
 * @param dt Delta time since the last frame.
 */
void UiEditor::Update(float /*dt*/) {
    UpdateDockRebuildRequests_();
}

/**
 * @brief Renders the UI Editor windows and dockspace.
 */
void UiEditor::Draw() {
    if (!visible_)
        return;

    if (BeginDockHost_()) {
        BuildDockLayoutIfNeeded_();

        DrawEditorControlsWindow_();
        DrawEditModeWindow_();
        DrawViewportWindow_();

        EndDockHost_();
    }
}

/**
 * @brief Renders the viewport window where the game scene is displayed.
 *
 * Handles letterboxing/pillarboxing to maintain the scene's aspect ratio
 * within the available ImGui window space.
 */
void UiEditor::DrawViewportWindow_() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    if (!vp) return;

    if (vp->WorkSize.x <= 0.0f || vp->WorkSize.y <= 0.0f) {
        viewportImagePos_ = ImVec2(0, 0);
        viewportImageSize_ = ImVec2(0, 0);
        return;
    }

    // Default = whole screen
    ImVec2 winPos = vp->WorkPos;
    ImVec2 winSize = vp->WorkSize;

    // Pull rect from center dock node
    if (ImGuiDockNode* centerNode = ImGui::DockBuilderGetNode(centerDockId_)) {
        if (centerNode->Size.x > 32.0f && centerNode->Size.y > 32.0f) {
            winPos = centerNode->Pos;
            winSize = centerNode->Size;

            const float pad = 8.0f;
            winPos.x += pad;
            winPos.y += pad;
            winSize.x = std::max(0.0f, winSize.x - 2.0f * pad);
            winSize.y = std::max(0.0f, winSize.y - 2.0f * pad);
        }
    }

    ImGui::SetNextWindowPos(winPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);
    ImGui::SetNextWindowViewport(vp->ID);

    if (!ImGui::Begin("UI Viewport", nullptr,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoDocking)) {

        viewportImagePos_ = ImVec2(0, 0);
        viewportImageSize_ = ImVec2(0, 0);
        ImGui::End();
        return;
    }

    // Texture validity
    const bool hasValidTexture = (sceneTextureId_ != 0 && sceneTexW_ > 0 && sceneTexH_ > 0);

    // Available content rect
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const ImVec2 cursor = ImGui::GetCursorScreenPos();

    if (!hasValidTexture || avail.x <= 1.0f || avail.y <= 1.0f) {
        viewportImagePos_ = ImVec2(0, 0);
        viewportImageSize_ = ImVec2(0, 0);
        ImGui::TextDisabled("No scene texture.");
        ImGui::End();
        return;
    }

    const float texAspect = (float)sceneTexW_ / (float)sceneTexH_;
    const float availAspect = (avail.y > 0.0f) ? (avail.x / avail.y) : texAspect;

    // Fit image to available rect (letterbox/pillarbox)
    ImVec2 drawSize = avail;
    if (availAspect > texAspect) {
        drawSize.x = avail.y * texAspect;   // too wide -> clamp width
    }
    else {
        drawSize.y = avail.x / texAspect;   // too tall -> clamp height
    }

    // Center the image in the available rect
    ImVec2 drawPos = cursor;
    drawPos.x += (avail.x - drawSize.x) * 0.5f;
    drawPos.y += (avail.y - drawSize.y) * 0.5f;

    // Cache for coordinate mapping if needed later
    viewportImagePos_ = drawPos;
    viewportImageSize_ = drawSize;

    // Background
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(cursor,
        ImVec2(cursor.x + avail.x, cursor.y + avail.y),
        IM_COL32(15, 15, 15, 255));

    // Draw the scene texture
    ImGui::SetCursorScreenPos(drawPos);
    ImGui::Image((ImTextureID)(intptr_t)sceneTextureId_, drawSize, ImVec2(0, 1), ImVec2(1, 0));

    ImGui::End();
}

/**
 * @brief Renders the main control window for the UI Editor.
 *
 * Contains toggle buttons for switching between editing and game modes.
 */
void UiEditor::DrawEditorControlsWindow_() {
    if (!ImGui::Begin("UI Editor Controls")) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("##UiEditorTabs")) {
        if (ImGui::BeginTabItem("Editing")) {
            const float btnW = 120.0f, btnH = 32.0f;
            
            if (!isEditing_) {
                if (ImGui::Button("Start Editing", ImVec2(btnW, btnH))) {
                    isEditing_ = true;
                    if (setUiUpdateEnabled_) {
                        setUiUpdateEnabled_(false);
                    }
                }
            } else {
                if (ImGui::Button("Stop Editing", ImVec2(btnW, btnH))) {
                    isEditing_ = false;
                    if (setUiUpdateEnabled_) {
                        setUiUpdateEnabled_(true);
                    }
                }
            }

            ImGui::SameLine();
            ImGui::TextColored(isEditing_ ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                isEditing_ ? "EDITING MODE ON" : "GAME MODE ON");

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

/**
 * @brief Renders the edit mode selection window.
 *
 * Provides tools like Snip and Cursor for UI manipulation.
 */
void UiEditor::DrawEditModeWindow_() {
    if (!ImGui::Begin("Edit Mode")) {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Snip")) {
        // Future implementation for snip mode
    }

    ImGui::SameLine();

    if (ImGui::Button("Cursor")) {
        // Future implementation for cursor mode
    }

    ImGui::End();
}

/**
 * @brief Sets up the main docking host window.
 * @return True if the host window was successfully started.
 */
bool UiEditor::BeginDockHost_() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    if (!vp) return false;

    if (vp->WorkSize.x <= 0.0f || vp->WorkSize.y <= 0.0f)
        return false;

    ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking;

    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    bool opened = ImGui::Begin("##UiDockHost", nullptr, hostFlags);

    ImGui::PopStyleVar(2);

    if (!opened) {
        ImGui::End();
        return false;
    }

    dockspaceId_ = ImGui::GetID("UiEditorDockSpace");
    ImGui::DockSpace(dockspaceId_, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

    return true;
}

/**
 * @brief Cleans up the docking host window.
 */
void UiEditor::EndDockHost_() {
    ImGui::End();
}

/**
 * @brief Rebuilds the dockspace layout if requested or on first run.
 *
 * Configures the initial split between the control panel, sidebar, and viewport.
 */
void UiEditor::BuildDockLayoutIfNeeded_() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    if (!vp) return;

    if (vp->WorkSize.x <= 0.0f || vp->WorkSize.y <= 0.0f)
        return;

    if (!requestDockRebuild_ && !firstDockBuild_)
        return;

    requestDockRebuild_ = false;
    firstDockBuild_ = false;

    ImGui::DockBuilderRemoveNode(dockspaceId_);
    ImGui::DockBuilderAddNode(
        dockspaceId_,
        (ImGuiDockNodeFlags)ImGuiDockNodeFlags_DockSpace |
        (ImGuiDockNodeFlags)ImGuiDockNodeFlags_PassthruCentralNode
    );

    ImGui::DockBuilderSetNodeSize(dockspaceId_, vp->WorkSize);

    ImGuiID dock_main = dockspaceId_;

    // Top strip for editor controls
    ImGuiID dock_top = ImGui::DockBuilderSplitNode(
        dock_main, ImGuiDir_Up, 0.12f, nullptr, &dock_main
    );

    // Right sidebar (keep empty for future implementation)
    ImGuiID dock_right = ImGui::DockBuilderSplitNode(
        dock_main, ImGuiDir_Right, 0.22f, nullptr, &dock_main
    );

    centerDockId_ = dock_main;

    ImGui::DockBuilderDockWindow("UI Editor Controls", dock_top);
    ImGui::DockBuilderDockWindow("Edit Mode", dock_right);
    ImGui::DockBuilderDockWindow("UI Viewport", dock_main);

    ImGui::DockBuilderFinish(dockspaceId_);
}

/**
 * @brief Checks for window resize events and requests a dock rebuild if necessary.
 */
void UiEditor::UpdateDockRebuildRequests_() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    static ImVec2 lastWorkSize = ImVec2(0, 0);

    if (vp && (vp->WorkSize.x != lastWorkSize.x || vp->WorkSize.y != lastWorkSize.y)) {
        requestDockRebuild_ = true;
        lastWorkSize = vp->WorkSize;
    }
}

#endif // ENABLE_EDITOR
