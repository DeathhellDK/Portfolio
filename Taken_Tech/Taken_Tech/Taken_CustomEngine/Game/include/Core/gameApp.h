#pragma once
/**
 * @file      gameApp.h
 * @author    Jethro Sung, Woh Kye Le
 * @co_author Tan Wei Liang Terril, Sng Swee Yong Dillon, Low JianLin, Lim Zhi Jie
 * @email     sung.h, w.kyele,sweeyongdillon.sng, t.weiliangterril, jianlin.low, zhijie.lim
 * @date      2025-09-11
 *
 * @brief    Declaration of the GameApp class.
 *
 * The GameApp class acts as the �world� manager for the game. It owns and manages
 * all entities and their components (Transform, MeshRenderer, Collider, etc.),
 * handles system initialization (movement, collision, rendering, animation),
 * runs per-frame update and draw calls, and integrates ImGui for debugging.
 *
 * It also provides factory methods for adding components to entities, manages
 * entity lifetime, and coordinates interactions between subsystems.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */


#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <string>
#include <deque>

 // Core / ECS
#include "Core/window.h"
#include "Core/gameobj.h"
#include "Core/transform.h"
#include "Core/entitymanager.h"
#include "Core/component.h"
#include "Core/componentcontext.h"
#include "Core/persistentTag.h"
#include "projectile.h"
#include "Mechanics/projectileSystem.h"

// Graphics / Rendering
#include "Graphics/renderer.h"
#include "Graphics/camera2d.h"
#include "Graphics/shader.h"
#include "Graphics/mesh2d.h"
#include "Graphics/vertex2d.h"
#include "Graphics/meshrenderer.h"
#include "Graphics/spriteanimator.h"
#include "Core/layering.h"

// Math
#include "Math/vect2.h"
#include "Math/vect3.h"

// Physics
#include "Physics/collider.h"
#include "Physics/PhysicsDebugger.h"
#include "Physics/PhysicsSpeedComponent.hpp"
#include "Physics/PhysicsMassComponent.hpp"
#include "Physics/ForceSystem.hpp"

// Systems
#include "Core/Systems/renderSystem.h"
#include "Core/Systems/lightSystem.h"
#include "Core/Systems/collisionSystem.h"
#include "Core/Systems/animationSystem.h"
#include "Core/Systems/particleSystem.h"
#include "Core/Systems/systemManager.h"
#include "movementsystem.h"

// Gameplay / Logic
#include "playerController.h"
#include "enemyController.h"
#include "World/DoorSystem.h"
#include "Scripting/scriptcomponent.h"
#include "puzzleObject.h"
#include "World/puzzleSystem.h"

// Particle
#include "Particle/particleEmitter.h"

// Light
#include "Light/lightComponent.h"

// Editor / UI
#include "UI/guiSys.h"
#include "Core/sceneManager.h"
#include "UI/mainMenu.h"
#include "UI/pauseMenu.h"
#include "UI/settingsMenu.h"
#include "UI/controlMenu.h"
#include "UI/creditsMenu.h"
#include "UI/tutorialMenu.h"
#include "UI/confirmDialog.h"
#include "UI/loseMenu.h"
#include "UI/playerHUD.h"

// Messaging
#include "Input/message_system.h"

// Miscellaneous
#include "config.h"
#include "Audio/audio.h"
#include "resourceManager.h"
#include "prefabManager.h"
#include "UI/miniMap.h"
#include "StressTestManager.h"
#include "loader.h"

// Undo and Redo
#include "Editor/UndoRedoManager.h"

#include "Scripting/LuaEngine.h"

#include "Core/Memory/FixedBlockPool.h"

#include "layerManager.h"

#ifndef ENABLE_EDITOR
#define ENABLE_EDITOR 0
#endif

namespace {
#ifdef _DEBUG
	constexpr bool DEBUG_MODE = true;
#else
	constexpr bool DEBUG_MODE = false;
#endif
}


// Forward-declare editor type so we can hold a pointer/unique_ptr without including editor.h here
class EditorOverlay;
class RoomEditor;
class UiEditor;
struct GLFWwindow;

namespace DebugDraw { void ColliderOutlines(class GameApp&, class Renderer&, int); }

class Shader;
class StressTestManager;






/**
 * @class GameApp
 * @brief Central application class that manages all entities, components, and systems.
 *
 * GameApp owns all ECS component arrays (e.g., Transform, MeshRenderer, Collider),
 * and is responsible for:
 * - Initializing subsystems and resources
 * - Managing entity/component creation and destruction
 * - Running the main per-frame update and draw cycles
 * - Providing editor/debug tools through ImGui
 */
class GameApp : public IComponentContext {
public:
	// ====================================== Construction & App Lifecycle ======================================
	// ========================================================================================================== 
	/**
	* @brief Construct a new GameApp.
	*/
	GameApp();

	/**
	 * @brief Destroy the GameApp and clean up resources.
	 *
	 * Defined in gameApp.cpp where EditorOverlay is fully visible.
	 */
	~GameApp();

