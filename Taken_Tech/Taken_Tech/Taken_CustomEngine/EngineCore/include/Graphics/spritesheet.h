#pragma once
/**
* @file     spritesheet.h
* @author   Woh Kye Le
* @email    w.kyele
* @co-author
* @email
* @date 2025-09-26
*
* @brief Header file for SpriteSheet class
* This class dscribes a sprite sheet: GL texture + grid metadata
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
#include "Graphics/texture2d.h"
//include <array>

/**
* @struct SpriteSheet
* @brief Holds the underlying texture and per-frame layout.
*
* @details
*   - texture: GL texture for the whole sheet.
*   - frameW/H: size of a single frame in pixels.
*   - cols/rows: grid layout.
*   - frameDuration: default timing (optional metadata).
*   UV computation is performed in the shader using frame size + indices.
*/
struct SpriteSheet {
    Texture2D texture;
    int frameW = 0, frameH = 0;
    int cols = 0, rows = 0;      // frames laid out in grid
    float frameDuration = 0.1f;  // seconds per frame

};
