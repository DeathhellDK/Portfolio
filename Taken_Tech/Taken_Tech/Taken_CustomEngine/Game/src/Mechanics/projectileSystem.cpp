/**
 * @file      projectileSystem.cpp
 * @author    Tan Wei Liang Terril
 * @email      t.weiliangterril
 * @date      2026-01-30
 *
 * @brief     Implements ProjectileSystem, which simulates projectile entities
 *            each frame (movement, lifetime countdown, collision checks, and
 *            cleanup).
 *
 * This system iterates all active ProjectileComponent instances stored by
 * GameApp and performs:
 *  - Lifetime management: remainingLife is decremented and expired projectiles
 *    are destroyed.
 *  - Movement integration: Transform position is advanced by velocity * dt.
 *  - Collision against world colliders: AABB overlap against non-trigger
 *    colliders, with special handling to allow trigger colliders only when
 *    the target is a PuzzleSystem ShootTarget.
 *  - Puzzle integration: when a ShootTarget is hit, PuzzleSystem::OnProjectileHit()
 *    is invoked to increment hit counters and potentially activate linked groups.
 *  - Player damage: enemy-fired projectiles (fromEnemy=true) additionally check
 *    collision with the player and apply damage using Interaction.
 *
 * Design notes:
 *  - Projectile spawning is kept in GameApp / ability logic; this system only
 *    updates existing projectiles.
 *  - Controllers/enemy controllers are ignored during world-collider checks so
 *    that projectile-vs-actor damage rules can be handled explicitly (e.g. enemy
 *    bullets only damage the player).
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */
//#include "Mechanics/interaction.hpp"
#include "Mechanics/projectileSystem.h"
#include "Mechanics/interaction.hpp"
#include "World/puzzleSystem.h"
#include "World/mapGenerator.h"
#include "Core/gameApp.h"
#include "Core/randomiser.h"
#include "Core/resourceManager.h"
#include <algorithm>
#include <vector>

static bool AabbOverlap(const Vector2& aPos, const Vector2& aScale,
    const Vector2& bPos, const Vector2& bScale)
{
    const float aMinX = aPos.x;
    const float aMaxX = aPos.x + aScale.x;
    const float aMinY = aPos.y;
    const float aMaxY = aPos.y + aScale.y;

    const float bMinX = bPos.x;
    const float bMaxX = bPos.x + bScale.x;
    const float bMinY = bPos.y;
    const float bMaxY = bPos.y + bScale.y;

    const bool overlapX = (aMinX < bMaxX) && (aMaxX > bMinX);
    const bool overlapY = (aMinY < bMaxY) && (aMaxY > bMinY);
    return overlapX && overlapY;
}

// ---------------------------------------------------------------------------
// Channeler mini-boss omni-burst direction table
//
// Maps each "BossOmniShot_*" message string to a pre-normalised unit vector.
// Used by GameApp's message handler (the same handler that already processes
// "RangedNormalAttack" / "RangedSpreadAttack") to spawn a slow projectile in
// the correct direction without needing any positional maths in ai.cpp.
//
// How to wire this in GameApp (wherever RangedNormalAttack is handled):
//
//   #include "Mechanics/projectileSystem.h"
//   ...
//   const Vector2* omniDir = ProjectileSystem::GetOmniShotDirection(messageId);
//   if (omniDir)
//   {
//       // Same spawn call you use for "RangedNormalAttack", but:
//       //   velocity  = *omniDir * OMNI_SHOT_SPEED   (slow — use ~80.0f)
//       //   fromEnemy = true
//       //   source    = channelerEntity
//   }
// ---------------------------------------------------------------------------
static const struct { const char* msg; float dx; float dy; } kOmniDirs[] = {
    { "BossOmniShot_N",   0.0f,     1.0f    },
    { "BossOmniShot_NE",  0.7071f,  0.7071f },
    { "BossOmniShot_E",   1.0f,     0.0f    },
    { "BossOmniShot_SE",  0.7071f, -0.7071f },
    { "BossOmniShot_S",   0.0f,    -1.0f    },
    { "BossOmniShot_SW", -0.7071f, -0.7071f },
    { "BossOmniShot_W",  -1.0f,     0.0f    },
    { "BossOmniShot_NW", -0.7071f,  0.7071f },
};
static constexpr int kOmniDirCount = static_cast<int>(sizeof(kOmniDirs) / sizeof(kOmniDirs[0]));

