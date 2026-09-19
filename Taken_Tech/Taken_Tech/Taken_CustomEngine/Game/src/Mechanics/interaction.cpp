/**
* @file     interaction.cpp
* @author   Tan Wei Liang Terril
* @co-author Low Jianlin, Jethro Sung
* @email    t.weiliangterril, jianlin.low, sung.h
* @date     2026-02-01
*
* @brief    Handles gameplay ability effects and interactions between entities.
*
* This file implements the Interaction class, which provides static
* utility functions for applying ability-related effects such as damage,
* healing, burrow state changes, and projectile actions.
*
* @version 1.0
*
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
#pragma once
#include "Mechanics/interaction.hpp"
#include "Input/DebugConsole.hpp"
#include "Core/Systems/collisionSystem.h"
#include "Core/randomiser.h"
#include "World/mapGenerator.h"
#include <algorithm>

/// == Ability Cooldonw Maps == ///
std::unordered_map<Entity, float> Interaction::damageCoolDown{};
std::unordered_map<Entity, float> Interaction::healingCoolDown{};

/// == Burrow Map == ///
std::unordered_map<Entity, float> Interaction::burrowTimers{};
std::unordered_map<Entity, bool>  Interaction::burrowActiveMap{};
std::unordered_map<Entity, float> Interaction::burrowCooldowns{};

std::unordered_map<Entity, float> Interaction::projectileCooldowns{};
std::unordered_map<Entity, float> Interaction::meleeCooldowns{};
std::unordered_map<Entity, bool> Interaction::bossCorpses{};
Entity Interaction::firstAbsorbCorpse = INVALID_ENTITY;
bool Interaction::firstAbsorbPromptTriggered = false;

/// == Mutation Map == ///
std::unordered_map<Entity, float> Interaction::mutationchargeCooldowns{};
std::unordered_map<Entity, float> Interaction::mutationDmgCooldowns{};
std::unordered_map<Entity, float> Interaction::mutationPurifyLock{};

/*
* @brief Mark an entity as a boss corpse for tracking purposes.
*/
void Interaction::MarkAsBossCorpse(Entity e) {
    /// Mark the entity as a boss corpse in the tracking map
    bossCorpses[e] = true;
}

/*
* @brief Check if an entity is marked as a boss corpse.
*/
bool Interaction::IsBossCorpse(Entity e) {
    /// Check if the entity exists in the boss corpse tracking map
    return bossCorpses.find(e) != bossCorpses.end();
}

/*
* @brief Remove an entity from the boss corpse tracking map.
* @param e Entity ID of the entity to remove from boss corpse tracking.
*/
void Interaction::RemoveBossCorpse(Entity e) {
    /// Remove the entity from the boss corpse tracking map if it exists
    bossCorpses.erase(e);
}

/**
* @brief Checks if a prefab tag grants an ability when absorbed.
* @param tag Prefab tag string.
* @return True when the tag is an ability granting enemy type.
*/
static bool IsAbsorbablePrefabTagLocal(const std::string& tag){
    return tag == "ranged_mini_boss" ||
        tag == "burrow_mini_boss" ||
        tag == "heal_mini_boss";
}

/**
* @brief Stores the first absorb corpse entity for the first absorb prompt.
* @param app Reference to the GameApp context.
* @param corpse Entity ID of the corpse.
*/
void Interaction::NotifyFirstAbsorbCorpseCreated(GameApp& app, Entity corpse){
    if (firstAbsorbPromptTriggered) return;
    if (!corpse) return;
    const std::string tag = app.GetPrefabTag(corpse);
    if (!IsAbsorbablePrefabTagLocal(tag)) return;
    firstAbsorbCorpse = corpse;
    firstAbsorbPromptTriggered = true;
}

/**
* @brief Returns the corpse entity that should show the first ability absorption prompt.
* @return Entity ID or INVALID_ENTITY.
*/
Entity Interaction::GetFirstAbsorbCorpse(){
    return firstAbsorbCorpse;
}

/**
* @brief Clears the stored first absorb corpse when it matches the provided entity.
* @param corpse Entity ID that was removed or absorbed.
*/
void Interaction::ClearFirstAbsorbCorpseIfMatch(Entity corpse){
    if (firstAbsorbCorpse == corpse) firstAbsorbCorpse = INVALID_ENTITY;
}

/*
* @brief Clear all entries from the boss corpse tracking map.
*/
void Interaction::ClearBossCorpses() {
    bossCorpses.clear();
}

/*
* @brief Get the cooldown time for a specific player ability.
*/
float Interaction::SetAbilityCooldown(PlayerAbility ability)
{
	/// Return cooldown time based on the ability type
    switch (ability)
    {
    case PlayerAbility::HEAL:
        return 1.5f;
    case PlayerAbility::BURROW:
        return 2.5f;
    case PlayerAbility::PROJECTILE:
        return 0.2f;
    default:
        return DEFAULT_COOLDOWN;
    }
}

