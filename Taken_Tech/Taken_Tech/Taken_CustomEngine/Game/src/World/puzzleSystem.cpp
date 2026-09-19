/**
 * @file      puzzlesystem.cpp
 * @author    Jethro Sung, Sng Swee Yong Dillon
 * @email     sung.h, sweeyongdillon.sng
 * @date      2026-02-04
 *
 * @brief     Implements PuzzleSystem, the gameplay system that evaluates and
 *            applies puzzle interactions (levers, plates, switches, locks,
 *            burrow walls, and shoot targets).
 *
 * This implementation supports:
 *  - AABB overlap checks between the player and puzzle activators.
 *  - Group-based linking via PuzzleObject::groupId, allowing one activator to
 *    control multiple puzzle objects efficiently through ApplyGroup().
 *  - ShootTarget progression via OnProjectileHit(), called by ProjectileSystem
 *    when projectiles collide with puzzle entities.
 *  - Runtime collider state changes (solid vs trigger) for BurrowWall and
 *    DoorLock targets when a group is activated/deactivated.
 *
 * Key responsibilities:
 *  - Update(): detects player interaction with puzzle activators and triggers
 *    group activation (e.g., lever toggle, plate overlap, switch toggle).
 *  - ApplyGroup(): applies an "active" state across all puzzle objects in the
 *    specified group, including collider mode swaps for passability.
 *  - OnProjectileHit(): increments ShootTarget hit counters and activates a
 *    linked group once all targets in that group are cleared.
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */
#include "World/puzzleSystem.h"
#include "Core/gameApp.h"
#include "Core/engine.hpp"
#include "Core/transform.h"
#include "Core/randomiser.h"
#include "Physics/collider.h"
#include "Input/DebugConsole.hpp"
#include "Mechanics/interaction.hpp"

#include "puzzleObject.h"
#include "Core/resourceManager.h"
#include "Graphics/meshrenderer.h"
#include <cmath>
#include "Graphics/camera2d.h"
#include "Graphics/renderer.h"
#include "Math/matrix3x3.h"
#include "Font/FontRenderer.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include "Core/assetsPath.h"

PuzzleSystem::PuzzleSystem(GameApp& a) : app(a) {
    activeOverlay = new MemoryVisionOverlay();
}

PuzzleSystem::~PuzzleSystem() {
    delete activeOverlay;
}

/**
 * @brief Resets the puzzle system state.
 * 
 * Clears the list of revealed memories and deactivates any active memory overlay.
 * Typically called when reloading a level or restarting the game.
 */
void PuzzleSystem::Reset()
{
    revealedMemories.clear();
    persistentStates.clear();
    if (activeOverlay)
        activeOverlay->Deactivate();
}

void PuzzleSystem::SaveAllPuzzleStates()
{
    for (const auto& [e, po] : app.puzzleObjects) {
        std::string name = app.GetEntityNameByEntity(e);
        if (name.empty()) continue;

        PersistentPuzzleState state;
        state.active = po.active;
        state.i0 = po.i0;
        state.i1 = po.i1;
        state.i2 = po.i2;
        state.i3 = po.i3;
        state.f0 = po.f0;
        state.f1 = po.f1;
        state.f2 = po.f2;
        state.f3 = po.f3;
        state.s0 = po.s0;
        state.s1 = po.s1;

        persistentStates[name] = state;
    }
}

