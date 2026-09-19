#pragma once
/**
* @file     playerController.h
* @author   Tan Wei Liang Terril
* @email    t.weiliangterril
* @co-author Woh Kye Le, Jethro Sung, Low JianLin
* @email     w.kyele, sung.h, jianlin.low
* @date      2025-09-11
*
* @brief    Declares the PlayerController component for handling player movement,
*           input, and sprite animation state.
*
* The PlayerController class is attached to an entity and processes keyboard input
* each frame to update its Transform (position, rotation, scale) and determine the
* correct animation state to display (Idle, Run, Attack, etc.). It binds to
* MeshRenderer and SpriteAnimator components to control both visuals and animation.
*
* @version 1.1
*
* @update version history
* @version 1.0 - Declare PlayerController class and player component
* @version 1.1 - Implementation of forceproxy for the player.
*
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
#include "Core/component.h"
#include "Core/transform.h"
#include "Core/engine.hpp"
#include "Core/gameobj.h"
#include "Graphics/meshrenderer.h"
#include "Graphics/spriteanimator.h"
#include "Input/input.h"
#include "Core/resourceManager.h"
#include "Physics/ForceProxy.hpp"
#include "Physics/EntityForceProxy.hpp"
#include <vector>

// Forward declare
class MeshRenderer;
class SpriteAnimator;
class Transform;
class ParticleEmitter;
class ForceProxy;
class Collider;
//class CollisionSystem;

/**
 * @enum PlayerAbility
 * @brief Enumerates the different abilities a player can have.
 */
enum class PlayerAbility {
    DEFAULT,
    BURROW,
    HEAL,
    PROJECTILE
};

/**
 * @class PlayerController
 * @brief Component responsible for handling player input and state transitions.
 *
 * This component reads keyboard input, updates the player's Transform, and determines
 * which sprite animation should play. It supports movement (WASD), scaling (U/I),
 * rotation (R), and attack (Right Arrow) controls. Internally, it uses a finite
 * state machine to manage animation transitions.
 */
class PlayerController : public Component {
public:
    bool abilitykey{ false };

    /// Check if player is underground
    bool isUnderground = false;

    bool godmode = false;
  
    float rotSpeed = 1.2f;

    float timer = 0.0f;

    bool IsMoving() const { return moving; }
    bool IsFacingRight() const { return facingRight; }

    /*
    * @brief The player default ability.
    */
    PlayerAbility currentAbility = PlayerAbility::DEFAULT;

    /**
     * @brief Constructs a PlayerController tied to a specific entity.
     * @param ownerId Entity ID this component belongs to.
     */
    explicit PlayerController(Entity ownerId) : Component(ownerId), abilitykey(false) {} //tie it to an entity id when created

    /**
     * @brief Binds a particle emitter to the player for footstep effects.
     * @param em Pointer to the ParticleEmitter component.
     */
    void BindEmitter(ParticleEmitter* em) { footEmitter = em; }

    /**
     * @brief Updates the player's position, rotation, scale, and animation state.
     *
     * Reads input each frame to:
     * - Move the player based on WASD keys.
     *
     * @param dt Delta time since the last frame.
     * @param transform Reference to the Transform component of the same entity.
     */
    void Update(float dt, Transform& transform, Collider* collider);

    /**
     * @brief Updates player visuals each frame (particles + animation state selection).
     *
     * @param transform Transform component of the player entity.
     */
    void Draw(Transform& transform);


    /**
     * @brief Binds external rendering dependencies.
     *
     * The controller needs access to MeshRenderer and SpriteAnimator components
     * to change textures and animations based on the current state.
     *
     * @param mr Pointer to the entity's MeshRenderer component.
     * @param an Pointer to the entity's SpriteAnimator component.
     */
    void BindRenderDeps(MeshRenderer* mr, SpriteAnimator* an) { pendingRenderer = mr; pendingAnimator = an; }

    /**
     * @brief Suppresses or allows player attack inputs.
     * @param suppressed True to prevent attacking, false to allow.
     */
    void SetAttackInputSuppressed(bool suppressed) { suppressAttackInput = suppressed; }

    /**
     * @brief Checks if player attack inputs are currently suppressed.
     * @return True if suppressed.
     */
    bool IsAttackInputSuppressed() const { return suppressAttackInput; }

    /**
    * @brief Sets the EntityForceProxy for this system or manager.
    *
    * @param proxy Pointer to the EntityForceProxy instance to be used.
    */
    void setForceProxy(ForceProxy* fp) { forceProxy = fp; } // Testing

    /*
    * @brief Sets the player's health points.
    * @param hp The new health points value.
    */
    void setPlayerHp(int hp) { playerHP = hp; }