/*
* @brief Apply healing effect to a target entity.
* @param app Reference to the GameApp context.
* @param target Pointer to the target entity to heal.
* @param amount Amount of health to restore.
* @param key Identifier for ability type/source.
*/
void Interaction::ApplyHealing(GameApp& app, Entity target, int amount)
{
    /// Check if it is validate target
    if (!target) {
        //DebugConsole::Get().Warning("Apply Healing called with null target.");
        return;
    }


    auto* pc = app.GetController(target);
    if (!pc) {
        return;
    }

    if (healingCoolDown[target] > 0.0f) {
        return;
    }

    int hp = pc->getPlayerHp();
    /// Prevent overhealing or healing when at 0 HP
    if (hp >= 100 || hp <= 0) {
        //DebugConsole::Get().Warning("Healing failed: Cannot heal beyond max HP or when HP is 0.");
        return;
    }

    /// Calculate new HP, ensuring it does not exceed 100
    int newHp = std::min(100, hp + amount);
    pc->setPlayerHp(newHp);
    pc->PlayHealAnim();
    ResourceManager::PlaySfx("Heal", 0.9f);
    /*DebugConsole::Get().Success("Player healed for " + std::to_string(amount) + "HP");
    DebugConsole::Get().Info("Player HP: " + std::to_string(hp) + " -> " + std::to_string(newHp));*/
    healingCoolDown[target] = SetAbilityCooldown(PlayerAbility::HEAL);


}

/*
* @brief Apply damage effect to a target entity.
* @param app Reference to the GameApp context.
* @param target Reference to the target entity to damage.
* @param damage Amount of damage to inflict.
* @param key Identifier for ability type/source.
*/
void Interaction::HandlePlayerDamage(GameApp& app, Entity& target, int damage)
{
    auto* pc = app.GetController(target);
    if (!pc) return;

    /// If cooldown is active, do not apply damage
    if (damageCoolDown[target] > 0.0f) return;

    if (pc->godmode == false) {
        int oldHp = pc->getPlayerHp();

        int newHp = std::max(0, static_cast<int>(oldHp - damage));

        if (oldHp > 0.0)
            PlayRandomPlayerGruntSfx(0.25f);
        //PlayRandomProjectileShotSfx(3.0f); -> moved to projectileSystem.cpp
        pc->setPlayerHp(newHp);
        pc->PlayDamageAnim();
        damageCoolDown[target] = 0.2f;
    }
    else
    {
        return;
    }


}

/*
* @brief Check and handle player health state.
* @param app Reference to the GameApp context.
* @param target Reference to the target entity (player).
*/
void Interaction::healthcontrol(GameApp& app, Entity& target)
{
    auto* pc = app.GetController(target);
    if (!pc) return;

    if (pc->getPlayerHp() <= 0) {
        app.OnPlayerDeath();
        return;
    }
}

/*
* @brief Initiate burrow effect on a target entity.
* @param app Reference to the GameApp context.
* @param target Pointer to the target entity to burrow.
* @param key Identifier for ability type/source.
*/
void Interaction::StartBurrow(GameApp& app, Entity target)
{
    /// Check if it is validate target
    if (!target) return;

    /// Check if burrow is on cooldown
    if (burrowCooldowns[target] > 0.0f)
    {
        return;
    }

    Collider* col = app.GetCollider(target);
    auto* pc = app.GetController(target);
    if (!col) return;

    /// Start burrow only if not already active
    if (!burrowActiveMap[target]) {
        /// Set burrow duration to 2 seconds
        burrowTimers[target] = SetAbilityCooldown(PlayerAbility::BURROW);
        /// Mark burrow as active
        burrowActiveMap[target] = true;
        /// Set collider to trigger to avoid physical collisions
        col->isBurrowed = true;
        pc->isUnderground = true;
        pc->PlayBurrowAnim();
        ResourceManager::PlaySfx("Burrow_In", 0.8f);
        burrowCooldowns[target] = 2.5f;

        DebugConsole::Get().Info("Burrow Started for 2.5 seconds");
    }

}