void PuzzleSystem::RestoreAllPuzzleStates()
{
    for (auto& [e, po] : app.puzzleObjects) {
        std::string name = app.GetEntityNameByEntity(e);
        if (name.empty()) continue;

        auto it = persistentStates.find(name);
        if (it != persistentStates.end()) {
            const PersistentPuzzleState& state = it->second;
            po.active = state.active;
            po.i0 = state.i0;
            po.i1 = state.i1;
            po.i2 = state.i2;
            po.i3 = state.i3;
            po.f0 = state.f0;
            po.f1 = state.f1;
            po.f2 = state.f2;
            po.f3 = state.f3;
            po.s0 = state.s0;
            po.s1 = state.s1;

            // Re-apply visuals/physics based on restored state
            if (po.kind == PuzzleKind::Lever) {
                SetLeverVisual(e, po.active);
            }
            else if (po.kind == PuzzleKind::ShootTarget) {
                SetShootTargetVisual(e, po.i1 >= po.i0);
            }
            else if (po.kind == PuzzleKind::DoorLock) {
                po.active = !state.active; // force update
                SetDoorLockOpen(e, state.active);
            }
            else if (po.kind == PuzzleKind::BurrowWall) {
                Collider* c = app.GetCollider(e);
                if (c) {
                    Vector2 oldSize = c->size;
                    app.RemoveColliderComponent(e);
                    app.AddCollider(e, oldSize, /*trigger*/ !po.active);
                }
            }
            else if (po.kind == PuzzleKind::BurrowFloor) {
                if (!po.active) {
                    if (auto* mr = app.GetRenderer(e)) mr->SetMesh(nullptr);
                    if (po.i2 != 1) app.RemoveColliderComponent(e);
                }
            }
            else if (po.kind == PuzzleKind::HealingMemory) {
                if (po.active) {
                    if (auto* mr = app.GetRenderer(e)) {
                        GLuint healedTex = ResourceManager::GetTexture("Corpse_Healed");
                        if (healedTex == 0) healedTex = ResourceManager::GetTexture("healingMemory_On");
                        if (healedTex != 0) mr->SetTexture(healedTex);
                    }
                }
            }
        }
    }
}

bool PuzzleSystem::AabbOverlap(const Vector2& aPos, const Vector2& aSize,
    const Vector2& bPos, const Vector2& bSize)
{
    Vector2 aMin = aPos;
    Vector2 aMax = { aPos.x + aSize.x, aPos.y + aSize.y };
    Vector2 bMin = bPos;
    Vector2 bMax = { bPos.x + bSize.x, bPos.y + bSize.y };

    return (aMin.x < bMax.x && aMax.x > bMin.x &&
        aMin.y < bMax.y && aMax.y > bMin.y);
}

namespace
{
    inline int ClampInt(int v, int lo, int hi)
    {
        return (v < lo) ? lo : (v > hi) ? hi : v;
    }

    inline float Distance(const Vector2& a, const Vector2& b)
    {
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        return std::sqrt(dx * dx + dy * dy);
    }
}

/**
 * @brief Sets the open/closed state of a DoorLock entity.
 * 
 * Updates the entity's collider to be a trigger (passable) when open, or solid when closed.
 * Also adjusts the visual scale to hide/show the door.
 * 
 * @param e The entity ID of the door.
 * @param open True to open the door, false to close it.
 */
void PuzzleSystem::SetDoorLockOpen(Entity e, bool open)
{
    auto it = app.puzzleObjects.find(e);
    if (it == app.puzzleObjects.end())
        return;

    PuzzleObject& po = it->second;
    if (po.kind != PuzzleKind::DoorLock)
        return;

    // Avoid rebuilding colliders every frame if nothing changed
    if (po.active == open)
        return;

    po.active = open;

    Collider* c = app.GetCollider(e);
    if (c)
    {
        Vector2 oldSize = c->size;

        app.RemoveColliderComponent(e);
        // open => trigger true; closed => trigger false
        app.AddCollider(e, oldSize, /*trigger*/ open);

        // visually disappear when opened
        if (Transform* t = app.GetTransform(e))
        {
            if (open)
                t->SetScale({ 0.f, 0.f });
            else
                t->SetScale(oldSize);
        }
    }
}

/**
 * @brief Checks if a door lock's requirements are met to be opened.
 * 
 * Evaluates the conditions for opening a door, which can depend on:
 * - A single group ID (legacy mode).
 * - Multiple group IDs (up to 4) being cleared.
 * - Whether linked ShootTarget groups are fully cleared.
 * 
 * @param doorPo The PuzzleObject component of the door.
 * @return True if all requirements are met, false otherwise.
 */
bool PuzzleSystem::IsDoorLockRequirementsMet(const PuzzleObject& doorPo)
{
    if (doorPo.kind != PuzzleKind::DoorLock)
        return false;

    // Manual-group doors: opened only via ApplyGroup lever
    if (doorPo.i0 == -1)
        return false;

    // Manual doors: groupId==0 and i0==0 => PuzzleSystem won't auto-open it
    if (doorPo.groupId == 0 && doorPo.i0 == 0)
        return false;

    // Legacy: i0==0 => require only groupId
    if (doorPo.i0 == 0)
        return CheckShootTargetGroupCleared(doorPo.groupId);

    // Multi-group mode: i0 = count (1..4)
    const int count = ClampInt(doorPo.i0, 1, 4);

    int groups[4] = { doorPo.groupId, doorPo.i1, doorPo.i2, doorPo.i3 };

    for (int i = 0; i < count; ++i)
    {
        const int g = groups[i];

        // If a required slot is 0, treat it as not satisfied
        if (g == 0)
            return false;

        if (!CheckShootTargetGroupCleared(g))
            return false;
    }

    return true;
}