	/**
	 * @brief Initialize the game application and its subsystems.
	 *
	 * Loads resources, configures the camera, initializes ECS systems,
	 * and loads initial levels or entities.
	 *
	 * @param renderer Reference to the Renderer used for drawing.
	 * @return true on success, false if initialization failed.
	 */
	bool Initialize(Renderer& renderer, float dt);

	/**
	* @brief Update the entire game world for one rendered frame (variable dt).
	*
	* This is a per-frame update. It handles real-time
	* inputs (key/mouse), window/viewport sync, ImGui/editor toggles,
	* non-deterministic UI and FX,
	*
	* @param dt Variable delta time in seconds since the previous rendered frame.
	*
	*/
	void Update(double dt);


	/**
	* @brief Advance deterministic simulation by one fixed step (fixed dt).
	*
	* This is the update that is called one or more times per frame by the
	* fixed-step eng::forEachFixedStep. all deterministic, time-critical systems
	* are handle here so the game behaves the same regardless of framerate:
	*
	* @param fdt Fixed delta time in seconds
	*/
	void FixedUpdate(double fdt);

	

	/**
	 * @brief Draw all renderable entities.
	 *
	 * Invokes the RenderSystem to draw entities and debug overlays.
	 *
	 * @param renderer Reference to the renderer.
	 */
	void Draw(Renderer& renderer);

	/**
	 * @brief Shutdown and free all game resources and systems.
	 */
	void Shutdown();

	/**
	* @brief Request the main loop to exit
	*/
	void RequestQuit() { shouldQuit = true; }

	/**
	 * @brief Ticks AI behavior and shooting logic during fixed update steps.
	 *
	 * @param fdt Time step in seconds.
	 */
	void TickAIAndShooting(double fdt);

	/**
	 * @brief Ticks projectile movement, lifetime, and collision.
	 * 
	 * @param fdt Time step in seconds.
	 */
	void TickProjectiles(float fdt);

	/**
	 * @brief Ticks scripts for all entities that have them.
	 * 
	 * @param fdt Time step in seconds.
	 */
	void TickScripts(float fdt);

	/**
	 * @brief Ticks animations for all entities.
	 * 
	 * @param fdt Time step in seconds.
	 */
	void TickAnimations(float fdt);

	/**
	 * @brief Simulates a fixed time step for physics and other systems.
	 *
	 * @param fdt Time step in seconds.
	 * @param stepAllowed If true, the simulation will proceed.
	 */
	void SimulateFixedStep(float fdt, bool stepAllowed);

	/**
	 * @brief Updates door timers and states during fixed update steps.
	 */
	void UpdateDoorsFixed();

	/**
	 * @brief Records the current position, scale, and rotation of all entities.
	 *
	 * Used for undo/redo and transform interpolation.
	 */
	void SavePreviousTransforms();

	// =================================== Systems Orchestration & Signatures ===================================
	// ========================================================================================================== 
	SystemManager    systemManager; // System manager handling movement, collision, render, animation systems
	MovementSystem* moveSys = nullptr;
	CollisionSystem* collisionSys = nullptr;
	AnimationSystem* animationSys = nullptr;
	RenderSystem* renderSys = nullptr;
	LightSystem* lightSys = nullptr;
	ParticleSystem* particleSys = nullptr;
	PuzzleSystem* puzzleSys = nullptr;

	/**
	* @brief Set the component signature for an entity.
	* @param entity Entity to modify.
	* @param signature Bitmask of components.
	*/
	void SetSignature(Entity entity, const Signature& signature)
	{
		entityManager.SetSignature(entity, signature);
		RefreshLayerMembership(entity);
	}

	/**
	 * @brief Get the component signature of an entity.
	 * @param entity Entity to query.
	 * @return Signature of the entity.
	 */
	Signature GetSignature(Entity entity) const { return entityManager.GetSignature(entity); }

	/**
	 * @brief Get all stored entity signatures.
	 * @return Map of entity to Signature.
	 */
	const std::unordered_map<Entity, Signature>& GetEntitySignatures() const override { return entityManager.GetAllSignatures(); }



	// ============================== Entity Lifecycle (create/destroy/bulk remove) =============================
	// ========================================================================================================== 
	EntityManager entityManager; // Entity manager responsible for allocation and destruction of entities.

	/**
	 * @brief Destroy an entity and remove all its components.
	 * @param e Entity to destroy.
	 */
	void DestroyEntity(Entity e);

	/**
	 * @brief Remove all components associated with entities in the given set.
	 * Used in stress tests or mass-despawn operations.
	 * @param doomed Set of entity IDs to remove.
	*/
	void RemoveEntities(const std::unordered_set<Entity>& doomed);



	// ==================================== Component Factories (Add*) writers ==================================
	// ========================================================================================================== 
	/**
	 * @brief Add a Transform component to an entity.
	 *
	 * @param e Entity to attach to.
	 * @param pos Initial position.
	 * @param scale Initial scale.
	 * @param rot Initial rotation (radians).
	 * @return Reference to the created Transform component.
	 */
	Transform& AddTransform(Entity e, const Vector2& pos, const Vector2& scale, float rot);

