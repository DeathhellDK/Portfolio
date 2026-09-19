#pragma once 
/**
* @file		  RoomEditor.h
* @author     Woh Kye Le
* @co-author  Lim Zhi Jie, Jethro Sung
* @email      w.kyele, zhijie.lim, sung.h
* @date		  2026 - 01 - 22
*
* @brief ImGui-based room editor for creating and editing grid-based room layouts
*
* The RoomEditor provides a user interface for designing room layouts using
* a grid-based tile system. It integrates with the game's room management system
* through callback functions and supports painting, validation, saving, and
* applying room layouts to the game world.
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

#include "Math/vect2.h"
#include "imgui.h" 
#include "Editor/PlayStopManager.h"
#include "Editor/EditorShared.h" 
#include "Core/component.h"
#include "Core/componentcontext.h"

#include <string>
#include <vector>
#include <functional>
#include <filesystem>

class Camera2D;
class UndoRedoManager;

// Hash function for the map key
struct PairHash {
    size_t operator()(const std::pair<int, int>& p) const noexcept {
        size_t h1 = std::hash<int>{}(p.first);
        size_t h2 = std::hash<int>{}(p.second);
        return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
    }
};



/**
 * @class RoomEditor
 * @brief Grid-based room editor with ImGui interface for tile-based level design
 *
 * This class provides a comprehensive editor for creating and modifying
 * room layouts using a grid of character-based tiles. It features painting
 * tools, validation, file I/O, and integration with the game's room system.
 */
class RoomEditor {
public:

    // callbacks provided by GAME layer
    using GetActiveRoomFn = std::function<std::string()>;
    using ResolveRoomTxtPathFn = std::function<std::string(const std::string& activeRoom)>;
    using ResolveRoomVariantPathFn = std::function<std::string(const std::string& activeRoom)>;
    using ResolveRoomLightPathFn = std::function<std::string(const std::string& activeRoom)>;
    using GetActiveCameraFn = std::function<Camera2D* ()>;
    using LoadGridFn = std::function<bool(const std::string& txtPath, std::vector<std::string>& outGrid, int& outW, int& outH)>;
    using ApplyRoomFn = std::function<void(const std::vector<std::string>& grid, int width, int height, const std::string& roomId)>;
    using GetEntitiesInRectFn = std::function<std::vector<Entity>(const Vector2& worldPos, const Vector2& worldSize)>;
    using GetPrefabTagFn = std::function<std::string(Entity)>;
    using CreateWallEntityFn = std::function<Entity(int gx, int gy, const std::string& textureKey, bool solid, float tileSize)>;
    using CreatePrefabEntityFn = std::function<Entity(int gx, int gy, const std::string& prefabName, float tileSize)>;
    using DestroyEntityFn = std::function<void(Entity)>;
    using RequestCameraLockFn = std::function<void(bool lock)>;
    using GetTextureIDFn = std::function<unsigned int(const std::string& textureKey)>;
    using GetPrefabTextureFn = std::function<unsigned int(const std::string& prefabName)>;
    using CustomPropertiesDrawFn = std::function<void(Entity, VariantPlacement*, bool&)>;

    // Play/Stop integration 
    using GetPlayStateFn = std::function<EditorPlayControlsState()>;

    // Assets browser
    enum class AssetBucket { Atlases, Audio, Fonts, Textures };



    // ------------------------------------ Lifecycle (main API) -------------------------------------
    // ---------------------
    /**
    * @brief Initialize the room editor
    *
    * Sets up internal state and loads available room files. Must be called
    * before any other editor operations.
    */
    void Init();

    /**
     * @brief Sets the component context for the editor.
     * @param ctx Reference to the component context.
     */
    void SetContext(IComponentContext& ctx) { context_ = &ctx; }

    /**
     * @brief Update the editor state
     * @param dt Delta time in seconds since last update
     *
     * Handles state updates, dirty flag management, and room change detection.
     */
    void Update(float dt);

    /**
     * @brief Draw the editor UI
     *
     * Renders the ImGui interface for the room editor including tools,
     * grid view, and controls. Must be called within an ImGui frame.
     */
    void Draw();

