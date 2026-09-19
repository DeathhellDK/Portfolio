/**
* @file     playerController.cpp
* @author   Jethro Sung
* @co-author Woh Kye Le, Tan Wei Liang Terril, Low JianLin
* @email     w.kyele, sung.h, t.weiliangterril, jianlin.low
* @date      2026-02-1
*
* @brief    Implements the PlayerController component.
*
* This file defines the runtime behavior of the PlayerController component, which
* handles player input (movement, rotation, scaling, attack) and updates the
* associated Transform, MeshRenderer, and SpriteAnimator components accordingly.
* A simple finite state machine is used to drive animation state transitions.
*
* @version 1.2
*
* @update version history
* @version 1.0 -
* @version 1.1 - Modify player movment to use force physics.
* @version 1.2 - Added player combat interaction logics
*
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
#include "playerController.h"
#include "Particle/particleEmitter.h"
#include "Input/input.h"
#include "Physics/ForceSystem.hpp"
#include "Core/randomiser.h"
#include <GLFW/glfw3.h>
#include <string>
#include "Input/DebugConsole.hpp"
#include "Mechanics/interaction.hpp"
#include "Core/Systems/collisionSystem.h"
#include "Core/gameApp.h" /// For Debug Mode 
#include "Physics/collider.h"

const float SECOND = (1.0f / 60.0f);

//float atkCooldown = 0.0f;


/**
 * @brief Updates the player entity each frame based on input.
 *
 * This function:
 * - Reads input keys (WASD for movement, U/I for scaling, R for rotation, Right Arrow for attack).
 * - Updates the entity's Transform position, scale, and rotation accordingly.
 * - Selects the correct animation state based on movement and attack status.
 * - Calls ApplyState() to update the MeshRenderer and SpriteAnimator if they are bound.
 *
 * @param dt         Delta time since the last frame (in seconds).
 * @param transform  Reference to the Transform component of the controlled entity.
 */
