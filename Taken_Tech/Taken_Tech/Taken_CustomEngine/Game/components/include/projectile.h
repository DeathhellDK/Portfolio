#pragma once
/**
 * @file      projectileComponent.h
 * @author    Tan Wei Liang Terril
 * @email      t.weiliangterril
 * @date      2026-01-30
 *
 * @brief     Declares ProjectileComponent, an ECS component that stores the
 *            runtime state for projectile entities (movement, lifetime, and
 *            damage attribution).
 *
 * ProjectileComponent is attached to projectile entities spawned by either
 * the player or enemies. It stores:
 *  - source:         The entity that fired/spawned the projectile (for ignoring
 *                    self-collisions and attributing hits).
 *  - velocity:       The projectile's per-second movement vector.
 *  - remainingLife:  Time remaining before the projectile despawns.
 *  - damage:         Damage applied on a valid hit.
 *  - fromEnemy:      True if spawned by an enemy, false if spawned by player.
 *
 * This component is typically updated by ProjectileSystem, which integrates
 * movement, decrements lifetime, and destroys the projectile on expiry or hit.
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */
#include "Core/component.h"
#include "Math/vect2.h"

struct ProjectileComponent : public Component
{
    /**
     * @brief Constructs a new ProjectileComponent.
     * @param self The entity ID of this projectile.
     * @param sourceEntity The entity ID that spawned this projectile.
     * @param vel Initial velocity vector.
     * @param lifeSeconds Duration in seconds before the projectile despawns.
     * @param dmg Amount of damage dealt on impact.
     * @param fromEnemy_ True if spawned by an enemy, false if by player.
     */
    ProjectileComponent(Entity self,
        Entity sourceEntity,
        const Vector2& vel,
        float lifeSeconds,
        int dmg,
        bool fromEnemy_)
        : Component(self),
        source(sourceEntity),
        velocity(vel),
        remainingLife(lifeSeconds),
        damage(dmg),
        fromEnemy(fromEnemy_) {
    }

    Entity  source = INVALID_ENTITY;
    Vector2 velocity{ 0.0f, 0.0f };
    float   remainingLife = 0.0f;
    int     damage = 1;
    bool    fromEnemy = true;
};