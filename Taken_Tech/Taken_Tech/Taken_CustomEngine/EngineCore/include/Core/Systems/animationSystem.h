#pragma once
/**
* @file     animationSystem.h
* @author   Woh Kye Le
* @email    w.kyele
* @co-author Jethro Sung
* @email    sung.h
* @date     2025-09-26
*
* @brief Header file for AnimationSystem class
* This class is incharge of updating sprite animations (frame stepping) each tick.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "Core/Systems/systemManager.h"
#include "Graphics/spriteanimator.h"
#include <vector>
#include "Core/componentcontext.h"

/**
* @class AnimationSystem
* @brief Advances frame indices of SpriteAnimator components.
*
* @details
*   - Integrates time (dt) into an accumulator per animator.
*   - Steps to next frame when accumulated time exceeds 1/speed.
*   - Wraps to startFrame after reaching endFrame.
*/

class AnimationSystem : public ISystem, public SystemBase {
    IComponentContext& context;

    public:
        explicit AnimationSystem(IComponentContext& ctx);

        /**
         * @brief Advances animation playback for all matching entities.
         * @param dt Delta time (seconds).
         */
        void Update(float dt) override;
};
