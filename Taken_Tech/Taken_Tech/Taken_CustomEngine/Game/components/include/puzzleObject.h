#pragma once
/**
 * @file      puzzleObject.h
 * @author    Jethro Sung, Sng Swee Yong Dillon
 * @email     sung.h, sweeyongdillon.sng
 * @date      2026-02-04
 *
 * @brief     Declares PuzzleKind and PuzzleObject, a lightweight ECS component
 *            used to tag and configure interactive puzzle entities.
 *
 * PuzzleObject provides a generic, data-driven way to represent multiple puzzle
 * behaviors (burrow walls, levers, plates, door locks, switches, shoot targets)
 * using a shared structure.
 *
 * Puzzle objects can be linked together using groupId, allowing one object
 * (e.g., a lever) to activate/deactivate other objects within the same group.
 *
 * The i0-i3 and f0-f3 parameter slots are interpreted differently depending on
 * the PuzzleKind, enabling prefab/JSON-driven puzzle configuration without
 * requiring separate component types for each puzzle interaction.
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */
#include "Core/component.h" // Entity, Component base

#include <string>

enum class PuzzleKind : uint8_t
{
    None = 0,
    BurrowWall,
    Lever,
    Plate,
    DoorLock,
    Switch,
    ShootTarget,
    HealingMemory,  // Dead soldier mem
    BurrowFloor,    // Collapsing floor
    MutationHealer  // Cleanses player mutation
};

struct PuzzleObject : public Component
{
    PuzzleObject() : Component(INVALID_ENTITY) {}
    explicit PuzzleObject(Entity ownerId) : Component(ownerId) {}

    PuzzleKind kind = PuzzleKind::None;

    // Linking: lever group 2 affects all puzzle objects group 2
    int groupId = 0;

    // Common state
    bool active = true;

    // Params interpreted based on kind:
    // BurrowWall:  i0 = requiresPlayerPhasing (0/1), i1 = solidWhenInactive(0/1)
    // Lever:       i0 = toggle(0/1)
    // Plate:       f0 = activationRadius
    // DoorLock:    groupId links to the ShootTarget group that must clear before the door opens.
    //              Spawn with trigger=false (solid). ApplyGroup will swap it to trigger when activated.
    // //   Legacy , for sing grp:
    //     - i0 == 0  => door requires only groupId to be fully cleared (all ShootTargets in that group).
    //
    //   Multi-group requirement mode:
    //     - i0 = number of required groups (1..4)
    //     - groupId = required group #1
    //     - i1/i2/i3 = required group #2/#3/#4
    //     
    //
    //   The door opens only when ALL req groups are cleared.
    //
    //   Spawn with trigger=false (solid). When open: trigger=true (walk-through).
    // ShootTarget: i0 = req hits, i1 = curr hits (starts 0), i2 = requiresPlayerProjectiles(0/1).
    //              Spawn with trigger=false so projectiles physically collide with it.
    // 
    // HealingMemory:
    //   i0 = memoryId
    //   i1 = requiredMemoryId (-1 if none)
    //   f0 = healRadius (default 100.0f)
    //   f1 = memoryDuration (default 8.0f)
    //   s0 = memoryNarrative
    //   s1 = memoryMapImagePath
    //   active = hasBeenHealed (initially false)
    //
    // BurrowFloor:
    //   i0 = ignoreWhenBurrowed (1=true)
    //   i1 = collapseOnce (1=true)
    //   i2 = createFallHazard (1=true)
    //   f0 = collapseDelay (time before break)
    //   f1 = respawnDelay (time before return)
    //   f2 = currentTimer (countdown for collapse or respawn)
    //   f3 = damageCooldownTimer (cooldown before dealing damage again)
    //   active = isStable (true=walkable, false=collapsed/pit)
    
    int i0 = 0, i1 = 0, i2 = 0, i3 = 0;
    float f0 = 0, f1 = 0, f2 = 0, f3 = 0;
    
    // String storage for complex puzzles (HealingMemory)
    std::string s0;
    std::string s1;
};