    /**
    * @brief Shutdown the editor
    *
    * Cleans up resources and resets internal state. Should be called
    * before the editor is destroyed.
    */
    void Shutdown();
    // ---------------------
    // -----------------------------------------------------------------------------------------------


    // -------------------------------------- Callback Setters ---------------------------------------
    // ---------------------
    /**
     * @brief Set the callback for retrieving the active room
     * @param fn Function that returns the current active room ID
     *
     * This callback is used to query which room is currently active
     * in the game for editing purposes.
     */
    void SetGetActiveRoom(GetActiveRoomFn fn);

    /**
     * @brief Set the callback for resolving room ID to file path
     * @param fn Function that converts a room ID to its text file path
     *
     * This callback maps room identifiers to their corresponding
     * file system paths for loading and saving operations.
     */
    void SetResolveRoomTxtPath(ResolveRoomTxtPathFn fn);

    /**
     * @brief Set the callback for resolving room ID to variant file path
     * @param fn Function that converts a room ID to its variant JSON file path
     *
     * This callback maps room identifiers to their corresponding
     * variant data file paths for loading and saving variant placements.
     */
    void SetResolveRoomVariantPath(ResolveRoomVariantPathFn fn);
    /**
     * @brief Set the callback for resolving room ID to light sidecar file path.
     * @param fn Function mapping room/scene ID to the light JSON path.
     *
     * When provided, the RoomEditor will:
     * - Load LightRecord data after a room is loaded
     * - Save LightRecord data when the user saves room data / light data
     */
    void SetResolveRoomLightPath(ResolveRoomLightPathFn fn);

    /**
     * @brief Set the callback for loading grid data from file
     * @param fn Function that loads grid dimensions and tile data from a file
     *
     * This callback handles the file parsing logic to load room layouts
     * from disk into the editor's grid representation.
     */
    void SetLoadGrid(LoadGridFn fn);

    /**
     * @brief Set the callback for applying room changes
     * @param fn Function that applies room data to the game world
     *
     * This callback is invoked when the user applies changes from
     * the editor to update the game's room system.
     */
    void SetApplyCallback(ApplyRoomFn fn);

    /**
     * @brief Set the callback for drawing custom entity properties.
     * @param fn Function that draws custom ImGui properties for an entity.
     */
    void SetCustomPropertiesDraw(CustomPropertiesDrawFn fn);

    /**
     * @brief Set the play controls callbacks and state getter
     * @param getState Function that returns the current play/stop state
     * @param cbs Callbacks for play/stop control operations
     *
     * Configures integration with the play/stop controls system for
     * managing game runtime state from within the editor.
     */
    void SetPlayControls(GetPlayStateFn getState, EditorPlayControlsCallbacks cbs);

    /**
     * @brief Set the callback for creating wall entities
     * @param fn Function that creates a wall entity at specified grid coordinates
     *
     * This callback is used to instantiate wall entities in the game world
     * during variant editing operations.
     */
    void SetCreateWallEntityCallback(CreateWallEntityFn fn) { createWallEntity_ = std::move(fn); }

    /**
     * @brief Set the callback for creating prefab entities.
     * @param fn Function that creates a prefab entity at specified grid coordinates.
     */
    void SetCreatePrefabEntityCallback(CreatePrefabEntityFn fn) { createPrefabEntity_ = std::move(fn); }

    /**
     * @brief Set the list of available prefabs for placement.
     * @param prefabs List of prefab names.
     */
    void SetAvailablePrefabs(std::vector<std::string> prefabs) { availablePrefabs_ = std::move(prefabs); }

    /**
     * @brief Set the callback for destroying entities.
     * @param fn Function that destroys a specified entity.
     *
     * This callback is used to remove entities from the game world
     * during variant editing operations.
     */
    void SetDestroyEntityCallback(DestroyEntityFn fn) { destroyEntity_ = std::move(fn); }

    /**
     * @brief Set the scene texture to display in the viewport.
     * @param texId OpenGL texture ID of the scene.
     * @param w Width of the scene texture.
     * @param h Height of the scene texture.
     */
    void SetSceneTexture(unsigned int texId, int w, int h);

