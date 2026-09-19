/*
 * @file doorsystem.cpp
 * @author   Jethro Sung
 * @email    sung.h,jianlin.low,t.weiliangterril,w.kyele
 * @co-author Low JianLin, Tan Wei Liang Terril, Woh kye Le
 * @date      2025-11-7
 * @brief Implements door registration, collision detection,
 *        and inter-scene transitions.
*/
#include "World/doorsystem.h"
#include "Core/gameApp.h"
#include "Core/transform.h"
#include "Physics/collider.h"
#include "Core/resourceManager.h"
#include "Core/engine.hpp"
#include <iostream>
#include <algorithm>   
#include <cctype>
#include "Input/DebugConsole.hpp"
#include "Physics/ForceSystem.hpp"
#include "Graphics/renderer.h"
#include "Graphics/spritesheet.h"
#include "Graphics/mesh2d.h"
#include "World/mapGenerator.h"


/**
 * @brief Queue a door spawn for later processing.
 *
 * Sets up a pending door spawn that will be processed after a delay.
 * This is used for scene transitions where the player needs to appear
 * at a specific door after loading a new scene.
 *
 * @param doorName Name of the door to spawn the player at.
 * @param delayFrames Number of frames to wait before spawning.
 */
void DoorSystem::SetPendingSpawnDoor(const std::string& doorName, int delayFrames){
    if (doorName.empty())
        return;

    pendingSpawn.active = true;
    pendingSpawn.doorName = doorName;
    pendingSpawn.delayFrames = delayFrames;

    // Prevent retrigger while waiting to snap
    isTransitioning = true;
}

/**
    * @brief Registers a door link for the current scene.
    *
    * Adds a DoorLink entry containing the door entity, target scene,
    * target door, and arrival direction. These links are later used by
    * CheckDoorTransitions() and HandlePendingSpawn() to perform transitions.
    *
    * @param link Structure describing door entity and target scene info.
*/
void DoorSystem::RegisterDoor(const DoorLink& link){
    for (const DoorLink& d : doorLinks){
        if (d.entity == link.entity || d.name == link.name) {
            return;
        }
    }

    doorLinks.push_back(link);
}



/**
    * @brief Removes all registered door links for the current scene.
    *
    * Called automatically during scene loads to ensure door mappings from
    * previous scenes are not reused.
*/
void DoorSystem::ClearDoors() {
    doorLinks.clear();
}

/**
 * @brief Remove door links for destroyed entities.
 *
 * Cleans up door links when entities are destroyed to prevent
 * accessing invalid door entities. Also checks if the pending
 * spawn door still exists and cancels the transition if not.
 *
 * @param doomed Set of entity IDs that have been destroyed.
 */
void DoorSystem::UnregisterDoorsForEntities(const std::unordered_set<Entity>& doomed){
    if (doomed.empty())
        return;


    doorLinks.erase(
        std::remove_if(doorLinks.begin(), doorLinks.end(),
            [&](const DoorLink& d) {
                return doomed.find(d.entity) != doomed.end();
            }),
        doorLinks.end()
    );
   
    if (pendingSpawn.active){
        bool exists = false;
        for (const DoorLink& d : doorLinks){
            if (d.name == pendingSpawn.doorName){
                exists = true;
                break;
            }
        }

        if (!exists){
            pendingSpawn.active = false;
            isTransitioning = false;
        }
    }

 
}

