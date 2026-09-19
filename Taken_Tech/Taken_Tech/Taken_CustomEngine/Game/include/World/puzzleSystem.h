#pragma once
/**
 * @file      puzzlesystem.h
 * @author    Jethro Sung
 * @co-author Sng Swee Yong Dillon
 * @email     sung.h, sweeyongdillon.sng
 * @date      2026-02-04
 *
 * @brief     Declares PuzzleSystem, the gameplay system responsible for
 *            updating and resolving puzzle interactions in the game.
 *
 * PuzzleSystem centralizes logic for puzzle entities such as:
 *  - Levers and linked puzzle groups
 *  - Pressure plates (AABB-based proximity checks)
 *  - Door locks that open after puzzle completion
 *  - Shoot targets that require projectile hits to clear
 *
 * It also exposes an OnProjectileHit() entry point so that ProjectileSystem
 * can notify the puzzle layer when a projectile collides with a puzzle object.
 *
 * Group-based activation is supported through ApplyGroup(), allowing puzzle
 * objects to be enabled/disabled efficiently using a shared groupId rather
 * than checking every object individually.
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */
#include "Core/Systems/systemManager.h"
#include "puzzleObject.h"
#include <vector>
#include <string>
#include <unordered_map>

class GameApp;

/**
 * @brief Manages the active memory vision overlay for Healing Mem
 * 
 * Handles the display state, narrative text, and fade-in/out transitions
 * for memory flashbacks triggered by healing interactions.
 */
struct MemoryVisionOverlay
{
    bool isActive = false;              ///< Whether the overlay is currently visible.
    int displayingMemoryId = -1;        ///< ID of the memory being shown.
    float timeRemaining = 0.0f;         ///< Time remaining before the overlay closes.
    std::string narrativeText;          ///< Text to display during the memory.
    std::string mapImagePath;           ///< Path to the memory image asset.

    float fadeInTime = 0.8f;            ///< Duration of the fade-in effect.
    float fadeOutTime = 1.0f;           ///< Duration of the fade-out effect.
    float currentAlpha = 0.0f;          ///< Current opacity of the overlay (0.0 to 1.0).

    /**
     * @brief Activates the memory overlay with specific content.
     * @param memId ID of the memory.
     * @param narrative Text to display.
     * @param mapPath Path to the image file.
     * @param duration How long the overlay remains active.
     */
    void Activate(int memId, const std::string& narrative, const std::string& mapPath, float duration)
    {
        isActive = true;
        displayingMemoryId = memId;
        narrativeText = narrative;
        mapImagePath = mapPath;
        timeRemaining = duration;
        currentAlpha = 0.0f;
    }

    /**
     * @brief Deactivates and resets the overlay.
     */
    void Deactivate()
    {
        isActive = false;
        displayingMemoryId = -1;
        timeRemaining = 0.0f;
        currentAlpha = 0.0f;
    }
};

struct PersistentPuzzleState {
    bool active = false;
    int i0 = 0, i1 = 0, i2 = 0, i3 = 0;
    float f0 = 0.f, f1 = 0.f, f2 = 0.f, f3 = 0.f;
    std::string s0;
    std::string s1;
};

class PuzzleSystem : public ISystem
{
public:
    explicit PuzzleSystem(GameApp& app);
    ~PuzzleSystem();

    /**
     * @brief Updates the puzzle system state.
     * 
     * Checks for player interactions with puzzle objects (levers, plates, switches),
     * updates timer-based puzzles (burrow floors), and manages the memory overlay.
     * 
     * @param dt Delta time in seconds.
     */
    void Update(float dt) override;

    /**
     * @brief Resets internal state, clearing revealed memories and closing overlays.
     */
    void Reset();

    /**
     * @brief Called by projectile system when a projectile hits a puzzle object.
     * 
     * Handles logic for ShootTarget puzzles, checking hit counts and triggering
     * completion if requirements are met.
     * 
     * @param puzzleEntity The entity that was hit.
     * @param fromPlayer True if the projectile was fired by the player.
     */
    void OnProjectileHit(Entity puzzleEntity, bool fromPlayer);

