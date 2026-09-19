/**
* @file     ai.cpp
* @author   Lim Zhi Jie
* @email    zhijie.lim,t.weiliangterril, sweeyongdillon.sng
* @co-author Tan Wei Liang Terril, Sng Swee Yong Dillon
* @date     2025-11-05
*
* @brief Implements the Enemy AI behavior system for all mob types.
*
* This file defines and implements the logic that drives enemy behavior
* in the game world, including patrol, chase, search, ranged attack,
* burrow, heal, and boss adaptations. Each enemy type follows unique
* state transitions and uses pathfinding and vision checks to determine
* how to react to the player's presence.
*
* The system supports messaging via the Messaging::Observable interface
* to broadcast key gameplay events such as when an enemy spots, loses,
* or stops chasing the player. These messages can be observed by
* GameApp or other systems
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
#include "World/ai.h"
#include "World/states.h"
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <random>
#include "Math/vect2.h"

// ---------------------------------------------------------------------------
// Internal helper: move a point toward a target at a fixed speed
// ---------------------------------------------------------------------------
/**
 * @brief Internal helper to move a point toward a target at a fixed speed.
 * 
 * @param pos Current position (updated in place).
 * @param target Target position.
 * @param speed Movement speed.
 * @param dt Delta time.
 */
static void stepToward(Vector2& pos, const Vector2& target, float speed, float dt) {
	Vector2 d{ target.x - pos.x, target.y - pos.y };
	float dist = std::sqrt(d.x * d.x + d.y * d.y);
	if (dist < 1.0f) { pos = target; return; }
	d.x /= dist; d.y /= dist;
	float step = speed * dt;
	if (step > dist) step = dist;
	pos.x += d.x * step;
	pos.y += d.y * step;
}

// ===========================================================================
// Constructor
// ===========================================================================
EnemyAi::EnemyAi(Entity e, MobType type, float searchDuration, Direction SearchDirection) :
	Component(e),
	mobType{ type },
	patrolRange(5),
	visionRange(3),
	currentDirection(SearchDirection),
	patrolDirection(SearchDirection),
	searchTimer(0.0f),
	searchDone(false),
	atSpawn(false),
	isBurrowed(false),
	hp(0),
	damage(0),
	fsm(std::make_unique<FSM>(this))
{
	(void)searchDuration;
	Vector2 grid = WorldToGrid(position, tileSize);
	position = GridToWorld(grid, tileSize);

	// Legacy enum — kept for any non-FSM code that still reads currentState
	ChangeState(StateType::PATROL);

	// Per-type stats
	switch (mobType) {
	case MobType::BASIC:     hp = 10;  damage = 1; break;
	case MobType::RANGED:    hp = 6;   damage = 3; break;
	case MobType::BURROW:    hp = 10;  damage = 5; break;
	case MobType::HEAL:      hp = 13;  damage = 2; break;
	case MobType::FINALBOSS:
		hp = 100; damage = 5;
		bossSequencePhase  = BossSequencePhase::MELEE;
		bossAbilityTimer   = 0.0f;
		bossAbilityDone    = false;
		bossOmniShotsFired = 0;
		bossOmniComboPhase = ChannelerComboPhase::NORMAL;
		bossOmniBurstIndex = 0;
		bossSanctifyTimer  = 0.0f;
		bossShockPhase     = TremorShockPhase::NONE;
		bossShockTimer     = 0.0f;
		bossShockIndex     = 0;
		break;
	// Mini-bosses — higher HP, inherit damage from base type
	case MobType::MINIBOSS_CHANNELER:
		hp = 20; damage = 3;
		channelerComboPhase = ChannelerComboPhase::NORMAL;
		channelerShotsFired = 0;
		channelerBurstIndex = 0;
		break;
	case MobType::MINIBOSS_WARDEN:
		hp = 30; damage = 2;
		wardenPhase       = WardenPhase::MELEE;
		wardenMeleeHits   = 0;
		wardenActionTimer = 0.0f;
		wardenHp          = 30;
		break;
	case MobType::MINIBOSS_POPPER:
		hp = 25; damage = 5;
		popperPhase       = PopperPhase::BASIC;
		popperBurrowCount = 0;
		popperPhaseTimer  = 0.0f;
		popperPopIndex    = 0;
		break;
	}

	// All enemies start patrolling.
	// PatrolState will transition to the correct typed AttackState via MakeAttackState().
	fsm->ChangeState(std::make_unique<PatrolState>());
}

// ===========================================================================
// update  - called every frame by the game loop / ECS system
// ===========================================================================
void EnemyAi::update(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* force) {
	fsm->Update(deltaTime, playerPos, e, force);
}

// ===========================================================================
// updateAttack  - basic / boss melee attack (called by BasicAttackState,
//                 BossAttackState). Stops movement, faces player, fires on
//                 cooldown.
// ===========================================================================
void EnemyAi::updateAttack(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy)
{
	// Stop moving while attacking
	if (forceproxy) {
		SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
		if (spd) spd->speed = { 0.0f, 0.0f };
	}

	// Face the player
	Vector2 enemyCenter{ position.x + tileSize * 0.5f, position.y + tileSize * 0.5f };
	float dx = playerPos.x - enemyCenter.x;
	float dy = playerPos.y - enemyCenter.y;
	if (std::fabs(dx) > std::fabs(dy))
		currentDirection = (dx > 0) ? Direction::RIGHT : Direction::LEFT;
	else
		currentDirection = (dy > 0) ? Direction::UP : Direction::DOWN;

	// Count down attack cooldown
	if (AttackTimer > 0.0f) {
		AttackTimer -= deltaTime;
		return;
	}

	// Fire attack and reset cooldown
	SendMessage("EnemyAttack");
	AttackTimer = actionDuration;
}

// ===========================================================================
// updateBasic  - dash attack for the basic enemy
//
// Three internal phases:
//   READY   - locks direction toward player and begins the dash immediately
//   DASHING - moves at dashSpeed, stops before walls, then rests
//   RESTING - frozen for dashRestDuration seconds, then resets to READY
// ===========================================================================
void EnemyAi::updateBasic(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy)
{
	switch (dashPhase)
	{
		// -----------------------------------------------------------------
	case DashPhase::READY:
	{
		// Lock direction toward player right now and begin dashing
		Vector2 enemyCenter{ position.x + tileSize * 0.5f, position.y + tileSize * 0.5f };
		Vector2 playerCenter{ playerPos.x + tileSize * 0.5f, playerPos.y + tileSize * 0.5f };

		float dx = playerCenter.x - enemyCenter.x;
		float dy = playerCenter.y - enemyCenter.y;
		float dist = std::sqrt(dx * dx + dy * dy);

		if (dist < 0.001f)
			break; // player is exactly on top — skip this frame

		// Normalize and store locked direction
		dashDir.x = dx / dist;
		dashDir.y = dy / dist;

		// Face the player
		if (std::fabs(dx) > std::fabs(dy))
			currentDirection = (dx > 0) ? Direction::RIGHT : Direction::LEFT;
		else
			currentDirection = (dy > 0) ? Direction::UP : Direction::DOWN;

		// Total world-units to travel
		dashRemaining = static_cast<float>(dashTiles) * tileSize;
		dashPhase = DashPhase::DASHING;

		DebugConsole::Get().Info("[BASIC] Dash started.\n");
		break;
	}

	// -----------------------------------------------------------------
	case DashPhase::DASHING:
	{
		if (dashRemaining <= 0.0f)
		{
			// Full distance travelled — start resting
			dashPhase = DashPhase::RESTING;
			dashRestTimer = 0.0f;
			if (forceproxy) {
				SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
				if (spd) spd->speed = { 0.0f, 0.0f };
			}
			DebugConsole::Get().Info("[BASIC] Dash complete, resting.\n");
			break;
		}

		// How far this frame?
		float stepDist = dashSpeed * deltaTime;
		if (stepDist > dashRemaining)
			stepDist = dashRemaining;

		Vector2 enemyCenter{ position.x + tileSize * 0.5f, position.y + tileSize * 0.5f };
		Vector2 nextCenter{
			enemyCenter.x + dashDir.x * stepDist,
			enemyCenter.y + dashDir.y * stepDist
		};

		// Check the leading edge in the dash direction for walls
		const float edgeOffset = tileSize * 0.5f + 2.0f;
		Vector2 leadingEdge{
			nextCenter.x + dashDir.x * edgeOffset,
			nextCenter.y + dashDir.y * edgeOffset
		};

		if (isBlocked(leadingEdge))
		{
			// Wall ahead — stop here and rest
			dashRemaining = 0.0f;
			dashPhase = DashPhase::RESTING;
			dashRestTimer = 0.0f;
			if (forceproxy) {
				SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
				if (spd) spd->speed = { 0.0f, 0.0f };
			}
			DebugConsole::Get().Info("[BASIC] Dash blocked by wall, resting.\n");
			break;
		}

		// Clear — apply movement
		dashRemaining -= stepDist;

		Vector2 desiredVel{ dashDir.x * dashSpeed, dashDir.y * dashSpeed };
		if (forceproxy)
			forceproxy->triggerForce(e, desiredVel);
		else {
			position.x = nextCenter.x - tileSize * 0.5f;
			position.y = nextCenter.y - tileSize * 0.5f;
		}
		break;
	}

	// -----------------------------------------------------------------
	case DashPhase::RESTING:
	{
		// Freeze the enemy completely
		if (forceproxy) {
			SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
			if (spd) spd->speed = { 0.0f, 0.0f };
		}

		dashRestTimer += deltaTime;
		if (dashRestTimer >= dashRestDuration)
		{
			dashPhase = DashPhase::READY;
			dashRestTimer = 0.0f;
			DebugConsole::Get().Info("[BASIC] Rest done, ready to dash again.\n");
		}
		break;
	}
	}
}

// ===========================================================================
// updatePatrol  - walk back and forth within patrol range
// NOTE: returning-to-spawn is now handled by ReturnToSpawnState, NOT here.
// ===========================================================================
void EnemyAi::updatePatrol(float deltaTime, Entity e, EntityForceProxy* forceproxy)
{
	Vector2 nextPos = position;

	switch (currentDirection) {
	case Direction::LEFT:  nextPos.x -= patrolSpeed * deltaTime; break;
	case Direction::RIGHT: nextPos.x += patrolSpeed * deltaTime; break;
	case Direction::UP:    nextPos.y += patrolSpeed * deltaTime; break;
	case Direction::DOWN:  nextPos.y -= patrolSpeed * deltaTime; break;
	}

	Vector2 checkPos = nextPos;
	switch (currentDirection) {
	case Direction::LEFT:  checkPos.x -= checkOffset;              break;
	case Direction::RIGHT: checkPos.x += tileSize + checkOffset;   break;
	case Direction::UP:    checkPos.y += tileSize + checkOffset;   break;
	case Direction::DOWN:  checkPos.y -= checkOffset;              break;
	}

	bool hitWall = isBlocked(checkPos);

	Vector2 gridSpawn = WorldToGrid(spawnPos, tileSize);
	Vector2 gridNow = WorldToGrid(nextPos, tileSize);

	bool outOfRange = false;
	switch (currentDirection) {
	case Direction::LEFT:  if (gridNow.x < gridSpawn.x - patrolRange) outOfRange = true; break;
	case Direction::RIGHT: if (gridNow.x > gridSpawn.x + patrolRange) outOfRange = true; break;
	case Direction::UP:    if (gridNow.y > gridSpawn.y + patrolRange) outOfRange = true; break;
	case Direction::DOWN:  if (gridNow.y < gridSpawn.y - patrolRange) outOfRange = true; break;
	}

	if (hitWall || outOfRange) {
		// Reverse direction
		switch (currentDirection) {
		case Direction::LEFT:  currentDirection = Direction::RIGHT; break;
		case Direction::RIGHT: currentDirection = Direction::LEFT;  break;
		case Direction::UP:    currentDirection = Direction::DOWN;  break;
		case Direction::DOWN:  currentDirection = Direction::UP;    break;
		}
	}
	else {
		if (forceproxy) {
			Vector2 desiredVel = (nextPos - position) / deltaTime;
			forceproxy->triggerForce(e, desiredVel);
		}
	}
}

// ===========================================================================
// updateChase  - move toward the player tile by tile
// ===========================================================================
void EnemyAi::updateChase(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy)
{
	// Remember last seen position for SearchState
	LastKnownPlayerPos = playerPos;
	HasLastSeenPlayer = true;

	// Work in centers to avoid 1-tile pops when changing target tiles
	Vector2 enemyCenter{ position.x + tileSize * 0.5f, position.y + tileSize * 0.5f };
	Vector2 playerCenter{ playerPos.x + tileSize * 0.5f,  playerPos.y + tileSize * 0.5f };

	Vector2 gridEnemy = WorldToGrid(enemyCenter, tileSize);
	Vector2 gridPlayer = WorldToGrid(playerCenter, tileSize);

	float dx = gridPlayer.x - gridEnemy.x;
	float dy = gridPlayer.y - gridEnemy.y;

	Vector2 nextGrid = gridEnemy;
	if (std::fabs(dx) > std::fabs(dy)) {
		if (dx > 0) { nextGrid.x++; currentDirection = Direction::RIGHT; }
		else if (dx < 0) { nextGrid.x--; currentDirection = Direction::LEFT; }
	}
	else {
		if (dy > 0) { nextGrid.y++; currentDirection = Direction::UP; }
		else if (dy < 0) { nextGrid.y--; currentDirection = Direction::DOWN; }
	}

	if (isBlockedGrid(nextGrid))
	{
		// Primary axis blocked — try the other axis
		Vector2 alt = gridEnemy;
		if (std::fabs(dx) > std::fabs(dy)) {
			if (dy > 0) { alt.y++; currentDirection = Direction::UP; }
			else if (dy < 0) { alt.y--; currentDirection = Direction::DOWN; }
		}
		else {
			if (dx > 0) { alt.x++; currentDirection = Direction::RIGHT; }
			else if (dx < 0) { alt.x--; currentDirection = Direction::LEFT; }
		}

		if (isBlockedGrid(alt))
			return; // both axes blocked — stop

		nextGrid = alt;
	}

	// Move toward center of the chosen tile
	Vector2 target = GridToWorld(nextGrid, tileSize);
	target.x += tileSize * 0.5f;
	target.y += tileSize * 0.5f;

	Vector2 toTarget = target - enemyCenter;
	float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);

	Vector2 desiredVel(0.0f, 0.0f);
	if (distance > 0.01f)
		desiredVel = (toTarget / distance) * chaseSpeed;

	Vector2 candidateCenter = enemyCenter + desiredVel * deltaTime;
	Vector2 candidatePos{
		candidateCenter.x - tileSize * 0.5f,
		candidateCenter.y - tileSize * 0.5f
	};

	// Commented wouldOverlapPlayer to allow the enemy to continuously walk into the player and trigger collision damage.
	// The physics system now handles dynamic pushback to prevent wall pinning. Dont del yet due to potentiall script updates
	// if (wouldOverlapPlayer(candidatePos, playerPos))
	// 	return;

	if (forceproxy)
		forceproxy->triggerForce(e, desiredVel);
}