// Recommended projectile speed for omni-burst shots — slow so the player has
// time to read all 8 directions and dodge.
static constexpr float kOmniShotSpeed = 80.0f;


void ProjectileSystem::Update(GameApp& app, float dt)
{
    auto& projectiles = app.GetProjectiles();
    const auto& allColliders = app.GetAllColliders();
    if (projectiles.empty()) return;

    Transform* pt = (app.playerEntity != INVALID_ENTITY) ? app.GetTransform(app.playerEntity) : nullptr;
    PlayerController* pc = (app.playerEntity != INVALID_ENTITY) ? app.GetController(app.playerEntity) : nullptr;

    std::vector<Entity> toDestroy;
    toDestroy.reserve(projectiles.size());

    for (auto& [e, projPtr] : projectiles)
    {
        if (!projPtr) { toDestroy.push_back(e); continue; }
        ProjectileComponent& proj = *projPtr;
        Transform* t = app.GetTransform(e);
        if (!t) { toDestroy.push_back(e); continue; }

        proj.remainingLife -= dt;
        if (proj.remainingLife <= 0.0f)
        {
            toDestroy.push_back(e);
            continue;
        }

        Vector2 p = t->GetPosition();
        p.x += proj.velocity.x * dt;
        p.y += proj.velocity.y * dt;
        t->SetPosition(p);

        const Vector2 bPos = t->GetPosition();
        const Vector2 bScale = t->GetScale();

        bool hitWorld = false;

        Entity hitPuzzle = INVALID_ENTITY;

        // Get puzzle system to check for shoot targets
        PuzzleSystem* puzzleSys = app.GetPuzzleSystem();

        for (const auto& [other, col] : allColliders)
        {
            //(void)col;
            if (other == e) continue;              // don't hit self
            if (other == proj.source) continue;    // don't instantly hit shooter

            // Ignore actors
            if (app.GetController(other)) continue;
            if (app.GetEnemyController(other)) continue;

            // Ignore other projectiles
            if (projectiles.find(other) != projectiles.end()) continue;

            if (col->isTrigger)
            {
                if (!(puzzleSys && puzzleSys->IsShootTarget(other)))
                    continue;
            }

            Transform* ot = app.GetTransform(other);
            if (!ot) continue;

            if (AabbOverlap(bPos, bScale, ot->GetPosition(), ot->GetScale()))
            {
                // Check if this is a ShootTarget puzzle object
                if (puzzleSys && puzzleSys->IsShootTarget(other))
                {
                    hitPuzzle = other;
                    hitWorld = true; // Destroy projectile after hitting target
                    break;
                }


                hitWorld = true;
                break;
            }
        }

        // If we hit a shoot target, notify the puzzle system
        if (hitPuzzle != INVALID_ENTITY && puzzleSys)
        {
            puzzleSys->OnProjectileHit(hitPuzzle, !proj.fromEnemy);
        }


        if (hitWorld)
        {
            toDestroy.push_back(e);
            continue; // skip player-hit check
        }

        // Player bullets damage enemies; enemy bullets damage player only
        if (!proj.fromEnemy)
        {
            // Prefer collider size when present; otherwise use transform scale
            Vector2 bulletSize = t->GetScale();
            if (Collider* bc = app.GetCollider(e))
            {
                bulletSize = (bc->type == ColliderType::Box) ? bc->size : Vector2(bc->size.x * 2.0f, bc->size.x * 2.0f);
            }
            Vector2 bSize = bulletSize;

            for (const auto& [enemyEnt, enemyCol] : allColliders)
            {
                if (!app.GetEnemyController(enemyEnt)) continue; // only enemies
                Transform* et = app.GetTransform(enemyEnt);
                if (!et) continue;

                Vector2 ePos = et->GetPosition();
                Vector2 eSize = et->GetScale();
                if (enemyCol)
                {
                    if (enemyCol->type == ColliderType::Box) eSize = enemyCol->size;
                    else if (enemyCol->type == ColliderType::Circle) eSize = Vector2(enemyCol->size.x * 2.0f, enemyCol->size.x * 2.0f);
                }

                if (AabbOverlap(bPos, bSize, ePos, eSize))
                {
                    if (EnemiesController* ec = app.GetEnemyController(enemyEnt))
                    {
                        const int dmg = proj.damage;
                        if (ec->TakeDamage(dmg))
                        {
                            const std::string bakedName = app.GetEntityNameByEntity(enemyEnt);
                            MapGenerator::MarkEnemyDefeatedByName(bakedName);
                            app.OnEnemyDeath(enemyEnt);
                            Interaction::NotifyFirstAbsorbCorpseCreated(app, enemyEnt);
                        }
                        else {
                            PlayRandomEnemyDamaged(app, ec->GetMobType(), ec->IsBoss(), glm::vec2(ePos.x, ePos.y));
                        }
                    }
                    toDestroy.push_back(e);
                    break;
                }
            }
        }

        // Enemy bullets damage player only
        if (proj.fromEnemy && pt && pc && pc->godmode == false)
        {
            const Vector2 pScale = pt->GetScale();
            const Vector2 bxScale = t->GetScale();

            const Vector2 pCenter{ pt->GetPosition().x + pScale.x * 0.5f, pt->GetPosition().y + pScale.y * 0.5f };
            const Vector2 bCenter{ t->GetPosition().x + bxScale.x * 0.5f, t->GetPosition().y + bxScale.y * 0.5f };

            const float pr = 0.5f * std::min(pScale.x, pScale.y);
            const float br = 0.5f * std::min(bxScale.x, bxScale.y);
            const float dx = pCenter.x - bCenter.x;
            const float dy = pCenter.y - bCenter.y;
            const float r = pr + br;

            static float hitSoundCooldown = 0.0f;
            hitSoundCooldown -= dt;

            if (dx * dx + dy * dy <= r * r)
            {

                int oldHp = pc->getPlayerHp();
                if (oldHp > 0 && hitSoundCooldown <= 0.0f)
                {
                    PlayRandomProjectileShotSfx(3.1f);
                    hitSoundCooldown = 0.08f; // This limits hit sounds to 12 per second max
                }
                int newHp = std::max(0, oldHp - proj.damage);
                pc->setPlayerHp(newHp);
                if (pc->getCheckUnderground() == false) {
                    Interaction::HandlePlayerDamage(app, app.playerEntity, proj.damage);
                }
                

                /*DebugConsole::Get().AddFormattedMessage(LogLevel::Info,
                    "[Projectile] Player hit! HP: ", oldHp, " -> ", newHp, " (damage = ", proj.damage, ")\n");*/


                toDestroy.push_back(e);
            }
        }
    }

    for (Entity e : toDestroy)
        app.DestroyEntityNoExpose(e);
}

