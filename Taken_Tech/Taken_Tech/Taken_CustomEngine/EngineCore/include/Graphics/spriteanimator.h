#pragma once
/**
* @file     spriteanimator.h
* @author   Woh Kye Le
* @email    w.kyele
* @co-author Jethro Sung
* @email    sung.h
* @date     2025-09-26
*
* @brief Header file for spriteanimator class
* This class is used for component storing sprite-sheet playback state
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "Core/component.h"
#include "Graphics/spritesheet.h"
#include "Graphics/mesh2d.h"
#include "Graphics/vertex2d.h"
#include <vector>

/**
* @class SpriteAnimator
* @brief Holds current frame and timing for sprite-sheet animation.
*
* Fields:
*   - sheet:   metadata + GL texture of the sheet (required for anim).
*   - cur:     current frame index.
*   - start/endFrame: looping range.
*   - speed:   frames per second.
*   - acc:     time accumulator (seconds).
*
* Helpers return the data required by the shader to compute UVs on GPU.
*/
class SpriteAnimator : public Component {
public:
    explicit SpriteAnimator(Entity ownerId) : Component(ownerId) {}

    // wiring
    const SpriteSheet* sheet = nullptr;  // required

    // playback
    int   startFrame = 0, endFrame = 0, cur = 0;
    float speed = 12.f;   // fps
    float acc = 0.f;      // accumulator

    // shader helpers
    /** @brief Gets the current frame index. */
    int   GetFrame() const { return cur; }
    
    /** @brief Gets the number of columns in the sprite sheet. */
    int   GetCols()  const { return sheet ? sheet->cols : 1; }
    
    /** @brief Gets the horizontal UV span of a single frame. */
    float FrameU()   const { return sheet ? float(sheet->frameW) / sheet->texture.Width() : 1.f; }
    
    /** @brief Gets the vertical UV span of a single frame. */
    float FrameV()   const { return sheet ? float(sheet->frameH) / sheet->texture.Height() : 1.f; }

    /**
     * @brief Assigns a new sprite sheet to the animator.
     * @param sh Pointer to the SpriteSheet.
     * @param reset Whether to reset the animation playback to the beginning.
     */
    void SetSheet(const SpriteSheet* sh, bool reset = true) {
        sheet = sh;

        if (!sheet) {
            startFrame = endFrame = cur = 0;
            acc = 0.f;
            return;
        }

        // default range = whole sheet
        startFrame = 0;
        endFrame = (sheet->cols * sheet->rows) - 1;

        if (reset) {
            cur = startFrame;
            acc = 0.f;
        }
    }

    // Set a custom frame range and reset to start.
    void SetRange(int start, int end, bool reset = true) {
        if (!sheet) return;
        const int maxFrame = (sheet->cols * sheet->rows) - 1;

        startFrame = std::max(0, std::min(start, maxFrame));
        endFrame = std::max(0, std::min(end, maxFrame));

        if (endFrame < startFrame) std::swap(startFrame, endFrame);

        if (reset) {
            cur = startFrame;
            acc = 0.f;
        }
    }

    void SetSpeedFromFrameDuration(float frameDuration, float fallbackFps = 12.f) {
        speed = (frameDuration > 0.f) ? (1.f / frameDuration) : fallbackFps;
    }

    // resets time/frame so it starts clean.
    void Restart() {
        if (!sheet) return;
        cur = startFrame;
        acc = 0.f;
    }
};