/**
    * @brief Detects player overlap with any door collider and initiates a scene transition.
    *
    * Behavior:
    *  - Returns early if a transition is already happening or if cooldown is active.
    *  - Retrieves the player's Transform and Collider components.
    *  - Performs an AABB overlap check against each registered door.
    *  - When a door is entered:
    *      * Plays a "door_open" sound if available.
    *      * Logs a formatted message via DebugConsole.
    *      * Activates `pendingSpawn` with target scene + door name.
    *      * Immediately loads the target scene via GameApp::LoadLevel().
    *      * Starts a cooldown timer to prevent instant re-trigger.
    *
    * @param app Reference to the running GameApp.
*/
void DoorSystem::CheckDoorTransitions(GameApp& app) {
    if (isTransitioning || cooldown > 0.0f)
        return;

    auto* playerT = app.GetTransform(GameApp::playerEntity);
    auto* playerC = app.GetCollider(GameApp::playerEntity);
    if (!playerT || !playerC) return;

    Vector2 pPos = playerT->GetPosition();
    Vector2 pSize = playerC->size;

    // Check overlap with each door
    for (const DoorLink& d : doorLinks) {
        const Collider* c = app.TryGetCollider(d.entity);
        const Transform* t = app.TryGetTransform(d.entity);
        if (!c || !t) continue;

        Vector2 dPos = t->GetPosition();
        Vector2 dSize = c->size;

        // AABB overlap check
        Vector2 pMin = pPos;
        Vector2 pMax = { pPos.x + pSize.x, pPos.y + pSize.y };
        Vector2 dMin = dPos;
        Vector2 dMax = { dPos.x + dSize.x, dPos.y + dSize.y };

        bool overlap = (pMin.x <= dMax.x && pMax.x >= dMin.x &&
            pMin.y <= dMax.y && pMax.y >= dMin.y);

        if (!overlap)
            continue;

        // Check for required keys for specific levels
        if (auto* pc = app.GetController(GameApp::playerEntity)) {
            bool hasAccess = true;
            std::string missingKeyMsg;

            // Level 2 requires Projectile Key
            if (d.targetScene.find("Level2") != std::string::npos) {
                if (!pc->HasKeyType(PlayerAbility::PROJECTILE)) {
                    hasAccess = false;
                    missingKeyMsg = "Locked! Requires Projectile Key.";
                }
            }
            // Level 3 requires Burrow Key
            else if (d.targetScene.find("Level3") != std::string::npos) {
                if (!pc->HasKeyType(PlayerAbility::BURROW)) {
                    hasAccess = false;
                    missingKeyMsg = "Locked! Requires Burrow Key.";
                }
            }
            // Level 4 requires All Keys (Burrow, Heal, Projectile)
            else if (d.targetScene.find("Level4") != std::string::npos) {
                bool hasBurrow = pc->HasKeyType(PlayerAbility::BURROW);
                bool hasHeal = pc->HasKeyType(PlayerAbility::HEAL);
                bool hasProjectile = pc->HasKeyType(PlayerAbility::PROJECTILE);

                if (!hasBurrow || !hasHeal || !hasProjectile) {
                    hasAccess = false;
                    missingKeyMsg = "Locked! Requires Burrow, Heal, and Projectile Keys.";
                }
            }
            // Level 5 requires Boss 4 defeated
            else if (d.targetScene.find("Level5") != std::string::npos) {
                if (!MapGenerator::IsBossDefeated(4)) {
                    hasAccess = false;
                    missingKeyMsg = "Locked! Defeat the boss in Room 4 first.";
                }
            }

            if (!hasAccess) {
                DebugConsole::Get().Warning(missingKeyMsg);
                cooldown = 1.0f; // Brief cooldown to prevent spamming
                return;
            }
        }

        DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[Door] Transitioning to : " + d.targetScene, " via door: " + d.name, "\n");

        // Queue Scene Load and Player Reposition
        isTransitioning = true;
        std::string fullPath = d.targetScene;
        if (fullPath.find("/") == std::string::npos)
            fullPath = "scene/" + fullPath;

        if (isTransitioning) {
            ResourceManager::PlaySfx("door_open", 0.7f);
        }


        if (requestScene) {
            // Mark door as entered
            enteredDoors.insert(d.name + "|" + d.targetScene);
            requestScene(fullPath, d.targetDoor);
        }
        else {
            DebugConsole::Get().Warning(
                "[Door] requestScene callback not set. Transition aborted.\n"
            );
            isTransitioning = false;
        }

        cooldown = 2.f;// 2 seconds cooldown to prevent re-triggering
        return;
    }
}