/*
* @brief Update burrow state for a target entity over time.
* @param app Reference to the GameApp context.
* @param target Pointer to the target entity to update.
* @param dt Delta time since last update.
* @param key Identifier for ability type/source.
*/
void Interaction::UpdateBurrow(GameApp& app, Entity target, float dt)
{
    if (!target) return;

    /// If burrow is active, update the timer and check for thresholds to trigger effects
    if (burrowActiveMap[target]) {
        auto* pc = app.GetController(target);
        float& timer = burrowTimers[target];
        float prevTimer = timer;
        timer -= dt;
        //DebugConsole::Get().Info("Burrow Duration Left: " + std::to_string(burrowTimers[target]) + " seconds");

        // --- 1. MUFFLE THRESHOLD (2.3s) ---
        // Triggers 0.2s after StartBurrow, letting half of "Burrow_In" play clear.
        if (prevTimer > 2.3f && timer <= 2.3f) {
            ResourceManager::SetLPFilter(true, 150);
        }

        // 2. Play Burrow_Out while still muffled (0.4s before end)
        if (prevTimer > 0.4f && timer <= 0.4f) {
            ResourceManager::PlaySfx("Burrow_Out", 0.68f);
        }

        if (timer <= 0.0f) {
            timer = 0.0f;
            burrowActiveMap[target] = false;

            // 3. Clear the filter as the player emerges
            ResourceManager::SetLPFilter(false, 200);

            Collider* col = app.GetCollider(target);
            if (col) col->isBurrowed = false;
            pc->isUnderground = false;
            DebugConsole::Get().Info("Burrow Ended");
        }
    }

    // Update cooldown
    /*if (burrowCooldowns[target] > 0.0f)
    {
        burrowCooldowns[target] -= dt;
        if (burrowCooldowns[target] < 0.0f)
            burrowCooldowns[target] = 0.0f;
    }*/

}

/**
 * @brief Handle projectile action for abilities.
 * 
 * Checks cooldowns, calculates aim direction from mouse position, and requests
 * the PlayerController to fire a projectile.
 * 
 * @param app Reference to the GameApp context.
 * @param target The entity firing the projectile (usually the player).
 */
void Interaction::Projectile(GameApp& app, Entity target)
{
    if (!target) return;

    if (projectileCooldowns[target] > 0.0f)
    {
        return;
    }

    PlayerController* pc = app.GetController(target);
    if (!pc) return;

    Transform* pt = app.GetTransform(target);
    if (!pt) {
        return;
    }

    Vector2 playerPos = pt->GetPosition();
    Vector2 playerScale = pt->GetScale();
    /// Calculate the center of the player for accurate shooting direction
    Vector2 playerCenter = { playerPos.x + playerScale.x * 0.5f, playerPos.y + playerScale.y * 0.5f };

    Vector2 shootDir{ 0.0f, 0.0f };

    if (eng::input().isGamepadConnected())
    {
        Vector2 stick = eng::input().getGamepadRightStick();
        stick.y *= -1.0f;
        if (stick.LengthSqd() > 0.0f)
            shootDir = stick;
    }

    if (shootDir.LengthSqd() <= 0.0f)
    {
        Vector2 worldMousePos = app.GetWorldMousePosition();
        shootDir = worldMousePos - playerCenter;
    }

    /// Normalize and shoot only if direction is valid
    if (shootDir.LengthSqd() > 0.0f)
    {
        /// Request shoot from player controller
        pc->RequestShoot(shootDir.Normalized());
    }

    /// Set projectile cooldown to 0.8 seconds
    projectileCooldowns[target] = SetAbilityCooldown(PlayerAbility::PROJECTILE);
}

/*
* @brief Helper function to check AABB overlap between two rectangles defined by position and size.
* @param aPos Position of the first rectangle.
* @param aSize Size of the first rectangle.
* @param bPos Position of the second rectangle.
* @param bSize Size of the second rectangle.
*/
static bool AabbOverlapLocal(const Vector2& aPos, const Vector2& aSize,
    const Vector2& bPos, const Vector2& bSize);