/**
 * @brief Applies an "active" state to all puzzle objects in a group.
 * 
 * Iterates through all puzzle objects and updates those matching the given groupId.
 * Specific behaviors include:
 * - BurrowWall: Toggles collider trigger state (solid <-> passable).
 * - DoorLock: Opens/closes if configured to respond to group activation.
 * 
 * @param groupId The ID of the target group.
 * @param active The new state to apply.
 */
void PuzzleSystem::ApplyGroup(int groupId, bool active)
{
    // Iterate all puzzle objects
    for (auto& kv : app.puzzleObjects)
    {
        Entity e = kv.first;
        PuzzleObject& po = kv.second;

        if (po.groupId != groupId)
            continue;

        // Example target type: BurrowWall.
        if (po.kind == PuzzleKind::BurrowWall)
        {
            po.active = active;

            // solid  => trigger=false
            // pass   => trigger=true
            Collider* c = app.GetCollider(e);
            if (c)
            {
                Vector2 oldSize = c->size;

                // remove + re-add with same size but different trigger
                app.RemoveColliderComponent(e);
                app.AddCollider(e, oldSize, /*trigger*/ !active);
            }
        }

        // DoorLock: becomes passable (trigger) when activated, solid when not.
        if (po.kind == PuzzleKind::DoorLock)
        {
            if (!(po.i0 == 0 || po.i0 == -1))
                continue;

            SetDoorLockOpen(e, active);
        }
    }
}

/**
 * @brief Checks if all ShootTarget objects in a specific group have been cleared.
 * 
 * @param groupId The group ID to check.
 * @return True if at least one target exists and all targets in the group are cleared.
 */
bool PuzzleSystem::CheckShootTargetGroupCleared(int groupId)
{
    bool foundAnyTarget = false;

    for (auto& kv : app.puzzleObjects)
    {
        PuzzleObject& po = kv.second;

        if (po.groupId != groupId)
            continue;

        if (po.kind == PuzzleKind::ShootTarget)
        {
            foundAnyTarget = true;

            const int requiredHits = po.i0;
            const int currentHits = po.i1;

            if (currentHits < requiredHits)
                return false;
        }
    }

    return foundAnyTarget;
}

/**
 * @brief Updates the texture of a ShootTarget entity based on its state.
 * 
 * @param e The entity ID.
 * @param cleared True if the target has been cleared (activated).
 */
void PuzzleSystem::SetShootTargetVisual(Entity e, bool cleared)
{
    auto* mr = app.GetRenderer(e);
    if (!mr) return;

    // tex
    const char* texKey = cleared ? "PuzzleGenerator_On" : "PuzzleGenerator";

    GLuint texID = ResourceManager::GetTexture(texKey);
    if (texID != 0)
        mr->SetTexture(texID);
}

/**
 * @brief Updates the texture of a Lever entity based on its state.
 * 
 * @param e The entity ID.
 * @param active True if the lever is in the active (pulled) position.
 */
void PuzzleSystem::SetLeverVisual(Entity e, bool active)
{
    auto* mr = app.GetRenderer(e);
    if (!mr) return;

    const char* texKey = active ? "LeverActivated" : "Lever";

    GLuint texID = ResourceManager::GetTexture(texKey);
    if (texID != 0)
        mr->SetTexture(texID);
}

/**
 * @brief Handles projectile impacts on puzzle entities.
 * 
 * Increments hit counters for ShootTarget objects. If a target is cleared,
 * it updates visuals, plays sounds, and checks if the entire group is cleared
 * to trigger linked events (like opening doors).
 * 
 * @param puzzleEntity The entity that was hit.
 * @param fromPlayer True if the projectile came from the player.
 */
