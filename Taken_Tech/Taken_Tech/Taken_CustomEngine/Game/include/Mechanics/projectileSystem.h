/**
 * @file      projectileSystem.h
 * @author    Tan Wei Liang Terril
 * @email      t.weiliangterril
 * @date      2026-01-30
 *
 * @brief     Declares ProjectileSystem, the gameplay system responsible for
 *            updating projectile entities (movement, lifetime, collision, and
 *            despawning).
 *
 * ProjectileSystem processes all entities that contain a ProjectileComponent.
 * Each update tick it:
 *  - Integrates projectile movement using its velocity.
 *  - Decrements remaining lifetime and destroys expired projectiles.
 *  - Performs hit detection against relevant colliders.
 *  - Notifies other systems (e.g., PuzzleSystem) when a projectile hits a
 *    puzzle target such as ShootTarget.
 *
 * Projectile spawning is intentionally kept in GameApp (via player abilities
 * or enemy attacks), while this system focuses only on runtime projectile
 * simulation and cleanup.
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */
#pragma once

#include "Core/entitymanager.h"
#include "Math/vect2.h"
#include <string>

class GameApp;
struct ProjectileComponent; // forward declaration

/**
 * @brief Updates projectile entities (movement, lifetime, hit detection, despawn).
 *
 * Spawn is intentionally kept in GameApp (ability triggers -> entity creation).
 */
class ProjectileSystem
{
public:
    /**
     * @brief Updates projectile entities logic for the current frame.
     * 
     * Integrates projectile movement using its velocity, decrements remaining lifetime,
     * performs hit detection, and notifies other systems upon hitting targets.
     * 
     * @param app Reference to the GameApp instance.
     * @param dt Time elapsed since the last frame.
     */
    void Update(GameApp& app, float dt);

    // -----------------------------------x`------------------------------------
    // Channeler mini-boss omni-burst helpers
    //
    // Call GetOmniShotDirection(messageId) inside the same GameApp message
    // handler that already handles "RangedNormalAttack".  If the message is
    // one of the eight "BossOmniShot_*" strings the function returns a pointer
    // to the pre-normalised direction vector; otherwise it returns nullptr.
    //
    // Example usage in GameApp:
    //
    //   const Vector2* dir = ProjectileSystem::GetOmniShotDirection(msg);
    //   if (dir)
    //   {
    //       // spawn slow enemy projectile at channelerEntity position
    //       // velocity  = (*dir) * ProjectileSystem::GetOmniShotSpeed()
    //       // fromEnemy = true,  source = channelerEntity
    //   }
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the unit-vector direction for a "BossOmniShot_*" message.
     * @param messageId  The message string received from EnemyAi::SendMessage.
     * @return Pointer to a Vector2 direction, or nullptr if not an omni-shot message.
     */
    static const Vector2* GetOmniShotDirection(const std::string& messageId);

    /**
     * @brief Returns the recommended speed for omni-burst projectiles (80 u/s).
     *        Slow enough that the player can read all 8 directions and dodge.
     */
    static float GetOmniShotSpeed();
};