// ===========================================================================
// updateSearch  - walk to last known position, then scan 360 degrees.
//                 Sets searchDone = true when the full scan completes.
// ===========================================================================
void EnemyAi::updateSearch(float deltaTime)
{
	Vector2 gridCurrent = WorldToGrid(position, tileSize);
	Vector2 gridTarget = WorldToGrid(LastKnownPlayerPos, tileSize);

	// --- Phase 1: walk toward last known player tile ---
	if (HasLastSeenPlayer)
	{
		if (gridCurrent.x == gridTarget.x && gridCurrent.y == gridTarget.y) {
			HasLastSeenPlayer = false;
			searchTimer = 0.0f;
			searchPhase = 0;
			return;
		}

		Vector2 nextGrid = gridCurrent;
		float dx = gridTarget.x - gridCurrent.x;
		float dy = gridTarget.y - gridCurrent.y;

		// dominant axis first
		if (std::fabs(dx) > std::fabs(dy)) {
			if (dx > 0) { nextGrid.x++; currentDirection = Direction::RIGHT; }
			else { nextGrid.x--; currentDirection = Direction::LEFT; }
		}
		else {
			if (dy > 0) { nextGrid.y++; currentDirection = Direction::UP; }
			else { nextGrid.y--; currentDirection = Direction::DOWN; }
		}

		if (!isBlockedGrid(nextGrid)) {
			Vector2 target = GridToWorld(nextGrid, tileSize);
			target.x += tileSize * 0.5f;
			target.y += tileSize * 0.5f;

			Vector2 center{ position.x + tileSize * 0.5f, position.y + tileSize * 0.5f };
			stepToward(center, target, searchSpeed, deltaTime);
			position.x = center.x - tileSize * 0.5f;
			position.y = center.y - tileSize * 0.5f;
			return;
		}

		// fallback: other axis
		Vector2 alt = gridCurrent;
		if (std::fabs(dx) > std::fabs(dy)) {
			if (dy > 0) { alt.y++; currentDirection = Direction::UP; }
			else { alt.y--; currentDirection = Direction::DOWN; }
		}
		else {
			if (dx > 0) { alt.x++; currentDirection = Direction::RIGHT; }
			else { alt.x--; currentDirection = Direction::LEFT; }
		}

		if (!isBlockedGrid(alt)) {
			Vector2 target = GridToWorld(alt, tileSize);
			target.x += tileSize * 0.5f;
			target.y += tileSize * 0.5f;

			Vector2 center{ position.x + tileSize * 0.5f, position.y + tileSize * 0.5f };
			stepToward(center, target, searchSpeed, deltaTime);
			position.x = center.x - tileSize * 0.5f;
			position.y = center.y - tileSize * 0.5f;
		}
		return;
	}

	// --- Phase 2: scan pattern — turn 90 degrees three times (~1.5s total) ---
	searchTimer += deltaTime;

	if (searchTimer > (searchPhase + 1) * 0.5f)
	{
		switch (currentDirection)
		{
		case Direction::UP:
			if (searchPhase == 0) currentDirection = Direction::LEFT;
			else if (searchPhase == 1) currentDirection = Direction::UP;
			else if (searchPhase == 2) currentDirection = Direction::RIGHT;
			break;
		case Direction::DOWN:
			if (searchPhase == 0) currentDirection = Direction::RIGHT;
			else if (searchPhase == 1) currentDirection = Direction::DOWN;
			else if (searchPhase == 2) currentDirection = Direction::LEFT;
			break;
		case Direction::LEFT:
			if (searchPhase == 0) currentDirection = Direction::DOWN;
			else if (searchPhase == 1) currentDirection = Direction::LEFT;
			else if (searchPhase == 2) currentDirection = Direction::UP;
			break;
		case Direction::RIGHT:
			if (searchPhase == 0) currentDirection = Direction::UP;
			else if (searchPhase == 1) currentDirection = Direction::RIGHT;
			else if (searchPhase == 2) currentDirection = Direction::DOWN;
			break;
		}

		searchPhase++;

		if (searchPhase > 2)
		{
			// Full 360-degree scan complete — signal SearchState to transition
			searchTimer = 0.0f;
			searchPhase = 0;
			searchDone = true;
		}
	}
}

// ===========================================================================
// updateRanged  - maintain optimal distance, fire projectiles in combo pattern
//
// Combo sequence (rangedShotsFired 0-5):
//   Shots 0, 1, 2  (NORMAL phase)  — single projectile, 2 s between shots
//   Shots 3, 4, 5  (SPREAD phase)  — 3-projectile fan, 0.4 s between each burst
//
// After all 6 shots a 3 s full-combo cooldown resets rangedShotsFired to 0.
//
// Messages:
//   "RangedNormalAttack"  — fire one projectile aimed directly at player
//   "RangedSpreadAttack"  — fire 3-projectile fan (renderer fires left/centre/right)
// ===========================================================================
void EnemyAi::updateRanged(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy)
{
	const float MinRange = 2.0f * tileSize;
	const float MaxRange = 4.0f * tileSize;
	const float normalShotDelay = 2.0f;   // seconds between normal shots
	const float spreadShotDelay = 0.4f;   // seconds between spread bursts
	const float comboCooldown = 3.0f;   // rest period after full 6-shot combo

	Vector2 enemyCenter{ position.x + tileSize * 0.5f,   position.y + tileSize * 0.5f };
	Vector2 playerCenter{ playerPos.x + tileSize * 0.25f, playerPos.y + tileSize * 0.5f };

	float dx = playerCenter.x - enemyCenter.x;
	float dy = playerCenter.y - enemyCenter.y;
	float dist = std::sqrt(dx * dx + dy * dy);

	if (!withinLineOfSight(playerPos))
		return;

	// Face player
	if (std::fabs(dx) > std::fabs(dy))
		currentDirection = (dx > 0) ? Direction::RIGHT : Direction::LEFT;
	else
		currentDirection = (dy > 0) ? Direction::UP : Direction::DOWN;

	// Reposition: stay between MinRange and MaxRange
	Vector2 moveDir{ 0.0f, 0.0f };
	if (dist < MinRange && dist > 0.001f) { moveDir.x = -dx / dist; moveDir.y = -dy / dist; }
	else if (dist > MaxRange) { moveDir.x = dx / dist; moveDir.y = dy / dist; }

	if (moveDir.x != 0.0f || moveDir.y != 0.0f)
	{
		Vector2 nextCenter{
			enemyCenter.x + moveDir.x * rangeSpeed * deltaTime,
			enemyCenter.y + moveDir.y * rangeSpeed * deltaTime
		};

		if (!isBlocked(nextCenter))
		{
			if (forceproxy)
				forceproxy->triggerForce(e, { moveDir.x * rangeSpeed, moveDir.y * rangeSpeed });
			else {
				position.x = nextCenter.x - tileSize * 0.5f;
				position.y = nextCenter.y - tileSize * 0.5f;
			}
		}
	}

	// Only fire when in sweet spot
	if (dist < MinRange || dist > MaxRange)
		return;

	AttackTimer -= deltaTime;
	if (AttackTimer > 0.0f)
		return;

	// Determine which shot in the combo we're on
	rangedComboPhase = (rangedShotsFired < 3) ? RangedComboPhase::NORMAL
		: RangedComboPhase::SPREAD;

	if (rangedComboPhase == RangedComboPhase::NORMAL)
	{
		SendMessage("RangedNormalAttack");
		DebugConsole::Get().Info("[RANGED] Normal shot " + std::to_string(rangedShotsFired + 1) + "/3\n");
		AttackTimer = normalShotDelay;
	}
	else
	{
		SendMessage("RangedSpreadAttack");
		DebugConsole::Get().Info("[RANGED] Spread burst " + std::to_string(rangedShotsFired - 2) + "/3\n");
		AttackTimer = spreadShotDelay;
	}

	rangedShotsFired++;

	if (rangedShotsFired >= 6)
	{
		// Full combo done — long cooldown before next cycle
		rangedShotsFired = 0;
		AttackTimer = comboCooldown;
		DebugConsole::Get().Info("[RANGED] Combo complete. Cooldown " + std::to_string(comboCooldown) + "s\n");
	}
}

// ===========================================================================
// updateBurrow  - whack-a-mole burrow attack
//
// Internal phase machine (BurrowPhase enum exposed in ai.h):
//
//   ABOVE_GROUND  Walk toward the player.  On entering burrow range with the
//                 cooldown expired, transition to TELEGRAPHING.
//
//   TELEGRAPHING  (~0.8 s)  Enemy freezes in place.  The strike target is
//                 locked to the player's CURRENT position this very frame so
//                 the player can see where the enemy will emerge and has a
//                 genuine dodge window.  "BurrowWarning" is broadcast so the
//                 renderer / audio layer can show a ground-crack VFX and play
//                 a rumble SFX at burrowStrikeTarget.
//
//   BURROWED      (~0.6 s)  Sprite hidden ("BurrowUnderground").  Enemy is
//                 conceptually travelling underground; position is unchanged
//                 until the phase ends.
//
//   STRIKING      (~1.2 s)  Enemy teleports to burrowStrikeTarget and surfaces.
//                 If the player is still within the hit radius (0.9 * tileSize)
//                 "BurrowStrike" is broadcast (damage); otherwise "BurrowMissed".
//                 "BurrowVulnerable" is always broadcast on emerge — your combat
//                 system should open a damage window here. "BurrowInvulnerable"
//                 is broadcast when the 1.2 s stun expires, closing the window.
//
//   COOLDOWN      (~2.5 s)  Enemy is slow / visible.  Explicit player-punish
//                 window.  After expiry the cycle repeats from ABOVE_GROUND.
//
// BurrowAttackState in states.cpp guards sight/range transitions so they only
// fire while the phase is ABOVE_GROUND (isBurrowed == false).
// ===========================================================================
void EnemyAi::updateBurrow(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy)
{
	// --------------- timing constants ----------------------------------------
	const float telegraphDuration = 0.8f;  // player dodge window
	const float burrowTravelTime = 0.6f;  // underground travel
	const float strikeDuration = 1.2f;  // emerge stun / punish window (long enough to actually hit)
	const float cooldownTime = 2.5f;  // visible recovery
	const float hitRadius = 0.9f * tileSize; // strike hit detection
	// -------------------------------------------------------------------------

	Vector2 enemyCenter{ position.x + tileSize * 0.5f,   position.y + tileSize * 0.5f };
	Vector2 playerCenter{ playerPos.x + tileSize * 0.25f, playerPos.y + tileSize * 0.5f };

	float dx = playerCenter.x - enemyCenter.x;
	float dy = playerCenter.y - enemyCenter.y;
	float dist = std::sqrt(dx * dx + dy * dy);

	switch (burrowPhase)
	{
		// -----------------------------------------------------------------------
	case BurrowPhase::ABOVE_GROUND:
	{
		// Tick down cooldown even while approaching
		if (burrowPhaseTimer > 0.0f)
			burrowPhaseTimer -= deltaTime;

		if (dist > burrowRange)
		{
			// Walk toward player
			Vector2 dir = { dx / dist, dy / dist };
			Vector2 desiredVel = { dir.x * burrowSpeed, dir.y * burrowSpeed };
			Vector2 nextCenter = {
				enemyCenter.x + desiredVel.x * deltaTime,
				enemyCenter.y + desiredVel.y * deltaTime
			};

			if (!isBlocked(nextCenter))
			{
				if (forceproxy)
					forceproxy->triggerForce(e, desiredVel);
				else {
					position.x = nextCenter.x - tileSize * 0.5f;
					position.y = nextCenter.y - tileSize * 0.5f;
				}
			}

			if (std::fabs(dx) > std::fabs(dy))
				currentDirection = (dx > 0) ? Direction::RIGHT : Direction::LEFT;
			else
				currentDirection = (dy > 0) ? Direction::UP : Direction::DOWN;
		}
		else if (burrowPhaseTimer <= 0.0f)
		{
			// Player is in range and cooldown has expired — start telegraphing.
			// Lock the strike target NOW so the player can see the destination.
			burrowStrikeTarget = playerCenter;
			burrowPhaseTimer = telegraphDuration;
			burrowPhase = BurrowPhase::TELEGRAPHING;
			isBurrowed = false; // still visible during telegraph

			// Freeze in place
			if (forceproxy) {
				SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
				if (spd) spd->speed = { 0.0f, 0.0f };
			}

			SendMessage("BurrowWarning");
			DebugConsole::Get().Info("[BURROW] Telegraphing — target locked, warning sent.\n");
		}
		break;
	}
	// -----------------------------------------------------------------------
	case BurrowPhase::TELEGRAPHING:
	{
		// Stand still; give the player time to dodge
		if (forceproxy) {
			SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
			if (spd) spd->speed = { 0.0f, 0.0f };
		}

		burrowPhaseTimer -= deltaTime;
		if (burrowPhaseTimer <= 0.0f)
		{
			// Go underground
			burrowPhaseTimer = burrowTravelTime;
			burrowPhase = BurrowPhase::BURROWED;
			isBurrowed = true;

			SendMessage("BurrowUnderground");
			DebugConsole::Get().Info("[BURROW] Went underground.\n");
		}
		break;
	}

	// -----------------------------------------------------------------------
	case BurrowPhase::BURROWED:
	{
		// Hidden underground — wait out the travel time
		burrowPhaseTimer -= deltaTime;
		if (burrowPhaseTimer <= 0.0f)
		{
			// Emerge at the locked strike target
			position.x = burrowStrikeTarget.x - tileSize * 0.5f;
			position.y = burrowStrikeTarget.y - tileSize * 0.5f;
			isBurrowed = false;

			if (forceproxy) {
				SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
				if (spd) spd->speed = { 0.0f, 0.0f };
			}

			// Re-compute distance to player at emerge point
			Vector2 newCenter{ position.x + tileSize * 0.5f, position.y + tileSize * 0.5f };
			float edx = playerCenter.x - newCenter.x;
			float edy = playerCenter.y - newCenter.y;
			float eDist = std::sqrt(edx * edx + edy * edy);

			if (eDist <= hitRadius) {
				SendMessage("BurrowStrike");
				DebugConsole::Get().Info("[BURROW] Emerged — HIT player!\n");
			}
			else {
				SendMessage("BurrowMissed");
				DebugConsole::Get().Info("[BURROW] Emerged — MISSED (player dodged).\n");
			}

			// Signal that the enemy is now stunned and can be damaged.
			// Your combat/health system should listen for "BurrowVulnerable"
			// to open a damage window, and "BurrowInvulnerable" to close it.
			SendMessage("BurrowVulnerable");
			DebugConsole::Get().Info("[BURROW] Enemy stunned — VULNERABLE to player attacks!\n");

			burrowPhaseTimer = strikeDuration;
			burrowPhase = BurrowPhase::STRIKING;
		}
		break;
	}

	// -----------------------------------------------------------------------
	case BurrowPhase::STRIKING:
	{
		// Stun after emerging — the player's kill/punish window.
		// Enemy is completely frozen. "BurrowVulnerable" was sent on entry;
		// "BurrowInvulnerable" is sent when the window closes.
		if (forceproxy) {
			SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
			if (spd) spd->speed = { 0.0f, 0.0f };
		}

		burrowPhaseTimer -= deltaTime;
		if (burrowPhaseTimer <= 0.0f)
		{
			SendMessage("BurrowInvulnerable");
			DebugConsole::Get().Info("[BURROW] Vulnerable window closed — entering cooldown.\n");

			burrowPhaseTimer = cooldownTime;
			burrowPhase = BurrowPhase::COOLDOWN;
		}
		break;
	}

	// -----------------------------------------------------------------------
	case BurrowPhase::COOLDOWN:
	{
		// Visible and slow — explicit counterattack window for the player
		burrowPhaseTimer -= deltaTime;
		if (burrowPhaseTimer <= 0.0f)
		{
			burrowPhase = BurrowPhase::ABOVE_GROUND;
			burrowPhaseTimer = 0.0f;
			DebugConsole::Get().Info("[BURROW] Cooldown over — ready to burrow again.\n");
		}

		// Slow idle movement: shuffle away from player slightly
		if (dist < burrowRange * 0.5f && dist > 0.001f)
		{
			// Back away slowly
			Vector2 away = { -(dx / dist) * (burrowSpeed * 0.3f),
								  -(dy / dist) * (burrowSpeed * 0.3f) };
			Vector2 nextCtr = { enemyCenter.x + away.x * deltaTime,
								  enemyCenter.y + away.y * deltaTime };
			if (!isBlocked(nextCtr))
			{
				if (forceproxy)
					forceproxy->triggerForce(e, away);
				else {
					position.x = nextCtr.x - tileSize * 0.5f;
					position.y = nextCtr.y - tileSize * 0.5f;
				}
			}
		}
		break;
	}
	} // end switch
}

