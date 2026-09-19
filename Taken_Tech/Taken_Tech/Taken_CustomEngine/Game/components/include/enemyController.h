#pragma once

/**
 * @file     enemiesController.h
 * @author   Sng Swee Yong Dillon
 * @co-author Tan Wei Liang Terril, Low Jianlin, Jethro Sung
 * @email    sweeyongdillon.sng, t.weiliangterril, jianlin.low, sung.h
 * @date     2025-11-07
 *
 * @brief    Declares the EnemiesController component for basic enemy AI.
 *
 * This component provides behavior control for "enemy" entities in the game.
 * It mirrors the structure of PlayerController to support clean ECS integration.
 *
 * Responsibilities:
 * - High-level AI state machine (Idle, Patrol, Chase)
 * - Driving movement through EntityForceProxy (if assigned)
 * - Relaying animation state changes to SpriteAnimator
 * - Tracking map boundaries and spawn information
 *
 * @note Core AI logic is delegated to EnemyAi (pathfinding & chasing).
 * @note Some features remain placeholders atm.
 */

#include "Core/component.h"
#include "Core/transform.h"
#include "Graphics/meshrenderer.h"
#include "Graphics/spriteanimator.h"
#include "World/ai.h"
#include "Physics/EntityForceProxy.hpp"
#include "Input/message_system.h"

class GameApp;

enum class EnemyType : uint8_t
{
    Normal = 0,
    Boss = 1
};

 /**
 * @class EnemiesController
 * @brief AI and movement control for enemy entities.
 *
 * Acts as a thin wrapper around EnemyAi for:
 * - Determining directional movement each frame
 * - Updating Transform and forcing spatial response
 * - Triggering animation based on behavior state
 *
 * The ECS System responsible for enemies will:
 * 1) Retrieve Transform
 * 2) Call Update(dt, transform, playerPos)
 * 3) Apply returned movement into physics or velocity systems
 */
class EnemiesController : public Component {
public:
    enum class State { Idle, Patrol, Chase };

    /**
        * @brief Constructs an EnemiesController for a specific entity.
        *
        * @param e      Entity ID being controlled.
        * @param msgHub Optional pointer for AI message events (e.g., player spotted).
    */
    explicit EnemiesController(Entity e, Messaging::Observable* msgHub)
        : Component(e), self(e), ai(e, MobType::RANGED, 2.0f, Direction::RIGHT) {
        if (msgHub)
            ai.SetObservable(msgHub);
    }

    // Wiring from factories / loader
    /**
     * @brief Sets the map file path for AI pathfinding.
     * @param path The path to the map file.
     */
    void SetMap(const std::string& path) { ai.setMap(path); }

    /**
     * @brief Sets the spawn position for the enemy.
     * @param pos The world coordinates to spawn the enemy at.
     */
    void SetSpawn(const Vector2& pos) { ai.setEnemySpawn(pos); }

    /**
     * @brief Sets the mob type for this enemy.
     * @param t The MobType enum value.
     */
    void SetMobType(MobType t) { ai.SetMobType(t); }

    /**
     * @brief Sets the pending MeshRenderer for this enemy.
     * @param r Pointer to the MeshRenderer.
     */
    void SetPendingRenderer(MeshRenderer* r) { pendingRenderer = r; }

    /**
     * @brief Sets the pending SpriteAnimator for this enemy.
     * @param a Pointer to the SpriteAnimator.
     */
    void SetPendingAnimator(SpriteAnimator* a) { pendingAnimator = a; }

    /**
        * @brief Per-frame behavior update for the controlled enemy.
        *
        * Behavior flow:
        *  - Select movement intent based on proximity to player
        *  - Push resulting speed via EntityForceProxy (if available)
        *  - Update animation state depending on direction & state
        *
        * @param dt        Delta time in seconds.
        * @param transform World Transform reference to modify.
        * @param playerPos Current world position of the player.
        * @param app       Reference to the GameApp for audio listener context.
    */
    void Update(float dt, Transform& transform, const Vector2& playerPos, GameApp& app);

    /**
     * @brief Consumes a pending shoot request from the AI.
     * @param outDir Direction vector to populate if a shot is consumed.
     * @return True if a shoot request was pending and consumed.
     */
    bool ConsumeShoot(Vector2& outDir);

    /**
     * @brief Sets the EntityForceProxy for this system or manager.
     * @param proxy Pointer to the EntityForceProxy instance to be used.
     */
    void setForceProxy(EntityForceProxy* proxy) { forceProxy = proxy; }