	/**
	* @brief Add a MeshRenderer component to an entity.
	*
	* @param e Entity to attach to.
	* @param mesh Pointer to the mesh to render.
	* @param color Tint color.
	* @return Reference to the created MeshRenderer.
	*/
	MeshRenderer& AddRenderer(Entity e, Mesh2D* mesh, const Vector3& color);

	/**
	 * @brief Add a Collider component to an entity.
	 *
	 * @param e Entity to attach to.
	 * @param size Collider size (Box or Circle radius in x).
	 * @param trigger True if trigger collider, false if physical.
	 * @return Reference to the created Collider.
	 */
	Collider& AddCollider(Entity e, const Vector2& size, bool trigger);

	/**
	 * @brief Add a PlayerController component to an entity.
	 * @param e Entity to attach to.
	 * @return Reference to the created PlayerController.
	 */
	PlayerController& AddController(Entity e);

	/**
	 * @brief Add an EnemiesController to an entity.
	 * @param e Entity ID.
	 * @return Reference to the created EnemiesController.
	 */
	EnemiesController& AddEnemyController(Entity e);

	/**
	 * @brief Add a SpriteAnimator component to an entity.
	 * @param e Entity to attach to.
	 * @return Reference to the created SpriteAnimator.
	 */
	SpriteAnimator& AddAnimator(Entity e);

	/**
	 * @brief Add a PersistentTag to an entity.
	 * @param e Entity ID.
	 * @return Reference to the created PersistentTag.
	 */
	PersistentTag& AddPersistentTag(Entity e) override;
	void RemovePersistentTag(Entity e) override;
	bool HasPersistentTag(Entity e) const override;

	/**
	 * @brief Add a ParticleEmitter to an entity.
	 * @param e Entity ID.
	 * @return Reference to the created ParticleEmitter.
	 */
	ParticleEmitter& AddEmitter(Entity e);
	LightComponent& AddLight(Entity e);


	PuzzleObject& AddPuzzleObject(Entity e, PuzzleKind kind, int groupId);

	ProjectileComponent& AddProjectile(Entity e, Entity source, const Vector2& vel, float lifeSeconds, int dmg, bool fromEnemy);

	/**
	* @brief Add a speed component to the entities
	* @param e Entity to which the SpeedComponent will be added.
	* @param speedComp SpeedComponent data to assign to the entity.
	*/
	void AddPhysicsSpeedComponent(Entity e, const SpeedComponent& speedComp);

	/**
	* @brief Adds a MassComponent to the specified entity.
	* @param e Entity to which the MassComponent will be added.
	* @param massComp MassComponent data to assign to the entity.
	*/
	void AddPhysicsMassComponent(Entity e, const MassComponent& massComp);



	// ============================= Component Accessors (Get* by entity) readers  ==============================
	// ========================================================================================================== 
	/**
	 * @brief Get a Transform component by entity.
	 */
	Transform* GetTransform(Entity e) override;
	/**
	 * @brief Get a MeshRenderer component by entity.
	 */
	MeshRenderer* GetRenderer(Entity e) override;
	/**
	 * @brief Get a Collider component by entity.
	 */
	Collider* GetCollider(Entity e) override;
	/**
	 * @brief Get a PlayerController component by entity.
	 */
	PlayerController* GetController(Entity e) override;
	/**
	 * @brief Get a EnemiesController component by entity.
	 */
	EnemiesController* GetEnemyController(Entity e);
	/**
	 * @brief Get a SpriteAnimator for an entity
	 */
	SpriteAnimator* GetAnimator(Entity e) override;
	/**
	 * @brief Get a ParticleEmitter for an entity
	 */
	ParticleEmitter* GetEmitter(Entity e) override;

	LightComponent* GetLight(Entity e) override;

	PuzzleObject* GetPuzzleObject(Entity e);

	/**
	 * @brief Get a PersistentTag map
	 */
	const std::unordered_map<Entity, PersistentTag>& GetPersistentTags() const { return persistentTags; }

	// IComponentContext: component creation/removal for editor
	MeshRenderer*	   AddRendererComponent(Entity e)	override;
	Collider*		   AddColliderComponent(Entity e)	override;
	SpriteAnimator*	   AddAnimatorComponent(Entity e)	override;
	PlayerController*  AddControllerComponent(Entity e) override;
	ParticleEmitter*   AddEmitterComponent(Entity e)	override;
	LightComponent* AddLightComponent(Entity e)     override;
	SpeedComponent*    AddSpeedComponent(Entity e)		override;
	MassComponent*     AddMassComponent(Entity e)       override;
	EnemiesController* AddEnemyControllerComponent(Entity e) override;

	void RemoveRendererComponent(Entity e)	 override;
	void RemoveColliderComponent(Entity e)	 override;
	void RemoveAnimatorComponent(Entity e)	 override;
	void RemoveControllerComponent(Entity e) override;
	void RemoveLightComponent(Entity e)     override;
	void RemoveEmitterComponent(Entity e)	 override;
	void RemoveSpeedComponent(Entity e)		 override;
	void RemoveMassComponent(Entity e)		 override;
	void RemoveEnemyControllerComponent(Entity e) override;