/*
* @brief Handle ability absorption from a corpse entity.
* @param app Reference to the GameApp context.
* @param player Entity ID of the player character attempting to absorb an ability.
*/
void Interaction::AbsorbAbilityFromCorpse(GameApp& app, Entity player)
{
    if (!player)
        return;

    PlayerController* pc = app.GetController(player);
    Transform* pt = app.GetTransform(player);
    if (!pc || !pt)
        return;

    const Vector2 pPos = pt->GetPosition();
    const Vector2 pSize = pt->GetScale();

    const auto& allColliders = app.GetAllColliders();

    Entity absorbed = 0;
    std::string prefab;

    /// Check for overlapping colliders that are valid corpse entities with abilities to absorb
    for (const auto& [other, otherCol] : allColliders)
    {
        /// Skip if collider is null or belongs to the player
        if (!other || other == player)
            continue;

        /// Skip if no collider component (shouldn't happen since we're iterating colliders, but just in case)
        if (!otherCol)
            continue;

        /// Skip if the entity is not an enemy (we only want to absorb from enemy corpses)
        if (app.GetEnemyController(other))
            continue;

        /// Only want to absorb from specific mini-boss types that grant abilities upon death
        const std::string tag = app.GetPrefabTag(other);
        if (tag != "ranged_mini_boss" &&
            tag != "EnemyContact" &&
            tag != "burrow_mini_boss" &&
            tag != "heal_mini_boss")
        {
            continue;
        }

        Transform* et = app.GetTransform(other);
        if (!et) {
            continue;
        }

        Vector2 ePos = et->GetPosition();
        Vector2 eSize = et->GetScale();

        /// Adjust size based on collider type for more accurate overlap checking
        if (otherCol->type == ColliderType::Box) {
            eSize = otherCol->size;
        }
        /// For circle colliders, we can treat the size as a diameter for AABB purposes
        else if (otherCol->type == ColliderType::Circle) {
            eSize = Vector2(otherCol->size.x * 2.0f, otherCol->size.x * 2.0f);
        }

        /// Check for AABB overlap between player and corpse entity
        if (AabbOverlapLocal(pPos, pSize, ePos, eSize))
        {
            absorbed = other;
            prefab = tag;
            break;
        }
    }

    if (!absorbed)
        return;

    PlayerAbility ability = PlayerAbility::DEFAULT;

    /// Determine which ability to grant based on the prefab tag of the absorbed corpse
    if (prefab == "ranged_mini_boss") {
        ability = PlayerAbility::PROJECTILE;
    }
    else if (prefab == "burrow_mini_boss") {
        ability = PlayerAbility::BURROW;
    }
    else if (prefab == "heal_mini_boss") {
        ability = PlayerAbility::HEAL;
    }

    /// If a valid ability was determined, grant it to the player and handle key drops if it's a boss corpse
    if (ability != PlayerAbility::DEFAULT)
    {
        pc->setMutationLevel(pc->getMutationLevel() * 3/4); // Reset mutation level on ability absorption
        const PlayerAbility prevAbility = pc->GetAbility();
        pc->SetAbility(ability);
        if (ability == PlayerAbility::PROJECTILE && prevAbility != PlayerAbility::PROJECTILE) {
            pc->PlayProjectileTransformAnim();
        }
        else {
            pc->PlayAbsorbAnim();
        }
        PlayRandomAbilityAbsorbSfx(1.0f);
        //DebugConsole::Get().Info("Player absorbed ability from corpse\n");

        /// If the absorbed entity is a mini-boss that grants an ability, check if we should drop a key for the player
        if (prefab == "ranged_mini_boss" ||
            prefab == "burrow_mini_boss" ||
            prefab == "heal_mini_boss")
        {
            if (Transform* t = app.GetTransform(absorbed))
            {
                std::string keyPrefab = "Key";
                PlayerAbility keyAbilityToCheck = PlayerAbility::DEFAULT;

                /// Drop key corresponding to the absorbed ability, but only if the player doesn't already have that type of key
                if (prefab == "ranged_mini_boss") {
                    keyPrefab = "Key_Projectile";
                    keyAbilityToCheck = PlayerAbility::PROJECTILE;
                }
                else if (prefab == "burrow_mini_boss") {
                    keyPrefab = "Key_Burrow";
                    keyAbilityToCheck = PlayerAbility::BURROW;
                }
                else if (prefab == "heal_mini_boss") {
                    keyPrefab = "Key_Heal";
                    keyAbilityToCheck = PlayerAbility::HEAL;
                }

                // Only spawn if the player does not already have this type of key
                if (!pc->HasKeyType(keyAbilityToCheck)) {
                    // Check if it's a boss corpse before dropping a key
                    if (IsBossCorpse(absorbed)) {
                        app.InstantiatePrefab(keyPrefab, t->GetPosition());
                    }
                    /* else {
                         DebugConsole::Get().Info("Not a boss, no key dropped.");
                     }*/
                }
                /*else {
                    DebugConsole::Get().Info("Player already has " + keyPrefab + ", skipping drop.");
                }*/
            }
        }
    }

    ClearFirstAbsorbCorpseIfMatch(absorbed);
    app.DestroyEntityNoExpose(absorbed);
}

/**
 * @brief Helper function to check AABB overlap between two rectangles.
 * 
 * @param aPos Position of the first rectangle (top-left).
 * @param aSize Size of the first rectangle.
 * @param bPos Position of the second rectangle (top-left).
 * @param bSize Size of the second rectangle.
 * @return True if the rectangles overlap.
 */
static bool AabbOverlapLocal(const Vector2& aPos, const Vector2& aSize,
    const Vector2& bPos, const Vector2& bSize)
{
    const float aMinX = aPos.x;
    const float aMaxX = aPos.x + aSize.x;
    const float aMinY = aPos.y;
    const float aMaxY = aPos.y + aSize.y;

    const float bMinX = bPos.x;
    const float bMaxX = bPos.x + bSize.x;
    const float bMinY = bPos.y;
    const float bMaxY = bPos.y + bSize.y;

    return (aMinX < bMaxX) && (aMaxX > bMinX) && (aMinY < bMaxY) && (aMaxY > bMinY);
}