// ===========================================================================
// updateHeal  - heal self when HP is low, attack player when healthy
// NOTE: sight-loss and range checks are handled by HealAttackState.
//       This function only handles the heal/attack action itself.
// ===========================================================================
void EnemyAi::updateHeal(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy)
{
	const int   maxHP = 13;
	const float healThreshold = 0.5f;
	const int   healAmount = 3;

	if (cooldownTimer > 0.0f)
		cooldownTimer -= deltaTime;

	Vector2 enemyCenter{ position.x + tileSize * 0.5f,   position.y + tileSize * 0.5f };
	Vector2 playerCenter{ playerPos.x + tileSize * 0.25f, playerPos.y + tileSize * 0.5f };

	float dx = playerCenter.x - enemyCenter.x;
	float dy = playerCenter.y - enemyCenter.y;
	float dist = std::sqrt(dx * dx + dy * dy);

	// Currently healing - stand still until heal completes
	if (isHealing)
	{
		actionTimer += deltaTime;
		if (actionTimer >= actionDuration)
		{
			hp += healAmount;
			if (hp > maxHP) hp = maxHP;

			isHealing = false;
			actionTimer = 0.0f;
			cooldownTimer = cooldownDuration;
		}

		if (forceproxy) {
			SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
			if (spd) spd->speed = { 0.0f, 0.0f };
		}
		return;
	}

	// Currently attacking - stand still until attack completes
	if (actionTimer > 0.0f)
	{
		actionTimer += deltaTime;
		if (actionTimer >= actionDuration)
		{
			actionTimer = 0.0f;
			cooldownTimer = cooldownDuration;
		}

		if (forceproxy) {
			SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
			if (spd) spd->speed = { 0.0f, 0.0f };
		}
		return;
	}

	// Ready for next action (cooldown done)
	if (cooldownTimer <= 0.0f)
	{
		if (hp < maxHP * healThreshold)
		{
			// HP is low — heal
			isHealing = true;
			actionTimer = deltaTime;

			if (forceproxy) {
				SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
				if (spd) spd->speed = { 0.0f, 0.0f };
			}
		}
		else if (dist <= attackRange)
		{
			// In range and healthy - attack
			actionTimer = deltaTime;
		}
		else
		{
			// Healthy but too far - move closer
			Vector2 dir = { dx / dist, dy / dist };
			Vector2 desiredVel = { dir.x * healMovSpeed, dir.y * healMovSpeed };

			Vector2 nextCenter = {
				enemyCenter.x + desiredVel.x * deltaTime,
				enemyCenter.y + desiredVel.y * deltaTime
			};

			if (!isBlocked(nextCenter))
			{
				if (forceproxy)
					forceproxy->triggerForce(e, desiredVel);
				else {
					position.x = nextCenter.x - tileSize * 0.5f;
					position.y = nextCenter.y - tileSize * 0.5f;
				}
			}

			if (std::fabs(dx) > std::fabs(dy))
				currentDirection = (dx > 0) ? Direction::RIGHT : Direction::LEFT;
			else
				currentDirection = (dy > 0) ? Direction::UP : Direction::DOWN;
		}
	}
	else
	{
		// On cooldown - drift slowly toward player
		Vector2 dir = { dx / dist, dy / dist };
		Vector2 desiredVel = { dir.x * healMovSpeed * 0.5f, dir.y * healMovSpeed * 0.5f };

		Vector2 nextCenter = {
			enemyCenter.x + desiredVel.x * deltaTime,
			enemyCenter.y + desiredVel.y * deltaTime
		};

		if (!isBlocked(nextCenter))
		{
			if (forceproxy)
				forceproxy->triggerForce(e, desiredVel);
			else {
				position.x = nextCenter.x - tileSize * 0.5f;
				position.y = nextCenter.y - tileSize * 0.5f;
			}
		}

		if (std::fabs(dx) > std::fabs(dy))
			currentDirection = (dx > 0) ? Direction::RIGHT : Direction::LEFT;
		else
			currentDirection = (dy > 0) ? Direction::UP : Direction::DOWN;
	}
}

// ===========================================================================
// updateBless  - bless mob behaviour
//
// MODE 1 — HEAL_ALLY  (rangedAllyAlive == true)
//   The mob continuously follows the nearest ranged ally whose position and
//   HP percent are supplied each frame via RegisterBlessAlly(pos, hpPercent).
//   It heals ONLY when the ally's HP is below BLESS_ALLY_HEAL_THRESHOLD (50%).
//   When healthy, the mob just stays close without casting.
//   This means the mob is always moving with the ally — it does not stand
//   still waiting for health to drop.
//
//   Sub-states:
//     FOLLOWING  — tracking ally, not casting
//     CASTING    — channelling heal (ally in range + below threshold)
//
//   Lua: s_mode == "HEAL_ALLY"
//   Lua vars: s_isCasting, s_actionTimer, s_cooldownTimer, s_allyPos, s_allyHp
//
// MODE 2 — AOE_ATTACK  (rangedAllyAlive == false)
//   Mob freezes completely.  Cycles: charge (1.5s) -> fire AOE -> cooldown.
//   AOE is centred on THIS mob — NOT on the player, does NOT track or home.
//   Player must be within BLESS_AOE_RADIUS_FACTOR (2.5 tiles) to be hit.
//   Player can absorb and mutate the AOE circle.
//
//   Lua: s_mode == "AOE_ATTACK"
//
// Constants (Lua: top-of-file constants):
//   BLESS_HEAL_RANGE_TILES    = 3.0   (tiles, Lua: multiply by ts)
//   BLESS_AOE_RADIUS_FACTOR   = 2.5   (tiles)
//   BLESS_ALLY_HEAL_THRESHOLD = 50    (HP percent; heal only below this)
//   HEAL_CAST_TIME            = 1.2s
//   HEAL_COOLDOWN             = 3.0s
//   ATTACK_CAST_TIME          = 1.5s
//   ATTACK_COOLDOWN           = 4.0s
//   BLESS_MOVE_SPEED          = 55.0
//
// Messages broadcast:
//   "BlessHealAlly"   — heal cast on ally (game system restores ally HP)
//   "BlessAOECharge"  — mob begins charging AOE (wind-up VFX)
//   "BlessAOEFire"    — AOE circle fires at mob's own position (not targeted)
//   "BlessAOEAbsorb"  — broadcast alongside fire; GameApp sets playerHasBless if absorbed
//   "BlessAOEHit"     — additionally sent if player is within AOE radius (and did not absorb)
// ===========================================================================
void EnemyAi::updateBless(float deltaTime, Vector2 playerPos,
	Entity e, EntityForceProxy* forceproxy)
{
	// ---- timing / tuning constants (Lua: top-of-file constants) ------------
	const float blessHealRange        = 3.0f * tileSize;  // Lua: BLESS_HEAL_RANGE_TILES  = 3.0
	const float blessAOERadius        = 2.5f * tileSize;  // Lua: BLESS_AOE_RADIUS_FACTOR = 2.5
	const int   BLESS_ALLY_HEAL_THRESHOLD = 50;           // Lua: BLESS_ALLY_HEAL_THRESHOLD
	const float healCastTime          = 1.2f;             // Lua: HEAL_CAST_TIME
	const float healCooldown          = 3.0f;             // Lua: HEAL_COOLDOWN
	const float attackCastTime        = 1.5f;             // Lua: ATTACK_CAST_TIME
	const float attackCooldown        = 4.0f;             // Lua: ATTACK_COOLDOWN
	const float blessMoveSpeed        = 55.0f;            // Lua: BLESS_MOVE_SPEED
	// -------------------------------------------------------------------------

	Vector2 enemyCenter  { position.x + tileSize * 0.5f,   position.y + tileSize * 0.5f };
	Vector2 playerCenter { playerPos.x + tileSize * 0.25f, playerPos.y + tileSize * 0.5f };

	// Distance to player — used only for AOE hit check in Mode 2
	// Lua: local pdist = ai_dist(ecx,ecy,pcx,pcy)
	float pdx   = playerCenter.x - enemyCenter.x;
	float pdy   = playerCenter.y - enemyCenter.y;
	float pdist = std::sqrt(pdx * pdx + pdy * pdy);

	// Tick cooldown every frame regardless of mode
	// Lua: if s_cooldownTimer > 0 then s_cooldownTimer = s_cooldownTimer - dt end
	if (blessCooldownTimer > 0.0f)
		blessCooldownTimer -= deltaTime;

	// =======================================================================
	// MODE 1 — HEAL_ALLY: follow the ranged ally, heal only when HP is low
	// Lua: if rangedAllyAlive then
	// =======================================================================
	if (rangedAllyAlive)
	{
		// blessAllyPos and blessAllyHp are refreshed each frame by the game
		// system calling RegisterBlessAlly(pos, hpPercent).
		// Lua: s_allyPos set by AI_BlessAllyPos(x, y, hp)
		Vector2 allyCenter { blessAllyPos.x + tileSize * 0.5f,
		                     blessAllyPos.y + tileSize * 0.5f };
		float dax  = allyCenter.x - enemyCenter.x;
		float day  = allyCenter.y - enemyCenter.y;
		float dist = std::sqrt(dax * dax + day * day);

		// ---------------------------------------------------------------
		// CASTING: channelling the heal — stand still until cast completes
		// Lua: if s_isCasting then
		// ---------------------------------------------------------------
		if (isCastingBless)
		{
			if (forceproxy) {
				SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
				if (spd) spd->speed = { 0.0f, 0.0f };
			}
			blessActionTimer += deltaTime;
			if (blessActionTimer >= healCastTime)
			{
				SendMessage("BlessHealAlly");
				DebugConsole::Get().Info(
					"[BLESS] Healed ranged ally (ally HP was " +
					std::to_string(blessAllyHp) + "%).\n");
				isCastingBless     = false;
				blessActionTimer   = 0.0f;
				blessCooldownTimer = healCooldown;
			}
			return;
		}

		// ---------------------------------------------------------------
		// FOLLOWING: always move toward ally regardless of HP.
		// Only stop to cast when close enough AND ally HP is low.
		// Lua: if dist <= RANGE and hp < THRESHOLD and cooldown <= 0 then
		// ---------------------------------------------------------------
		if (dist <= blessHealRange
		    && blessAllyHp < BLESS_ALLY_HEAL_THRESHOLD
		    && blessCooldownTimer <= 0.0f)
		{
			// Ally is close and hurting — begin heal cast
			isCastingBless   = true;
			blessActionTimer = 0.0f;
			if (forceproxy) {
				SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
				if (spd) spd->speed = { 0.0f, 0.0f };
			}
			DebugConsole::Get().Info("[BLESS] Ally HP low — starting heal cast.\n");
		}
		else
		{
			// Move toward ally (regardless of whether ally is healthy or hurt)
			// Lua: safeMove toward s_allyPos at BLESS_MOVE_SPEED
			if (dist > 0.001f)
			{
				Vector2 dir        = { dax / dist, day / dist };
				Vector2 desiredVel = { dir.x * blessMoveSpeed, dir.y * blessMoveSpeed };
				Vector2 nextCenter = {
					enemyCenter.x + desiredVel.x * deltaTime,
					enemyCenter.y + desiredVel.y * deltaTime
				};
				if (!isBlocked(nextCenter))
				{
					if (forceproxy)
						forceproxy->triggerForce(e, desiredVel);
					else {
						position.x = nextCenter.x - tileSize * 0.5f;
						position.y = nextCenter.y - tileSize * 0.5f;
					}
				}
				if (std::fabs(dax) > std::fabs(day))
					currentDirection = (dax > 0) ? Direction::RIGHT : Direction::LEFT;
				else
					currentDirection = (day > 0) ? Direction::UP : Direction::DOWN;
			}
		}
		return;
	}

	// =======================================================================
	// MODE 2 — AOE_ATTACK: freeze, charge, fire non-targeted AOE
	// Mob does NOT move or chase. AOE is centred on THIS mob's position.
	// Lua: if not rangedAllyAlive then
	// =======================================================================

	// Freeze completely — no movement at all in attack mode
	// Lua: SetPos(entity, ex, ey)  (no velocity)
	if (forceproxy) {
		SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
		if (spd) spd->speed = { 0.0f, 0.0f };
	}

	// Face player for animation direction only (no movement)
	// Lua: if math.abs(pdx) > math.abs(pdy) then face horizontal else face vertical
	if (std::fabs(pdx) > std::fabs(pdy))
		currentDirection = (pdx > 0) ? Direction::RIGHT : Direction::LEFT;
	else
		currentDirection = (pdy > 0) ? Direction::UP : Direction::DOWN;

	// -----------------------------------------------------------------------
	// CASTING: channelling the AOE — count up timer, fire on expiry
	// Lua: if s_isCasting then
	// -----------------------------------------------------------------------
	if (isCastingBless)
	{
		blessActionTimer += deltaTime;
		if (blessActionTimer >= attackCastTime)
		{
			// Fire AOE centred on THIS entity — NOT at the player.
			// The renderer shows an expanding ring at enemyCenter.
			// The player can absorb this circle and mutate it.
			// Lua: SendMessage("BlessAOEFire")
			SendMessage("BlessAOEFire");
			DebugConsole::Get().Info(
				"[BLESS] AOE bless fired (non-targeted) at self. Radius = " +
				std::to_string(blessAOERadius) + " units.\n");

			// Absorb signal — GameApp sets playerHasBless if player absorbs.
			// Lua: SendMessage("BlessAOEAbsorb")
			SendMessage("BlessAOEAbsorb");

			// Hit check: is player inside the AOE radius at the moment of firing?
			// Lua: if ai_dist(ecx,ecy,pcx,pcy) <= BLESS_AOE_RADIUS*ts then
			if (pdist <= blessAOERadius)
			{
				SendMessage("BlessAOEHit");
				DebugConsole::Get().Info("[BLESS] AOE bless HIT player.\n");
			}
			else
			{
				DebugConsole::Get().Info(
					"[BLESS] AOE bless missed — player not within radius.\n");
			}

			isCastingBless     = false;
			blessActionTimer   = 0.0f;
			blessCooldownTimer = attackCooldown;
		}
		return;
	}

	// Cooldown elapsed — start the next charge cycle
	// Lua: if s_cooldownTimer <= 0 and not s_isCasting then
	if (blessCooldownTimer <= 0.0f)
	{
		isCastingBless   = true;
		blessActionTimer = 0.0f;
		SendMessage("BlessAOECharge");
		DebugConsole::Get().Info("[BLESS] AOE bless charging.\n");
	}
	// While on cooldown the mob stands frozen (movement zeroed above)
}

