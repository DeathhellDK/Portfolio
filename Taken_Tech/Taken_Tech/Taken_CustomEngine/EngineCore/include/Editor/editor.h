#pragma once
/**
 * @file      editor.h
 * @author    Jethro Sung
 * @email     sung.h, w.kyele, sweeyongdillon.sng, jianlin.low, t.weiliangterril
 * @co-author Woh Kye Le, Sng Swee Yong Dillon, Low JianLin, Tan Wei Liang Terril
 * @date      2025-11-7
 * @brief Declares the EditorOverlay class.
 *
 * EditorOverlay is an in-game editor layer built on ImGui.
 * It lets designers / programmers:
 *  - view performance stats
 *  - inspect active entities
 *  - edit component properties (Transform, Collider, etc.)
 *  - spawn / delete entities
 *  - drag entities in the viewport
 *  - change and apply textures
 *  - switch / save levels
 *  - Play/Stop toggling for runtime simulation
 *
 * The editor interacts with the ECS only through IComponentContext, so it does
 * not depend directly on GameApp. 
 */
#ifndef ENABLE_EDITOR
#define ENABLE_EDITOR 0
#endif

#if ENABLE_EDITOR

#include "imgui.h"
#include "Core/componentcontext.h"
#include "Graphics/camera2d.h"
#include "Math/vect2.h"
#include <vector>
#include <algorithm>
#include <functional>
#include <string>
#include "Graphics/mesh2D.h"
#include "ImGuizmo.h"
#include "Input/DebugConsole.hpp"
#include "Input/message_system.h"
#include "Editor/EditorShared.h"
#include <unordered_map>

struct SpriteSheet;

/**
 * @struct SpawnRequest
 * @brief Describes a pending entity spawn request issued by the editor.
 *
 * This structure is populated by editor UI actions such as the Spawner panel
 * or drag-and-drop placement tools. The game layer reads this request after
 * the editor frame and creates the requested entity in the scene.
 *
 * Depending on the request type, the structure may represent:
 * - a generic textured object
 * - a player entity
 * - a prefab instance
 *
 * The request also stores the desired spawn transform and any associated
 * texture or prefab selection metadata.
 */
struct SpawnRequest {

    /**
    * @enum Type
    * @brief Identifies what kind of object the editor wants to spawn.
    */
    enum class Type { None, Object, Player, Prefab };
    Type type = Type::None; //type of spawn req
    Vector2 pos{ 0.f, 0.f };
    Vector2 scale{ 64.f, 64.f };
    float rotation = 0.f;
    int texChoice = 0;//idx of selected tex option
    //tex and prefab name assoc with spawn
    std::string textureName;
    std::string prefabName;
};

/**
 * @struct RemovedMeshRendererState
 * @brief Stores a removed MeshRenderer component state for later restoration.
 *
 * This structure is used by the editor when a MeshRenderer component is
 * temporarily removed through the inspector or undo/redo workflow. It keeps
 * enough renderer information to restore the component later without losing
 * the previous visual setup.
 */
struct RemovedMeshRendererState
{
    bool valid = false;// true if cached contains valid rend data
    Vector3 color{ 1.f, 1.f, 1.f };
    unsigned texture = 0;
    Mesh2D* mesh = nullptr;
};

/**
 * @struct RemovedAnimatorState
 * @brief Stores a removed SpriteAnimator component state for restoration.
 *
 * Used when the editor disables or removes a SpriteAnimator component but
 * needs to preserve its previous playback state so it can be restored later.
 */
struct RemovedAnimatorState
{
    bool valid = false;
    int cur = 0;//curr frama t time of removal
    int startFrame = 0;
    int endFrame = 0;
    float speed = 0.0f;
    const SpriteSheet* sheet = nullptr;
};

/**
 * @struct RemovedColliderState
 * @brief Stores a removed Collider component state for restoration.
 *
 * This cache is used so the editor can re-add a collider component with its
 * previous collision size and trigger mode intact.
 */
struct RemovedColliderState
{
    bool valid = false;
    Vector2 size{ 1.f, 1.f };
    bool isTrigger = false;
};

/**
 * @struct RemovedEmitterState
 * @brief Stores a removed ParticleEmitter component state for restoration.
 *
 * This structure preserves all editable emitter parameters so the particle
 * emitter can be re-created later with the same visual and simulation setup.
 */
struct RemovedEmitterState
{
    bool valid = false;