void PlayerController::Update(float dt, Transform& transform, Collider* collider) {
    transform.BackupPosition();

    if (!hasNormalMeshScale) {
        normalMeshScale = transform.GetScale();
        hasNormalMeshScale = true;
    }

    const bool fullyBurrowed = (isUnderground && burrowunderAnim);
    if (fullyBurrowed) {
        if (!burrowMeshScaled) {
            normalMeshScale = transform.GetScale();
        }

        Vector2 burrowScale = normalMeshScale;
        burrowScale.y *= (1.0f / 3.0f);
        transform.SetScale(burrowScale);
        burrowMeshScaled = true;
    }
    else if (meleeAnim) {
        if (!meleeMeshScaled) {
            normalMeshScale = transform.GetScale();
        }

        Vector2 meleeScale = normalMeshScale;
        meleeScale.x *= 1.4f;
        transform.SetScale(meleeScale);
        meleeMeshScaled = true;
    }
    else {
        if (burrowMeshScaled) {
            transform.SetScale(normalMeshScale);
            burrowMeshScaled = false;
        }
        if (meleeMeshScaled) {
            transform.SetScale(normalMeshScale);
            meleeMeshScaled = false;
        }
        if (!isUnderground && !meleeAnim) {
            normalMeshScale = transform.GetScale();
        }
    }

    if (collider && collider->type == ColliderType::Box) {
        if (!hasNormalColliderSize) {
            normalColliderSize = collider->size;
            hasNormalColliderSize = true;
        }

        if (fullyBurrowed) {
            if (!burrowColliderScaled) {
                normalColliderSize = collider->size;
            }

            Vector2 burrowColSize = normalColliderSize;
            burrowColSize.y *= (1.0f / 3.0f);
            collider->size = burrowColSize;
            burrowColliderScaled = true;
        }
        else if (meleeAnim) {
            if (!meleeColliderScaled) {
                normalColliderSize = collider->size;
            }

            Vector2 meleeColSize = normalColliderSize;
            meleeColSize.x *= 1.4f;
            collider->size = meleeColSize;
            meleeColliderScaled = true;
        }
        else {
            if (burrowColliderScaled) {
                collider->size = normalColliderSize;
                burrowColliderScaled = false;
            }
            if (meleeColliderScaled) {
                collider->size = normalColliderSize;
                meleeColliderScaled = false;
            }
            if (!isUnderground && !meleeAnim) {
                normalColliderSize = collider->size;
            }
        }
    }

    if (stunTimer > 0.0f) {
        stunTimer -= dt;
        if (stunTimer < 0.0f) stunTimer = 0.0f;
    }

    auto& in = eng::input();
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    
    float stickX = 0.0f;
    float stickY = 0.0f;

    /// === Gamepad Support === ///
    GLFWgamepadstate state;
    bool gamepadConnected = glfwGetGamepadState(GLFW_JOYSTICK_1, &state);

    if (stunTimer <= 0.0f) {
        left = in.isKeyDown(GLFW_KEY_A);
        right = in.isKeyDown(GLFW_KEY_D);
        up = in.isKeyDown(GLFW_KEY_W);
        down = in.isKeyDown(GLFW_KEY_S);

        if (gamepadConnected)
        {
            stickX = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
            stickY = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];

            float deadzone = 0.2f;

            /// Apply deadzone to prevent drift
            if (fabs(stickX) < deadzone) stickX = 0.0f;
            if (fabs(stickY) < deadzone) stickY = 0.0f;

            /// invert Y so up is positive (same feel as keyboard)
            stickY *= -1.0f;
        }
    }


    if (DEBUG_MODE) {
        if (godmode == true) {
            keyInventory.clear();
            keyInventory.push_back(PlayerAbility::BURROW);
            keyInventory.push_back(PlayerAbility::HEAL);
            keyInventory.push_back(PlayerAbility::PROJECTILE);
        }
        if (in.isKeyPressed(GLFW_KEY_K)) {
            keyInventory.clear();
            keyInventory.push_back(PlayerAbility::BURROW);
            keyInventory.push_back(PlayerAbility::HEAL);
            keyInventory.push_back(PlayerAbility::PROJECTILE);
        }

    }

    float inputX = 0.0f;
    float inputY = 0.0f;

    if (left) { inputX -= 1.0f; facingRight = false; }
    if (right) { inputX += 1.0f; facingRight = true; }
    if (down) { inputY -= 1.0f; }
    if (up) { inputY += 1.0f; }

    inputX += stickX;
    inputY += stickY;

    moving = (inputX != 0.0f || inputY != 0.0f);

    Vector2 defaultForce(inputX, inputY);

    if (!left && !right && fabs(stickX) > 0.3f)
    {
        facingRight = (stickX > 0.0f);
    }

    // Normalize the force vector if it has non-zero length
    /*
    if (defaultForce.x != 0.0f || defaultForce.y != 0.0f) {
        float length = sqrt(defaultForce.x * defaultForce.x + defaultForce.y * defaultForce.y);
        defaultForce.x = (defaultForce.x / length) * moveSpeed;
        defaultForce.y = (defaultForce.y / length) * moveSpeed;
    }*/
    if (isUnderground == false) {
        /// Normalize the force vector if it has non-zero length
        if (defaultForce.x != 0.0f || defaultForce.y != 0.0f) {
            float length = sqrt(defaultForce.x * defaultForce.x + defaultForce.y * defaultForce.y);
            defaultForce.x = (defaultForce.x / length) * moveSpeed;
            defaultForce.y = (defaultForce.y / length) * moveSpeed;
            //DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "Player is above ground, applying normal move speed.");
        }
    }
    else {
        if (defaultForce.x != 0.0f || defaultForce.y != 0.0f) {
            float length = sqrt(defaultForce.x * defaultForce.x + defaultForce.y * defaultForce.y);
            defaultForce.x = (defaultForce.x / length) * burrowSpeed;
            defaultForce.y = (defaultForce.y / length) * burrowSpeed;
            //DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "Player is underground, applying burrow speed.");
        }
    }

    /// Set the player's velocity
    setPlayerVel(defaultForce);

    /// Apply the force to the player entity using the ForceProxy
    if (!moving && forceProxy) {
        if (auto* spd = forceProxy->GetSpeedComponent(owner)) {
			spd->maxSpeed = (isUnderground) ? burrowPlayerMaxSpeed : movementPlayerMaxSpeed;
			spd->friction = (isUnderground) ? burrowPlayerfriction : movementPlayerfriction;
            if (spd->speed.x != 0.0f || spd->speed.y != 0.0f) {
                float currentLength = sqrt(spd->speed.x * spd->speed.x +
                    spd->speed.y * spd->speed.y);

                // Friction force opposes current velocity direction
                float frictionDelta = spd->friction * dt;

                if (frictionDelta >= currentLength) {
                    // Friction is strong enough to stop the player fully
                    spd->speed = { 0.0f, 0.0f };
                }
                else {
                    float scale = (currentLength - frictionDelta) / currentLength;
                    spd->speed.x *= scale;
                    spd->speed.y *= scale;
                }
            }
        }
    }

    /// === Attack cooldown logic ===
    //if (atkCooldown > 0.0f) {
    //    atkCooldown -= dt;
    //    if (atkCooldown < 0.0f) atkCooldown = 0.0f;
    //}

    /// == Attack input logic == (handled by Interaction)

    /// - Player Abliluty Logic -
    /// Check if ability key is pressed
    if (abilitykey) {
        /// Trigger ability based on current ability of the player
        switch (currentAbility)
        {
        case PlayerAbility::DEFAULT:
            /* DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[Ability] No ability selected.");*/
            abilitykey = false;
            break;
        case PlayerAbility::BURROW:
            abilitykey = false;
            break;
        case PlayerAbility::HEAL:
            abilitykey = false;
            break;
        case PlayerAbility::PROJECTILE:
            abilitykey = false;
            break;
        default:
            abilitykey = false;
            break;
        }
    }
    /// Update animation timers and states
    if (healingAnim) {
        healingAnimTimer -= dt;
        if (healingAnimTimer <= 0.0f) {
            healingAnimTimer = 0.0f;
            healingAnim = false;
        }
    }
    if (burrowAnim) {
        burrowAnimTimer -= dt;

        if (burrowAnimTimer <= 0.0f) {
            burrowAnimTimer = 0.0f;

            if (!burrowunderAnim) {
                burrowunderAnim = true;
                burrowunderAnimTimer = 2.0f;
            }
        }
    }
    if (burrowunderAnim) {

        burrowunderAnimTimer -= dt;

        if (burrowunderAnimTimer <= 0.0f) {
            burrowunderAnimTimer = 0.0f;
            burrowunderAnim = false;
            burrowAnim = false; // sequence finished
        }
    }

    if (meleeAnim) {
        meleeAnimTimer -= dt;
        if (meleeAnimTimer <= 0.0f) {
            meleeAnimTimer = 0.0f;
            meleeAnim = false;
        }
    }

    if (damageAnim) {
        damageAnimTimer -= dt;
        if (damageAnimTimer <= 0.0f) {
            damageAnimTimer = 0.0f;
            damageAnim = false;
        }
    }
    if (absorbAnim) {
        absorbAnimTimer -= dt;
        if (absorbAnimTimer <= 0.0f) {
            absorbAnimTimer = 0.0f;
            absorbAnim = false;
        }
    }
    if (projectileTransformAnim) {
        projectileTransformAnimTimer -= dt;
        if (projectileTransformAnimTimer <= 0.0f) {
            projectileTransformAnimTimer = 0.0f;
            projectileTransformAnim = false;
            if (currentAbility == PlayerAbility::PROJECTILE) {
                projectileFormActive = true;
            }
        }
    }

}