	// =================================== Prefabs, Scenes & Serialization  =====================================
	// ========================================================================================================== 
	PrefabManager prefabManager;
	static Entity playerEntity; //player's entity 

	// --- Prefab instance defaults for override detection ---
	struct PrefabInstanceDefaults {
		Vector2 scale{ 1.f, 1.f };
		float   rotation{ 0.f };
		Vector3 color{ 1.f, 1.f, 1.f };
	};

	/**
	 * @brief Initialize RoomEditor, UiEditor and EditorOverlay.
	 */
	void InitializeRoomEditor();

	/**
	 * @brief Initialize the UI Editor.
	 */
	void InitializeUiEditor();

	/**
	* @brief Instantiate a prefab at a position.
	* @param name Prefab name.
	* @param pos  Spawn world position.
	* @return New entity ID.
	*/
	Entity InstantiatePrefab(const std::string& name, const Vector2& pos);

	/**
	* @brief Save the current scene to a JSON level file.
	* @param path Path where the level JSON file will be saved.
	*/
	void SaveLevel(const std::string& path);

	/**
	* @brief Set the prefab tag for an entity.
	* @param e Entity ID.
	* @param name Prefab tag name to assign.
	*/
	void SetPrefabTag(Entity e, const std::string& name);

	/**
	* @brief Get the prefab tag for an entity.
	* @param e Entity ID.
	* @return The prefab tag if the entity has one, otherwise an empty string.
	*/
	std::string GetPrefabTag(Entity e) const {
		auto it = prefabTags.find(e);
		return (it != prefabTags.end()) ? it->second : std::string{};
	}

	const PrefabInstanceDefaults* TryGetPrefabDefaults(Entity e) const {
		auto it = prefabDefaults.find(e);
		return (it != prefabDefaults.end()) ? &it->second : nullptr;
	}
	
	/**
	* @brief Register the original prefab values for an entity instance.
	*
	* Stores the default values of a prefab instance before any JSON overrides are applied.
	* @param e Entity ID to register defaults for.
	*/
	void RegisterPrefabDefaults(Entity e);

	/**
	* @brief Revert an instance's transform/color back to its stored prefab defaults.
	* @param e Entity ID to revert.
	*/
	void RevertInstanceToPrefab(Entity e);

	/**
	* @brief Use an instance's current values as new prefab defaults.
	*
	* Applies the given instance's current values as new prefab defaults
	* and propagates them to all non-overridden instances of that prefab
	* in the currently loaded scene.
	* @param e Entity ID whose values should become the new prefab defaults.
	*/
	void ApplyInstanceAsPrefab(Entity e);

	/**
	* @brief Save all prefabs to JSON files.
	*
	* Serializes the current prefab definitions to JSON format for persistent storage.
	*/
	void SavePrefabsToJson() const;

	/**
	* @brief Clears all enemy controllers from the system.
	*/
	void ClearEnemiesControllers() { enemiesControllers.clear(); }

	/**
	* @brief Clears the list of spawned entities and resets the selection.
	*/
	void ClearSpawnedEntitiesSelection() { spawnedEntities.clear(); selectedEntity = -1; }

	/**
	* @brief Clears all prefab default values.
	*/
	void ClearPrefabDefaults() { prefabDefaults.clear(); }

	/**
	* @brief Removes the prefab tag from an entity if it exists.
	* @param e The entity to check and remove prefab tag from.
	*/
	void ClearPrefabTagIfAny(Entity e);

	/**
	* @brief Destroys all entities that are not marked as persistent.
	*
	* Removes all entities that have a Transform component but are not
	* in the persistent tags set. Also cleans up prefab tag metadata.
	*/
	void DestroyAllNonPersistentEntities();

	// =============================================== Scripting  ===============================================
	// ========================================================================================================== 
	// --- Attach a script to an entity ---
	void AddScript(Entity e, std::unique_ptr<IScript> script);
	void AddLuaScript(Entity e, const std::string& luaFile);
	float AI_TileSize() const { return aiTileSize_; }

	// Script editor bridge (implements IComponentContext)
	const std::vector<std::string>& GetScriptNameList() const override;
	std::string GetEntityScriptName(Entity e) const override;
	void SetEntityScriptName(Entity e, const std::string& name) override;

	bool AI_IsBlockedWorld(float wx, float wy) const;
	bool AI_IsWallWorld(float wx, float wy) const;
	bool AI_HasLOSWorld(float ax, float ay, float bx, float by, Entity e, PlayerAbility ability) const;

	// Use physics/speed component if possible (like ai.cpp forceproxy behavior)
	void AI_MoveEntityWorld(Entity e, float vx, float vy, float dt);

	ScriptComponent* GetScript(Entity e) {
		auto it = scriptComponents.find(e);
		return (it != scriptComponents.end()) ? &it->second : nullptr;
	}

