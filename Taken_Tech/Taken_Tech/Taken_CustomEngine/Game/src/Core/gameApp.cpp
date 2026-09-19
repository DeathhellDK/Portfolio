/**
 * @file      gameApp.cpp
 * @author    Jethro Sung, Woh Kye Le
 * @co_author Tan Wei Liang Terril, Sng Swee Yong Dillon, Low JianLin, Lim Zhi Jie
 * @email     sung.h , w.kyele,sweeyongdillon.sng, t.weiliangterril, jianlin.low, zhijie.lim
 * @date      2025-09-11
 *
 * @brief    Implementation of the GameApp class.
 *
 * This file contains the definitions for all GameApp methods, including:
 * - Initialization of audio, resources, camera, and ECS systems
 * - Per-frame update and draw logic
 * - ImGui debug/editor interface
 * - Entity/component creation and destruction
 * - Stress-test helpers for spawning large numbers of entities
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#undef APIENTRY
#include <Windows.h>
#undef APIENTRY
#ifdef DrawText
#undef DrawText
#endif
static HMODULE hScripts = nullptr;
#endif

// Standard Lib
#include <vector>
#include <memory>
#include <cctype>
#include <random>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>


// External Lib
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// Engine core
#include "Core/gameApp.h"
#include "Core/engine.hpp"
#include "Font/FontRenderer.h"
#include "Core/component.h"
#include "Core/transform.h"
#include "Core/utils.h"
#include "Core/Systems/scriptSystem.h"
#include "Core/assetsPath.h"
#include "Core/randomiser.h"

// Input System
#include "Input/input.h"
#include "Input/DebugConsole.hpp"

// gamePlay
#include "factories.h"
#include "movementsystem.h"
#include "World/mapGenerator.h"
#include "Editor/UndoRedoManager.h"
#include "World/ai.h"

// Combat
#include "Mechanics/interaction.hpp"

#include "Core/Memory/FixedBlockPool.h"


#if ENABLE_EDITOR
#include "Editor/editor.h"
#include "Editor/RoomEditor.h"
#include "Editor/UiEditor.h"
#include "Editor/ImGuiHost.h"
#endif


// -----------------------------------------------------------------------------
// Static member definitions / Helper functions
// -----------------------------------------------------------------------------
namespace fs = std::filesystem;
Entity GameApp::playerEntity = INVALID_ENTITY;
static constexpr size_t MESH_MINIMAP_CIRCLE = 3;

[[maybe_unused]] static bool IsRoomScenePath(const std::string& p) { return p.find("entities_Level") != std::string::npos; }
[[maybe_unused]] static bool IsLabyrinthScenePath(const std::string& p) { return p.find("labyrinth.json") != std::string::npos; }

// -----------------------------------------------------------------------------

void GameApp::HandleEditorLayerHotkeys(GLFWwindow* window) {
	(void)window;
#if ENABLE_EDITOR
	const bool downF3 = (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS);
	if (downF3 && !keyHeldF3) { layering_.ToggleRender(eng::LayerId::World); }
	keyHeldF3 = downF3;
	const bool downF4 = (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS);
	if (downF4 && !keyHeldF4) { layering_.ToggleUpdate(eng::LayerId::World); }
	keyHeldF4 = downF4;
	const bool downF7 = (glfwGetKey(window, GLFW_KEY_F7) == GLFW_PRESS);
	if (downF7 && !keyHeldF7) { layering_.ToggleRender(eng::LayerId::UI); }
	keyHeldF7 = downF7;
	const bool downF8 = (glfwGetKey(window, GLFW_KEY_F8) == GLFW_PRESS);
	if (downF8 && !keyHeldF8) { layering_.ToggleRender(eng::LayerId::Debug); }
	keyHeldF8 = downF8;
	const bool downLB = (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS);
	if (downLB && !keyHeldLB) { layering_.ToggleRender(eng::LayerId::Background); layering_.ToggleUpdate(eng::LayerId::Background); }
	keyHeldLB = downLB;
	const bool downRB = (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS);
	if (downRB && !keyHeldRB) { layering_.ToggleRender(eng::LayerId::World); layering_.ToggleUpdate(eng::LayerId::World); }
	keyHeldRB = downRB;
#endif
}

void GameApp::HandleGuiToggle(GLFWwindow* window) {
	(void)window;
#if ENABLE_EDITOR
	const bool downF2 = (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS);
	if (downF2 && !keyHeldF2) { guiVisible = !guiVisible; DebugConsole::Get().Info(std::string("[GUI] Toggled GUI ") + (guiVisible ? "Visible\n" : "Hidden\n")); }
	keyHeldF2 = downF2;
#endif
}

void GameApp::HandleFullscreenToggle(GLFWwindow* window) {
	const bool downF6 = (glfwGetKey(window, GLFW_KEY_F6) == GLFW_PRESS);
	if (downF6 && !keyHeldF6) { eng::toggleFullscreen(windowedWidth, windowedHeight); cameraNeedsRebuild = true; skipNextDraw = true; }
	keyHeldF6 = downF6;
}

void GameApp::UpdateFpsAndSpawnPos(double dt, GLFWwindow* window) {
	fpsElapsed += dt; fpsFrames++;
	if (fpsElapsed >= 0.5) {
		currentFPS = (float)(fpsFrames / fpsElapsed);
		if (GLFWwindow* win = eng::windowHandle()) {
			std::string title = "Taken";
			glfwSetWindowTitle(win, title.c_str());
		}
		if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
			spawnPos = Vector2(camera.viewportWidth * 0.5f, camera.viewportHeight * 0.5f);
		}
		fpsFrames = 0; fpsElapsed = 0.0;
	}
}

void GameApp::HandleFpsToggle(GLFWwindow* window) {
	(void)window;
	auto& in = eng::input();
	const bool pressed = in.isKeyPressed(GLFW_KEY_TAB);
	if (pressed && !keyHeldTab) {
		showFPS = !showFPS;
		keyHeldTab = true;
	}
	else if (!pressed) {
		keyHeldTab = false;
	}
}

void GameApp::HandleScriptToggle() {
	auto& in = eng::input();
	const bool pressed = in.isKeyPressed(GLFW_KEY_F5);
	if (pressed && !keyHeldF5) {
		scriptsEnabled = !scriptsEnabled;
		DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[Scripts] ", scriptsEnabled ? "Enabled\n" : "Paused\n");
		keyHeldF5 = true;
	}
	else if (!pressed) {
		keyHeldF5 = false;
	}
}

#ifdef _DEBUG
void GameApp::HandleDarkeningToggle() {
	auto& in = eng::input();
	const bool pressed = in.isKeyPressed(GLFW_KEY_L);
	if (pressed && !keyHeldP) {
		if (lightSys) {
			bool current = lightSys->IsDarkeningEnabled();
			lightSys->SetDarkeningEnabled(!current);
		}
		keyHeldP = true;
	}
	else if (!pressed) {
		keyHeldP = false;
	}
}
#endif

void GameApp::UpdateCameraAndMinimap() {
	if (playerEntity != INVALID_ENTITY) {
		if (isCameraLocked) {
			if (Transform* t = GetTransform(playerEntity)) {
				Vector2 playerCenter = t->GetPosition() + t->GetScale() * 0.5f;
				camera.lookAt(playerCenter, worldMinBound, worldMaxBound);
			}
		}
		if (Transform* t = GetTransform(playerEntity)) {
			Vector2 playerCenter = t->GetPosition() + t->GetScale() * 0.5f;
			minimapHUD.SetPlayerWorldPos(playerCenter);
		}
	}
}

void GameApp::HandlePlayerAbilities(double dt) {
	// Only allowability input during active gameplay or tutorial
	if (sceneManager.IsPaused() || sceneManager.IsEditorPaused()) {
		return;
	}
	if (!(sceneManager.IsPlaying() || sceneManager.IsTutorial())) {
		return;
	}
	if (playerEntity != INVALID_ENTITY) {
		Interaction::HandlePlayerAbilityInput(*this, playerEntity, dt);
	}
}

void GameApp::HandleEditorToggle(GLFWwindow* window) {
	(void)window;
#if ENABLE_EDITOR
	const bool downF1 = (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS);
	if (downF1 && !keyHeldF1) {
		sceneManager.ToggleEditorsForActiveScene();
	}
	keyHeldF1 = downF1;
#endif
}

void GameApp::HandlePhysicsDebugToggle() {
	auto& in = eng::input();
	PhysicsDebug::HandleDebugToggle(in);
}

void GameApp::UpdateSceneGUIPerState(double dt) {
	auto& in = eng::input();
	sceneManager.Update(in, dt);
}

void GameApp::DoorUpdateCooldown() {
	doorSystem.UpdateCooldown();
}

bool GameApp::PrepareFrame(Renderer& renderer) {
	if (cameraNeedsRebuild) {
		RebuildCameraToCurrentFramebuffer(renderer);
		cameraNeedsRebuild = false;
	}
	if (skipNextDraw) {
		skipNextDraw = false;
		return true;
	}
	return false;
}

void GameApp::DrawSceneAndEditor(Renderer& renderer) {
#if ENABLE_EDITOR
	const bool usingEditor =
		(editorOverlay && editorOverlay->IsVisible()) ||
		(roomEditor && roomEditor->IsVisible()) ||
		(uiEditor && uiEditor->IsVisible());
#else
	bool usingEditor = false;
#endif
	int fbw = 0, fbh = 0;
	if (GLFWwindow* win = glfwGetCurrentContext()) {
		glfwGetFramebufferSize(win, &fbw, &fbh);
	}
	Utility::BeginGameScene(renderer, usingEditor, fbw, fbh);
	sceneManager.Draw(renderer);

	/// if FPS display is toggled on, display the fps
	if (showFPS) {
		std::stringstream ss;
		ss << "FPS: " << std::fixed << std::setprecision(1) << currentFPS;
		std::string fpsStr = ss.str();

		// use framebuffer size for consistency with GUI
		float x = (float)fbw * 0.5f;
		float y = (float)fbh - 30.0f; // margin from top

		float scale = (float)fbh / 1080.f;
		float scaledSize = 32.0f * scale;

		EngineCore::FontRenderer::DrawText(
			renderer,
			"default",
			scaledSize,
			x,
			y,
			0xFFFFFFFF,
			fpsStr,
			0.0f,
			FontSys::Align::Center
		);
	}

	{
		static bool s_done = false;
		static float s_t = 0.0f;

		if (!s_done) {
			const float dt = static_cast<float>(eng::deltaTime());
			s_t += dt;

			const float total = 6.0f;
			const float fadeIn = 1.0f;
			const float fadeOut = 1.0f;

			float a = 1.0f;
			if (s_t < fadeIn) a = s_t / fadeIn;
			else if (s_t > total - fadeOut) a = (total - s_t) / fadeOut;
			a = std::clamp(a, 0.0f, 1.0f);

			if (s_t >= total) {
				s_done = true;
			}
			else if (a > 0.0f) {
				Camera2D* prevCam = renderer.getCamera();
				renderer.setCamera(nullptr);

				Shader* prevShader = renderer.GetShader();
				Shader* defaultShader = renderer.GetDefaultShader();
				renderer.SetShader(defaultShader);

				if (defaultShader) {
					defaultShader->Use();
					defaultShader->SetInt("u_Frame", 0);
					defaultShader->SetInt("u_Cols", 1);
					defaultShader->SetVec2("u_FrameSize", Vector2(1.0f, 1.0f));
				}
				GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
				glDisable(GL_DEPTH_TEST);

				GLboolean blendEnabled = glIsEnabled(GL_BLEND);
				if (!blendEnabled) glEnable(GL_BLEND);
				GLint srcAlpha, dstAlpha;
				glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
				glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				Mesh2D* quad = GetMesh(1);
				if (quad && fbw > 0 && fbh > 0) {
					const float l = 0.0f, r = static_cast<float>(fbw);
					const float b = 0.0f, t = static_cast<float>(fbh);
					Matrix3x3 ortho = Matrix3x3::Identity();
					ortho(0, 0) = 2.0f / (r - l);
					ortho(1, 1) = 2.0f / (t - b);
					ortho(2, 0) = -(r + l) / (r - l);
					ortho(2, 1) = -(t + b) / (t - b);

					{
						Matrix3x3 bgModel =
							Matrix3x3::BuildTranslation(0.0f, 0.0f) *
							Matrix3x3::BuildScaling(static_cast<float>(fbw), static_cast<float>(fbh));

						renderer.DrawMesh(*quad, ortho * bgModel, Vector3(0.0f, 0.0f, 0.0f) * a, 0);
					}
					if (GLuint tex = ResourceManager::GetTexture("digipenLogo")) {
						const float targetW = static_cast<float>(fbw);
						const float targetH = static_cast<float>(fbh);
						const float x = 0.0f;
						const float y = 0.0f;

						Matrix3x3 model =
							Matrix3x3::BuildTranslation(x, y) *
							Matrix3x3::BuildScaling(targetW, targetH);

						renderer.DrawMesh(*quad, ortho * model, Vector3(a, a, a), tex);
					}
				}

				if (!blendEnabled) glDisable(GL_BLEND);
				glBlendFunc(srcAlpha, dstAlpha);

				if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
				renderer.SetShader(prevShader);
				renderer.setCamera(prevCam);
			}
		}
	}
	Utility::EndGameScene(renderer, usingEditor, fbw, fbh);
#if ENABLE_EDITOR
	sceneManager.DrawEditorFrame(renderer, static_cast<float>(eng::deltaTime()), usingEditor);
#endif
}

/*
* @brief Handles player mutations (Helper functions).
* @param app Reference to the GameApp context.
* @param player Entity ID of the player character.
*/
void GameApp::HandleMutationInteraction(GameApp& app, Entity player, double dt)
{
	(void)app;
	(void)player;
	Interaction::HandleMutationChargeUp(*this, playerEntity, static_cast<float>(dt));
	Interaction::HandleMutationDamage(*this, playerEntity, static_cast<float>(dt));
}

