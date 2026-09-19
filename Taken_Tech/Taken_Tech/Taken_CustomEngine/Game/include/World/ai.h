#pragma once
/**
* @file     ai.h
* @author   Lim Zhi Jie
* @email    zhijie.lim
* @co-author Sng Swee Yong Dillon
* @email	sweeyongdillon.sng
* @date     2025-11-05
*
* @brief Declares the EnemyAi class and related AI behavior interfaces.
*
* This header defines the EnemyAi component responsible for controlling
* the logic, state, and behavior of all enemy types in the game. It
* provides the structure and public interface for per-frame updates,
* movement, state transitions, and event-driven communication through
* the messaging system.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include <vector>
#include <memory>
#include "Core/component.h"
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include "Math/vect2.h"
#include "Input/message_system.h"
#include "Physics/EntityForceProxy.hpp"
#include "Core/assetsPath.h"
#include "Input/DebugConsole.hpp"
#include "fsm.h"

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------
enum class MobType : uint8_t {
	BASIC,
	RANGED,
	HEAL,
	BURROW,
	FINALBOSS,
	// ---------------------------------------------------------------------------
	// Mini-bosses (one per room, each with 3 basic attacks + a signature ability)
	// ---------------------------------------------------------------------------
	MINIBOSS_CHANNELER, // Ranged type: 3x aimed shot  -> 8-direction omni burst
	MINIBOSS_WARDEN,    // Bless type:  3x melee hit   -> AOE bless circle + self-heal
	MINIBOSS_POPPER     // Burrow type: 3x basic burrow -> closing triangle pop (wide hitbox)
};

enum class StateType {
	PATROL,
	CHASE,
	SEARCH,
	IDLE
};

enum class Direction {
	UP,
	DOWN,
	LEFT,
	RIGHT
};

// ---------------------------------------------------------------------------
// EnemyAi
// ---------------------------------------------------------------------------
class EnemyAi : public Component {
public:
	EnemyAi(Entity e, MobType type, float searchDuration = 2.0f, Direction SearchDirection = Direction::RIGHT);

	/**
	 * @brief Sets the spawn position for the enemy.
	 * @param enemyPos The world position to spawn at.
	 */
	void setEnemySpawn(Vector2 enemyPos) {
		spawnPos = enemyPos;
		position = enemyPos;
	}

	/**
	 * @brief Loads the navigation map from a file.
	 * 
	 * Reads a text file representing the grid map used for collision and pathfinding.
	 * 
	 * @param path Path to the map file.
	 */
	void setMap(const std::string& path) {
		map.clear();
		std::filesystem::path p(path);

		if (p.is_relative())
			p = AssetPath(path);

		if (!std::filesystem::exists(p)) {
			DebugConsole::Get().Error("[AI] setMap: file does NOT exist, nav map stays empty\n");
			return;
		}

		std::ifstream file(p);
		std::string line;
		while (std::getline(file, line)) {
			if (!line.empty())
				map.push_back(line);
		}

		std::reverse(map.begin(), map.end());
	}

	/**
	 * @brief Gets the current world position of the enemy.
	 * @return A constant reference to the Vector2 position.
	 */
	const Vector2& getPosition() const { return position; }

	/**
	 * @brief Sets the world position of the enemy.
	 * @param pos The new Vector2 position.
	 */
	void setPosition(const Vector2& pos);

	/**
	 * @brief Updates the enemy AI logic for the current frame.
	 * @param deltaTime Time elapsed since the last frame.
	 * @param playerPos The current position of the player.
	 * @param e The entity ID of this enemy.
	 * @param force Physics proxy for applying movement forces.
	 */
	void update(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* force);

	/**
	 * @brief Configures and returns the physics speed component for the enemy.
	 * @param e The entity ID of this enemy.
	 * @param force Physics proxy for the enemy.
	 * @return The configured SpeedComponent.
	 */
	SpeedComponent SetComponentEnemyPhysics(Entity e, EntityForceProxy* force);

	/**
	 * @brief Sets the messaging observable hub for the enemy to send/receive events.
	 * @param hub Pointer to the messaging observable.
	 */
	void SetObservable(Messaging::Observable* hub) { messageHub = hub; }

	/**
	 * @brief Sets the collision grid and tile size for the enemy's pathfinding.
	 * @param grid A 2D vector of strings representing the map grid.
	 * @param tileSize The size of each tile in world units.
	 */
	void SetGrid(const std::vector<std::string>& grid, float tileSize);

	/**
	 * @brief Sets the mob type for this enemy, determining its behavior.
	 * @param type The MobType enum value.
	 */
	void     SetMobType(MobType type) { mobType = type; }

	/**
	 * @brief Gets the current mob type of this enemy.
	 * @return The current MobType enum value.
	 */
	MobType  GetMobType()        const { return mobType; }

	/**
	 * @brief Checks if the final boss is currently in its ranged attack phase.
	 * @return True if this is the FINALBOSS and it is in phase 2.
	 */
	bool IsBossRangedPhase() const {
		return mobType == MobType::FINALBOSS && bossCurrentPhase == 2;
	}

	/**
	 * @brief Gets the Finite State Machine (FSM) controlling this enemy's logic.
	 * @return A pointer to the FSM instance.
	 */
	FSM* GetFSM() const { return fsm.get(); }

	// ------- public so states can call them -------

	// Movement / action update functions
	/**
	 * @brief Updates the patrol behavior.
	 * 
	 * Moves the enemy back and forth within a defined patrol range.
	 * 
	 * @param deltaTime Time elapsed since last frame.
	 * @param e The enemy entity.
	 * @param forceproxy Physics proxy for movement.
	 */
	void updatePatrol(float deltaTime, Entity e, EntityForceProxy* forceproxy);

	/**
	 * @brief Updates the chase behavior.
	 * 
	 * Moves the enemy towards the player's position using simple pathfinding.
	 * 
	 * @param deltaTime Time elapsed since last frame.
	 * @param playerPos Current player position.
	 * @param e The enemy entity.
	 * @param forceproxy Physics proxy for movement.
	 */
	void updateChase(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy);

	/**
	 * @brief Updates the search behavior.
	 * 
	 * Moves to the last known player position and performs a 360-degree scan.
	 * 
	 * @param deltaTime Time elapsed since last frame.
	 */
	void updateSearch(float deltaTime);

	/**
	 * @brief Updates the ranged attack behavior.
	 * 
	 * Manages distance from player and executes the 6-shot combo pattern.
	 * 
	 * @param deltaTime Time elapsed since last frame.
	 * @param playerPos Current player position.
	 * @param e The enemy entity.
	 * @param forceproxy Physics proxy for movement.
	 */
	void updateRanged(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy);

	/**
	 * @brief Updates the burrow behavior.
	 * 
	 * Manages the whack-a-mole phases: approach, telegraph, burrow, strike, cooldown.
	 * 
	 * @param deltaTime Time elapsed since last frame.
	 * @param playerPos Current player position.
	 * @param e The enemy entity.
	 * @param forceproxy Physics proxy for movement.
	 */
	void updateBurrow(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy);

	/**
	 * @brief Updates the heal behavior.
	 * 
	 * Heals self when low HP, otherwise attacks or moves closer.
	 * 
	 * @param deltaTime Time elapsed since last frame.
	 * @param playerPos Current player position.
	 * @param e The enemy entity.
	 * @param forceproxy Physics proxy for movement.
	 */
	void updateHeal(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy);

	/**
	 * @brief Updates the bless behavior.
	 * 
	 * Heals ranged allies if alive, otherwise attacks the player.
	 * 
	 * @param deltaTime Time elapsed since last frame.
	 * @param playerPos Current player position (or ally position in ally mode).
	 * @param e The enemy entity.
	 * @param forceproxy Physics proxy for movement.
	 */
	void updateBless(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy);

	/**
	 * @brief Updates the basic melee attack behavior.
	 * 
	 * Stops movement and triggers an attack animation/event.
	 * 
	 * @param deltaTime Time elapsed since last frame.
	 * @param playerPos Current player position.
	 * @param e The enemy entity.
	 * @param forceproxy Physics proxy for movement.
	 */
	void updateAttack(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy);

	/**
	 * @brief Updates the basic enemy dash attack behavior.
	 * 
	 * Manages the dash phases: ready, dashing, resting.
	 * 
	 * @param deltaTime Time elapsed since last frame.
	 * @param playerPos Current player position.
	 * @param e The enemy entity.
	 * @param forceproxy Physics proxy for movement.
	 */
	void updateBasic(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy);

	/**
	 * @brief Updates the boss behavior.
	 * 
	 * Manages the boss phase sequence and ability selection.
	 * 
	 * @param deltaTime Time elapsed since last frame.
	 * @param playerPos Current player position.
	 * @param e The enemy entity.
	 * @param forceproxy Physics proxy for movement.
	 */
	void updateBoss(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy);

	/**
	 * @brief Updates the player's absorbed bless ability.
	 *
	 * Call this from the player update loop every frame, passing the current
	 * key states.  Handles mode switching (HEAL/ATTACK), charge-up, firing,
	 * and cooldown.  Does nothing if playerHasBless == false.
	 *
	 * Lua equivalent: UpdatePlayerBless() called from player OnUpdate.
	 *
	 * @param deltaTime       Time elapsed since last frame.
	 * @param switchKeyPressed True on the frame KEY_BLESS_SWITCH (Q) is pressed.
	 * @param useKeyHeld       True while KEY_BLESS_USE (E) is held.
	 * @param useKeyReleased   True on the frame KEY_BLESS_USE (E) is released.
	 * @param playerPos        Current player world position.
	 */
	void UpdatePlayerBless(float deltaTime, bool switchKeyPressed,
	                       bool useKeyHeld, bool useKeyReleased,
	                       Vector2 playerPos);

	/**
	 * @brief Updates the Channeler mini-boss attack behavior.
	 *
	 * Kites between MIN_RANGE and MAX_RANGE tiles. Fires 3 aimed normal shots
	 * then charges for 0.6s before releasing an 8-direction slow radial burst.
	 * 4.0s cooldown resets the combo.
	 *
	 * Lua equivalent: OnUpdate ST_ATTACK block in miniboss_channeler.lua
	 *
	 * @param deltaTime Time elapsed since last frame.
	 * @param playerPos Current player position.
	 * @param e The enemy entity.
	 * @param forceproxy Physics proxy for movement.
	 */
	void updateMinibossChanneler(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy);

	/**
	 * @brief Updates the Warden mini-boss attack behavior.
	 *
	 * Closes on the player, delivers 3 melee hits, then channels Sanctify:
	 * becomes invulnerable, fires a small AOE bless circle, and heals itself.
	 * 5.0s vulnerable cooldown follows before the combo resets.
	 *
	 * Lua equivalent: OnUpdate ST_ATTACK block in miniboss_warden.lua
	 *
	 * @param deltaTime Time elapsed since last frame.
	 * @param playerPos Current player position.
	 * @param e The enemy entity.
	 * @param forceproxy Physics proxy for movement.
	 */
	void updateMinibossWarden(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy);

	/**
	 * @brief Updates the Popper mini-boss attack behavior.
	 *
	 * Performs 3 standard burrow cycles with a wider-than-normal hit radius,
	 * then executes the Triangle Pop signature: telegraphs all 3 pop positions
	 * simultaneously (giving the player 1.2s to find the gap), then pops emerge
	 * in sequence at 0.4s intervals closing inward. 4.0s cooldown follows.
	 *
	 * Lua equivalent: OnUpdate ST_ATTACK block in miniboss_popper.lua
	 *
	 * @param deltaTime Time elapsed since last frame.
	 * @param playerPos Current player position.
	 * @param e The enemy entity.
	 * @param forceproxy Physics proxy for movement.
	 */
	void updateMinibossPopper(float deltaTime, Vector2 playerPos, Entity e, EntityForceProxy* forceproxy);

	/**
	 * @brief Moves the enemy back to its spawn position.
	 * 
	 * Called by ReturnToSpawnState. Sets atSpawn = true upon arrival.
	 * 
	 * @param deltaTime Time elapsed since last frame.
	 * @param e The enemy entity.
	 * @param forceproxy Physics proxy for movement.
	 */
	void moveToSpawn(float deltaTime, Entity e, EntityForceProxy* forceproxy);

	/**
	 * @brief Checks if the player is within the enemy's forward vision cone.
	 * @param playerPos Player position.
	 * @return True if player is in front and visible.
	 */
	bool playerInFront(Vector2 playerPos) const;

	/**
	 * @brief Checks if there is a clear line of sight to the player.
	 * @param playerPos Player position.
	 * @return True if no walls block the view.
	 */
	bool withinLineOfSight(Vector2 playerPos) const;

	/**
	 * @brief Checks if the player is within attack range.
	 * @param playerPos Player position.
	 * @return True if in range.
	 */
	bool withinAttackRange(Vector2 playerPos) const;

	/**
	 * @brief Sends a message to the messaging system.
	 * @param id The message ID string.
	 */
	void SendMessage(const std::string& id) const;

	// ------- state data that FSM states read / write -------
	float searchTimer = 0.0f;
	bool  isBurrowed = false;
	bool  searchDone = false;  // set true by updateSearch() when 360 scan completes
	bool  atSpawn = false;  // set true by moveToSpawn() when enemy arrives at spawn

	// Basic enemy dash state � exposed so BasicAttackState can reset on Exit()
	enum class DashPhase { READY, DASHING, RESTING };
	DashPhase dashPhase = DashPhase::READY;
	float     dashRestTimer = 0.0f;
	float     dashRemaining = 0.0f;

	// -----------------------------------------------------------------------
	// Burrow enemy whack-a-mole phases, exposed so BurrowAttackState can read them.
	//
	// Phase flow (loops while player stays in range):
	//
	//   ABOVE_GROUND  � visible, approaches player
	//       | player within burrowRange && cooldown done
	//   TELEGRAPHING  � frozen, target locked at player's CURRENT pos,
	//                   "BurrowWarning" message -> renderer shows ground-crack VFX
	//       | telegraph timer expires  (~0.8 s � player's dodge window)
	//   BURROWED      � hidden underground, "BurrowUnderground" message -> hide sprite
	//       | travel timer expires  (~0.6 s)
	//   STRIKING      � emerged at target, stun window (~0.3 s, player can punish),
	//                   "BurrowStrike" or "BurrowMissed" depending on player overlap
	//       | stun timer expires
	//   COOLDOWN      � visible + slow, player counterattack window  (~2.5 s)
	//       | cooldown expires
	//   ABOVE_GROUND  -> repeat
	//
	// isBurrowed remains a convenience alias: true while phase == BURROWED.
	// BurrowAttackState uses it to suppress sight/range transitions mid-burrow.
	// -----------------------------------------------------------------------
	enum class BurrowPhase { ABOVE_GROUND, TELEGRAPHING, BURROWED, STRIKING, COOLDOWN };
	BurrowPhase burrowPhase = BurrowPhase::ABOVE_GROUND;
	float       burrowPhaseTimer = 0.0f;
	Vector2     burrowStrikeTarget = { 0.0f, 0.0f }; // locked when TELEGRAPHING starts

	// -----------------------------------------------------------------------
	// Ranged enemy combo attack phases
	//
	// The ranged enemy cycles through a 6-shot combo:
	//   Shots 1-3 : NORMAL  � single projectile aimed at player, 2 s apart
	//   Shots 4-6 : SPREAD  � 3 projectiles fired simultaneously (fan pattern)
	//                         fired in fast succession, 0.4 s apart
	// After shot 6 a longer cooldown (3 s) resets the combo to shot 1.
	//
	// Messages broadcast:
	//   "RangedNormalAttack"  � fire one projectile toward player
	//   "RangedSpreadAttack"  � fire 3-projectile fan (left / centre / right)
	// -----------------------------------------------------------------------
	enum class RangedComboPhase { NORMAL, SPREAD };
	int             rangedShotsFired = 0;   // 0-5; resets after full combo
	RangedComboPhase rangedComboPhase = RangedComboPhase::NORMAL;

	// -----------------------------------------------------------------------
	// Group awareness system
	//
	// Mobs in the same group share a group ID (set by the level/spawner).
	// The EnemyGroup registry (below) lets bless mobs look up the HP of their
	// ranged ally without needing a direct pointer.
	//
	// Usage:
	//   enemy->SetGroupId(1);                  // assign group
	//   EnemyGroup::Register(1, rangedEntity, this);  // spawner registers
	//   enemy->GetGroupRangedHp()              // bless mob queries HP
	// -----------------------------------------------------------------------
	int  groupId = -1; // -1 = no group
	bool rangedAllyAlive = true; // set false when ranged ally dies ("RangedAllyDied")

	void SetGroupId(int id) { groupId = id; }
	int  GetGroupId()  const { return groupId; }

	// Called by the game system when the group's ranged mob dies.
	// After this, bless mobs switch from healing the ally to attacking the player.
	void NotifyRangedAllyDied() { rangedAllyAlive = false; }
	bool IsRangedAllyAlive() const { return rangedAllyAlive; }

	// -----------------------------------------------------------------------
	// Bless mob state — exposed so BlessState variants can read/write it.
	//
	// MODE 1 — HEAL_ALLY (rangedAllyAlive == true):
	//   Mob continuously follows the nearest RANGED ally (position supplied
	//   by game system via RegisterBlessAlly each frame).  It only stops and
	//   channels a heal when the ally's HP is below BLESS_ALLY_HEAL_THRESHOLD
	//   (50%) AND the mob is within blessHealRange (3 tiles).  If the ally is
	//   healthy the mob simply stays close without healing.
	//   Message: "BlessHealAlly"
	//
	// MODE 2 — AOE_ATTACK (rangedAllyAlive == false):
	//   Mob freezes in place, channels for ATTACK_CAST_TIME (1.5s), then fires
	//   a non-targeted AOE bless circle centred on itself (radius 2.5 tiles).
	//   Does NOT move or track the player.  Player can absorb and mutate it.
	//   Messages: "BlessAOECharge", "BlessAOEFire", "BlessAOEHit"
	//
	// Lua translation notes:
	//   s_isCasting        -> isCastingBless
	//   s_actionTimer      -> blessActionTimer
	//   s_cooldownTimer    -> blessCooldownTimer
	//   s_allyPos          -> blessAllyPos
	//   s_allyHp           -> blessAllyHp     (0-100, percent)
	//   BLESS_HEAL_RANGE_TILES   = 3.0
	//   BLESS_AOE_RADIUS_FACTOR  = 2.5
	//   BLESS_ALLY_HEAL_THRESHOLD = 50  (heal only when ally HP% < this)
	//   HEAL_CAST_TIME    = 1.2
	//   HEAL_COOLDOWN     = 3.0
	//   ATTACK_CAST_TIME  = 1.5
	//   ATTACK_COOLDOWN   = 4.0
	//   BLESS_MOVE_SPEED  = 55.0
	// -----------------------------------------------------------------------
	bool    isCastingBless    = false;
	float   blessActionTimer  = 0.0f;
	float   blessCooldownTimer = 0.0f;
	Vector2 blessAllyPos      = { 0.0f, 0.0f }; // last known ally position
	int     blessAllyHp       = 100;             // ally HP percent (0-100); set by RegisterBlessAlly

	// Game system calls this each frame for the living ranged ally.
	// Lua: called from GameApp as AI_BlessAllyPos(x, y, hpPercent)
	void RegisterBlessAlly(Vector2 allyWorldPos, int allyHpPercent)
	{
		blessAllyPos = allyWorldPos;
		blessAllyHp  = allyHpPercent;
	}

	// -----------------------------------------------------------------------
	// Player absorbed bless ability
	//
	// When the player absorbs a "BlessAOEFire" from a bless mob, the game
	// system sets playerHasBless = true and resets all timers.
	//
	// The player has two modes, switchable with KEY_BLESS_SWITCH (default Q):
	//
	//   HEAL mode  — player fires a self-heal AOE centred on themselves.
	//                Restores HP by PLAYER_BLESS_HEAL_AMOUNT.
	//                Message: "PlayerBlessHeal"
	//
	//   ATTACK mode — player fires an AOE bless circle centred on themselves
	//                 (same as the mob's AOE, same radius).
	//                 Message: "PlayerBlessAOE"
	//
	// Both modes use KEY_BLESS_USE (default E) to activate.
	// Both have a shared cooldown (PLAYER_BLESS_COOLDOWN = 3.0s) and a
	// short cast time (PLAYER_BLESS_CAST_TIME = 0.8s).
	//
	// Key assignments (change here to remap — Lua: top-of-file constants):
	//   KEY_BLESS_SWITCH = 'Q'   — toggle between HEAL and ATTACK mode
	//   KEY_BLESS_USE    = 'E'   — activate current mode
	//
	// Messages broadcast:
	//   "PlayerBlessSwitchMode"  — player switched modes (UI feedback)
	//   "PlayerBlessCharging"    — player is holding USE (wind-up VFX)
	//   "PlayerBlessHeal"        — heal AOE fired (centred on player)
	//   "PlayerBlessAOE"         — attack AOE fired (centred on player)
	//   "PlayerBlessCooldown"    — ability entered cooldown
	//
	// Lua translation notes:
	//   s_hasBless         -> playerHasBless
	//   s_blessMode        -> playerBlessMode  ("HEAL" / "ATTACK")
	//   s_chargeTimer      -> playerBlessChargeTimer
	//   s_blessCooldown    -> playerBlessCooldownTimer
	//   KEY_BLESS_SWITCH   = "Q"   (Lua: mapped in OnUpdate key check)
	//   KEY_BLESS_USE      = "E"
	//   PLAYER_BLESS_CAST_TIME = 0.8
	//   PLAYER_BLESS_COOLDOWN  = 3.0
	//   PLAYER_BLESS_HEAL_AMOUNT = 20  (HP restored per heal cast)
	//   PLAYER_BLESS_AOE_RADIUS  = 2.5 (tiles, same as mob AOE)
	// -----------------------------------------------------------------------
	enum class PlayerBlessMode { HEAL, ATTACK };
	bool            playerHasBless          = false;
	PlayerBlessMode playerBlessMode         = PlayerBlessMode::HEAL;
	bool            playerBlessIsCharging   = false;
	float           playerBlessChargeTimer  = 0.0f;
	float           playerBlessCooldownTimer = 0.0f;


	// -----------------------------------------------------------------------
	// Final boss attack sequence
	//
	// These two enums are declared here (before their first use in the boss
	// member variables below) and are also reused by the mini-boss members
	// further down in the file.
	// Lua: s_comboPhase ("NORMAL"/"CHARGING"/"FIRING"), s_shockPhase ("NONE"/...)
	// -----------------------------------------------------------------------
	enum class ChannelerComboPhase { NORMAL, CHARGING, FIRING };
	enum class TremorShockPhase    { NONE, EMERGE, WAVE, COOL };

	// -----------------------------------------------------------------------
	// Final boss attack sequence
	//
	// Fixed rotation (loops forever):
	//   MELEE        - only used when player is within meleeTriggerRange.
	//                  Boss closes in and strikes.  If player escapes range
	//                  before a hit lands, sequence advances to OMNI_BURST.
	//                  Message: "BossMeleeAttack"
	//
	//   OMNI_BURST   - Channeler signature: 3 aimed normal shots then 8-dir
	//                  slow burst (same logic as updateMinibossChanneler).
	//                  Messages: "RangedNormalAttack", "BossOmniCharge",
	//                            "BossOmniShot_N/NE/E/SE/S/SW/W/NW"
	//
	//   SANCTIFY     - Warden signature: freeze, fire AOE bless circle, heal
	//                  all living allies within bossHealRange.
	//                  Messages: "BossSanctifyStart", "BossInvulnStart",
	//                            "BossAOEBless", "BossAOEBlessHit" (if player caught),
	//                            "BossHealAlly", "BossInvulnEnd"
	//
	//   SHOCKWAVE    - Popper signature: freeze, show all 3 pop warning spots
	//                  simultaneously, then pops close inward on player at
	//                  0.4s intervals. 4.0s cooldown after all 3 fire.
	//                  Reuses TremorShockPhase enum:
	//                    EMERGE = telegraph (show all 3 spots, 1.2s)
	//                    WAVE   = popping   (fire in sequence)
	//                    COOL   = cooldown  (vulnerable retreat)
	//                  Messages: "BossTriangleTelegraph",
	//                            "BossTrianglePop_1/2/3",
	//                            "BossTrianglePopHit_1/2/3",
	//                            "BossTrianglePopEnd"
	//
	// Ally spawning:
	//   After OMNI_BURST completes the boss broadcasts "BossSpawnRangedAlly".
	//   GameApp listens for this and spawns a RANGED mob near the boss.
	//   The boss heals allies (HP < BOSS_ALLY_HEAL_THRESHOLD) during SANCTIFY.
	//
	// Ally HP awareness:
	//   Each frame the game system must call RegisterBossAlly(pos, hp) for
	//   every living ally. The boss reads hp to decide whether to heal.
	//   bossAllyData is cleared at the start of each updateBoss call.
	//
	// Lua translation notes:
	//   s_bossPhase        -> bossSequencePhase  ("MELEE"/"OMNI_BURST"/"SANCTIFY"/"SHOCKWAVE")
	//   s_bossAbilityTimer -> bossAbilityTimer
	//   s_bossAbilityDone  -> bossAbilityDone
	//   s_omniShotsFired   -> bossOmniShotsFired
	//   s_omniComboPhase   -> bossOmniComboPhase ("NORMAL"/"CHARGING"/"FIRING")
	//   s_omniIndex        -> bossOmniBurstIndex
	//   s_sanctifyTimer    -> bossSanctifyTimer
	//   s_shockPhase       -> bossShockPhase  ("EMERGE"=telegraph/"WAVE"=popping/"COOL"=cooldown)
	//   s_shockTimer       -> bossShockTimer
	//   s_shockIndex       -> bossShockIndex  (0-2, which pop has fired)
	//   s_shockZones[i]    -> bossShockZonePos[i]  (pop world positions around player)
	// -----------------------------------------------------------------------
	enum class BossSequencePhase { MELEE, OMNI_BURST, SANCTIFY, SHOCKWAVE };
	BossSequencePhase bossSequencePhase = BossSequencePhase::MELEE;
	float             bossAbilityTimer  = 0.0f;
	bool              bossAbilityDone   = false;

	// OMNI_BURST sub-state (mirrors Channeler, reuses ChannelerComboPhase enum)
	// Lua: s_omniComboPhase / s_omniShotsFired / s_omniIndex
	int                 bossOmniShotsFired  = 0;   // 0-2 normal shots; 3 triggers burst
	ChannelerComboPhase bossOmniComboPhase  = ChannelerComboPhase::NORMAL;
	int                 bossOmniBurstIndex  = 0;   // 0-7 while FIRING

	// Compatibility alias: states.cpp references bossRangedShots directly.
	// updateBoss keeps this in sync with bossOmniShotsFired each frame.
	// Lua: s_omniShotsFired (same variable, two names in C++)
	int bossRangedShots = 0;

	// SANCTIFY sub-state
	// Lua: s_sanctifyTimer
	float bossSanctifyTimer = 0.0f;

	// SHOCKWAVE sub-state (mirrors Tremor, reuses TremorShockPhase enum)
	// Lua: s_shockPhase / s_shockTimer / s_shockIndex / s_shockCenter / s_shockZones
	TremorShockPhase bossShockPhase    = TremorShockPhase::NONE;
	float            bossShockTimer    = 0.0f;
	int              bossShockIndex    = 0;
	Vector2          bossShockCenter   = { 0.0f, 0.0f };
	Vector2          bossShockZonePos[3] = {};

	// Ally awareness: game system calls RegisterBossAlly(worldPos, currentHp)
	// each frame for every living ally. Cleared at the start of updateBoss.
	// Lua: s_allies[i] = { x, y, hp }
	struct BossAllyInfo { Vector2 pos; int hp; };
	std::vector<BossAllyInfo> bossAllyData;
	void RegisterBossAlly(Vector2 allyWorldPos, int allyHp = 999)
	{
		bossAllyData.push_back({ allyWorldPos, allyHp });
		// Keep legacy bossAllyPositions in sync for any external code still using it
		bossAllyPositions.push_back(allyWorldPos);
	}

	// Legacy ally position list — kept so existing external code still compiles.
	// Prefer bossAllyData for new code (it carries HP as well).
	std::vector<Vector2> bossAllyPositions;

	// -----------------------------------------------------------------------
	// Mini-boss: Channeler (MINIBOSS_CHANNELER)
	//
	// Combo:
	//   Shots 0-2  (NORMAL)   - single aimed projectile, 2.0s between shots
	//                           Message: "RangedNormalAttack"
	//   Shot  3    (CHARGING) - 0.6s frozen wind-up
	//                           Message: "BossOmniCharge"
	//              (FIRING)   - one SendMessage per 8-direction per frame
	//                           Messages: "BossOmniShot_N/NE/E/SE/S/SW/W/NW"
	// After burst: 4.0s cooldown, then combo resets.
	//
	// Lua translation notes:
	//   s_shotsFired      -> channelerShotsFired
	//   s_comboPhase      -> channelerComboPhase  ("NORMAL"/"CHARGING"/"FIRING")
	//   s_burstIndex      -> channelerBurstIndex  (0-7 while FIRING)
	//   s_attackTimer     -> AttackTimer
	// -----------------------------------------------------------------------
	// NOTE: ChannelerComboPhase enum is declared above the boss section so
	// it is available to both the final boss and the Channeler mini-boss.
	ChannelerComboPhase channelerComboPhase = ChannelerComboPhase::NORMAL;
	int channelerShotsFired = 0; // 0-2 normal shots; 3 triggers burst
	int channelerBurstIndex = 0; // 0-7 while FIRING, which direction has been sent

	// -----------------------------------------------------------------------
	// Mini-boss: Warden (MINIBOSS_WARDEN)
	//
	// Combo:
	//   Hits 0-2  (MELEE)    - close on player, melee strike, 1.2s gap
	//                          Message: "WardenMeleeAttack"
	//   Hit  3    (SANCTIFY) - 1.5s invulnerable channel:
	//                            open:    "WardenSanctifyStart", "WardenInvulnStart"
	//                            charge:  "WardenBlessAOECharge" (first frame of channel)
	//                            fire:    "WardenBlessAOEFire" — non-targeted AOE circle
	//                                      centred on Warden (radius 2.5 tiles).
	//                                      Identical behaviour to "BlessAOEFire".
	//                                      Player can absorb this circle.
	//                            absorb:  "WardenBlessAOEAbsorb" — sent alongside fire;
	//                                      GameApp sets playerHasBless if player absorbs.
	//                            hit:     "WardenBlessAOEHit" — if player in radius and
	//                                      did NOT absorb (GameApp resolves priority).
	//                            heal:    "WardenHeal"  (+HEAL_AMOUNT HP, capped)
	//                            close:   "WardenInvulnEnd"
	// After Sanctify: 5.0s vulnerable cooldown, then combo resets.
	//
	// Lua translation notes:
	//   s_meleeHits       -> wardenMeleeHits
	//   s_wardenPhase     -> wardenPhase  ("MELEE"/"SANCTIFY"/"COOLDOWN")
	//   s_actionTimer     -> wardenActionTimer
	//   s_hp              -> wardenHp
	//   MELEE_RANGE_TILES       = 2.0
	//   APPROACH_SPEED          = 110.0
	//   MELEE_HIT_DELAY         = 1.2
	//   SANCTIFY_DURATION       = 1.5
	//   SANCTIFY_COOLDOWN       = 5.0
	//   BLESS_AOE_RADIUS_FACTOR = 2.5  (same constant as bless mob)
	//   HEAL_AMOUNT             = 5
	//   WARDEN_MAX_HP           = 30
	// -----------------------------------------------------------------------
	enum class WardenPhase { MELEE, SANCTIFY, COOLDOWN };
	WardenPhase wardenPhase       = WardenPhase::MELEE;
	int         wardenMeleeHits   = 0;     // 0-2; reaching 3 triggers Sanctify
	float       wardenActionTimer = 0.0f;  // Sanctify channel / cooldown timer
	int         wardenHp          = 30;    // separate HP pool for self-heal tracking

	// -----------------------------------------------------------------------
	// Mini-boss: Popper (MINIBOSS_POPPER)
	//
	// A large burrow-type mini-boss with a wide hitbox.
	//
	// Combo:
	//   Burrows 0-2  (BASIC phase) — standard burrow cycle identical to the
	//                regular burrow mob (same BurrowPhase machine, same messages).
	//                Hit radius is wider: POPPER_HIT_RADIUS = 1.5 * tileSize.
	//
	//   Burrow  3    (TRIANGLE_POP signature):
	//     Step 1 — TELEGRAPH: all 3 pop positions shown simultaneously as
	//              ground-crack warnings ("PopperTriangleTelegraph").
	//              The 3 spots form a triangle around the player at radii
	//              3.0 / 2.0 / 1.0 tiles (large -> medium -> small), each
	//              offset 120 degrees apart.  Player has POPPER_TELEGRAPH_DUR
	//              (1.2s) to read the pattern and find the gap.
	//     Step 2 — POPPING: pops emerge in sequence at 0.4s intervals.
	//              Each pop checks the player against POPPER_POP_RADIUS (1.5 * tileSize).
	//              Messages per pop: "PopperPop_1/2/3", "PopperPopHit_1/2/3"
	//     Step 3 — COOLDOWN: Popper surfaces at its last position, fully
	//              vulnerable for POPPER_COOLDOWN (4.0s).
	//              Message: "PopperVulnerable", "PopperCooldownEnd"
	//
	// Skill test: player reads all 3 warning spots shown simultaneously and
	//             picks the gap between them before the sequence closes in.
	//
	// Messages (basic burrow cycle — same as updateBurrow):
	//   "BurrowWarning", "BurrowUnderground", "BurrowStrike"/"BurrowMissed",
	//   "BurrowVulnerable", "BurrowInvulnerable"
	//
	// Lua translation notes:
	//   s_burrowCount      -> popperBurrowCount   (0-2 basic, 3 = triangle pop)
	//   s_popPhase         -> popperPhase         ("BASIC"/"TELEGRAPH"/"POPPING"/"COOLDOWN")
	//   s_popTimer         -> popperPhaseTimer
	//   s_popIndex         -> popperPopIndex      (0-2, which pop has fired)
	//   s_popPositions[i]  -> popperPopPos[i]     (world pos of each pop)
	// -----------------------------------------------------------------------
	enum class PopperPhase { BASIC, TELEGRAPH, POPPING, COOLDOWN };
	PopperPhase popperPhase      = PopperPhase::BASIC;
	int         popperBurrowCount = 0;    // 0-2 basic burrows; 3 triggers triangle pop
	float       popperPhaseTimer  = 0.0f; // telegraph duration / pop interval / cooldown
	int         popperPopIndex    = 0;    // 0-2, which pop in the sequence has fired
	Vector2     popperPopPos[3]   = {};   // world positions of the 3 triangle pop spots