void PuzzleSystem::OnProjectileHit(Entity puzzleEntity, bool fromPlayer)
{
    auto it = app.puzzleObjects.find(puzzleEntity);
    if (it == app.puzzleObjects.end())
        return;

    PuzzleObject& po = it->second;

    if (po.kind != PuzzleKind::ShootTarget)
        return;

    // Check if we require player projectiles
    bool requiresPlayer = (po.i2 == 1);
    if (requiresPlayer && !fromPlayer)
        return;

    int requiredHits = po.i0; // i0 = required hits
    int currentHits = po.i1;  // i1 = current hits

    // Already cleared
    if (currentHits >= requiredHits)
        return;

    // Increment hit count
    po.i1 = currentHits + 1;

    DebugConsole::Get().AddFormattedMessage(
        LogLevel::Info,
        "[Puzzle] ShootTarget hit! ",
        std::to_string(po.i1) + "/" + std::to_string(requiredHits),
        ""
    );

    // Check if this target is now cleared
    if (po.i1 >= requiredHits)
    {
        DebugConsole::Get().AddFormattedMessage(
            LogLevel::Info,
            "[Puzzle] ShootTarget CLEARED!",
            "",
            ""
        );

        // swap generator tex to on
        if (!po.active)
        {
            po.active = true;
            SetShootTargetVisual(puzzleEntity, true);

            if (Transform* t = app.GetTransform(puzzleEntity)) {
                Vector2 pos = t->GetPosition();

                // Play "On" sound at the generator's position
                ResourceManager::PlaySfx(app, "generator_triggered",
                    glm::vec2(pos.x, pos.y),
                    100.0f, 500.0f, 1.0f);
            }
        }

        // Check if all targets in the group are cleared
        if (CheckShootTargetGroupCleared(po.groupId))
        {
            DebugConsole::Get().AddFormattedMessage(
                LogLevel::Info,
                "[Puzzle] All ShootTargets in group " + std::to_string(po.groupId) + " CLEARED!",
                " Activating linked objects...",
                ""
            );

            // Activate all linked puzzle objects in the group
            ApplyGroup(po.groupId, true);

            for (auto& [e, targetPo] : app.puzzleObjects)
            {
                if (targetPo.groupId == po.groupId &&
                    (targetPo.kind == PuzzleKind::DoorLock))
                {
                    if (Transform* targetT = app.GetTransform(e)) {
                        Vector2 targetPos = targetT->GetPosition();

                        // Spatial hint: Player hears the door/wall open in the distance
                        ResourceManager::PlaySfx(app, "generator_activated",
                            glm::vec2(targetPos.x, targetPos.y),
                            100.0f, 800.0f, 2.0f);
                        break; // Only play the solved sound once
                    }
                }
            }
        }
    }
}

/**
 * @brief Triggered when the player uses the Heal ability.
 * 
 * Scans for nearby HealingMemory puzzle objects (corpses). If one is found
 * and requirements are met, it reveals a memory, updates visuals, and shows
 * the memory overlay.
 * 
 * @param playerPos The player's current position.
 * @return True if a memory was successfully revealed.
 */
bool PuzzleSystem::OnPlayerHealUsed(const Vector2& playerPos)
{
    Entity closestCorpse = INVALID_ENTITY;
    float closestDist = 999999.0f;

    for (auto& [e, po] : app.puzzleObjects)
    {
        if (po.kind != PuzzleKind::HealingMemory || po.active)
            continue;

        // Check sequential requirement
        if (po.i1 >= 0) // i1 = requiredMemoryId
        {
            bool found = false;
            for (int id : revealedMemories)
            {
                if (id == po.i1) {
                    found = true;
                    break;
                }
            }
            if (!found) continue;
        }

        Transform* t = app.GetTransform(e);
        if (!t) continue;

        float dist = Distance(playerPos, t->GetPosition());
        if (dist <= po.f0) // f0 = healRadius
        {
            if (dist < closestDist)
            {
                closestDist = dist;
                closestCorpse = e;
            }
        }
    }

    if (closestCorpse == INVALID_ENTITY)
        return false;

    PuzzleObject& po = app.puzzleObjects[closestCorpse];
    po.active = true; // hasBeenHealed

    // Visual update: Switch to healed corpse texture
    if (auto* mr = app.GetRenderer(closestCorpse))
    {
        GLuint healedTex = ResourceManager::GetTexture("Corpse_Healed");
        if (healedTex == 0) healedTex = ResourceManager::GetTexture("healingMemory_On"); // Fallback

        if (healedTex != 0)
            mr->SetTexture(healedTex);
    }

    if (activeOverlay)
    {
        activeOverlay->Activate(
            po.i0, // memoryId
            po.s0, // narrative
            po.s1, // map image path
            po.f1  // duration
        );
    }

    revealedMemories.push_back(po.i0);

    DebugConsole::Get().AddFormattedMessage(
        LogLevel::Info,
        "[HealMemory] Soldier's memory revealed: Memory #",
        std::to_string(po.i0),
        "\n"
    );

    return true;
}