Vector2 GameApp::GetWorldMousePosition() {
#if ENABLE_EDITOR
	if (roomEditor && roomEditor->IsActive()) {
		if (roomEditor->IsMouseInViewport()) {
			return roomEditor->GetWorldMousePosition();
		}
	}

	if (editorOverlay && editorOverlay->IsVisible()) {
		if (editorOverlay->IsMouseInViewport()) {
			return editorOverlay->GetWorldMousePosition();
		}
	}
#endif
	auto& in = eng::input();
	Vector2 mousePos = in.getMousePos();
	mousePos.y = camera.viewportHeight - mousePos.y;
	return camera.ScreenToWorld(mousePos);
}

void GameApp::SavePreviousTransforms() {
	for (auto& [id, tr] : transforms)
		if (tr) tr->SavePreviousState();
}

void GameApp::SimulateFixedStep(float fdt, bool stepAllowed) {
	if (!stepAllowed) return;
	systemManager.UpdateAll(static_cast<float>(fdt));
	ExecuteForceSystem(*this, static_cast<float>(fdt));
}

void GameApp::TickAIAndShooting(double fdt) {
	Vector2 playerPos{ 0,0 };
	if (playerEntity != INVALID_ENTITY) {
		if (Transform* pt = GetTransform(playerEntity)) playerPos = pt->GetPosition();
		PlayerController* pc = GetController(playerEntity);
		if (pc) {
			Vector2 shotDir{};
			if (pc->ConsumeShoot(shotDir)) {
				MakeProjectile(*this, playerEntity, shotDir, false);
			}
		}
	}
	for (auto& kv : enemiesControllers) {
		Entity e = kv.first;
		EnemiesController& ctrl = kv.second;
		if (Transform* t = GetTransform(e)) {
			ctrl.Update(static_cast<float>(fdt), *t, playerPos, *this);
			Vector2 shotDir{};
			if (ctrl.ConsumeShoot(shotDir)) {
				MakeProjectile(*this, e, shotDir, true);
				// Retrieve the enemy's world position for the sound source
				Vector2 ePos = t->GetPosition();

				// Use the enemy's actual shootRange as the maxRoll
				float range = ctrl.GetShootRange();

				//// If the boss has an increased range, the audio will automatically adjust
				//if (ctrl.IsBoss()) {
				//	range *= 1.3f;
				//}

				// Pass 'this' as the GameApp reference to find the player listener
				ResourceManager::PlaySfx(*this, "ranged_mini_boss",
					glm::vec2(ePos.x, ePos.y),
					100.0f, range, 0.7f);
			}
		}
	}
}


void GameApp::TickProjectiles(float fdt) {
	projectileSystem.Update(*this, fdt);
}


void GameApp::TickScripts(float fdt) {
	for (auto& [e, sc] : scriptComponents) {
		ScriptStartIfNeeded(e);
		ScriptUpdate(e, fdt);
	}
}


void GameApp::TickAnimations(float fdt) {
	if (animationSys) {
		animationSys->Update(fdt);
	}
}



void GameApp::UpdateDoorsFixed() {
	doorSystem.HandlePendingSpawn(*this);
	doorSystem.CheckDoorTransitions(*this);
	doorSystem.UpdateDoorTextures(*this);
}



// ============================== MAIN API (APP LIFECYCLES) ===============================
// =====================
/**
 * @brief Construct a new GameApp instance.
 *
 * Initializes the orthographic camera using a default viewport size (1400x700).
*/
GameApp::GameApp() : camera(1400.0f, 700.0f) {} // Initialize camera with window size

GameApp::~GameApp() = default;