	// --- IComponentContext scripting bridge ---
	std::vector<Entity> GetScriptedEntities() const override;
	void ScriptStartIfNeeded(Entity e) override;
	void ScriptUpdate(Entity e, float dt) override;
	void SpawnEnemyProjectile(Entity shooter, float dirX, float dirY) override;

	// --- Script DLL lifecycle ---
	//bool LoadScriptsDLL(const char* dllPath = "Scripts.dll");
	//void UnloadScriptsDLL();
	//bool ReloadScriptsDLL();     // deletes scripts, reloads, reattaches

	//// --- Helpers ---
	//void ScriptDetachAll();      // delete instances, keep kind
	//void ScriptReattachAll();    // rebuild instances based on kind

	bool AreScriptsEnabled() const override { return scriptsEnabled; }


	// ====================================== Editor (ImGui / HUD Interface) ====================================
	// ========================================================================================================== 


	// --- editor / ImGui state ---
	std::vector<Entity> spawnedEntities;
	int selectedEntity = -1;
	int texChoice = 0;
	Vector2 spawnPos = Vector2(0.f, 0.f);
	Vector2 spawnScale = Vector2(120.f, 120.f);
	float spawnRotDeg = 0.f;

	// --- undo/redo 
	bool GetIsPlaying()      const override { return isPlaying; }
	UndoRedoManager& GetUndoRedo() override { return *undoRedo; }

	// Returns true if this entity was spawned from a prefab and has stored defaults.
	bool HasPrefabDefaults(Entity e) const { return prefabDefaults.find(e) != prefabDefaults.end(); }

	EditorOverlay* GetEditorOverlay()
	{
		#if ENABLE_EDITOR
				return editorOverlay.get();
		#else
				return nullptr;
		#endif
	}

	RoomEditor* GetRoomEditor()
	{
		#if ENABLE_EDITOR
				return roomEditor.get();
		#else
				return nullptr;
		#endif
	}

	UiEditor* GetUiEditor()
	{
		#if ENABLE_EDITOR
				return uiEditor.get();
		#else
				return nullptr;
		#endif
	}
#if ENABLE_EDITOR
	void SyncEditorToActiveScene();
	unsigned int EditorGetTextureID(const std::string& textureKey);
	unsigned int EditorGetPrefabTexture(const std::string& prefabName);
#endif


	// --- Recreate ImGui editor after map regeneration ---
	void RecreateEditorOverlay();

	// --- Editor Asset Hooks ---
	virtual const std::vector<std::string>& GetTextureList()						                 const override;
	virtual void                            ApplyTexture(Entity e, const std::string& name)               override;
	virtual bool                            ImportTexture(const std::string& abs, const std::string& name) override;

	GuiSystem& GetHUDGui() { return gui; }
	bool IsHUDVisible() const { return guiVisible; }

	// =================================== World Systems (Doors, MiniMap, etc) ==================================
	// ========================================================================================================== 
	// --- door entrance (location) --- 

	DoorSystem& GetDoorSystem() { return doorSystem; }

	PuzzleSystem* GetPuzzleSystem() { return puzzleSys; }
	const std::unordered_map<Entity, PuzzleObject>& GetPuzzleObjects() const { return puzzleObjects; }
	MinimapHUD& GetMinimapHUD() { return minimapHUD; }
	const MinimapHUD& GetMinimapHUD() const { return minimapHUD; }

	void ComposeHierarchy();

	void SetWorldBounds(const Vector2& minB, const Vector2& maxB) {
		worldMinBound = minB;
		worldMaxBound = maxB;
	}

	Vector2 GetWorldMinBound() const { return worldMinBound; }
	Vector2 GetWorldMaxBound() const { return worldMaxBound; }

	void SetLabyrinthTileSize(float s) { labyrinthTileSize = s; }
	float GetLabyrinthTileSize() const { return labyrinthTileSize; }

	void SetEnemyGrid(const std::vector<std::string>& grid, float tileSize);

	// Draws the gameplay scene: world + in-game HUD + debug
	void DrawGameplay(Renderer& renderer);

	// restart game 
	void ResetGameplayWorld();
	void ResetPlayerSpeedPresetOutside();

	// ======================================== Player Data & snapshots  ========================================
	// ========================================================================================================== 
	void SetPlayerSpawn(const Vector2& pos);
	Vector2 GetPlayerSpawn() const;
	Vector2 GetPlayerPos() const;
	Vector2 GetWorldMousePosition();

	Vector2 snapPlayerPos = { 0,0 };
	// optional: float snapPlayerOxygen = 0.f; int snapHP = 0; Direction snapFacing = Direction::RIGHT;

	// snapshot helpers
	void CapturePlayerSnapshot();
	void RestorePlayerSnapshot();
	Entity FindPlayer() const;

	// ===============================================  DebugTools  =============================================
	// ========================================================================================================== 
	// --- Outline Debug ---
	friend void DebugDraw::ColliderOutlines(GameApp&, Renderer&, int);

	Messaging::Observable gameEvents{ "GameAppEvents" };


