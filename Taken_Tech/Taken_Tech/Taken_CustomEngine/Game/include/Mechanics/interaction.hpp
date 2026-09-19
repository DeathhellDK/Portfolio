/**
* @file     interaction.hpp
* @author   Tan Wei Liang Terril
* @co-author Jethro Sung
* @email    t.weiliangterril, sung.h
* @date     2026-02-01
*
* @brief    Handles gameplay ability effects and interactions between entities.
*
* This file defines the Interaction class, which provides static
* utility functions for applying ability-related effects such as damage,
* healing, burrow state changes, and projectile actions.
*
* @version 1.1
*
* @version update
* @version 1.0 - Add ability handling functions for damage, healing, burrow, and projectiles.
* @version 1.1 - Add cooldown management, mutation and boss corpse tracking for abilities.
*
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#pragma once
#include "Core/entitymanager.h"
#include <unordered_map>
#include "Core/gameApp.h"

static constexpr float DEFAULT_COOLDOWN = 0.4f;

class GameApp;

/**
 * @class Interaction
 * @brief Centralized handler for player and entity ability effects.
 */
class Interaction {
public:
	/// Maps entities to their remaining burrow duration
	static std::unordered_map<Entity, float> burrowTimers;
	/// Maps entities to their burrow active state
	static std::unordered_map<Entity, bool>  burrowActiveMap;

	/// cooldown for burrow
	static std::unordered_map<Entity, float> burrowCooldowns;

	/// cooldown for projectile
	static std::unordered_map<Entity, float> projectileCooldowns;

	/// cooldown for melee attack
	static std::unordered_map<Entity, float> meleeCooldowns;

	/// Map to track which entities are boss corpses
	static std::unordered_map<Entity, bool>  bossCorpses;

	/// Stores the corpse entity that should show the first ability absorption prompt.
	static Entity firstAbsorbCorpse;

	///Tracks if the first ability absorption prompt has been triggered.
	static bool firstAbsorbPromptTriggered;

	/// cooldown for mutation charge up
	static std::unordered_map<Entity, float> mutationchargeCooldowns;

	/// cooldown for mutation damage
	static std::unordered_map<Entity, float> mutationDmgCooldowns;

	/// cooldown lock for mutation purification (prevents re-purifying immediately after purifying)
	static std::unordered_map<Entity, float> mutationPurifyLock;

	/// cooldown for damage on player
	static std::unordered_map<Entity, float> damageCoolDown;

	/// cooldown for healing on player
	static std::unordered_map<Entity, float> healingCoolDown;

	//static std::unordered_map<Entity, float> mutationPurifyCooldowns; /// Tempo feature

	/// ==== Functions ------////


	/**
	* @brief Get the cooldown time remaining for a specific player ability.
	* @param ability The player ability to check cooldown for.
	*/
	static float SetAbilityCooldown(PlayerAbility ability);

	/**
	* @brief Apply damage effect to a target entity.
	* @param app Reference to the GameApp context.
	* @param target Reference to the target entity to damage.
	* @param damage Amount of damage to inflict.
	*/
	static void HandlePlayerDamage(GameApp& app, Entity& target, int damage);

	/**
	* @brief Apply healing effect to a target entity.
	* @param app Reference to the GameApp context.
	* @param target Pointer to the target entity to heal.
	* @param amount Amount of health to restore.
	*/
	static void ApplyHealing(GameApp& app, Entity target, int amount);

	/**
	* @brief Check and handle player health state.
	* @param app Reference to the GameApp context.
	* @param target Reference to the target entity (player).
	*/
	static void healthcontrol(GameApp& app, Entity& target);

	/**
	* @brief Initiate burrow effect on a target entity.
	* @param app Reference to the GameApp context.
	* @param target Pointer to the target entity to burrow.
	*/
	static void StartBurrow(GameApp& app, Entity target);

	/**
	* @brief Update burrow state for a target entity over time.
	* @param app Reference to the GameApp context.
	* @param target Pointer to the target entity to update.
	* @param dt Delta time since last update.
	*/
	static void UpdateBurrow(GameApp& app, Entity target, float dt);