    bool enabled = false;
    float rate = 50.f;
    float particleLife = 0.6f;
    Vector2 offset{ 0.f, 0.f };
    Vector2 velMin{ -30.f, 50.f };
    Vector2 velMax{ 30.f, 90.f };
    Vector3 colorStart{ 1.f, 1.f, 1.f };
    Vector3 colorEnd{ 0.8f, 0.8f, 0.8f };
    float sizeStart = 10.f;
    float sizeEnd = 1.f;

    Mesh2D* quad = nullptr;
    unsigned texture = 0;
    float timeAccumulator = 0.f;
};

/**
 * @class EditorOverlay
 * @brief ImGui-based in-game editor overlay for scene and entity editing.
 *
 * EditorOverlay provides the full runtime editor interface used by the game.
 * It manages the docked ImGui layout, viewport interaction, entity selection,
 * property editing, spawning tools, tile editing, asset browsing, debug views,
 * and play/stop integration.
 *
 * The editor does not directly own core game logic. Instead, it communicates
 * with the rest of the engine through:
 * - IComponentContext for ECS data access
 * - deferred spawn and destroy queues
 * - callback functions supplied by the game layer
 *
 * This keeps editor functionality separated from the gameplay systems while
 * still allowing live editing of scene content.
 */
class EditorOverlay {
public:
    /**
     * @brief Constructs an editor overlay bound to an ECS component context.
     *
     * Initializes editor-side state and prepares the overlay to inspect and edit
     * entities through the provided ECS context.
     *
     * @param ctx Reference to the ECS component context used by the editor.
     */
    explicit EditorOverlay(IComponentContext& ctx);

    /**
     * @brief Begins a new ImGui frame for the editor.
     *
     * Prepares the editor UI for rendering during the current frame.
     */
    void BeginFrame();  ///< Start a new ImGui frame

    /**
     * @brief Draws the complete editor overlay.
     *
     * Renders the editor dockspace and all enabled editor panels such as the
     * hierarchy, property inspector, asset browser, viewport, history, and
     * play controls.
     *
     * @param dt Delta time in seconds for the current frame.
     */
    void Draw(float dt);        ///< Draw all editor windows (Performance, Hierarchy)

    /**
     * @brief Ends the current ImGui frame.
     */
    void EndFrame();    ///< Render ImGui draw data
    ~EditorOverlay(); //dtor

    // ---------------------------------------------------------------------
    // External integration (GameApp interfacing)
    // ---------------------------------------------------------------------

    /**
     * @brief Assigns the active camera used by the editor viewport.
     *
     * The editor uses this camera for viewport interaction, world-to-screen
     * conversion, gizmo placement, and optional camera-follow behavior.
     *
     * @param cam Pointer to the camera used by the editor.
     */
    void SetCamera(Camera2D* cam) { activeCamera = cam; }

    /**
     * @brief Returns the current pending spawn request.
     *
     * The game layer reads this request after the editor frame and performs the
     * actual entity creation.
     *
     * @return Constant reference to the pending spawn request.
     */
    const SpawnRequest& GetSpawnRequest() const { return spawnRequest; }
    /** @brief Clears spawn intentions after GameApp handles them. */
    void ClearSpawnRequest() { spawnRequest.type = SpawnRequest::Type::None; }

    /** @brief Retrieves pending requests to destroy entities.
     *  @return A constant reference to the vector of entities to destroy.
     */
    const std::vector<Entity>& GetPendingDestroyList() const { return pendingDestroyList; }
    /** @brief Clears entity destroy queue. */
    void ClearPendingDestroyList() { pendingDestroyList.clear(); }

    /** @return True if the editor is visible. */
    bool IsVisible() const { return visible; }
    /** @brief Toggles editor overlay visibility. */
    void ToggleVisible() { visible = !visible; }

    /**
     * @brief Sets the visibility state of the editor overlay.
     * @param v True to make visible, false to hide.
     */
    void SetVisible(bool v) { visible = v; }

    /**
     * @brief Callback invoked when the editor requests a level change.
     * @param scenePath Scene file path selected by the editor UI.
     */
    std::function<void(const std::string&)> onLevelChange;

    /**
     * @brief Callback invoked when the editor requests a full level save.
     * @param scenePath Scene file path currently shown in the editor UI.
     */
    std::function<void(const std::string&)> onLevelSave;

