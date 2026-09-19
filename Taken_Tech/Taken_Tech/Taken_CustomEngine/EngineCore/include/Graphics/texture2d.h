#pragma once
/**
* @file     texture2d.h
* @author   Woh Kye Le
* @email    w.kyele
* @co-author
* @email
* @date     2025-09-26
*
* @brief Header file for Texture2D class
* This class acts as a RAII wrapper around an OpenGL 2D texture
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
#include <glad/glad.h>
#include <string>

/**
* @class Texture2D
* @brief Loads image bytes (via stb_image) and uploads to GL as a 2D texture.
*
* Notes:
*   - Non-copyable to avoid double-free of the GL name.
*   - Movable so textures can be transferred between owners safely.
*   - LoadFromFile() currently uses nearest filtering and no mipmaps,
*     ideal for pixel-art/sprite-sheet
*/
class Texture2D {
public:
    Texture2D();
    ~Texture2D();

    // non-copyable (not used as it will cause double deletion )
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    // movable
    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    // loading & binding
    bool LoadFromFile(const std::string& path, bool flipY = true);
    void Bind(unsigned unit = 0) const;

    // getters
    GLuint ID()     const { return id; }
    int    Width()  const { return w; }
    int    Height() const { return h; }

private:
    GLuint id;
    int w, h;
};

