/**
 * @file    collisionSystem.cpp
 * @author  Jethro Sung, Tan Wei Liang Terril
 * @email    sung.h, t.weiliangterril
 * @date    2025-09-29
 *
 * @brief   Implements the CollisionSystem class declared in collisionSystem.h.
 *
 * The CollisionSystem performs two-phase collision detection:
 * - Broad phase: grid-based spatial partitioning (BroadCollision)
 * - Narrow phase: AABB + rotation-corrected checks from Collider component
 *
 * Resolution currently supports:
 * - Dynamic vs Static: Player rolled back and velocity corrected
 * - Trigger colliders: skipped for position resolution (future event hooks)
 * - Parent-child (attached objects): ignored to avoid self-collision
 *
 * @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
#include "Core/Systems/CollisionSystem.h"
#include "Physics/PhysicsDebugger.h"
#include "Physics/BroadCollision.hpp" // To be tested 
#include "Physics/EntityForceProxy.hpp"

 //bool CollisionSystem::collisionOccurred = false;
/**
* @brief Finds the index of a Transform component belonging to a given Entity.
*
* Performs a linear search in the transformOwners vector to locate the
* transform associated with an entity.
*
* @param e Entity whose transform index is requested.
* @param transformOwners Vector of entity IDs that own transforms.
* @return Index of the transform in the transforms array,
*         or (size_t)-1 if not found.
*/
//size_t FindTransformIndex(Entity e,
//    const std::vector<Entity>& transformOwners) {
//    auto it = std::find(transformOwners.begin(), transformOwners.end(), e);
//    return (it != transformOwners.end()) ?
//        std::distance(transformOwners.begin(), it) : (size_t)-1;
//}

/**
 * @brief Constructs a CollisionSystem with component filtering for
 * Transform + Collider enabled entities.
 *
 * @param ctx ECS component interface for entity lookup and modification.
 * @param proxy Optional interface for velocity correction after resolution.
 */
CollisionSystem::CollisionSystem(IComponentContext& ctx, EntityForceProxy* proxy)
    : context(ctx), forceProxy(proxy)
{
    Signature sig;
    sig.set(TRANSFORM);
    sig.set(COLLIDER);
    SetSignature(sig);
}

/**
 * @brief Performs collision detection and resolution.
 *
 * Execution steps:
 *
 * 1) Filter:
 * Collect entities that match this system's signature (Transform + Collider).
 *
 * 2) Broad Phase:
 * Inserts each collider into spatial buckets to reduce collision checks.
 *
 * 3) Narrow Phase Filtering:
 * - Skip resolving static-static collisions
 * - Skip Trigger collider pairs (future events possible)
 * - Skip collisions between parent/child attachment pairs
 * - Resolve only when at least one entity is dynamic
 *   (currently defined as having a Controller)
 *
 * 4) Resolution:
 * Pushes the player entity out along minimal penetration axis.
 * If available, also zeroes out blocked velocity components using EntityForceProxy.
 *
 * @param dt Delta time (unused currently, included for future predict-based motion)
 */