private:
	MobType    mobType;
	Vector2    position;
	Vector2    spawnPos;
	int        patrolRange;
	int        visionRange;
	int        searchPhase = 0;
	StateType  currentState;
	Direction  currentDirection;
	Direction  patrolDirection;
	Vector2    LastKnownPlayerPos;
	bool       HasLastSeenPlayer = false;
	bool       movingToTile = false;
	Vector2    currentTargetGrid;
	float      AttackTimer = 0.0f;

	int   bossCurrentPhase = 0;
	float bossPhaseTimer = 0.0f;

	float tileSize = 100.0f;

	// Movement parameters
	const float patrolSpeed = 100.0f;
	const float chaseSpeed = 150.0f;
	const float enemyFriction = 600.0f;
	const float enemyMaxSpeed = 70.0f;

	// Ranged enemy parameters
	const float rangeSpeed = 80.0f;
	const float rangedAtkTimer = 2.0f;

	// Burrow enemy parameters
	const float burrowSpeed = 80.0f;
	const float burrowRange = 2.0f * tileSize;

	// Heal enemy parameters
	const float healMovSpeed = 60.0f;
	const float attackRange = 1.5f * tileSize;
	const float healRange = 2.5f * tileSize;
	const float actionDuration = 1.0f;
	const float cooldownDuration = 2.0f;

	const float checkOffset = 5.0f;
	const float searchSpeed = 40.0f;
	const float visionTiles = 3.0f;

	SpeedComponent* enemyspd = nullptr;

	Messaging::Observable* messageHub = nullptr;

	int hp = 0;
	int damage = 0;

	float searchDuration = 2.0f;

	// Basic enemy dash parameters
	const float dashSpeed = 400.0f;
	const float dashRestDuration = 2.0f;
	const int   dashTiles = 3;

	// dashDir is private � only updateBasic needs it
	Vector2 dashDir = { 0, 0 };

	// Burrow state: managed by burrowPhase / burrowPhaseTimer / burrowStrikeTarget (public)

	// Heal state variables
	bool  isHealing = false;
	float actionTimer = 0.0f;
	float cooldownTimer = 0.0f;

	void SnapToGrid(float tilesize);

	std::vector<std::string> map;

	void ChangeState(StateType newState);
	void ChangeDirection(Direction newDirection);

	bool isBlocked(const Vector2& nextPos)                            const;
	bool isBlockedGrid(const Vector2& gridPos)                            const;
	bool wouldOverlapPlayer(const Vector2& nextPos, const Vector2& playerPos) const;
	bool isAtSpawn(const Vector2& pos);

	std::unique_ptr<FSM> fsm;
};

// Coordinate conversion helpers
Vector2 WorldToGrid(const Vector2& pos, float tileSize);
Vector2 GridToWorld(const Vector2& pos, float tileSize);