/**
 * @brief Checks if an entity is a ShootTarget puzzle object.
 * 
 * @param e The entity ID.
 * @return True if the entity is a ShootTarget.
 */
bool PuzzleSystem::IsShootTarget(Entity e) const
{
    auto it = app.puzzleObjects.find(e);
    if (it == app.puzzleObjects.end())
        return false;

    return it->second.kind == PuzzleKind::ShootTarget;
}

/**
 * @brief Updates the logic for BurrowFloor tiles (fragile floors).
 * 
 * Handles:
 * - Detecting player overlap.
 * - Counting down to collapse if the player stands on it.
 * - Playing synchronized crack/break sounds.
 * - Changing collider state (trigger/solid) upon collapse/respawn.
 * - Managing visual tinting to warn the player.
 * 
 * @param e The entity ID.
 * @param po The PuzzleObject data.
 * @param dt Delta time.
 * @param playerPos Player's current position.
 * @param isBurrowed Whether the player is currently burrowed (may prevent collapse).
 */
void PuzzleSystem::UpdateBurrowFloor(Entity e, PuzzleObject& po, float dt, const Vector2& playerPos, bool isBurrowed)
{
    // po.active = isStable (true = solid, false = collapsed)
    // po.f2 = currentTimer (countdown for collapse or respawn)

    if (po.active) // Stable tile
    {
        // Check for player overlap
        Collider* c = app.GetCollider(e);
        Transform* t = app.GetTransform(e);
        MeshRenderer* mr = app.GetRenderer(e);
        if (!c || !t) return;

        // Fragile floor must always be walk-through while stable
        c->isTrigger = true;

        // Using transform scale as visual size for overlap
        if (AabbOverlap(playerPos, { 32.f, 32.f }, t->GetPosition(), c->size)) // Assuming player size approx 32x32 for logic
        {
            // If player is burrowed and tile ignores burrow, don't collapse
            if (isBurrowed && po.i0 == 1) return;

            // Stun the player if they walked on it without burrowing
            if (!isBurrowed) {
                if (PlayerController* pc = app.GetController(GameApp::playerEntity)) {
                    pc->Stun(1.0f);
                }
            }

            // Start collapse timer
            po.f2 -= dt;

            // --- SYNCED BREAKING LOGIC ---
            // 1. Calculate player movement delta
            static Vector2 lastPPos = playerPos;
            float distSq = (playerPos.x - lastPPos.x) * (playerPos.x - lastPPos.x) + (playerPos.y - lastPPos.y) * (playerPos.y - lastPPos.y);
            lastPPos = playerPos;

            // 2. Play randomized crack only when the player takes a step
            if (distSq > 0.001f) {
                static float breakSyncTimer = 0.0f;
                breakSyncTimer += dt;

                // Use 0.45f to match engine's standard footstep interval
                if (breakSyncTimer >= 0.45f) {
                    // This triggers the crunch exactly when the footstep thud happens
                    PlayRandomFloorCrack(app, "tile_crack", glm::vec2(t->GetPosition().x, t->GetPosition().y), 4, 0.8f);
                    breakSyncTimer = 0.0f;
                }
            }

            // 3. Final Collapse (Still a one-shot)
            if (po.f2 <= 0.0f)
            {
                // Play the definitive "Collapse" sound
                // Use a higher volume (1.0f) and larger maxRoll (600.0f) so it sounds "big"
                ResourceManager::PlaySfx(app, "tile_break",
                    glm::vec2(t->GetPosition().x, t->GetPosition().y),
                    100.0f, 600.0f, 1.0f);

                po.active = false;
            }

            // Visual Warning, tmp Tint Red for now
            if (mr)
            {
                // Pulse Red as timer decreases
                float intensity = 0.5f + 0.5f * (po.f2 / po.f0); // 1.0 -> 0.5
                mr->SetColor({ 1.0f, intensity, intensity });
            }

            if (po.f2 <= 0.0f)
            {
                po.active = false;
                po.f2 = po.f1; // Reset timer to respawnDelay

                // Handle collider
                if (po.i2 == 1) // createFallHazard
                {
                    c->isTrigger = true;
                }
                else
                {
                    app.RemoveColliderComponent(e);
                }

                // Hide Mesh
                if (mr) mr->SetMesh(nullptr);

                DebugConsole::Get().Warning("[CollapseFloor] Tile COLLAPSED!\n");
            }
        }
        else
        {
            // Reset collapse timer if player leaves before it breaks
            po.f2 = po.f0;
            if (mr) mr->SetColor({ 1.0f, 1.0f, 1.0f }); // Reset color
        }
    }
    else // Collapsed tile
    {
        // Decrease damage cooldown timer
        if (po.f3 > 0.0f) {
            po.f3 -= dt;
        }

        // Check for player overlap when collapsed
        if (po.i2 == 1)
        {
            Collider* c = app.GetCollider(e);
            Transform* t = app.GetTransform(e);
            if (c && t)
            {
                // Using transform scale as visual size for overlap
                if (po.f3 <= 0.0f && AabbOverlap(playerPos, { 32.f, 32.f }, t->GetPosition(), c->size))
                {
                    //For when player falls to pit
                    Interaction::HandlePlayerDamage(app, GameApp::playerEntity, 30); // 30% hp gone for now
                    DebugConsole::Get().Warning("[CollapseFloor] Player fell!\n");
                    
                    // Reset damage cooldown to 2 seconds
                    po.f3 = 2.0f;
                }
            }
        }

        if (po.i1 == 1) return; // collapseOnce = true, no respawn

        po.f2 -= dt;
        if (po.f2 <= 0.0f)
        {
            po.active = true;
            po.f2 = po.f0; // Reset timer to collapseDelay

            // Restore collider
            if (Collider* c = app.GetCollider(e))
            {
                c->isTrigger = true; // Was false, now true to match initialization
            }
            else
            {
                // Re-add collider if it was removed
                app.AddCollider(e, { 32.f, 32.f }, true); // Default tile size, isTrigger=true
            }

            // Restore Mesh & Color
            if (MeshRenderer* mr = app.GetRenderer(e))
            {
                mr->SetMesh(app.GetMesh(1)); // Restore Quad
                mr->SetColor({ 1.0f, 1.0f, 1.0f });
            }

            DebugConsole::Get().Info("[CollapseFloor] Tile respawned.\n");
        }
    }
}