    /**
     * @brief Called by AbilityInteraction when player uses heal near a corpse.
     * 
     * Checks if the player is close enough to a HealingMemory puzzle object to
     * trigger a memory flashback.
     * 
     * @param playerPos Current position of the player.
     * @return True if a memory was successfully triggered.
     */
    bool OnPlayerHealUsed(const Vector2& playerPos);

    /**
     * @brief Check if an entity is a ShootTarget puzzle object.
     * @param e Entity to check.
     * @return True if the entity is a shoot target.
     */
    bool IsShootTarget(Entity e) const;

    // Memory Overlay access for rendering
    /**
     * @brief Checks if a memory overlay is currently active.
     * @return True if active.
     */
    bool IsMemoryActive() const;

    /**
     * @brief Renders the active memory overlay.
     * @param renderer Reference to the renderer.
     */
    void DrawMemoryOverlay(class Renderer& renderer);

    /**
    * @brief Saves the current state of all puzzle objects to the persistent storage.
    */
    void SaveAllPuzzleStates();

    /**
     * @brief Restores the state of all puzzle objects from the persistent storage.
     */
    void RestoreAllPuzzleStates();


private:
    GameApp& app;
    MemoryVisionOverlay* activeOverlay = nullptr;
    std::vector<int> revealedMemories;
    std::unordered_map<std::string, PersistentPuzzleState> persistentStates;

    /**
     * @brief Checks if two Axis-Aligned Bounding Boxes (AABBs) overlap.
     * @param aPos Position of first box.
     * @param aSize Size of first box.
     * @param bPos Position of second box.
     * @param bSize Size of second box.
     * @return True if they overlap.
     */
    static bool AabbOverlap(const Vector2& aPos, const Vector2& aSize,
        const Vector2& bPos, const Vector2& bSize);

    /**
     * @brief Applies an "active" state to all puzzle objects in a specific group.
     * 
     * Updates visual states and collider properties (e.g., opening doors,
     * disabling barriers) for all objects sharing the group ID.
     * 
     * @param groupId The ID of the group to update.
     * @param active The new active state (true = activated/open).
     */
    void ApplyGroup(int groupId, bool active);

    /**
     * @brief Checks if all ShootTarget puzzles in a group have been cleared.
     * @param groupId The group ID to check.
     * @return True if all targets in the group are cleared.
     */
    bool CheckShootTargetGroupCleared(int groupId);

    /**
     * @brief Sets the state of a door lock.
     * @param e The door entity.
     * @param open True to open, false to close.
     */
    void SetDoorLockOpen(Entity e, bool open);

    /**
     * @brief Checks if a door lock's opening requirements are met.
     * @param doorPo The puzzle object data for the door.
     * @return True if requirements are satisfied.
     */
    bool IsDoorLockRequirementsMet(const PuzzleObject& doorPo);

    /**
     * @brief Updates the visual appearance of a shoot target.
     * @param e The entity.
     * @param cleared True if the target is cleared (hit enough times).
     */
    void SetShootTargetVisual(Entity e, bool cleared);

    /**
     * @brief Updates the visual appearance of a lever.
     * @param e The entity.
     * @param active True if the lever is in the active position.
     */
    void SetLeverVisual(Entity e, bool active);

    // Internal Puzzle Logic
    /**
     * @brief Updates the state of a burrow floor tile.
     * 
     * Handles collapse timers, player proximity checks, and respawning logic.
     * 
     * @param e The entity.
     * @param po Puzzle object data.
     * @param dt Delta time.
     * @param playerPos Player position.
     * @param isBurrowed Whether the player is currently burrowed.
     */
    void UpdateBurrowFloor(Entity e, PuzzleObject& po, float dt, const Vector2& playerPos, bool isBurrowed);

    /**
     * @brief Updates the memory overlay timers and alpha values.
     * @param dt Delta time.
     */
    void UpdateMemoryOverlay(float dt);
};