void DoorSystem::DrawActionHints(Renderer& renderer, GameApp& app) {
	// Only show action hints in the labyrinth scene
	std::string scenePath = app.sceneManager.GetCurrentGameplayScene();
	
	// Case-insensitive check for "labyrinth.json"
	std::string lowerPath = scenePath;
	std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	
	
	if (lowerPath.empty() || lowerPath.find("labyrinth.json") == std::string::npos) {
		return;
	}

	const SpriteSheet* hintSS = ResourceManager::GetSpriteSheet("ActionHint");
	if (!hintSS) return;

	auto* pc = app.GetController(GameApp::playerEntity);
	if (!pc) return;

	// Viewport clamping logic
	Camera2D& cam = app.GetCamera();

	for (const DoorLink& d : doorLinks) {
		// 1. Check if door is entered
		if (enteredDoors.find(d.name + "|" + d.targetScene) != enteredDoors.end()) {
			continue;
		}

		// 2. Check if door is unlocked 
		int levelIndex = 1;
		std::string s = d.targetScene;
		size_t p = s.rfind("Level");
		if (p != std::string::npos) {
			p += 5;
			size_t q = p;
			while (q < s.size() && std::isdigit((unsigned char)s[q])) ++q;
			if (q > p) {
				try { levelIndex = std::stoi(s.substr(p, q - p)); }
				catch (...) {}
			}
		}

		bool unlocked = true;
		if (levelIndex == 2) {
			if (!pc->HasKeyType(PlayerAbility::PROJECTILE)) unlocked = false;
		}
		else if (levelIndex == 3) {
			if (!pc->HasKeyType(PlayerAbility::BURROW)) unlocked = false;
		}
		else if (levelIndex == 4) {
			if (!pc->HasKeyType(PlayerAbility::BURROW) ||
				!pc->HasKeyType(PlayerAbility::HEAL) ||
				!pc->HasKeyType(PlayerAbility::PROJECTILE)) unlocked = false;
		}
		else if (levelIndex == 5) {
			if (!MapGenerator::IsBossDefeated(4)) unlocked = false;
		}

		if (!unlocked) continue;

		// 3. Render hint
		const Transform* t = app.TryGetTransform(d.entity);
		const Collider* c = app.TryGetCollider(d.entity);
		if (!t || !c) continue;

		Vector2 dPos = t->GetPosition();
		Vector2 dSize = c->size;

		// Calculate hint position (beside the door)
		// Default to above and centered
		float hintX = dPos.x + (dSize.x - 52.0f) * 0.5f;
		float hintY = dPos.y - 52.0f - 10.0f;

		const float tileSize = 100.0f;
		const float centerOffset = (tileSize - 52.0f) * 0.5f;
		const float step = centerOffset;

		switch (d.arrivalDir) {
		case DoorArrivalDir::Top:
			hintX = dPos.x + (dSize.x - 52.0f) * 0.5f;
			hintY = dPos.y - 52.0f - step;
			break;
		case DoorArrivalDir::Bottom:
			hintX = dPos.x + (dSize.x - 52.0f) * 0.5f;
			hintY = dPos.y + dSize.y + step;
			break;
		case DoorArrivalDir::Left:
			hintX = dPos.x + dSize.x + step;
			hintY = dPos.y + (dSize.y - 52.0f) * 0.5f;
			break;
		case DoorArrivalDir::Right:
			hintX = dPos.x - 52.0f - step;
			hintY = dPos.y + (dSize.y - 52.0f) * 0.5f;
			break;
		default:
			break;
		}

		float pulseFreq = 4.0f;
		float pulseScale = 1.05f + 0.15f * std::sin(totalTime * pulseFreq * 2.0f * 3.14159f);
		float finalSize = 52.0f * pulseScale;
		float centerOff = (52.0f - finalSize) * 0.5f;

		// Compute screen-space top-left and actual screen-space size via world-to-screen deltas
		Vector2 topLeftWorld(hintX + centerOff, hintY + centerOff);
		Vector2 screenPos = cam.WorldToScreen(topLeftWorld);
		float screenW = cam.viewportWidth;
		float screenH = cam.viewportHeight;
		
		Vector2 rightWorld(hintX + centerOff + finalSize, hintY + centerOff);
		Vector2 downWorld(hintX + centerOff, hintY + centerOff + finalSize);
		Vector2 rightScreen = cam.WorldToScreen(rightWorld);
		Vector2 downScreen = cam.WorldToScreen(downWorld);
		float iconScreenSizeX = std::abs(rightScreen.x - screenPos.x);
		float iconScreenSizeY = std::abs(downScreen.y - screenPos.y);

		if (screenPos.x < 0.0f || screenPos.x > screenW - iconScreenSizeX ||
			screenPos.y < 0.0f || screenPos.y > screenH - iconScreenSizeY) {
			
			screenPos.x = std::clamp(screenPos.x, 0.0f, std::max(0.0f, screenW - iconScreenSizeX));
			screenPos.y = std::clamp(screenPos.y, 0.0f, std::max(0.0f, screenH - iconScreenSizeY));

			Vector2 clampedWorld = cam.ScreenToWorld(screenPos); // back to world top-left
			hintX = clampedWorld.x - centerOff;
			hintY = clampedWorld.y - centerOff;
		}

		int totalFrames = hintSS->cols * hintSS->rows;
		if (totalFrames <= 0) continue;
		
		float animSpeed = 1.0f;
		int frameIdx = static_cast<int>((totalTime * animSpeed) / hintSS->frameDuration) % totalFrames;

		Shader* sh = renderer.GetShader();
		if (sh) {
			sh->Use();
			sh->SetInt("u_Frame", frameIdx);
			sh->SetInt("u_Cols", hintSS->cols);
			sh->SetVec2("u_FrameSize", Vector2(
				(float)hintSS->frameW / (float)hintSS->texture.Width(),
				(float)hintSS->frameH / (float)hintSS->texture.Height()
			));
		}

		// Draw the hint quad with scaling
		Matrix3x3 model = Matrix3x3::BuildTranslation(hintX + centerOff, hintY + centerOff) * Matrix3x3::BuildScaling(finalSize, finalSize);

		renderer.DrawMesh(*app.GetQuadMesh(), model, Vector3(1, 1, 1), hintSS->texture.ID());

		// Reset shader uniforms after drawing spritesheet
		if (sh) {
			sh->SetInt("u_Frame", 0);
			sh->SetInt("u_Cols", 1);
			sh->SetVec2("u_FrameSize", Vector2(1.f, 1.f));
		}
	}
}