// ===========================================================================
// UpdatePlayerBless  - handles player use of the absorbed bless ability
//
// Called every frame from the player update loop with current key states.
// Does nothing if playerHasBless == false.
//
// Two modes (toggle with KEY_BLESS_SWITCH = Q):
//
//   HEAL mode   — activating heals the player (self-heal AOE centred on player)
//                 Message: "PlayerBlessHeal"
//
//   ATTACK mode — activating fires an AOE circle centred on the player
//                 (same radius as the mob AOE: PLAYER_BLESS_AOE_RADIUS = 2.5 tiles)
//                 Message: "PlayerBlessAOE"
//
// Both modes share the same hold-to-charge mechanic:
//   Hold KEY_BLESS_USE (E) for PLAYER_BLESS_CAST_TIME (0.8s) to fire.
//   Releasing before cast completes cancels the charge.
//   After firing, PLAYER_BLESS_COOLDOWN (3.0s) before next use.
//
// Lua translation notes:
//   s_hasBless        -> playerHasBless
//   s_blessMode       -> playerBlessMode  ("HEAL" / "ATTACK")
//   s_isCharging      -> playerBlessIsCharging
//   s_chargeTimer     -> playerBlessChargeTimer
//   s_blessCooldown   -> playerBlessCooldownTimer
//   KEY_BLESS_SWITCH  = "Q"  (check: GetKey("Q") just pressed)
//   KEY_BLESS_USE     = "E"  (check: GetKey("E") held / released)
//   PLAYER_BLESS_CAST_TIME    = 0.8s
//   PLAYER_BLESS_COOLDOWN     = 3.0s
//   PLAYER_BLESS_HEAL_AMOUNT  = 20  (HP restored per heal cast)
//   PLAYER_BLESS_AOE_RADIUS   = 2.5 (tiles)
// ===========================================================================
void EnemyAi::UpdatePlayerBless(float deltaTime, bool switchKeyPressed,
	bool useKeyHeld, bool useKeyReleased, [[maybe_unused]] Vector2 playerPos)
{
	// ---- tuning constants (Lua: top-of-file constants) ----------------------
	const float PLAYER_BLESS_CAST_TIME  = 0.8f;   // Lua: PLAYER_BLESS_CAST_TIME
	const float PLAYER_BLESS_COOLDOWN   = 3.0f;   // Lua: PLAYER_BLESS_COOLDOWN
	// -------------------------------------------------------------------------
	// KEY_BLESS_SWITCH = Q  (mapped by caller — see header Lua note)
	// KEY_BLESS_USE    = E  (mapped by caller)

	if (!playerHasBless)
		return;

	// Tick cooldown
	// Lua: if s_blessCooldown > 0 then s_blessCooldown = s_blessCooldown - dt end
	if (playerBlessCooldownTimer > 0.0f)
	{
		playerBlessCooldownTimer -= deltaTime;
		playerBlessIsCharging = false; // can't charge while on cooldown
		return;
	}

	// -----------------------------------------------------------------------
	// MODE SWITCH — KEY_BLESS_SWITCH (Q) just pressed
	// Lua: if GetKey("Q") == JUST_PRESSED then
	// -----------------------------------------------------------------------
	if (switchKeyPressed)
	{
		// Toggle mode; cancel any in-progress charge
		playerBlessMode = (playerBlessMode == PlayerBlessMode::HEAL)
		                ? PlayerBlessMode::ATTACK
		                : PlayerBlessMode::HEAL;
		playerBlessIsCharging  = false;
		playerBlessChargeTimer = 0.0f;

		SendMessage("PlayerBlessSwitchMode");
		DebugConsole::Get().Info(
			std::string("[PLAYER BLESS] Mode switched to: ") +
			(playerBlessMode == PlayerBlessMode::HEAL ? "HEAL\n" : "ATTACK\n"));
	}

	// -----------------------------------------------------------------------
	// USE KEY HELD — KEY_BLESS_USE (E) — charge up the ability
	// Lua: if GetKey("E") == HELD then
	// -----------------------------------------------------------------------
	if (useKeyHeld)
	{
		if (!playerBlessIsCharging)
		{
			// First frame of hold — begin charge
			playerBlessIsCharging  = true;
			playerBlessChargeTimer = 0.0f;
			SendMessage("PlayerBlessCharging");
			DebugConsole::Get().Info("[PLAYER BLESS] Charging...\n");
		}

		playerBlessChargeTimer += deltaTime;

		if (playerBlessChargeTimer >= PLAYER_BLESS_CAST_TIME)
		{
			// Charge complete — fire based on current mode
			// Lua: if s_blessMode == "HEAL" then ... else ... end
			if (playerBlessMode == PlayerBlessMode::HEAL)
			{
				// Self-heal AOE centred on player.
				// Game system listens for "PlayerBlessHeal" and restores HP.
				// Lua: SendMessage("PlayerBlessHeal")
				SendMessage("PlayerBlessHeal");
				DebugConsole::Get().Info("[PLAYER BLESS] HEAL fired.\n");
			}
			else
			{
				// Attack AOE centred on player — same radius as mob AOE.
				// Damages enemies within PLAYER_BLESS_AOE_RADIUS tiles.
				// Lua: SendMessage("PlayerBlessAOE")
				SendMessage("PlayerBlessAOE");
				DebugConsole::Get().Info("[PLAYER BLESS] AOE ATTACK fired.\n");
			}

			// Reset charge and start cooldown
			playerBlessIsCharging  = false;
			playerBlessChargeTimer = 0.0f;
			playerBlessCooldownTimer = PLAYER_BLESS_COOLDOWN;
			SendMessage("PlayerBlessCooldown");
		}
		return;
	}

	// -----------------------------------------------------------------------
	// USE KEY RELEASED — cancel charge if player let go before cast completed
	// Lua: if GetKey("E") == JUST_RELEASED then
	// -----------------------------------------------------------------------
	if (useKeyReleased && playerBlessIsCharging)
	{
		playerBlessIsCharging  = false;
		playerBlessChargeTimer = 0.0f;
		DebugConsole::Get().Info("[PLAYER BLESS] Charge cancelled.\n");
	}
}