// ---------------------------------------------------------------------------
// ProjectileSystem::GetOmniShotDirection
//
// Called by the GameApp message handler to resolve a "BossOmniShot_*" message
// into a spawn direction.  Returns nullptr for any non-omni-shot message so
// the caller can use a simple null-check rather than a string comparison chain.
//
// The returned pointer is to a static table — it remains valid for the
// lifetime of the program and must not be freed.
// ---------------------------------------------------------------------------
const Vector2* ProjectileSystem::GetOmniShotDirection(const std::string& messageId)
{
    // Static Vector2 wrappers over the direction table so we can return a
    // typed pointer without depending on a global Vector2 array.
    static const Vector2 kDirVecs[kOmniDirCount] = {
        { kOmniDirs[0].dx, kOmniDirs[0].dy },
        { kOmniDirs[1].dx, kOmniDirs[1].dy },
        { kOmniDirs[2].dx, kOmniDirs[2].dy },
        { kOmniDirs[3].dx, kOmniDirs[3].dy },
        { kOmniDirs[4].dx, kOmniDirs[4].dy },
        { kOmniDirs[5].dx, kOmniDirs[5].dy },
        { kOmniDirs[6].dx, kOmniDirs[6].dy },
        { kOmniDirs[7].dx, kOmniDirs[7].dy },
    };

    for (int i = 0; i < kOmniDirCount; ++i)
    {
        if (messageId == kOmniDirs[i].msg)
            return &kDirVecs[i];
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// ProjectileSystem::GetOmniShotSpeed
//
// Returns the recommended projectile speed for omni-burst shots.
// Kept as a function so GameApp never needs to hard-code the constant.
// ---------------------------------------------------------------------------
float ProjectileSystem::GetOmniShotSpeed()
{
    return kOmniShotSpeed;
}