/*
* @brief Handle melee attack action for abilities.
* @param app Reference to the GameApp context.
* @param player Entity ID of the player character performing the melee attack.
*/
void Interaction::MeleeAttack(GameApp& app, Entity player)
{
    if (!player) return;

    // If currently burrowed, don't allow melee (player is underground / intangible).
    if (burrowCooldowns[player] > 0.0f) {
        return;
    }

    // Cooldown gate
    if (meleeCooldowns[player] > 0.0f) {
        return;
    }

    PlayerController* pc = app.GetController(player);
    Transform* pt = app.GetTransform(player);
    if (!pc || !pt) return;
    if (pc->IsAttackInputSuppressed()) return;

    pc->PlayMeleeAnim();
    PlayRandomAtkSfx(0.40f);

    // Attack box in front of the player top-left coords
    const Vector2 pPos = pt->GetPosition();
    const Vector2 pSize = pt->GetScale();

    constexpr float kRangeX = 65.0f;  // forward reach
    const float hitW = kRangeX;
    const float hitH = std::max(40.0f, pSize.y * 0.55f);

    Vector2 hitPos;
    hitPos.x = pc->IsFacingRight() ? (pPos.x + pSize.x) : (pPos.x - hitW);
    hitPos.y = pPos.y + (pSize.y - hitH) * 0.5f;

    const Vector2 hitSize{ hitW, hitH };
    int dmg;

    if (pc->godmode == true) {
        dmg = 9999;
    }
    else {
        dmg = pc->getPlayerMeleeDmg(); // instant kill 
    }
    
    bool hitSomething = false;

    const auto& allColliders = app.GetAllColliders();

    /// Check for enemies overlapping with the melee hitbox and apply damage if hit
    for (const auto& [other, otherCol] : allColliders)
    {
        if (!other || other == player) {
            continue;
        }

        if (!app.GetEnemyController(other)) {
            continue;
        }

        Transform* et = app.GetTransform(other);
        if (!et) {
            continue;
        }

        Vector2 ePos = et->GetPosition();
        Vector2 eSize = et->GetScale();

        // Use collider size if available
        if (otherCol)
        {
            if (otherCol->type == ColliderType::Box) {
                eSize = otherCol->size;
            }

            else if (otherCol->type == ColliderType::Circle) {
                eSize = Vector2(otherCol->size.x * 2.0f, otherCol->size.x * 2.0f);
            }
        }

        /// Check for AABB overlap between the melee hitbox and the enemy's collider
        if (AabbOverlapLocal(hitPos, hitSize, ePos, eSize))
        {
            if (EnemiesController* enemy = app.GetEnemyController(other))
            {
                if (enemy->TakeDamage(dmg))
                {
                    const std::string bakedName = app.GetEntityNameByEntity(other);
                    /// Mark the enemy as defeated in the MapGenerator so it doesn't respawn on re-entry
                    MapGenerator::MarkEnemyDefeatedByName(bakedName);
                    app.OnEnemyDeath(other);
                    NotifyFirstAbsorbCorpseCreated(app, other);
                }
                else {
                    PlayRandomEnemyDamaged(app, enemy->GetMobType(), enemy->IsBoss(), glm::vec2(ePos.x, ePos.y));
                }
                hitSomething = true;
            }

            break;
        }
    }

    // Cooldown (seconds)
    meleeCooldowns[player] = DEFAULT_COOLDOWN;
}

/*
* @brief Update cooldown timers for all abilities.
* @param dt Delta time since last update.
*/
void Interaction::UpdateCooldowns(float dt)
{
    for (auto& [entity, cd] : damageCoolDown)
    {
        if (cd > 0.0f)
            cd = std::max(0.0f, cd - dt);
    }

    for (auto& [entity, cd] : healingCoolDown)
    {
        if (cd > 0.0f)
            cd = std::max(0.0f, cd - dt);
    }

    for (auto& [entity, cd] : projectileCooldowns)
    {
        if (cd > 0.0f)
            cd = std::max(0.0f, cd - dt);
    }

    for (auto& [entity, cd] : meleeCooldowns)
        if (cd > 0.0f) cd = std::max(0.0f, cd - dt);

    for (auto& [entity, cd] : burrowCooldowns)
    {
        if (cd > 0.0f)
            cd = std::max(0.0f, cd - dt);
    }

    for (auto& [entity, cd] : mutationchargeCooldowns) {
        cd += dt;
    }

    for (auto& [entity, cd] : mutationDmgCooldowns) {
        if (cd > 0.0f)
            cd = std::max(0.0f, cd - dt);
    }

    for (auto& [entity, cd] : mutationPurifyLock)
    {
        if (cd > 0.0f)
            cd = std::max(0.0f, cd - dt);
    }
    /*for (auto& [entity, cd] : mutationPurifyCooldowns) /// Tempo feature
    {
        if (cd > 0.0f)
            cd = std::max(0.0f, cd - dt);
    }*/
}

/*
* @brief Check for player collision with key entities and handle key pickup logic.
* @param app Reference to the GameApp context.
* @param player Entity ID of the player character.
*/

