/**
* @file states.cpp
* @author     Lim Zhi Jie
* @co-author  Sng Swee Yong Dillon
* @email      zhijie.lim, sweeyongdillon.sng
* @date       2026-03-07
* @brief FSM state implementations.
*
* State flow:
*
*   PatrolState
*     -> ChaseState                (player spotted)
*
*   ChaseState
*     -> [Typed]AttackState        (player within attack range)
*     -> SearchState               (sight lost)
*
*   [Typed]AttackState
*     -> ChaseState                (player moved out of attack range)
*     -> SearchState               (sight lost)
*
*   SearchState
*     -> ChaseState                (player re-spotted during scan)
*     -> ReturnToSpawnState        (scan completed, player not found)
*
*   ReturnToSpawnState
*     -> ChaseState                (player spotted en route to spawn)
*     -> PatrolState               (physically arrived at spawn)
*
* Ranged enemy combo (RangedAttackState):
*   Shots 1-3 : "RangedNormalAttack" (single, 2 s apart)
*   Shots 4-6 : "RangedSpreadAttack" (3-projectile fan, 0.4 s apart)
*   After shot 6 : 3 s combo cooldown, then repeat
*
* Bless mob group behaviour:
*   BlessHealAllyState      (while ranged ally alive)
*     -> BlessAttackPlayerState  (rangedAllyAlive == false)
*
*   BlessAttackPlayerState  (after ranged ally dies)
*     -> SearchState             (sight lost)
*
* Player ability absorption:
*   Absorbing RANGED ability  -> player gains projectile shoot
*   Absorbing BLESS ability   -> player gains:
*       bless on self   = heal
*       bless on enemy  = slow homing projectile (never misses)
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "World/states.h"
#include "World/ai.h"

 /**
  * @brief Factory: returns the correct attack state for this enemy's mob type.
  * Only this function needs updating when a new MobType is added.
  * @param enemy The enemy AI component.
  * @return Unique pointer to the created attack state.
  */
 static std::unique_ptr<State> MakeAttackState(EnemyAi* enemy)
{
    switch (enemy->GetMobType())
    {
    case MobType::RANGED:    return std::make_unique<RangedAttackState>();
    case MobType::BURROW:    return std::make_unique<BurrowAttackState>();
    case MobType::HEAL:      return std::make_unique<BlessHealAllyState>();
    case MobType::FINALBOSS: return std::make_unique<BossAttackState>();
    case MobType::BASIC:
    default:                 return std::make_unique<BasicAttackState>();
    }
}

// ===========================================================================
// PatrolState  (shared)
// ===========================================================================
void PatrolState::Enter(EnemyAi* /* enemy */) {}

void PatrolState::Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy)
{
    if (enemy->playerInFront(playerPos) && enemy->withinLineOfSight(playerPos))
    {
        enemy->SendMessage("EnemySpottedPlayer");
        enemy->GetFSM()->ChangeState(std::make_unique<ChaseState>());
        return;
    }

    enemy->updatePatrol(deltaTime, e, forceproxy);
}

void PatrolState::Exit(EnemyAi* /* enemy */) {}

// ===========================================================================
// ChaseState  (shared)
// ===========================================================================
void ChaseState::Enter(EnemyAi* /* enemy */) {}

void ChaseState::Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy)
{
    // Sight lost -> search
    if (!enemy->playerInFront(playerPos) && !enemy->withinLineOfSight(playerPos))
    {
        enemy->searchTimer = 0.0f;
        enemy->GetFSM()->ChangeState(std::make_unique<SearchState>());
        return;
    }

    // Within attack range -> hand off to the typed attack state
    if (enemy->withinAttackRange(playerPos))
    {
        enemy->GetFSM()->ChangeState(MakeAttackState(enemy));
        return;
    }

    // Still too far away � keep closing in
    enemy->updateChase(deltaTime, playerPos, e, forceproxy);
}

void ChaseState::Exit(EnemyAi* /* enemy */) {}

// ===========================================================================
// SearchState  (shared)
// Walk to last known position, scan 360 degrees, then give up.
// ===========================================================================
void SearchState::Enter(EnemyAi* enemy)
{
    enemy->searchTimer = 0.0f;
}

void SearchState::Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity /* e */, EntityForceProxy* /* forceproxy */)
{
    // Re-spotted player -> chase
    if (enemy->playerInFront(playerPos) || enemy->withinLineOfSight(playerPos))
    {
        enemy->GetFSM()->ChangeState(std::make_unique<ChaseState>());
        return;
    }

    // updateSearch advances the timer and scan; sets searchDone = true when
    // the full 360-degree scan completes without finding the player
    enemy->updateSearch(deltaTime);

    if (enemy->searchDone)
        enemy->GetFSM()->ChangeState(std::make_unique<ReturnToSpawnState>());
}

void SearchState::Exit(EnemyAi* enemy)
{
    enemy->searchDone = false; // reset for next time
}

// ===========================================================================
// ReturnToSpawnState  (shared)
// Walk back to spawn. Re-enters chase loop if player is spotted en route.
// ===========================================================================
void ReturnToSpawnState::Enter(EnemyAi* /* enemy */) {}

