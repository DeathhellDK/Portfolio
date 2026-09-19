/**
 * @file      gameApp_Callbacks.cpp
 * @author    Woh Kye Le, Jethro Sung
 * @email     w.kyele, sung.h
 * @date      2026-02-17
 *
 * @brief     Defines and binds runtime/editor callback handlers used by GameApp.
 *
 *            Includes:
 *            - Collision response (damage application with burrow immunity)
 *            - GUI button actions (mute/resume audio, exit)
 *            - Door scene transition request callback
 *            - Editor/RoomEditor hooks (play controls, save/load/apply grid baking, prefab ops, undo/redo, camera lock)
 *            - Game event observer handlers (enemy spotting/chasing state logs)
 *
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include <filesystem>

#include "Core/gameApp.h"
#include "Core/utils.h"
#include "Input/DebugConsole.hpp"
#include "factories.h"
#include "World/mapGenerator.h"
#include "Mechanics/interaction.hpp"
#include "playerController.h"

#if ENABLE_EDITOR
#include "Editor/editor.h"
#include "Editor/RoomEditor.h"
#include "Editor/UiEditor.h"
#endif

 /**
  * @brief Initializes the collision system callback for player-enemy interactions.
  *
  * Sets CollisionSystem::collisionCallback to apply damage when the player collides
  * with an enemy. If the player is currently burrowed (tracked via
  * Interaction::burrowActiveMap), damage is skipped.
  *
  * - Enemy takes 10 damage (type/flag = 1)
  * - Player takes 5 damage (type/flag = 0)
  *
  * @note No callback is bound if collisionSys is null.
  */
void GameApp::InitializeCollisionCallback(float dt) {
	(void)dt;
	if (!collisionSys) {
		return;
	}
	collisionSys->collisionCallback = [&](Entity* player, Entity* enemy) {
		(void)enemy;
		auto it = Interaction::burrowActiveMap.find(*player);
		const bool isBurrowed = (it != Interaction::burrowActiveMap.end() && it->second);

		if (!isBurrowed) {
			///Interaction::HandlePlayerDamage(*this, *enemy, 10);
			Interaction::HandlePlayerDamage(*this, *player, 1);
		}
	};
}

/**
 * @brief Registers GUI button callbacks for common in-game actions.
 *
 * Adds:
 * - "Mute" button: toggles pausing/resuming all audio via ResourceManager and
 *   updates the isAudioPaused state.
 * - "Exit" button: sets shouldQuit to true, signaling the app to close.
 */
void GameApp::InitializeGuiCallbacks() {
	gui.AddButton(GuiButton({ 100, 180 }, { 200, 60 }, "Mute", [this]() {
		if (!isAudioPaused) {
			DebugConsole::Get().Warning("[GUI] Pausing all audio...\n");
			ResourceManager::PauseAudio();
			isAudioPaused = true;
		}
		else {
			DebugConsole::Get().Warning("[GUI] Resuming all audio...\n");
			ResourceManager::ResumeAudio();
			isAudioPaused = false;
		}
		}));

	gui.AddButton(GuiButton({ 100, 100 }, { 200, 60 }, "Exit", [this]() {
		DebugConsole::Get().Warning("[GUI] Exit clicked\n");
		shouldQuit = true;
		}));
}

/**
 * @brief Binds the door system scene-transition request callback.
 *
 * Connects DoorSystem's request-scene function to SceneManager so that doors can
 * trigger a gameplay scene load and specify the spawn door name to appear at.
 */
void GameApp::InitializeDoorCallback() {
	doorSystem.SetRequestSceneFn(
		[this](const std::string& sceneFile, const std::string& spawnDoorName) {
			sceneManager.RequestGameplayScene(sceneFile, spawnDoorName);
		}
	);
}

/**
 * @brief Sets up editor and room editor callback hooks (editor builds only).
 *
 * When ENABLE_EDITOR is enabled and required editor components are valid,
 * binds callbacks for:
 * - Camera lock toggling and zoom reset
 * - Level save / level change commands
 * - Play / pause / stop controls for editor play mode
 * - Prefab operations (detect instance, revert/apply, save all)
 * - RoomEditor hooks (create wall entities, resolve paths, load/apply grid)
 * - Query helpers (active room path, active camera pointer, prefab tag lookup)
 * - Selection helpers (get entities within a world-space rectangle)
 * - Entity lifecycle actions (destroy entity)
 * - Undo/redo and transform/entity record events for the undo stack
 *
 * @note This function returns immediately if editorOverlay or roomEditor is null.
 */