    /*
    * @brief Gets the player's current health points.
    */
    int getPlayerHp() const { return playerHP; }

    /*
    * @brief Sets the player's current ability.
    * @param newAbility The new ability to assign to the player.
    */
    void SetAbility(PlayerAbility newAbility);

    /*
    * @brief Gets the player's current ability.
    */
    PlayerAbility GetAbility() const { return currentAbility; }

    /*
    * @brief Handles logic when an enemy is killed by the player.
    * @param playerGetAbility The ability used by the player to kill the enemy.
    */
    void OnEnemyKilled(PlayerAbility playerGetAbility);

    /*
    * @brief Requests a shooting action in the specified direction.
    * @param dir The direction vector for the shooting action.
    */
    void RequestShoot(const Vector2& dir);

    /*
    * @brief Consumes a shoot request and outputs the direction.
    * @param outDir Reference to store the output direction vector.
    */
    bool ConsumeShoot(Vector2& outDir);

    /*
    * @brief Gets the current aim direction of the player.
    */
    Vector2 GetAimDirection() const;

    /*
        * @brief Sets the player's velocity.
        * @param vel The new velocity vector for the player.
    */
    void setPlayerVel(Vector2 vel) { playerVel = vel; }
    /*
        * @brief Gets the player's current velocity.
    */
    Vector2 getPlayerVel() const { return playerVel; }
    /*
        * @brief Updates the player's velocity based on physics calculations.
        * @param velocity The current velocity vector to be updated with physics effects.
        * @param dt Delta time since the last frame (in seconds).
    */
    void GetPhysicsSpeed(Vector2 velocity/*, float dt*/);

    /**
     * @brief Checks if the player has any keys in their inventory.
     * @return True if inventory is not empty.
     */
    bool HasKey() const { return !keyInventory.empty(); }

    /**
     * @brief Checks if the player has a specific type of key.
     * @param type The ability type associated with the key.
     * @return True if the player possesses this key type.
     */
    bool HasKeyType(PlayerAbility type) const {
        for (const auto& k : keyInventory) {
            if (k == type) return true;
        }
        return false;
    }

    /**
     * @brief Adds a key to the player's inventory, up to a maximum of 3.
     * @param type The ability type of the key.
     */
    void AddKey(PlayerAbility type) { if (keyInventory.size() < 3) keyInventory.push_back(type); }

    /**
     * @brief Removes the most recently added key from the inventory.
     */
    void RemoveKey() { if (!keyInventory.empty()) keyInventory.pop_back(); }

    /**
     * @brief Gets the total number of keys in the inventory.
     * @return Number of keys.
     */
    int GetKeyCount() const { return static_cast<int>(keyInventory.size()); }

    /**
     * @brief Gets the key at a specific inventory index.
     * @param index The inventory index.
     * @return The key's ability type, or DEFAULT if out of bounds.
     */
    PlayerAbility GetKeyAt(int index) const {
        if (index >= 0 && index < keyInventory.size()) return keyInventory[index];
        return PlayerAbility::DEFAULT;
    }

    /*
        * @brief Gets the player's projectile damage value.
    */
    int getPlayerProjectileDmg() const { return static_cast<int>(playerProjectileDmg); }

    /**
     * @brief Gets the player's melee damage value.
     * @return The melee damage value.
     */
    int getPlayerMeleeDmg() const { return playerMeleeDmg; }

    /*
        * @brief Gets the player's projectile speed value.
    */
    float getPlayerProjectileSpeed() const { return playerProjectileSpeed; }

    /*
    * @brief Sets the player's mutation level.
    * @param level The new mutation level to assign to the player.
    */
    void setMutationLevel(int level) { mutatationLevel = level; }
    /*
    * @brief Gets the player's current mutation level.
    */
    int getMutationLevel() const { return mutatationLevel; }

    /*
    * @brief Sets the player's mutation damage per second (DPS) value.
    * @param level The new mutation DPS value to assign to the player.
    */
    void setMuationDPS(int level) { mutationDPS = level; }

    /*
    * @brief Gets the player's current mutation damage per second (DPS) value.
    */
    int getMutationDPS() const { return mutationDPS; }

	/**
	 * @brief Gets the player's heal amount per activation.
	 * @return Heal amount value.
	 */
	int getHealAmount() const { return healAmount; }

	/**
	 * @brief Checks if the player is currently underground (burrowing).
	 * @return True if underground.
	 */
	bool getCheckUnderground() const { return isUnderground; }