    /**
     * @brief Callback invoked when the editor requests saving light data only.
     * @param scenePath Scene file path used to derive the light sidecar path.
     */
    std::function<void(const std::string&)> onLightSave;

    /** @brief Callback invoked when the editor requests saving labyrinth variants. */
    std::function<void()> onSaveLabyrinthVariants;

    /** @brief Callback invoked to request camera lock/unlock in the game layer. */
    std::function<void(bool)> onRequestCameraLock;

    // callbacks into GameApp:
    /** @brief Callback invoked when entering tile edit mode (game should provide the grid). */
    std::function<void()> onTileEditEnter;
    /** @brief Callback invoked when leaving tile edit mode. */
    std::function<void()> onTileEditExit;
    /** @brief Callback invoked when applying the edited tile grid back into the game. */
    std::function<void(const std::vector<char>& grid, int w, int h)> onTileApply;

    // Called by GameApp when it wants to feed a labyrinth into the editor
    /**
     * @brief Loads a tile grid from the game application into the editor.
     * @param data Raw tile data vector.
     * @param w Grid width.
     * @param h Grid height.
     */
    void SetTileGrid(const std::vector<char>& data, int w, int h) { tileGrid_ = data; gridW_ = w; gridH_ = h; tileDirty = false; }
    
    /**
     * @brief Retrieves the current tile grid from the editor.
     * @param out Output vector to store the tile data.
     * @param w Output parameter for grid width.
     * @param h Output parameter for grid height.
     */
    void GetTileGrid(std::vector<char>& out, int& w, int& h) const {out = tileGrid_; w = gridW_; h = gridH_; }

    /** @brief Path to the level currently being edited. */
    std::string currentLevelPath = "Assets/scene/labyrinth.json";

    /** @return Pending level file path (if changed via UI). */
    const std::string& GetPendingLevelChange() const { return pendingLevelPath; }
    /** @brief Clear pending level load request after GameApp processes it. */
    void ClearPendingLevelChange() { pendingLevelPath.clear(); }

    /**
     * @brief Request a level reload using a scene path.
     * @param scenePath Scene path to reload (e.g. "scene/labyrinth.json").
     */
    void RequestLevelReload(const std::string& scenePath) { pendingLevelPath = scenePath; }




    // Callback: game enters playing mode
    std::function<void()> onPlay;
    std::function<void()> onStop; // Callback: game reverts to editor mode

    // ======================
    // UNDO / REDO CALLBACKS
    // ======================

    /**
     * @brief Callback invoked when the editor requests an undo operation.
     */
    std::function<void()> onUndo;
    /**
     * @brief Callback invoked when the editor requests a redo operation.
     */
    std::function<void()> onRedo;
    std::function<void(Entity)> onRecordEntityCreated;
    std::function<void(Entity)> onRecordEntityDeleted;
    std::function<void(Entity)> onRecordBeforeTransform;
    std::function<void(Entity)> onRecordAfterTransform;

    UndoRedoManager* undoRedoPtr = nullptr;

    /**
     * @brief Sets the internal playing state flag.
     * @param v True if the game is playing, false if stopped/editing.
     */
    void SetIsPlaying(bool v) { isPlaying = v; }
    
    /**
     * @brief Checks if the game is currently in play mode.
     * @return True if playing.
     */
    bool GetIsPlaying() const { return isPlaying; }

    /**
     * @brief Sets the scene texture to be displayed in the editor viewport.
     * @param textureId OpenGL texture ID for the rendered scene.
     * @param width Width of the scene texture.
     * @param height Height of the scene texture.
     */
    void SetSceneTexture(unsigned int textureId, int width, int height);

    /**
     * @brief Enables or disables the transform gizmo.
     * @param v True to enable the gizmo.
     */
    void SetGizmoEnabled(bool v) { gizmoEnabled = v; }
    
    /**
     * @brief Checks if the transform gizmo is currently enabled.
     * @return True if enabled.
     */
    bool IsGizmoEnabled() const { return gizmoEnabled; }

    Messaging::Observable consoleObservable;

    // --- Prefab tools ---
    std::function<bool(Entity)>         isPrefabInstance;        // does entity come from a prefab
    std::function<void(Entity)>         onPrefabRevertInstance;  // revert this instance to prefab
    std::function<void(Entity)>         onPrefabApplyFromInstance; // use this instance as new prefab defaults
    std::function<void()> onPrefabSaveAll;

    bool transformEditing = false;
    bool transformEditStarted = false;
    Entity transformEditEntity = INVALID_ENTITY;