/**
	* @brief Initializes all core engine subsystems, resources, ECS systems,
	*        GUI, editor overlay, and gameplay dependencies.
	*
	* Responsibilities include:
	*  - Load and compile all shaders (default, alt, instanced)
	*  - Bootstrap audio engine (FMOD wrapper)
	*  - Initialize textures, meshes, prefabs, config, and resource paths
	*  - Build default quad/circle/outline meshes
	*  - Initialize ECS systems: movement, collision, animation, particles, rendering, scripting
	*  - Set up minimap HUD, stress-test manager, and player spawn defaults
	*  - Create GUI buttons for Mute/Exit and build all main/pause menu layouts
	*  - Initialize Undo/Redo manager and hook editor callbacks (transform, entity create/delete)
	*  - Register gameplay message observers for enemy-AI notifications
	*
	* @param renderer Reference to the Renderer used for all draw calls.
	* @return true if initialization succeeds, false if any shader/audio/resource fails to load.
	*
*/
bool GameApp::Initialize(Renderer& renderer, float dt) {

	// --- loading shader ---
	if (!renderer.Initialize(ShaderPath("shader.vert"), ShaderPath("shader.frag"),
		ShaderPath("shader_instanced.vert"), ShaderPath("shader_instanced.frag"))) {
		return false;
	}

	renderer.SetShader(renderer.GetDefaultShader());

	if (!shaderLight.LoadFromFile(ShaderPath("shader.vert"), ShaderPath("light.frag"))) {
		return false;
	}

	// --- Initialise audio data ---
	AudioBootstrap audioBoot{ 60000, 2, 256, "", 0.8f, true };

	// --- Initialise Assets ---
	if (!ResourceManager::Init(&audioBoot)) { return false; }


	// --- default config ---
	const Config& cfg = ResourceManager::GetConfig();

	// Remember the windowed resolution from config
	windowedWidth = cfg.width;
	windowedHeight = cfg.height;

	// --- Per-vertex colors for all quads ---
	std::vector<Vector3> colors = cfg.colors;

	RebuildCameraToCurrentFramebuffer(renderer);

	spawnPos = Vector2(
		camera.viewportWidth * 0.5f,
		camera.viewportHeight * 0.5f
	);

	// --- different meshes (tho will need to clean up those not in use) ---
	meshes.push_back(std::make_unique<Mesh2D>(Utility::BuildQuad({ {0,0}, {1,0}, {1,1}, {0,1} }, colors)));               // Full square 
	meshes.push_back(std::make_unique<Mesh2D>(Utility::BuildQuad({ {0,0}, {1,0}, {1,1}, {0,1} }, colors)));               // Small square (also unit, Transform will scale)
	meshes.push_back(std::make_unique<Mesh2D>(Utility::BuildQuad({ {0,0}, {1,0}, {1,0.66f}, {0,0.66f} }, colors)));       // Shifted rectangle (1.0.66 in local space)
	meshes.push_back(std::make_unique<Mesh2D>(Utility::BuildCircle(48, Vector3(1.f, 1.f, 1.f), true)));				      // black circle mesh 
	meshes.push_back(std::make_unique<Mesh2D>(Utility::BuildRectOutline()));                                              // OutLine mesh

	// --- telling minimap HUD what mesh to use ---
	minimapHUD.SetCamera(&camera);
	minimapHUD.SetMesh(meshes[MESH_MINIMAP_CIRCLE].get());

	// --- set radius of fog ---
	minimapHUD.SetDiscoveryRadius(600.0f);
	minimapHUD.SetVisibleRange(2000.0f);

	prefabManager.LoadPrefabs(AssetPath("prefabs.json"));

	editScenePath = "scene/labyrinth.json";
	runtimeScenePath.clear();
	playScenePath.clear();


	if (cfg.enableMovement) { moveSys = &systemManager.AddSystem<MovementSystem>(*this); }
	if (cfg.enableAnimation) { animationSys = &systemManager.AddSystem<AnimationSystem>(*this); }
	if (cfg.enableEmitter) { particleSys = &systemManager.AddSystem<ParticleSystem>(*this); }
	if (cfg.enableRender) { renderSys = &systemManager.AddSystem<RenderSystem>(*this); }
	if (cfg.enableLight) { lightSys = &systemManager.AddSystem<LightSystem>(*this); }
	systemManager.AddSystem<ScriptSystem>(*this);
	if (cfg.enableCollision) {
		collisionSys = &systemManager.AddSystem<CollisionSystem>(*this, g_entityForceProxy);
		InitializeCollisionCallback(dt);
	}
	puzzleSys = &systemManager.AddSystem<PuzzleSystem>(*this);

	// --- Initialize Undo/Redo Manager ---
	undoRedo = new UndoRedoManager(*this);

	// hand the shader to the particle system once
	if (particleSys) particleSys->SetShader(renderer.GetParticleShader());
	if (lightSys) lightSys->SetShader(&shaderLight);

	// --- Setup GUI buttons ---
	InitializeGuiCallbacks();

	// -- intialise UI menus ---
	sceneManager.Attach(this, &mainMenu, &mainMenuGui, &pauseMenu, &pauseGui, &loseMenu, &loseGui, &settingsMenu, &settingsGui, &controlMenu, &controlGui, &creditsMenu, &creditsGui, &tutorialMenu, &tutorialGui, &confirmQuitDialog, &confirmGui);
	sceneManager.Init(this);

	InitializeDoorCallback();

	scriptNameList = { "None", "LuaSpin", "LuaChasePlayer", "LuaBasic", "LuaBurrow", "LuaHeal", "LuaRanged", "LuaBless", "LuaBoss", "LuaChanneler" };

#if ENABLE_EDITOR

	editorOverlay = std::make_unique<EditorOverlay>(*this);
	roomEditor = std::make_unique<RoomEditor>();
	uiEditor = std::make_unique<UiEditor>();

	roomEditor->Init();
	InitializeRoomEditor();

	uiEditor->Init();
	InitializeUiEditor();

	editorOverlay->SetCamera(&camera); // A pointer to active camera (a way for editor to get cam)

	if (!editScenePath.empty()) editorOverlay->currentLevelPath = editScenePath;

	InitializeEditorCallbacks();
#endif

	InitializeGameEventCallbacks();

	luaEngine.BindAPI(*this);

	return true;
}


/**
	* @brief Variable-timestep per-frame update responsible for real-time logic.
	*
	* Responsibilities:
	*  - Window title + FPS update
	*  - Global hotkeys (F1: editor, F2: GUI, F5: scripts, F6: fullscreen)
	*  - ESC pause/resume logic
	*  - Shader switching (K key)
	*  - Stress-test toggle (O key)
	*  - Camera follow if locked to player
	*  - Update minimap player marker
	*  - Update resource manager (audio, textures)
	*  - Toggle debug physics rendering
	*  - Update HUD GUI & pause menu based on SceneManager state
	*  - Update sword attachment transform (player weapon)
	*  - Handle Quit button (Exit)
	*
	* @param dt Delta-time in seconds since the previous frame.
*/
void GameApp::Update(double dt) {
	GLFWwindow* window = glfwGetCurrentContext();
	static bool f1Pressed = false; // variable to toggle imgui 
	const bool simPaused = sceneManager.IsGameplaySimPaused();
#if ENABLE_EDITOR
	HandleEditorLayerHotkeys(window);
	ProcessEditorRequests();
#endif
	UpdateFpsAndSpawnPos(dt, window);

	// --- IMGUI toggle on and off --- 

//#endif

	//checkGamepadStatus();

	HandleEditorToggle(window);

	HandleGuiToggle(window);

	HandleFullscreenToggle(window);

	const bool worldUpdateEnabled = layering_.UpdateEnabled(eng::LayerId::World);

	// --- updating all assets --- 
	ResourceManager::Update();

	if (!simPaused && worldUpdateEnabled) {
		UpdateCameraAndMinimap();
		HandlePlayerAbilities(dt);

		// --- SPATIAL TORCH AUDIO LOOP ---
		for (auto& [e, tPtr] : transforms) {
			auto it = prefabTags.find(e);
			if (it != prefabTags.end() && it->second == "Torch") {
				Vector2 torchPos = tPtr->GetPosition();

				// Pass 'e' so the manager can track this specific torch's voice
				ResourceManager::PlayLoopSfx(*this, e, "fire",
					glm::vec2(torchPos.x, torchPos.y),
					75.0f, 190.0f, 0.57f);
			}
		}

		/// Handle mutation interactions (charge-up and damage application) for the player character.
		HandleMutationInteraction(*this, playerEntity, dt);

		HandlePhysicsDebugToggle();

		DoorUpdateCooldown();

	}
	/// --- toggle showing FPS with Tab ---
	HandleFpsToggle(window);
	HandleScriptToggle();
#ifdef _DEBUG
	HandleDarkeningToggle();
#endif

	// ---  update GUI per scene state---
	UpdateSceneGUIPerState(dt);

	if (shouldQuit) {
		DebugConsole::Get().Info("[GameApp] Quit requested, closing window...\n");
		GLFWwindow* win = glfwGetCurrentContext();
		if (win)
			glfwSetWindowShouldClose(win, GLFW_TRUE);
	}
}

/**
	* @brief Deterministic fixed-timestep simulation update.
	*
	* Called 0–N times per frame by the fixed-step loop to maintain
	* uniform physics and AI behavior regardless of framerate.
	*
	* Responsibilities:
	*  - Run PhysicsDebug gating & step-mode logic
	*  - Compose transform hierarchy before simulation
	*  - Update ECS systems (movement, collision, animation, particles)
	*  - Apply force system
	*  - Tick enemy AI using fixed timestep
	*  - Handle door transitions and pending spawns
	*
	* @param fdt Fixed delta-time in seconds.
*/
void GameApp::FixedUpdate(double fdt) {

#if ENABLE_EDITOR
	if (!sceneManager.IsEditorPlaying()) return;
	if (sceneManager.IsEditorPaused()) return;
#endif

	// if pauseMenu stop update 
	if (sceneManager.IsGameplaySimPaused())
		return;

	if (!layering_.UpdateEnabled(eng::LayerId::World))
		return;

	SavePreviousTransforms();

	auto& in = eng::input();

	// --- Update controllers n collision ---
	PhysicsDebug::HandleStepKeyInput(in);
	bool stepAllowed = PhysicsDebug::BeginFrameAllowStep();

	// --- Update transform hierarchy before simulation ---
	ComposeHierarchy();

	TickAIAndShooting(fdt);

	SimulateFixedStep(static_cast<float>(fdt), stepAllowed);

	if (stepAllowed) {
		projectileSystem.Update(*this, static_cast<float>(fdt));
		// DO NOT DELETE THIS LINE OF CODE, UNCOMMENT THIS FOR LOSE MENU IMPLEMENTATION 
		// RIGHT NOW ITS COMMENTED FOR USER TESTING PURPOSES

		if (playerEntity != INVALID_ENTITY) {
			Interaction::healthcontrol(*this, playerEntity);
		}
	}

	// player-bullet vs enemy damage handled inside ProjectileSystem::Update now

	UpdateDoorsFixed();

	//if (stepAllowed && scriptsEnabled)
	//{
	//	// Start scripts once + update every fixed tick
	//	for (auto& [e, sc] : scriptComponents)
	//	{
	//		ScriptStartIfNeeded(e);
	//		ScriptUpdate(e, static_cast<float>(fdt));
	//	}
	//}

	PhysicsDebug::EndFrameStop();
}