/**
 * @brief Updates the active memory overlay.
 * 
 * Manages the fade-in, display duration, and fade-out of the memory vision.
 * 
 * @param dt Delta time.
 */
void PuzzleSystem::UpdateMemoryOverlay(float dt)
{
    if (!activeOverlay || !activeOverlay->isActive) return;

    // Fade in/out
    if (activeOverlay->timeRemaining > activeOverlay->fadeOutTime)
    {
        activeOverlay->currentAlpha += dt / activeOverlay->fadeInTime;
        if (activeOverlay->currentAlpha > 1.0f)
            activeOverlay->currentAlpha = 1.0f;
    }
    else
    {
        activeOverlay->currentAlpha = activeOverlay->timeRemaining / activeOverlay->fadeOutTime;
    }

    // Countdown
    activeOverlay->timeRemaining -= dt;
    if (activeOverlay->timeRemaining <= 0.0f)
    {
        activeOverlay->Deactivate();
    }
}

/**
 * @brief Main update loop for the PuzzleSystem.
 * 
 * Performs per-frame logic:
 * - Updates memory overlay.
 * - Checks interactions for all puzzle objects (Levers, Plates, Switches, BurrowFloors).
 * - Handles input (e.g., 'F' key) for interactive puzzles.
 * - Automatically evaluates DoorLock states.
 * 
 * @param dt Delta time.
 */
