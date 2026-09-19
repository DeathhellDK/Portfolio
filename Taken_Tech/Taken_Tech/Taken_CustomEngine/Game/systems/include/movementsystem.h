#pragma once
/**
 * @class MovementSystem
 * @author    Jethro Sung
 * @email    sung.h, sweeyongdillon.sng
 * @co-author Sng Swee Yong Dillon
 * @date      2025-11-7
 * @brief Processes all player-controlled movement each frame.
 *
 * This system queries entities that have both Transform and PlayerController
 * components. For each such entity:
 *  - Dependencies (MeshRenderer + SpriteAnimator) are bound into the
 *    PlayerController for directional flipping + animation control
 *  - PlayerController::Update is invoked to apply input-based movement
 *
 */
#include "playerController.h"
#include "Core/transform.h"
#include "Core/Systems/systemManager.h"
#include "Graphics/meshrenderer.h"
#include "Graphics/spriteanimator.h"
#include <vector>
#include "Core/componentcontext.h"

class MovementSystem : public ISystem, public SystemBase {
    IComponentContext& context;
    float pixelsPerUnit = 1.f;

public:
    explicit MovementSystem(IComponentContext& ctx, float ppu = 1.f) // this indicates how many pixels for 1 unit in world coordinate
        : context(ctx), pixelsPerUnit(ppu)
    {
        Signature sig;
        sig.set(PLAYERCONTROLLER);
        sig.set(TRANSFORM);
        SetSignature(sig);
    }

    /**
     * @brief Updates movement logic for all entities controlled by PlayerController.
     * @param dt Delta time (seconds).
     */
    void Update(float dt) override;

    /**
     * @brief Draw pass for all PlayerController-controlled entities.
     *
     * Iterates over entities matching this system's signature and delegates rendering-side
     * updates (animation state, emitter toggles, footstep timing, etc.) to
     * PlayerController::Draw(). The Renderer reference is currently unused because
     * rendering is handled via bound component dependencies.
     *
     * @param renderer Renderer interface for the frame
     */
    void Draw(Renderer& renderer) override;
};