/**
	* @brief Renders the entire game frame including world, GUI, editor overlay, and debug layers.
	*
	* Responsibilities:
	*  - Rebuild camera on framebuffer resize or fullscreen change
	*  - Skip one frame after resolution change to avoid flicker
	*  - Select active shader (default/alt)
	*  - Render SceneManager scenes via off-screen game viewport
	*  - If editor enabled, draw editor overlay panels and gizmos
	*  - Provide rendered scene texture to editor viewport (ImGui)
	*
	* @param renderer Rendering interface used to issue draw calls.
*/
void GameApp::Draw(Renderer& renderer) {
	if (PrepareFrame(renderer)) return;
	renderer.SetShader(renderer.GetDefaultShader());
	DrawSceneAndEditor(renderer);
}

/**
 * @brief Shutdown and release all game resources.
 *
 * Frees meshes, components, and ECS data, then tears down
 * all resource and audio systems via the ResourceManager.
*/
void GameApp::Shutdown() {

	isShuttingDown = true;

	//gameObjects.clear();
	meshes.clear();
	for (auto& [id, tr] : transforms)
		transformPool.Destroy(tr);
	transforms.clear();
	for (auto& [id, re] : renderers)
		rendererPool.Destroy(re);
	renderers.clear();
	for (auto& [id, co] : colliders)
		colliderPool.Destroy(co);
	colliders.clear();
	controllers.clear();
	animators.clear();
	persistentTags.clear();
	enemiesControllers.clear();
	emitters.clear();
	scriptComponents.clear();
	prefabTags.clear();
	prefabDefaults.clear();
	doorLinks.clear();
	puzzleObjects.clear();
	for (auto& [id, p] : projectiles)
		projectilePool.Destroy(p);
	projectiles.clear();
	ShutdownForceSystem();

	// Destroy undo/redo manager (heap object)
	if (undoRedo) {
		delete undoRedo;
		undoRedo = nullptr;
	}

#if ENABLE_EDITOR
	editorOverlay.reset();
	roomEditor.reset();
	uiEditor.reset();
	ImGuiHost::Get().Release();
#endif
	ResourceManager::Shutdown();
	entityIdCache.clear();
	entityIdCache.shrink_to_fit();

	scriptNameList.clear();
	scriptNameList.shrink_to_fit();
}
// =====================
// =======================================================================================

// ============================================================
// IComponentContext: Undo/Redo helpers
// ============================================================

/**
	* @brief Immediately removes an entity from all ECS component storages.
	*
	* Deletes all associated components:
	*  Transform, Renderer, Collider, Controller, Animator, ScriptComponent,
	*  PersistentTag, EnemyController, Emitter, PrefabTag.
	*
	* Also informs EntityManager to reclaim the entity ID.
	*
	* @param e Entity to destroy.
*/
void GameApp::DestroyEntityImmediate(Entity e)
{
	// Remove from ECS storage
	if (auto it = transforms.find(e); it != transforms.end()) {
		transformPool.Destroy(it->second);
		transforms.erase(it);
	}
	if (auto it = renderers.find(e); it != renderers.end()) {
		rendererPool.Destroy(it->second);
		renderers.erase(it);
	}

	if (auto it = colliders.find(e); it != colliders.end()) {
		colliderPool.Destroy(it->second);
		colliders.erase(it);
	}
	controllers.erase(e);
	animators.erase(e);
	persistentTags.erase(e);
	enemiesControllers.erase(e);
	emitters.erase(e);
	scriptComponents.erase(e);
	puzzleObjects.erase(e);
	temp_spdComponent.erase(e);
	temp_massComponent.erase(e);
	if (auto it = projectiles.find(e); it != projectiles.end()) {
		projectilePool.Destroy(it->second);
		projectiles.erase(it);
	}

	// Remove prefab info if exists
	prefabTags.erase(e);

	layerManager_.Remove(e);

	// Inform EntityManager
	entityManager.DestroyEntity(e);

	// Clean up boss corpse status if applicable
	Interaction::RemoveBossCorpse(e);
}

/**
 * @brief Creates an entity using a fixed ID (used for Undo/Redo restoration).
 *
 * Ensures that nextId never reuses a restored ID. Inserts an empty
 * GameObject record and initializes its signature.
 */
Entity GameApp::CreateEntityWithFixedID(Entity id)
{
	// Forward to EntityManager
	entityManager.CreateEntityWithFixedID(id);

	layerManager_.Add(eng::LayerId::Background, id);

	// Ensure transform always exists
	Transform* t = transformPool.Create(id);
	transforms[id] = t;

	//make sure signature reflects that Transform exists
	Signature sig = entityManager.GetSignature(id);
	sig.set(TRANSFORM);
	SetSignature(id, sig);

	return id;
}


// ================================== HELPER/UTILITIES ===================================
// =====================

/**
 * @brief Creates a new Mesh2D owned by a specific entity.
 * 
 * The mesh is stored in the main meshes vector but tracked via a map
 * so it can be easily destroyed when the entity is destroyed.
 * 
 * @param owner The entity that will own this mesh.
 * @return Pointer to the newly created Mesh2D.
 */
Mesh2D* GameApp::CreateOwnedMeshFor(Entity owner) {
	meshes.push_back(std::make_unique<Mesh2D>());
	Mesh2D* ptr = meshes.back().get();
	entityToOwnedMesh[owner] = ptr;
	ownedMeshIndex[ptr] = meshes.size() - 1;

	return ptr;
}

/**
 * @brief Destroys the Mesh2D owned by the specified entity.
 * 
 * Removes the mesh from the main meshes vector and the tracking maps.
 * Handles the swap-and-pop logic to keep the vector contiguous.
 * 
 * @param owner The entity whose owned mesh should be destroyed.
 */
void GameApp::DestroyOwnedMeshFor(Entity owner) {
	auto it = entityToOwnedMesh.find(owner);
	if (it == entityToOwnedMesh.end())
		return;

	Mesh2D* meshPtr = it->second;
	entityToOwnedMesh.erase(it);

	auto idxIt = ownedMeshIndex.find(meshPtr);
	if (idxIt == ownedMeshIndex.end())
		return;

	size_t idx = idxIt->second;
	ownedMeshIndex.erase(idxIt);

	// swap-remove from meshes vector 
	size_t last = meshes.size() - 1;
	if (idx != last) {
		std::swap(meshes[idx], meshes[last]);

		// update index map for the swapped mesh
		Mesh2D* swappedPtr = meshes[idx].get();
		ownedMeshIndex[swappedPtr] = idx;
	}

	meshes.pop_back();
}

/**
	* @brief Saves the current level either as labyrinth.txt, room entities_LevelX.txt,
	*        or as a JSON scene depending on path.
	*
	* For labyrinth.json:
	*  - Saves procedural maze layout.
	*
	* For room scenes (Level1–Level5):
	*  - Writes a TXT representation used for procedural room generation.
	*
	* Otherwise:
	*  - Performs a normal JSON scene save.
	*
	* @param path Output path req by editor or runtime.
*/
void GameApp::SaveLevel(const std::string& path)
{
	(void)path;
#if ENABLE_EDITOR
	//ONLY saves objects created via the editor spawner panel.
	SceneLoader::SaveLevel(*this, path, &spawnedEntities);
	DebugConsole::Get().Info("[GameApp] Saved ONLY spawned entities to " + path + "\n");
#endif
}

/**
 * @brief Removes the prefab tag from an entity if it exists.
 * @param e The entity to check and remove prefab tag from.
 */
void GameApp::ClearPrefabTagIfAny(Entity e) {
	auto it = prefabTags.find(e);
	if (it != prefabTags.end())
		prefabTags.erase(it);
}

/**
 * @brief Destroys all entities that are not marked as persistent.
 *
 * Removes all entities that have a Transform component but are not
 * in the persistent tags set. Also cleans up prefab tag metadata.
 */
void GameApp::DestroyAllNonPersistentEntities() {
	std::unordered_set<Entity> doomed;

	// Collect ALL active entities
	for (const auto& obj : entityManager.GetEntities()) {
		Entity id = obj.id;
		if (persistentTags.find(id) == persistentTags.end())
			doomed.insert(id);
	}

	// clear prefab tag metadata first (optional but keeps things clean)
	for (Entity e : doomed)
		ClearPrefabTagIfAny(e);

	RemoveEntities(doomed);
}