void PlayerController::Draw(Transform& transform) {
    if (!pendingRenderer || !pendingAnimator) {
        return;
    }

    if (projectileTransformAnim) {
        ApplyState(facingRight ? State::ProjectileTransform_R : State::ProjectileTransform_L,
            *pendingRenderer, *pendingAnimator, transform);
        return;
    }

    if (absorbAnim) {
        ApplyState(facingRight ? State::Absorb_R : State::Absorb_L,
            *pendingRenderer, *pendingAnimator, transform);
        return;
    }

    if (footEmitter) {
        footEmitter->enabled = moving;

        const Vector2 sz = transform.GetScale();
        constexpr float kFootPadY = 2.0f;

        if (facingRight) {
            footEmitter->offset.x = 10.0f;
            footEmitter->offset.y = kFootPadY;
        }
        else {
            footEmitter->offset.x = sz.x - 10.0f;
            footEmitter->offset.y = kFootPadY;
        }
    }

    if (healingAnim) {
        ApplyState(facingRight ? State::Heal_R : State::Heal_L,
            *pendingRenderer, *pendingAnimator, transform);
        return;
    }

    if (meleeAnim) {
        ApplyState(facingRight ? State::Melee_R : State::Melee_L,
            *pendingRenderer, *pendingAnimator, transform);
        return;
    }

    if (damageAnim) {
        ApplyState(facingRight ? State::Damage_R : State::Damage_L,
            *pendingRenderer, *pendingAnimator, transform);
        return;
    }

    if (burrowAnim) {
        if (!burrowunderAnim) {
            ApplyState(
                facingRight ? State::Burrow_R : State::Burrow_L,
                *pendingRenderer, *pendingAnimator, transform
            );
            return;
        }
    }

    if (burrowunderAnim) {
        ApplyState(
            facingRight ? State::BurrowU_R : State::BurrowU_L,
            *pendingRenderer, *pendingAnimator, transform
        );
        return;
    }

    if (currentAbility == PlayerAbility::PROJECTILE && projectileFormActive) {
        if (moving) {
            ApplyState(facingRight ? State::ProjectileWalk_R : State::ProjectileWalk_L, *pendingRenderer, *pendingAnimator, transform);
        }
        else {
            ApplyState(facingRight ? State::ProjectileIdle_R : State::ProjectileIdle_L, *pendingRenderer, *pendingAnimator, transform);
        }
    }
    else {
        if (moving) {
            ApplyState(facingRight ? State::Run_R : State::Run_L, *pendingRenderer, *pendingAnimator, transform);
        }
        else {
            ApplyState(facingRight ? State::Idle_R : State::Idle_L, *pendingRenderer, *pendingAnimator, transform);
        }
    }

    float dt = static_cast<float>(eng::deltaTime());

    if (footstepCooldown > 0.0f) {
        footstepCooldown -= dt;
        if (footstepCooldown < 0.0f) footstepCooldown = 0.0f;
    }

    if (pendingAnimator) {
        int curFrame = pendingAnimator->cur;
        int prevFrame = lastFootFrame;

        bool isFootDown = (curFrame == 1 || curFrame == 3);
        bool wasFootDown = (prevFrame == 1 || prevFrame == 3);

        if (isFootDown && !wasFootDown && moving && footstepCooldown <= 0.0f)
        {
            PlayRandomFootstep(0.05f);
            footstepCooldown = 0.22f;
        }

        lastFootFrame = curFrame;
    }
}
/*
*@brief Updates the player's velocity based on physics calculations.
* @param velocity The current velocity vector to be updated with physics effects.
* @param dt Delta time since the last frame(in seconds).
*/
void PlayerController::GetPhysicsSpeed(Vector2 velocity/*, float dt*/) {
    if (forceProxy) {

        /// Pass the player's maxSpeed and friction
        forceProxy->triggerForce(owner, velocity);

    }
}