void DoorSystem::UpdateDoorTextures(GameApp& app) {
	auto* pc = app.GetController(GameApp::playerEntity);
	if (!pc) return;

	for (const DoorLink& d : doorLinks) {
		MeshRenderer* mr = app.GetRenderer(d.entity);
		if (!mr) continue;

		// Parse level index
		int levelIndex = 1;
		if (d.targetScene.empty()) {
			// If no target scene, skip
			continue;
		}

		std::string s = d.targetScene;
		size_t p = s.rfind("Level");
		if (p != std::string::npos) {
			p += 5;
			size_t q = p;
			while (q < s.size() && std::isdigit((unsigned char)s[q])) ++q;
			if (q > p) {
				try { levelIndex = std::stoi(s.substr(p, q - p)); }
				catch (...) {}
			}
		}

		// Determine if unlocked
		bool unlocked = true;
		if (levelIndex == 2) {
			if (!pc->HasKeyType(PlayerAbility::PROJECTILE)) unlocked = false;
		}
		else if (levelIndex == 3) {
			if (!pc->HasKeyType(PlayerAbility::BURROW)) unlocked = false;
		}
		else if (levelIndex == 4) {
			if (!pc->HasKeyType(PlayerAbility::BURROW) ||
				!pc->HasKeyType(PlayerAbility::HEAL) ||
				!pc->HasKeyType(PlayerAbility::PROJECTILE)) unlocked = false;
		}
		else if (levelIndex == 5) {
			if (!MapGenerator::IsBossDefeated(4)) unlocked = false;
		}

		// Determine texture
		std::string texName = "door";
		if (unlocked) {
			texName = "door_unlocked_lvl" + std::to_string(levelIndex);
		}
		else {
			texName = "door_lvl" + std::to_string(levelIndex);
		}

		GLuint texID = ResourceManager::GetTexture(texName);
		if (texID == 0) {
			// Fallback
			if (unlocked) texID = ResourceManager::GetTexture("door_unlocked_lvl1");
			if (texID == 0) texID = ResourceManager::GetTexture("door");

		}

		if (texID != 0 && mr->GetTexture() != texID) {
			mr->SetTexture(texID);
		}
	}
}