    /**
     * @brief Set the callback for retrieving the active camera.
     * @param fn Function that returns the current active camera.
     *
     * This callback provides access to the camera for viewport
     * navigation and coordinate transformation operations.
     */
    void SetGetActiveCamera(GetActiveCameraFn fn) { getActiveCamera_ = std::move(fn); }

    /**
     * @brief Set the callback for retrieving entities within a rectangle.
     * @param fn Function that returns entities within a specified world-space rectangle.
     *
     * This callback is used for entity selection operations within the
     * editor viewport.
     */
    void SetGetEntitiesInRect(GetEntitiesInRectFn fn) { getEntitiesInRect_ = std::move(fn); }

    /**
     * @brief Set the callback for retrieving prefab tags
     * @param fn Function that returns the prefab tag for a given entity
     *
     * This callback identifies entities by their prefab type for
     * selection and manipulation operations.
     */
    void SetGetPrefabTag(GetPrefabTagFn fn) { getPrefabTag_ = std::move(fn); }

    /**
     * @brief Set the callback for requesting camera lock state
     * @param fn Function that requests camera input lock/unlock
     *
     * This callback controls whether the camera should be locked for
     * navigation or available for game input.
     */
    void SetRequestCameraLock(RequestCameraLockFn fn) { requestCameraLock_ = std::move(fn); }

    /**
     * @brief Set the callback for retrieving texture IDs
     * @param fn Function that returns the OpenGL texture ID for a given key
     */
    void SetGetTextureID(GetTextureIDFn fn) { getTextureID_ = std::move(fn); }

    /**
     * @brief Set the callback for retrieving prefab texture IDs
     * @param fn Function that returns the OpenGL texture ID for a given prefab name
     */
    void SetGetPrefabTexture(GetPrefabTextureFn fn) { getPrefabTexture_ = std::move(fn); }
    // ---------------------
    // -----------------------------------------------------------------------------------------------


    // ------------------------------------------ Editor state ---------------------------------------
    // ---------------------
    /**
     * @brief Check if the editor is currently editing a room.
     * @return true if a valid room context exists
     */
    bool IsInRoomContext() const;

    /**
     * @brief Get world mouse position based on editor viewport.
     */
    Vector2 GetWorldMousePosition() const;

    /**
     * @brief Check if mouse is hovering over the viewport image.
     */
    bool IsMouseInViewport() const;

    /**
     * @brief Check if the editor is active and visible.
     * @return true if editor is both active and visible
     */
    bool IsActive() const;

    /**
     * @brief Check if the editor UI should be visible.
     * @return true if editor UI should be shown
     */
    bool IsVisible() const { return visible_; }

    /**
     * @brief Set the visibility state of the editor UI.
     * @param v true to show the editor, false to hide it
     */
    void SetVisible(bool v) { visible_ = v; }

    /**
     * @brief Toggle the visibility state of the editor UI.
     */
    void ToggleVisible() { visible_ = !visible_; }

    /**
    * @brief Detects a Resume transition (paused -> running) and restores default camera/editor navigation state.
    */
    void HandleResumeReset_();

    /**
     * @brief Handles the case where the editor is not in a valid room context or not visible.
     * @param nowContext True if the current active room is an editable room context.
     * @return true if the caller should early-return from Update(), false otherwise.
     */
    bool HandleNotInContextOrHidden_(bool nowContext);
    // ---------------------
    // ----------------------------------------------------------------------------------------------


    // --------------------------------- docking / viewport layout ----------------------------------
    // ---------------------
    /**
     * @brief Begin the docking host window for editor layout.
     */
    bool BeginDockHost_();

    /**
     * @brief End the docking host window.
     */
    void EndDockHost_();

    /**
     * @brief Build or rebuild the dock layout if needed.
     */
    void BuildDockLayoutIfNeeded_();

    /**
     * @brief Draw the main viewport window.
     */
    void DrawViewportWindow_();

    /**
     * @brief Updates dock rebuild request state based on viewport work area changes.
     */
    void UpdateDockRebuildRequests_();