    /**
     * @brief Binds external rendering dependencies.
     * @param mr Pointer to the entity's MeshRenderer.
     * @param an Pointer to the entity's SpriteAnimator.
     */
     /* void BindRenderDeps(MeshRenderer* mr, SpriteAnimator* an) {
          pendingRenderer = mr;
          pendingAnimator = an;
      }*/

    /**
     * @brief Forwards map grid data to the underlying AI component.
     * @param grid 2D map grid representation.
     * @param tileSize Size of each tile in world units.
     */
    void SetGrid(const std::vector<std::string>& grid, float tileSize);

    /**
     * @brief Sets the primary enemy class (Normal vs Boss).
     * @param t The EnemyType enum value.
     */
    void SetEnemyType(EnemyType t) {
        type = t;
        maxHp = (type == EnemyType::Boss) ? 300 : 30;
        hp = maxHp;
    }

    /**
     * @brief Gets the primary enemy class (Normal vs Boss).
     * @return The EnemyType enum value.
     */
    EnemyType GetEnemyType() const { return type; }

    /**
     * @brief Checks if this enemy is a boss.
     * @return True if the enemy is a boss.
     */
    bool IsBoss() const { return type == EnemyType::Boss; }

    /**
     * @brief Promotes this enemy to a boss entity.
     * @param v Unused boolean flag.
     */
    void SetBoss(bool /*v*/) {
        type = EnemyType::Boss;
        maxHp = 300;
        hp = maxHp;
    }

    /**
     * @brief Sets the specific AI mobility/behavior type.
     * @param t The MobType enum value.
     */
    void SetAiMobType(MobType t) {
        ai.SetMobType(t);
    }

    /**
     * @brief Gets the shooting range for spatial audio and AI logic.
     * @return The current shootRange value.
     */
    float GetShootRange() const { return shootRange; }

    /**
     * @brief Returns the AI's mobility type (RANGED, MELEE, etc).
     */
    MobType GetMobType() const { return ai.GetMobType(); }

    /**
     * @brief Gets the current health of the enemy.
     * @return The current health value.
     */
    int GetHP() const { return hp; }

    /**
     * @brief Gets the maximum health of the enemy.
     * @return The max health value.
     */
    int GetMaxHP() const { return maxHp; }

    /**
     * @brief Applies damage to the enemy.
     * @param dmg Amount of damage to deal.
     * @return True if the enemy was successfully damaged (not in i-frames).
     */
    bool TakeDamage(int dmg);

    /**
     * @brief Restores health to the enemy, up to its max.
     * @param amount Amount of health to restore.
     */
    void RestoreHp(int amount) { hp = std::min(hp + amount, maxHp); }

    /**
     * @brief Resets the enemy's health back to its maximum.
     */
    void ResetHealth(); 

    /**
     * @brief Sets whether the enemy's movement is driven by Lua scripts.
     * @param v True to enable Lua-driven movement.
     */
    void SetLuaDrivenMovement(bool v) { luaDrivenMovement = v; }

private:
    Entity self;
    EnemyAi ai;
    float timer = 0.0;
    float footstepTimer = 0.5f;
    Vector2 lastPosition = { 0, 0 };
    bool hasLastPosition = false;
    bool facingRight = true;
    float attackAnimTimer = 0.0f;
    bool attackFacingRight = true;
    EnemyType type = EnemyType::Normal;
    Vector2 normalMeshScale{ 0.0f, 0.0f };
    bool hasNormalMeshScale = false;
    bool burrowMeshScaled = false;
    Vector2 normalColliderSize{ 0.0f, 0.0f };
    bool hasNormalColliderSize = false;
    bool burrowColliderScaled = false;

    // Projectile ability req only; spawning handled by GameApp
    float shootCooldown = 1.25f;   // Seconds between shots.
    float shootTimer = 0.0f;    // Accumulates dt until cooldown met.
    float shootRange = 350.0f;  // Range at which enemy can shoot the player.
    bool  shootRequested = false;  // Latches a single shot req.
    Vector2 shootDir{ 1.0f, 0.0f };// Cached aim direction for the pending shot.

    MeshRenderer* pendingRenderer = nullptr;
    SpriteAnimator* pendingAnimator = nullptr;

    /// Pointer to the current EntityForceProxy
    EntityForceProxy* forceProxy = nullptr;

    //keep for debug prints
    State cur = State::Idle;
    void ApplyState(State s, MeshRenderer& mr, SpriteAnimator& an);

    int maxHp = 30;
    int hp = 30;
    float hurtIFrame = 0.0f;
    bool luaDrivenMovement = false;
};