	// ======================================= Utilities & Small Helpers  =======================================
	// ========================================================================================================== 

	// --- audio state handle --- 
	Audio::SoundID bgm = -1;
	Audio::SoundID crateSfxID = -1;

	/**
	* @brief Get a pointer to a mesh stored in the app.
	*/
	Mesh2D* GetMesh(size_t index) override { 
		if (index >= meshes.size() || !meshes[index]) return nullptr;
		
		return meshes[index].get();
	}

	/**
	 * @brief Retrieves the default quad mesh used for general 2D rendering.
	 * @return Pointer to the quad Mesh2D.
	 */
	Mesh2D* GetQuadMesh() const { return meshes[0].get(); }

	Mesh2D* CreateOwnedMesh(){
		meshes.push_back(std::make_unique<Mesh2D>());
		return meshes.back().get();
	}

	Mesh2D* CreateOwnedMeshFor(Entity owner);
	void    DestroyOwnedMeshFor(Entity owner);

	void ClearOwnedMesh() { meshes.clear(); }
	std::string GetEntityNameByEntity(Entity e) const;
	void SetEntityName(Entity e, const std::string& name);
	const std::vector<Entity>& GetEntitiesPublic() const { return GetEntities(); }
	const std::vector<GameObject>& GetAllEntities() const;

	std::string GetEntityDisplayName(Entity e) const override {
		// During shutdown, avoid alloc / late string building
		if (isShuttingDown)
			return "Entity";
		auto it = prefabTags.find(e);
		if (it != prefabTags.end() && !it->second.empty())
			return it->second + " (" + std::to_string(e) + ")";
		return "Entity " + std::to_string(e);
	}

	Camera2D& GetCamera() { return camera; }
	const Camera2D& GetCamera() const { return camera; }

	bool isPlaying = false;
	bool stopRequested = false;
	std::string editScenePath;
	std::string playScenePath; // set on Play, consumed on Stop
	// The scene currently loaded while playing (rooms/labyrinth during Play Mode)
	std::string runtimeScenePath;

	const std::string& GetActiveScenePath() const {
		return isPlaying ? playScenePath : editScenePath;
	}	

	bool IsShuttingDown() const { return isShuttingDown; }

	eng::Layering& GetLayering() { return layering_; }
	//const eng::Layering& GetLayering() const { return layering_; }

	// Allow systems to access projectile storage safely
	std::unordered_map<Entity, ProjectileComponent*>& GetProjectiles() { return projectiles; }
	const std::unordered_map<Entity, ProjectileComponent*>& GetProjectiles() const { return projectiles; }

	// Allow systems to destroy entities without exposing internals
	void DestroyEntityNoExpose(Entity e) { DestroyEntityImmediate(e); }

	const std::unordered_map<Entity, Collider*>& GetAllColliders() const { return colliders; }

	// Lua/Runtime Scripting API
	bool TryGetWorldPos(Entity e, float& outX, float& outY) const override;
	bool SetWorldPos(Entity e, float x, float y) override;
	bool TryGetRotation(Entity e, float& outR) const override;
	bool SetRotation(Entity e, float r) override;
	void SetColliderActive(Entity e, bool active) override;
	void SetAnimationRow(Entity e, int row) override;
	int  GetEnemyHp(Entity e) const override;
	int  GetEnemyMaxHp(Entity e) const override;
	void HealEnemyHp(Entity e, int amount) override;

	Entity GetPlayerEntity() const override;

	void LuaDetachAll();
	void LuaReloadAll();
	float GetTileSize() const override { return aiTileSize_; }

	bool IsBlockedWorld(float wx, float wy) const override
	{
		return AI_IsBlockedWorld(wx, wy);
	}
	bool IsWallWorld(float wx, float wy) const override
	{
		return AI_IsWallWorld(wx, wy);
	}
	bool HasLineOfSightWorld(float ax, float ay, float bx, float by, Entity e) const override
	{
		return AI_HasLOSWorld(ax, ay, bx, by, e, PlayerAbility::BURROW);
	}

	void MoveEntityWorld(Entity e, float vx, float vy, float dt) override
	{
		AI_MoveEntityWorld(e, vx, vy, dt);
	}
	void BroadcastMessage(const std::string& id) override
	{
		Messaging::IMessage msg(id);
		gameEvents.ProcessMessage(&msg);
	}

	// Memory pool setup for the current level.
	// Call before spawning scene objects.
	void ReserveLevelPools(std::size_t expectedSceneObjects);

	// Proof/debug counters
	std::size_t TransformPoolPages() const;
	std::size_t TransformPoolLive() const;
	std::size_t TransformPoolExpansions() const;

	std::size_t ColliderPoolPages() const;
	std::size_t ColliderPoolLive() const;
	std::size_t ColliderPoolExpansions() const;

	std::size_t RendererPoolPages() const;
	std::size_t RendererPoolLive() const;
	std::size_t RendererPoolExpansions() const;

	std::size_t ProjectilePoolPages() const;
	std::size_t ProjectilePoolLive() const;
	std::size_t ProjectilePoolExpansions() const;

