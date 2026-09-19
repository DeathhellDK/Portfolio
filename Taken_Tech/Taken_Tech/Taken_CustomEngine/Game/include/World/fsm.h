#pragma once

/**
* @file      fsm.h
* @author    Lim Zhi Jie
* @co-author
* @email     zhijie.lim
* @date      2025-03-07
*
* @brief     Defines the Finite State Machine (FSM) system used to control enemy AI
* behaviour in the game.
*
* This file declares the base State interface and the FSM controller used
* by enemy AI. The FSM pattern allows AI behaviour to be separated into
* individual states, where each state represents a specific behaviour such
* as patrolling, chasing the player, searching, or attacking.
*
* Each state inherits from the State class and overrides its virtual
* functions to implement behaviour logic.
*
* The FSM class manages:
*  - The currently active state
*  - Transitions between states
*  - Per-frame updates of the active state
*
* @version 1.0
* @copyright
* Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the
* prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include <memory>
#include "Math/vect2.h"
#include "Physics/EntityForceProxy.hpp"
#include "Core/component.h"

class EnemyAi;

/**
 * @class State
 * @brief Abstract base class for all AI states.
 * 
 * Provides the interface for entering, updating, and exiting states.
 * Specific behaviors (Patrol, Chase, Attack) inherit from this class.
 */
class State {
public:
    virtual ~State() = default;

    /**
     * @brief Called once when the state becomes active.
     * @param enemy Pointer to the EnemyAi component.
     */
    virtual void Enter(EnemyAi*) {}

    /**
     * @brief Called every frame to update state logic.
     * @param enemy Pointer to the EnemyAi component.
     * @param deltaTime Time elapsed since last frame.
     * @param playerPos Current player position.
     * @param e The enemy entity.
     * @param forceproxy Physics proxy for movement.
     */
    virtual void Update(EnemyAi*, float , Vector2, Entity, EntityForceProxy*) {}

    /**
     * @brief Called once when transitioning away from the state.
     * @param enemy Pointer to the EnemyAi component.
     */
    virtual void Exit(EnemyAi*) {}
};

/**
 * @class FSM
 * @brief Finite State Machine controller.
 * 
 * Manages the current active state and handles transitions between states.
 */
class FSM {
private:
    EnemyAi* enemy;
    std::unique_ptr<State> currentState;

public:
    /**
     * @brief Constructor.
     * @param enemy Pointer to the owning EnemyAi component.
     */
    FSM(EnemyAi* enemy);

    /**
     * @brief Switches the current state to a new one.
     * 
     * Calls Exit() on the old state and Enter() on the new state.
     * 
     * @param newState Unique pointer to the new state instance.
     */
    void ChangeState(std::unique_ptr<State> newState);

    /**
     * @brief Updates the currently active state.
     * @param deltaTime Time elapsed.
     * @param playerPos Player position.
     * @param e Enemy entity.
     * @param forceproxy Physics proxy.
     */
    void Update(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy);

    /**
     * @brief Returns a raw pointer to the current state.
     * @return Pointer to the active State object.
     */
    State* GetCurrentState() const;
};