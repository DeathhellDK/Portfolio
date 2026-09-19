/**
 * @file     enemiesController.cpp
 * @author   Sng Swee Yong Dillon
 * @co-author Tan Wei Liang Terril, Low Jianlin, Jethro Sung
 * @email    sweeyongdillon.sng, t.weiliangterril, jianlin.low, sung.h
 * @date     2025-11-07
 *
 * @brief    Implements the behavior logic for the EnemiesController component.
 *
 * This file forms the bridge between:
 *  - EnemyAi (decision-making + pathfinding)
 *  - Transform updates in world space
 *  - Optional animation / rendering updates (ApplyState)
 *
 * It currently contains placeholder behavior wiring. Full state-driven
 * visuals and physics interaction will be implemented later in the project.
 */

#include "enemyController.h"
#include "Core/gameApp.h"
#include <iostream>
#include <cmath>
#include "Input/DebugConsole.hpp"
#include "Core/resourceManager.h"
#include "Core/randomiser.h"

 /**
     * @brief Updates the enemy each frame based on AI behavior + world movement.
     *
     * Flow:
     *  1) Push Transform position -> AI (sync physics after collision resolution)
     *  2) AI updates internal state machine (Patrol/Chase/Search)
     *  3) AI returns new world-space movement
     *  4) Apply motion directly to Transform (temporary � will later use physics)
     *  5) Trigger animation state based on behavioral intent (stub)
     *
     * @param dt        Delta time in seconds.
     * @param transform Transform component of the enemy entity (modified in-place).
     * @param playerPos World position of the player for visibility & pursuit logic.
 */