	LayerManager& GetLayerManager() { return layerManager_; }
	const LayerManager& GetLayerManager() const { return layerManager_; }

	void RefreshLayerMembership(Entity e); // uses sig bits to pick Background vs World

	const eng::Layering& GetLayering() const override { return layering_; }

	const std::vector<Entity>& GetEntitiesInLayer(eng::LayerId id) const override
	{
		return layerManager_.Entities(id);
	}

	void OnPlayerDeath();
	void OnEnemyDeath(Entity enemy);

	//void RemoveEnemyControllerComponent(Entity e);
	//void checkGamepadStatus();

private:
	friend class ResourceManager;
	friend class DoorSystem;
	friend class PuzzleSystem;
	friend class SceneManager;
	friend class MinimapHUD;
	ProjectileSystem projectileSystem;

	// ============================================================
	// Core Data Member
	// ============================================================
	std::vector<std::unique_ptr<Mesh2D>> meshes;           // multi meshes
	std::unordered_map<Entity, Mesh2D*> entityToOwnedMesh; // Tracks which entity owns which "owned mesh"
	std::unordered_map<Mesh2D*, size_t> ownedMeshIndex;    // for reverse lookup 

	Camera2D camera;             // single camera
	bool isCameraLocked = true;  // mode for navigation camera
	MinimapHUD minimapHUD;
	PlayerHUD playerHUD;

	bool collisionsEnabled = true;

	StressTestManager stressMgr;

	eng::Layering layering_;

	// shader related 
	Shader     shaderDefault;
	Shader     shaderParticles; // shader for particle system (instanced)
	Shader     shaderLight;

	// ECS storage (sparse maps per component)
	std::unordered_map<Entity, Transform*>        transforms;
	eng::mem::FixedBlockPool<Transform, 512> transformPool;
	std::unordered_map<Entity, MeshRenderer*> renderers;
	std::unordered_map<Entity, Collider*>     colliders;
	eng::mem::FixedBlockPool<MeshRenderer, 512> rendererPool;
	eng::mem::FixedBlockPool<Collider, 512>     colliderPool;
	std::unordered_map<Entity, PlayerController>  controllers;
	std::unordered_map<Entity, SpriteAnimator>    animators;
	std::unordered_map<Entity, PersistentTag>     persistentTags;
	std::unordered_map<Entity, EnemiesController> enemiesControllers;
	std::unordered_map<Entity, ParticleEmitter>   emitters;
	std::unordered_map<Entity, LightComponent>    lights;
	std::unordered_map<Entity, PuzzleObject> puzzleObjects;
	std::unordered_map<Entity, ProjectileComponent*> projectiles;
	eng::mem::FixedBlockPool<ProjectileComponent, 512> projectilePool;

	//prefab metadata
	std::unordered_map<Entity, std::string> prefabTags;

	// maze size 
	Vector2 worldMinBound = { 0.f, 0.f };
	Vector2 worldMaxBound = { 0.f, 0.f };
	float labyrinthTileSize = 100.0f;

	// ============================================================
	// IComponentContext: accessors
	// ============================================================
	const std::vector<Entity>& GetEntities()                 const override;
	const SystemManager&	 GetSystemManager()              const override;
	const Transform*		 TryGetTransform(Entity e)       const override;
	const Collider*			 TryGetCollider(Entity e)        const override;
	const MeshRenderer*		 TryGetRenderer(Entity e)        const override;
	const SpriteAnimator*	 TryGetAnimator(Entity e)        const override;
	const PlayerController*	 TryGetController(Entity e)      const override;
	const LightComponent* TryGetLight(Entity e)           const override;
	const ParticleEmitter*	 TryGetEmitter(Entity e)         const override;
	const EnemiesController* TryGetEnemyController(Entity e) const override;
	const SpeedComponent*	 TryGetSpeed(Entity e)           const override;
	const MassComponent*	 TryGetMass(Entity e)            const override;
	const PuzzleObject* TryGetPuzzleObject(Entity e) const;

	// ============================================================
	// Undo/Redo required API from IComponentContext
	// ============================================================
	void DestroyEntityImmediate(Entity e) override;
	Entity CreateEntityWithFixedID(Entity id) override;


	// ============================================================
	// Editor/GUI wiring & app flags
	// ============================================================
#if ENABLE_EDITOR
	std::unique_ptr<EditorOverlay> editorOverlay;
	std::unique_ptr<RoomEditor> roomEditor;
	std::unique_ptr<UiEditor> uiEditor;
#endif
	GuiSystem gui;
	bool shouldQuit = false;
	bool guiVisible = false; // start OFF initially
	bool isAudioPaused = false;

	void ProcessEditorRequests();
	void EditorDoFrame(float dt);

	// ============================================================
	// Undo / Redo
	// ============================================================

	UndoRedoManager* undoRedo = nullptr;

	// ============================================================
	// Scene / Menu management
	// ============================================================
	SceneManager sceneManager;