/**
    * @brief Teleports the player to the correct destination door after the new scene loads.
    *
    * This function executes a few frames after the scene is loaded to ensure
    * all door entities in the new scene are registered.
    *
    * Workflow:
    *  - Waits until `pendingSpawn.delayFrames` reaches zero.
    *  - Finds the destination door by matching door name.
    *  - Computes the spawn offset using the door's arrival direction
    *    (Top, Bottom, Left, Right) to avoid spawning the player directly
    *    inside the door collider.
    *  - Moves the player to the computed world position.
    *  - Plays a "door_close" SFX if available.
    *  - Clears the pendingSpawn flag and completes the transition.
    *
    * @param app Reference to the GameApp instance.
*/
void DoorSystem::HandlePendingSpawn(GameApp& app) {
    if (!pendingSpawn.active)
        return;

    if (pendingSpawn.delayFrames-- > 0)
        return;

    Entity destDoor = INVALID_ENTITY;
    for (const DoorLink& d : doorLinks)
        if (d.name == pendingSpawn.doorName)
            destDoor = d.entity;

    if (destDoor == INVALID_ENTITY) {
        DebugConsole::Get().Warning("Destination door not found\n");
        pendingSpawn.active = false;
        isTransitioning = false;
        return;
    }

    const Transform* destT = app.TryGetTransform(destDoor);
    const Collider* destC = app.TryGetCollider(destDoor);
    if (!destT || !destC) {
        pendingSpawn.active = false;
        isTransitioning = false;
        return;
    }

    Vector2 pos = destT->GetPosition();
    Vector2 dSize = destC->size;

    const Collider* playerC = app.TryGetCollider(GameApp::playerEntity);
    if (!playerC) { pendingSpawn.active = false; isTransitioning = false; return; }
    Vector2 pSize = playerC->size;

    DoorArrivalDir dir = DoorArrivalDir::None;
    for (const DoorLink& l : doorLinks)
        if (l.entity == destDoor)
            dir = l.arrivalDir;

    const float pad = 2.f;
    switch (dir) {
    case DoorArrivalDir::Top:    pos.y += dSize.y + pad; break;
    case DoorArrivalDir::Bottom: pos.y -= pSize.y + pad; break;
    case DoorArrivalDir::Left:   pos.x -= pSize.x + pad; break;
    case DoorArrivalDir::Right:  pos.x += dSize.x + pad; break;
    default: break;
    }

    /*if (auto* playerCtrl = app.GetController(GameApp::playerEntity)) {
         playerCtrl->ResetSpeed();
       }*/

   /* if (auto* playerCtrl = app.GetController(GameApp::playerEntity)) {
        playerCtrl->speedTrigger();
    }*/


    if (auto* tP = app.GetTransform(GameApp::playerEntity)) {
        tP->SetPosition(pos);
    }


    //if (auto* tP = app.GetTransform(GameApp::playerEntity)) {
    //    tP->SetPosition(pos);
    //
    //    // --- IMPORTANT: clear leftover motion after teleport ---
    //    // 1) clear ForceSystem speed component
    //    if (auto it = temp_spdComponent.find(GameApp::playerEntity);
    //        it != temp_spdComponent.end())
    //    {
    //        it->second.speed = { 0.f, 0.f };
    //    }
    //
    //
    //    // 2) reset controller internal timer + any proxy-owned speed
    //    if (auto* pc = app.GetController(GameApp::playerEntity)) {
    //        pc->ResetSpeed(pos);
    //    }
    //
    //    // 3) optional: sync backup/previous position so collision/rollback doesn't "snap back"
    //    tP->SavePreviousState();
    //
    //    //}
    //
    //    DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[Door] Player repositioned at (" + std::to_string(pos.x) + "," + std::to_string(pos.y) + ")\n");
    //
    //    if (isTransitioning) {
    //        ResourceManager::PlaySfx("door_close", 0.4f);
    //    }
    //
    //    pendingSpawn.active = false;
    //    isTransitioning = false;
    //}

    DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[Door] Player repositioned at (" + std::to_string(pos.x) + "," + std::to_string(pos.y) + ")\n");

    if (isTransitioning) {
        ResourceManager::PlaySfx("door_close", 0.5f);
    }

    pendingSpawn.active = false;
    isTransitioning = false;
}

