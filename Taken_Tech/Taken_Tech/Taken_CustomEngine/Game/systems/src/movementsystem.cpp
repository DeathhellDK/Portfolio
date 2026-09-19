/**
 * @file      movementsystem.cpp
 * @author    Jethro Sung
 * @email    sung.h, sweeyongdillon.sng
 * @co-author Sng Swee Yong Dillon
 * @date      2025-11-7
 *
 * @brief    Implements MovementSystem, the ECS system responsible for handling
 *           player input-driven movement and animation state updates.
 *
 * The MovementSystem iterates over entities that contain both a
 * PlayerController and a Transform component. It delegates:
 *  - Input handling and movement logic to PlayerController
 *  - Render-facing directional and animation state to MeshRenderer +
 *    SpriteAnimator (bound as dependencies)
 *
 * This system only influences Transform through gameplay intent;
 * physical collision resolution and rollback are handled later by
 * CollisionSystem to prevent tunneling and wall phasing.
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
*/
#include "movementsystem.h"

/**
 * @brief Processes all Player-controlled entities and applies movement input.
 *
 * Execution Steps:
 *  1) Iterate through every entity signature registered in the ECS.
 *  2) Filter for entities whose signature matches MovementSystem requirements:
 *     - PlayerController component -> provides input-driven motion logic
 *     - Transform component         -> position to be modified
 *  3) Bind MeshRenderer + SpriteAnimator dependencies to the PlayerController:
 *     - Enables controller to trigger animation state changes (Idle/Walk/etc.)
 *     - Allows directional sprite flipping based on player facing
 *  4) Call PlayerController::Update() to apply gameplay inputs
 *
 * @param dt Time elapsed since previous frame, in seconds.
*/
void MovementSystem::Update(float dt)
{
    const auto& entities = context.GetEntitySignatures();
    const Signature& sysSig = GetSignature();

    for (auto it = entities.begin(); it != entities.end(); ++it)
    {
        Entity e = it->first;
        const Signature& sig = it->second;

        if ((sig & sysSig) != sysSig)
            continue; // entity doesn't match required components

        PlayerController* controller = context.GetController(e);
        Transform* tr = context.GetTransform(e);
        MeshRenderer* mr = context.GetRenderer(e);
        SpriteAnimator* anim = context.GetAnimator(e);
        Collider* col = context.GetCollider(e);

        if (!controller || !tr)
            continue;

        controller->BindRenderDeps(mr, anim);

        controller->Update(dt, *tr, col);
        controller->GetPhysicsSpeed(controller->getPlayerVel()/*, dt*/);
        /*controller->ResetSpeed()*/
    }
}

/**
 * @brief Performs the draw/update pass for player controller visuals.
 *
 * @param renderer Renderer interface
 */
void MovementSystem::Draw(Renderer& renderer)
{
    (void)renderer;

    const auto& entities = context.GetEntitySignatures();
    const Signature& sysSig = GetSignature();

    for (auto it = entities.begin(); it != entities.end(); ++it)
    {
        Entity e = it->first;
        const Signature& sig = it->second;

        if ((sig & sysSig) != sysSig)
            continue;

        PlayerController* controller = context.GetController(e);
        Transform* tr = context.GetTransform(e);

        if (!controller || !tr)
            continue;

        controller->Draw(*tr);
    }
}