/**
	* @brief Handles pending spawn/delete operations created by the in-editor UI.
	*
	* Supports:
	*  - Spawning generic objects
	*  - Spawning player entities
	*  - Assigning textures to new objects
	*  - Recording Undo/Redo events
	*  - Processing batch entity deletions
	*
	* @note Called ONLY when editor overlay exists and not in Play mode.
*/
void GameApp::ProcessEditorRequests() {
#if ENABLE_EDITOR
	if (!editorOverlay) return;

	// --- Handle requests from the editor ---
	const SpawnRequest& req = editorOverlay->GetSpawnRequest();
	if (req.type != SpawnRequest::Type::None) {
		Mesh2D* mesh = GetMesh(1);
		const Vector3 white(1.f, 1.f, 1.f);
		const Vector2 unit(1.f, 1.f);

		if (req.type == SpawnRequest::Type::Object) {
			Vector2 centeredPos = req.pos - (req.scale * 0.5f);
			Entity e = MakeGameObject(*this,
				"Obj_" + std::to_string(spawnedEntities.size()),
				mesh, centeredPos, req.scale,
				req.rotation * 0.0174532925f,  // deg -> rad
				white, ColliderType::Box, false, unit);

			if (GLuint texID = ResourceManager::GetTexture(req.textureName); texID) {
				if (auto* mr = GetRenderer(e)) mr->SetTexture(texID);
			}
			else {
				DebugConsole::Get().Error("Texture " + req.textureName + "' not found\n");
			}

			if (editorOverlay && editorOverlay->onRecordEntityCreated) {
				editorOverlay->onRecordEntityCreated(e);
			}

			spawnedEntities.push_back(e);
			selectedEntity = (int)spawnedEntities.size() - 1;
		}
		else if (req.type == SpawnRequest::Type::Player) {
			Vector2 centeredPos = req.pos - (req.scale * 0.5f);
			Entity e = MakePlayer(*this,
				"Player_" + std::to_string(spawnedEntities.size()),
				mesh, centeredPos, req.scale,
				req.rotation * 0.0174532925f,
				white, unit);

			if (editorOverlay && editorOverlay->onRecordEntityCreated) {
				editorOverlay->onRecordEntityCreated(e);
			}

			spawnedEntities.push_back(e);
			selectedEntity = (int)spawnedEntities.size() - 1;
		}
		else if (req.type == SpawnRequest::Type::Prefab)
		{
			Entity e = InstantiatePrefab(req.prefabName, req.pos);

			if (e != INVALID_ENTITY)
			{
				if (auto* t = GetTransform(e))
				{
					t->SetPosition(req.pos);
					t->SetScale(req.scale);
					t->SetRotation(req.rotation);
				}

				if (editorOverlay && editorOverlay->onRecordEntityCreated) {
					editorOverlay->onRecordEntityCreated(e);
				}

				spawnedEntities.push_back(e);
				selectedEntity = (int)spawnedEntities.size() - 1;
			}
		}

		editorOverlay->ClearSpawnRequest();
	}

	const auto& destroys = editorOverlay->GetPendingDestroyList();
	if (!destroys.empty()) {
		for (auto e : destroys) {
			DestroyEntity(e);
			spawnedEntities.erase(
				std::remove(spawnedEntities.begin(), spawnedEntities.end(), e),
				spawnedEntities.end());
		}
		editorOverlay->ClearPendingDestroyList();
	}
#endif
}

/**
	* @brief Runs one full ImGui editor frame (Begin -> Draw -> End).
	*
	* If the editor is visible:
	*  - Starts a new ImGui frame
	*  - Renders all editor panels and gizmos
	*  - Ends the frame and submits draw calls
	*  - Executes pending entity operations
	*
	* @param dt Delta time for editor timing and statistics.
*/
void GameApp::EditorDoFrame(float dt) {
	(void)dt;


#if ENABLE_EDITOR
	if (!ImGuiHost::Get().IsReady())
		return;

	const bool anyEditorVisible =
		(editorOverlay && editorOverlay->IsVisible()) ||
		(roomEditor && roomEditor->IsVisible()) ||
		(uiEditor && uiEditor->IsVisible());

	if (!anyEditorVisible)
		return;

	ImGuiHost::Get().BeginFrame();

	if (editorOverlay && editorOverlay->IsVisible())
		editorOverlay->Draw(dt);

	if (roomEditor && roomEditor->IsVisible()) {
		roomEditor->Update(dt);
		roomEditor->Draw();
	}

	if (uiEditor && uiEditor->IsVisible()) {
		uiEditor->Update(dt);
		uiEditor->Draw();
	}

	ImGuiHost::Get().EndFrame();
#endif
}

#if ENABLE_EDITOR
/**
 * @brief Synchronizes the visible editor tool with the currently active scene type.
 * 
 * Automatically switches between RoomEditor, EditorOverlay (Labyrinth), and UiEditor
 * based on the file path of the current scene.
 */
void GameApp::SyncEditorToActiveScene() {
	if (!roomEditor || !editorOverlay || !uiEditor) return;

	const std::string& sp = sceneManager.GetActiveScenePath();
	const bool inRoom = (sp.find("entities_Level") != std::string::npos);
	const bool inLab = (sp.find("labyrinth.json") != std::string::npos);
	const bool inUi = (sp.find("Menu.json") != std::string::npos) ||
		(sp.find("menu.json") != std::string::npos) ||
		(sp.find("Dialog.json") != std::string::npos) ||
		(sp.find("dialog.json") != std::string::npos) ||
		(sp.find("tut") != std::string::npos);

	const bool wantAnyEditor = roomEditor->IsVisible() || editorOverlay->IsVisible() || uiEditor->IsVisible();
	if (!wantAnyEditor) return; // user didn't have editor open, do nothing

	if (inRoom) {
		// switch to RoomEditor
		roomEditor->SetVisible(true);
		editorOverlay->SetVisible(false);
		uiEditor->SetVisible(false);
	}
	else if (inLab) {
		// switch to EditorOverlay
		editorOverlay->SetVisible(true);
		roomEditor->SetVisible(false);
		uiEditor->SetVisible(false);
	}
	else if (inUi) {
		// switch to UiEditor
		uiEditor->SetVisible(true);
		roomEditor->SetVisible(false);
		editorOverlay->SetVisible(false);
	}
	else {
		// unknown scene type: hide both (or pick one)
		roomEditor->SetVisible(false);
		editorOverlay->SetVisible(false);
		uiEditor->SetVisible(false);
	}
}
#endif

/**
	* @brief Rebuilds the EditorOverlay from scratch, rebinding all callbacks,
	*        undo/redo hooks, transform recorders, and level-change events.
	*
	* Used whenever:
	*  - A scene is reloaded
	*  - A Play/Stop transition happens
	*  - Editor needs full ECS reattachment
	*
	* @note Ensures the editor->currentLevelPath always matches editScenePath.
*/
void GameApp::RecreateEditorOverlay() {
#if ENABLE_EDITOR
	const bool wasPlaying = isPlaying;
	const bool wasPaused = (editorOverlay && editorOverlay->IsPaused());

	// If we were in play, this is the runtime level we r actually in.
	// If we were stopped, this is the edit level.
	const std::string carriedPath = wasPlaying ? (!runtimeScenePath.empty() ? runtimeScenePath : (!playScenePath.empty() ? playScenePath : editScenePath)) : editScenePath;

	if (editorOverlay) {
		editorOverlay.reset();
	}

	editorOverlay = std::make_unique<EditorOverlay>(*this);
	editorOverlay->SetCamera(&camera);

	InitializeEditorCallbacks();

	//editorOverlay->onRequestCameraLock = [this](bool lock) {
	//	isCameraLocked = lock;
	//	};

	//// Keep the last edited scene path consistent in the UI
	//if (!editScenePath.empty())
	//	editorOverlay->currentLevelPath = editScenePath;

	//editorOverlay->onLevelChange = [this](const std::string& path) { sceneManager.EnqueueEditorCmd({ SceneManager::EditorCmdType::ChangeLevel, path }); };

	////editorOverlay->onLevelChange = [this](const std::string& path) {
	////	sceneManager.EnqueueEditorCmd({ SceneManager::EditorCmdType::ChangeLevel, path });
	////	
	////	// If editor is paused, Update() may not commit loads this frame.
	////	// Force commit immediately so the scene appears right away.
	////	sceneManager.CommitEditorChangesNow();
	////	};
	//editorOverlay->onLevelSave = [this](const std::string& path) { SaveLevel(path); };

	//auto cb = sceneManager.GetEditorPlayCallbacks();
	//editorOverlay->onPlay = cb.onPlay;
	//editorOverlay->onPause = cb.onPause;
	//editorOverlay->onStop = cb.onStop;

	//editorOverlay->isPrefabInstance = [this](Entity e) { return prefabTags.find(e) != prefabTags.end(); };
	//editorOverlay->onPrefabRevertInstance = [this](Entity e) { RevertInstanceToPrefab(e); };
	//editorOverlay->onPrefabApplyFromInstance = [this](Entity e) { ApplyInstanceAsPrefab(e); };
	//editorOverlay->onPrefabSaveAll = [this]() { SavePrefabsToJson(); };

	//Utility::BindEditorOverlay(*this, *editorOverlay);

	//if (roomEditor) {
	//	auto playCb = sceneManager.GetEditorPlayCallbacks();
	//	roomEditor->SetPlayControls(
	//		[this]() { return sceneManager.GetPlayControlsState(); },
	//		playCb
	//	);
	//}

	//// ---------------------------------------------------------------
	//// Attach Undo / Redo buttons from EditorOverlay to UndoRedoManager
	//// ---------------------------------------------------------------
	//editorOverlay->onUndo = [this]() {
	//	if (undoRedo)
	//		undoRedo->Undo();
	//	};

	//editorOverlay->onRedo = [this]() {
	//	if (undoRedo)
	//		undoRedo->Redo();
	//	};
	//// ======================================================================
	//// UNDO / REDO TRANSFORM & ENTITY RECORDING HOOKS
	//// ======================================================================

	//// Called BEFORE the editor begins modifying a transform
	//editorOverlay->onRecordBeforeTransform = [this](Entity e) {
	//	const bool allowRecord = (!sceneManager.IsEditorPlaying()) || sceneManager.IsEditorPaused();

	//	if (!allowRecord) return;
	//	if (!undoRedo) return;

	//	if (Transform* t = GetTransform(e)) {
	//		beforeCachePos = t->GetPosition();
	//		beforeCacheScale = t->GetScale();
	//		beforeCacheRot = t->GetRotation();
	//	}
	//	};

	//// Called AFTER the editor finishes modifying a transform
	//editorOverlay->onRecordAfterTransform = [this](Entity e) {
	//	const bool allowRecord = (!sceneManager.IsEditorPlaying()) || sceneManager.IsEditorPaused();

	//	if (!allowRecord) return;
	//	if (!undoRedo) return;

	//	if (Transform* t = GetTransform(e)) {
	//		undoRedo->Push_TransformUpdate(
	//			e,
	//			beforeCachePos,
	//			t->GetPosition(),
	//			beforeCacheScale,
	//			t->GetScale(),
	//			beforeCacheRot,
	//			t->GetRotation()
	//		);
	//	}
	//	};

	//// Record entity create
	//editorOverlay->onRecordEntityCreated = [this](Entity e) {
	//	const bool allowRecord = (!sceneManager.IsEditorPlaying()) || sceneManager.IsEditorPaused();

	//	if (!allowRecord) return;
	//	if (undoRedo)
	//		undoRedo->Push_EntityCreated(e);
	//	};

	//// Record entity deletion
	//editorOverlay->onRecordEntityDeleted = [this](Entity e) {
	//	const bool allowRecord = (!sceneManager.IsEditorPlaying()) || sceneManager.IsEditorPaused();

	//	if (!allowRecord) return;
	//	if (!undoRedo) return;

	//	if (Transform* t = GetTransform(e)) {
	//		undoRedo->Push_EntityDeleted(
	//			e,
	//			t->GetPosition(),
	//			t->GetScale(),
	//			t->GetRotation()
	//		);
	//	}
	//	else {
	//		undoRedo->Push_EntityDeleted(e, { 0,0 }, { 1,1 }, 0.0f);
	//	}
	//	};

	//editorOverlay->undoRedoPtr = undoRedo;

	editorOverlay->currentLevelPath = carriedPath;

	// Preserve the play/pause UI state across rebuilds
	editorOverlay->SetIsPlaying(wasPlaying);
	editorOverlay->SetIsPaused(wasPlaying ? wasPaused : false);

	DebugConsole::Get().Info("[GameApp] EditorOverlay recreated and rebound to current ECS context.\n");
#endif
}