void EnemiesController::Update(float dt, Transform& transform, const Vector2& playerPos, GameApp& app) {
    // Push current Transform -> AI (in case physics/collision moved it)
    ai.setPosition(transform.GetPosition());
    ai.SetComponentEnemyPhysics(self, forceProxy);
    if (hurtIFrame > 0.0f) hurtIFrame -= dt;

    timer += dt;

    Vector2 currentPos = transform.GetPosition();

    // 1. Calculate how far we moved since last frame
    Vector2 prevPos = lastPosition;
    if (!hasLastPosition) {
        prevPos = currentPos;
        lastPosition = currentPos;
        hasLastPosition = true;
    }

    const float dxMoved = currentPos.x - prevPos.x;
    const float dyMoved = currentPos.y - prevPos.y;
    float distMovedSq = (dxMoved * dxMoved) + (dyMoved * dyMoved);
    lastPosition = currentPos;

    if (std::fabs(dxMoved) > 0.001f) {
        facingRight = dxMoved > 0.0f;
    }

    if (attackAnimTimer > 0.0f) {
        attackAnimTimer -= dt;
        if (attackAnimTimer < 0.0f) attackAnimTimer = 0.0f;
    }

    // --- NEW: PROXIMITY ATTACK ATTEMPT AUDIO ---

    // 1. Calculate squared distance to the player
    const float distToPlayerSq = (playerPos.x - currentPos.x) * (playerPos.x - currentPos.x) +
        (playerPos.y - currentPos.y) * (playerPos.y - currentPos.y);

    // 2. Define a "Strike Range" threshold
    // Tweak this number to match the exact size of enemy sprites!
    const float strikeRangeSq = 65.0f * 65.0f;

    // 3. If they enter strike range, and the vocal cooldown is ready
    if (distToPlayerSq <= strikeRangeSq && attackAnimTimer <= 0.0f) {

        // Only trigger this for Melee/Heal/Burrow. 
        if (ai.GetMobType() != MobType::RANGED) {

            PlayRandomEnemyAttack(app, ai.GetMobType(), IsBoss(), glm::vec2(currentPos.x, currentPos.y), 400.0f);

            // Set the cooldown so they don't grunt 60 times a second while overlapping the player
            attackAnimTimer = 1.5f;
        }
    }

    // 2. Individual timer
    footstepTimer += dt;

    // 3. Trigger if enemy moved more than a tiny threshold (0.001f)
    if (distMovedSq > 0.001f && footstepTimer >= 0.45f) {
        //DebugConsole::Get().Info("[AUDIO] Footstep Triggered!\n");

        glm::vec2 spatialPos(currentPos.x, currentPos.y);

        // Determine individual volume
        float individualGain = 0.45f; // Default

        MobType typeMob = ai.GetMobType();

        if (typeMob == MobType::FINALBOSS) individualGain = 0.85f;
        else if (typeMob == MobType::BURROW) individualGain = 0.39f;
        else if (typeMob == MobType::RANGED) individualGain = 0.45f;
        else if (typeMob == MobType::HEAL) individualGain = 0.45f;

        // PlayRandomEnemyFootstep(app, typeMob, spatialPos, individualGain);

        footstepTimer = 0.0f;
    }

    // Projectile ability req only; spawning handled by GameApp
    if (ai.GetMobType() == MobType::RANGED || ai.IsBossRangedPhase()) {
        shootTimer += dt;
        const Vector2 enemyPos = transform.GetPosition();
        const Vector2 toPlayer{ playerPos.x - enemyPos.x, playerPos.y - enemyPos.y };
        const float distSq = toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y;

        float localCooldown = shootCooldown;
        float localRange = shootRange;

        if (IsBoss())
        {
            localCooldown *= 0.6f;
            localRange *= 1.3f;
        }


        if (distSq <= shootRange * shootRange && shootTimer >= shootCooldown)
        {
            const float len = std::sqrt(distSq);
            if (len > 1e-3f) shootDir = Vector2{ toPlayer.x / len, toPlayer.y / len };
            else             shootDir = Vector2{ 1.0f, 0.0f };

            shootRequested = true;
            shootTimer = 0.0f;
        }
    }

    if (!IsBoss())
    {
        if (timer > 1.0 / 60.0) {
            if (!luaDrivenMovement) {
                /*ai.update(dt, playerPos, self, forceProxy);
                transform.SetPosition(ai.getPosition());*/
                ai.update(dt, playerPos, self, forceProxy);

                if (forceProxy) {
                    if (auto* spd = forceProxy->GetSpeedComponent(self)) {
                        Vector2 curr = transform.GetPosition();
                        Vector2 next = ai.getPosition();
                        Vector2 vel = (next - curr) / dt;
                        spd->speed = vel;
                    }
                }
            }
            timer = 0.0;
        }
    }
    else
    {
        if (timer > 1.0f) timer = 0.0f;
    }

    if (ai.GetMobType() == MobType::RANGED) {
        if (SpriteAnimator* an = app.GetAnimator(self)) {
            const bool isAttacking = attackAnimTimer > 0.0f;
            const bool isMoving = distMovedSq > 0.001f;
            const bool dirRight = isAttacking ? attackFacingRight : facingRight;

            const char* key = nullptr;
            if (isAttacking) key = dirRight ? "Attack_ranged_enemy_R" : "Attack_ranged_enemy_L";
            else if (isMoving) key = dirRight ? "Walking_ranged_enemy_R" : "Walking_ranged_enemy_L";
            else key = dirRight ? "Idle_ranged_enemy_R" : "Idle_ranged_enemy_L";

            if (key) {
                const SpriteSheet* sh = ResourceManager::GetSpriteSheet(key);
                if (!sh) sh = ResourceManager::GetSpriteSheet("ranged_enemy");

                if (sh && an->sheet != sh) {
                    an->SetSheet(sh);
                    an->SetSpeedFromFrameDuration(sh->frameDuration);
                    an->Restart();
                }
            }
        }
    }
    else if (ai.GetMobType() == MobType::BASIC) {
        if (SpriteAnimator* an = app.GetAnimator(self)) {
            bool isDashing = false;
            if (luaDrivenMovement) {
                if (dt > 1e-6f) {
                    const float speedNow = std::sqrt(distMovedSq) / dt;
                    isDashing = speedNow >= 250.0f;
                }
            }
            else {
                isDashing = (ai.dashPhase == EnemyAi::DashPhase::DASHING);
            }

            const char* key = nullptr;
            if (isDashing) key = facingRight ? "default_dash_enemy_R" : "default_dash_enemy_L";
            else key = facingRight ? "default_enemy_R" : "default_enemy_L";

            if (key) {
                const SpriteSheet* sh = ResourceManager::GetSpriteSheet(key);

                if (sh && an->sheet != sh) {
                    an->SetSheet(sh);
                    an->SetSpeedFromFrameDuration(sh->frameDuration);
                    an->Restart();
                }
            }
        }
    }
    else if (ai.GetMobType() == MobType::BURROW) {
        if (!hasNormalMeshScale) {
            normalMeshScale = transform.GetScale();
            hasNormalMeshScale = true;
        }

        bool underground = false;
        if (luaDrivenMovement) {
            if (Collider* col = app.GetCollider(self)) {
                underground = col->isBurrowed;
            }
        }
        else {
            underground = (ai.burrowPhase == EnemyAi::BurrowPhase::BURROWED);
        }

        if (underground) {
            if (!burrowMeshScaled) {
                normalMeshScale = transform.GetScale();
            }
            Vector2 s = normalMeshScale;
            s.y *= (1.0f / 3.0f);
            transform.SetScale(s);
            burrowMeshScaled = true;
        }
        else if (burrowMeshScaled) {
            transform.SetScale(normalMeshScale);
            burrowMeshScaled = false;
        }

        if (Collider* col = app.GetCollider(self)) {
            if (col->type == ColliderType::Box) {
                if (!hasNormalColliderSize) {
                    normalColliderSize = col->size;
                    hasNormalColliderSize = true;
                }

                if (underground) {
                    if (!burrowColliderScaled) {
                        normalColliderSize = col->size;
                    }
                    Vector2 cs = normalColliderSize;
                    cs.y *= (1.0f / 3.0f);
                    col->size = cs;
                    burrowColliderScaled = true;
                }
                else if (burrowColliderScaled) {
                    col->size = normalColliderSize;
                    burrowColliderScaled = false;
                }
            }
        }

        if (SpriteAnimator* an = app.GetAnimator(self)) {
            const char* key = nullptr;
            const SpriteSheet* shUnder = ResourceManager::GetSpriteSheet("BurrowUnderEnemy");
            const SpriteSheet* shUnderground = ResourceManager::GetSpriteSheet("underground_burrow_enemy");
            const SpriteSheet* shRising = ResourceManager::GetSpriteSheet("RisingUpEnemy");

            if (luaDrivenMovement) {
                const bool isSpecial = (an->sheet == shUnder) || (an->sheet == shUnderground) || (an->sheet == shRising);
                if (!isSpecial) {
                    key = facingRight ? "Idle_burrow_enemy_R" : "Idle_burrow_enemy_L";
                }
            }
            else {
                switch (ai.burrowPhase) {
                case EnemyAi::BurrowPhase::TELEGRAPHING:
                    key = "BurrowUnderEnemy";
                    break;
                case EnemyAi::BurrowPhase::BURROWED:
                    key = "underground_burrow_enemy";
                    break;
                case EnemyAi::BurrowPhase::STRIKING:
                    key = "RisingUpEnemy";
                    break;
                case EnemyAi::BurrowPhase::ABOVE_GROUND:
                case EnemyAi::BurrowPhase::COOLDOWN:
                default:
                    key = facingRight ? "Idle_burrow_enemy_R" : "Idle_burrow_enemy_L";
                    break;
                }
            }

            if (key) {
                const SpriteSheet* sh = ResourceManager::GetSpriteSheet(key);
                if (!sh) sh = ResourceManager::GetSpriteSheet("burrow_enemy");

                if (sh && an->sheet != sh) {
                    an->SetSheet(sh);
                    an->SetSpeedFromFrameDuration(sh->frameDuration);
                    an->Restart();
                }
            }
        }
    }

    // choose visuals based on �in front / los� for debug states
    if (pendingRenderer && pendingAnimator) {
        // PATROL -> Patrol, CHASE -> Chase, SEARCH -> Idle (placeholder)
        ApplyState(State::Patrol, *pendingRenderer, *pendingAnimator);
    }

}

