/**
* @file     spriteanimator.cpp
* @author   Woh Kye Le
* @email    w.kyele
* @co-author
* @email
* @date     2025-09-26
*
* @brief    Free function that advances SpriteAnimator playback
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
#include "Graphics/spriteanimator.h"

/**
     * @brief Advances a SpriteAnimator by delta time.
     *
     * Increments the animator's internal time accumulator, determines when
     * to advance frames based on the playback speed, and wraps from endFrame
     * back to startFrame to maintain looping animations.
     *
     * Behavior:
     *  - Does nothing if no sprite sheet is assigned.
     *  - `acc` accumulates time until it exceeds one frame duration.
     *  - `cur` increments one frame at a time based on elapsed time.
     *  - On reaching endFrame, loops back to startFrame.
     *
     * @param an Reference to the SpriteAnimator component to update.
     * @param dt Delta time in seconds.
     *
     * @note This is intentionally a free func to avoid
     *       colliding with ISystem naming in the ECS architecture.
*/
void SpriteAnimator_Update(SpriteAnimator& an, float dt) {
    if (!an.sheet) return;
    an.acc += dt;                       // accumulate elapsed time
    const float step = 1.0f / an.speed; // duration of one frame in seconds
    while (an.acc >= step) {
        an.acc -= step;
        an.cur = (an.cur + 1 > an.endFrame) ? an.startFrame : (an.cur + 1);
    }
}

// this function is kept as a free function to avoid colliding with ISystem, this function 
// is called from animationSystem.h