    std::function<void()> onPause;
    
    /**
     * @brief Sets the internal paused state flag.
     * @param v True if the game is paused.
     */
    void SetIsPaused(bool v) { isPaused = v; }
    
    /**
     * @brief Checks if the game is currently paused via the editor.
     * @return True if paused.
     */
    bool IsPaused() const { return isPaused; }

    /**
     * @brief Sets the list of available prefabs for spawning.
     * @param prefabs Vector of prefab name strings.
     */
    void SetAvailablePrefabs(std::vector<std::string> prefabs)
    {
        availablePrefabs_ = std::move(prefabs);
    }

    void SetTextureLookup(std::function<unsigned int(const std::string&)> fn)
    {
        getTextureID = std::move(fn);
    }

    void SetPrefabTextureLookup(std::function<unsigned int(const std::string&)> fn)
    {
        getPrefabTexture = std::move(fn);
    }

    /**
     * @brief Registers an entity associated with a specific labyrinth wall cell.
     * @param gx Grid X coordinate.
     * @param gy Grid Y coordinate.
     * @param e Entity ID to register.
     */
    void RegisterLabyrinthWallEntity(int gx, int gy, Entity e);

    /**
     * @brief Clears all registered labyrinth wall entities.
     */
    void ClearLabyrinthWallEntities();

    /**
     * @brief Updates or inserts a new wall variant (texture/prefab) at the specified grid cell.
     * @param gx Grid X coordinate.
     * @param gy Grid Y coordinate.
     * @param textureKey Texture or prefab identifier.
     * @param solid Whether the variant acts as a solid collision wall.
     * @param isPrefab Whether the key refers to a prefab rather than a simple texture.
     */
    void UpsertLabyrinthWallVariant(int gx, int gy, const std::string& textureKey, bool solid = true, bool isPrefab = false);

    /**
     * @brief Retrieves the current list of all labyrinth wall variants.
     * @return A constant reference to the vector of VariantPlacement structures.
     */
    const std::vector<VariantPlacement>& GetLabyrinthWallVariants() const;

    /**
     * @brief Sets the list of labyrinth wall variants.
     * @param variants The vector of VariantPlacement structures to use.
     */
    void SetLabyrinthWallVariants(const std::vector<VariantPlacement>& variants);

    /**
     * @brief Clears all stored labyrinth wall variants.
     */
    void ClearLabyrinthWallVariants();

    /**
     * @brief Removes a specific labyrinth wall variant at the given coordinates.
     * @param gx Grid X coordinate.
     * @param gy Grid Y coordinate.
     */
    void RemoveLabyrinthWallVariant(int gx, int gy);

    /**
     * @brief Unregisters the entity associated with a specific labyrinth wall cell.
     * @param gx Grid X coordinate.
     * @param gy Grid Y coordinate.
     */
    void UnregisterLabyrinthWallEntity(int gx, int gy);

    /**
     * @brief Attempts to find the grid coordinates of a given labyrinth wall entity.
     * @param e The entity ID to search for.
     * @param gx Output reference for the grid X coordinate.
     * @param gy Output reference for the grid Y coordinate.
     * @return True if the entity was found, false otherwise.
     */
    bool TryFindLabyrinthWallCell(Entity e, int& gx, int& gy) const;

    /** @brief Callback invoked to spawn a new labyrinth wall variant entity. */
    std::function<Entity(int gx, int gy, const std::string& textureKey, float tileSize)> onCreateLabyrinthWallVariant;

    /**
     * @brief Sets the size of labyrinth tiles in world units.
     * @param ts The tile size.
     */
    void SetLabyrinthTileSize(float ts) { labyrinthTileSize_ = ts; }

    /**
     * @brief Gets the current mouse position in world space based on the editor's active camera.
     * @return The mouse position vector.
     */
    Vector2 GetWorldMousePosition() const;

    /**
     * @brief Checks whether the mouse cursor is currently inside the editor viewport.
     * @return True if inside the viewport, false otherwise.
     */
    bool IsMouseInViewport() const;

private:
    // ---------------------------------------------------------------------
    // Internal UI windows and interaction handlers
    // ---------------------------------------------------------------------
    void DrawPerformance(float dt);
    void DrawSpawner();
    void DrawHierarchy();
    void HandleViewportDrag();
    void DrawPropertyEditor();
    void DrawLevelSelector();
    void DrawAssetsBrowser();
    void DrawViewportDropTarget();
    void DrawCameraModePanel();    
    void HandleNavigationClick();
    void DrawPlayBar();
	void DrawGizmoInViewport();
    void DrawGizmoBar();
    void DrawTilemapEditor();
    void DrawDebugConsole();
    void DrawHistory();
    void DrawItemPalette();