void PuzzleSystem::Update(float dt)
{
    UpdateMemoryOverlay(dt);

    Entity player = GameApp::playerEntity;
    if (player == INVALID_ENTITY) return;

    Transform* pt = app.GetTransform(player);
    if (!pt) return;
    Vector2 pPos = pt->GetPosition(); // Use Position directly for checks

    // Calculate interaction input once per frame using a static flag for robust single-trigger
    auto& in = eng::input();
    static bool s_fKeyWasPressed = false;
    bool interactInput = (in.isKeyDown(GLFW_KEY_F) || in.isGamepadButtonDown(GLFW_GAMEPAD_BUTTON_X));
    bool interactPressed = interactInput && !s_fKeyWasPressed;
    s_fKeyWasPressed = interactInput;

    // Scan puzzle objects and update activators.
    for (auto& kv : app.puzzleObjects)
    {
        Entity e = kv.first;
        PuzzleObject& po = kv.second;

        if (po.kind == PuzzleKind::BurrowFloor)
        {
            // Pass player position and burrow state
            UpdateBurrowFloor(e, po, dt, pPos, Interaction::burrowActiveMap[player]);
            continue;
        }

        Transform* t = app.GetTransform(e);
        Collider* c = app.GetCollider(e);
        if (!t || !c) continue;

        const Vector2 oPos = t->GetPosition();
        const Vector2 oSize = c->size;

        // Re-get player info for generic puzzles
        Collider* pc = app.GetCollider(player);
        if (!pc) continue;
        const Vector2 pSize = pc->size; // Use collider size for puzzle interaction

        const bool overlap = AabbOverlap(pPos, pSize, oPos, oSize);

        switch (po.kind)
        {
        case PuzzleKind::Lever:
        {
            if (!overlap || !interactPressed) break;
            interactPressed = false; // Consume input

            if (po.i0 == 1) po.active = !po.active;
            else po.active = true;

            // Use the Lever's position for spatialization
            if (Transform* tran = app.GetTransform(e)) {
                Vector2 pos = tran->GetPosition();

                PlayRandomLeverActivate(app, "lever", glm::vec2(tran->GetPosition().x, tran->GetPosition().y), 3, 0.8f);
            }

            SetLeverVisual(e, po.active);
            ApplyGroup(po.groupId, po.active);
            break;
        }
        case PuzzleKind::Plate:
        {
            bool newActive = overlap;
            if (newActive != po.active)
            {
                po.active = newActive;
                ApplyGroup(po.groupId, po.active);
            }
            break;
        }
        case PuzzleKind::Switch:
        {
            if (!overlap || !interactPressed) break;
            interactPressed = false; // Consume input
            po.active = !po.active;
            ApplyGroup(po.groupId, po.active);
            break;
        }
        case PuzzleKind::MutationHealer:
        {
            // If the healer is already active, it's on cooldown
            if (po.active)
            {
                po.f2 -= dt;
                if (po.f2 <= 0.0f)
                {
                    po.active = false;
                    // Switch back to original texture
                    if (MeshRenderer* mr = app.GetRenderer(e))
                    {
                        GLuint defaultTex = ResourceManager::GetTexture("mutationHealer");
                        if (defaultTex != 0) mr->SetTexture(defaultTex);
                    }
                }
                // Do not allow interaction while on cooldown
            }
            else
            {
                if (!overlap || !interactPressed) break;
                interactPressed = false; // Consume input

                Interaction::HandleMutationPurification(app, player);
                
                // Set cooldown
                po.active = true;
                po.f2 = 2.0f; // 2 seconds cooldown
                
                // Switch texture to "On" state
                if (MeshRenderer* mr = app.GetRenderer(e))
                {
                    GLuint onTex = ResourceManager::GetTexture("mutationHealer_On");
                    if (onTex != 0) mr->SetTexture(onTex);
                }
            }
            break;
        }
        default:
            break;
        }
    }

    // DoorLock auto-eval pass
    for (auto& kv : app.puzzleObjects)
    {
        Entity e = kv.first;
        PuzzleObject& po = kv.second;
        if (po.kind != PuzzleKind::DoorLock) continue;

        const bool shouldOpen = IsDoorLockRequirementsMet(po);
        if (po.i0 != -1 && !(po.groupId == 0 && po.i0 == 0))
            SetDoorLockOpen(e, shouldOpen);
    }
}

