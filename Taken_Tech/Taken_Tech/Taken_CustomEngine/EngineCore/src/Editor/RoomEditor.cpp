/**
* @file		  RoomEditor.cpp
* @author     Woh Kye Le
* @co-author  Lim Zhi Jie, Jethro Sung
* @email      w.kyele, zhijie.lim. sung.h
* @date		  2026 - 01 - 22
*
* @brief      Engine-side room txt editor
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


#include "Editor/RoomEditor.h"
#include "Editor/ImGuiHost.h"
#include "Editor/EditorShared.h"
#include "Graphics/Camera2D.h"
#include "Core/transform.h"
#include "Light/lightComponent.h"
#include "Graphics/meshrenderer.h"
#include "Physics/collider.h"
#include "Graphics/spriteanimator.h"
#include "Editor/UndoRedoManager.h"

#include <GLFW/glfw3.h>

#include "imgui_internal.h"
#include <cstring>   
#include <fstream>
#include <string>
#include <algorithm> 
#include <cmath>  
#include <system_error>

/**
 * @brief Convert a world-space grid Y coordinate into a room-editor row index.
 *
 * RoomEditor stores grid rows in top-origin order (row 0 is the top row), while
 * many game-side helpers treat grid Y as bottom-origin. This helper flips Y to
 * the editor's row convention.
 *
 * @param worldGy Bottom-origin grid Y coordinate.
 * @param gridHeight Total number of rows in the grid.
 * @return Row index in the editor grid (top-origin).
 */
static int GridRowFromWorldGy(int worldGy, int gridHeight) {
    return (gridHeight - 1) - worldGy;
}

// Default zoom limits/step
namespace {
    constexpr float kNavMinZoom = 0.6f;
    constexpr float kNavMaxZoom = 4.00f;
    constexpr float kNavZoomStep = 1.10f;   // multiplier per wheel notch

    /**
     * @brief Compute squared distance between two points.
     * @param a First point.
     * @param b Second point.
     * @return Squared Euclidean distance.
     */
    float DistSq_(const Vector2& a, const Vector2& b) {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        return dx * dx + dy * dy;
    }

    /**
     * @brief Collect LightComponent values from the current ECS into LightRecord snapshots.
     *
     * Only entities that have both a Transform and a LightComponent are included.
     * If @p getPrefabTag is provided, the record's prefabTag is filled for matching
     * during later re-application.
     *
     * @param ctx ECS context used to query components.
     * @param getPrefabTag Optional callback for retrieving an entity's prefab tag.
     * @return Vector of LightRecord values suitable for sidecar JSON serialization.
     */
    std::vector<LightRecord> CollectLightRecords_(IComponentContext& ctx, const RoomEditor::GetPrefabTagFn& getPrefabTag) {
        std::vector<LightRecord> out;
        const auto& entities = ctx.GetEntities();
        out.reserve(entities.size());

        for (Entity e : entities) {
            const Transform* t = ctx.TryGetTransform(e);
            const LightComponent* l = ctx.TryGetLight(e);
            if (!t || !l)
                continue;

            LightRecord r{};
            if (getPrefabTag)
                r.prefabTag = getPrefabTag(e);

            const Vector2 p = t->GetPosition();
            r.position[0] = p.x;
            r.position[1] = p.y;

            r.enabled = l->enabled;

            r.glowEnabled = l->glow.enabled;
            r.glowRadius = l->glow.radius;
            r.glowColor[0] = l->glow.color.x;
            r.glowColor[1] = l->glow.color.y;
            r.glowColor[2] = l->glow.color.z;
            r.glowOffset[0] = l->glow.offset.x;
            r.glowOffset[1] = l->glow.offset.y;
            r.glowIntensity = l->glow.intensity;
            r.glowOpacity = l->glow.opacity;
            r.glowSoftness = l->glow.softness;

            r.sourceEnabled = l->source.enabled;
            r.sourceRadius = l->source.radius;
            r.sourceColor[0] = l->source.color.x;
            r.sourceColor[1] = l->source.color.y;
            r.sourceColor[2] = l->source.color.z;
            r.sourceOffset[0] = l->source.offset.x;
            r.sourceOffset[1] = l->source.offset.y;
            r.sourceIntensity = l->source.intensity;
            r.sourceOpacity = l->source.opacity;
            r.sourceAttenuation = l->source.attenuation;

            r.emberEnabled = l->ember.enabled;
            r.emberRate = l->ember.rate;
            r.emberParticleLife = l->ember.particleLife;
            r.emberVelMin[0] = l->ember.velMin.x;
            r.emberVelMin[1] = l->ember.velMin.y;
            r.emberVelMax[0] = l->ember.velMax.x;
            r.emberVelMax[1] = l->ember.velMax.y;
            r.emberColorStart[0] = l->ember.colorStart.x;
            r.emberColorStart[1] = l->ember.colorStart.y;
            r.emberColorStart[2] = l->ember.colorStart.z;
            r.emberColorEnd[0] = l->ember.colorEnd.x;
            r.emberColorEnd[1] = l->ember.colorEnd.y;
            r.emberColorEnd[2] = l->ember.colorEnd.z;
            r.emberSizeStart = l->ember.sizeStart;
            r.emberSizeEnd = l->ember.sizeEnd;
            r.emberAdditive = l->ember.additive;
            r.emberOffset[0] = l->ember.offset.x;
            r.emberOffset[1] = l->ember.offset.y;

            r.flickerEnabled = l->flicker.enabled;
            r.flickerIntensityMin = l->flicker.intensityMin;
            r.flickerIntensityMax = l->flicker.intensityMax;
            r.flickerSpeed = l->flicker.speed;
            r.flickerTime = l->flicker.time;

            out.push_back(r);
        }

        return out;
    }

    /**
     * @brief Find the best matching entity for a saved LightRecord.
     *
     * The record is matched to the nearest entity by Transform position, with an
     * optional prefabTag filter when provided in the record and a tag callback is
     * available.
     *
     * @param ctx ECS context used to enumerate entities/transforms.
     * @param r   Saved LightRecord to match.
     * @param getPrefabTag Optional callback for retrieving an entity's prefab tag.
     * @param maxDistSq Maximum squared distance allowed for matching.
     * @return Best matching entity, or INVALID_ENTITY if no match is found.
     */
    Entity FindBestLightTarget_(IComponentContext& ctx, const LightRecord& r, const RoomEditor::GetPrefabTagFn& getPrefabTag, float maxDistSq) {
        const Vector2 target{ r.position[0], r.position[1] };
        Entity best = INVALID_ENTITY;
        float bestDistSq = maxDistSq;

        const auto& entities = ctx.GetEntities();
        for (Entity e : entities) {
            const Transform* t = ctx.TryGetTransform(e);
            if (!t)
                continue;

            if (!r.prefabTag.empty() && getPrefabTag) {
                const std::string tag = getPrefabTag(e);
                if (tag != r.prefabTag)
                    continue;
            }

            const float d2 = DistSq_(t->GetPosition(), target);
            if (d2 < bestDistSq) {
                bestDistSq = d2;
                best = e;
            }
        }

        return best;
    }

    /**
     * @brief Apply a LightRecord snapshot onto an entity's LightComponent.
     *
     * If the entity does not already have a LightComponent, this will add one.
     *
     * @param ctx ECS context used to add/query/write LightComponent.
     * @param e   Target entity.
     * @param r   Snapshot values to apply.
     */
    void ApplyLightRecord_(IComponentContext& ctx, Entity e, const LightRecord& r) {
        LightComponent* l = ctx.GetLight(e);
        if (!l) {
            ctx.AddLightComponent(e);
            l = ctx.GetLight(e);
        }
        if (!l)
            return;

        l->enabled = r.enabled;

        l->glow.enabled = r.glowEnabled;
        l->glow.radius = r.glowRadius;
        l->glow.color = Vector3(r.glowColor[0], r.glowColor[1], r.glowColor[2]);
        l->glow.offset = Vector2(r.glowOffset[0], r.glowOffset[1]);
        l->glow.intensity = r.glowIntensity;
        l->glow.opacity = r.glowOpacity;
        l->glow.softness = r.glowSoftness;

        l->source.enabled = r.sourceEnabled;
        l->source.radius = r.sourceRadius;
        l->source.color = Vector3(r.sourceColor[0], r.sourceColor[1], r.sourceColor[2]);
        l->source.offset = Vector2(r.sourceOffset[0], r.sourceOffset[1]);
        l->source.intensity = r.sourceIntensity;
        l->source.opacity = r.sourceOpacity;
        l->source.attenuation = r.sourceAttenuation;

        l->ember.enabled = r.emberEnabled;
        l->ember.rate = r.emberRate;
        l->ember.particleLife = r.emberParticleLife;
        l->ember.velMin = Vector2(r.emberVelMin[0], r.emberVelMin[1]);
        l->ember.velMax = Vector2(r.emberVelMax[0], r.emberVelMax[1]);
        l->ember.colorStart = Vector3(r.emberColorStart[0], r.emberColorStart[1], r.emberColorStart[2]);
        l->ember.colorEnd = Vector3(r.emberColorEnd[0], r.emberColorEnd[1], r.emberColorEnd[2]);
        l->ember.sizeStart = r.emberSizeStart;
        l->ember.sizeEnd = r.emberSizeEnd;
        l->ember.additive = r.emberAdditive;
        l->ember.offset = Vector2(r.emberOffset[0], r.emberOffset[1]);

        l->flicker.enabled = r.flickerEnabled;
        l->flicker.intensityMin = r.flickerIntensityMin;
        l->flicker.intensityMax = r.flickerIntensityMax;
        l->flicker.speed = r.flickerSpeed;
        l->flicker.time = r.flickerTime;
    }
}



// ---- injected dependencies ----
/**
 * @brief Set the callback for retrieving the active room ID.
 * @param fn Function returning the current active room name.
 */
void RoomEditor::SetGetActiveRoom(GetActiveRoomFn fn) { getActiveRoom_ = std::move(fn); }

/**
 * @brief Set the callback for resolving a room ID to its text layout file path.
 * @param fn Function mapping room ID to filesystem path.
 */
void RoomEditor::SetResolveRoomTxtPath(ResolveRoomTxtPathFn fn) { resolveRoomTxtPath_ = std::move(fn); }

/**
 * @brief Set the callback for resolving a room ID to its variant data file path.
 * @param fn Function mapping room ID to variant data path.
 */
void RoomEditor::SetResolveRoomVariantPath(ResolveRoomVariantPathFn fn) { resolveRoomVariantPath_ = std::move(fn); }

void RoomEditor::SetResolveRoomLightPath(ResolveRoomLightPathFn fn) { resolveRoomLightPath_ = std::move(fn); }

/**
 * @brief Set the callback for loading room grid data from a text file.
 * @param fn Function for loading grid dimensions and tiles.
 */
void RoomEditor::SetLoadGrid(LoadGridFn fn) { loadGrid_ = std::move(fn); }

/**
 * @brief Set the callback for applying edited room changes to the game world.
 * @param fn Function invoked when room data is applied.
 */
void RoomEditor::SetApplyCallback(ApplyRoomFn fn) { onApply_ = std::move(fn); }

/**
 * @brief Set the callback for drawing custom properties in the entity editor.
 * @param fn Function for rendering additional ImGui property fields.
 */
void RoomEditor::SetCustomPropertiesDraw(CustomPropertiesDrawFn fn) { customPropertiesDraw_ = std::move(fn); }

/**
 * @brief Initialize the room editor.
 * 
 * Resets all internal state to default values, clears cached room data,
 * grid dimensions, and navigation/selection states. Resolves the assets root directory.
 */
void RoomEditor::Init() {

    roomIdCached.clear();
    roomTxtPath.clear();
    grid.clear();

    width = height = 0;
    loaded = false;
    dirty = false;
    dirtyVariants_ = false;
    paint = '1';

    requestDockRebuild_ = false;
    prevActive_ = false;
    visible_ = false;
    wantsViewportInput_ = false;

    firstDockBuild_ = true;
    centerDockId_ = 0;
    dockspaceId_ = 0;

    selectedEntity_ = INVALID_ENTITY;
    hasSelection_ = false;
    selectedGx_ = selectedGy_ = -1;
    multiSelection_.clear();
    paintbrushMode_ = false;
    lastPaintCell_ = { -1, -1 };
    selectedPrefabKey_.clear();

    navMode_ = false;
    navLock_ = false;
    navPanning_ = false;

    lastWantFollow_ = true;
    prevVisible_ = false;

    assetEntries_.clear();
    selectedAssetIdx_ = -1;
    assetStatus_.clear();
    importSrcPath_[0] = 0;
    importNewName_[0] = 0;

    assetsRoot_ = ResolveAssetsRoot_();
}