// =====================
// =======================================================================================

/**
	* @brief Attaches a script instance to an entity.
	*
	* Marks the script as not started so ScriptSystem can call OnStart().
	*
	* @param e Entity receiving a script.
	* @param script Unique pointer to script instance.
*/
void GameApp::AddScript(Entity e, std::unique_ptr<IScript> script)
{
	auto& sc = scriptComponents[e];
	sc.backend = ScriptBackend::NativeCpp;
	sc.kind = ScriptKind::None; // optional
	sc.native = std::move(script);
	sc.started = false;
}


/**
	* @brief Returns a list of all entities containing a ScriptComponent.
	*
	* @return Vector of entity IDs.
*/
std::vector<Entity> GameApp::GetScriptedEntities() const {
	std::vector<Entity> out; out.reserve(scriptComponents.size());
	for (auto& kv : scriptComponents) out.push_back(kv.first);
	return out;
}

/**
	* @brief Calls OnStart() exactly once for a script, then enables OnUpdate().
	*
	* @param e Entity to process.
*/
void GameApp::ScriptStartIfNeeded(Entity e)
{
	auto it = scriptComponents.find(e);
	if (it == scriptComponents.end()) return;
	auto& sc = it->second;

	if (sc.backend == ScriptBackend::NativeCpp)
	{
		if (!sc.native) return;
		if (!sc.started) { sc.native->OnStart(*this, e); sc.started = true; }
		return;
	}

	if (sc.backend == ScriptBackend::Lua)
	{
		if (!sc.started)
		{
			// lazy-load script if needed
			if (sc.luaUpdateRef < 0) luaEngine.LoadForEntity(*this, e, sc);
			luaEngine.CallStart(*this, e, sc);
			sc.started = true;
		}
		return;
	}
}

/**
 * @brief Updates a specific script component.
 * 
 * Calls the appropriate OnUpdate method for either Native C++ scripts or Lua scripts.
 * Handles lazy loading for Lua scripts if needed.
 * 
 * @param e The entity owning the script.
 * @param dt Delta time for the update.
 */
void GameApp::ScriptUpdate(Entity e, float dt)
{
	auto it = scriptComponents.find(e);
	if (it == scriptComponents.end()) return;
	auto& sc = it->second;

	if (sc.backend == ScriptBackend::NativeCpp)
	{
		if (sc.native) sc.native->OnUpdate(*this, e, dt);
		return;
	}

	if (sc.backend == ScriptBackend::Lua)
	{
		if (sc.luaUpdateRef < 0) luaEngine.LoadForEntity(*this, e, sc);
		luaEngine.CallUpdate(*this, e, sc, dt);
		return;
	}
}

/**
 * @brief Spawns a projectile from an enemy.
 * 
 * Normalizes the direction vector and creates a new projectile entity.
 * 
 * @param shooter The entity firing the projectile.
 * @param dirX X-component of the direction.
 * @param dirY Y-component of the direction.
 */
void GameApp::SpawnEnemyProjectile(Entity shooter, float dirX, float dirY)
{
	// Normalize direction
	float len = std::sqrt(dirX * dirX + dirY * dirY);
	if (len < 1e-3f) return;
	Vector2 dir{ dirX / len, dirY / len };
	MakeProjectile(*this, shooter, dir, true); // true = fromEnemy
}

/**
 * @brief Detaches/Unloads all Lua scripts.
 * 
 * Used when shutting down or reloading to clean up Lua references.
 */
void GameApp::LuaDetachAll()
{
	for (auto& [e, sc] : scriptComponents)
	{
		if (sc.backend == ScriptBackend::Lua)
		{
			luaEngine.Unload(sc);   // unref env/start/update
			sc.started = false;
		}
	}
}

/**
 * @brief Reloads all Lua scripts.
 * 
 * Forces a reload of the Lua script file for all entities using Lua backend.
 * Useful for hot-reloading scripts during development.
 */
void GameApp::LuaReloadAll()
{
	for (auto& [e, sc] : scriptComponents)
	{
		if (sc.backend == ScriptBackend::Lua)
		{
			luaEngine.LoadForEntity(*this, e, sc);
			sc.started = false; // will call OnStart again next frame
		}
	}
}

/**
 * @brief Adds a Lua script component to an entity.
 * 
 * Configures the script component to use the Lua backend and sets the script file path.
 * 
 * @param e The entity to add the script to.
 * @param luaFile Path to the Lua script file.
 */
void GameApp::AddLuaScript(Entity e, const std::string& luaFile)
{
	auto& sc = scriptComponents[e];
	sc.backend = ScriptBackend::Lua;
	sc.kind = ScriptKind::None;     // not used for Lua
	sc.luaFile = luaFile;
	if (sc.luaFile.rfind("./", 0) == 0)      // starts with "./"
		sc.luaFile.erase(0, 2);

	// force lazy reload on next tick
	luaEngine.Unload(sc);
	sc.started = false;
}

//
///**

/**
	* @brief Stores current player world position so Stop can restore it.
*/
void GameApp::CapturePlayerSnapshot() {
	const Entity p = FindPlayer();
	if (p == INVALID_ENTITY) return;

	if (const Transform* t = TryGetTransform(p)) {   // const-safe read
		snapPlayerPos = t->GetPosition();
	}
}

/**
	* @brief Restores player position after leaving Play mode.
*/
void GameApp::RestorePlayerSnapshot() {
	const Entity p = FindPlayer();
	if (p == INVALID_ENTITY) return;

	if (Transform* t = GetTransform(p)) {            // non-const write
		t->SetPosition(snapPlayerPos);
	}

	// Optional ONLY: zero controller motion if keep velocity etc. there.
	// velocity/state fields are not public in the header.
	// So only touch it if add such fields.
	if (PlayerController* pc = GetController(p)) {
		// e.g., if later add pc->velocity, can clear it here.
		(void)pc;
	}
}

/**
 * @brief Resets the player's speed component to "outside" preset.
 * 
 * Used when transitioning from a room (inside) to the labyrinth (outside).
 * Adjusts max speed, friction, and flags.
 */