void GameApp::InitializeEditorCallbacks() {
#if ENABLE_EDITOR
	if (!editorOverlay || !roomEditor) {
		return;
	}

	editorOverlay->onRequestCameraLock = [this](bool lock) {
		isCameraLocked = lock;
		};

	editorOverlay->onLevelSave = [this](const std::string& path) { SaveLevel(path); };
	editorOverlay->onLightSave = [this](const std::string& path) { SceneLoader::SaveLightSidecar(*this, path); };
	editorOverlay->onLevelChange = [this](const std::string& path) { sceneManager.EnqueueEditorCmd({ SceneManager::EditorCmdType::ChangeLevel, path }); };

	editorOverlay->onSaveLabyrinthVariants = [this]()
		{
			if (!editorOverlay)
				return;

			const std::string variantPath = AssetPath("variants/labyrinth_variants.json");

			std::vector<std::string> tempGrid;
			{
				std::ifstream in(AssetPath("maps/labyrinth.txt"));
				std::string line;
				while (std::getline(in, line))
				{
					if (!line.empty() && line.back() == '\r')
						line.pop_back();
					if (!line.empty())
						tempGrid.push_back(line);
				}
			}

			Variant::SaveRoomData(
				"",
				variantPath,
				"labyrinth",
				GetLabyrinthTileSize(),
				tempGrid,
				editorOverlay->GetLabyrinthWallVariants()
			);

			DebugConsole::Get().Info("[GameApp] Saved labyrinth variants only to variants/labyrinth_variants.json\n");
		};

	auto cb = sceneManager.GetEditorPlayCallbacks();
	editorOverlay->onPlay = cb.onPlay;
	editorOverlay->onPause = cb.onPause;
	editorOverlay->onStop = cb.onStop;

	editorOverlay->isPrefabInstance = [this](Entity e) { return prefabTags.find(e) != prefabTags.end(); };
	editorOverlay->onPrefabRevertInstance = [this](Entity e) { RevertInstanceToPrefab(e); };
	editorOverlay->onPrefabApplyFromInstance = [this](Entity e) { ApplyInstanceAsPrefab(e); };
	editorOverlay->onPrefabSaveAll = [this]() { SavePrefabsToJson(); };

	roomEditor->SetCreateWallEntityCallback([this](int gx, int gy, const std::string& textureKey, bool solid, float tileSize) -> Entity {
		VariantPlacement variant;
		variant.type = "WallTile";
		variant.gx = gx;
		variant.gy = gy;
		variant.textureKey = textureKey;
		variant.solid = solid;
		variant.name = "V_WallTile_" + std::to_string(gx) + "_" + std::to_string(gy);

		return MakeWallTileFromVariant(*this, variant, tileSize);
		});
	roomEditor->SetCreatePrefabEntityCallback([this](int gx, int gy, const std::string& prefabName, float tileSize) -> Entity {
		(void)tileSize;
		const float ts = GetLabyrinthTileSize();
		const Prefab* p = prefabManager.Get(prefabName);
		Vector2 scale = p ? p->scale : Vector2(ts, ts);

		Vector2 worldPos(
			(gx + 0.5f) * ts - scale.x * 0.5f,
			(gy + 0.5f) * ts - scale.y * 0.5f
		);

		Entity e = InstantiatePrefab(prefabName, worldPos);
		if (e == INVALID_ENTITY)
			return e;

		// If this prefab is an enemy, immediately bind it to the current room grid
		if (prefabName == "ranged_mini_boss" ||
			prefabName == "EnemyContact" ||
			prefabName == "burrow_mini_boss" ||
			prefabName == "heal_mini_boss" ||
			prefabName == "Boss")
		{
			if (auto* ec = GetEnemyController(e)) {
				if (Transform* t = GetTransform(e)) {
					ec->SetSpawn(t->GetPosition());
				}

				//// Rebind to current room AI data
				//std::filesystem::path scenePath(sceneManager.GetActiveScenePath());
				//const std::string stem = scenePath.stem().string();
				//const std::string txtPath = AssetPath("maps/" + stem + ".txt");

				//std::vector<std::string> grid;
				//if (SceneRuntime::LoadGridTxt(txtPath, grid, nullptr, nullptr)) {
				//	SetEnemyGrid(grid, ts);   // updates app aiGrid_ and pushes to all enemies
				//}

				// no longer reload grid from file here (SceneRuntime::LoadGridTxt).
				// curr rely on 'aiGrid_' via SetApplyCallback or initial ld
				// Reloading from txt would read stale data if the user hasn't saved the file yet.
				// removed ec->SetMap() for now to prevent AI from reloading map from disk.


				//ec->SetMap("maps/" + stem + ".txt"); Dont del yet

				ec->SetLuaDrivenMovement(true);
				ec->setForceProxy(g_entityForceProxy);

				std::string scriptPath = "Scripts/include/chase.lua";
				if (prefabName == "ranged_mini_boss") {
					scriptPath = "Scripts/include/enemy_ranged.lua";
				} else if (prefabName == "burrow_mini_boss") {
					scriptPath = "Scripts/include/enemy_burrow.lua";
				} else if (prefabName == "heal_mini_boss") {
					scriptPath = "Scripts/include/enemy_heal.lua";
				} else if (prefabName == "Boss") {
					scriptPath = "Scripts/include/enemy_boss.lua";
				} else if (prefabName == "EnemyContact") {
					scriptPath = "Scripts/include/enemy_basic.lua";
				}
				AddLuaScript(e, scriptPath);
			}
		}


		return e;
		});

	roomEditor->SetAvailablePrefabs(prefabManager.GetNames());

	roomEditor->SetGetActiveRoom([this]() -> std::string { return sceneManager.GetActiveScenePath(); });
	roomEditor->SetGetActiveCamera([this]() -> Camera2D* { return &camera; });
	roomEditor->SetResolveRoomVariantPath([](const std::string& roomScenePath) {
		std::filesystem::path p(roomScenePath);
		const std::string stem = p.stem().string();
		return AssetPath("variants/" + stem + "_variants.json");
		});

	roomEditor->SetResolveRoomLightPath([](const std::string& roomScenePath) {
		std::filesystem::path p(roomScenePath);
		const std::string stem = p.stem().string();
		return AssetPath("scene/" + stem + "_lights.json");
		});

	roomEditor->SetResolveRoomTxtPath([this](const std::string& roomScenePath) -> std::string {
		std::filesystem::path p(roomScenePath);
		const std::string stem = p.stem().string();
		return AssetPath("maps/" + stem + ".txt");
		});

	roomEditor->SetLoadGrid([](const std::string& path, std::vector<std::string>& outGrid, int& outW, int& outH) -> bool {
		const bool ok = SceneRuntime::LoadGridTxt(path, outGrid, nullptr, nullptr);
		if (!ok) {
			outW = 0;
			outH = 0;
			return false;
		}

		outH = (int)outGrid.size();
		outW = outH > 0 ? (int)outGrid[0].size() : 0;
		return true;
		});

	roomEditor->SetApplyCallback([this](const std::vector<std::string>& grid, int w, int h, const std::string& roomScenePath) {
		(void)w;

		const float tileSize = GetLabyrinthTileSize();
		const std::string scenePath = !roomScenePath.empty() ? roomScenePath : sceneManager.GetActiveScenePath();
		MapGenerator::ClearBakedRoom(*this, scenePath);
		MapGenerator::BakeRoomFromGrid(*this, grid, tileSize, scenePath);

		SetEnemyGrid(grid, tileSize);

		std::vector<Entity> variantEntities = ApplyRoomVariants(*this, scenePath, h, tileSize);
		(void)variantEntities;

		SceneLoader::ApplyLightOverrides(*this, scenePath);
		});

	roomEditor->SetPlayControls([this]() { return sceneManager.GetPlayControlsState(); }, cb);

	roomEditor->SetGetEntitiesInRect([&](const Vector2& worldPos, const Vector2& worldSize) -> std::vector<Entity> {
		std::vector<Entity> result;

		for (const auto& entry : transforms) {
			Entity entity = entry.first;
			Transform* transform = entry.second;

			Vector2 entityPos = transform->GetPosition();
			Vector2 entitySize;

			if (auto* collider = GetCollider(entity)) {
				switch (collider->type) {
				case ColliderType::Box:
					entitySize = collider->size;
					break;
				case ColliderType::Circle:
					entitySize = Vector2(collider->size.x * 2.0f, collider->size.x * 2.0f);
					break;
				case ColliderType::Triangle:
					entitySize = transform->GetScale();
					break;
				default:
					entitySize = transform->GetScale();
					break;
				}
			}
			else {
				entitySize = transform->GetScale();
			}

			if (entityPos.x < worldPos.x + worldSize.x &&
				entityPos.x + entitySize.x > worldPos.x &&
				entityPos.y < worldPos.y + worldSize.y &&
				entityPos.y + entitySize.y > worldPos.y) {
				result.push_back(entity);
			}
		}

		return result;
		});

	roomEditor->SetGetPrefabTag([&](Entity entity) -> std::string {
		return GetPrefabTag(entity);
		});

	roomEditor->SetGetTextureID([this](const std::string& key) { return EditorGetTextureID(key); });
	roomEditor->SetGetPrefabTexture([this](const std::string& name) { return EditorGetPrefabTexture(name); });

	roomEditor->SetDestroyEntityCallback([this](Entity entity) {
		DestroyEntity(entity);
		});

	roomEditor->SetRequestCameraLock([this](bool lock) {
		isCameraLocked = lock;

		if (lock) {
			camera.setZoom(1.0f);
			camera.UpdateView();
		}
		});

	editorOverlay->onUndo = [this]() {
		if (undoRedo) {
			undoRedo->Undo();
		}
		};

	editorOverlay->onRedo = [this]() {
		if (undoRedo) {
			undoRedo->Redo();
		}
		};

	editorOverlay->onRecordBeforeTransform = [this](Entity e) {
		const bool allowRecord = (!sceneManager.IsEditorPlaying()) || sceneManager.IsEditorPaused();

		if (!allowRecord) {
			return;
		}
		if (!undoRedo) return;
		if (Transform* t = GetTransform(e)) {
			beforeCachePos = t->GetPosition();
			beforeCacheScale = t->GetScale();
			beforeCacheRot = t->GetRotation();
		}
		if (Collider* c = GetCollider(e)) {
			beforeCacheHadCollider = true;
			beforeCacheColliderSize = c->size;
			beforeCacheColliderTrigger = c->isTrigger;
		}
		else {
			beforeCacheHadCollider = false;
			beforeCacheColliderSize = { 1.f, 1.f };
			beforeCacheColliderTrigger = false;
		}
		};

	editorOverlay->onRecordAfterTransform = [this](Entity e) {
		const bool allowRecord = (!sceneManager.IsEditorPlaying()) || sceneManager.IsEditorPaused();

		if (!allowRecord) {
			return;
		}
		if (!undoRedo) {
			return;
		}

		Transform* t = GetTransform(e);
		if (!t) return;

		Collider* c = GetCollider(e);

		const bool hasColliderNow = (c != nullptr);
		const Vector2 newColSize = hasColliderNow ? c->size : Vector2{ 1.f, 1.f };
		const bool newColTrigger = hasColliderNow ? c->isTrigger : false;

		const bool colliderChanged =
			(beforeCacheHadCollider == hasColliderNow) &&
			hasColliderNow &&
			(
				beforeCacheColliderSize.x != newColSize.x ||
				beforeCacheColliderSize.y != newColSize.y ||
				beforeCacheColliderTrigger != newColTrigger
				);

		if (colliderChanged)
		{
			undoRedo->Push_TransformUpdateWithCollider(
				e,
				beforeCachePos,
				t->GetPosition(),
				beforeCacheScale,
				t->GetScale(),
				beforeCacheRot,
				t->GetRotation(),
				true,
				beforeCacheColliderSize,
				newColSize,
				beforeCacheColliderTrigger,
				newColTrigger
			);
		}
		else
		{
			undoRedo->Push_TransformUpdate(
				e,
				beforeCachePos,
				t->GetPosition(),
				beforeCacheScale,
				t->GetScale(),
				beforeCacheRot,
				t->GetRotation()
			);
		}
	};

	editorOverlay->onRecordEntityCreated = [this](Entity e) {
		const bool allowRecord = (!sceneManager.IsEditorPlaying()) || sceneManager.IsEditorPaused();

		if (!allowRecord) {
			return;
		}
		if (undoRedo) {
			undoRedo->Push_EntityCreated(e);
		}
		};

	editorOverlay->onRecordEntityDeleted = [this](Entity e) {
		const bool allowRecord = (!sceneManager.IsEditorPlaying()) || sceneManager.IsEditorPaused();

		if (!allowRecord) {
			return;
		}
		if (!undoRedo) {
			return;
		}

		if (Transform* t = GetTransform(e)) {
			undoRedo->Push_EntityDeleted(
				e,
				t->GetPosition(),
				t->GetScale(),
				t->GetRotation());
		}
		else {
			undoRedo->Push_EntityDeleted(e, { 0, 0 }, { 1, 1 }, 0.0f);
		}
		};

	editorOverlay->undoRedoPtr = undoRedo;

	editorOverlay->SetAvailablePrefabs(prefabManager.GetNames());

	editorOverlay->SetTextureLookup(
		[this](const std::string& key)
		{
			return EditorGetTextureID(key);
		});

	editorOverlay->SetPrefabTextureLookup(
		[this](const std::string& name)
		{
			return EditorGetPrefabTexture(name);
		});

	editorOverlay->SetLabyrinthTileSize(GetLabyrinthTileSize());

	editorOverlay->onCreateLabyrinthWallVariant =
		[this](int gx, int gy, const std::string& textureKey, float tileSize) -> Entity
		{
			VariantPlacement v;
			v.type = "WallTile";
			v.gx = gx;
			v.gy = gy;
			v.textureKey = textureKey;
			v.solid = true;
			v.name = "LabyrinthWall_" + std::to_string(gx) + "_" + std::to_string(gy);

			Entity e = MakeWallTileFromVariant(*this, v, tileSize);
			if (e != INVALID_ENTITY)
				RemoveColliderComponent(e); // keep merged colliders

			return e;
		};

	Utility::BindEditorOverlay(*this, *editorOverlay);
#endif
}