    /**
     * @brief Updates cached context/visibility flags and triggers dock rebuild when state changes.
     * @param nowContext True if the current active room is an editable room context.
     */
    void UpdateContextAndVisibility_(bool nowContext);

    /**
    * @brief Check if editor requests dock layout rebuild
    * @return true if editor wants to rebuild dock layout
    *
    * Used by the UI system to know when to rearrange docking spaces
    * for optimal editor layout.
    */
    bool WantsDockLayoutRebuild() const { return requestDockRebuild_; }

    /**
     * @brief Clear the dock layout rebuild request flag
     *
     * Should be called after the dock layout has been rebuilt
     * to reset the request flag.
     */
    void ClearDockLayoutRebuildFlag() { requestDockRebuild_ = false; }

    /**
     * @brief Check if editor wants to capture viewport input
     * @return true if editor should receive viewport mouse/keyboard input
     *
     * Used to determine input routing between editor and game viewport.
     */
    bool WantsViewportInput() const { return wantsViewportInput_; }
    // ---------------------
    // ----------------------------------------------------------------------------------------------

    void SetUndoRedoManager(UndoRedoManager* mgr) { undoRedo_ = mgr; }

private:

    // -------------------------------- Drawing various IMGUi panel ---------------------------------
    // ---------------------
    /**
     * @brief Draw the variant palette window for selecting wall textures/prefabs.
     */
    void DrawVariantPaletteWindow_();

    /**
     * @brief Draw the list of placed variants in the room.
     */
    void DrawVariantListWindow_();

    /**
     * @brief Draw the play/stop controls window.
     */
    void DrawPlayControlsWindow_();

    /**
     * @brief Draw the assets browser window.
     */
    void DrawAssetsWindow_();

    /**
     * @brief Draw the properties window for the selected entity.
     */
    void DrawPropertiesWindow_();

    /**
     * @brief Refresh the list of assets in the current bucket.
     */
    void RefreshAssets_();
    // ---------------------
    // -----------------------------------------------------------------------------------------------


    // ---------------------------------- variants editing helpers -----------------------------------
    // ---------------------
    /**
     * @brief Find the index of a variant at the specified grid coordinates.
     * @param gx Grid X coordinate.
     * @param gy Grid Y coordinate.
     * @return Index in the variants list, or -1 if not found.
     */
    int  FindVariantIndexAt_(int gx, int gy) const;

    /**
     * @brief Insert or update a wall variant at the specified grid coordinates.
     * @param gx Grid X coordinate.
     * @param gy Grid Y coordinate.
     * @param textureKey Texture key for the wall.
     * @param solid Whether the wall is solid.
     */
    void UpsertWallVariant_(int gx, int gy, const std::string& textureKey, bool solid);

    /**
     * @brief Insert or update a prefab variant at the specified grid coordinates.
     * @param gx Grid X coordinate.
     * @param gy Grid Y coordinate.
     * @param prefabName Name of the prefab.
     */
    void UpsertPrefabVariant_(int gx, int gy, const std::string& prefabName);

    /**
     * @brief Remove a variant at the specified grid coordinates.
     * @param gx Grid X coordinate.
     * @param gy Grid Y coordinate.
     */
    void RemoveVariantAt_(int gx, int gy);

    /**
     * @brief Convert mouse screen coordinates to grid coordinates.
     * @param outGX Output grid X coordinate.
     * @param outGY Output grid Y coordinate.
     * @return true if mouse is within the grid.
     */
    bool MouseToGrid_(int& outGX, int& outGY) const;

    /**
     * @brief Convert mouse screen coordinates to world coordinates.
     * @param outWorld Output world coordinates.
     * @return true if mouse is within the viewport.
     */
    bool MouseToWorld_(Vector2& outWorld) const;

    /**
     * @brief Destroy all live entities tracked by the editor.
     */
    void ClearAllLiveEntities_();

    /**
     * @brief Scan the scene and populate liveEntities_ with existing wall entities.
     */
    void PopulateLiveEntitiesFromScene_();
    // ---------------------
    // -----------------------------------------------------------------------------------------------