void CollisionSystem::Update(float /*dt*/) {
    //collisionOccurred = false;
    const auto& entities = context.GetEntitySignatures();
    const Signature& sysSig = GetSignature();

    // 1) Collect entities that have Transform + Collider
    std::vector<Entity> active;
    active.reserve(entities.size());
    for (auto it = entities.begin(); it != entities.end(); ++it) {
        if ((it->second & sysSig) == sysSig) active.push_back(it->first);
    }

    // 2) Build broad phase ONCE (cell size - your tileSize;)
    BroadCollision broadPhase(100.0f);
    for (Entity e : active) {
        Transform* t = context.GetTransform(e);
        Collider* c = context.GetCollider(e);
        if (!t || !c) continue;
        // Insert using top-left position + full size (matches your narrowphase)
        broadPhase.insert(e, t->GetPosition(), c->size);
    }

    // 3) Compute candidate pairs ONCE
    auto candidates = broadPhase.computePotentialCollisions();

    // 4) Narrow phase only for useful pairs (skip static-static)
    for (const auto& pr : candidates) {
        Entity a = pr.first;
        Entity b = pr.second;

        Transform* t1 = context.GetTransform(a);
        Transform* t2 = context.GetTransform(b);
        Collider* c1 = context.GetCollider(a);
        Collider* c2 = context.GetCollider(b);
        if (!t1 || !t2 || !c1 || !c2) continue;

        // Skip TRIGGER colliders entirely for positional resolution
        if ((c1 && c1->isTrigger) || (c2 && c2->isTrigger)) {
            // (Optionally fire trigger callbacks here)
            continue;
        }

        // Skip resolution between a parent and its child (attachments like Sword)
        if (t1 && t2) {
            if (t1->GetParent() == b || t2->GetParent() == a) {
                continue;
            }
        }

        // Treat "dynamic" as "has controller" (player). Adjust if you have a proper isDynamic flag.
        const bool aIsPlayer = (context.GetController(a) != nullptr);
        const bool bIsPlayer = (context.GetController(b) != nullptr);

        const bool aIsEnemy = (context.GetEnemyController(a) != nullptr);
        const bool bIsEnemy = (context.GetEnemyController(b) != nullptr);

        // For burrow ability, we want to allow player not to trigger with the walls.
        if (aIsPlayer && c1->isBurrowed && (bIsEnemy || c2->isPassableWhenBurrowed)) continue;

        if (bIsPlayer && c2->isBurrowed && (aIsEnemy || c1->isPassableWhenBurrowed)) continue;

        // Skip static vs static
        if (!aIsPlayer && !bIsPlayer && !aIsEnemy && !bIsEnemy) continue;
        
        // Skip enemy vs enemy to maintain previous behavior where they don't block each other
        if (aIsEnemy && bIsEnemy) continue;

        // --- existing narrow-phase ---
        const Vector2 center1 = t1->GetPosition() + c1->size * 0.5f;
        const Vector2 center2 = t2->GetPosition() + c2->size * 0.5f;

        const float rotation1 = t1->GetRotation();
        const float rotation2 = t2->GetRotation();

        const bool hit = c1->CheckCollision(*c2, t1->GetPosition(), t2->GetPosition(), rotation1, rotation2);
        if (!hit) continue;

        Vector2 diff = center1 - center2;
        Vector2 totalHalf = (c1->size * 0.5f) + (c2->size * 0.5f);
        Vector2 overlap = totalHalf - Vector2(std::abs(diff.x), std::abs(diff.y));
        if (overlap.x <= 0.f || overlap.y <= 0.f) continue;

        // Resolve only the player side
        Vector2 normal;
        float penetration;
        if (overlap.x < overlap.y) { normal = (diff.x > 0.f) ? Vector2(1, 0) : Vector2(-1, 0); penetration = overlap.x; }
        else { normal = (diff.y > 0.f) ? Vector2(0, 1) : Vector2(0, -1); penetration = overlap.y; }

        /// -- Player and Enemy Collision
        if ((aIsPlayer && bIsEnemy) || (bIsPlayer && aIsEnemy)) {
            if (collisionCallback) {
                Entity* player = aIsPlayer ? &a : &b;
                Entity* enemy = aIsEnemy ? &a : &b;
                collisionCallback(player, enemy);
            }

            // Resolve ONLY the player, never the enemy.
            if (aIsPlayer) {
                Vector2 newCenterPlayer = center1 + normal * penetration;
                t1->SetPosition(newCenterPlayer - c1->size * 0.5f);

                if (forceProxy) {
                    if (auto* speed = forceProxy->GetSpeedComponent(a)) {
                        if (normal.x != 0.f) speed->speed.x = 0.f;
                        if (normal.y != 0.f) speed->speed.y = 0.f;
                    }
                }
            }
            else { // bIsPlayer
                Vector2 newCenterPlayer = center2 - normal * penetration;
                t2->SetPosition(newCenterPlayer - c2->size * 0.5f);

                if (forceProxy) {
                    if (auto* speed = forceProxy->GetSpeedComponent(b)) {
                        if (normal.x != 0.f) speed->speed.x = 0.f;
                        if (normal.y != 0.f) speed->speed.y = 0.f;
                    }
                }
            }

            continue;
        }

        /// Player Collides with non player where A is player
        if (aIsPlayer && !bIsPlayer) {
            /// Push the player out of collision
            Vector2 newCenter = center1 + normal * penetration;
            t1->SetPosition(newCenter - c1->size * 0.5f);

            /// Stop the player's movement in the blocked direction
            if (forceProxy) {
                auto speed = forceProxy->GetSpeedComponent(a);
                if (speed) {
                    if (normal.x != 0.f)
                        speed->speed.x = 0.f;
                    if (normal.y != 0.f)
                        speed->speed.y = 0.f;
                }
            }

        }
        /// Player Collides with non player where B is player
        else if (bIsPlayer && !aIsPlayer) {
            /// Push the player out of collision
            Vector2 newCenter = center2 - normal * penetration;
            t2->SetPosition(newCenter - c2->size * 0.5f);

            /// Stop the player's movement in the blocked direction
            if (forceProxy) {
                auto speed = forceProxy->GetSpeedComponent(b);
                if (speed) {
                    if (normal.x != 0.f)
                        speed->speed.x = 0.f;
                    if (normal.y != 0.f)
                        speed->speed.y = 0.f;
                }
            }
        }
        /// Enemy Collides with non player/non enemy where A is enemy
        else if (aIsEnemy && !bIsPlayer && !bIsEnemy) {
            /// Push the enemy out of collision
            Vector2 newCenter = center1 + normal * penetration;
            t1->SetPosition(newCenter - c1->size * 0.5f);

            /// Stop the enemy's movement in the blocked direction
            if (forceProxy) {
                auto speed = forceProxy->GetSpeedComponent(a);
                if (speed) {
                    if (normal.x != 0.f)
                        speed->speed.x = 0.f;
                    if (normal.y != 0.f)
                        speed->speed.y = 0.f;
                }
            }
        }
        /// Enemy Collides with non player/non enemy where B is enemy
        else if (bIsEnemy && !aIsPlayer && !aIsEnemy) {
            /// Push the enemy out of collision
            Vector2 newCenter = center2 - normal * penetration;
            t2->SetPosition(newCenter - c2->size * 0.5f);

            /// Stop the enemy's movement in the blocked direction
            if (forceProxy) {
                auto speed = forceProxy->GetSpeedComponent(b);
                if (speed) {
                    if (normal.x != 0.f)
                        speed->speed.x = 0.f;
                    if (normal.y != 0.f)
                        speed->speed.y = 0.f;
                }
            }
        }
    }
}