void Interaction::CheckKeyPickup(GameApp& app, Entity player)
{
    Transform* pt = app.GetTransform(player);
    if (!pt) return;

    Vector2 pPos = pt->GetPosition();
    Vector2 pSize = pt->GetScale();
    if (Collider* pc = app.GetCollider(player)) {
        if (pc->type == ColliderType::Box) {
            pSize = pc->size;
        }

        else if (pc->type == ColliderType::Circle) {
            pSize = Vector2(pc->size.x * 2.0f, pc->size.x * 2.0f);
        }
    }

    const auto& allColliders = app.GetAllColliders();
    Entity keyEntity = 0;
    PlayerAbility keyType = PlayerAbility::DEFAULT;

    for (const auto& [other, otherCol] : allColliders)
    {
        if (!other || other == player) {
            continue;
        }

        if (!otherCol) {
            continue;
        }

        const std::string tag = app.GetPrefabTag(other);
        PlayerAbility foundType = PlayerAbility::DEFAULT;

        /// Only want to check for entities that are keys (either specific ability keys or a generic key)
        if (tag == "Key_Projectile") {
            foundType = PlayerAbility::PROJECTILE;
        }
        else if (tag == "Key_Burrow") {
            foundType = PlayerAbility::BURROW;
        }
        else if (tag == "Key_Heal") {
            foundType = PlayerAbility::HEAL;
        }
        else if (tag == "Key") {
            foundType = PlayerAbility::DEFAULT; // Fallback
        }
        else {
            continue;
        }

        Transform* ot = app.GetTransform(other);
        if (!ot) {
            continue;
        }

        Vector2 oPos = ot->GetPosition();
        Vector2 oSize = ot->GetScale();

        if (otherCol->type == ColliderType::Box) {
            oSize = otherCol->size;
        }

        else if (otherCol->type == ColliderType::Circle) {
            oSize = Vector2(otherCol->size.x * 2.0f, otherCol->size.x * 2.0f);
        }

        /// Check for AABB overlap between the melee hitbox and the enemy's collider
        if (AabbOverlapLocal(pPos, pSize, oPos, oSize))
        {
            keyEntity = other;
            keyType = foundType;
            break;
        }
    }

    /// If a key entity was found, check if the player can pick it up
    if (keyEntity)
    {
        PlayerController* pc = app.GetController(player);
        if (pc) {
            // Check for duplicates
            if (pc->HasKeyType(keyType)) {
                //DebugConsole::Get().Info("Duplicate Key found. Destroyed.\n");
                app.DestroyEntityNoExpose(keyEntity);
                return;
            }

            if (pc->GetKeyCount() >= 3) {
                //DebugConsole::Get().Info("Inventory Full! Cannot pick up more keys.\n");
                return;
            }
            pc->AddKey(keyType);
            //DebugConsole::Get().Info("Player picked up a " + std::string((keyType == PlayerAbility::PROJECTILE ? "Projectile" : (keyType == PlayerAbility::BURROW ? "Burrow" : (keyType == PlayerAbility::HEAL ? "Heal" : "Default")))) + " Key!\n");
            PlayRandomKeyCollectedSfx(1.0f);
        }
        app.DestroyEntityNoExpose(keyEntity);
    }
}