// ===========================================================================
// updateBoss  - final boss fixed-rotation ability sequence
//
// Rotation (loops forever):
//   MELEE       -> OMNI_BURST -> SANCTIFY -> SHOCKWAVE -> MELEE -> ...
//
// MELEE
//   Only triggers when the player is within meleeTriggerRange (2.5 tiles).
//   Boss closes on the player.  On entering range it freezes and strikes.
//   If the player leaves range before the hit lands, the sequence advances
//   immediately to OMNI_BURST so the boss is never stuck chasing.
//   Message: "BossMeleeAttack"
//   Lua: s_bossPhase == "MELEE"
//
// OMNI_BURST  (Channeler signature, reuses ChannelerComboPhase enum)
//   Boss freezes.  Fires 3 aimed normal shots at 2.0s intervals, then
//   charges for 0.6s ("BossOmniCharge") and releases all 8 directional
//   slow projectiles.  After the burst, broadcasts "BossSpawnRangedAlly"
//   so GameApp can spawn a RANGED mob near the boss.  1.5s end-pause then
//   advances to SANCTIFY.
//   Messages: "RangedNormalAttack", "BossOmniCharge",
//             "BossOmniShot_N/NE/E/SE/S/SW/W/NW", "BossSpawnRangedAlly"
//   Lua: s_bossPhase == "OMNI_BURST", s_omniComboPhase, s_omniShotsFired,
//        s_omniIndex
//
// SANCTIFY  (Warden signature)
//   Boss freezes, becomes invulnerable ("BossInvulnStart").
//   Channels for bossSanctifyDuration (1.5s), then:
//     - Fires AOE bless circle ("BossAOEBless"; "BossAOEBlessHit" if player
//       is within bossAOERadius).
//     - Heals every living ally whose HP is below BOSS_ALLY_HEAL_THRESHOLD
//       ("BossHealAlly" once per ally healed).
//   Closes invuln ("BossInvulnEnd").  0.5s end-pause then advances to
//   SHOCKWAVE.
//   Lua: s_bossPhase == "SANCTIFY", s_sanctifyTimer
//
// SHOCKWAVE  (Popper signature — triangle pop)
//   Boss freezes, computes 3 pop positions around the player at radii
//   3/2/1 tiles, telegraphs all 3 simultaneously ("BossTriangleTelegraph"),
//   then pops emerge in sequence 0.4s apart closing inward.
//   Per pop: "BossTrianglePop_N"; "BossTrianglePopHit_N" if player caught.
//   After all 3: "BossTrianglePopEnd".  4.0s cooldown then advances to MELEE.
//   Lua: s_bossPhase == "SHOCKWAVE", s_shockPhase ("EMERGE"=telegraph /
//        "WAVE"=popping / "COOL"=cooldown), s_shockTimer, s_shockIndex,
//        s_shockZones[i] (pop world positions)
//
// Ally spawning:
//   "BossSpawnRangedAlly" is sent once after each OMNI_BURST completes.
//   GameApp should spawn a MobType::RANGED entity near the boss on receipt.
//   Ally HP is tracked via RegisterBossAlly(pos, hp) which the game system
//   calls each frame for every living ally.
//
// Constants (Lua: top-of-function constants):
//   meleeTriggerRange    = 2.5 * tileSize
//   bossMoveSpeed        = 130.0
//   meleeRecovery        = 1.0s
//   omniNormalShotDelay  = 2.0s
//   omniBurstChargeTime  = 0.6s
//   omniEndPause         = 1.5s
//   bossSanctifyDuration = 1.5s
//   bossAOERadius        = 2.5 * tileSize
//   bossHealRange        = 5.0 * tileSize
//   BOSS_ALLY_HEAL_THRESHOLD = 50  (percent of max HP; heal if below this)
//   bossTriTelegraphDur  = 1.2s   (Lua: TRI_TELEGRAPH_DUR)
//   bossPopInterval      = 0.4s   (Lua: POP_INTERVAL)
//   bossPopCooldown      = 4.0s   (Lua: POPPER_COOLDOWN)
//   bossPopRadius        = 1.5 * tileSize  (Lua: POP_RADIUS_FACTOR = 1.5)
// ===========================================================================
void EnemyAi::updateBoss(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy)
{
	// ---- timing / tuning constants (Lua: top-of-file constants) ------------
	const float meleeTriggerRange    = 2.5f * tileSize;    // Lua: MELEE_TRIGGER_TILES = 2.5
	const float bossMoveSpeed        = 130.0f;              // Lua: BOSS_MOVE_SPEED
	const float meleeRecovery        = 1.0f;                // Lua: MELEE_RECOVERY
	const float omniNormalShotDelay  = 2.0f;                // Lua: OMNI_NORMAL_SHOT_DELAY
	const float omniBurstChargeTime  = 0.6f;                // Lua: OMNI_BURST_CHARGE_TIME
	const float omniEndPause         = 1.5f;                // Lua: OMNI_END_PAUSE
	const float bossSanctifyDuration = 1.5f;                // Lua: SANCTIFY_DURATION
	const float bossAOERadius        = 2.5f * tileSize;     // Lua: AOE_RADIUS_FACTOR = 2.5
	const float bossHealRange        = 5.0f * tileSize;     // Lua: HEAL_RANGE_TILES  = 5.0
	const int   BOSS_ALLY_HEAL_THRESHOLD = 50;              // Lua: ALLY_HEAL_THRESHOLD
	// Triangle pop constants (Popper signature) --------------------------------
	//const float bossTriTelegraphDur  = 1.2f;                // Lua: TRI_TELEGRAPH_DUR
	const float bossPopInterval      = 0.4f;                // Lua: POP_INTERVAL
	const float bossPopCooldown      = 4.0f;                // Lua: POPPER_COOLDOWN
	const float bossPopRadius        = 1.5f * tileSize;     // Lua: POP_RADIUS_FACTOR = 1.5
	// -------------------------------------------------------------------------

	// Triangle pop: 3 positions around the PLAYER (not room centre).
	// Radii (tiles) and angles (degrees from world +X axis) — same as Popper mini-boss.
	// Pop 0: 3.0 tiles at  90 deg — above player (furthest, fires first)
	// Pop 1: 2.0 tiles at 210 deg — lower-left
	// Pop 2: 1.0 tile  at 330 deg — lower-right (closest, fires last)
	// Safe dodge gap: upper-right quadrant between pop 0 and pop 2.
	// Lua: POP_RADIUS_TILES = {3.0, 2.0, 1.0},  POP_ANGLE_DEG = {90, 210, 330}
	static const float kPopRadiusTiles[3] = { 3.0f, 2.0f, 1.0f };
	static const float kPopAngleDeg[3]    = { 90.0f, 210.0f, 330.0f };

	// 8-direction burst table (Lua: BURST_DIRS)
	static const struct { float dx; float dy; const char* msg; } kOmni[8] = {
		{  0.0f,    1.0f,   "BossOmniShot_N"  },
		{  0.7071f, 0.7071f,"BossOmniShot_NE" },
		{  1.0f,    0.0f,   "BossOmniShot_E"  },
		{  0.7071f,-0.7071f,"BossOmniShot_SE" },
		{  0.0f,   -1.0f,   "BossOmniShot_S"  },
		{ -0.7071f,-0.7071f,"BossOmniShot_SW" },
		{ -1.0f,    0.0f,   "BossOmniShot_W"  },
		{ -0.7071f, 0.7071f,"BossOmniShot_NW" },
	};
	// -------------------------------------------------------------------------

	Vector2 enemyCenter  { position.x + tileSize * 0.5f,   position.y + tileSize * 0.5f  };
	Vector2 playerCenter { playerPos.x + tileSize * 0.25f, playerPos.y + tileSize * 0.5f };
	float dx   = playerCenter.x - enemyCenter.x;
	float dy   = playerCenter.y - enemyCenter.y;
	float dist = std::sqrt(dx * dx + dy * dy);

	// Sync legacy member so states.cpp reads/writes remain valid
	bossRangedShots = bossOmniShotsFired;

	// Snapshot ally list then clear — game system refills next frame
	std::vector<BossAllyInfo> allies = bossAllyData;
	bossAllyData.clear();
	bossAllyPositions.clear(); // keep legacy list in sync

	//bool alliesPresent = !allies.empty();

	// Always face the player
	if (std::fabs(dx) > std::fabs(dy))
		currentDirection = (dx > 0) ? Direction::RIGHT : Direction::LEFT;
	else
		currentDirection = (dy > 0) ? Direction::UP : Direction::DOWN;

	// -----------------------------------------------------------------------
	// Helper: freeze entity in place
	// Lua: SetPos(entity, ex, ey)  (no velocity)
	// -----------------------------------------------------------------------
	auto freeze = [&]()
	{
		if (forceproxy) {
			SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
			if (spd) spd->speed = { 0.0f, 0.0f };
		}
	};

	// -----------------------------------------------------------------------
	// Helper: move boss toward a world-space centre point
	// Lua: safeMove(entity, ndx*speed, ndy*speed, dt)
	// -----------------------------------------------------------------------
	auto moveToward = [&](const Vector2& target, float speed)
	{
		float tdx  = target.x - enemyCenter.x;
		float tdy  = target.y - enemyCenter.y;
		float tdist = std::sqrt(tdx * tdx + tdy * tdy);
		if (tdist < 0.001f) return;
		Vector2 dir = { tdx / tdist, tdy / tdist };
		Vector2 nextCenter {
			enemyCenter.x + dir.x * speed * deltaTime,
			enemyCenter.y + dir.y * speed * deltaTime
		};
		if (!isBlocked(nextCenter))
		{
			if (forceproxy)
				forceproxy->triggerForce(e, { dir.x * speed, dir.y * speed });
			else {
				position.x = nextCenter.x - tileSize * 0.5f;
				position.y = nextCenter.y - tileSize * 0.5f;
			}
		}
	};

	// -----------------------------------------------------------------------
	// Helper: advance to next phase in the fixed rotation
	// Rotation: MELEE -> OMNI_BURST -> SANCTIFY -> SHOCKWAVE -> MELEE -> ...
	// Lua: s_bossPhase = next; reset all sub-state timers
	// -----------------------------------------------------------------------
	auto advanceTo = [&](BossSequencePhase next)
	{
		bossSequencePhase = next;
		bossAbilityTimer  = 0.0f;
		bossAbilityDone   = false;
		AttackTimer       = 0.0f;

		// Reset sub-state for the incoming phase
		if (next == BossSequencePhase::OMNI_BURST)
		{
			bossOmniShotsFired = 0;
			bossOmniComboPhase = ChannelerComboPhase::NORMAL;
			bossOmniBurstIndex = 0;
		}
		else if (next == BossSequencePhase::SANCTIFY)
		{
			bossSanctifyTimer = 0.0f;
		}
		else if (next == BossSequencePhase::SHOCKWAVE)
		{
			// EMERGE = telegraph phase, WAVE = popping phase, COOL = cooldown
			// Lua: s_shockPhase = "EMERGE", s_shockTimer = 0, s_shockIndex = 0
			bossShockPhase = TremorShockPhase::EMERGE;
			bossShockTimer = 0.0f;
			bossShockIndex = 0;
		}

		const char* names[] = { "MELEE", "OMNI_BURST", "SANCTIFY", "SHOCKWAVE" };
		SendMessage("BossSequenceAdv");
		DebugConsole::Get().Info(
			std::string("[BOSS] -> ") + names[static_cast<int>(next)] + "\n");
	};

	// -----------------------------------------------------------------------
	// Fixed rotation helper
	// Lua: function nextPhase(current) ... end
	// -----------------------------------------------------------------------
	auto nextPhase = [](BossSequencePhase current) -> BossSequencePhase
	{
		switch (current)
		{
		case BossSequencePhase::MELEE:      return BossSequencePhase::OMNI_BURST;
		case BossSequencePhase::OMNI_BURST: return BossSequencePhase::SANCTIFY;
		case BossSequencePhase::SANCTIFY:   return BossSequencePhase::SHOCKWAVE;
		case BossSequencePhase::SHOCKWAVE:  return BossSequencePhase::MELEE;
		default:                            return BossSequencePhase::MELEE;
		}
	};

	// =======================================================================
	switch (bossSequencePhase)
	{

	// -----------------------------------------------------------------------
	// MELEE — close-range only; advances immediately if player escapes range
	// Lua: if s_bossPhase == "MELEE" then
	// -----------------------------------------------------------------------
	case BossSequencePhase::MELEE:
	{
		if (dist > meleeTriggerRange)
		{
			// Player not in melee range — close in
			moveToward(playerCenter, bossMoveSpeed);

			// If player is far (> 1.5x trigger range) skip directly to OMNI_BURST
			// so the boss doesn't endlessly chase a kiting player
			if (dist > meleeTriggerRange * 1.5f)
			{
				DebugConsole::Get().Info("[BOSS] Player out of melee range — skipping to OMNI_BURST.\n");
				advanceTo(BossSequencePhase::OMNI_BURST);
			}
			return;
		}

		if (!bossAbilityDone)
		{
			// In melee range — strike once then wait for recovery
			freeze();
			SendMessage("BossMeleeAttack");
			DebugConsole::Get().Info("[BOSS] Melee strike!\n");
			bossAbilityDone  = true;
			bossAbilityTimer = 0.0f;
		}
		else
		{
			// Recovery pause before advancing
			bossAbilityTimer += deltaTime;
			if (bossAbilityTimer >= meleeRecovery)
				advanceTo(nextPhase(BossSequencePhase::MELEE));
		}
		break;
	}

	// -----------------------------------------------------------------------
	// OMNI_BURST — Channeler signature (3 aimed shots + 8-dir burst)
	// Reuses bossOmniComboPhase (ChannelerComboPhase enum)
	// Lua: if s_bossPhase == "OMNI_BURST" then
	// -----------------------------------------------------------------------
	case BossSequencePhase::OMNI_BURST:
	{
		freeze();

		// End-of-ability pause: spawn ally then advance
		if (bossAbilityDone)
		{
			bossAbilityTimer += deltaTime;
			if (bossAbilityTimer >= omniEndPause)
			{
				// Spawn a ranged ally — GameApp listens for this message
				SendMessage("BossSpawnRangedAlly");
				DebugConsole::Get().Info("[BOSS] Ranged ally spawned.\n");
				advanceTo(nextPhase(BossSequencePhase::OMNI_BURST));
			}
			return;
		}

		// CHARGING: frozen wind-up
		// Lua: if s_omniComboPhase == "CHARGING" then
		if (bossOmniComboPhase == ChannelerComboPhase::CHARGING)
		{
			AttackTimer -= deltaTime;
			if (AttackTimer <= 0.0f)
			{
				bossOmniComboPhase = ChannelerComboPhase::FIRING;
				bossOmniBurstIndex = 0;
				DebugConsole::Get().Info("[BOSS] Omni burst firing.\n");
			}
			return;
		}

		// FIRING: one direction per frame until all 8 sent
		// Lua: if s_omniComboPhase == "FIRING" then
		if (bossOmniComboPhase == ChannelerComboPhase::FIRING)
		{
			if (bossOmniBurstIndex < 8)
			{
				SendMessage(kOmni[bossOmniBurstIndex].msg);
				DebugConsole::Get().Info(
					std::string("[BOSS] Omni burst ") +
					std::to_string(bossOmniBurstIndex + 1) + "/8\n");
				++bossOmniBurstIndex;
			}
			else
			{
				// All 8 fired — mark ability done, start end pause
				bossAbilityDone  = true;
				bossAbilityTimer = 0.0f;
				DebugConsole::Get().Info("[BOSS] Omni burst complete.\n");
			}
			return;
		}

		// NORMAL phase: fire aimed shots then transition to charge
		// Lua: if s_omniComboPhase == "NORMAL" then
		AttackTimer -= deltaTime;
		if (AttackTimer > 0.0f) return;

		if (bossOmniShotsFired < 3)
		{
			SendMessage("RangedNormalAttack");
			DebugConsole::Get().Info(
				"[BOSS] Omni normal shot " +
				std::to_string(bossOmniShotsFired + 1) + "/3\n");
			AttackTimer = omniNormalShotDelay;
			++bossOmniShotsFired;
		}
		else
		{
			// 3 shots done — begin burst charge
			bossOmniComboPhase = ChannelerComboPhase::CHARGING;
			AttackTimer        = omniBurstChargeTime;
			SendMessage("BossOmniCharge");
			DebugConsole::Get().Info("[BOSS] Omni burst charging.\n");
		}
		break;
	}

	// -----------------------------------------------------------------------
	// SANCTIFY — Warden signature (AOE bless circle + heal living allies)
	// AOE is non-targeted, centred on boss, absorbable by player.
	// Lua: if s_bossPhase == "SANCTIFY" then
	// -----------------------------------------------------------------------
	case BossSequencePhase::SANCTIFY:
	{
		freeze();

		if (!bossAbilityDone)
		{
			// Channel — invulnerable during this window
			bossSanctifyTimer += deltaTime;

			// First frame: open invuln + broadcast charge wind-up VFX
			// Lua: if s_sanctifyTimer == 0 then
			if (bossSanctifyTimer < deltaTime + 0.001f && bossSanctifyTimer > 0.0f)
			{
				SendMessage("BossSanctifyStart");
				SendMessage("BossInvulnStart");
				SendMessage("BossAOEBlessCharge"); // wind-up VFX, same as "BlessAOECharge"
				DebugConsole::Get().Info("[BOSS] Sanctify started — invulnerable, charging AOE.\n");
			}

			if (bossSanctifyTimer >= bossSanctifyDuration)
			{
				// Fire AOE bless circle centred on boss — NOT targeted.
				// Identical behaviour to "BlessAOEFire" and "WardenBlessAOEFire".
				// Player can absorb this circle.
				// Lua: SendMessage("BossAOEBless")
				SendMessage("BossAOEBless");
				DebugConsole::Get().Info(
					"[BOSS] Sanctify: AOE bless fired (non-targeted, radius = " +
					std::to_string(bossAOERadius) + ").\n");

				// Absorb signal — GameApp sets playerHasBless if player absorbs.
				// Lua: SendMessage("BossAOEBlessAbsorb")
				SendMessage("BossAOEBlessAbsorb");

				// Hit check — only if player did NOT absorb (GameApp resolves priority).
				// Lua: if ai_dist(ecx,ecy,pcx,pcy) <= AOE_RADIUS then
				if (dist <= bossAOERadius)
				{
					SendMessage("BossAOEBlessHit");
					DebugConsole::Get().Info("[BOSS] Sanctify AOE HIT player.\n");
				}

				// Heal allies below the HP threshold
				// Lua: for i=1,#s_allies do if s_allies[i].hp < THRESHOLD then heal end end
				int healed = 0;
				for (const BossAllyInfo& ally : allies)
				{
					Vector2 ac  = { ally.pos.x + tileSize * 0.5f, ally.pos.y + tileSize * 0.5f };
					float   adx = ac.x - enemyCenter.x;
					float   ady = ac.y - enemyCenter.y;
					float   ad  = std::sqrt(adx * adx + ady * ady);
					if (ad <= bossHealRange && ally.hp < BOSS_ALLY_HEAL_THRESHOLD)
					{
						SendMessage("BossHealAlly");
						++healed;
						DebugConsole::Get().Info(
							"[BOSS] Healed ally (hp was " +
							std::to_string(ally.hp) + ").\n");
					}
				}
				if (healed == 0)
					DebugConsole::Get().Info("[BOSS] Sanctify: no allies needed healing.\n");

				// Close invuln window
				SendMessage("BossInvulnEnd");
				bossAbilityDone  = true;
				bossAbilityTimer = 0.0f;
			}
		}
		else
		{
			// Short end-pause then advance
			bossAbilityTimer += deltaTime;
			if (bossAbilityTimer >= 0.5f)
				advanceTo(nextPhase(BossSequencePhase::SANCTIFY));
		}
		break;
	}

	// -----------------------------------------------------------------------
	// SHOCKWAVE — Popper signature (triangle pop closing inward on player)
	//
	// Reuses TremorShockPhase enum with remapped semantics:
	//   EMERGE = TELEGRAPH  : all 3 warning spots shown simultaneously
	//   WAVE   = POPPING    : pops fire in sequence 0.4s apart
	//   COOL   = COOLDOWN   : boss vulnerable, slow retreat
	//
	// Lua: if s_bossPhase == "SHOCKWAVE" then
	// -----------------------------------------------------------------------
	case BossSequencePhase::SHOCKWAVE:
	{
		// -------------------------------------------------------------------
		// EMERGE (= TELEGRAPH): freeze, compute 3 pop positions around player,
		// broadcast all 3 warning spots simultaneously.
		// Lua: if s_shockPhase == "EMERGE" then
		// -------------------------------------------------------------------
		if (bossShockPhase == TremorShockPhase::EMERGE)
		{
			freeze();

			// Compute triangle pop positions around the CURRENT player centre.
			// Angles and radii match the Popper mini-boss exactly.
			// Lua: for i=1,3 do s_shockZones[i] = playerCenter + polar(radius, angle) end
			for (int i = 0; i < 3; ++i)
			{
				float angleRad        = kPopAngleDeg[i] * 3.14159265f / 180.0f;
				float radius          = kPopRadiusTiles[i] * tileSize;
				bossShockZonePos[i].x = playerCenter.x + std::cos(angleRad) * radius;
				bossShockZonePos[i].y = playerCenter.y + std::sin(angleRad) * radius;
			}

			// Broadcast all 3 warning positions at once — renderer shows
			// ground-crack VFX at each zone simultaneously so the player can
			// read the full triangle during the telegraph window.
			SendMessage("BossTriangleTelegraph");
			DebugConsole::Get().Info("[BOSS] Triangle pop: telegraph started.\n");

			bossShockPhase = TremorShockPhase::WAVE; // advance to POPPING
			bossShockTimer = 0.0f;
			bossShockIndex = 0;
			return;
		}

		// -------------------------------------------------------------------
		// WAVE (= POPPING): fire pops in sequence, 0.4s apart, closing inward
		// Lua: if s_shockPhase == "WAVE" then
		// -------------------------------------------------------------------
		if (bossShockPhase == TremorShockPhase::WAVE)
		{
			freeze();
			bossShockTimer += deltaTime;

			if (bossShockIndex < 3)
			{
				// Fire pop i when timer crosses i * popInterval
				// Lua: if s_shockTimer >= s_shockIndex * POP_INTERVAL then
				if (bossShockTimer >= static_cast<float>(bossShockIndex) * bossPopInterval)
				{
					// Teleport to this pop position and surface
					position.x = bossShockZonePos[bossShockIndex].x - tileSize * 0.5f;
					position.y = bossShockZonePos[bossShockIndex].y - tileSize * 0.5f;
					isBurrowed = false;

					// Hit check at new position
					Vector2 newCenter { position.x + tileSize * 0.5f, position.y + tileSize * 0.5f };
					float   pdx  = playerCenter.x - newCenter.x;
					float   pdy  = playerCenter.y - newCenter.y;
					float   pDist = std::sqrt(pdx * pdx + pdy * pdy);

					SendMessage("BossTrianglePop_" + std::to_string(bossShockIndex + 1));

					if (pDist <= bossPopRadius)
					{
						SendMessage("BossTrianglePopHit_" + std::to_string(bossShockIndex + 1));
						DebugConsole::Get().Info(
							"[BOSS] Triangle pop " +
							std::to_string(bossShockIndex + 1) + " HIT player.\n");
					}
					else
					{
						DebugConsole::Get().Info(
							"[BOSS] Triangle pop " +
							std::to_string(bossShockIndex + 1) + " missed.\n");
					}

					++bossShockIndex;

					// Re-hide between pops (unless this was the last one)
					if (bossShockIndex < 3)
						isBurrowed = true;
				}
			}
			else
			{
				// All 3 pops fired — enter cooldown at final pop position
				isBurrowed     = false;
				bossShockPhase = TremorShockPhase::COOL;
				bossShockTimer = 0.0f;
				SendMessage("BossTrianglePopEnd");
				DebugConsole::Get().Info("[BOSS] Triangle pop complete — cooldown.\n");
			}
			return;
		}

		// -------------------------------------------------------------------
		// COOL (= COOLDOWN): boss fully vulnerable, slow retreat, then advance
		// Lua: if s_shockPhase == "COOL" then
		// -------------------------------------------------------------------
		if (bossShockPhase == TremorShockPhase::COOL)
		{
			bossShockTimer += deltaTime;

			// Slow drift away from player during cooldown
			if (dist < meleeTriggerRange && dist > 0.001f)
			{
				Vector2 away = { -(dx / dist) * bossMoveSpeed * 0.3f,
				                 -(dy / dist) * bossMoveSpeed * 0.3f };
				Vector2 nextCenter {
					enemyCenter.x + away.x * deltaTime,
					enemyCenter.y + away.y * deltaTime
				};
				if (!isBlocked(nextCenter))
				{
					if (forceproxy)
						forceproxy->triggerForce(e, away);
					else {
						position.x = nextCenter.x - tileSize * 0.5f;
						position.y = nextCenter.y - tileSize * 0.5f;
					}
				}
			}

			if (bossShockTimer >= bossPopCooldown)
			{
				// Reset sub-state and advance rotation
				bossShockPhase = TremorShockPhase::NONE;
				bossShockTimer = 0.0f;
				bossShockIndex = 0;
				isBurrowed     = false;
				advanceTo(nextPhase(BossSequencePhase::SHOCKWAVE));
			}
			return;
		}
		break;
	}

	} // end switch bossSequencePhase

	// Reverse-sync: if states.cpp wrote to bossRangedShots, reflect it back
	bossOmniShotsFired = bossRangedShots;
}