bool EnemiesController::ConsumeShoot(Vector2& outDir)
{
    if (!shootRequested) return false;
    shootRequested = false;
    outDir = shootDir;
    attackFacingRight = outDir.x >= 0.0f;
    attackAnimTimer = 0.35f;
    return true;
}

/**
 * @brief Applies animation/visuals for the specified state (placeholder).
 *
 * @param s  The state being transitioned into.
 * @param mr The entity's MeshRenderer component.
 * @param an The entity's SpriteAnimator component.
 *
 * @note For now, only debug prints are issued � no renderer/anim state change.
 *       Full animation graph will be implemented once attack/idle/run clips exist.
 */
void EnemiesController::ApplyState(State s, MeshRenderer& mr, SpriteAnimator& an) {
    (void)mr; (void)an; // silence unused params for now
    cur = s;

    // Debug placeholder
    switch (cur) {
    case State::Idle:   DebugConsole::Get().Info("[Enemy] Idle\n"); break;
    case State::Patrol: DebugConsole::Get().Info("[Enemy] Patrol\n"); break;
    case State::Chase:  DebugConsole::Get().Info("[Enemy] Chase\n"); break;
    }
}

void EnemiesController::SetGrid(const std::vector<std::string>& grid, float tileSize)
{
    ai.SetGrid(grid, tileSize);
}

bool EnemiesController::TakeDamage(int dmg)
{
    if (dmg <= 0) return false;

    // so a bullet doesn't hit multiple times in the same overlap scenario
    if (hurtIFrame > 0.0f) return false;

    hp -= dmg;
    hurtIFrame = 0.08f;

    if (hp <= 0)
        return true;

    return false;
}

void EnemiesController::ResetHealth()
{
    hp = maxHp;
}