    // ------------------------------------- Navigation helpers --------------------------------------
    // ---------------------
    /**
     * @brief Process navigation camera input (pan/zoom) for the viewport.
     * @param cam Pointer to the active camera used for world/screen conversions.
     * @param hoveredImage True if the mouse is currently hovering the viewport image rect.
     * @param focusedWin True if the viewport window is focused.
     */
    void HandleNavigationInput_(Camera2D* cam, bool hoveredImage, bool focusedWin);

    /**
     * @brief Resets navigation state flags to default.
     */
    void ResetNavigationState_();

    /**
     * @brief Forces the camera back into follow-player mode and resets zoom to default.
     */
    void ForceFollowAndResetZoom_();

    /**
     * @brief Updates whether the camera should follow the player based on current navigation mode.
     */
    void UpdateFollowMode_();

    /**
     * @brief Enforces invariants between navigation mode and navigation lock.
     */
    void EnforceNavModeRules_();
    // ---------------------
    // -----------------------------------------------------------------------------------------------


    // ----------------------------- Entities Selection & management ---------------------------------
    // ---------------------
    /**
     * @brief Check whether an entity is protected from editor deletion.
     * @param e Entity to test.
     * @return true if the entity should not be deleted by editor operations.
     */
    bool IsProtectedEntity_(Entity e) const;

    /**
     * @brief Attempt to select an entity under the mouse cursor in the viewport.
     */
    void TrySelectEntityUnderMouse_();

    /**
     * @brief Attempt to delete the currently selected entity.
     */
    void TryDeleteSelected_();

    /**
     * @brief Delete all cells in the multi-selection.
     */
    void DeleteMultiSelection_();

    /**
     * @brief Handle paintbrush painting in the viewport (called each frame).
     * @param dl ImGui draw list to use for rendering overlays.
     * @param cam Active camera for world/screen transformations.
     * @param hoveredImage Whether the viewport image is currently hovered.
     */
    void DrawPaintbrushTool_(ImDrawList* dl, Camera2D* cam, bool hoveredImage);

    /**
     * @brief Removes an entity from the liveEntities_ mapping if it is owned/tracked by the editor.
     * @param e Entity to remove from the internal ownership map.
     */
    void RemoveFromLiveEntitiesIfOwned_(Entity e);

    /**
     * @brief Finds the grid cell position for a tracked live entity.
     * @param e Entity to locate.
     * @param outGx Output grid x coordinate.
     * @param outGy Output grid y coordinate.
     * @return true if the entity exists in liveEntities_ and a grid position was found.
     */
    bool FindGridPosForLiveEntity_(Entity e, int& outGx, int& outGy) const;
    // ---------------------
    // -----------------------------------------------------------------------------------------------



    // ------------------------------------- Room management -----------------------------------------
    // ---------------------
    /**
     * @brief Handle room change detection and loading
     *
     * Called internally when the active room changes to load
     * the corresponding room data into the editor.
     */
    void OnRoomChanged();

    /**
     * @brief Load available room files from disk
     *
     * Scans the rooms directory and populates the room selection
     * list with available room files.
     */
    void LoadRoomFiles();

    /**
     * @brief Validate the current room layout
     * @param outMsg Output parameter for validation message
     * @return true if room layout is valid
     * @return false if room layout has errors
     *
     * Performs validation checks on the current room grid and
     * provides an error/warning message if invalid.
     */
    bool Validate(std::string& outMsg) const;

    /**
     * @brief Apply current room changes to the game
     *
     * Calls the registered apply callback to push editor changes
     * to the game's room system.
     */
    void Apply();

    /**
     * @brief Save the current room to disk
     *
     * Serializes the current room grid to a text file in the
     * rooms directory.
     */
    void Save();
    // ---------------------
    // -----------------------------------------------------------------------------------------------


    // ---------------------------------- Assets Browser helper  -------------------------------------
    // ---------------------
    /**
     * @brief Checks whether a filesystem path is within the editor's assets root folder.
     * @param p Path to validate.
     * @return true if the canonical path lies under assetsRoot_, false otherwise.
     */
    bool IsInsideAssetsRoot_(const std::filesystem::path& p) const;