void GameApp::ResetPlayerSpeedPresetOutside() {
	const Entity p = FindPlayer();
	if (p == INVALID_ENTITY) return;
	if (const SpeedComponent* s = TryGetSpeed(p)) {
		SpeedComponent* spd = const_cast<SpeedComponent*>(s);
		spd->speed = { 0.0f, 0.0f };
		spd->maxSpeed = 250.0f;
		spd->friction = 625.0f;
		spd->roomSpeed = false;
		spd->outsideSpeed = true;
	}
	/*if (PlayerController* pc = GetController(p)) {
		pc->ResetSpeed();
	}*/
}
/**
	* @brief Reconstructs the main Camera2D using actual framebuffer size.
	*
	* Called during:
	*  - Fullscreen toggle
	*  - Window resize
	*  - Editor viewport rebuild
	*
	* Reapplies:
	*  - Viewport and orthographic bounds
	*  - Position and zoom from config
	*
	* @param renderer Renderer that binds the camera.
*/
void GameApp::RebuildCameraToCurrentFramebuffer(Renderer& renderer) {

	int fbw, fbh;
	glfwGetFramebufferSize(glfwGetCurrentContext(), &fbw, &fbh);

	if (fbw <= 0 || fbh <= 0)
		return;

	camera.SetViewport(0, 0, fbw, fbh);

	renderer.setCamera(&camera);
}

/**
	* @brief Renders the actual gameplay world, minimap overlay,
	*        GUI HUD, and debug collider outlines.
	*
	* Used outside the Editor’s split-viewport system.
	*
	* @param renderer Renderer used for issuing draw calls.
*/
void GameApp::DrawGameplay(Renderer& renderer) {
	// World + minimap + stress test
	renderer.setCamera(&camera);

	if (layering_.RenderEnabled(eng::LayerId::Background) ||
		layering_.RenderEnabled(eng::LayerId::World))
	{
		Utility::DrawWorld(renderer, systemManager, minimapHUD, stressMgr, *this);
	}

	// In-game HUD (Mute/Exit etc. if you keep it)
	if (layering_.RenderEnabled(eng::LayerId::UI))
	{
		Utility::DrawGuiLayer(renderer, gui, guiVisible);
		playerHUD.Draw(renderer, *this);

		if (GetPuzzleSystem())
			GetPuzzleSystem()->DrawMemoryOverlay(renderer);
	}

	// Debug drawing
	if (layering_.RenderEnabled(eng::LayerId::Debug))
	{
		DebugDraw::ColliderOutlines(*this, renderer, 4);
	}
}

/**
 * @brief Fully resets the gameplay state and returns the game to its initial scene.
 *
 * This function is called when the player exits back to the Main Menu after
 * entering gameplay. Unlike normal scene transitions, RestartGame() performs
 * a *full gameplay reset* to ensure the game always boots from the canonical
 * starting environment.
 *
 */
void GameApp::ResetGameplayWorld() {
	DebugConsole::Get().Info("[GameApp] Resetting gameplay world...\n");

	if (playerEntity != INVALID_ENTITY) {
		if (PlayerController* pc = GetController(playerEntity)) {
			pc->Reset();
		}
	}

	playerHUD.Reset();

	isPausedByESC = false;

	playerEntity = INVALID_ENTITY;

	isCameraLocked = true;

	DestroyAllNonPersistentEntities();

	MapGenerator::ClearDefeatedEnemies();
	Interaction::ClearBossCorpses();
	if (puzzleSys) puzzleSys->Reset();
	doorSystem.ClearEnteredDoors();
}

/**
 * @brief Retrieves the world position (X, Y) of an entity.
 * 
 * @param e The entity ID.
 * @param outX Output parameter for X coordinate.
 * @param outY Output parameter for Y coordinate.
 * @return true if the entity has a Transform component, false otherwise.
 */
bool GameApp::TryGetWorldPos(Entity e, float& outX, float& outY) const
{
	const Transform* t = TryGetTransform(e);
	if (!t) return false;

	Vector2 p = t->GetPosition();
	outX = p.x;
	outY = p.y;
	return true;
}

/**
 * @brief Sets the world position of an entity.
 * 
 * @param e The entity ID.
 * @param x The new X coordinate.
 * @param y The new Y coordinate.
 * @return true if the entity has a Transform component and position was set, false otherwise.
 */
bool GameApp::SetWorldPos(Entity e, float x, float y)
{
	Transform* t = GetTransform(e);
	if (!t) return false;

	if (GetEnemyController(e) != nullptr && playerEntity != INVALID_ENTITY)
	{
		Collider* enemyCol = GetCollider(e);
		Transform* playerTr = GetTransform(playerEntity);
		Collider* playerCol = GetCollider(playerEntity);

		if (enemyCol && playerTr && playerCol)
		{
			const Vector2 oldPos = t->GetPosition();
			const Vector2 reqPos{ x, y };
			const Vector2 playerPos = playerTr->GetPosition();

			const bool wouldHitPlayer =
				enemyCol->CheckCollision(
					*playerCol,
					reqPos,
					playerPos,
					t->GetRotation(),
					playerTr->GetRotation());

			if (wouldHitPlayer)
			{
				// Move to the furthest position along old->requested that is still legal.
				Vector2 best = oldPos;
				float lo = 0.0f;
				float hi = 1.0f;

				for (int i = 0; i < 10; ++i) {
					float mid = (lo + hi) * 0.5f;
					Vector2 test = oldPos + (reqPos - oldPos) * mid;

					bool hit =
						enemyCol->CheckCollision(
							*playerCol,
							test,
							playerPos,
							t->GetRotation(),
							playerTr->GetRotation());

					if (!hit) {
						best = test;
						lo = mid;
					}
					else {
						hi = mid;
					}
				}

				t->SetPosition(best);

				// Treat the blocked charge as a successful contact.
				auto it = Interaction::burrowActiveMap.find(playerEntity);
				const bool isBurrowed =
					(it != Interaction::burrowActiveMap.end() && it->second);

				if (!isBurrowed) {
					Interaction::HandlePlayerDamage(*this, playerEntity, 1);
				}

				return true;
			}
		}
	}

	t->SetPosition(Vector2{ x, y });
	return true;
}

/**
 * @brief Retrieves the rotation of an entity.
 * 
 * @param e The entity ID.
 * @param outR Output parameter for rotation (in degrees or radians depending on usage).
 * @return true if the entity has a Transform component, false otherwise.
 */
bool GameApp::TryGetRotation(Entity e, float& outR) const {
	if (const Transform* t = TryGetTransform(e))
	{
		outR = t->GetRotation();
		return true;
	}
	return false;
}

/**
 * @brief Sets the rotation of an entity.
 * 
 * @param e The entity ID.
 * @param r The new rotation value.
 * @return true if the entity has a Transform component and rotation was set, false otherwise.
 */
bool GameApp::SetRotation(Entity e, float r)
{
	if (Transform* t = GetTransform(e))
	{
		t->SetRotation(r);
		return true;
	}
	return false;
}

/**
 * @brief Gets the current player entity ID.
 * 
 * @return The Entity ID of the player, or INVALID_ENTITY if none exists.
 */
Entity GameApp::GetPlayerEntity() const
{
	return playerEntity; // whatever already stored as the player id
}

/**
 * @brief Checks if a world position is blocked by the AI grid.
 * 
 * Converts world coordinates to grid coordinates and checks the AI grid
 * for obstacles (walls, etc.).
 * 
 * @param wx World X coordinate.
 * @param wy World Y coordinate.
 * @return true if the position is blocked or out of bounds, false if clear.
 */
bool GameApp::AI_IsBlockedWorld(float wx, float wy) const
{
	if (aiGrid_.empty() || aiTileSize_ <= 0.0f) return true;

	int col = (int)std::floor(wx / aiTileSize_);
	int row = (int)std::floor(wy / aiTileSize_);

	if (row < 0 || row >= (int)aiGrid_.size() ||
		col < 0 || col >= (int)aiGrid_[row].size())
		return true;

	char c = aiGrid_[row][col];
	return (c == '1' || c == '3' || c == '.' || c == 'R' || c == '2' || c == '8' || c == 'P' || c == 'D'); // match blocked logic frm the ai cpp
	//return (c != '0');
}

bool GameApp::AI_IsWallWorld(float wx, float wy) const
{
	if (aiGrid_.empty() || aiTileSize_ <= 0.0f) return true;
	int col = (int)std::floor(wx / aiTileSize_);
	int row = (int)std::floor(wy / aiTileSize_);
	if (row < 0 || row >= (int)aiGrid_.size() ||
		col < 0 || col >= (int)aiGrid_[row].size())
		return true;
	char c = aiGrid_[row][col];
	return (c == '1' || c == '3' || c == '.' || c == 'R' || c == '2' || c == '8' || c == 'D');
	// 'P' intentionally excluded — player tile is not a wall
}

/**
 * @brief Checks Line of Sight (LOS) between two world positions.
 * 
 * Uses Bresenham's line algorithm on the AI grid to determine if there is a clear
 * path between the start and end points.
 * 
 * @param ax Start X coordinate.
 * @param ay Start Y coordinate.
 * @param bx End X coordinate.
 * @param by End Y coordinate.
 * @return true if there is a clear line of sight, false if obstructed.
 */
bool GameApp::AI_HasLOSWorld(float ax, float ay, float bx, float by, Entity e, PlayerAbility ability) const
{
	if (Interaction::GetAbilityCooldown(e, ability) > 0) return false;
	if (aiGrid_.empty() || aiTileSize_ <= 0.0f) return false;

	auto toGrid = [&](float x, float y) {
		int gx = (int)std::floor(x / aiTileSize_);
		int gy = (int)std::floor(y / aiTileSize_);
		return std::pair<int, int>{gx, gy};
		};

	auto [x0, y0] = toGrid(ax, ay);
	auto [x1, y1] = toGrid(bx, by);

	auto isWall = [&](int y, int x) {
		if (y < 0 || y >= (int)aiGrid_.size()) return true;
		if (x < 0 || x >= (int)aiGrid_[y].size()) return true;
		char c = aiGrid_[y][x];
		return (c == '1' || c == '3' || c == 'R' || c == '2' || c == '8' || c == 'P' || c == 'D'); // LOS blocks on walls
		//return (c != '0');
		};

	int dx = std::abs(x1 - x0);
	int dy = std::abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	while (true)
	{
		if (isWall(y0, x0)) return false;
		if (x0 == x1 && y0 == y1) break;

		int e2 = 2 * err;
		if (e2 > -dy) { err -= dy; x0 += sx; }
		if (e2 < dx) { err += dx; y0 += sy; }
	}
	return true;
}