/**
* @brief Applies a specific animation state to the renderer and animator.
*
* Based on the given state, this function:
* - Sets the appropriate sprite sheet, frame range, and playback speed.
* - Updates the MeshRenderer's texture to match the selected animation.
* - Prints debug information to the console about the bound sprite sheet.
*
* @param s   The animation state to apply.
* @param mr  Reference to the entity's MeshRenderer component.
* @param an  Reference to the entity's SpriteAnimator component.
* @param tr  Reference to the entity's Transform component.
*/
void PlayerController::ApplyState(State s, MeshRenderer& mr, SpriteAnimator& an, Transform& /*tr*/) {
    if (s == cur && an.sheet) return; // no state change
    cur = s;

    std::string name;
    int start = 0, end = 0;
    float fps = 10.f;

    switch (cur) {
    case State::Idle_R: name = "player_idle_right"; start = 0; end = 1;  fps = 2.f;  facingRight = true;  break;
    case State::Idle_L: name = "player_idle_left";  start = 0; end = 1;  fps = 2.f;  facingRight = false; break;
    case State::Run_R:  name = "player_run_right";  start = 0; end = 4;  fps = 12.f; facingRight = true;  break;
    case State::Run_L:  name = "player_run_left";   start = 0; end = 4;  fps = 12.f; facingRight = false; break;
    case State::Heal_R: name = "player_heal_R";     start = 0; end = 4;  fps = 12.f; facingRight = true;  break;
    case State::Heal_L: name = "player_heal_L";     start = 0; end = 4;  fps = 12.f; facingRight = false; break;
    case State::Burrow_R: name = "player_burrow_R"; start = 0; end = 5;  fps = 12.f; facingRight = true;  break;
    case State::Burrow_L: name = "player_burrow_L"; start = 0; end = 5;  fps = 12.f; facingRight = false;  break;
    case State::BurrowU_R: name = "player_burrowUnder_R"; start = 0; end = 1;  fps = 12.0f; facingRight = true;  break;
    case State::BurrowU_L: name = "player_burrowUnder_L"; start = 0; end = 1;  fps = 12.0f; facingRight = false;  break;
    case State::Melee_R: name = "char_melee_R"; start = 0; end = 3;  fps = 10.0f; facingRight = true;  break;
    case State::Melee_L: name = "char_melee_L"; start = 0; end = 3;  fps = 10.0f; facingRight = false;  break;
    case State::Damage_R: name = "char_damage_R"; start = 0; end = 2;  fps = 10.0f; facingRight = true;  break;
    case State::Damage_L: name = "char_damage_L"; start = 0; end = 2;  fps = 10.0f; facingRight = false;  break;
    case State::Absorb_R: name = "player_ability_absorb_R"; start = 0; end = 4; fps = 12.0f; facingRight = true; break;
    case State::Absorb_L: name = "player_ability_absorb_L"; start = 0; end = 4; fps = 12.0f; facingRight = false; break;
    case State::ProjectileTransform_R: name = "player_projectile_transform_R"; start = 0; end = 4; fps = 12.0f; facingRight = true; break;
    case State::ProjectileTransform_L: name = "player_projectile_transform_L"; start = 0; end = 4; fps = 12.0f; facingRight = false; break;
    case State::ProjectileIdle_R: name = "Player_projectile_idle_R"; start = 0; end = 2; fps = 4.0f; facingRight = true; break;
    case State::ProjectileIdle_L: name = "Player_projectile_idle_L"; start = 0; end = 2; fps = 4.0f; facingRight = false; break;
    case State::ProjectileWalk_R: name = "Player_projectile_walk_R"; start = 0; end = 4; fps = 12.0f; facingRight = true; break;
    case State::ProjectileWalk_L: name = "Player_projectile_walk_L"; start = 0; end = 4; fps = 12.0f; facingRight = false; break;

    }

    if (!name.empty()) {
        if (const SpriteSheet* sheet = ResourceManager::GetSpriteSheet(name)) {
            an.sheet = sheet;
            an.startFrame = start;
            an.endFrame = end;
            an.cur = start;
            an.acc = 0.0f; // Reset accumulator
            an.speed = fps;
        }
    }
    if (GLuint tex = ResourceManager::GetTexture(name)) {
        mr.SetTexture(tex);
    }

    // ================ DO NOT REMOVE, FOR DEBUGGING PRINT ================ // 
   /* if (an.sheet) {
        DebugConsole::Get().AddFormattedMessage(LogLevel::Info,"[AnimBind] sheet=" + name +
            " tex=" + std::to_string(an.sheet->texture.Width()) + "x" + std::to_string(an.sheet->texture.Height()) +
            " cols=" + std::to_string(an.sheet->cols) + " rows=" + std::to_string(an.sheet->rows) +
            " frame=" + std::to_string(an.sheet->frameW) + "x" + std::to_string(an.sheet->frameH) +
            " UVsize=(" +
            std::to_string(float(an.sheet->frameW) / an.sheet->texture.Width()) + "," +
            std::to_string(float(an.sheet->frameH) / an.sheet->texture.Height()) + ")"
        );
    }
    else {
        DebugConsole::Get().AddFormattedMessage(LogLevel::Info,"[AnimBind] sheet=" + name + " (null)");
    }*/
}