/**
 * @brief Checks if a memory overlay is currently active.
 * @return True if active.
 */
bool PuzzleSystem::IsMemoryActive() const
{
    return activeOverlay && activeOverlay->isActive;
}

/**
 * @brief Renders the memory overlay to the screen.
 * 
 * Draws a full-screen backdrop, the memory image, and narrative text
 * using the provided renderer. Switches to an orthographic projection for UI drawing.
 * 
 * @param renderer The renderer instance to use.
 */
void PuzzleSystem::DrawMemoryOverlay(Renderer& renderer)
{
    if (!activeOverlay || !activeOverlay->isActive)
        return;

    Camera2D* prevCam = renderer.getCamera();
    renderer.setCamera(nullptr);

    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &fbw, &fbh);
    if (fbw <= 0 || fbh <= 0)
    {
        renderer.setCamera(prevCam);
        return;
    }

    const float l = 0.0f;
    const float r = static_cast<float>(fbw);
    const float b = 0.0f;
    const float t = static_cast<float>(fbh);

    Matrix3x3 ortho = Matrix3x3::Identity();
    ortho(0, 0) = 2.0f / (r - l);
    ortho(1, 1) = 2.0f / (t - b);
    ortho(2, 0) = -(r + l) / (r - l);
    ortho(2, 1) = -(t + b) / (t - b);

    Shader* sh = renderer.GetShader();
    if (sh)
    {
        sh->Use();
        sh->SetInt("u_Frame", 0);
        sh->SetInt("u_Cols", 1);
        sh->SetVec2("u_FrameSize", Vector2(1.f, 1.f));
    }

    Mesh2D* quad = app.GetMesh(1);
    if (!quad)
    {
        renderer.setCamera(prevCam);
        return;
    }

    const float a = std::clamp(activeOverlay->currentAlpha, 0.0f, 1.0f);

    // Fullscreen dark backdrop
    {
        Matrix3x3 bgModel =
            Matrix3x3::BuildTranslation(0.0f, 0.0f) *
            Matrix3x3::BuildScaling(static_cast<float>(fbw), static_cast<float>(fbh));

        renderer.DrawMesh(*quad, ortho * bgModel, Vector3(0.03f * a, 0.06f * a, 0.04f * a), 0);
    }

    // import and draw the memory image from mapImagePath
    GLuint memTex = 0;
    if (!activeOverlay->mapImagePath.empty())
    {
        const std::string texKey =
            "MemoryOverlay_" + activeOverlay->mapImagePath;

        memTex = ResourceManager::GetTexture(texKey);

        if (memTex == 0)
        {
            std::string relPath = activeOverlay->mapImagePath;

            // currently stored in "Assets/MemoryMaps/memory_01.png"
            // Convert into a path relative to AssetPath
            if (relPath.rfind("Assets/", 0) == 0)
                relPath = relPath.substr(7);
            else if (relPath.rfind("assets/", 0) == 0)
                relPath = relPath.substr(7);

            const std::string absPath = AssetPath(relPath);

            if (ResourceManager::ImportTexture(absPath, texKey))
                memTex = ResourceManager::GetTexture(texKey);
        }
    }

    if (memTex != 0)
    {
        // Fit image inside the screen while keeping it centered
        const float imgW = fbw * 0.62f;
        const float imgH = fbh * 0.42f;
        const float imgX = (fbw - imgW) * 0.5f;
        const float imgY = (fbh - imgH) * 0.50f;

        Matrix3x3 imgModel =
            Matrix3x3::BuildTranslation(imgX, imgY) *
            Matrix3x3::BuildScaling(imgW, imgH);

        renderer.DrawMesh(*quad, ortho * imgModel, Vector3(a, a, a), memTex);
    }

    // Narrative text centered under the image
    {
        const float textSize = 28.0f * (static_cast<float>(fbh) / 1080.0f);
        const float wrapWidth = fbw * 0.70f;
        const float textX = (fbw - wrapWidth) * 0.5f;
        const float textY = fbh * 0.15f;

        EngineCore::FontRenderer::DrawText(
            renderer,
            "default",
            textSize,
            textX,
            textY,
            0xFFFFFFFF,
            activeOverlay->narrativeText,
            wrapWidth,
            FontSys::Align::Center
        );
    }

    renderer.setCamera(prevCam);
}