    /// === Animation State Functions ===
    /** @brief Sets animation state to healing. */
    void PlayHealAnim();
    /** @brief Sets animation state to burrowing. */
    void PlayBurrowAnim();
    /** @brief Sets animation state to fully underground burrow. */
    void PlayBurrowUnderAnim();
    /** @brief Sets animation state to melee attack. */
    void PlayMeleeAnim();
    /** @brief Sets animation state to taking damage. */
    void PlayDamageAnim();
    /** @brief Sets animation state to absorbing ability. */
    void PlayAbsorbAnim();
    /** @brief Sets animation state to transforming into projectile form. */
    void PlayProjectileTransformAnim();

    /** @brief Stuns the player for a specified duration. */
    void Stun(float duration) { stunTimer = duration; }

    /** @brief Checks if the player is currently stunned. */
    bool IsStunned() const { return stunTimer > 0.0f; }

    /**
     * @brief Resets the player's state to default.
     */
    void Reset();

    /**
     * @brief Returns true if the melee animation is currently playing.
     */
    bool IsMeleeAnimating() const;

    /**
     * @brief Returns true if a burrow-related animation is currently playing.
     */
    bool IsBurrowAnimating() const;


private:

    // state of different sprite rendering 
     /**
     * @enum State
     * @brief Represents different player animation states.
     */
    enum class State { Idle_R, Idle_L, Run_R, Run_L, Heal_R, Heal_L, Burrow_R, Burrow_L, BurrowU_R, BurrowU_L, Melee_R, Melee_L, Damage_R, Damage_L, Absorb_R, Absorb_L, ProjectileTransform_R, ProjectileTransform_L, ProjectileIdle_R, ProjectileIdle_L, ProjectileWalk_R, ProjectileWalk_L };

    /**
    * @brief Applies the given state to the animator and renderer.
    *
    * Sets the active sprite sheet, frame range, speed, and texture based on the
    * chosen state. This function is typically called during Update when input
    * changes.
    *
    * @param s  Target animation state to apply.
    * @param mr Reference to the MeshRenderer component.
    * @param an Reference to the SpriteAnimator component.
    * @param tr Reference to the Transform component.
    */
    void ApplyState(State s, MeshRenderer& mr, SpriteAnimator& an, Transform& tr);

    Vector2 playerVel{ 0.0f, 0.0f };

    bool moving = false;
    bool  facingRight = true;
    State cur = State::Idle_R;
    bool absorbAnim = false;
    float absorbAnimTimer = 0.0f;
    bool projectileTransformAnim = false;
    float projectileTransformAnimTimer = 0.0f;
    bool projectileFormActive = false;

    int playerHP = 100; // max is 100
    float moveSpeed = 800.0f;
    float movementPlayerMaxSpeed = 300.0f;
    float movementPlayerfriction = 700.0f;
    float burrowSpeed = 950.0f;
    float burrowPlayerMaxSpeed = 400.0f;
    float burrowPlayerfriction = 800.0f;


    float footstepCooldown = 0.0f;
    int lastFootFrame = -1;

    bool  healingAnim = false;
    float healingAnimTimer = 0.0f;
	int healAmount = 20;

    bool  burrowAnim = false;
    bool burrowUnderStarted = false;
    float burrowAnimTimer = 0.0f;

    std::vector<PlayerAbility> keyInventory;

    bool  burrowunderAnim = false;
    float burrowunderAnimTimer = 0.0f;
    
    bool  meleeAnim = false;
    float meleeAnimTimer = 0.0f;

    bool  damageAnim = false;
    float damageAnimTimer = 0.0f;

    float stunTimer = 0.0f;

    Vector2 normalMeshScale{ 0.0f, 0.0f };
    bool hasNormalMeshScale = false;
    bool burrowMeshScaled = false;
    bool meleeMeshScaled = false;

    Vector2 normalColliderSize{ 0.0f, 0.0f };
    bool hasNormalColliderSize = false;
    bool burrowColliderScaled = false;
    bool meleeColliderScaled = false;

    bool shootRequested = false;
    Vector2 shootDir{ 0.0f, 0.0f };

    bool suppressAttackInput = false;

    float playerProjectileDmg = 9.0f;
	int playerMeleeDmg = 10;
    float playerProjectileSpeed = 500.f;

    float mutationLevelTimer = 0.0f;
    float mutationDamageTimer = 0.0f;
    int mutatationLevel = 0;
    int mutationDPS = 0;

    // Cached pointer to the entity's MeshRenderer for rendering changes.
    MeshRenderer* pendingRenderer = nullptr;

    /// Pointer to the current EntityForceProxy
    ForceProxy* forceProxy = nullptr;

    // Cached pointer to the entity's SpriteAnimator for animation control.
    SpriteAnimator* pendingAnimator = nullptr;

    ParticleEmitter* footEmitter = nullptr;
};