void ReturnToSpawnState::Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy)
{
    // Player spotted while returning -> chase immediately
    if (enemy->playerInFront(playerPos) && enemy->withinLineOfSight(playerPos))
    {
        enemy->SendMessage("EnemySpottedPlayer");
        enemy->GetFSM()->ChangeState(std::make_unique<ChaseState>());
        return;
    }

    // moveTowardSpawn sets atSpawn = true when the enemy arrives
    enemy->moveToSpawn(deltaTime, e, forceproxy);

    if (enemy->atSpawn)
        enemy->GetFSM()->ChangeState(std::make_unique<PatrolState>());
}

void ReturnToSpawnState::Exit(EnemyAi* /* enemy */) {}

// ===========================================================================
// BasicAttackState
// Dash attack - dashes 3 tiles toward the player, rests 2s, repeats.
// Resumes chase if player steps out of range between dashes (READY phase only).
// Does NOT interrupt a dash or rest mid-way.
// ===========================================================================
void BasicAttackState::Enter(EnemyAi* /* enemy */) {}

void BasicAttackState::Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy)
{
    // Only check transitions when READY - never interrupt an active dash or rest
    if (enemy->dashPhase == EnemyAi::DashPhase::READY)
    {
        if (!enemy->withinLineOfSight(playerPos))
        {
            enemy->searchTimer = 0.0f;
            enemy->GetFSM()->ChangeState(std::make_unique<SearchState>());
            return;
        }

        if (!enemy->withinAttackRange(playerPos))
        {
            enemy->GetFSM()->ChangeState(std::make_unique<ChaseState>());
            return;
        }
    }

    enemy->updateBasic(deltaTime, playerPos, e, forceproxy);
}

void BasicAttackState::Exit(EnemyAi* enemy)
{
    // Reset dash so it starts fresh next time the enemy enters this state
    enemy->dashPhase = EnemyAi::DashPhase::READY;
    enemy->dashRestTimer = 0.0f;
    enemy->dashRemaining = 0.0f;
}

// ===========================================================================
// RangedAttackState
// Maintains optimal distance and fires the 6-shot combo:
//   Shots 1-3: single projectile ("RangedNormalAttack")
//   Shots 4-6: 3-projectile fan  ("RangedSpreadAttack") fired fast
// After 6 shots a 3 s combo cooldown resets the sequence.
// ===========================================================================
void RangedAttackState::Enter(EnemyAi* /* enemy */) {}

void RangedAttackState::Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy)
{
    if (!enemy->withinLineOfSight(playerPos))
    {
        enemy->searchTimer = 0.0f;
        enemy->GetFSM()->ChangeState(std::make_unique<SearchState>());
        return;
    }

    if (!enemy->withinAttackRange(playerPos))
    {
        enemy->GetFSM()->ChangeState(std::make_unique<ChaseState>());
        return;
    }

    enemy->updateRanged(deltaTime, playerPos, e, forceproxy);
}

void RangedAttackState::Exit(EnemyAi* enemy)
{
    // Reset combo so re-entry always starts from shot 1
    enemy->rangedShotsFired = 0;
}

// ===========================================================================
// Drives the burrow enemy's whack-a-mole attack cycle.
//
// Transition rules:
//   - Sight/range checks ONLY fire when the enemy is in ABOVE_GROUND phase.
//     Every other phase (TELEGRAPHING, BURROWED, STRIKING, COOLDOWN) is
//     committed and must finish before the FSM can leave this state.
//
//   - On Exit(), the burrow phase is reset to ABOVE_GROUND so the next entry
//     starts cleanly regardless of how the state was interrupted.
// ===========================================================================
void BurrowAttackState::Enter(EnemyAi* /* enemy */) {}

void BurrowAttackState::Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy)
{
    // Only check FSM transitions when fully above ground and not committed.
    // TELEGRAPHING, BURROWED, STRIKING, and COOLDOWN phases all run to
    // completion before we allow a state change.
    if (enemy->burrowPhase == EnemyAi::BurrowPhase::ABOVE_GROUND)
    {
        if (!enemy->withinLineOfSight(playerPos))
        {
            enemy->searchTimer = 0.0f;
            enemy->GetFSM()->ChangeState(std::make_unique<SearchState>());
            return;
        }

        if (!enemy->withinAttackRange(playerPos))
        {
            enemy->GetFSM()->ChangeState(std::make_unique<ChaseState>());
            return;
        }
    }

    enemy->updateBurrow(deltaTime, playerPos, e, forceproxy);
}

void BurrowAttackState::Exit(EnemyAi* enemy)
{
    // Reset phase so re-entry is always clean (e.g. enemy was chased away
    // mid-cooldown and comes back into attack range later).
    enemy->burrowPhase = EnemyAi::BurrowPhase::ABOVE_GROUND;
    enemy->burrowPhaseTimer = 0.0f;
    enemy->isBurrowed = false;
}

// ===========================================================================
// HealAttackState  (kept for backwards compatibility � routes to bless logic)
// ===========================================================================
void HealAttackState::Enter(EnemyAi* /* enemy */) {}