/**
 * @brief Handles logic when an enemy is killed.
 * Updates the player's ability based on the ability granted by the defeated enemy.
 *
 * @param playerGetAbility The ability obtained from the killed enemy.
 */
void PlayerController::OnEnemyKilled(PlayerAbility playerGetAbility)
{
    /// Update the player's ability
    SetAbility(playerGetAbility);
}

/**
 * @brief Sets the player's current ability.
 *
 * If the new ability is different from the current one, updates the current ability
 * and logs the change to the debug console.
 *
 * @param newAbility The new ability to assign to the player.
 */
void PlayerController::SetAbility(PlayerAbility newAbility)
{
    /// Only update if the ability is changing
    if (currentAbility == newAbility)  return;

    /// Update the current ability
    currentAbility = newAbility;
    if (currentAbility != PlayerAbility::PROJECTILE) {
        projectileFormActive = false;
        projectileTransformAnim = false;
        projectileTransformAnimTimer = 0.0f;
    }

}

/**
 * @brief Requests a shooting action in the specified direction.
 *
 * Sets the internal state to indicate that a shoot action has been requested,
 * along with the direction vector for the shooting action.
 *
 * @param dir The direction vector for the shooting action.
 */
void PlayerController::RequestShoot(const Vector2& dir)
{
    /// Set shoot request state
    shootRequested = true;
    /// Store the shooting direction
    shootDir = dir;
}