/**
 * @brief Update the editor state.
 * @param dt Delta time in seconds since last update (unused).
 *
 * Handles state updates, dirty flag management, and room change detection.
 * Manages dock layout rebuild requests, viewport input state, and camera follow modes.
 * Enforces navigation rules and handles editor context transitions.
 */
void RoomEditor::Update(float /*dt*/) {
    UpdateDockRebuildRequests_();

    const bool nowContext = IsInRoomContext();
    UpdateContextAndVisibility_(nowContext);

    wantsViewportInput_ = false;

    HandleResumeReset_();

    if (HandleNotInContextOrHidden_(nowContext))
        return;

    UpdateFollowMode_();

    EnforceNavModeRules_();

    OnRoomChanged();
}


/**
 * @brief Draw the editor UI
 *
 * Renders the ImGui interface for the room editor including tools,
 * grid view, and controls. Creates two windows: "Room Tools" for
 * editing controls and "Room Grid" for tile painting.
 */
void RoomEditor::Draw() {
    if (!IsInRoomContext() || !visible_)
        return;

    if (!playStateFn_) {
        DrawPlayControlsWindow_();
        return;
    }

    const EditorPlayControlsState st = playStateFn_();
    const bool allowEditor = (!st.isPlaying || st.isPaused);

    if (BeginDockHost_()) {
        BuildDockLayoutIfNeeded_();
        EndDockHost_();
    }
    else return;



    DrawPlayControlsWindow_();
    DrawViewportWindow_();   // (center)

    if (!allowEditor) ImGui::BeginDisabled(true);


    DrawVariantPaletteWindow_();
    DrawVariantListWindow_();
    DrawAssetsWindow_();
    DrawPropertiesWindow_();

    if (!allowEditor) ImGui::EndDisabled();

}

/**
 * @brief Shutdown the editor
 *
 * Clears all internal state and resources.
 */
void RoomEditor::Shutdown() {
    roomIdCached.clear();
    roomTxtPath.clear();
    grid.clear();

    loaded = false;
    dirty = false;
    dirtyVariants_ = false;
    width = height = 0;

    requestDockRebuild_ = false;
    prevActive_ = false;
    wantsViewportInput_ = false;
    selectedEntity_ = INVALID_ENTITY;
    hasSelection_ = false;
    selectedGx_ = selectedGy_ = -1;
    multiSelection_.clear();
    paintbrushMode_ = false;

    lastPaintCell_ = { -1, -1 };
    selectedPrefabKey_.clear();

    navMode_ = false;
    navLock_ = false;
    navPanning_ = false;

    lastWantFollow_ = true;
    prevVisible_ = false;

    assetEntries_.clear();
    selectedAssetIdx_ = -1;
    assetStatus_.clear();
    importSrcPath_[0] = 0;
    importNewName_[0] = 0;
}

/**
 * @brief Set the play controls callbacks and state getter.
 * @param getState Function that returns the current play/stop state.
 * @param cbs Callbacks for play/stop control operations.
 */
void RoomEditor::SetPlayControls(GetPlayStateFn getState, EditorPlayControlsCallbacks cbs) {
    playStateFn_ = std::move(getState);
    playCbs_ = std::move(cbs);
}

/**
 * @brief Set the scene texture for display in the viewport.
 * @param texId OpenGL texture ID of the scene
 * @param w Width of the scene texture
 * @param h Height of the scene texture
 */
void RoomEditor::SetSceneTexture(unsigned int texId, int w, int h) {
    sceneTextureId_ = texId;
    sceneTexW_ = w;
    sceneTexH_ = h;
}

/**
 * @brief Check if the editor is currently active
 * @return true if editor UI is visible and active
 * @return false if editor UI is hidden or inactive
 *
 * The editor is considered active when a valid room callback exists
 * and the current room ID contains "entities_Level".
 */
bool RoomEditor::IsInRoomContext() const {
    if (!getActiveRoom_) return false;
    const std::string room = getActiveRoom_();
    return room.find("entities_Level") != std::string::npos;
}

/**
 * @brief Check if the editor is currently active.
 * @return true if editor is visible and in room context.
 */
bool RoomEditor::IsActive() const {
    return visible_ && IsInRoomContext();
}

Vector2 RoomEditor::GetWorldMousePosition() const {
    Camera2D* cam = getActiveCamera_ ? getActiveCamera_() : activeCamera_;
    if (!cam) return Vector2(0, 0);

    if (viewportImageSize_.x <= 0 || viewportImageSize_.y <= 0) return Vector2(0, 0);

    const ImVec2 m = ImGui::GetIO().MousePos;
    const float u = (m.x - viewportImagePos_.x) / viewportImageSize_.x;
    const float v = 1.0f - (m.y - viewportImagePos_.y) / viewportImageSize_.y;

    const float px = u * cam->viewportWidth;
    const float py = v * cam->viewportHeight;

    return cam->ScreenToWorld(Vector2(px, py));
}

bool RoomEditor::IsMouseInViewport() const {
    const ImVec2 m = ImGui::GetIO().MousePos;
    return (m.x >= viewportImagePos_.x && m.x <= viewportImagePos_.x + viewportImageSize_.x &&
            m.y >= viewportImagePos_.y && m.y <= viewportImagePos_.y + viewportImageSize_.y);
}

/**
 * @brief Detects a Resume transition (paused -> running) and restores default camera state.
 * 
 * When the game resumes from a paused state, navigation state and camera zoom
 * are reset to ensure the editor doesn't interfere with gameplay.
 */
void RoomEditor::HandleResumeReset_() {
    static bool wasPaused = false;

    if (!playStateFn_) {
        wasPaused = false;
        return;
    }

    const EditorPlayControlsState st = playStateFn_();
    const bool isPausedNow = (st.isPlaying && st.isPaused);

    // Resume pressed: previously paused, now running
    if (wasPaused && !isPausedNow) {
        ResetNavigationState_();
        ForceFollowAndResetZoom_();

        // Clear selection when resuming gameplay
        hasSelection_ = false;
        selectedEntity_ = INVALID_ENTITY;
        selectedGx_ = -1;
        selectedGy_ = -1;
        multiSelection_.clear();
    }

    wasPaused = isPausedNow;
}

/**
 * @brief Handles the case where the editor is not in a valid room context or not visible.
 * @param nowContext True if the current active room is an editable room context.
 * @return true if the caller should early-return from Update(), false otherwise.
 * 
 * Resets navigation and ensures camera follow mode is restored when exiting editor context.
 */
bool RoomEditor::HandleNotInContextOrHidden_(bool nowContext) {
    if (nowContext && visible_)
        return false;

    ResetNavigationState_();

    // Ensure we are following player when editor isn't active
    if (requestCameraLock_ && !lastWantFollow_) {
        requestCameraLock_(true);
        lastWantFollow_ = true;
    }

    return true; // caller should return
}

/**
 * @brief Begin the docking host window for editor layout.
 *
 * Creates a full-screen docking host window with no borders or title bar,
 * sets up a dock space for organizing editor windows.
 */
bool RoomEditor::BeginDockHost_() {
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

    bool opened = ImGui::Begin("##RoomDockHost", nullptr, hostFlags);

    ImGui::PopStyleVar(2);

    if (!opened) {
        ImGui::End();
        return false;
    }

    dockspaceId_ = ImGui::GetID("RoomEditorDockSpace");
    const ImGuiDockNodeFlags dockFlags =
        ImGuiDockNodeFlags_PassthruCentralNode |
        ImGuiDockNodeFlags_NoDockingSplit |
        ImGuiDockNodeFlags_NoUndocking;
    ImGui::DockSpace(dockspaceId_, ImVec2(0, 0), dockFlags);
    return true;
}


/**
 * @brief End the docking host window.
 *
 * Closes the docking host window created by BeginDockHost_.
 */
void RoomEditor::EndDockHost_() {
    ImGui::End();
}

/**
 * @brief Build or rebuild the dock layout if needed.
 *
 * Creates a dock layout with viewport in center, tools on right top,
 * and grid editor on right bottom. Only rebuilds when requested.
 */
/**
 * @brief Build or rebuild the dock layout for the editor.
 * 
 * Clears the existing dock layout and creates a new one with 
 * designated areas for the viewport, item palette, variant list, 
 * assets browser, and property editor.
 */
void RoomEditor::BuildDockLayoutIfNeeded_() {
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
        (ImGuiDockNodeFlags)ImGuiDockNodeFlags_PassthruCentralNode |
        (ImGuiDockNodeFlags)ImGuiDockNodeFlags_NoDockingSplit |
        (ImGuiDockNodeFlags)ImGuiDockNodeFlags_NoUndocking
    );

    ImGui::DockBuilderSetNodeSize(dockspaceId_, vp->WorkSize);

    ImGuiID dock_main = dockspaceId_;

    // Top strip for play controls 
    ImGuiID dock_top = ImGui::DockBuilderSplitNode(
        dock_main, ImGuiDir_Up, 0.12f, nullptr, &dock_main
    );

    // Right sidebar (IMGUI panels)
    ImGuiID dock_right = ImGui::DockBuilderSplitNode(
        dock_main, ImGuiDir_Right, 0.22f, nullptr, &dock_main
    );

    // Split right sidebar into Tools (top) + Grid (bottom)
    ImGuiID dock_right_bottom = ImGui::DockBuilderSplitNode(
        dock_right, ImGuiDir_Down, 0.40f, nullptr, &dock_right
    );

    centerDockId_ = dock_main;

    ImGui::DockBuilderDockWindow("Room Play Controls", dock_top);
    ImGui::DockBuilderDockWindow("Item Palette", dock_right);

    ImGuiID dock_right_bottom_right = ImGui::DockBuilderSplitNode(
        dock_right_bottom, ImGuiDir_Right, 0.5f, nullptr, &dock_right_bottom
    );

    ImGui::DockBuilderDockWindow("Assets", dock_right_bottom);
    ImGui::DockBuilderDockWindow("Variants", dock_right_bottom);
    ImGui::DockBuilderDockWindow("Property Selector##Room", dock_right_bottom_right);

    const ImGuiDockNodeFlags lockFlags =
        ImGuiDockNodeFlags_NoDockingSplit |
        ImGuiDockNodeFlags_NoUndocking;

    auto lockNode = [&](ImGuiID id) {
        if (ImGuiDockNode* n = ImGui::DockBuilderGetNode(id)) {
            n->LocalFlags |= lockFlags;
        }
    };

    lockNode(dockspaceId_);
    lockNode(dock_top);
    lockNode(dock_right);
    lockNode(dock_right_bottom);
    lockNode(dock_right_bottom_right);
    lockNode(dock_main);

    ImGui::DockBuilderFinish(dockspaceId_);
}

/**
 * @brief Updates dock rebuild request state based on viewport work area changes.
 */
void RoomEditor::UpdateDockRebuildRequests_() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    static ImVec2 lastWorkSize = ImVec2(0, 0);

    if (vp && (vp->WorkSize.x != lastWorkSize.x || vp->WorkSize.y != lastWorkSize.y)) {
        requestDockRebuild_ = true;
        lastWorkSize = vp->WorkSize;
    }
}

/**
 * @brief Updates cached context/visibility flags and triggers dock rebuild when state changes.
 * @param nowContext True if the current active room is an editable room context.
 */
void RoomEditor::UpdateContextAndVisibility_(bool nowContext) {
    if (nowContext != prevActive_) {
        requestDockRebuild_ = true;
        prevActive_ = nowContext;
    }

    if (visible_ && !prevVisible_)
        requestDockRebuild_ = true;

    prevVisible_ = visible_;
}

/**
 * @brief Draw the main viewport window containing the scene texture.
 * 
 * Handles viewport layout, coordinate mapping from world to ImGui space,
 * and mouse input for navigation and editing tools.
 */