	GuiSystem     mainMenuGui;
	GuiSystem     pauseGui;
	GuiSystem     settingsGui;
	GuiSystem     controlGui;
	GuiSystem     creditsGui;
	GuiSystem     tutorialGui;
	GuiSystem     confirmGui;
	GuiSystem	  loseGui;

	MainMenu      mainMenu;
	PauseMenu     pauseMenu;
	SettingsMenu  settingsMenu;
	ControlMenu   controlMenu;
	CreditsMenu   creditsMenu;
	TutorialMenu   tutorialMenu;
	ConfirmDialog confirmQuitDialog;
	LoseMenu	  loseMenu;

	bool isPausedByESC = false;


	// ============================================================
	// ECS entity-signature mapping
	// ============================================================

	template <typename T>
	static T* FindComponent(std::unordered_map<Entity, T>& map, Entity e) {
		auto it = map.find(e);
		return (it != map.end()) ? &it->second : nullptr;
	}
	template <typename T>
	static T* FindComponent(std::unordered_map<Entity, T*>& map, Entity e) {
		auto it = map.find(e);
		return (it != map.end()) ? it->second : nullptr;
	}


	std::vector<DoorLink> doorLinks;  // all doors in current level
	bool isTransitioning = false;     // prevent multiple triggers per frame
	std::string previousScene;        // track return point

	DoorSystem doorSystem;

	Vector2 playerSpawn{ 0.f, 0.f };  // set by MapGenerator


	// ============================================================
	// Scripting
	// ============================================================
	std::unordered_map<Entity, ScriptComponent> scriptComponents;

	bool scriptsEnabled = true;

	int windowedWidth = 1400;
	int windowedHeight = 700;

	void RebuildCameraToCurrentFramebuffer(Renderer& renderer);
	bool cameraNeedsRebuild = false;
	bool skipNextDraw = false;



	std::unordered_map<Entity, PrefabInstanceDefaults> prefabDefaults;

	// Undo / Redo helpers
	Vector2 beforeCachePos;
	Vector2 beforeCacheScale;
	float   beforeCacheRot = 0.f;
	Vector2 beforeCacheColliderSize;
	bool    beforeCacheColliderTrigger = false;
	bool    beforeCacheHadCollider = false;

	Messaging::Observer m_gameObserver{ "GameObserver" };

	bool isShuttingDown = false;

	mutable std::vector<Entity> entityIdCache;     // replaces static ids
	std::vector<std::string> scriptNameList;       // replaces static names

	LuaEngine luaEngine;
	std::vector<std::string> aiGrid_;  // current room grid for AI/Lua
	float aiTileSize_ = 100.0f;        // keep in sync with room tile size'

	LayerManager layerManager_;
	// Hotkey state
	bool keyHeldF1 = false;
	bool keyHeldF2 = false;
	bool keyHeldF3 = false;
	bool keyHeldF4 = false;
	bool keyHeldF7 = false;
	bool keyHeldF8 = false;
	bool keyHeldLB = false;
	bool keyHeldRB = false;
	bool keyHeldF6 = false;
	bool keyHeldF5 = false;
	bool keyHeldTab = false;
#ifdef _DEBUG
	bool keyHeldP = false;
#endif
	bool showFPS = false;
	float currentFPS = 0.0f;
	double fpsElapsed = 0.0;
	int fpsFrames = 0;
	// Helpers
	void HandleEditorLayerHotkeys(GLFWwindow* window);
	void HandleGuiToggle(GLFWwindow* window);
	void HandleFullscreenToggle(GLFWwindow* window);
	void UpdateFpsAndSpawnPos(double dt, GLFWwindow* window);
	void HandleFpsToggle(GLFWwindow* window);
	void HandleScriptToggle();
#ifdef _DEBUG
	void HandleDarkeningToggle();
#endif
	void UpdateCameraAndMinimap();
	void HandlePlayerAbilities(double dt);
	void HandleEditorToggle(GLFWwindow* window);
	void HandlePhysicsDebugToggle();
	void UpdateSceneGUIPerState(double dt);
	void DoorUpdateCooldown();
	bool PrepareFrame(Renderer& renderer);
	void DrawSceneAndEditor(Renderer& renderer);
	void HandleMutationInteraction(GameApp& app, Entity player, double dt);

	/**
	 * @brief Initializes collision response callbacks for player-enemy interactions.
	 *
	 * Binds damage application logic to the collision system, including burrow immunity checks.
	 */
	void InitializeCollisionCallback(float dt);
	/**
	 * @brief Registers GUI button actions for common in-game operations.
	 *
	 * Adds buttons for muting audio, exiting the game, and other global UI tasks.
	 */
	void InitializeGuiCallbacks();
	/**
	 * @brief Binds the scene transition request handler for door objects.
	 */
	void InitializeDoorCallback();
	/**
	 * @brief Configures editor-specific hooks and callbacks for room editing.
	 *
	 * Sets up drag-and-drop prefab spawning, grid baking, and undo/redo registration.
	 */
	void InitializeEditorCallbacks();
	/**
	 * @brief Initializes event observation handlers for gameplay state changes.
	 */
	void InitializeGameEventCallbacks();
};