	/**
	* @brief Handle projectile action for abilities.
	* @param app Reference to the GameApp context.
	* @param target Pointer to the target entity performing the projectile action.
	*/
	static void Projectile(GameApp& app, Entity target);
	/**
	* @brief Update cooldown timers for all entities and abilities.
	* @param dt Delta time since last update.
	*/
	static void UpdateCooldowns(float dt);

	/**
	* @brief Handle player input related to abilities and interactions.
	* @param app Reference to the GameApp context.
	* @param player Entity ID of the player character.
	* @param dt Delta time since last update.
	*/
	static void HandlePlayerAbilityInput(GameApp& app, Entity player, double dt);

	/**
	* @brief Handle melee attack action for abilities.
	* @param app Reference to the GameApp context.
	* @param player Entity ID of the player character performing the melee attack.
	*/
	static void MeleeAttack(GameApp& app, Entity player);

	/**
	* @brief Handle ability absorption from nearby corpses.
	* @param app Reference to the GameApp context.
	* @param player Entity ID of the player character attempting to absorb an ability.
	*/
	static void AbsorbAbilityFromCorpse(GameApp& app, Entity player);

	/**
	* @brief Notifies Interaction that an absorbable corpse was created for the first absorb prompt.
	* @param app Reference to the GameApp context.
	* @param corpse Entity ID of the corpse.
	*/
	static void NotifyFirstAbsorbCorpseCreated(GameApp& app, Entity corpse);

	/**
	* @brief Returns the corpse entity that should show the first ability absorption prompt.
	* @return Entity ID or INVALID_ENTITY.
	*/
	static Entity GetFirstAbsorbCorpse();

	/**
	* @brief Clears the stored first absorb corpse if it matches the provided entity.
	* @param corpse Entity ID that was removed or absorbed.
	*/
	static void ClearFirstAbsorbCorpseIfMatch(Entity corpse);

	/**
	* @brief Check if the player can pick up a key and handle the pickup logic.
	* @param app Reference to the GameApp context.
	* @param player Entity ID of the player character attempting to pick up a key.
	*/
	static void CheckKeyPickup(GameApp& app, Entity player);
	/**
	* @brief Mark an entity as a boss corpse for tracking purposes.
	* @param e Entity ID of the entity to mark as a boss corpse.
	*/
	static void MarkAsBossCorpse(Entity e);
	/**
	* @brief Check if an entity is marked as a boss corpse.
	* @param e Entity ID of the entity to mark as a boss corpse.
	*/
	static bool IsBossCorpse(Entity e);
	/**
	* @brief Remove an entity from the boss corpse tracking map.
	* @param e Entity ID of the entity to remove from boss corpse tracking.
	*/
	static void RemoveBossCorpse(Entity e);

	/**
	* @brief Clear all entries from the boss corpse tracking map.
	*/
	static void ClearBossCorpses();

	/**
	* @brief Handle mutation-related logic for the player, including charge-up and damage effects.
	* @param app Reference to the GameApp context.
	* @param player Entity ID of the player character.
	* @param dt Delta time since last update.
	*/
	static void HandleMutationChargeUp(GameApp& app, Entity player, float dt);

	/**
	* @brief Handle mutation damage effect for the player.
	* @param app Reference to the GameApp context.
	* @param player Entity ID of the player character.
	* @param dt Delta time since last update.
	*/
	static void HandleMutationDamage(GameApp& app, Entity player, float dt);

	/**
	* @brief Handle mutation purification logic for the player.
	* @param app Reference to the GameApp context.
	* @param player Entity ID of the player character.
	*/
	static void HandleMutationPurification(GameApp& app, Entity player);

	/*
	* @brief Get the remaining cooldown time for a specific ability on a target entity.
	* @param target The entity to check cooldown for.
	* @param ability The player ability to check cooldown for.
	*/
	static float GetAbilityCooldown(Entity target, PlayerAbility ability);

	/*
	* @brief Get the cooldown ratio (remaining cooldown / total cooldown) for a specific ability on a target entity.
	* @param target The entity to check cooldown for.
	* @param ability The player ability to check cooldown for.
	*/
	static float GetAbilityCooldownRatio(Entity target, PlayerAbility ability);
};
