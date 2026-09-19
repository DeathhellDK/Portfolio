/**
* @file      fsm.cpp
* @author    Lim Zhi Jie
* @co-author
* @email     zhijie.lim
* @date      2025-03-07
*
* @brief     Implements the Finite State Machine (FSM) system used by enemy AI.
*
* This file contains the implementation of the FSM class, which manages
* behavior states for enemies in the game. The FSM controls which AI state
* is currently active and handles transitions between states.
*
* Each state derives from the base `State` class and represents a specific
* behavior such as patrolling, chasing the player, searching for the player,
* or performing an attack.
*
* The FSM ensures that:
*  - The current state's Exit() function is called before switching states.
*  - The new state's Enter() function is called when it becomes active.
*  - The current state's Update() function runs every frame.
*
* @version 1.0
* @copyright
* Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the
* prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "World/fsm.h"
#include "World/ai.h"

/**
 * @brief Constructor for FSM.
 * @param enemy Pointer to the EnemyAi component that owns this FSM.
 */
FSM::FSM(EnemyAi* enemy) : enemy(enemy), currentState(nullptr) {}

/**
 * @brief Changes the active state of the FSM.
 * 
 * Exits the current state (if any), replaces it with the new state,
 * and enters the new state.
 * 
 * @param newState The new state to transition to.
 */
void FSM::ChangeState(std::unique_ptr<State> newState) {
    if (currentState) currentState->Exit(enemy);

    currentState = std::move(newState); // old state automatically deleted here

    if (currentState) currentState->Enter(enemy);
}

/**
 * @brief Updates the current state.
 * 
 * Delegates the update call to the currently active state object.
 * 
 * @param deltaTime Time elapsed since last frame.
 * @param playerPos Current position of the player.
 * @param e The enemy entity ID.
 * @param forceproxy Pointer to the physics force proxy.
 */
void FSM::Update(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy) {
    if (currentState) currentState->Update(enemy, deltaTime, playerPos, e, forceproxy);
}

/**
 * @brief Retrieves the current state.
 * @return Raw pointer to the active state.
 */
State* FSM::GetCurrentState() const {
    return currentState.get();
}