/*
* @brief Handle player input related to abilities and interactions.
* @param app Reference to the GameApp context.
* @param player Entity ID of the player character.
* @param dt Delta time since last update.
*/
void Interaction::HandlePlayerAbilityInput(GameApp& app, Entity player, double dt)
{
    CheckKeyPickup(app, player);
    auto& in = eng::input();
    PlayerController* pc = app.GetController(player);
    if (!pc) return;

    static bool fKeyWasPressed = false;

	/// Check for ability absorption input (F key or gamepad X button) and ensure it only triggers once per key press
    if (in.isKeyPressed(GLFW_KEY_F)|| in.isGamepadButtonPressed(GLFW_GAMEPAD_BUTTON_X)) {
        if (!fKeyWasPressed) {
            AbsorbAbilityFromCorpse(app, player);
            fKeyWasPressed = true;
        }
    }
    else {
        fKeyWasPressed = false;
    }

    bool hudConsumes = false;
    {
        const auto& btns = app.GetHUDGui().GetButtons();
        for (const auto& b : btns) {
            if (b.visible && (b.hovered || b.pressed)) { hudConsumes = true; break; }
        }
    }
    if ((in.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)|| in.isGamepadButtonPressed(GLFW_GAMEPAD_BUTTON_B)) && !hudConsumes && !pc->IsAttackInputSuppressed()) {
        if (pc->GetAbility() != PlayerAbility::PROJECTILE) {

            MeleeAttack(app, player);
        }
    }

    /*
       * List of cheat code for the the game:
       * 0: On / Off God Mod
       * 1: Set ability to default
       * 2: Set ability to heal
       * 3: Set ability to burrow
       * 4: Set ability to projectile
       * 5: Set mutation charge up to 0
       * 6: Set mutation charge up to 100
       * 7: Set the player hp to 100
       * 8: Set the player hp to 0
    */

    if (DEBUG_MODE) {
        Collider* col = app.GetCollider(player);
        if (pc->godmode == true) {
            col->isTrigger = true;

            pc->SetAbility(PlayerAbility::DEFAULT);
            bool hudConsumesDbg = false;
            {
                const auto& btns = app.GetHUDGui().GetButtons();
                for (const auto& b : btns) {
                    if (b.visible && (b.hovered || b.pressed)) { hudConsumesDbg = true; break; }
                }
            }
            if ((in.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) && !hudConsumesDbg && !pc->IsAttackInputSuppressed()) {
                MeleeAttack(app, player);
            }
            if (in.isKeyPressed(GLFW_KEY_E) || in.isGamepadButtonPressed(GLFW_GAMEPAD_BUTTON_Y)) {
                StartBurrow(app, player);
            }

            if (in.isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
            {
                Projectile(app, player);
            }
            
            if (in.isGamepadButtonPressed(GLFW_GAMEPAD_BUTTON_RIGHT_THUMB))
            {
                /// Check projectile cooldown before allowing shooting
                if (projectileCooldowns[player] <= 0.0f)
                {
                    Vector2 stick = in.getGamepadRightStick();
                    stick.y *= -1.0f;
                    if (stick.LengthSqd() > 0.0f)
                    {
                        /// Request shoot from player controller in the direction of the right stick
                        pc->RequestShoot(stick.Normalized());
                        projectileCooldowns[player] = SetAbilityCooldown(PlayerAbility::PROJECTILE);
                    }
                }
            }

        }
        else if (pc->godmode == true) {
            col->isTrigger = false;
        }

        if (in.isKeyPressed(GLFW_KEY_0))
        {
            pc->godmode = !pc->godmode;
        }
        if (pc->godmode == false) {
            if (in.isKeyPressed(GLFW_KEY_1)) {
                pc->SetAbility(PlayerAbility::DEFAULT);
            }
            if (in.isKeyPressed(GLFW_KEY_2)) {
                pc->SetAbility(PlayerAbility::HEAL);
            }
            if (in.isKeyPressed(GLFW_KEY_3)) {
                pc->SetAbility(PlayerAbility::BURROW);
            }
            if (in.isKeyPressed(GLFW_KEY_4)) {
                pc->SetAbility(PlayerAbility::PROJECTILE);
            }
            if (in.isKeyPressed(GLFW_KEY_5)) {
                if (pc->getMutationLevel() > 0) {
                    pc->setMutationLevel(0);
                }
            }
            if (in.isKeyPressed(GLFW_KEY_6)) {
                pc->setMutationLevel(100);
            }
            if (in.isKeyPressed(GLFW_KEY_7)) {
                pc->setPlayerHp(100);
            }
            if (in.isKeyPressed(GLFW_KEY_8)) {
                pc->setPlayerHp(0);
            }
        }

    }

    /// Key to activate the player current ability skill
    if (in.isKeyPressed(GLFW_KEY_E) || in.isGamepadButtonPressed(GLFW_GAMEPAD_BUTTON_Y)) {
        if (pc->GetAbility() == PlayerAbility::DEFAULT) {
            //DebugConsole::Get().Info("Player try to use skill but realise he is skilless. NOOB\n");
        }
        else if (pc->GetAbility() == PlayerAbility::HEAL) {
            // Try to heal a dead soldier corpse first (Puzzle #2)
            Transform* pt = app.GetTransform(player);
            bool healedCorpse = false;
            if (pt && app.GetPuzzleSystem()) {
                healedCorpse = app.GetPuzzleSystem()->OnPlayerHealUsed(pt->GetPosition());
            }

            if (!healedCorpse) {
                // No corpse nearby - do normal self-healing
                ApplyHealing(app, player, pc->getHealAmount());
                //DebugConsole::Get().Info("Player need to heal cause user skill issue\n");
            }
            else {
                // DebugConsole::Get().Info("[Heal] Dead soldier's memory revealed!\n");
            }
        }
        else if (pc->GetAbility() == PlayerAbility::BURROW) {
            StartBurrow(app, player);
            // DebugConsole::Get().Info("Player burrowing cause user is pussy\n");
        }
    }

	/// Handle projectile shooting input if the player has the projectile ability
    if (pc->GetAbility() == PlayerAbility::PROJECTILE)
    {
        if (in.isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
        {
            Projectile(app, player);
        }
		/// Also allow gamepad input for shooting (Right stick)
        else if (in.isGamepadButtonPressed(GLFW_GAMEPAD_BUTTON_RIGHT_THUMB))
        {
			/// Check projectile cooldown before allowing shooting
            if (projectileCooldowns[player] <= 0.0f)
            {
                Vector2 stick = in.getGamepadRightStick();
                stick.y *= -1.0f;
                if (stick.LengthSqd() > 0.0f)
                {
					/// Request shoot from player controller in the direction of the right stick
                    pc->RequestShoot(stick.Normalized());
                    projectileCooldowns[player] = SetAbilityCooldown(PlayerAbility::PROJECTILE);
                }
            }
        }
    }

    UpdateBurrow(app, player, static_cast<float>(dt));

    UpdateCooldowns(static_cast<float>(dt));

}

/*
   * @brief Handle mutation charge-up logic for the player.
   * @param app Reference to the GameApp context.
   * @param player Entity ID of the player character.
   * @param dt Delta time since last update.
   */
void Interaction::HandleMutationChargeUp(GameApp& app, Entity player, float dt) {

    auto* pc = app.GetController(player);

    if (!pc) return;

    /// If currently under mutation purification lock, do not allow charge up
    if (mutationPurifyLock[player] > 0.0f) {
        mutationPurifyLock[player] -= dt;
        return;
    }

    /// Only allow mutation charge up if player has an active ability
    if (pc->GetAbility() == PlayerAbility::DEFAULT) {
        return;
    }

    /// Charge up mutation level every 1 second
    if (mutationchargeCooldowns[player] >= 1.0f)
    {
        int mutationlevel = pc->getMutationLevel();

        /// Check if the mutation level is 0, if it is set as 1 and start to slowly increase
        if (mutationlevel == 0) {
            mutationlevel = 1;
        }
        else
        {
            /// Exponetially increase until hit the maximum which is 100
            mutationlevel = std::min(mutationlevel+=1, 100);
        }
        pc->setMutationLevel(mutationlevel);
        mutationchargeCooldowns[player] = 0.0f;
        //DebugConsole::Get().Info("Mutation Level of the player: " + std::to_string(pc->getMutationLevel()) + "\n");
    }
}

/*
* @brief Handle mutation damage effect for the player.
* @param app Reference to the GameApp context.
* @param player Entity ID of the player character.
* @param dt Delta time since last update.
*/
void Interaction::HandleMutationDamage(GameApp& app, Entity player, float dt) {
    (void)dt;
    auto* pc = app.GetController(player);

    if (!pc) return;

    /// Only apply mutation damage if player has an active ability
    if (pc->GetAbility() == PlayerAbility::DEFAULT) {
        return;
    }

    int mutationlevel = pc->getMutationLevel();

    /// Only apply damage when mutation level is at maximum (100), and the damage is applied every 5 seconds
    if (mutationlevel == 100) {
        int mutationDPS = pc->getMutationDPS();
        if (mutationDmgCooldowns[player] <= 0.0f) {
            if (mutationDPS == 0) {
                mutationDPS = 1;
            }
            else {
                mutationDPS = std::min(mutationDPS+=1, 100);
            }

            HandlePlayerDamage(app, player, mutationDPS);
            pc->setMuationDPS(mutationDPS);
            mutationDmgCooldowns[player] = 5.0f;
        }
    }
}

/*
* @brief Handle mutation purification logic for the player.
* @param app Reference to the GameApp context.
* @param player Entity ID of the player character.
*/
void Interaction::HandleMutationPurification(GameApp& app, Entity player) {
    auto* pc = app.GetController(player);

    if (!pc) return;

    /// ---- Implementation of a tempo feature ----
    //if (mutationPurifyCooldowns[player] > 0.0f)
    //{
    //    //DebugConsole::Get().Warning("Mutation purification is on cooldown for " + std::to_string(mutationPurifyCooldowns[player]) + " more seconds.\n");
    //    return;
    //}
    /// ----------------------- ///

    //DebugConsole::Get().Success("Player can now activate Purification Healing\n");

    int mutationLv = pc->getMutationLevel();

    /// If mutation level is above 0, reset it to 0 and start purification lock
    if (mutationLv > 0) {
        mutationLv = 0;
        ResourceManager::PlaySfx("Heal", 0.9f);
        pc->setMutationLevel(mutationLv);
    }

    mutationchargeCooldowns[player] = 0.0f;
    mutationPurifyLock[player] = 1.0f; // block charge for 1 sec

    //mutationPurifyCooldowns[player] = 20.0f; // 20 seconds cooldown after each use (Tempo feature)
}

float Interaction::GetAbilityCooldown(Entity target, PlayerAbility ability)
{
    switch (ability) {
    case PlayerAbility::BURROW:
        return burrowCooldowns[target];

    case PlayerAbility::HEAL:
        return healingCoolDown[target];

    case PlayerAbility::PROJECTILE:
        return projectileCooldowns[target];

    default:
        return 0.0f;
    }
}

float Interaction::GetAbilityCooldownRatio(Entity target, PlayerAbility ability)
{
    float cd = GetAbilityCooldown(target, ability);
    float maxTime = SetAbilityCooldown(ability);           // max cooldown

    if (maxTime <= 0.0f)
        return 0.0f;

    /* float maxTime = 0.0f;
     switch (ability) {
     case PlayerAbility::BURROW:
         return SetAbilityCooldown(PlayerAbility::BURROW);
     case PlayerAbility::HEAL:
         return SetAbilityCooldown(PlayerAbility::HEAL);
     case PlayerAbility::PROJECTILE:
         return SetAbilityCooldown(PlayerAbility::PROJECTILE);
     default:
         return 0.0f;
     }*/
    return std::clamp(cd / maxTime, 0.0f, 1.0f);

}