/**
 * @brief Consumes a shoot request and outputs the direction.
 *
 * If a shoot request is pending, this function resets the request state,
 * outputs the stored shooting direction, and returns true. If no shoot
 * request is pending, it returns false.
 *
 * @param outDir Reference to store the output direction vector.
 * @return true if a shoot request was consumed; false otherwise.
 */
bool PlayerController::ConsumeShoot(Vector2& outDir)
{
    /// Check if a shoot request is pending
    if (!shootRequested)
        return false;

    /// Output the shooting direction and reset the request state
    shootRequested = false;
    /// Provide the stored shooting direction
    outDir = shootDir;
    return true;
}

/**
 * @brief Gets the current aim direction of the player.
 *
 * This function returns a unit vector representing the player's aim direction
 * based on their facing direction.
 *
 * @return A Vector2 representing the aim direction.
 */
Vector2 PlayerController::GetAimDirection() const
{
    /// Return the aim direction based on facing direction
    if (facingRight)
        return Vector2{ 1.0f, 0.0f };
    else
        return Vector2{ -1.0f, 0.0f };
}


void PlayerController::PlayHealAnim() {
    healingAnim = true;

    constexpr float healFps = 12.0f;

    healingAnimTimer = 5.0f / healFps;
}

void PlayerController::PlayBurrowAnim() {
    burrowAnim = true;

    constexpr float burrowFps = 12.0f;

    burrowAnimTimer = 5.0f / burrowFps;
}

void PlayerController::PlayBurrowUnderAnim() {

    if (burrowUnderStarted)
        return;
    constexpr float burrowUnderFps = 12.0f;
    constexpr float burrowUnderFrames = 5.0f;
    constexpr float burrowUnderDuration = 2.0f;

    burrowunderAnimTimer = burrowUnderDuration - (burrowUnderFrames / burrowUnderFps);
}

void PlayerController::PlayMeleeAnim() {
    meleeAnim = true;
    constexpr float meleeFps = 10.0f;
    meleeAnimTimer = 4.0f / meleeFps;
}

void PlayerController::PlayDamageAnim() {
    damageAnim = true;
    constexpr float damageFps = 10.0f;
    damageAnimTimer = 3.0f / damageFps;
}

void PlayerController::PlayAbsorbAnim() {
    absorbAnim = true;
    constexpr float absorbFps = 12.0f;
    absorbAnimTimer = 5.0f / absorbFps;
}

void PlayerController::PlayProjectileTransformAnim() {
    projectileTransformAnim = true;
    constexpr float tfFps = 12.0f;
    projectileTransformAnimTimer = 5.0f / tfFps;
}

void PlayerController::Reset() {
    playerHP = 100;
    currentAbility = PlayerAbility::DEFAULT;
    mutatationLevel = 0;
    mutationDPS = 0;
    //mutationLevelTimer = 0.0f;
    //mutationDamageTimer = 0.0f;
    moving = false;
    healingAnim = false;
    healingAnimTimer = 0.0f;
    burrowAnim = false;
    burrowUnderStarted = false;
    burrowAnimTimer = 0.0f;
    burrowunderAnim = false;
    burrowunderAnimTimer = 0.0f;
    meleeAnim = false;
    meleeAnimTimer = 0.0f;
    damageAnim = false;
    damageAnimTimer = 0.0f;
    shootRequested = false;
    keyInventory.clear();
    //atkCooldown = 0.0f;
    footstepCooldown = 0.0f;
    lastFootFrame = -1;
    hasNormalMeshScale = false;
    burrowMeshScaled = false;
    meleeMeshScaled = false;
    hasNormalColliderSize = false;
    burrowColliderScaled = false;
    meleeColliderScaled = false;
    absorbAnim = false;
    absorbAnimTimer = 0.0f;
    projectileTransformAnim = false;
    projectileTransformAnimTimer = 0.0f;
    projectileFormActive = false;
    stunTimer = 0.0f;
    if (footEmitter) footEmitter->enabled = false;
}

bool PlayerController::IsMeleeAnimating() const {
    return meleeAnim;
}

bool PlayerController::IsBurrowAnimating() const {
    return burrowAnim || burrowunderAnim;
}