void RoomEditor::DrawViewportWindow_() {

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

    if (!ImGui::Begin("Room Viewport", nullptr,
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

    Camera2D* cam = getActiveCamera_ ? getActiveCamera_() : activeCamera_;

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

    if (cam && drawSize.x > 1.0f && drawSize.y > 1.0f) {

        // Only update if different to avoid rebuilding every frame
        if (cam && sceneTexW_ > 0 && sceneTexH_ > 0) {
            if (cam->viewportWidth != (float)sceneTexW_ ||
                cam->viewportHeight != (float)sceneTexH_) {
                cam->SetViewport(0, 0, sceneTexW_, sceneTexH_);
            }
        }
    }

    // Cache for MouseToWorld_()
    viewportImagePos_ = drawPos;
    viewportImageSize_ = drawSize;

    // Background
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(cursor,
        ImVec2(cursor.x + avail.x, cursor.y + avail.y),
        IM_COL32(15, 15, 15, 255));

    // ---- Draw scene texture ----
    if (drawSize.x <= 1.0f || drawSize.y <= 1.0f) {
        ImGui::End();
        return;
    }

    ImGui::SetCursorScreenPos(drawPos);
    ImGui::Image((ImTextureID)(intptr_t)sceneTextureId_, drawSize, ImVec2(0, 1), ImVec2(1, 0));

    // Use rect test for hover
    const ImVec2 imgMin = drawPos;
    const ImVec2 imgMax = ImVec2(drawPos.x + drawSize.x, drawPos.y + drawSize.y);
    const bool hoveredImage = ImGui::IsMouseHoveringRect(imgMin, imgMax, false);

    // Check play state
    bool isPlaying = false;
    bool isPaused = false;
    if (playStateFn_) {
        EditorPlayControlsState st = playStateFn_();
        isPlaying = st.isPlaying;
        isPaused = st.isPaused;
    }

    const bool allowEditInViewport = ((!navMode_) || (navMode_ && navLock_)) && (!isPlaying || isPaused);

    // Focus on click 
    if (hoveredImage && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        ImGui::SetWindowFocus();
    }

    // Recompute focus AFTER focusing
    const bool focusedWin = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    // Update dock rebuild requests
    UpdateDockRebuildRequests_();

    // ---- NAV INPUT (UPDATED) ----
    HandleNavigationInput_(cam, hoveredImage, focusedWin);

    // ---- (A) Left-click select ----
    if (allowEditInViewport && hoveredImage) {
        TrySelectEntityUnderMouse_();
    }

    // ---- (B) Delete key deletes selected ----
    if (allowEditInViewport && hoveredImage && focusedWin) {
        TryDeleteSelected_();
    }

    // ---- [P] keyboard shortcut to toggle paintbrush ----
    if (allowEditInViewport && focusedWin && !ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_P, false)) {
            paintbrushMode_ = !paintbrushMode_;
            lastPaintCell_ = { -1, -1 };
        }
    }

    // ---- (C) Paintbrush painting ----
    if (allowEditInViewport && paintbrushMode_) {
        DrawPaintbrushTool_(dl, cam, hoveredImage);
    }

    // ---- Multi-select outlines ----
    if (!multiSelection_.empty() && cam &&
        cam->viewportWidth > 0 && cam->viewportHeight > 0 &&
        viewportImageSize_.x > 0.0f && viewportImageSize_.y > 0.0f)
    {
        const float ts = tileSize_;

        auto worldToImGui = [&](const Vector2& world) -> ImVec2 {
            Vector2 screen = cam->WorldToScreen(world);
            float u = screen.x / cam->viewportWidth;
            float v = screen.y / cam->viewportHeight;
            return ImVec2(
                viewportImagePos_.x + u * viewportImageSize_.x,
                viewportImagePos_.y + (1.0f - v) * viewportImageSize_.y
            );
            };

        for (auto& [gx, gy] : multiSelection_) {
            const bool isPrimary = (gx == selectedGx_ && gy == selectedGy_);
            const ImU32 col = isPrimary
                ? IM_COL32(0, 220, 255, 255)   // bright cyan = primary
                : IM_COL32(0, 180, 200, 200);   // dimmer teal = secondary
            const float thickness = isPrimary ? 2.5f : 1.5f;


            Entity e = INVALID_ENTITY;
            auto it = liveEntities_.find({ gx, gy });
            if (it != liveEntities_.end()) e = it->second;

            if (e != INVALID_ENTITY && context_) {
                const Transform* t = context_->TryGetTransform(e);
                const Collider* c = context_->GetCollider(e);

                if (t && c) {
                    Vector2 pos = t->GetPosition();
                    Vector2 scale = t->GetScale();
                    float rot = t->GetRotation();

                    // Center = pos + 0.5 * scale
                    Vector2 center = pos + scale * 0.5f;

                    if (c->type == ColliderType::Box) {
                        Vector2 halfSize = c->size * 0.5f;
                        Vector2 corners[4] = {
                            center + Vector2(-halfSize.x, -halfSize.y).Rotate(rot),
                            center + Vector2(halfSize.x, -halfSize.y).Rotate(rot),
                            center + Vector2(halfSize.x, halfSize.y).Rotate(rot),
                            center + Vector2(-halfSize.x, halfSize.y).Rotate(rot)
                        };

                        ImVec2 pIm[4];
                        for (int i = 0; i < 4; ++i) pIm[i] = worldToImGui(corners[i]);
                        dl->AddPolyline(pIm, 4, col, ImDrawFlags_Closed, thickness);
                        continue;
                    }
                    else if (c->type == ColliderType::Circle) {
                        float radius = c->size.x;
                        ImVec2 centerIm = worldToImGui(center);

                        // Project a point on the circle edge to get screen-space radius
                        ImVec2 edgeIm = worldToImGui(center + Vector2(radius, 0));
                        float screenRadius = static_cast<float>(std::sqrt(std::pow(edgeIm.x - centerIm.x, 2) + std::pow(edgeIm.y - centerIm.y, 2)));

                        dl->AddCircle(centerIm, screenRadius, col, 32, thickness);
                        continue;
                    }
                    else if (c->type == ColliderType::Triangle) {
                        Vector2 p1 = center + c->triPt1.Rotate(rot);
                        Vector2 p2 = center + c->triPt2.Rotate(rot);
                        Vector2 p3 = center + c->triPt3.Rotate(rot);

                        ImVec2 pIm[3] = { worldToImGui(p1), worldToImGui(p2), worldToImGui(p3) };
                        dl->AddPolyline(pIm, 3, col, ImDrawFlags_Closed, thickness);
                        continue;
                    }
                }
            }

            // Fallback: Grid-based drawing if no entity/collider found
            Vector2 bl(gx * ts, gy * ts);
            Vector2 tr((gx + 1) * ts, (gy + 1) * ts);

            ImVec2 p0 = worldToImGui(bl);
            ImVec2 p1 = worldToImGui(tr);

            ImVec2 pMin(std::min(p0.x, p1.x), std::min(p0.y, p1.y));
            ImVec2 pMax(std::max(p0.x, p1.x), std::max(p0.y, p1.y));

            dl->AddRect(pMin, pMax, col, 0.0f, 0, thickness);
        }

        // Selection count hint
        if (multiSelection_.size() > 1) {
            char hint[64];
            std::snprintf(hint, sizeof(hint),
                "%d selected  |  Del = delete all  |  Shift+click to toggle",
                (int)multiSelection_.size());
            dl->AddText(
                ImVec2(viewportImagePos_.x + 8.0f,
                    viewportImagePos_.y + viewportImageSize_.y - 20.0f),
                IM_COL32(0, 220, 255, 220), hint);
        }
    }

    // ---- Drag-drop placement  ----
    if (allowEditInViewport && hoveredImage && ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload =
            ImGui::AcceptDragDropPayload(
                "WALL_TEXTUREKEY",
                ImGuiDragDropFlags_AcceptBeforeDelivery |
                ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
        {
            if (payload->IsDelivery()) {   // only commit on drop release
                const char* droppedKey = (const char*)payload->Data;
                int gx, gy;
                if (MouseToGrid_(gx, gy)) {
                    UpsertWallVariant_(gx, gy, droppedKey ? droppedKey : "wall_B&T", true);
                }
            }
        }

        if (const ImGuiPayload* payload =
            ImGui::AcceptDragDropPayload(
                "PREFAB_KEY",
                ImGuiDragDropFlags_AcceptBeforeDelivery |
                ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
        {
            if (payload->IsDelivery()) {
                const char* droppedPrefab = (const char*)payload->Data;
                int gx, gy;
                if (MouseToGrid_(gx, gy)) {
                    UpsertPrefabVariant_(gx, gy, droppedPrefab ? droppedPrefab : "Crate");
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    // ---- Drag preview (yellow) ONLY while dragging a wall or prefab payload ----
    if (allowEditInViewport && hoveredImage && ImGui::IsDragDropActive()) {
        const ImGuiPayload* p = ImGui::GetDragDropPayload();
        const bool draggingWall = (p && p->IsDataType("WALL_TEXTUREKEY"));
        const bool draggingPrefab = (p && p->IsDataType("PREFAB_KEY"));

        if (draggingWall || draggingPrefab) {
            int gx, gy;
            if (MouseToGrid_(gx, gy)) {
                Camera2D* camP = getActiveCamera_ ? getActiveCamera_() : activeCamera_;
                if (camP && camP->viewportWidth > 0 && camP->viewportHeight > 0) {

                    const float ts = tileSize_;

                    auto world_to_imgui = [&](const Vector2& world) -> ImVec2 {
                        Vector2 screen = camP->WorldToScreen(world);
                        float u = screen.x / camP->viewportWidth;
                        float v = screen.y / camP->viewportHeight; // 0 bottom -> 1 top
                        float ix = viewportImagePos_.x + u * viewportImageSize_.x;
                        float iy = viewportImagePos_.y + (1.0f - v) * viewportImageSize_.y;
                        return ImVec2(ix, iy);
                        };

                    Vector2 bl(gx * ts, gy * ts);
                    Vector2 tr((gx + 1) * ts, (gy + 1) * ts);

                    ImVec2 p_bl = world_to_imgui(bl);
                    ImVec2 p_tr = world_to_imgui(tr);

                    ImVec2 mn(std::min(p_bl.x, p_tr.x), std::min(p_bl.y, p_tr.y));
                    ImVec2 mx(std::max(p_bl.x, p_tr.x), std::max(p_bl.y, p_tr.y));

                    // Try to get texture for preview
                    unsigned int texId = 0;
                    if (draggingWall && getTextureID_) {
                        texId = getTextureID_((const char*)p->Data);
                    }
                    else if (draggingPrefab && getPrefabTexture_) {
                        texId = getPrefabTexture_((const char*)p->Data);
                    }

                    if (texId != 0) {
                        // Draw texture preview
                        dl->AddImage((ImTextureID)(uintptr_t)texId, mn, mx, ImVec2(0, 1), ImVec2(1, 0), IM_COL32(255, 255, 255, 180));
                        // Add a subtle border
                        dl->AddRect(mn, mx, IM_COL32(255, 255, 0, 150), 0.0f, 0, 2.0f);
                    }
                    else {
                        // Fallback to yellow box if no texture found
                        dl->AddRectFilled(mn, mx, IM_COL32(255, 255, 0, 120));
                        dl->AddRect(mn, mx, IM_COL32(255, 255, 0, 220));
                    }
                }
            }
        }
    }

    ImGui::End();
}



/**
 * @brief Draw the variant palette window for selecting wall textures and prefabs.
 * 
 * Provides a list of available wall textures and prefabs for painting.
 * Supports drag-and-drop of items from the palette into the viewport.
 */
void RoomEditor::DrawVariantPaletteWindow_() {
    if (!ImGui::Begin("Item Palette")) { ImGui::End(); return; }

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        wantsViewportInput_ = true;

    const std::string activeRoom = getActiveRoom_ ? getActiveRoom_() : std::string{};
    ImGui::Text("Room: %s", activeRoom.c_str());
    ImGui::Text("Grid: %dx%d | tileSize: %.1f", width, height, tileSize_);
    ImGui::Separator();

    // Your wall keys (from your texture atlas map)
    static const char* kWallKeys[] = {
        "wall_B&T",
        "wall_BL_1", "wall_BL_2",
        "wall_BR_1", "wall_BR_2",
        "wall_B_2",
        "wall_L_1", "wall_L_2", "wall_L_3",
        "wall_R_1", "wall_R_2", "wall_R_3",
        "wall_TL", "wall_TR",
        "Floor_Tile"
    };

    // ---- tabs ----
    if (ImGui::BeginTabBar("##PaletteTabs", ImGuiTabBarFlags_None)) {

        if (ImGui::BeginTabItem("Walls")) {
            ImGui::Text("Drag a wall into the viewport:");
            ImGui::Separator();

            ImGui::BeginChild("##WallScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            for (const char* key : kWallKeys) {
                const bool selected = (selectedTextureKey_ == key);

                ImGui::PushID(key);

                // Texture Thumbnail
                unsigned int texId = getTextureID_ ? getTextureID_(key) : 0;
                if (texId != 0) {
                    ImGui::Image((ImTextureID)(uintptr_t)texId, ImVec2(24, 24), ImVec2(0, 1), ImVec2(1, 0));
                    ImGui::SameLine();
                }

                if (ImGui::Selectable(key, selected)) {
                    selectedTextureKey_ = key;
                    selectedPrefabKey_.clear();
                }

                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    // Preview in tool tip
                    if (texId != 0) {
                        ImGui::Image((ImTextureID)(uintptr_t)texId, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));
                        ImGui::SameLine();
                    }
                    ImGui::Text("Place: %s", key);
                    ImGui::SetDragDropPayload("WALL_TEXTUREKEY", key, strlen(key) + 1);
                    ImGui::EndDragDropSource();
                }

                ImGui::PopID();
            }
            ImGui::EndChild();

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Enemies")) {
            ImGui::Text("Drag an enemy into the viewport:");
            ImGui::Separator();

            ImGui::BeginChild("##EnemyScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            for (const auto& prefabName : availablePrefabs_) {
                // Heuristic: if name contains "Enemy", "Boss", or ends with "mini_boss"
                bool isEnemy = prefabName.find("Enemy") != std::string::npos ||
                    prefabName.find("Boss") != std::string::npos ||
                    prefabName.find("mini_boss") != std::string::npos;

                // Exclude puzzle objects from Enemies tab even if they match heuristics
                bool isPuzzle = (prefabName == "BurrowWall") ||
                                (prefabName == "Lever") ||
                                (prefabName == "LeverActivated") ||
                                (prefabName == "PuzzleGenerator") ||
                                (prefabName == "PuzzleGenerator_On") ||
                                (prefabName == "healingMemory") ||
                                (prefabName == "healingMemory_On") ||
                                (prefabName == "DoorLock") ||
                                (prefabName == "MutationHealer") ||
                                (prefabName == "Floor_Tile_Fragile");

                if (isEnemy && !isPuzzle) {
                    ImGui::PushID(prefabName.c_str());

                    // Texture Thumbnail
                    unsigned int texId = getPrefabTexture_ ? getPrefabTexture_(prefabName) : 0;
                    if (texId != 0) {
                        ImGui::Image((ImTextureID)(uintptr_t)texId, ImVec2(24, 24), ImVec2(0, 1), ImVec2(1, 0));
                        ImGui::SameLine();
                    }

                    if (ImGui::Selectable(prefabName.c_str())) {
                        selectedPrefabKey_ = prefabName;
                    }

                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                        if (texId != 0) {
                            ImGui::Image((ImTextureID)(uintptr_t)texId, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));
                            ImGui::SameLine();
                        }
                        ImGui::Text("Place: %s", prefabName.c_str());
                        ImGui::SetDragDropPayload("PREFAB_KEY", prefabName.c_str(), prefabName.size() + 1);
                        ImGui::EndDragDropSource();
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Puzzle Objects")) {
            ImGui::Text("Drag a puzzle object into the viewport:");
            ImGui::Separator();

            ImGui::BeginChild("##PuzzleScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            for (const auto& prefabName : availablePrefabs_) {
                // Heuristic: exactly 9 items total
                bool isPuzzle = (prefabName == "BurrowWall") ||
                                (prefabName == "Lever") ||
                                (prefabName == "LeverActivated") ||
                                (prefabName == "PuzzleGenerator") ||
                                (prefabName == "PuzzleGenerator_On") ||
                                (prefabName == "healingMemory") ||
                                (prefabName == "healingMemory_On") ||
                                (prefabName == "DoorLock") ||
                                (prefabName == "MutationHealer") ||
                                (prefabName == "Floor_Tile_Fragile");

                if (isPuzzle) {
                    ImGui::PushID(prefabName.c_str());

                    // Texture Thumbnail
                    unsigned int texId = getPrefabTexture_ ? getPrefabTexture_(prefabName) : 0;
                    if (texId != 0) {
                        ImGui::Image((ImTextureID)(uintptr_t)texId, ImVec2(24, 24), ImVec2(0, 1), ImVec2(1, 0));
                        ImGui::SameLine();
                    }

                    if (ImGui::Selectable(prefabName.c_str())) {
                        selectedPrefabKey_ = prefabName;
                    }

                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                        if (texId != 0) {
                            ImGui::Image((ImTextureID)(uintptr_t)texId, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));
                            ImGui::SameLine();
                        }
                        ImGui::Text("Place: %s", prefabName.c_str());
                        ImGui::SetDragDropPayload("PREFAB_KEY", prefabName.c_str(), prefabName.size() + 1);
                        ImGui::EndDragDropSource();
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Environment")) {
            ImGui::Text("Drag an object into the viewport:");
            ImGui::Separator();

            ImGui::BeginChild("##EnvScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            for (const auto& prefabName : availablePrefabs_) {
                // Heuristic: if not enemy/boss/mini_boss and not player
                bool isEnemy = prefabName.find("Enemy") != std::string::npos ||
                    prefabName.find("Boss") != std::string::npos ||
                    prefabName.find("mini_boss") != std::string::npos;
                bool isPuzzle = (prefabName == "BurrowWall") ||
                    (prefabName == "Lever") ||
                    (prefabName == "LeverActivated") ||
                    (prefabName == "PuzzleGenerator") ||
                    (prefabName == "PuzzleGenerator_On") ||
                    (prefabName == "healingMemory") ||
                    (prefabName == "healingMemory_On") ||
                    (prefabName == "DoorLock") ||
                    (prefabName == "MutationHealer") ||
                    (prefabName == "Floor_Tile_Fragile");
                bool isPlayer = prefabName == "Player";
                bool isWall = prefabName == "Wall";
                bool isDoor = prefabName == "Doorway";

                if (!isEnemy && !isPuzzle && !isPlayer && !isWall && !isDoor) {
                    ImGui::PushID(prefabName.c_str());

                    // Texture Thumbnail
                    unsigned int texId = getPrefabTexture_ ? getPrefabTexture_(prefabName) : 0;
                    if (texId != 0) {
                        ImGui::Image((ImTextureID)(uintptr_t)texId, ImVec2(24, 24), ImVec2(0, 1), ImVec2(1, 0));
                        ImGui::SameLine();
                    }

                    if (ImGui::Selectable(prefabName.c_str())) {
                        selectedPrefabKey_ = prefabName;
                    }

                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                        if (texId != 0) {
                            ImGui::Image((ImTextureID)(uintptr_t)texId, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));
                            ImGui::SameLine();
                        }
                        ImGui::Text("Place: %s", prefabName.c_str());
                        ImGui::SetDragDropPayload("PREFAB_KEY", prefabName.c_str(), prefabName.size() + 1);
                        ImGui::EndDragDropSource();
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::Separator();

    // ---- Paintbrush indicator (toggle with [P] key) ----
    if (paintbrushMode_) {
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "Paintbrush: ON  (press P to toggle)");
        if (!selectedPrefabKey_.empty())
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "Armed: %s", selectedPrefabKey_.c_str());
        else
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "Armed: %s", selectedTextureKey_.c_str());
    }
    else {
        ImGui::TextDisabled("Paintbrush: OFF (press P to toggle)");
    }

    ImGui::Separator();

    if (ImGui::Button("Save Room")) {
        std::string stem = roomIdCached;
        if (getActiveRoom_) {
            std::filesystem::path p(getActiveRoom_());
            stem = p.stem().string();
        }
        if (Variant::SaveRoomData(roomTxtPath, roomVariantPath, stem, tileSize_, grid, variants_)) {
            dirty = false;
            dirtyVariants_ = false;
        }
    }

    ImGui::SameLine();
    if (dirtyVariants_)
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Unsaved variant changes");

    ImGui::TextDisabled("Left-click & press 'Del' in viewport to delete.");

    ImGui::End();
}


/**
 * @brief Draw the list of all current variants in the room.
 * 
 * Lists all `VariantPlacement` objects currently tracked, allowing 
 * selection and inspection.
 */
void RoomEditor::DrawVariantListWindow_() {
    if (!ImGui::Begin("Variants")) { ImGui::End(); return; }

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        wantsViewportInput_ = true;

    ImGui::Text("Count: %d", (int)variants_.size());
    if (multiSelection_.size() > 1)
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f),
            "%d selected  (Shift+click in viewport to toggle)", (int)multiSelection_.size());
    ImGui::Separator();

    for (int i = 0; i < (int)variants_.size(); ++i) {
        auto& v = variants_[i];
        ImGui::PushID(i);

        // Highlight row if this variant's cell is in the multi-selection
        auto cell = std::make_pair(v.gx, v.gy);
        bool isInSel = std::find(multiSelection_.begin(), multiSelection_.end(), cell)
            != multiSelection_.end();

        if (isInSel)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.9f, 1.0f, 1.0f));

        if (v.type == "WallTile") {
            ImGui::Text("[%d] Wall (%d,%d) %s solid=%s",
                i, v.gx, v.gy, v.textureKey.c_str(), v.solid ? "true" : "false");
        }
        else {
            ImGui::Text("[%d] Prefab: %s (%d,%d)",
                i, v.type.c_str(), v.gx, v.gy);
            ImGui::SameLine();
            if (ImGui::Checkbox("Signature", &v.hasSignature)) {
                dirtyVariants_ = true;
            }
        }

        if (isInSel)
            ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::SmallButton("X")) {
            variants_.erase(variants_.begin() + i);
            dirtyVariants_ = true;
            // Remove from multi-selection too
            multiSelection_.erase(
                std::remove(multiSelection_.begin(), multiSelection_.end(), cell),
                multiSelection_.end());
            ImGui::PopID();
            break;
        }
        ImGui::PopID();
    }

    // Bulk delete button when multiple cells are selected
    if (multiSelection_.size() > 1) {
        ImGui::Separator();
        if (ImGui::Button("Delete All Selected")) {
            DeleteMultiSelection_();
        }
    }

    ImGui::End();
}


/**
 * @brief Draw the play controls window for simulation/testing.
 * 
 * Includes buttons for resuming, resetting, or pausing the room simulation.
 */
void RoomEditor::DrawPlayControlsWindow_() {
    if (!ImGui::Begin("Room Play Controls")) { ImGui::End(); return; }

    if (ImGui::BeginTabBar("##TopTabs"))
    {
        // -------------------- Play tab --------------------
        if (ImGui::BeginTabItem("Play"))
        {
            const float btnW = 110.0f, btnH = 28.0f, gap = 10.0f;

            // If not wired, just show disabled controls
            if (!playStateFn_) {
                ImGui::BeginDisabled(true);
                EditorPlayControlsState st{};
                EditorPlayControlsCallbacks cb{};
                PlayStopSystem::DrawPlayControls(st, cb, btnW, btnH, gap);
                ImGui::EndDisabled();
            }
            else {
                EditorPlayControlsState st = playStateFn_();
                PlayControlsUIResult r = PlayStopSystem::DrawPlayControls(st, playCbs_, btnW, btnH, gap);

                if (r.pressed)
                    PlayStopSystem::UpdatePlayControls(st, playCbs_, r.action);
            }

            ImGui::EndTabItem();
        }

        // ---------------- Navigation tab ------------------
        if (ImGui::BeginTabItem("Navigation"))
        {
            EditorPlayControlsState st{};
            if (playStateFn_) st = playStateFn_();

            const bool allowNavigation = (!st.isPlaying || st.isPaused);
            ImGui::BeginDisabled(!allowNavigation);

            ImGui::Checkbox("Enable Navigation", &navMode_);

            // lock can only be toggled when navMode_ is enabled
            ImGui::BeginDisabled(!navMode_);
            ImGui::SameLine();
            ImGui::Checkbox("Lock View", &navLock_);
            ImGui::EndDisabled();
            ImGui::EndDisabled();

            if (!navMode_) {
                ImGui::TextDisabled("Navigation OFF: camera follows player.");
            }
            else {
                if (navLock_) ImGui::TextDisabled("Locked: edit allowed");
                else          ImGui::TextDisabled("Navigation ON: scroll zoom + left-drag pan.");
            }

            ImGui::Separator();
            ImGui::TextDisabled("Scroll = Zoom, Left-drag = Pan (only when Navigation ON + not Locked).");

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}



/**
 * @brief Find the index of a variant at the specified grid coordinates.
 * @param gx Grid X coordinate.
 * @param gy Grid Y coordinate.
 * @return Index in the variants list, or -1 if not found.
 */
int RoomEditor::FindVariantIndexAt_(int gx, int gy) const {
    for (int i = 0; i < (int)variants_.size(); ++i) {
        if (variants_[i].gx == gx && variants_[i].gy == gy)
            return i;
    }
    return -1;
}


/**
 * @brief Insert or update a wall variant at the specified grid coordinates.
 * @param gx Grid X coordinate.
 * @param gy Grid Y coordinate.
 * @param textureKey Texture key for the wall.
 * @param solid Whether the wall is solid.
 */
void RoomEditor::UpsertWallVariant_(int gx, int gy, const std::string& textureKey, bool solid) {
    // Remove existing entity at this position first
    RemoveVariantAt_(gx, gy);

    // Create actual game entity immediately via callback
    if (createWallEntity_) {
        Entity newEntity = createWallEntity_(gx, gy, textureKey, solid, tileSize_);
        if (newEntity != INVALID_ENTITY) {
            auto key = std::make_pair(gx, gy);
            liveEntities_[key] = newEntity;
        }
    }

    // Update the TXT grid immediately
    if (gx >= 0 && gx < width && gy >= 0 && gy < height) {
        const int row = GridRowFromWorldGy(gy, height);
        if (row >= 0 && row < (int)grid.size() && gx < (int)grid[row].size()) {
            if (grid[row][gx] != '2')
                grid[row][gx] = '0'; // Prefabs are NOT walls
            dirty = true;
        }
    }

    // Update metadata for saving
    int idx = FindVariantIndexAt_(gx, gy);
    if (idx < 0) {
        VariantPlacement vp{};
        vp.type = "WallTile";
        vp.gx = gx;
        vp.gy = gy;
        vp.textureKey = textureKey;
        vp.solid = solid;
        vp.name = "V_WallTile_" + std::to_string(gx) + "_" + std::to_string(gy);
        variants_.push_back(std::move(vp));
    }
    else {
        variants_[idx].textureKey = textureKey;
        variants_[idx].solid = solid;
    }

    dirtyVariants_ = true;
}

/**
 * @brief Insert or update a prefab variant at the specified grid coordinates.
 * @param gx Grid X coordinate.
 * @param gy Grid Y coordinate.
 * @param prefabName Name of the prefab.
 */
void RoomEditor::UpsertPrefabVariant_(int gx, int gy, const std::string& prefabName) {
    // Remove existing entity at this position first
    RemoveVariantAt_(gx, gy);

    // Create actual game entity immediately via callback
    if (createPrefabEntity_) {
        Entity newEntity = createPrefabEntity_(gx, gy, prefabName, tileSize_);
        if (newEntity != INVALID_ENTITY) {
            auto key = std::make_pair(gx, gy);
            liveEntities_[key] = newEntity;
        }
    }

    // Update the TXT grid immediately
    if (gx >= 0 && gx < width && gy >= 0 && gy < height) {
        const int row = GridRowFromWorldGy(gy, height);
        if (row >= 0 && row < (int)grid.size() && gx < (int)grid[row].size()) {
            if (grid[row][gx] != '2') {
                // If it's a wall or door lock, it should be '1' (wall)
                // If it's a crate/torch/enemy, it should be '0' (walkable)
                if (prefabName == "BlackWall" || prefabName == "BlackTile" || 
                    prefabName == "BurrowWall" || prefabName == "DoorLock") {
                    grid[row][gx] = '1';
                }
                else {
                    grid[row][gx] = '0';
                }
            }
            dirty = true;
        }
    }

    // Update metadata for saving
    int idx = FindVariantIndexAt_(gx, gy);
    if (idx < 0) {
        VariantPlacement vp{};
        vp.type = prefabName;
        vp.gx = gx;
        vp.gy = gy;
        vp.textureKey = ""; // Prefabs use their own texture from definition
        vp.solid = true;    // Prefabs use their own collision from definition
        vp.name = "V_" + prefabName + "_" + std::to_string(gx) + "_" + std::to_string(gy);
        variants_.push_back(std::move(vp));
    }
    else {
        variants_[idx].type = prefabName;
        variants_[idx].name = "V_" + prefabName + "_" + std::to_string(gx) + "_" + std::to_string(gy);
    }

    dirtyVariants_ = true;
}




/**
 * @brief Remove a variant at the specified grid coordinates.
 * @param gx Grid X coordinate.
 * @param gy Grid Y coordinate.
 */
/**
 * @brief Remove a variant at the specified grid coordinates.
 * @param gx Grid X coordinate.
 * @param gy Grid Y coordinate.
 * 
 * Removes the variant from the metadata list, destroys the 
 * corresponding game entity, and updates the grid character.
 */
void RoomEditor::RemoveVariantAt_(int gx, int gy) {
    // Remove actual entity if it exists
    auto key = std::make_pair(gx, gy);
    auto it = liveEntities_.find(key);
    if (it != liveEntities_.end()) {

        if (destroyEntity_) {
            destroyEntity_(it->second);
        }
        liveEntities_.erase(it);
    }

    // Update the TXT grid immediately
    if (gx >= 0 && gx < width && gy >= 0 && gy < height) {
        const int row = GridRowFromWorldGy(gy, height);
        if (row >= 0 && row < (int)grid.size() && gx < (int)grid[row].size()) {
            // don't erase door
            if (grid[row][gx] != '2')
                grid[row][gx] = '0';
            dirty = true;
        }
    }

    int idx = FindVariantIndexAt_(gx, gy);
    if (idx >= 0) {
        variants_.erase(variants_.begin() + idx);
        dirtyVariants_ = true;
    }
}


/**
 * @brief Convert mouse screen coordinates to grid coordinates.
 * @param gx Output grid X coordinate.
 * @param gy Output grid Y coordinate.
 * @return true if mouse is within the grid.
 */
bool RoomEditor::MouseToGrid_(int& gx, int& gy) const {
    Vector2 w;
    if (!MouseToWorld_(w)) return false;

    gx = (int)std::floor(w.x / tileSize_);
    gy = (int)std::floor(w.y / tileSize_);

    if (gx < 0 || gy < 0 || gx >= width || gy >= height) return false;
    return true;
}


/**
 * @brief Convert mouse screen coordinates to world coordinates.
 * @param outWorld Output world coordinates.
 * @return true if mouse is within the viewport.
 */
bool RoomEditor::MouseToWorld_(Vector2& outWorld) const {
    Camera2D* cam = getActiveCamera_ ? getActiveCamera_() : activeCamera_;
    if (!cam) return false;

    if (viewportImageSize_.x <= 0 || viewportImageSize_.y <= 0) return false;
    if (cam->viewportWidth <= 0 || cam->viewportHeight <= 0) return false;

    const ImVec2 m = ImGui::GetIO().MousePos;

    // inside drawn image rect only
    if (m.x < viewportImagePos_.x || m.y < viewportImagePos_.y ||
        m.x >= viewportImagePos_.x + viewportImageSize_.x ||
        m.y >= viewportImagePos_.y + viewportImageSize_.y)
        return false;

    // mouse -> normalized UV in the IMAGE rect
    const float u = (m.x - viewportImagePos_.x) / viewportImageSize_.x;         // 0..1 left->right
    const float v = 1.0f - (m.y - viewportImagePos_.y) / viewportImageSize_.y;  // 0..1 bottom->top

    // UV -> camera "screen pixels" (bottom-left origin)
    const float px = u * cam->viewportWidth;
    const float py = v * cam->viewportHeight;

    outWorld = cam->ScreenToWorld(Vector2(px, py));
    return true;
}

/**
 * @brief Destroy all live entities tracked by the editor.
 */
void RoomEditor::ClearAllLiveEntities_() {
    if (destroyEntity_) {
        for (const auto& [pos, entity] : liveEntities_) {
            destroyEntity_(entity);
        }
    }
    liveEntities_.clear();
}

/**
 * @brief Scan the scene and populate liveEntities_ with existing wall entities.
 */
void RoomEditor::PopulateLiveEntitiesFromScene_() {
    if (!getEntitiesInRect_) {
        return;
    }
    liveEntities_.clear();
    float ts = tileSize_;
    int foundCount = 0;
    for (int gy = 0; gy < height; ++gy) {
        for (int gx = 0; gx < width; ++gx) {
            Vector2 worldPos(gx * ts, gy * ts);
            Vector2 worldSize(ts, ts);

            // Query entities at this grid position
            std::vector<Entity> entities = getEntitiesInRect_(worldPos, worldSize);

            for (Entity e : entities) {
                // Check if this is a relevant entity (has a prefab tag)
                std::string tag = getPrefabTag_(e);

                if (!tag.empty()) {
                    liveEntities_[std::make_pair(gx, gy)] = e;
                    foundCount++;
                    break;
                }
            }
        }
    }
}

/**
 * @brief Handle viewport navigation input (pan and zoom).
 * @param cam Active camera to manipulate.
 * @param hoveredImage Whether the viewport image is currently hovered.
 * @param focusedWin Whether the viewport window is currently focused.
 */
void RoomEditor::HandleNavigationInput_(Camera2D* cam, bool hoveredImage, bool focusedWin) {
    // Navigation only when enabled AND not locked
    if (!navMode_ || navLock_ || !cam) {
        navPanning_ = false;
        return;
    }

    if (!hoveredImage || !focusedWin) {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            navPanning_ = false;
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    // -------------------- Zoom (scroll) --------------------
    if (io.MouseWheel != 0.0f) {

        Vector2 centerPx(cam->viewportWidth * 0.5f, cam->viewportHeight * 0.5f);
        Vector2 worldBefore = cam->ScreenToWorld(centerPx);

        float z = cam->GetZoom();
        z = std::clamp(z * std::pow(kNavZoomStep, io.MouseWheel), kNavMinZoom, kNavMaxZoom);

        cam->setZoom(z);
        cam->UpdateView();

        Vector2 worldAfter = cam->ScreenToWorld(centerPx);
        cam->setPosition(cam->GetPosition() + (worldBefore - worldAfter));
        cam->UpdateView();
    }

    // -------------------- Pan (left-drag) --------------------
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        navPanning_ = true;
    }

    if (navPanning_) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ImVec2 d = io.MouseDelta;

            // Convert UI pixels to camera pixels 
            float dx_cam = d.x * (cam->viewportWidth / viewportImageSize_.x);
            float dy_cam = d.y * (cam->viewportHeight / viewportImageSize_.y);

            //  convert camera pixel delta to world delta 
            Vector2 worldDelta(
                -dx_cam / cam->GetZoom(),
                +dy_cam / cam->GetZoom()
            );

            cam->setPosition(cam->GetPosition() + worldDelta);
            cam->UpdateView();
        }
        else {
            navPanning_ = false;
        }
    }
}

/**
 * @brief Resets all navigation state flags (panning, zoom, and navigation mode).
 */
void RoomEditor::ResetNavigationState_() {
    navMode_ = false;
    navLock_ = false;
    navPanning_ = false;
}

/**
 * @brief Restores camera follow mode and resets zoom to default (1.0).
 */
void RoomEditor::ForceFollowAndResetZoom_() {
    if (requestCameraLock_) {
        requestCameraLock_(true);
        lastWantFollow_ = true;
    }

    if (Camera2D* cam = getActiveCamera_ ? getActiveCamera_() : activeCamera_) {
        cam->setZoom(1.0f);
        cam->UpdateView();
    }
}


/**
 * @brief Toggles camera follow mode based on current navigation mode.
 * 
 * When navigation mode is off, the camera follows the player.
 */
void RoomEditor::UpdateFollowMode_() {
    // follow player only when navMode_ is off
    const bool wantFollow = !navMode_;

    if (requestCameraLock_ && wantFollow != lastWantFollow_) {
        requestCameraLock_(wantFollow);
        lastWantFollow_ = wantFollow;

        // when returning to follow, reset zoom
        if (wantFollow) {
            if (Camera2D* cam = getActiveCamera_ ? getActiveCamera_() : activeCamera_) {
                cam->setZoom(1.0f);
                cam->UpdateView();
            }
        }
    }
}

/**
 * @brief Ensures navigation flags are consistent when navigation mode is disabled.
 */
void RoomEditor::EnforceNavModeRules_() {
    if (!navMode_) {
        navLock_ = false;
        navPanning_ = false;
    }
}


/**
 * @brief Resolves the assets root directory at runtime by walking up the directory tree.
 * @return Canonical filesystem path to the assets directory.
 */
std::filesystem::path RoomEditor::ResolveAssetsRoot_() {
    std::error_code ec;

    std::filesystem::path p = std::filesystem::current_path();

    std::filesystem::path firstAssetsCandidate; // remember, don't early return

    while (!p.empty()) {
        ec.clear();

        // 1) Prefer source tree: .../Game/assets
        auto gameAssets = p / "Game" / "assets";
        if (std::filesystem::exists(gameAssets, ec) &&
            std::filesystem::is_directory(gameAssets, ec)) {
            return std::filesystem::weakly_canonical(gameAssets, ec);
        }

        // 2) Remember nearest .../assets, but KEEP WALKING
        auto candidate = p / "assets";
        if (firstAssetsCandidate.empty() &&
            std::filesystem::exists(candidate, ec) &&
            std::filesystem::is_directory(candidate, ec)) {
            firstAssetsCandidate = candidate; // don't return yet
        }

        auto parent = p.parent_path();
        if (parent == p) break;
        p = parent;
    }

    // 3) If we never found Game/assets, use the first assets we saw
    if (!firstAssetsCandidate.empty()) {
        return std::filesystem::weakly_canonical(firstAssetsCandidate, ec);
    }

    // 4) Last resort: create ./assets
    auto fallback = std::filesystem::current_path() / "assets";
    std::filesystem::create_directories(fallback, ec);
    return std::filesystem::weakly_canonical(fallback, ec);
}




/**
 * @brief Resolves the directory path for a given asset bucket.
 * @param b Asset bucket type.
 * @return Filesystem path corresponding to the chosen bucket.
 */
std::filesystem::path RoomEditor::BucketPath_(AssetBucket b) const {
    switch (b) {
    case AssetBucket::Atlases:    return assetsRoot_ / "atlases";
    case AssetBucket::Audio:      return assetsRoot_ / "audio";
    case AssetBucket::Fonts:      return assetsRoot_ / "fonts";
    case AssetBucket::Textures:   return assetsRoot_ / "textures";
    default:                      return assetsRoot_;
    }
}

/**
 * @brief Checks whether a filesystem path is within the editor's assets root folder.
 * @param p Path to validate.
 * @return true if the canonical path lies under assetsRoot_, false otherwise.
 */
bool RoomEditor::IsInsideAssetsRoot_(const std::filesystem::path& p) const {
    std::error_code ec;
    auto root = std::filesystem::weakly_canonical(assetsRoot_, ec);
    if (ec) return false;

    auto cand = std::filesystem::weakly_canonical(p, ec);
    if (ec) return false;

    auto r = root.generic_string();
    auto c = cand.generic_string();
    if (!r.empty() && r.back() != '/') r.push_back('/');

    return c.rfind(r, 0) == 0; // cand starts with root
}

/**
 * @brief Refresh the list of assets in the current bucket by scanning the directory.
 */
void RoomEditor::RefreshAssets_() {
    assetEntries_.clear();
    selectedAssetIdx_ = -1;

    std::error_code ec;
    std::filesystem::create_directories(assetsRoot_, ec);

    auto folder = BucketPath_(assetBucket_);
    std::filesystem::create_directories(folder, ec);

    for (auto& e : std::filesystem::directory_iterator(folder, ec)) {
        if (ec) break;
        if (!e.is_regular_file()) continue;
        assetEntries_.push_back(e);
    }

    std::sort(assetEntries_.begin(), assetEntries_.end(),
        [](auto& a, auto& b) {
            return a.path().filename().string() < b.path().filename().string();
        });
}

/**
 * @brief Import (copy) an external file into the assets folder.
 * @param src Source path (external file).
 * @param dst Destination path under assetsRoot_.
 * @return true if the import succeeded, false otherwise.
 */
bool RoomEditor::ImportFile_(const std::filesystem::path& src,
    const std::filesystem::path& dst) {
    std::error_code ec;

    if (!std::filesystem::exists(src, ec) || ec) {
        assetStatus_ = "Import failed: source not found.";
        return false;
    }
    if (!std::filesystem::is_regular_file(src, ec) || ec) {
        assetStatus_ = "Import failed: source is not a file.";
        return false;
    }

    std::filesystem::create_directories(dst.parent_path(), ec);

    std::filesystem::copy_file(src, dst,
        std::filesystem::copy_options::overwrite_existing, ec);

    if (ec) {
        assetStatus_ = std::string("Import failed: ") + ec.message();
        return false;
    }

    assetStatus_ = "Imported: " + dst.filename().string();
    RefreshAssets_();
    return true;
}

/**
 * @brief Deletes a file from disk within the assets directory.
 * @param p File path to delete.
 * @return true if deletion succeeded, false otherwise.
 */
bool RoomEditor::DeleteFile_(const std::filesystem::path& p) {
    if (!IsInsideAssetsRoot_(p)) {
        assetStatus_ = "Refused: can only delete inside assets/";
        return false;
    }

    std::error_code ec;
    bool ok = std::filesystem::remove(p, ec);
    if (!ok || ec) {
        assetStatus_ = std::string("Delete failed: ") + (ec ? ec.message() : "unknown");
        return false;
    }

    assetStatus_ = "Deleted: " + p.filename().string();
    RefreshAssets_();
    return true;
}

/**
 * @brief Draw the assets browser window with bucket selection and file operations.
 */
void RoomEditor::DrawAssetsWindow_() {
    if (!ImGui::Begin("Assets")) { ImGui::End(); return; }

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        wantsViewportInput_ = true;

    // bucket selector
    const char* labels[] = { "atlases", "audio", "fonts", "textures" };
    int cur = (int)assetBucket_;
    if (ImGui::Combo("Folder", &cur, labels, IM_ARRAYSIZE(labels))) {
        assetBucket_ = (AssetBucket)cur;
        RefreshAssets_();
    }

    ImGui::TextDisabled("Root: %s", std::filesystem::absolute(assetsRoot_).string().c_str());

    if (ImGui::SmallButton("Refresh")) RefreshAssets_();

    // lazy refresh
    if (assetEntries_.empty())
        RefreshAssets_();

    ImGui::Separator();
    ImGui::Text("Add (copy file into assets):");
    ImGui::InputText("Source path", importSrcPath_, IM_ARRAYSIZE(importSrcPath_));
    ImGui::InputText("Rename (optional)", importNewName_, IM_ARRAYSIZE(importNewName_));

    if (ImGui::Button("Import")) {
        std::filesystem::path src(importSrcPath_);
        std::filesystem::path name =
            (std::strlen(importNewName_) > 0) ? std::filesystem::path(importNewName_) : src.filename();
        std::filesystem::path dst = BucketPath_(assetBucket_) / name;
        ImportFile_(src, dst);
    }

    if (!assetStatus_.empty()) {
        ImGui::Spacing();
        ImGui::TextWrapped("%s", assetStatus_.c_str());
    }

    ImGui::Separator();

    ImGui::BeginChild("##asset_list", ImVec2(0, 220), true);
    for (int i = 0; i < (int)assetEntries_.size(); ++i) {
        auto n = assetEntries_[i].path().filename().string();
        bool sel = (selectedAssetIdx_ == i);
        if (ImGui::Selectable(n.c_str(), sel)) selectedAssetIdx_ = i;
    }
    ImGui::EndChild();

    if (selectedAssetIdx_ >= 0 && selectedAssetIdx_ < (int)assetEntries_.size()) {
        auto p = assetEntries_[selectedAssetIdx_].path();
        ImGui::Text("Selected: %s", p.filename().string().c_str());
        ImGui::TextDisabled("%s", p.string().c_str());

        if (ImGui::Button("Delete...")) {
            ImGui::OpenPopup("ConfirmDeleteAsset");
        }

        if (ImGui::BeginPopupModal("ConfirmDeleteAsset", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextWrapped("Delete this file from disk?\n\n%s", p.string().c_str());
            ImGui::Separator();

            if (ImGui::Button("Delete", ImVec2(120, 0))) {
                DeleteFile_(p);
                selectedAssetIdx_ = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}



/**
 * @brief Handle room change detection and loading
 *
 * Detects when the active room has changed and triggers loading
 * of the new room data. Only processes rooms containing "entities_Level".
 */
void RoomEditor::OnRoomChanged() {

    if (!getActiveRoom_) return;

    const std::string room = getActiveRoom_();
    if (room == roomIdCached)
        return;

    ClearAllLiveEntities_();
    roomIdCached.clear();
    loaded = false;
    grid.clear();

    if (room.find("entities_Level") == std::string::npos)
        return;

    roomIdCached = room;
    LoadRoomFiles();
}

/**
 * @brief Load available room files from disk
 *
 * Uses the registered callbacks to resolve the room ID to a file path,
 * then loads the grid data from that file. Updates internal state
 * with the loaded dimensions and tile data.
 */
void RoomEditor::LoadRoomFiles() {
    loaded = false;

    if (!getActiveRoom_ || !resolveRoomTxtPath_ || !loadGrid_)
        return;

    const std::string room = getActiveRoom_();
    roomTxtPath = resolveRoomTxtPath_(room);

    if (resolveRoomVariantPath_)
        roomVariantPath = resolveRoomVariantPath_(room);
    else
        roomVariantPath.clear();

    if (resolveRoomLightPath_)
        roomLightPath = resolveRoomLightPath_(room);
    else
        roomLightPath.clear();

    grid.clear();
    int outW = 0, outH = 0;

    if (!loadGrid_(roomTxtPath, grid, outW, outH)) {
        loaded = false;
        return;
    }

    width = outW;
    height = outH;
    loaded = (width > 0 && height > 0);

    variants_.clear();
    dirtyVariants_ = false;
    dirtyLights_ = false;

    if (loaded && !roomVariantPath.empty()) {
        // Load the metadata from JSON
        Variant::LoadFile(roomVariantPath, height, variants_);
    }

    if (loaded && context_ && !roomLightPath.empty()) {
        std::vector<LightRecord> records;
        if (LightEffects::LoadFile(roomLightPath, records)) {
            const float maxDistSq = (tileSize_ > 0.0f ? (tileSize_ * 0.5f) * (tileSize_ * 0.5f) : 4096.0f);
            for (const auto& r : records) {
                Entity target = FindBestLightTarget_(*context_, r, getPrefabTag_, maxDistSq);
                if (target != INVALID_ENTITY)
                    ApplyLightRecord_(*context_, target, r);
            }
        }
    }

    // scan for any existing walls not in JSON
    PopulateLiveEntitiesFromScene_();

    dirty = false;
}


/**
 * @brief Validate the current room layout
 * @param outMsg Output parameter for validation message
 * @return true if room layout is valid
 * @return false if room layout has errors
 *
 * Performs validation checks on the current room grid.
 * Currently only validates that exactly one door tile ('2') exists.
 */
bool RoomEditor::Validate(std::string& outMsg) const {
    if (!loaded) { outMsg = "Room not loaded."; return false; }

    int doorCount = 0;
    for (const auto& row : grid)
        for (char c : row)
            if (c == '2') ++doorCount;

    if (doorCount != 1) {
        outMsg = "Room must contain exactly ONE '2' door marker.";
        return false;
    }

    outMsg.clear();
    return true;
}

/**
 * @brief Apply current room changes to the game
 *
 * Validates the current room layout and, if valid, calls the
 * registered apply callback to push changes to the game system.
 * Clears the dirty flag after successful application.
 */
void RoomEditor::Apply() {
    std::string msg;
    if (!Validate(msg))
        return;

    if (onApply_) {
        onApply_(grid, width, height, roomIdCached);
        dirty = false;
    }
}

/**
 * @brief Save the current room to disk
 *
 * Serializes the current room grid to a text file at the
 * cached roomTxtPath. Each row is written as a separate line.
 * Clears the dirty flag after successful save.
 */
void RoomEditor::Save() {
    if (roomTxtPath.empty()) return;
    std::ofstream out(roomTxtPath);
    if (!out.is_open()) return;
    for (auto& row : grid) out << row << "\n";
    dirty = false;
}


/**
 * @brief Checks whether an entity is protected from editor deletion.
 * @param e Entity to test.
 * @return true if the entity should not be deleted by editor operations.
 * 
 * Doors and player entities are currently protected from deletion.
 */
bool RoomEditor::IsProtectedEntity_(Entity e) const {
    if (e == INVALID_ENTITY) return true;

    if (getPrefabTag_) {
        std::string tag = getPrefabTag_(e);

        for (char& c : tag) c = (char)std::tolower((unsigned char)c);

        if (tag.find("door") != std::string::npos) return true;
        if (tag.find("player") != std::string::npos) return true;
    }

    return false;
}

/**
 * @brief Removes an entity from the liveEntities_ mapping if it is owned/tracked by the editor.
 * @param e Entity to remove from the internal ownership map.
 */
void RoomEditor::RemoveFromLiveEntitiesIfOwned_(Entity e) {
    if (e == INVALID_ENTITY) return;

    for (auto it = liveEntities_.begin(); it != liveEntities_.end(); ++it) {
        if (it->second == e) {
            liveEntities_.erase(it);
            return;
        }
    }
}


/**
 * @brief Attempt to select an entity under the mouse cursor in the viewport.
 * 
 * Handles single selection, multi-selection with Shift key, and clearing 
 * selection when clicking on empty space.
 */
void RoomEditor::TrySelectEntityUnderMouse_() {
    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        return;
    }

    const bool shiftHeld = ImGui::GetIO().KeyShift;

    int gx, gy;
    if (!MouseToGrid_(gx, gy)) {
        // Click on empty space with no shift: clear everything
        if (!shiftHeld) {
            hasSelection_ = false;
            selectedEntity_ = INVALID_ENTITY;
            selectedGx_ = selectedGy_ = -1;
            multiSelection_.clear();
        }
        return;
    }

    auto cell = std::make_pair(gx, gy);

    if (shiftHeld) {
        // Shift-click: toggle this cell in/out of multi-selection
        auto it = std::find(multiSelection_.begin(), multiSelection_.end(), cell);
        if (it != multiSelection_.end()) {
            // Already selected: deselect it
            multiSelection_.erase(it);
            if (selectedGx_ == gx && selectedGy_ == gy) {
                // Primary was removed move primary to last remaining
                if (!multiSelection_.empty()) {
                    selectedGx_ = multiSelection_.back().first;
                    selectedGy_ = multiSelection_.back().second;
                    auto lit = liveEntities_.find(multiSelection_.back());
                    selectedEntity_ = (lit != liveEntities_.end()) ? lit->second : INVALID_ENTITY;
                    hasSelection_ = (selectedEntity_ != INVALID_ENTITY);
                }
                else {
                    selectedEntity_ = INVALID_ENTITY;
                    hasSelection_ = false;
                    selectedGx_ = selectedGy_ = -1;
                }
            }
        }
        else {
            // Not yet selected: add it
            multiSelection_.push_back(cell);
            selectedGx_ = gx;
            selectedGy_ = gy;
            auto lit = liveEntities_.find(cell);
            selectedEntity_ = (lit != liveEntities_.end()) ? lit->second : INVALID_ENTITY;
            hasSelection_ = true;
        }
        return;
    }

    // Cache the cell we clicked no matter what
    // Plain click: replace selection with just this cell
    multiSelection_.clear();
    selectedGx_ = gx;
    selectedGy_ = gy;

    // Prefer liveEntities mapping
    auto it = liveEntities_.find(cell);
    if (it != liveEntities_.end()) {
        selectedEntity_ = it->second;
        hasSelection_ = (selectedEntity_ != INVALID_ENTITY);
        multiSelection_.push_back(cell);
        return;
    }

    // Fallback: query scene, but still keep the gx/gy we clicked
    if (!getEntitiesInRect_) {
        hasSelection_ = false;
        selectedEntity_ = INVALID_ENTITY;
        return;
    }

    Vector2 world{};
    if (!MouseToWorld_(world)) {
        hasSelection_ = false;
        selectedEntity_ = INVALID_ENTITY;
        return;
    }

    const float pick = std::max(6.0f, tileSize_ * 0.20f);
    Vector2 pickSize{ pick, pick };
    Vector2 pickMin = world - pickSize * 0.5f;

    std::vector<Entity> hits = getEntitiesInRect_(pickMin, pickSize);
    if (hits.empty()) {
        hasSelection_ = false;
        selectedEntity_ = INVALID_ENTITY;
        return;
    }

    selectedEntity_ = hits.back();
    hasSelection_ = (selectedEntity_ != INVALID_ENTITY);
    if (hasSelection_) {
        multiSelection_.push_back(cell);
        liveEntities_[cell] = selectedEntity_;
    }
}




/**
 * @brief Handles paintbrush painting in the room viewport.
 *
 * When paintbrush mode is active, holding LMB over the viewport continuously
 * paints the hovered grid cell with the currently armed wall texture or prefab.
 *
 * - Tracks the last painted cell so each cell is only painted once per drag stroke.
 * - On mouse release the last-painted cell tracker resets, ready for the next stroke.
 * - Draws a green highlight over the hovered cell as a cursor preview.
 * - Overwrites whatever was previously at that cell (replace-anything behaviour).
 * - Skips door cells ('2') to protect room connections.
 */
void RoomEditor::DrawPaintbrushTool_(ImDrawList* dl, Camera2D* cam, bool hoveredImage) {
    // Nothing armed yet nothing to paint
    if (selectedTextureKey_.empty() && selectedPrefabKey_.empty()) return;

    if (!cam || cam->viewportWidth <= 0 || cam->viewportHeight <= 0) return;
    if (viewportImageSize_.x <= 0.0f || viewportImageSize_.y <= 0.0f) return;

    const float ts = tileSize_;

    // World-to-ImGui projection (same as used elsewhere in this file)
    auto worldToImGui = [&](const Vector2& world) -> ImVec2 {
        Vector2 screen = cam->WorldToScreen(world);
        float u = screen.x / cam->viewportWidth;
        float v = screen.y / cam->viewportHeight;
        return ImVec2(
            viewportImagePos_.x + u * viewportImageSize_.x,
            viewportImagePos_.y + (1.0f - v) * viewportImageSize_.y
        );
        };

    int gx, gy;
    const bool onGrid = MouseToGrid_(gx, gy);

    // Mouse released: reset stroke tracker
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        lastPaintCell_ = { -1, -1 };
    }

    // Draw green hover-cell preview whenever the mouse is over a valid cell
    if (onGrid) {
        // Skip door cells visually too
        const int row = (height - 1) - gy;
        const bool isDoor = (row >= 0 && row < (int)grid.size() &&
            gx >= 0 && gx < (int)grid[row].size() &&
            grid[row][gx] == '2');

        Vector2 bl(gx * ts, gy * ts);
        Vector2 tr((gx + 1) * ts, (gy + 1) * ts);
        ImVec2 p0 = worldToImGui(bl);
        ImVec2 p1 = worldToImGui(tr);
        ImVec2 pMin(std::min(p0.x, p1.x), std::min(p0.y, p1.y));
        ImVec2 pMax(std::max(p0.x, p1.x), std::max(p0.y, p1.y));

        if (isDoor) {
            // Orange tint = protected, won't paint
            dl->AddRect(pMin, pMax, IM_COL32(255, 140, 0, 220), 0.0f, 0, 2.0f);
        }
        else {
            // Green fill + border = will paint here
            dl->AddRectFilled(pMin, pMax, IM_COL32(50, 220, 80, 60));
            dl->AddRect(pMin, pMax, IM_COL32(50, 220, 80, 220), 0.0f, 0, 2.0f);

            // Show armed item name inside the cell
            const std::string& armLabel = selectedPrefabKey_.empty()
                ? selectedTextureKey_ : selectedPrefabKey_;
            dl->AddText(ImVec2(pMin.x + 3.0f, pMin.y + 3.0f),
                IM_COL32(255, 255, 255, 200), armLabel.c_str());
        }

        // Paint on LMB hold only when mouse is actually over the viewport image
        if (hoveredImage && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            auto cell = std::make_pair(gx, gy);
            if (cell != lastPaintCell_ && !isDoor) {
                lastPaintCell_ = cell;

                if (!selectedPrefabKey_.empty()) {
                    UpsertPrefabVariant_(gx, gy, selectedPrefabKey_);
                }
                else {
                    UpsertWallVariant_(gx, gy, selectedTextureKey_, true);
                }
            }
        }
    }

    // Reset stroke tracker when mouse is released anywhere (inside or outside grid)
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        lastPaintCell_ = { -1, -1 };
    }
}




/**
 * @brief Attempt to delete the currently selected entity or multi-selection.
 * 
 * Checks for Delete key press and handles removal of single or multi-selected 
 * variants and their corresponding game entities.
 */
void RoomEditor::TryDeleteSelected_() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput) return;
    if (!ImGui::IsKeyPressed(ImGuiKey_Delete, false)) return;

    // Multi-select: delete all selected cells
    if (!multiSelection_.empty()) {
        DeleteMultiSelection_();
        return;
    }

    // Fallback: single legacy selection
    if (!hasSelection_ || selectedEntity_ == INVALID_ENTITY) return;
    if (IsProtectedEntity_(selectedEntity_)) return;

    Entity e = selectedEntity_;

    // --- resolve which cell to edit ---
    int gx = selectedGx_;
    int gy = selectedGy_;

    if (gx < 0 || gy < 0) {
        FindGridPosForLiveEntity_(e, gx, gy);
    }

    Entity mapped = INVALID_ENTITY;
    if (gx >= 0 && gy >= 0) {
        auto it = liveEntities_.find(std::make_pair(gx, gy));
        if (it != liveEntities_.end()) mapped = it->second;

        RemoveVariantAt_(gx, gy);  // updates grid + variants, destroys mapped entity

        // Also write '0' directly for txt-loaded entities that were never in variants_
        const int row = (height - 1) - gy;
        if (row >= 0 && row < (int)grid.size() &&
            gx >= 0 && gx < (int)grid[row].size())
        {
            if (grid[row][gx] != '2')
                grid[row][gx] = '0';
        }
        dirty = true;
    }

    // Ensure runtime reflects delete
    if (destroyEntity_) {
        if (mapped != e) {
            RemoveFromLiveEntitiesIfOwned_(e);
            destroyEntity_(e);
        }
    }

    selectedEntity_ = INVALID_ENTITY;
    hasSelection_ = false;
    selectedGx_ = selectedGy_ = -1;
}




/**
 * @brief Draw the properties window for the currently selected entity.
 * 
 * Displays and allows editing of common properties like transform, 
 * MeshRenderer color, Collider size, and SpriteAnimator settings.
 */
void RoomEditor::DrawPropertiesWindow_() {
    if (!ImGui::Begin("Property Selector##Room")) { ImGui::End(); return; }

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        wantsViewportInput_ = true;

    if (ImGui::BeginTabBar("##PropertySelectorTabsRoom"))
    {
        if (ImGui::BeginTabItem("Properties"))
        {
            if (!hasSelection_ || selectedEntity_ == INVALID_ENTITY) {
                ImGui::TextDisabled("No selection.");
            }
            else {
        ImGui::Text("Entity ID: %d", (int)selectedEntity_);
        ImGui::Text("Grid Pos: (%d, %d)", selectedGx_, selectedGy_);
        ImGui::Separator();

        int idx = FindVariantIndexAt_(selectedGx_, selectedGy_);
        
        // If no variant exists yet, create one
        if (idx < 0 && (selectedGx_ >= 0 && selectedGx_ < width && selectedGy_ >= 0 && selectedGy_ < height)) {
            VariantPlacement vp{};
            vp.gx = selectedGx_;
            vp.gy = selectedGy_;
            
            if (getPrefabTag_) {
                std::string tag = getPrefabTag_(selectedEntity_);
                
                size_t clonePos = tag.find("(Clone)");
                if (clonePos != std::string::npos) {
                    tag = tag.substr(0, clonePos);
                }

                if (tag.empty() || tag == "Wall") {
                    // Check for a special wall in our grid
                    int row = GridRowFromWorldGy(selectedGy_, height);
                    if (row >= 0 && row < (int)grid.size() && selectedGx_ >= 0 && selectedGx_ < (int)grid[row].size()) {
                        if (grid[row][selectedGx_] == '1') {
                            if (tag.empty()) tag = "WallTile";
                        }
                    }
                }

                vp.type = tag.empty() ? "Unknown" : tag;
            } else {
                vp.type = "Unknown";
            }
            
            vp.name = "V_" + vp.type + "_" + std::to_string(vp.gx) + "_" + std::to_string(vp.gy);
            
            // Sync current runtime state into the new variant
            if (context_) {
                if (MeshRenderer* mr = context_->GetRenderer(selectedEntity_)) {
                    Vector3 mc = mr->GetColor();
                    vp.color[0] = mc.x; vp.color[1] = mc.y; vp.color[2] = mc.z; vp.color[3] = 1.0f;
                }
                if (Collider* col = context_->GetCollider(selectedEntity_)) {
                    vp.colliderSize[0] = col->size.x;
                    vp.colliderSize[1] = col->size.y;
                }
                if (SpriteAnimator* anim = context_->GetAnimator(selectedEntity_)) {
                    vp.animStart = anim->startFrame;
                    vp.animEnd = anim->endFrame;
                    vp.animSpeed = anim->speed;
                }
            }

            variants_.push_back(std::move(vp));
            idx = (int)variants_.size() - 1;
        }

        VariantPlacement* v = (idx >= 0) ? &variants_[idx] : nullptr;
        bool changed = false;

        if (v) {
            if (getPrefabTag_ && (v->type == "Unknown" || v->type == "WallTile" || v->type == "Wall")) {
                std::string tag = getPrefabTag_(selectedEntity_);
                size_t clonePos = tag.find("(Clone)");
                if (clonePos != std::string::npos) tag = tag.substr(0, clonePos);

                if (!tag.empty() && tag != "Wall" && tag != v->type) {
                    v->type = tag;
                    v->name = "V_" + v->type + "_" + std::to_string(v->gx) + "_" + std::to_string(v->gy);
                    changed = true;
                }
            }

            ImGui::LabelText("Type", "%s", v->type.c_str());
            ImGui::LabelText("Name", "%s", v->name.c_str());

            bool hasSig = v->hasSignature;
            if (ImGui::Checkbox("Persistent Signature", &hasSig)) {
                bool beforeHasSig = context_ ? context_->HasPersistentTag(selectedEntity_) : v->hasSignature;

                v->hasSignature = hasSig;
                changed = true;

                if (context_) {
                    if (hasSig) context_->AddPersistentTag(selectedEntity_);
                    else context_->RemovePersistentTag(selectedEntity_);

                    bool afterHasSig = context_->HasPersistentTag(selectedEntity_);
                    if (undoRedo_ && beforeHasSig != afterHasSig) {
                        undoRedo_->Push_PersistentTagToggle(
                            selectedEntity_,
                            beforeHasSig,
                            afterHasSig
                        );
                    }
                }
            }
        }
        else {
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "No persistent variant data.");
            if (context_) {
                bool hasSig = context_->HasPersistentTag(selectedEntity_);
                if (ImGui::Checkbox("Persistent Signature (Runtime)", &hasSig)) {
                    bool beforeHasSig = context_->HasPersistentTag(selectedEntity_);

                    if (hasSig) context_->AddPersistentTag(selectedEntity_);
                    else context_->RemovePersistentTag(selectedEntity_);

                    bool afterHasSig = context_->HasPersistentTag(selectedEntity_);
                    if (undoRedo_ && beforeHasSig != afterHasSig) {
                        undoRedo_->Push_PersistentTagToggle(
                            selectedEntity_,
                            beforeHasSig,
                            afterHasSig
                        );
                    }
                }
            }
        }

        // --- Transform (Read Only) ---
        if (context_) {
            if (const Transform* t = context_->TryGetTransform(selectedEntity_)) {
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    Vector2 pos = t->GetPosition();
                    float rot = t->GetRotation(); // Radians
                    Vector2 scale = t->GetScale();

                    float pos3[3] = { pos.x, pos.y, 0.0f };
                    ImGui::InputFloat3("Position", pos3, "%.2f", ImGuiInputTextFlags_ReadOnly);

                    float rotDeg = rot * 180.0f / 3.14159f;
                    ImGui::InputFloat("Rotation", &rotDeg, 0.0f, 0.0f, "%.2f", ImGuiInputTextFlags_ReadOnly);

                    float scale3[3] = { scale.x, scale.y, 1.0f };
                    ImGui::InputFloat3("Scale", scale3, "%.2f", ImGuiInputTextFlags_ReadOnly);
                }
            }
        }

        // --- Components ---
        if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (context_) {
                // MeshRenderer
                if (MeshRenderer* mr = context_->GetRenderer(selectedEntity_)) {
                    if (ImGui::TreeNode("MeshRenderer")) {
                        // Color
                        float c[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
                        if (v) {
                            c[0] = v->color[0]; c[1] = v->color[1]; c[2] = v->color[2]; c[3] = v->color[3];
                        }
                        else {
                            Vector3 mc = mr->GetColor();
                            c[0] = mc.x; c[1] = mc.y; c[2] = mc.z;
                        }

                        if (ImGui::ColorEdit4("Color", c)) {
                            mr->SetColor(Vector3(c[0], c[1], c[2])); // Update runtime
                            if (v) {
                                v->color[0] = c[0]; v->color[1] = c[1]; v->color[2] = c[2]; v->color[3] = c[3];
                                changed = true;
                            }
                        }
                        ImGui::TreePop();
                    }
                }

                // Collider
                if (Collider* col = context_->GetCollider(selectedEntity_)) {
                    if (ImGui::TreeNode("Collider")) {
                        // Size
                        float s[2] = { col->size.x, col->size.y };

                        if (ImGui::DragFloat2("Size", s, 0.1f)) {
                            col->size = Vector2(s[0], s[1]); // Update runtime
                            if (v) {
                                v->colliderSize[0] = s[0];
                                v->colliderSize[1] = s[1];
                                changed = true;
                            }
                        }

                        if (v && (v->colliderSize[0] != 0.0f || v->colliderSize[1] != 0.0f)) {
                            if (ImGui::Button("Reset Size to Default")) {
                                v->colliderSize[0] = 0.0f;
                                v->colliderSize[1] = 0.0f;
                                changed = true;
                            }
                        }
                        ImGui::TreePop();
                    }
                }

                // SpriteAnimator
                if (SpriteAnimator* anim = context_->GetAnimator(selectedEntity_)) {
                    if (ImGui::TreeNode("SpriteAnimator")) {
                        int cur = anim->cur;
                        int start = anim->startFrame;
                        int end = anim->endFrame;
                        float speed = anim->speed;

                        bool animChanged = false;
                        if (ImGui::DragInt("Start Frame", &start, 1, 0, 1000)) {
                            anim->startFrame = start;
                            animChanged = true;
                        }
                        if (ImGui::DragInt("End Frame", &end, 1, 0, 1000)) {
                            anim->endFrame = end;
                            animChanged = true;
                        }
                        if (ImGui::DragFloat("Speed", &speed, 0.1f, 0.0f, 60.0f)) {
                            anim->speed = speed;
                            animChanged = true;
                        }

                        if (animChanged) {
                            changed = true;
                            if (v) {
                                v->animStart = anim->startFrame;
                                v->animEnd = anim->endFrame;
                                v->animSpeed = anim->speed;
                            }
                        }

                        // Current frame is usually driven by update, but can be forced
                        if (ImGui::SliderInt("Current Frame", &cur, start, end)) {
                            anim->cur = cur;
                        }

                        if (v && (v->animStart != 0 || v->animEnd != 0 || v->animSpeed != 0.0f)) {
                            if (ImGui::Button("Reset Animator to Default")) {
                                v->animStart = 0;
                                v->animEnd = 0;
                                v->animSpeed = 0.0f;
                                changed = true;
                            }
                        }

                        ImGui::Text("Sheet: %s", anim->sheet ? "Assigned" : "None");
                        ImGui::TreePop();
                    }
                }

                // Custom Game Components
                if (customPropertiesDraw_) {
                    customPropertiesDraw_(selectedEntity_, v, changed);
                }
            }
        }

        // --- Variant Properties ---
        if (v && ImGui::CollapsingHeader("Variant Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
            // --- Solid Checkbox ---
            bool isWall = (v->type == "WallTile");

            bool solid = v->solid;
            if (ImGui::Checkbox("Solid", &solid)) {
                v->solid = solid;
                changed = true;

                // Sync with grid char 
                if (selectedGx_ >= 0 && selectedGx_ < width && selectedGy_ >= 0 && selectedGy_ < height) {
                    int row = GridRowFromWorldGy(selectedGy_, height);
                    if (row >= 0 && row < (int)grid.size() && selectedGx_ < (int)grid[row].size()) {
                        // Don't overwrite door '2'
                        if (grid[row][selectedGx_] != '2') {
                            grid[row][selectedGx_] = solid ? '1' : '0';
                            dirty = true;
                        }
                    }
                }
            }

            // --- Signature Checkbox (Prefab only) ---
            if (!isWall) {
                if (ImGui::Checkbox("Signature", &v->hasSignature)) {
                    changed = true;
                }
            }

            // --- Texture Key (Wall only) ---
            if (isWall) {
                ImGui::LabelText("Texture", "%s", v->textureKey.c_str());
            }
        }

        if (changed) {
            dirtyVariants_ = true;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save Room Data", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            std::string stem = roomIdCached;
            if (getActiveRoom_) {
                std::filesystem::path p(getActiveRoom_());
                stem = p.stem().string();
            }

            if (Variant::SaveRoomData(roomTxtPath, roomVariantPath, stem, tileSize_, grid, variants_)) {
                dirty = false;
                dirtyVariants_ = false;
            }

            if (context_ && !roomLightPath.empty()) {
                const std::vector<LightRecord> records = CollectLightRecords_(*context_, getPrefabTag_);
                if (LightEffects::SaveFile(roomLightPath, stem, records)) {
                    dirtyLights_ = false;
                }
            }
        }

        if (dirty || dirtyVariants_ || dirtyLights_) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Unsaved changes (Grid/Variants/Lights)");
        }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Lighting effects"))
        {
            if (!hasSelection_ || selectedEntity_ == INVALID_ENTITY || !context_) {
                ImGui::TextDisabled("No selection.");
            }
            else {
                if (LightEffects::DrawLightEffectsTab(*context_, selectedEntity_, undoRedo_)) {
                    dirtyLights_ = true;
                }
            }

            if (context_ && !roomLightPath.empty()) {
                if (ImGui::Button("Save Light Data", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    std::string stem = roomIdCached;
                    if (getActiveRoom_) {
                        std::filesystem::path p(getActiveRoom_());
                        stem = p.stem().string();
                    }

                    const std::vector<LightRecord> records = CollectLightRecords_(*context_, getPrefabTag_);
                    if (LightEffects::SaveFile(roomLightPath, stem, records)) {
                        dirtyLights_ = false;
                    }
                }
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

/**
 * @brief Delete all variants in the multi-selection.
 * 
 * Iterates through all selected cells and removes them from the 
 * variants list, destroying their game entities and updating the grid.
 */
void RoomEditor::DeleteMultiSelection_() {
    for (auto& [gx, gy] : multiSelection_) {
        // Check for protected entities (doors/player) before doing anything
        auto it = liveEntities_.find(std::make_pair(gx, gy));
        Entity mapped = (it != liveEntities_.end()) ? it->second : INVALID_ENTITY;

        if (mapped != INVALID_ENTITY && IsProtectedEntity_(mapped))
            continue;

        // RemoveVariantAt_ handles: destroying the live ECS entity,
        // erasing from liveEntities_, removing from variants_, and
        // setting grid[row][gx] = '0' for cells it knows about.
        RemoveVariantAt_(gx, gy);

        // CRITICAL: also write '0' directly into the grid for cells that were
        // loaded from the txt file and were never in variants_ / liveEntities_.
        // Without this, SaveRoomData re-syncs surviving WallTile variants back
        // into the grid as '1', but has no way to know this cell was deleted
        // since it was never tracked as a variant to begin with.
        const int row = (height - 1) - gy;
        if (row >= 0 && row < (int)grid.size() &&
            gx >= 0 && gx < (int)grid[row].size())
        {
            if (grid[row][gx] != '2') // never overwrite door marker
                grid[row][gx] = '0';
        }

        dirty = true;
    }

    multiSelection_.clear();
    selectedEntity_ = INVALID_ENTITY;
    hasSelection_ = false;
    selectedGx_ = selectedGy_ = -1;
}




/**
 * @brief Find the grid cell position for a tracked live entity.
 * @param e Entity to locate.
 * @param outGx Output grid x coordinate.
 * @param outGy Output grid y coordinate.
 * @return true if the entity exists in liveEntities_ and a grid position was found.
 */
bool RoomEditor::FindGridPosForLiveEntity_(Entity e, int& outGx, int& outGy) const {
    for (const auto& kv : liveEntities_) {
        if (kv.second == e) {
            outGx = kv.first.first;
            outGy = kv.first.second;
            return true;
        }
    }
    return false;
}

#endif