    /**
     * @brief Ensures Camera2D viewport matches ImGui working area.
     * @param vp ImGui viewport from engine window.
     */
    void SyncCameraViewportToWorkArea_(const ImGuiViewport* vp);

    void UpdateSelectionCameraFollow();

    Camera2D* activeCamera = nullptr;

    IComponentContext& context;
    bool visible = false;
    int selectedEntity = -1;

    ImGuiID centerDockId_ = 0;
    bool firstDockBuild_ = true;
    bool lastGizmoWasUsing = false;

    bool gizmoActive = false;
    Entity gizmoEntity = INVALID_ENTITY;

    ImVec2 spawnerPosPct{ 0.0075f, 0.27f }; // initial anchor: 2% from left, 20% from top
    ImVec2 lastWorkSize{ 0, 0 };   // track viewport size 
    Vector2 grabDeltaScreenBL{ 0.f, 0.f }; 

    int texChoice = 0;
    Vector2 spawnPos = { 0.f, 0.f };
    Vector2 spawnScale = { 64.f, 64.f };
    float spawnRotDeg = 0.f;

    SpawnRequest spawnRequest;
    std::vector<Entity> pendingDestroyList;

    bool dragging = false;
    Entity dragEntity = 0;   // custom invalid constant or 0
    Vector2 dragOffset = { 0, 0 };
    std::string pendingLevelPath;

    ImVec2 lastSpawnerPos{ 0,0 };
    ImVec2 lastSpawnerSize{ 0,0 };
    bool assetsBrowserPlacedOnce = false;

    bool navMode = false;          // true = Navigation Camera mode

    ImVec2 workToFBScale_{ 1.f, 1.f };

    bool isPlaying = false;

    bool focusPropsNextFrame = false;

    bool gizmoEnabled = false;

    ImGuizmo::OPERATION gizmoOp = ImGuizmo::TRANSLATE;
    
    // Offscreen game texture (rendered by Renderer when editor is visible)
    unsigned int sceneTextureId = 0;
    int sceneTexWidth = 0;
    int sceneTexHeight = 0;

    // --- Camera follow for selected entity (editor-side) ---
    bool   followSelection = false;  // true = camera tracks selected entity
    Entity followEntity = 0;      // entity to follow when followSelection is true

    // --- tile editor ---
    bool tileEditMode = false;
    bool tileEditModePrev = false;

    // local copy of the map grid the editor is painting on
    std::vector<char> tileGrid_;
    int gridW_ = 0;
    int gridH_ = 0;
    bool tileDirty = false;

    void CenterGroup(float groupWidth);

    // per-level tilemap storage
    std::unordered_map<std::string, std::vector<char>> tileGridPerLevel;
    std::unordered_map<std::string, std::pair<int, int>> tileSizePerLevel;

    bool isPaused = false;

    std::unordered_map<Entity, RemovedMeshRendererState> removedRendererState;
    std::unordered_map<Entity, RemovedAnimatorState> removedAnimatorStates;
    std::unordered_map<Entity, RemovedColliderState> removedColliderStates;
    std::unordered_map<Entity, RemovedEmitterState> removedEmitterStates;

    /* ------------------------------------------------------------ */
    /* Item palette (shared with RoomEditor)                         */
    /* ------------------------------------------------------------ */

    std::vector<std::string> availablePrefabs_;

    std::string selectedPaletteTextureKey_;
    std::string selectedPalettePrefabKey_;

    std::function<unsigned int(const std::string&)> getTextureID;
    std::function<unsigned int(const std::string&)> getPrefabTexture;

    std::unordered_map<long long, Entity> labyrinthWallEntities_;

    static long long MakeWallCellKey(int gx, int gy);
    Entity FindLabyrinthWallEntity(int gx, int gy) const;

    std::vector<VariantPlacement> labyrinthWallVariants_;

    VariantPlacement* FindLabyrinthWallVariant(int gx, int gy);
    const VariantPlacement* FindLabyrinthWallVariant(int gx, int gy) const;

    float labyrinthTileSize_ = 100.0f;
};
#endif