void HealAttackState::Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy)
{
    enemy->updateHeal(deltaTime, playerPos, e, forceproxy);
}

void HealAttackState::Exit(EnemyAi* /* enemy */) {}

// ===========================================================================
// BlessHealAllyState
//
// The bless mob's primary state while its ranged ally is alive.
// It ignores the player and focuses entirely on keeping the ally healthy.
//
// IMPORTANT: The game system (spawner / group manager) must pass the ranged
// ally's world position as 'playerPos' when calling enemy->update() for a
// bless mob in this state.  This lets updateBless() navigate toward the ally
// without needing a direct pointer.
//
// Transitions:
//   -> BlessAttackPlayerState  immediately when rangedAllyAlive == false
//      (set by EnemyGroup / game system calling NotifyRangedAllyDied())
// ===========================================================================
void BlessHealAllyState::Enter(EnemyAi* enemy)
{
    enemy->isCastingBless = false;
    enemy->blessActionTimer = 0.0f;
    enemy->blessCooldownTimer = 0.0f;
}

void BlessHealAllyState::Update(EnemyAi* enemy, float deltaTime, Vector2 allyPos, Entity e, EntityForceProxy* forceproxy)
{
    // As soon as the ranged ally dies, switch to aggression immediately
    if (!enemy->IsRangedAllyAlive())
    {
        enemy->isCastingBless = false;
        enemy->GetFSM()->ChangeState(std::make_unique<BlessAttackPlayerState>());
        return;
    }

    // Pass ally position as the target; updateBless handles navigation + casting
    enemy->updateBless(deltaTime, allyPos, e, forceproxy);
}

void BlessHealAllyState::Exit(EnemyAi* enemy)
{
    enemy->isCastingBless = false;
    enemy->blessActionTimer = 0.0f;
}

// ===========================================================================
// BlessAttackPlayerState
//
// Activated after the ranged ally dies.  The bless mob chases the player
// and fires slow homing bless projectiles that always connect.
//
// Transitions:
//   -> SearchState  if line of sight to the player is lost
// ===========================================================================
void BlessAttackPlayerState::Enter(EnemyAi* enemy)
{
    enemy->isCastingBless = false;
    enemy->blessActionTimer = 0.0f;
    enemy->blessCooldownTimer = 0.0f;
    enemy->SendMessage("BlessAllyDied"); // notify renderer/audio of mode switch
}

void BlessAttackPlayerState::Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy)
{
    // Sight lost � search for player
    if (!enemy->withinLineOfSight(playerPos))
    {
        enemy->searchTimer = 0.0f;
        enemy->GetFSM()->ChangeState(std::make_unique<SearchState>());
        return;
    }

    enemy->updateBless(deltaTime, playerPos, e, forceproxy);
}

void BlessAttackPlayerState::Exit(EnemyAi* enemy)
{
    enemy->isCastingBless = false;
    enemy->blessActionTimer = 0.0f;
}

// ===========================================================================
// Drives the final boss 4-ability sequence:
//   MELEE -> RANGED_COMBO -> BLESS_HEAL -> BURROW -> MELEE -> ...
//
// The boss never leaves this state through normal sight/range checks while
// mid-sequence (RANGED_COMBO, BLESS_HEAL, or BURROW phases). Only in the
// MELEE approach phase can it transition to SearchState if sight is lost.
//
// On Exit() the full sequence and burrow state are reset so re-entry is clean.
// ===========================================================================
void BossAttackState::Enter(EnemyAi* enemy)
{
    enemy->bossSequencePhase = EnemyAi::BossSequencePhase::MELEE;
    enemy->bossRangedShots = 0;
    enemy->bossAbilityTimer = 0.0f;
    enemy->bossAbilityDone = false;
    enemy->burrowPhase = EnemyAi::BurrowPhase::ABOVE_GROUND;
    enemy->burrowPhaseTimer = 0.0f;
    enemy->isBurrowed = false;
}

void BossAttackState::Update(EnemyAi* enemy, float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy)
{
    // Only allow interruption during the MELEE approach (boss is mobile).
     // All other sequence phases run to completion.
    if (enemy->bossSequencePhase == EnemyAi::BossSequencePhase::MELEE &&
        !enemy->bossAbilityDone)
    {
        if (!enemy->withinLineOfSight(playerPos))
        {
            enemy->searchTimer = 0.0f;
            enemy->GetFSM()->ChangeState(std::make_unique<SearchState>());
            return;
        }
    }

    enemy->updateBoss(deltaTime, playerPos, e, forceproxy);
}

void BossAttackState::Exit(EnemyAi* enemy)
{
    enemy->bossSequencePhase = EnemyAi::BossSequencePhase::MELEE;
    enemy->bossRangedShots = 0;
    enemy->bossAbilityTimer = 0.0f;
    enemy->bossAbilityDone = false;
    enemy->burrowPhase = EnemyAi::BurrowPhase::ABOVE_GROUND;
    enemy->burrowPhaseTimer = 0.0f;
    enemy->isBurrowed = false;
}