/**
 * @brief Moves an entity in the world for AI purposes.
 * 
 * Sets the velocity in the SpeedComponent if present, otherwise directly updates
 * the Transform position (fallback).
 * 
 * @param e The entity to move.
 * @param vx Velocity X.
 * @param vy Velocity Y.
 * @param dt Delta time (unused in current implementation but kept for API consistency).
 */
void GameApp::AI_MoveEntityWorld(Entity e, float vx, float vy, float /*dt*/)
{
	// set speed component velocity
	if (auto it = temp_spdComponent.find(e); it != temp_spdComponent.end())
	{
		it->second.speed = Vector2{ vx, vy };
		return;
	}

	// Fallback: directly move transform if no speed comp
	if (Transform* t = GetTransform(e))
	{
		Vector2 p = t->GetPosition();
		t->SetPosition(Vector2{ p.x + vx * 0.016f, p.y + vy * 0.016f }); // fallback
	}
}

/**
 * @brief Reserves memory pages for ECS component pools based on expected object count.
 * 
 * Pre-allocates memory for Transform, Collider, Renderer, and Projectile pools to
 * minimize runtime allocations.
 * 
 * @param expectedSceneObjects Estimated number of objects in the scene.
 */
void GameApp::ReserveLevelPools(std::size_t expectedSceneObjects)
{
	// Transform exists on almost every entity, so reserve based on scene object count.
	// Pool has 512 blocks/page -> compute pages needed + 1 safety page.
	std::size_t pages = (expectedSceneObjects + 511) / 512;
	pages += 1; // safety buffer to avoid runtime expansion during normal play

	transformPool.ReservePages(pages);
	colliderPool.ReservePages(pages);
	rendererPool.ReservePages(pages);
	projectilePool.ReservePages(pages);
}

// --- Debug / Statistics for Pools ---

std::size_t GameApp::TransformPoolPages() const
{
	return transformPool.PagesAllocated();
}

std::size_t GameApp::TransformPoolLive() const
{
	return transformPool.LiveObjects();
}

std::size_t GameApp::TransformPoolExpansions() const
{
	return transformPool.Expansions();
}

std::size_t GameApp::ColliderPoolPages() const { return colliderPool.PagesAllocated(); }
std::size_t GameApp::ColliderPoolLive() const { return colliderPool.LiveObjects(); }
std::size_t GameApp::ColliderPoolExpansions() const { return colliderPool.Expansions(); }

std::size_t GameApp::RendererPoolPages() const { return rendererPool.PagesAllocated(); }
std::size_t GameApp::RendererPoolLive() const { return rendererPool.LiveObjects(); }
std::size_t GameApp::RendererPoolExpansions() const { return rendererPool.Expansions(); }

std::size_t GameApp::ProjectilePoolPages() const { return projectilePool.PagesAllocated(); }
std::size_t GameApp::ProjectilePoolLive() const { return projectilePool.LiveObjects(); }
std::size_t GameApp::ProjectilePoolExpansions() const { return projectilePool.Expansions(); }

/**
 * @brief Handles the player's death event.
 * 
 * Transitions the game state to the "Lose" screen if not already there.
 */
void GameApp::OnPlayerDeath()
{
	if (sceneManager.GetState() == SceneState::Lose)
		return;

	DebugConsole::Get().Error("[Game] Player HP reached 0 -> Lose screen.\n");

	sceneManager.SetState(SceneState::Lose);
}

/**
 * @brief Handles an enemy's death event.
 * 
 * Plays death sound effects (spatialized), updates boss status if applicable,
 * removes enemy components (Controller, Speed, Mass, Animator), and fades out the renderer.
 * 
 * @param enemy The enemy entity that died.
 */
void GameApp::OnEnemyDeath(Entity enemy)
{
	if (!enemy)
		return;

	// --- NEW: SPATIAL AUDIO BLOCK ---
	float ex = 0.0f, ey = 0.0f;
	if (TryGetWorldPos(enemy, ex, ey)) {

		if (auto* ec = GetEnemyController(enemy)) {
			// Handle gameplay logic (Boss corpse marking)
			if (ec->IsBoss()) {
				Interaction::MarkAsBossCorpse(enemy);
			}

			// Handle Audio
			PlayRandomEnemyDeath(*this, ec->GetMobType(), ec->IsBoss(), glm::vec2(ex, ey));
		}
	}

	RemoveEnemyControllerComponent(enemy);
	RemoveSpeedComponent(enemy);
	RemoveMassComponent(enemy);

	SetEntityScriptName(enemy, "None");

	SpriteAnimator* animator = GetAnimator(enemy);
	MeshRenderer* renderer = GetRenderer(enemy);

	if (animator)
	{
		const std::string prefab = GetPrefabTag(enemy);
		const SpriteSheet* sheet = animator->sheet;

		if (!sheet) {
			if (prefab == "EnemyContact")
				sheet = ResourceManager::GetSpriteSheet("default_enemy_R");
			else if (prefab == "burrow_mini_boss")
				sheet = ResourceManager::GetSpriteSheet("burrow_enemy");
			else if (prefab == "heal_mini_boss")
				sheet = ResourceManager::GetSpriteSheet("heal_enemy");
		}

		if (sheet)
		{
			animator->SetSheet(sheet, false);
			const int maxFrame = sheet->cols * sheet->rows - 1;
			animator->SetRange(maxFrame, maxFrame, true);
			animator->speed = 0.0f;
		}
		else
		{
			RemoveAnimatorComponent(enemy);
			animator = nullptr;
		}
	}

	if (renderer)
	{
		Vector3 color = renderer->GetColor();
		color.x *= 0.4f;
		color.y *= 0.4f;
		color.z *= 0.4f;
		renderer->SetColor(color);
	}
}

void GameApp::SetColliderActive(Entity e, bool active)
{
	Collider* col = GetCollider(e);
	if (col) col->isBurrowed = !active;  // isBurrowed=true means untargetable/no-collision
}

void GameApp::SetAnimationRow(Entity e, int row)
{
	SpriteAnimator* anim = GetAnimator(e);
	if (!anim || !anim->sheet) return;

	// Burrow mini-boss uses row indices as phase markers (visual-only). Map them to
	// dedicated spritesheets instead of assuming stacked rows on a single sheet.
	auto it = prefabTags.find(e);
	if (it != prefabTags.end() && it->second == "burrow_mini_boss") {
		const char* sheetKey = nullptr;
		switch (row) {
		case 1: sheetKey = "BurrowUnderEnemy"; break;           // ground -> underground
		case 2: sheetKey = "underground_burrow_enemy"; break;   // underground moving
		case 3: sheetKey = "RisingUpEnemy"; break;              // underground -> ground
		default: sheetKey = "burrow_enemy"; break;              // above-ground base (idle handled by controller)
		}

		if (sheetKey) {
			const SpriteSheet* sh = ResourceManager::GetSpriteSheet(sheetKey);
			if (sh) {
				const bool sheetChanged = (anim->sheet != sh);
				if (sheetChanged) {
					anim->SetSheet(sh, true);
					anim->SetSpeedFromFrameDuration(sh->frameDuration);
				}

				const int startFrame = 0;
				const int endFrame = std::max(0, (sh->cols * sh->rows) - 1);
				const bool rangeChanged = (anim->startFrame != startFrame || anim->endFrame != endFrame);
				if (rangeChanged) {
					anim->SetRange(startFrame, endFrame, true);
				}
				else if (sheetChanged) {
					anim->Restart();
				}
			}
		}
		return;
	}

	const int cols = anim->sheet->cols;
	const int rows = std::max(1, anim->sheet->rows);
	const int clampedRow = std::max(0, std::min(row, rows - 1));
	const int startFrame = clampedRow * cols;
	const int endFrame = startFrame + cols - 1;
	const bool reset = !(anim->startFrame == startFrame && anim->endFrame == endFrame);
	anim->SetRange(startFrame, endFrame, reset);
}

int GameApp::GetEnemyHp(Entity e) const
{
	const EnemiesController* ec = TryGetEnemyController(e);
	return ec ? ec->GetHP() : 0;
}

int GameApp::GetEnemyMaxHp(Entity e) const
{
	const EnemiesController* ec = TryGetEnemyController(e);
	return ec ? ec->GetMaxHP() : 1;
}

void GameApp::HealEnemyHp(Entity e, int amount)
{
	EnemiesController* ec = GetEnemyController(e);
	if (ec) ec->RestoreHp(amount);
}

//void GameApp::checkGamepadStatus()
//{
//	if (glfwJoystickPresent(GLFW_JOYSTICK_1)) {
//		if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1)) {
//			DebugConsole::Get().Success("Gamepad connected!\n");
//			
//		}
//		else
//		{
//			DebugConsole::Get().Error("Gamepad not found");
//		}
//	}
//	
//}