// ===========================================================================
// moveToSpawn  - walk back to spawn position tile by tile.
//                Sets atSpawn = true when the enemy arrives.
//                Called every frame by ReturnToSpawnState.
// ===========================================================================
void EnemyAi::moveToSpawn(float dt, Entity e, EntityForceProxy* forceproxy)
{
	atSpawn = false; // will be set true only when we physically arrive

	Vector2 enemyCenter{
		position.x + tileSize * 0.5f,
		position.y + tileSize * 0.5f
	};

	Vector2 spawnCenter{
		spawnPos.x + tileSize * 0.5f,
		spawnPos.y + tileSize * 0.5f
	};

	Vector2 diff = spawnCenter - enemyCenter;
	float   d2 = diff.x * diff.x + diff.y * diff.y;

	SpeedComponent* spd = forceproxy ? forceproxy->GetSpeedComponent(e) : nullptr;

	// Close enough - snap to spawn and signal arrival
	if (d2 < 4.0f)
	{
		atSpawn = true;
		position = spawnPos;
		movingToTile = false;
		currentDirection = patrolDirection;

		if (spd) spd->speed = { 0.0f, 0.0f };
		return;
	}

	// Pick next tile toward spawn if not already navigating to one
	if (!movingToTile)
	{
		Vector2 ge = WorldToGrid(enemyCenter, tileSize);
		Vector2 gs = WorldToGrid(spawnCenter, tileSize);

		float dx = gs.x - ge.x;
		float dy = gs.y - ge.y;

		Vector2 next = ge;
		if (fabs(dx) > fabs(dy))
			next.x += (dx > 0 ? 1 : -1);
		else
			next.y += (dy > 0 ? 1 : -1);

		if (isBlockedGrid(next))
		{
			// Try fallback axis
			Vector2 alt = ge;
			if (fabs(dx) > fabs(dy))
				alt.y += (dy > 0 ? 1 : -1);
			else
				alt.x += (dx > 0 ? 1 : -1);

			if (isBlockedGrid(alt))
			{
				if (spd) spd->speed = { 0.0f, 0.0f };
				return; // fully blocked - wait
			}

			next = alt;
		}

		currentTargetGrid = next;
		movingToTile = true;
	}

	// Move toward center of the current target tile
	Vector2 target = GridToWorld(currentTargetGrid, tileSize);
	target.x += tileSize * 0.5f;
	target.y += tileSize * 0.5f;

	Vector2 toTarget = target - enemyCenter;
	float   d2Tile = toTarget.x * toTarget.x + toTarget.y * toTarget.y;

	if (d2Tile < 4.0f)
	{
		// Reached tile - snap and pick next tile next frame
		movingToTile = false;
		position.x = currentTargetGrid.x * tileSize;
		position.y = currentTargetGrid.y * tileSize;
		if (spd) spd->speed = { 0.0f, 0.0f };
		return;
	}

	// Axial movement - no diagonal drift
	Vector2 step{ 0.0f, 0.0f };
	if (fabs(toTarget.x) > 1.0f)
	{
		step.x = (toTarget.x > 0 ? patrolSpeed : -patrolSpeed);
		currentDirection = (toTarget.x > 0 ? Direction::RIGHT : Direction::LEFT);
	}
	else
	{
		step.y = (toTarget.y > 0 ? patrolSpeed : -patrolSpeed);
		currentDirection = (toTarget.y > 0 ? Direction::UP : Direction::DOWN);
	}

	if (spd) spd->speed = { 0.0f, 0.0f };

	if (forceproxy)
		forceproxy->triggerForce(e, step);
	else {
		position.x += step.x * dt;
		position.y += step.y * dt;
	}
}

// ===========================================================================
// withinAttackRange  - true when the player is within this enemy's attack range
// ===========================================================================
bool EnemyAi::withinAttackRange(Vector2 playerPos) const
{
	Vector2 enemyCenter{ position.x + tileSize * 0.5f, position.y + tileSize * 0.5f };
	Vector2 playerCenter{ playerPos.x + tileSize * 0.5f, playerPos.y + tileSize * 0.5f };

	float dx = playerCenter.x - enemyCenter.x;
	float dy = playerCenter.y - enemyCenter.y;
	float distSq = dx * dx + dy * dy;

	float range = attackRange; // default — overridden per type below

	switch (mobType) {
	case MobType::BASIC:     range = 1.5f * tileSize; break;
	case MobType::RANGED:    range = 4.0f * tileSize; break;
	case MobType::BURROW:    range = burrowRange;      break;
	case MobType::HEAL:      range = attackRange;      break;
	case MobType::FINALBOSS: range = 2.0f * tileSize;  break;
	case MobType::MINIBOSS_CHANNELER: range = 5.0f * tileSize; break; // wide kite range
	case MobType::MINIBOSS_WARDEN:    range = 2.0f * tileSize; break; // melee range
	case MobType::MINIBOSS_POPPER:    range = burrowRange;      break; // same trigger as burrow
	}

	return distSq <= (range * range);
}

// ===========================================================================
// playerInFront  - true when the player is within the enemy's forward cone
// ===========================================================================
bool EnemyAi::playerInFront(Vector2 playerPos) const
{
	float fx = 0.0f, fy = 0.0f;
	switch (currentDirection) {
	case Direction::RIGHT: fx = 1.0f; break;
	case Direction::LEFT:  fx = -1.0f; break;
	case Direction::UP:    fy = 1.0f; break;
	case Direction::DOWN:  fy = -1.0f; break;
	}

	Vector2 enemyCenter = { position.x + tileSize * 0.5f, position.y + tileSize * 0.5f };
	Vector2 dir = { playerPos.x - enemyCenter.x,  playerPos.y - enemyCenter.y };
	float   magToPlayer = std::sqrt(dir.x * dir.x + dir.y * dir.y);

	if (magToPlayer < 0.001f)               return false;
	if (magToPlayer > visionTiles * tileSize) return false;

	float cosAngle = (dir.x * fx + dir.y * fy) / magToPlayer;
	if (cosAngle < 0.7071f)                 return false; // outside 45-degree cone

	// Bresenham line-of-sight trace
	Vector2 grid0 = WorldToGrid(enemyCenter, tileSize);
	Vector2 grid1 = WorldToGrid(playerPos, tileSize);

	int x0 = static_cast<int>(grid0.x), y0 = static_cast<int>(grid0.y);
	int x1 = static_cast<int>(grid1.x), y1 = static_cast<int>(grid1.y);

	int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1, sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	auto isWall = [&](int y, int x) {
		return (y < 0 || y >= static_cast<int>(map.size()) ||
			x < 0 || x >= static_cast<int>(map[y].size()) ||
			map[y][x] == '1' || map[y][x] == '3' ||
			map[y][x] == '.' || map[y][x] == 'R');
		};

	while (true) {
		if (isWall(y0, x0)) return false;
		if (x0 == x1 && y0 == y1) break;
		int e2 = 2 * err;
		if (e2 > -dy) { err -= dy; x0 += sx; }
		if (e2 < dx) { err += dx; y0 += sy; }
	}

	return true;
}

// ===========================================================================
// withinLineOfSight  - true when no wall tiles block the line to the player
// ===========================================================================
bool EnemyAi::withinLineOfSight(Vector2 playerPos) const
{
	Vector2 enemyCenter = { position.x + tileSize * 0.5f, position.y + tileSize * 0.5f };

	Vector2 grid0 = WorldToGrid(enemyCenter, tileSize);
	Vector2 grid1 = WorldToGrid(playerPos, tileSize);

	int x0 = static_cast<int>(grid0.x), y0 = static_cast<int>(grid0.y);
	int x1 = static_cast<int>(grid1.x), y1 = static_cast<int>(grid1.y);

	int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1, sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	auto isWall = [&](int y, int x) {
		return (y < 0 || y >= static_cast<int>(map.size()) ||
			x < 0 || x >= static_cast<int>(map[y].size()) ||
			map[y][x] == '1' || map[y][x] == '3' ||
			map[y][x] == '.' || map[y][x] == 'R');
		};

	while (true) {
		if (isWall(y0, x0)) return false;
		if (x0 == x1 && y0 == y1) break;
		int e2 = 2 * err;
		if (e2 > -dy) { err -= dy; x0 += sx; }
		if (e2 < dx) { err += dx; y0 += sy; }
	}

	return true;
}

// ===========================================================================
// Messaging
// ===========================================================================
void EnemyAi::SendMessage(const std::string& id) const {
	if (messageHub) {
		Messaging::IMessage msg(id);
		messageHub->ProcessMessage(&msg);
	}
}

// ===========================================================================
// Physics setup
// ===========================================================================
SpeedComponent EnemyAi::SetComponentEnemyPhysics(Entity e, EntityForceProxy* force)
{
	enemyspd = nullptr;
	if (!force)
	{
		SpeedComponent temp(e);
		temp.friction = enemyFriction;
		temp.maxSpeed = enemyMaxSpeed;
		return temp;
	}

	SpeedComponent* spd = force->GetSpeedComponent(e);
	if (!spd)
	{
		SpeedComponent temp(e);
		temp.friction = enemyFriction;
		temp.maxSpeed = enemyMaxSpeed;
		return temp;
	}

	enemyspd = spd;
	enemyspd->friction = enemyFriction;
	enemyspd->maxSpeed = enemyMaxSpeed;
	return *enemyspd;
}

// ===========================================================================
// Grid / map helpers
// ===========================================================================
void EnemyAi::SetGrid(const std::vector<std::string>& g, float ts)
{
	map = g;
	std::reverse(map.begin(), map.end());
	tileSize = ts;

	Vector2 gridPos = WorldToGrid(position, tileSize);
	position = GridToWorld(gridPos, tileSize);
}

bool EnemyAi::isBlocked(const Vector2& nextPos) const
{
	int col = static_cast<int>(std::floor(nextPos.x / tileSize));
	int row = static_cast<int>(std::floor(nextPos.y / tileSize));

	if (row < 0 || row >= static_cast<int>(map.size()) ||
		col < 0 || col >= static_cast<int>(map[row].size()))
		return true;

	return (map[row][col] == '1' || map[row][col] == '3' ||
		map[row][col] == '.' || map[row][col] == 'R');
}

bool EnemyAi::isBlockedGrid(const Vector2& gridPos) const
{
	int col = static_cast<int>(gridPos.x);
	int row = static_cast<int>(gridPos.y);

	if (row < 0 || row >= static_cast<int>(map.size()) ||
		col < 0 || col >= static_cast<int>(map[row].size()))
		return true;

	return (map[row][col] == '1' || map[row][col] == '3' ||
		map[row][col] == '.' || map[row][col] == 'R');
}

bool EnemyAi::wouldOverlapPlayer(const Vector2& nextPos, const Vector2& playerPos) const
{
	Vector2 enemyCenter{ nextPos.x + tileSize * 0.5f,  nextPos.y + tileSize * 0.5f };
	Vector2 playerCenter{ playerPos.x + tileSize * 0.5f, playerPos.y + tileSize * 0.5f };
	const float minDistance = tileSize * 0.5f;

	float dx = enemyCenter.x - playerCenter.x;
	float dy = enemyCenter.y - playerCenter.y;
	float distSq = dx * dx + dy * dy;

	return distSq < (minDistance * minDistance);
}

bool EnemyAi::isAtSpawn(const Vector2& pos)
{
	return std::fabs(pos.x - spawnPos.x) < 2.0f &&
		std::fabs(pos.y - spawnPos.y) < 2.0f;
}

// ===========================================================================
// updateMinibossChanneler  — Ranged mini-boss (3x aimed shot + 8-dir burst)
//
// Attack combo (mirrors updateRanged structure, Lua: miniboss_channeler.lua):
//
//   Shots 0-2  (NORMAL phase)
//     Kite into sweet spot (2-5 tiles).
//     Fire one aimed projectile.  "RangedNormalAttack"
//     Wait normalShotDelay (2.0s) before next shot.
//
//   Shot 3  -> CHARGING phase
//     Freeze in place.  Broadcast "BossOmniCharge" (wind-up VFX).
//     Hold for burstChargeTime (0.6s) then move to FIRING.
//
//   FIRING phase
//     One projectile per frame across 8 directions (N/NE/E/SE/S/SW/W/NW).
//     Each direction sends its own message ("BossOmniShot_N" etc.) so the
//     renderer / audio layer can show the slow-projectile VFX per direction.
//     After all 8 are sent: 4.0s combo cooldown, channelerShotsFired reset.
//
// Lua variable mapping:
//   s_shotsFired  -> channelerShotsFired
//   s_comboPhase  -> channelerComboPhase  (NORMAL / CHARGING / FIRING)
//   s_burstIndex  -> channelerBurstIndex  (0-7 while FIRING)
//   s_attackTimer -> AttackTimer
// ===========================================================================
void EnemyAi::updateMinibossChanneler(float deltaTime, Vector2 playerPos,
	Entity e, EntityForceProxy* forceproxy)
{
	// ---- timing constants (Lua: top-of-file constants) ----------------------
	const float minRange        = 2.0f * tileSize;
	const float maxRange        = 5.0f * tileSize;
	const float kiteSpeed       = 80.0f;
	const float normalShotDelay = 2.0f;
	const float burstChargeTime = 0.6f;
	const float comboCooldown   = 4.0f;
	// -------------------------------------------------------------------------

	// 8-direction burst table — unit vectors + per-direction message strings.
	// Diagonals are pre-normalised (1/sqrt(2) ~ 0.7071).
	// Lua equivalent: BURST_DIRS table at top of miniboss_channeler.lua
	static const struct { float dx; float dy; const char* msg; } kDirs[8] = {
		{  0.0f,    1.0f,   "BossOmniShot_N"  },
		{  0.7071f, 0.7071f,"BossOmniShot_NE" },
		{  1.0f,    0.0f,   "BossOmniShot_E"  },
		{  0.7071f,-0.7071f,"BossOmniShot_SE" },
		{  0.0f,   -1.0f,   "BossOmniShot_S"  },
		{ -0.7071f,-0.7071f,"BossOmniShot_SW" },
		{ -1.0f,    0.0f,   "BossOmniShot_W"  },
		{ -0.7071f, 0.7071f,"BossOmniShot_NW" },
	};

	Vector2 enemyCenter  { position.x + tileSize * 0.5f,   position.y + tileSize * 0.5f  };
	Vector2 playerCenter { playerPos.x + tileSize * 0.25f, playerPos.y + tileSize * 0.5f };
	float dx   = playerCenter.x - enemyCenter.x;
	float dy   = playerCenter.y - enemyCenter.y;
	float dist = std::sqrt(dx * dx + dy * dy);

	if (!withinLineOfSight(playerPos))
		return;

	// Face player
	if (std::fabs(dx) > std::fabs(dy))
		currentDirection = (dx > 0) ? Direction::RIGHT : Direction::LEFT;
	else
		currentDirection = (dy > 0) ? Direction::UP : Direction::DOWN;

	// -----------------------------------------------------------------------
	// CHARGING: frozen wind-up before the burst fires
	// Lua: if s_comboPhase == CP_BURST then ... end
	// -----------------------------------------------------------------------
	if (channelerComboPhase == ChannelerComboPhase::CHARGING)
	{
		if (forceproxy) {
			SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
			if (spd) spd->speed = { 0.0f, 0.0f };
		}
		AttackTimer -= deltaTime;
		if (AttackTimer <= 0.0f)
		{
			channelerComboPhase = ChannelerComboPhase::FIRING;
			channelerBurstIndex = 0;
			DebugConsole::Get().Info("[CHANNELER] Burst firing.\n");
		}
		return;
	}

	// -----------------------------------------------------------------------
	// FIRING: one direction per frame until all 8 are sent
	// Lua: if s_comboPhase == CP_FIRING then ... end
	// -----------------------------------------------------------------------
	if (channelerComboPhase == ChannelerComboPhase::FIRING)
	{
		if (forceproxy) {
			SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
			if (spd) spd->speed = { 0.0f, 0.0f };
		}

		if (channelerBurstIndex < 8)
		{
			const auto& d = kDirs[channelerBurstIndex];
			// Each directional message is handled by the projectile listener in
			// GameApp the same way "RangedNormalAttack" is — the listener reads
			// the direction from the message string and spawns the entity there.
			SendMessage(d.msg);
			DebugConsole::Get().Info(
				std::string("[CHANNELER] Omni burst ") +
				std::to_string(channelerBurstIndex + 1) + "/8\n");
			++channelerBurstIndex;
		}
		else
		{
			// All 8 fired — full-combo cooldown then reset
			channelerComboPhase = ChannelerComboPhase::NORMAL;
			channelerShotsFired = 0;
			AttackTimer         = comboCooldown;
			DebugConsole::Get().Info("[CHANNELER] Combo complete. Cooldown.\n");
		}
		return;
	}

	// -----------------------------------------------------------------------
	// NORMAL phase: kite into sweet spot, then fire aimed shots
	// Lua: normal repositioning + timer block at bottom of ST_ATTACK
	// -----------------------------------------------------------------------

	// Kite — mirror of updateRanged repositioning
	Vector2 moveDir{ 0.0f, 0.0f };
	if      (dist < minRange && dist > 0.001f) { moveDir.x = -dx / dist; moveDir.y = -dy / dist; }
	else if (dist > maxRange)                  { moveDir.x =  dx / dist; moveDir.y =  dy / dist; }

	if (moveDir.x != 0.0f || moveDir.y != 0.0f)
	{
		Vector2 nextCenter {
			enemyCenter.x + moveDir.x * kiteSpeed * deltaTime,
			enemyCenter.y + moveDir.y * kiteSpeed * deltaTime
		};
		if (!isBlocked(nextCenter))
		{
			if (forceproxy)
				forceproxy->triggerForce(e, { moveDir.x * kiteSpeed, moveDir.y * kiteSpeed });
			else {
				position.x = nextCenter.x - tileSize * 0.5f;
				position.y = nextCenter.y - tileSize * 0.5f;
			}
		}
	}

	// Only fire when inside the sweet spot
	if (dist < minRange || dist > maxRange)
		return;

	AttackTimer -= deltaTime;
	if (AttackTimer > 0.0f)
		return;

	if (channelerShotsFired < 3)
	{
		// Normal aimed single shot — mirrors updateRanged exactly:
		// the listener in GameApp handles "RangedNormalAttack" and reads the
		// enemy's facing direction to spawn the projectile entity.
		SendMessage("RangedNormalAttack");
		DebugConsole::Get().Info(
			"[CHANNELER] Normal shot " +
			std::to_string(channelerShotsFired + 1) + "/3\n");
		AttackTimer = normalShotDelay;
		++channelerShotsFired;
	}
	else
	{
		// 3 shots done — begin burst charge
		channelerComboPhase = ChannelerComboPhase::CHARGING;
		AttackTimer         = burstChargeTime;
		SendMessage("BossOmniCharge");
		DebugConsole::Get().Info("[CHANNELER] Charging omni burst.\n");
	}
}

