#pragma once

/**
* @file      states.h
* @author    Lim Zhi Jie
* @co-author Sng Swee Yong Dillon
* @email     zhijie.lim, sweeyongdillon.sng
* @date      2025-03-07
*
* @brief    Defines all finite state machine (FSM) states used by enemy AI entities
* This file contains the complete list of AI states used by enemies in the
* game. The AI is implemented using the Finite State Machine (FSM) pattern,
* where each state represents a distinct behavior such as patrolling,
* chasing the player, searching for the player, or attacking.
*
* Each state derives from the base `State` class defined in `fsm.h`.
* The FSM controls enemy behaviour by transitioning between states based
* on conditions such as player visibility, attack range, or health levels.
*
* Every state implements three core functions:
* Enter()   - Runs once when the state becomes active.
* Update()  - Runs every frame while the state is active.
* Exit()    - Runs once when transitioning away from the state.
*
* @version 1.0
* @copyright
* Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the
* prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "fsm.h"

// ---------------------------------------------------------------------------
// SHARED STATES  (all enemy types pass through these)
// ---------------------------------------------------------------------------

/**
 * @class PatrolState
 * @brief Default state: walk back and forth within patrol range.
 * 
 * Transitions:
 * -> ChaseState when player is spotted (in front and line of sight).
 */
class PatrolState : public State {
public:
    void Enter(EnemyAi* enemy) override;
    void Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy) override;
    void Exit(EnemyAi* enemy) override;
};

/**
 * @class ChaseState
 * @brief Moves the enemy toward the player's position.
 * 
 * Transitions:
 * -> [Typed]AttackState when within attack range.
 * -> SearchState when line of sight is lost.
 */
class ChaseState : public State {
public:
    void Enter(EnemyAi* enemy) override;
    void Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy) override;
    void Exit(EnemyAi* enemy) override;
};

/**
 * @class SearchState
 * @brief Moves to last known player position and performs a 360-degree scan.
 * 
 * Transitions:
 * -> ChaseState if player is re-spotted during scan.
 * -> ReturnToSpawnState when scan completes without finding player.
 */
class SearchState : public State {
public:
    void Enter(EnemyAi* enemy) override;
    void Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy) override;
    void Exit(EnemyAi* enemy) override;
};

/**
 * @class ReturnToSpawnState
 * @brief Moves the enemy back to its original spawn position.
 * 
 * Transitions:
 * -> ChaseState if player is spotted en route.
 * -> PatrolState once physically arrived at spawn.
 */
class ReturnToSpawnState : public State {
public:
    void Enter(EnemyAi* enemy) override;
    void Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy) override;
    void Exit(EnemyAi* enemy) override;
};

// ---------------------------------------------------------------------------
// PER-TYPE ATTACK STATES
// Each one owns only its own attack logic and transition rules.
// To change how an enemy attacks, edit only its class below.
// ---------------------------------------------------------------------------

/**
 * @class BasicAttackState
 * @brief Melee attack: stops moving, strikes player.
 * 
 * Transitions:
 * -> ChaseState if player moves out of attack range.
 * -> SearchState if sight is lost.
 */
class BasicAttackState : public State {
public:
    void Enter(EnemyAi* enemy) override;
    void Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy) override;
    void Exit(EnemyAi* enemy) override;
};

/**
 * @class RangedAttackState
 * @brief Ranged attack: maintains distance, fires projectiles.
 * 
 * Transitions:
 * -> ChaseState if player moves out of attack range.
 * -> SearchState if sight is lost.
 */
class RangedAttackState : public State {
public:
    void Enter(EnemyAi* enemy) override;
    void Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy) override;
    void Exit(EnemyAi* enemy) override;
};

/**
 * @class BurrowAttackState
 * @brief Burrow attack: burrows underground and emerges near player.
 * 
 * Transitions:
 * -> ChaseState if player moves out of attack range (only when above ground).
 * -> SearchState if sight is lost (only when above ground).
 */
class BurrowAttackState : public State {
public:
    void Enter(EnemyAi* enemy) override;
    void Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy) override;
    void Exit(EnemyAi* enemy) override;
};

/**
 * @class HealAttackState
 * @brief Heal attack: heals self when HP is low, attacks when healthy.
 * 
 * Transitions:
 * -> ChaseState if player moves out of attack range.
 * -> SearchState if sight is lost.
 */
class HealAttackState : public State {
public:
    void Enter(EnemyAi* enemy) override;
    void Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy) override;
    void Exit(EnemyAi* enemy) override;
};

// ---------------------------------------------------------------------------
// BLESS MOB STATES
// ---------------------------------------------------------------------------

/**
 * @class BlessHealAllyState
 * @brief Bless mob: move toward the ranged ally and cast bless to heal it.
 * 
 * Caller must pass the ally's position as playerPos when calling update().
 * 
 * Transitions:
 * -> BlessAttackPlayerState when notified that the ranged ally has died.
 */
class BlessHealAllyState : public State {
public:
    void Enter(EnemyAi* enemy) override;
    void Update(EnemyAi* enemy, float deltaTime, Vector2 allyPos, Entity e, EntityForceProxy* forceproxy) override;
    void Exit(EnemyAi* enemy) override;
};

/**
 * @class BlessAttackPlayerState
 * @brief Bless mob: ranged ally is dead. Chase player and fire slow homing bless.
 * 
 * Transitions:
 * -> SearchState if sight of player is lost.
 */
class BlessAttackPlayerState : public State {
public:
    void Enter(EnemyAi* enemy) override;
    void Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy) override;
    void Exit(EnemyAi* enemy) override;
};

/**
 * @class BossAttackState
 * @brief Boss attack: cycles through multiple phases (Melee, Ranged, Heal, Burrow).
 * 
 * Transitions:
 * -> SearchState if sight is lost (only during Melee approach phase).
 */
class BossAttackState : public State {
public:
    void Enter(EnemyAi* enemy) override;
    void Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy) override;
    void Exit(EnemyAi* enemy) override;
};