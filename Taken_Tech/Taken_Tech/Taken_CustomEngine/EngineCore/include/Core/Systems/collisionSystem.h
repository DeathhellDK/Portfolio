#pragma once
/**
 * @file    collisionSystem.h
 * @author  Jethro Sung
 * @email    sung.h
 * @date    2025-09-29
 *
 * @brief   Declares the CollisionSystem class responsible for handling
 *          collision detection between entities that own Collider and
 *          Transform components.
 *
 * The CollisionSystem manages interactions between colliders and updates
 * entity states accordingly. It is integrated with the ECS SystemManager
 * and ensures gameplay mechanics such as player rollback or physical
 * interactions are respected.
 *
 * @version 1.0
 * @copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without prior
 * written consent of DigiPen Institute of Technology is prohibited.
 */
#include "Physics/collider.h"
#include "Core/transform.h"
#include <vector>
#include <iostream>
#include <functional>
#include "Core/Systems/systemManager.h"
#include "Core/componentcontext.h"

class EntityForceProxy; /// Forward declaration to avoid cicular dependency 

/**
 * @class CollisionSystem
 * @brief Handles collision detection and resolution between game entities.
 *
 * This system iterates over all Collider and Transform components,
 * determines overlaps, and resolves collisions as needed. It also
 * provides rollback checks for player controllers to ensure
 * gameplay stability.
 */
class CollisionSystem : public ISystem, public SystemBase {
	IComponentContext& context;  ///< Reference to ECS context (implemented by GameApp)

	EntityForceProxy* forceProxy; /// Pointer to the EntityForceProxy instance

	/*bool collisionDisabled = false;
	float collisionTimer = 0.0f;*/

	/*public:
		explicit CollisionSystem(IComponentContext& ctx, EntityForceProxy* proxy = nullptr);*/

public:
	/**
	 * @brief Constructs a CollisionSystem with component references.
	 *
	 * @param ctx Reference to the ECS component context.
	 * @param proxy Optional pointer to the EntityForceProxy instance for resolving physics forces.
	 */
	explicit CollisionSystem(IComponentContext& ctx, EntityForceProxy* proxy = nullptr);

	/*void SetCollisionDisabled(float seconds) {
		collisionDisabled = true;
		collisionTimer = seconds;
	}

	bool IsCollisionDisabled() const {
		return collisionDisabled;
	}*/

	/**
	 * @brief Updates collision state for all entities in the system.
	 *
	 * Iterates through all colliders, checks for pairwise collisions,
	 * and applies resolution logic (e.g., applying forces, dealing damage).
	 *
	 * @param dt Delta time for this frame update.
	 */
	void Update(float dt) override;

	/**
	 * @brief Sets the force proxy for the collision system.
	 * @param proxy Pointer to the EntityForceProxy to use.
	 */
	void setForceProxy(EntityForceProxy* proxy) {
		forceProxy = proxy;
	}

	/// Callback function for player-enemy collision events
	std::function<void(Entity*, Entity*)> collisionCallback;


private:
	/*static bool collisionOccurred;*/
	static bool playerburrowActive;

	//    std::vector<Collider>& colliders;
	//    std::vector<Entity>& colliderOwners;
	//    std::vector<Transform>& transforms;
	//    std::vector<Entity>& transformOwners;
	//    std::vector<Entity>& controllerOwners; // to detect player rollback
	//
	//public:
	//    /**
	//     * @brief Constructs a CollisionSystem with component references.
	//     *
	//     * @param c Reference to list of colliders.
	//     * @param cOwners Reference to collider owner entity IDs.
	//     * @param t Reference to list of transforms.
	//     * @param tOwners Reference to transform owner entity IDs.
	//     * @param ctrlOwners Reference to controller owner entity IDs.
	//     */
	//    CollisionSystem(std::vector<Collider>& c,
	//        std::vector<Entity>& cOwners,
	//        std::vector<Transform>& t,
	//        std::vector<Entity>& tOwners,
	//        std::vector<Entity>& ctrlOwners)
	//        : colliders(c), colliderOwners(cOwners),
	//        transforms(t), transformOwners(tOwners),
	//        controllerOwners(ctrlOwners) {
	//    }
	//
	//    /**
	//     * @brief Updates collision state for all entities in the system.
	//     *
	//     * Iterates through all colliders, checks for pairwise collisions,
	//     * and applies resolution logic (e.g., rolling back a player
	//     * to its last safe position).
	//     *
	//     * @param dt Delta time for this frame update.
	//     */
	//    void Update(float dt) override;
};