// ===========================================================================
// updateMinibossWarden  — Bless mini-boss (3x melee + AOE bless + self-heal)
//
// Attack combo (Lua: miniboss_warden.lua):
//
//   Hits 0-2  (MELEE phase)
//     Close on player at approachSpeed (110).
//     Strike when within meleeRange (2 tiles).  "WardenMeleeAttack"
//     Wait meleeHitDelay (1.2s) before next hit.
//
//   Hit 3  -> SANCTIFY phase
//     Freeze, become invulnerable.  "WardenSanctifyStart" + "WardenInvulnStart"
//     Immediately broadcast "WardenBlessAOECharge" so renderer shows wind-up VFX.
//     Channel for sanctifyDuration (1.5s).  On expiry:
//       Fire AOE bless circle centred on the Warden — NOT targeted, identical
//       to the regular bless mob AOE.  "WardenBlessAOEFire"
//       Player absorption window is open the moment the AOE fires:
//         "WardenBlessAOEAbsorb" — broadcast alongside fire so GameApp can
//                                   set playerHasBless = true if player is
//                                   within blessRadius and presses absorb key.
//       If player does NOT absorb and is within radius: "WardenBlessAOEHit"
//       Self-heal:  wardenHp += wardenHealAmount (capped at wardenMaxHp).
//                   "WardenHeal"
//       Close invuln.  "WardenInvulnEnd"
//       Begin COOLDOWN (5.0s — player punish window).
//
//   COOLDOWN phase (5.0s)
//     Warden fully vulnerable.  Drifts slowly toward player.
//     On expiry: combo resets to MELEE.
//
// AOE behaviour note:
//   "WardenBlessAOEFire" behaves IDENTICALLY to "BlessAOEFire" from the
//   regular bless mob — centred on the entity, non-targeted, absorbable.
//   The only difference is the message prefix so the renderer can use
//   the Warden-specific VFX style.
//
// Lua variable mapping:
//   s_meleeHits         -> wardenMeleeHits
//   s_wardenPhase       -> wardenPhase  ("MELEE"/"SANCTIFY"/"COOLDOWN")
//   s_actionTimer       -> wardenActionTimer
//   s_chargeStarted     -> wardenChargeStarted  (bool, first-frame flag)
//   s_hp                -> wardenHp
//   MELEE_HIT_DELAY     -> meleeHitDelay     (1.2f)
//   SANCTIFY_DURATION   -> sanctifyDuration  (1.5f)
//   SANCTIFY_COOLDOWN   -> sanctifyCooldown  (5.0f)
//   BLESS_AOE_RADIUS_FACTOR = 2.5  (same as bless mob)
//   HEAL_AMOUNT         -> wardenHealAmount  (5)
// ===========================================================================
void EnemyAi::updateMinibossWarden(float deltaTime, Vector2 playerPos,
	Entity e, EntityForceProxy* forceproxy)
{
	// ---- timing / tuning constants (Lua: top-of-file constants) ------------
	const float meleeRange        = 2.0f * tileSize;   // Lua: MELEE_RANGE_TILES    = 2.0
	const float approachSpeed     = 110.0f;             // Lua: APPROACH_SPEED
	const float meleeHitDelay     = 1.2f;               // Lua: MELEE_HIT_DELAY
	const float sanctifyDuration  = 1.5f;               // Lua: SANCTIFY_DURATION
	const float sanctifyCooldown  = 5.0f;               // Lua: SANCTIFY_COOLDOWN
	const float blessRadius       = 2.5f * tileSize;    // Lua: BLESS_AOE_RADIUS_FACTOR = 2.5
	const int   wardenHealAmount  = 5;                  // Lua: HEAL_AMOUNT
	const int   wardenMaxHp       = 30;                 // Lua: WARDEN_MAX_HP
	// -------------------------------------------------------------------------

	Vector2 enemyCenter  { position.x + tileSize * 0.5f,   position.y + tileSize * 0.5f  };
	Vector2 playerCenter { playerPos.x + tileSize * 0.25f, playerPos.y + tileSize * 0.5f };
	float dx   = playerCenter.x - enemyCenter.x;
	float dy   = playerCenter.y - enemyCenter.y;
	float dist = std::sqrt(dx * dx + dy * dy);

	// -----------------------------------------------------------------------
	// SANCTIFY: invulnerable channel — AOE bless (absorbable) + self-heal
	// Lua: if s_wardenPhase == "SANCTIFY" then
	// -----------------------------------------------------------------------
	if (wardenPhase == WardenPhase::SANCTIFY)
	{
		if (forceproxy) {
			SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
			if (spd) spd->speed = { 0.0f, 0.0f };
		}

		// First frame of SANCTIFY: broadcast charge wind-up so renderer can
		// show the same charge VFX as the regular bless mob.
		// Lua: if s_actionTimer == 0 then SendMessage("WardenBlessAOECharge") end
		if (wardenActionTimer == 0.0f)
		{
			SendMessage("WardenBlessAOECharge");
			DebugConsole::Get().Info("[WARDEN] Sanctify charging AOE.\n");
		}

		wardenActionTimer += deltaTime;
		if (wardenActionTimer >= sanctifyDuration)
		{
			// Fire AOE bless circle centred on this entity — NOT targeted.
			// Behaviour is identical to "BlessAOEFire" from the regular bless mob:
			//   - centred on the Warden's position, not the player's
			//   - player can absorb this and mutate it (GameApp handles on receipt)
			// Lua: SendMessage("WardenBlessAOEFire")
			SendMessage("WardenBlessAOEFire");
			DebugConsole::Get().Info(
				"[WARDEN] Sanctify: bless AOE fired (non-targeted), radius = " +
				std::to_string(blessRadius) + " units.\n");

			// Broadcast absorb signal — GameApp checks if player pressed absorb
			// key within this frame and sets playerHasBless if so.
			// Lua: SendMessage("WardenBlessAOEAbsorb")
			SendMessage("WardenBlessAOEAbsorb");

			// Hit check: only damages player if they did NOT absorb the AOE.
			// GameApp resolves absorb vs hit priority; we broadcast both and
			// let it decide.  If player is outside radius, no hit either way.
			// Lua: if ai_dist(ecx,ecy,pcx,pcy) <= BLESS_AOE_RADIUS_FACTOR*ts then
			if (dist <= blessRadius)
			{
				SendMessage("WardenBlessAOEHit");
				DebugConsole::Get().Info("[WARDEN] Bless AOE HIT player.\n");
			}

			// Self-heal — always happens regardless of whether player was hit
			wardenHp = std::min(wardenHp + wardenHealAmount, wardenMaxHp);
			SendMessage("WardenHeal");
			DebugConsole::Get().Info(
				"[WARDEN] Healed " + std::to_string(wardenHealAmount) +
				" HP. wardenHp = " + std::to_string(wardenHp) + "\n");

			// Close invuln window and start vulnerable cooldown
			SendMessage("WardenInvulnEnd");
			wardenPhase       = WardenPhase::COOLDOWN;
			wardenActionTimer = 0.0f;
			DebugConsole::Get().Info("[WARDEN] Invuln ended — player punish window open.\n");
		}
		return;
	}

	// -----------------------------------------------------------------------
	// COOLDOWN: fully vulnerable; slow drift toward player
	// Lua: if s_wardenPhase == "COOLDOWN" then
	// -----------------------------------------------------------------------
	if (wardenPhase == WardenPhase::COOLDOWN)
	{
		wardenActionTimer += deltaTime;
		if (wardenActionTimer >= sanctifyCooldown)
		{
			wardenMeleeHits   = 0;
			wardenPhase       = WardenPhase::MELEE;
			wardenActionTimer = 0.0f;
			AttackTimer       = 0.0f;
			DebugConsole::Get().Info("[WARDEN] Cooldown over — combo reset.\n");
		}
		else if (dist > 0.001f)
		{
			// Drift slowly so player cannot simply stand still
			Vector2 dir = { dx / dist, dy / dist };
			const float driftSpeed = approachSpeed * 0.3f;
			Vector2 nextCenter {
				enemyCenter.x + dir.x * driftSpeed * deltaTime,
				enemyCenter.y + dir.y * driftSpeed * deltaTime
			};
			if (!isBlocked(nextCenter))
			{
				if (forceproxy)
					forceproxy->triggerForce(e, { dir.x * driftSpeed, dir.y * driftSpeed });
				else {
					position.x = nextCenter.x - tileSize * 0.5f;
					position.y = nextCenter.y - tileSize * 0.5f;
				}
			}
		}
		return;
	}

	// -----------------------------------------------------------------------
	// MELEE: close on player and land up to 3 hits
	// Lua: if s_wardenPhase == "MELEE" then
	// -----------------------------------------------------------------------

	// Face player
	if (std::fabs(dx) > std::fabs(dy))
		currentDirection = (dx > 0) ? Direction::RIGHT : Direction::LEFT;
	else
		currentDirection = (dy > 0) ? Direction::UP : Direction::DOWN;

	// Close the gap if outside melee range
	if (dist > meleeRange)
	{
		Vector2 dir = { dx / dist, dy / dist };
		Vector2 nextCenter {
			enemyCenter.x + dir.x * approachSpeed * deltaTime,
			enemyCenter.y + dir.y * approachSpeed * deltaTime
		};
		if (!isBlocked(nextCenter))
		{
			if (forceproxy)
				forceproxy->triggerForce(e, { dir.x * approachSpeed, dir.y * approachSpeed });
			else {
				position.x = nextCenter.x - tileSize * 0.5f;
				position.y = nextCenter.y - tileSize * 0.5f;
			}
		}
		return;
	}

	// Within melee range — freeze and tick the inter-hit delay
	if (forceproxy) {
		SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
		if (spd) spd->speed = { 0.0f, 0.0f };
	}

	AttackTimer -= deltaTime;
	if (AttackTimer > 0.0f)
		return;

	if (wardenMeleeHits < 3)
	{
		SendMessage("WardenMeleeAttack");
		DebugConsole::Get().Info(
			"[WARDEN] Melee hit " +
			std::to_string(wardenMeleeHits + 1) + "/3\n");
		AttackTimer = meleeHitDelay;
		++wardenMeleeHits;
	}
	else
	{
		// 3 hits done — begin Sanctify
		// Note: "WardenBlessAOECharge" is sent on the first frame of SANCTIFY
		// (when wardenActionTimer == 0), not here, so the charge VFX starts
		// exactly when the Warden freezes rather than one frame earlier.
		SendMessage("WardenSanctifyStart");
		SendMessage("WardenInvulnStart");
		wardenPhase       = WardenPhase::SANCTIFY;
		wardenActionTimer = 0.0f;
		DebugConsole::Get().Info("[WARDEN] Sanctify started — invulnerable.\n");
	}
}