void GameApp::InitializeRoomEditor() {
#if ENABLE_EDITOR
	if (!roomEditor) return;

	roomEditor->SetContext(*this);
	roomEditor->SetUndoRedoManager(undoRedo);
	roomEditor->SetCustomPropertiesDraw([this](Entity e, VariantPlacement* v, bool& changed) {
		if (PlayerController* pc = this->GetController(e)) {
			if (ImGui::TreeNode("PlayerController")) {
				// Ability
				const char* abilityNames[] = { "Default", "Burrow", "Heal", "Projectile" };
				int currentAbility = (int)pc->GetAbility();
				if (ImGui::Combo("Ability", &currentAbility, abilityNames, IM_ARRAYSIZE(abilityNames))) {
					pc->SetAbility((PlayerAbility)currentAbility);
				}

				// HP
				int hp = pc->getPlayerHp();
				if (ImGui::InputInt("HP", &hp)) {
					pc->setPlayerHp(hp);
				}

				// Rot Speed
				ImGui::DragFloat("Rot Speed", &pc->rotSpeed, 0.1f);

				ImGui::TreePop();
			}
		}

		if (PuzzleObject* po = this->GetPuzzleObject(e)) {
			if (ImGui::TreeNode("PuzzleObject")) {
				// If variant has never stored puzzle data yet, seed it from runtime
				if (v && !v->hasPuzzleData) {
					v->hasPuzzleData = true;
					v->puzzleKind = (int)po->kind;
					v->puzzleGroupId = po->groupId;
					v->puzzleActive = po->active;

					v->pi0 = po->i0; v->pi1 = po->i1; v->pi2 = po->i2; v->pi3 = po->i3;
					v->pf0 = po->f0; v->pf1 = po->f1; v->pf2 = po->f2; v->pf3 = po->f3;
					v->ps0 = po->s0; v->ps1 = po->s1;
				}

				auto syncToVariant = [&]() {
					if (!v) return;
					v->hasPuzzleData = true;
					v->puzzleKind = (int)po->kind;
					v->puzzleGroupId = po->groupId;
					v->puzzleActive = po->active;

					v->pi0 = po->i0; v->pi1 = po->i1; v->pi2 = po->i2; v->pi3 = po->i3;
					v->pf0 = po->f0; v->pf1 = po->f1; v->pf2 = po->f2; v->pf3 = po->f3;
					v->ps0 = po->s0; v->ps1 = po->s1;
					changed = true;
					};

				ImGui::Text("Kind: %d", (int)po->kind);
				//if (ImGui::InputInt("Group ID", &po->groupId)) syncToVariant();
				if (ImGui::Checkbox("Active", &po->active)) syncToVariant();

				switch (po->kind) {
				case PuzzleKind::Lever:
				{
					if (ImGui::InputInt("Link Group ID", &po->groupId))
						syncToVariant();

					bool toggle = (po->i0 != 0);
					if (ImGui::Checkbox("Toggle Lever", &toggle)) {
						po->i0 = toggle ? 1 : 0;
						syncToVariant();
					}

					ImGui::TextWrapped("Use the same Link Group ID on a manual DoorLock or BurrowWall.");
					break;
				}

				case PuzzleKind::ShootTarget:
				{
					if (ImGui::InputInt("Target Group ID", &po->groupId))
						syncToVariant();

					if (ImGui::InputInt("Required Hits", &po->i0))
						syncToVariant();

					if (ImGui::InputInt("Current Hits", &po->i1))
						syncToVariant();

					bool playerOnly = (po->i2 != 0);
					if (ImGui::Checkbox("Requires Player Projectile", &playerOnly)) {
						po->i2 = playerOnly ? 1 : 0;
						syncToVariant();
					}

					ImGui::TextWrapped("Use the same Target Group ID on a DoorLock in ShootTarget mode.");
					break;
				}

				case PuzzleKind::DoorLock:
				{
					int mode = 0;
					// 0 = manual
					// 1 = single ShootTarget group
					// 2 = multi ShootTarget groups

					if (po->i0 == -1) mode = 0;
					else if (po->i0 == 0) mode = 1;
					else mode = 2;

					const char* modeNames[] = {
						"Manual (Lever / Switch Group)",
						"Single ShootTarget Group",
						"Multi ShootTarget Groups"
					};

					if (ImGui::Combo("Door Link Mode", &mode, modeNames, IM_ARRAYSIZE(modeNames))) {
						if (mode == 0) {
							po->i0 = -1;
							po->i1 = 0;
							po->i2 = 0;
							po->i3 = 0;
						}
						else if (mode == 1) {
							po->i0 = 0;
							po->i1 = 0;
							po->i2 = 0;
							po->i3 = 0;
						}
						else {
							if (po->i0 <= 0)
								po->i0 = 2;
						}
						syncToVariant();
					}

					if (ImGui::InputInt("Link Group ID", &po->groupId))
						syncToVariant();

					if (mode == 2) {
						int requiredCount = po->i0;
						if (requiredCount < 2) requiredCount = 2;
						if (requiredCount > 4) requiredCount = 4;

						if (ImGui::SliderInt("Required Group Count", &requiredCount, 2, 4)) {
							po->i0 = requiredCount;
							syncToVariant();
						}

						if (requiredCount >= 2) {
							if (ImGui::InputInt("Required Group 2", &po->i1))
								syncToVariant();
						}
						if (requiredCount >= 3) {
							if (ImGui::InputInt("Required Group 3", &po->i2))
								syncToVariant();
						}
						if (requiredCount >= 4) {
							if (ImGui::InputInt("Required Group 4", &po->i3))
								syncToVariant();
						}
					}

					if (mode == 0) {
						ImGui::TextWrapped("Manual mode: opens/closes when a Lever or Switch activates the same Link Group ID.");
					}
					else if (mode == 1) {
						ImGui::TextWrapped("Single-group mode: opens when all ShootTargets in Link Group ID are cleared.");
					}
					else {
						ImGui::TextWrapped("Multi-group mode: opens when all listed ShootTarget groups are cleared.");
					}

					break;
				}

				case PuzzleKind::HealingMemory:
					if (ImGui::InputInt("Memory ID##i0", &po->i0)) syncToVariant();
					if (ImGui::InputInt("Required Memory ID##i1", &po->i1)) syncToVariant();
					if (ImGui::DragFloat("Heal Radius##f0", &po->f0, 1.0f, 0.0f, 10000.0f)) syncToVariant();
					if (ImGui::DragFloat("Memory Duration##f1", &po->f1, 0.1f, 0.0f, 1000.0f)) syncToVariant();

					{
						char buf0[512]{};
						char buf1[512]{};
						std::snprintf(buf0, sizeof(buf0), "%s", po->s0.c_str());
						std::snprintf(buf1, sizeof(buf1), "%s", po->s1.c_str());

						if (ImGui::InputTextMultiline("Narrative##s0", buf0, sizeof(buf0))) {
							po->s0 = buf0;
							syncToVariant();
						}
						if (ImGui::InputText("Overlay Image Path##s1", buf1, sizeof(buf1))) {
							po->s1 = buf1;
							syncToVariant();
						}
						ImGui::TextWrapped("Example: MemoryMaps/memory_01.png");
					}
					break;

				case PuzzleKind::BurrowFloor:
					if (ImGui::Checkbox("Ignore When Burrowed##i0", reinterpret_cast<bool*>(&po->i0))) {
						po->i0 = po->i0 ? 1 : 0;
						syncToVariant();
					}
					if (ImGui::Checkbox("Collapse Once##i1", reinterpret_cast<bool*>(&po->i1))) {
						po->i1 = po->i1 ? 1 : 0;
						syncToVariant();
					}
					if (ImGui::Checkbox("Create Fall Hazard##i2", reinterpret_cast<bool*>(&po->i2))) {
						po->i2 = po->i2 ? 1 : 0;
						syncToVariant();
					}
					if (ImGui::DragFloat("Collapse Delay##f0", &po->f0, 0.05f, 0.0f, 100.0f)) syncToVariant();
					if (ImGui::DragFloat("Respawn Delay##f1", &po->f1, 0.05f, 0.0f, 100.0f)) syncToVariant();
					break;

				default:
					ImGui::InputInt("i0", &po->i0); ImGui::SameLine();
					if (ImGui::Button("Save Params")) syncToVariant();
					break;
				}

				ImGui::TreePop();
			}
		}
		});
#endif
}