    /**
     * @brief Import (copy) an external file into the selected assets folder.
     * @param src Source path (external file).
     * @param dst Destination path under assetsRoot_.
     * @return true if the import succeeded, false otherwise.
     */
    bool ImportFile_(const std::filesystem::path& src, const std::filesystem::path& dst);

    /**
     * @brief Deletes a file from disk within the assets directory.
     * @param p File path to delete.
     * @return true if deletion succeeded, false otherwise.
     */
    bool DeleteFile_(const std::filesystem::path& p);

    /**
     * @brief Resolves the directory path for a given asset bucket.
     * @param b Asset bucket type (atlases/audio/fonts/textures).
     * @return Filesystem path corresponding to the chosen bucket.
     */
    std::filesystem::path BucketPath_(AssetBucket b) const;

    /**
     * @brief Resolves the assets root directory at runtime.
     * @return Canonical filesystem path to the assets directory.
     */
    static std::filesystem::path ResolveAssetsRoot_();
    // ---------------------
    // -----------------------------------------------------------------------------------------------

private:
    // ----- injected callbacks -----
    GetActiveRoomFn          getActiveRoom_;          /**< Callback to get the active room name. */
    ResolveRoomTxtPathFn     resolveRoomTxtPath_;     /**< Callback to resolve room ID to TXT path. */
    ResolveRoomVariantPathFn resolveRoomVariantPath_; /**< Callback to resolve room ID to variant JSON path. */
    ResolveRoomLightPathFn   resolveRoomLightPath_;   /**< Callback to resolve room ID to light JSON path. */
    LoadGridFn               loadGrid_;               /**< Callback to load grid data from file. */
    ApplyRoomFn              onApply_;                /**< Callback to apply room changes to the game. */
    GetPlayStateFn           playStateFn_;            /**< Callback to get the current editor play state. */
    GetActiveCameraFn        getActiveCamera_;        /**< Callback to retrieve the active camera. */
    GetEntitiesInRectFn      getEntitiesInRect_;      /**< Callback to retrieve entities within a world-space rectangle. */
    GetPrefabTagFn           getPrefabTag_;           /**< Callback to retrieve the prefab tag of an entity. */
    CreateWallEntityFn       createWallEntity_;       /**< Callback to create a wall entity. */
    CreatePrefabEntityFn     createPrefabEntity_;     /**< Callback to create a prefab entity. */
    DestroyEntityFn          destroyEntity_;          /**< Callback to destroy an entity. */
    GetTextureIDFn           getTextureID_;           /**< Callback to retrieve the OpenGL texture ID for a given key. */
    GetPrefabTextureFn       getPrefabTexture_;       /**< Callback to retrieve the texture ID for a given prefab. */
    CustomPropertiesDrawFn   customPropertiesDraw_;   /**< Callback for drawing custom entity properties. */

    IComponentContext* context_ = nullptr;            /**< Pointer to the component context. */
    UndoRedoManager* undoRedo_ = nullptr;

    std::string roomVariantPath;                      /**< Cached path to the current room's variant file. */
    std::string roomLightPath;                        /**< Cached path to the current room's light sidecar JSON file. */

    std::vector<VariantPlacement> variants_;          /**< List of current variant placements in the room. */
    bool dirtyVariants_ = false;                      /**< Flag indicating variant metadata has changed. */
    bool dirtyLights_ = false;                        /**< Flag indicating light data has unsaved changes. */
    std::string selectedTextureKey_ = "wall_B&T";     /**< Currently selected wall texture key for painting. */
    std::string selectedPrefabKey_;                   /**< Currently selected prefab name for painting. */
    std::vector<std::string> availablePrefabs_;       /**< List of all available prefabs for placement. */

    // ----- Paintbrush -----
    bool paintbrushMode_ = false;                     /**< Whether paintbrush mode is currently active. */
    std::pair<int, int> lastPaintCell_ = { -1,-1 };   /**< Coordinates of the last cell painted in the current stroke. */

    // Track created entities for cleanup    
    std::unordered_map<std::pair<int, int>, Entity, PairHash> liveEntities_; /**< Mapping from grid coordinates to tracked game entities. */