// ===========================================================================
// updateMinibossPopper  — Burrow mini-boss (3x basic burrow + triangle pop)
//
// A large burrow-type mini-boss with a wider-than-normal hit radius.
// Skill test: read all 3 simultaneous warning spots and find the gap.
//
// Attack combo (Lua: miniboss_popper.lua):
//
//   Burrows 0-2  (BASIC phase)
//     Identical to updateBurrow: ABOVE_GROUND -> TELEGRAPHING -> BURROWED ->
//     STRIKING -> COOLDOWN.  popperBurrowCount increments each cycle.
//     Hit radius is POPPER_HIT_RADIUS (1.5 * tileSize) — wider than normal.
//     All standard "Burrow*" messages broadcast.
//
//   Burrow 3  -> Triangle Pop signature (popperBurrowCount >= 3):
//
//     TELEGRAPH phase (POPPER_TELEGRAPH_DUR = 1.2s)
//       All 3 pop positions shown simultaneously via "PopperTriangleTelegraph".
//       Positions are computed around the player at three distances / angles:
//         Pop 0: 3.0 tiles away, 90 degrees (directly above the player)
//         Pop 1: 2.0 tiles away, 210 degrees (lower-left)
//         Pop 2: 1.0 tile  away, 330 degrees (lower-right)
//       The positions close inward so the safe gap is between pops 0 and 2.
//       The Popper freezes in place during this window.
//
//     POPPING phase
//       Pops fire in sequence: pop 0 -> pop 1 -> pop 2, 0.4s apart.
//       Each pop: Popper teleports to that position and surfaces.
//         "PopperPop_N"    — renderer shows emerge VFX
//         "PopperPopHit_N" — if player is within POPPER_POP_RADIUS (1.5 * tileSize)
//       Between pops the Popper is underground (isBurrowed = true).
//
//     COOLDOWN phase (POPPER_COOLDOWN = 4.0s)
//       Popper surfaces at its last pop position, fully vulnerable.
//       "PopperVulnerable" — open damage window
//       "PopperCooldownEnd" — close window, full combo reset.
//
// Constants (Lua: top-of-file constants):
//   POPPER_APPROACH_SPEED  = 80.0
//   POPPER_BASIC_RANGE     = 2.0 * tileSize
//   POPPER_TELEGRAPH_DUR   = 0.8   (basic burrow telegraph)
//   POPPER_TRAVEL_TIME     = 0.6   (underground travel)
//   POPPER_STRIKE_DUR      = 1.2   (basic burrow stun)
//   POPPER_BASIC_COOLDOWN  = 2.5   (between basic burrows)
//   POPPER_HIT_RADIUS      = 1.5 * tileSize  (wide hitbox)
//   POPPER_TRI_TELEGRAPH   = 1.2   (all-3-spots shown window)
//   POPPER_POP_INTERVAL    = 0.4   (seconds between each pop)
//   POPPER_COOLDOWN        = 4.0
//   POPPER_POP_RADIUS      = 1.5 * tileSize
//
// Lua variable mapping:
//   s_burrowCount      -> popperBurrowCount
//   s_popPhase         -> popperPhase  ("BASIC"/"TELEGRAPH"/"POPPING"/"COOLDOWN")
//   s_popTimer         -> popperPhaseTimer
//   s_popIndex         -> popperPopIndex
//   s_popPositions[i]  -> popperPopPos[i]
// ===========================================================================
void EnemyAi::updateMinibossPopper(float deltaTime, Vector2 playerPos,
	Entity e, EntityForceProxy* forceproxy)
{
	// ---- timing / tuning constants (Lua: top-of-file constants) ------------
	// Each name maps directly to a Lua constant of the same name.
	const float approachSpeed      = 80.0f;   // Lua: APPROACH_SPEED
	const float basicRange         = 2.0f * tileSize; // Lua: BASIC_RANGE_TILES   = 2.0
	const float telegraphDur       = 0.8f;    // Lua: TELEGRAPH_DUR
	const float travelTime         = 0.6f;    // Lua: BURROW_TRAVEL_TIME
	const float strikeDur          = 1.2f;    // Lua: STRIKE_DUR
	const float basicCooldown      = 2.5f;    // Lua: BASIC_COOLDOWN
	const float popperHitRadius    = 1.5f * tileSize; // Lua: HIT_RADIUS_FACTOR   = 1.5
	const float triTelegraphDur    = 1.2f;    // Lua: TRI_TELEGRAPH_DUR
	const float popInterval        = 0.4f;    // Lua: POP_INTERVAL
	const float popperCooldown     = 4.0f;    // Lua: POPPER_COOLDOWN
	const float popRadius          = 1.5f * tileSize; // Lua: POP_RADIUS_FACTOR   = 1.5
	// -------------------------------------------------------------------------

	// Triangle pop radii (tiles) and angles (degrees, measured from world +X axis)
	// Pop 0: 3.0 tiles at  90 deg — above the player  (furthest, fires first)
	// Pop 1: 2.0 tiles at 210 deg — lower-left        (mid distance)
	// Pop 2: 1.0 tile  at 330 deg — lower-right       (closest, fires last)
	// The gap to dodge through is between pop 0 and pop 2 (upper-right quadrant).
	//
	// Lua equivalent (top-of-file constants):
	//   POP_RADIUS_TILES = { 3.0, 2.0, 1.0 }
	//   POP_ANGLE_DEG    = { 90,  210,  330 }
	//   (convert to radians: math.rad(POP_ANGLE_DEG[i]) before calling math.cos/sin)
	static const float kPopRadiusTiles[3] = { 3.0f, 2.0f, 1.0f };
	static const float kPopAngleDeg[3]    = { 90.0f, 210.0f, 330.0f };
	// -------------------------------------------------------------------------

	Vector2 enemyCenter  { position.x + tileSize * 0.5f,   position.y + tileSize * 0.5f  };
	Vector2 playerCenter { playerPos.x + tileSize * 0.25f, playerPos.y + tileSize * 0.5f };
	float dx   = playerCenter.x - enemyCenter.x;
	float dy   = playerCenter.y - enemyCenter.y;
	float dist = std::sqrt(dx * dx + dy * dy);

	// Freeze helper
	// Lua: SetPos(entity, ex, ey) with no velocity
	auto freeze = [&]() {
		if (forceproxy) {
			SpeedComponent* spd = forceproxy->GetSpeedComponent(e);
			if (spd) spd->speed = { 0.0f, 0.0f };
		}
	};

	// =======================================================================
	// TRIANGLE POP PATH — popperPhase != BASIC
	// Lua: if s_popPhase ~= "BASIC" then
	// =======================================================================

	// -------------------------------------------------------------------
	// TELEGRAPH: freeze, show all 3 warning spots simultaneously
	// Lua: if s_popPhase == "TELEGRAPH" then
	// -------------------------------------------------------------------
	if (popperPhase == PopperPhase::TELEGRAPH)
	{
		freeze();
		popperPhaseTimer += deltaTime;

		if (popperPhaseTimer >= triTelegraphDur)
		{
			// Telegraph window over — begin popping sequence
			popperPhase      = PopperPhase::POPPING;
			popperPhaseTimer = 0.0f;
			popperPopIndex   = 0;
			isBurrowed       = true; // hide between pops
			DebugConsole::Get().Info("[POPPER] Triangle telegraph done — popping.\n");
		}
		return;
	}

	// -------------------------------------------------------------------
	// POPPING: fire pops in sequence at popInterval apart
	// Lua: if s_popPhase == "POPPING" then
	// -------------------------------------------------------------------
	if (popperPhase == PopperPhase::POPPING)
	{
		popperPhaseTimer += deltaTime;

		if (popperPopIndex < 3)
		{
			// Wait for the next pop's interval
			if (popperPhaseTimer >= static_cast<float>(popperPopIndex) * popInterval)
			{
				// Teleport to this pop's position
				const Vector2& popTarget = popperPopPos[popperPopIndex];
				position.x = popTarget.x - tileSize * 0.5f;
				position.y = popTarget.y - tileSize * 0.5f;
				isBurrowed = false;
				freeze();

				// Recalculate player distance from new position
				Vector2 newCenter { position.x + tileSize * 0.5f, position.y + tileSize * 0.5f };
				float   pdx  = playerCenter.x - newCenter.x;
				float   pdy  = playerCenter.y - newCenter.y;
				float   pDist = std::sqrt(pdx * pdx + pdy * pdy);

				std::string popMsg = "PopperPop_" + std::to_string(popperPopIndex + 1);
				SendMessage(popMsg);

				if (pDist <= popRadius)
				{
					SendMessage("PopperPopHit_" + std::to_string(popperPopIndex + 1));
					DebugConsole::Get().Info(
						"[POPPER] Pop " + std::to_string(popperPopIndex + 1) + " HIT player.\n");
				}
				else
				{
					DebugConsole::Get().Info(
						"[POPPER] Pop " + std::to_string(popperPopIndex + 1) + " missed.\n");
				}

				++popperPopIndex;

				// Re-hide between pops (unless this was the last one)
				if (popperPopIndex < 3)
					isBurrowed = true;
			}
		}
		else
		{
			// All 3 pops fired — enter cooldown at final pop position
			isBurrowed       = false;
			popperPhase      = PopperPhase::COOLDOWN;
			popperPhaseTimer = 0.0f;
			SendMessage("PopperVulnerable");
			DebugConsole::Get().Info("[POPPER] Triangle pop complete — cooldown.\n");
		}
		return;
	}

	// -------------------------------------------------------------------
	// COOLDOWN: fully vulnerable, slow retreat, then full reset
	// Lua: if s_popPhase == "COOLDOWN" then
	// -------------------------------------------------------------------
	if (popperPhase == PopperPhase::COOLDOWN)
	{
		popperPhaseTimer += deltaTime;

		// Slow drift away from player — player's punish window
		if (dist < basicRange && dist > 0.001f)
		{
			Vector2 away = { -(dx / dist) * approachSpeed * 0.3f,
			                 -(dy / dist) * approachSpeed * 0.3f };
			Vector2 nextCenter {
				enemyCenter.x + away.x * deltaTime,
				enemyCenter.y + away.y * deltaTime
			};
			if (!isBlocked(nextCenter))
			{
				if (forceproxy)
					forceproxy->triggerForce(e, away);
				else {
					position.x = nextCenter.x - tileSize * 0.5f;
					position.y = nextCenter.y - tileSize * 0.5f;
				}
			}
		}

		if (popperPhaseTimer >= popperCooldown)
		{
			// Full combo reset
			popperBurrowCount = 0;
			popperPhase       = PopperPhase::BASIC;
			popperPhaseTimer  = 0.0f;
			popperPopIndex    = 0;
			burrowPhase       = BurrowPhase::ABOVE_GROUND;
			burrowPhaseTimer  = 0.0f;
			isBurrowed        = false;
			SendMessage("PopperCooldownEnd");
			DebugConsole::Get().Info("[POPPER] Cooldown over — combo reset.\n");
		}
		return;
	}

	// =======================================================================
	// BASIC BURROW PATH — first 3 strikes, mirrors updateBurrow
	// Lua: if s_popPhase == "BASIC" then
	// =======================================================================
	switch (burrowPhase)
	{
		// -------------------------------------------------------------------
	case BurrowPhase::ABOVE_GROUND:
	{
		if (burrowPhaseTimer > 0.0f)
			burrowPhaseTimer -= deltaTime;

		if (dist > basicRange)
		{
			// Walk toward player
			Vector2 dir = { dx / dist, dy / dist };
			Vector2 desiredVel = { dir.x * approachSpeed, dir.y * approachSpeed };
			Vector2 nextCenter {
				enemyCenter.x + desiredVel.x * deltaTime,
				enemyCenter.y + desiredVel.y * deltaTime
			};
			if (!isBlocked(nextCenter))
			{
				if (forceproxy)
					forceproxy->triggerForce(e, desiredVel);
				else {
					position.x = nextCenter.x - tileSize * 0.5f;
					position.y = nextCenter.y - tileSize * 0.5f;
				}
			}
			if (std::fabs(dx) > std::fabs(dy))
				currentDirection = (dx > 0) ? Direction::RIGHT : Direction::LEFT;
			else
				currentDirection = (dy > 0) ? Direction::UP : Direction::DOWN;
		}
		else if (burrowPhaseTimer <= 0.0f)
		{
			// Check if 3 basic burrows done -> transition to triangle pop
			if (popperBurrowCount >= 3)
			{
				// Precompute the 3 pop positions around the player
				// Lua: for i=0,2 do s_popPositions[i] = ... end
				for (int i = 0; i < 3; ++i)
				{
					float angleRad = kPopAngleDeg[i] * 3.14159265f / 180.0f;
					float radius   = kPopRadiusTiles[i] * tileSize;
					popperPopPos[i].x = playerCenter.x + std::cos(angleRad) * radius;
					popperPopPos[i].y = playerCenter.y + std::sin(angleRad) * radius;
				}

				// Broadcast all 3 warning positions at once so the player
				// can see the full triangle before the first pop fires
				SendMessage("PopperTriangleTelegraph");
				DebugConsole::Get().Info("[POPPER] Triangle telegraph started.\n");

				popperPhase      = PopperPhase::TELEGRAPH;
				popperPhaseTimer = 0.0f;
				isBurrowed       = false;
				freeze();
				return;
			}

			// Standard burrow telegraph
			burrowStrikeTarget = playerCenter;
			burrowPhaseTimer   = telegraphDur;
			burrowPhase        = BurrowPhase::TELEGRAPHING;
			isBurrowed         = false;
			freeze();
			SendMessage("BurrowWarning");
			DebugConsole::Get().Info(
				"[POPPER] Basic burrow #" +
				std::to_string(popperBurrowCount + 1) + " telegraphing.\n");
		}
		break;
	}
		// -------------------------------------------------------------------
	case BurrowPhase::TELEGRAPHING:
	{
		freeze();
		burrowPhaseTimer -= deltaTime;
		if (burrowPhaseTimer <= 0.0f)
		{
			burrowPhase      = BurrowPhase::BURROWED;
			burrowPhaseTimer = travelTime;
			isBurrowed       = true;
			SendMessage("BurrowUnderground");
			DebugConsole::Get().Info("[POPPER] Went underground.\n");
		}
		break;
	}
		// -------------------------------------------------------------------
	case BurrowPhase::BURROWED:
	{
		burrowPhaseTimer -= deltaTime;
		if (burrowPhaseTimer <= 0.0f)
		{
			// Emerge at locked strike target
			position.x = burrowStrikeTarget.x - tileSize * 0.5f;
			position.y = burrowStrikeTarget.y - tileSize * 0.5f;
			isBurrowed = false;
			freeze();

			// Re-evaluate hit with wide radius
			Vector2 newCenter { position.x + tileSize * 0.5f, position.y + tileSize * 0.5f };
			float   edx   = playerCenter.x - newCenter.x;
			float   edy   = playerCenter.y - newCenter.y;
			float   eDist = std::sqrt(edx * edx + edy * edy);

			if (eDist <= popperHitRadius) {
				SendMessage("BurrowStrike");
				DebugConsole::Get().Info("[POPPER] Basic burrow HIT player (wide radius).\n");
			}
			else {
				SendMessage("BurrowMissed");
				DebugConsole::Get().Info("[POPPER] Basic burrow missed.\n");
			}

			SendMessage("BurrowVulnerable");
			burrowPhaseTimer = strikeDur;
			burrowPhase      = BurrowPhase::STRIKING;
		}
		break;
	}
		// -------------------------------------------------------------------
	case BurrowPhase::STRIKING:
	{
		freeze();
		burrowPhaseTimer -= deltaTime;
		if (burrowPhaseTimer <= 0.0f)
		{
			SendMessage("BurrowInvulnerable");
			++popperBurrowCount;
			DebugConsole::Get().Info(
				"[POPPER] Basic burrow " +
				std::to_string(popperBurrowCount) + "/3 complete.\n");
			burrowPhaseTimer = basicCooldown;
			burrowPhase      = BurrowPhase::COOLDOWN;
		}
		break;
	}
		// -------------------------------------------------------------------
	case BurrowPhase::COOLDOWN:
	{
		burrowPhaseTimer -= deltaTime;
		if (burrowPhaseTimer <= 0.0f)
		{
			burrowPhase      = BurrowPhase::ABOVE_GROUND;
			burrowPhaseTimer = 0.0f;
			DebugConsole::Get().Info("[POPPER] Basic burrow cooldown over.\n");
		}
		// Slow retreat — mirrors updateBurrow cooldown drift
		if (dist < basicRange * 0.5f && dist > 0.001f)
		{
			Vector2 away = { -(dx / dist) * approachSpeed * 0.3f,
			                 -(dy / dist) * approachSpeed * 0.3f };
			Vector2 nextCenter {
				enemyCenter.x + away.x * deltaTime,
				enemyCenter.y + away.y * deltaTime
			};
			if (!isBlocked(nextCenter))
			{
				if (forceproxy)
					forceproxy->triggerForce(e, away);
				else {
					position.x = nextCenter.x - tileSize * 0.5f;
					position.y = nextCenter.y - tileSize * 0.5f;
				}
			}
		}
		break;
	}
	} // end switch burrowPhase
}


Vector2 WorldToGrid(const Vector2& pos, float tilesize) {
	return { std::floor(pos.x / tilesize), std::floor(pos.y / tilesize) };
}

Vector2 GridToWorld(const Vector2& pos, float tilesize) {
	return { pos.x * tilesize, pos.y * tilesize };
}

// ===========================================================================
// Misc
// ===========================================================================
void EnemyAi::SnapToGrid(float tilesize)
{
	position.x = static_cast<int>(std::floor(position.x / tilesize)) * tilesize;
	position.y = static_cast<int>(std::floor(position.y / tilesize)) * tilesize;
}

void EnemyAi::setPosition(const Vector2& pos) {
	position = pos;
}

void EnemyAi::ChangeState(StateType newState) {
	currentState = newState;
}

void EnemyAi::ChangeDirection(Direction newDirection) {
	currentDirection = newDirection;
}