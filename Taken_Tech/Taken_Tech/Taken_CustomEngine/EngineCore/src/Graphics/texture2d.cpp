/**
* @file     texture2d.cpp
* @author   Woh Kye Le
* @email    w.kyele ,t.weiliangterril
* @co-author Tan Wei Liang Terril
* @date     2025-09-26
*
* @brief    stb_image-based loader and GL texture creation
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "Graphics/texture2d.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>
#include "Input/DebugConsole.hpp"

/**
 * @brief Constructs an empty Texture2D.
 *
 * Initializes the handle to 0, meaning "no GL texture yet".
 */
Texture2D::Texture2D() : id(0), w(0), h(0) {}

/**
 * @brief Destructor.
 *
 * Deletes the OpenGL texture if one is currently owned.
 */
Texture2D::~Texture2D() { if (id) { glDeleteTextures(1, &id); } }

/**
 * @brief Move constructor.
 *
 * Transfers the GL texture handle and dimensions from @p other,
 * and clears @p other so it won't delete the texture on destruction.
 *
 * @param other Object to take ownership from.
 */
Texture2D::Texture2D(Texture2D&& other) noexcept : id(other.id), w(other.w), h(other.h) {
    other.id = 0;
    other.w = other.h = 0;
}

/**
 * @brief Move assignment operator.
 *
 * Releases any currently owned texture, then steals the GL handle
 * and metadata from @p other. Leaves @p other in a safe, empty state.
 *
 * @param other Object to take ownership from.
 * @return *this
 */
Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this != &other) {
        if (id) {
            glDeleteTextures(1, &id);
        }
        id = other.id;
        w = other.w;
        h = other.h;

        other.id = 0;
        other.w = other.h = 0;
    }
    return *this;
}

/**
 * @brief Loads image data from disk and uploads it as a GL texture.
 *
 * Steps:
 * 1. Destroys any existing texture owned by this object.
 * 2. Uses stb_image to decode the file into RGBA8.
 * 3. Creates and configures an OpenGL texture2D.
 *
 * The sampler is configured with:
 *  - GL_NEAREST min/mag filter (pixel-perfect / retro look)
 *  - GL_CLAMP_TO_EDGE wrap
 *  - No mipmaps (2D UI / sprites don't usually need them)
 *
 * @param path  Filesystem path to the texture image.
 * @param flipY If true, flip image vertically when loading to match OpenGL UV origin.
 * @return true if the texture loaded and uploaded successfully, false otherwise.
 */
bool Texture2D::LoadFromFile(const std::string& path, bool flipY) {
    if (id) { // delete any existing textures 
        glDeleteTextures(1, &id);
        id = 0;
    }

    // ensures correct vertical orientation 
    stbi_set_flip_vertically_on_load(flipY);
    int comp = 0;
    // using STB_IMAGE, force 4 channels (RGBA), stores dimension in w, h
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &comp, STBI_rgb_alpha);
    if (!data) {
        DebugConsole::Get().AddFormattedMessage(
            LogLevel::Error,
            "[Texture2D] Failed to load image: " + path + "\n"
        );
        return false;
    }

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    // handle sprite sheets that arent multiples of 4 bytes per row
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // no mipmaps for now; nearest so pixels stay crisp
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    return true;
}

/**
 * @brief Binds this texture to the specified texture unit.
 * @param unit Texture unit index (0 for GL_TEXTURE0, etc.).
 */
void Texture2D::Bind(unsigned unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}