void GameApp::InitializeUiEditor() {
#if ENABLE_EDITOR
	if (!uiEditor) return;

	uiEditor->SetUiUpdateEnabledCallback([this](bool enabled) {
		layering_.SetUpdateEnabled(eng::LayerId::UI, enabled);
		});
#endif
}

#if ENABLE_EDITOR
unsigned int GameApp::EditorGetTextureID(const std::string& textureKey) {
	return ResourceManager::GetTexture(textureKey);
}

unsigned int GameApp::EditorGetPrefabTexture(const std::string& prefabName) {
	const Prefab* p = prefabManager.Get(prefabName);
	if (p && !p->texture.empty()) {
		return ResourceManager::GetTexture(p->texture);
	}
	return 0;
}
#endif

/**
 * @brief Registers game event observer handlers for common enemy state events.
 *
 * Attaches debug-log handlers to the GameApp observer for:
 * - "EnemySpottedPlayer"
 * - "EnemyLostPlayer"
 * - "EnemyStartedChasing"
 * - "EnemyStoppedChasing"
 *
 * Then registers those event names with the GameEvent system using the observer.
 */
void GameApp::InitializeGameEventCallbacks() {
	Messaging::Observer& gameObserver = m_gameObserver;

	gameObserver.AttachHandler("EnemySpottedPlayer", [](Messaging::IMessage* msg) {
		(void)msg;
		DebugConsole::Get().Info("[GameApp] Enemy spotted the player!\n");
		});

	gameObserver.AttachHandler("EnemyLostPlayer", [](Messaging::IMessage* msg) {
		(void)msg;
		DebugConsole::Get().Info("[GameApp] Enemy lost sight of the player.\n");
		});

	gameObserver.AttachHandler("EnemyStartedChasing", [](Messaging::IMessage* msg) {
		(void)msg;
		DebugConsole::Get().Info("[GameApp] Enemy started chasing!\n");
		});

	gameObserver.AttachHandler("EnemyStoppedChasing", [](Messaging::IMessage* msg) {
		(void)msg;
		DebugConsole::Get().Info("[GameApp] Enemy stopped chasing.\n");
		});

	// Register the events with the system
	gameEvents.Register("EnemySpottedPlayer", &gameObserver);
	gameEvents.Register("EnemyLostPlayer", &gameObserver);
	gameEvents.Register("EnemyStartedChasing", &gameObserver);
	gameEvents.Register("EnemyStoppedChasing", &gameObserver);
}