    EditorPlayControlsCallbacks playCbs_{};           /**< Callbacks for play/stop/pause operations. */

    // cached state
    std::string roomIdCached;                         /**< Cached ID of the active room. */
    std::string roomTxtPath;                          /**< Cached path to the room's layout text file. */

    std::vector<std::string> grid;                    /**< The current room's tile grid data. */
    int width = 0;                                    /**< Width of the room grid in cells. */
    int height = 0;                                   /**< Height of the room grid in cells. */
    int roomEditorGridH_ = 0;                         /**< Cached height for coordinate transformations. */

    bool loaded = false;                              /**< Whether a room is currently loaded in the editor. */
    bool dirty = false;                               /**< Whether the room layout has unsaved changes. */
    char paint = '1';                                 /**< Currently selected tile character for legacy painting. */

    // ----- editor state -----
    bool prevActive_ = false;                         /**< Previous active state for change detection. */
    bool prevVisible_ = false;                        /**< Previous visibility state for change detection. */
    bool visible_ = false;                            /**< Whether the editor UI is currently visible. */
    bool wantsViewportInput_ = false;                 /**< Whether the editor wants to capture viewport input. */

    // docking / viewport layout 
    bool requestDockRebuild_ = false;                 /**< Whether a dock layout rebuild has been requested. */
    bool firstDockBuild_ = true;                      /**< Whether the first dock layout build is still pending. */
    ImGuiID dockspaceId_ = 0;                         /**< ID of the main dockspace. */
    ImGuiID centerDockId_ = 0;                        /**< ID of the central dock node for the viewport. */

    // viewport texture
    unsigned int sceneTextureId_ = 0;                 /**< OpenGL ID of the scene texture to render. */
    int sceneTexW_ = 0;                               /**< Width of the scene texture. */
    int sceneTexH_ = 0;                               /**< Height of the scene texture. */

    // viewport image rect (for input mapping)
    ImVec2 viewportImagePos_{ 0,0 };                  /**< Top-left position of the viewport image in ImGui coordinates. */
    ImVec2 viewportImageSize_{ 0,0 };                 /**< Rendered size of the viewport image. */

    Vector2 workToFBScale_{ 1,1 };                    /**< Scale factor from ImGui work area to framebuffer. */

    int selectedGx_ = -1;                             /**< Grid X coordinate of the primary selection. */
    int selectedGy_ = -1;                             /**< Grid Y coordinate of the primary selection. */

    // tile size
    float tileSize_ = 100.0f;                         /**< Size of each grid tile in world units. */

    // ----- Navigation camera ----- 
    RequestCameraLockFn requestCameraLock_;           /**< Callback to request camera input lock. */
    bool  navMode_ = false;                           /**< Whether editor navigation mode is active. */
    bool  navLock_ = false;                           /**< Whether navigation is currently locked. */
    bool  lastWantFollow_ = true;                     /**< Previous camera follow state. */
    bool  navPanning_ = false;                        /**< Whether the camera is currently being panned. */
    Camera2D* activeCamera_ = nullptr;                /**< Cached pointer to the active camera. */

    // selection and deletion 
    Entity selectedEntity_ = INVALID_ENTITY;          /**< Currently selected entity in the viewport. */
    bool   hasSelection_ = false;                     /**< Whether an entity is currently selected. */

    // multi-select: set of grid cells currently selected
    std::vector<std::pair<int, int>> multiSelection_; /**< List of currently selected grid cells. */


    // ----- Assets browser -----
    std::filesystem::path assetsRoot_;                /**< Root directory for assets managed by the editor. */

    AssetBucket assetBucket_ = AssetBucket::Textures; /**< Currently selected asset bucket. */

    std::vector<std::filesystem::directory_entry> assetEntries_; /**< List of files in the current asset bucket. */
    int selectedAssetIdx_ = -1;                       /**< Index of the currently selected asset entry. */

    char importSrcPath_[512] = { 0 };                 /**< Buffer for the source path of a file to import. */
    char importNewName_[256] = { 0 };                 /**< Buffer for the optional new name during import. */
    std::string assetStatus_;                         /**< Status message to display in the assets browser. */




};

#endif