/**
    * @brief Updates the door cooldown timer to prevent repeated triggering.
    *
    * Reduces the cooldown using the engine's global delta time. When cooldown
    * reaches zero, doors can be activated again.
*/
void DoorSystem::UpdateCooldown() {
    if (cooldown > 0.0f) {
        cooldown -= static_cast<float>(eng::deltaTime());
        if (cooldown < 0.0f) cooldown = 0.0f;
    }
    totalTime += static_cast<float>(eng::deltaTime());
}

/**
    * @brief Immediately moves the player to a specified door in the current scene.
    *
    * This function is used by the editor to allow designers to test door
    * transitions without performing a full scene-load cycle.
    *
    * Behavior:
    *  - Searches for the door by name among registered DoorLinks.
    *  - Retrieves the destination door's Transform and Collider.
    *  - Computes the arrival offset based on DoorArrivalDir.
    *  - Moves the player to the adjusted position.
    *  - Optionally plays a "door_close" sound.
    *  - Prints a formatted message to the DebugConsole.
    *
    * @param app       GameApp instance providing access to ECS components.
    * @param doorName  Name of the door in the current scene.
    * @param playSound Whether to play the "door_close" SFX.
*/
void DoorSystem::SnapPlayerToDoor(GameApp& app, const std::string& doorName, bool playSound)
{
    // Find the door entity by its name in the current scene
    Entity destDoor = INVALID_ENTITY;
    for (const DoorLink& d : doorLinks) {
        if (d.name == doorName) {
            destDoor = d.entity;
            break;
        }
    }

    if (destDoor == INVALID_ENTITY) {
        DebugConsole::Get().Warning("[Door] SnapPlayerToDoor: door '" + doorName + "' not found in current scene.\n");
        return;
    }

    const Transform* destT = app.TryGetTransform(destDoor);
    const Collider* destC = app.TryGetCollider(destDoor);
    const Collider* playerC = app.TryGetCollider(GameApp::playerEntity);
    if (!destT || !destC || !playerC)
        return;

    Vector2 pos = destT->GetPosition();
    Vector2 dSize = destC->size;
    Vector2 pSize = playerC->size;

    DoorArrivalDir dir = DoorArrivalDir::None;
    for (const DoorLink& l : doorLinks) {
        if (l.entity == destDoor) {
            dir = l.arrivalDir;
            break;
        }
    }

    const float pad = 2.f;
    switch (dir) {
    case DoorArrivalDir::Top:    pos.y += dSize.y + pad; break;
    case DoorArrivalDir::Bottom: pos.y -= pSize.y + pad; break;
    case DoorArrivalDir::Left:   pos.x -= pSize.x + pad; break;
    case DoorArrivalDir::Right:  pos.x += dSize.x + pad; break;
    default: break;
    }

    if (auto* tP = app.GetTransform(GameApp::playerEntity))
        tP->SetPosition(pos);

    DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[Door] (Editor) Player snapped to door '" + doorName + "' at (" + std::to_string(pos.x) + "," + std::to_string(pos.y) + ")\n");

    if (playSound) {
        if (auto sid = ResourceManager::GetAudio("door_close"); sid >= 0) {
            Audio::PlayDesc pd;
            pd.bus = Audio::Bus::Sfx;
            pd.loop = false;
            pd.gain = 0.5f;
            Audio::Play(sid, pd);
        }
    }
}

/**
 * @brief Clear all doors and reset state for non-persistent doors.
 *
 * Removes all door links and resets the door system state.
 * Used when transitioning between scenes or resetting the game.
 */
void DoorSystem::ClearNonPersistentDoors(){
    doorLinks.clear();
    pendingSpawn.active = false;
    isTransitioning = false;
    cooldown = 0.0f;

    DebugConsole::Get().Info("[DoorSystem] Cleared non-persistent doors\n");
}
