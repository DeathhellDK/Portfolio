#pragma once
/**
 * @file     material.h
 * @author   Woh kye Le 
 * @email    w.kyele
 * @date     2025-10-29
 *
 * @brief   Lightweight Material struct for tint + texture info.
 *
 * A Material describes how a mesh should be visually rendered:
 * its tint color and optional bound texture. This keeps visual
 * state separate from geometry (Mesh2D) and logic (Transform).
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include "Math/vect3.h"

using TextureHandle = unsigned; // alias for GLuint

class Material {
public:
    Material() = default;
    Material(const Vector3& color, TextureHandle tex = 0) : tint(color), texture(tex) {}

    // --- Getters ---
    const Vector3& GetColor()   const { return tint; }
    TextureHandle  GetTexture() const { return texture; }
    float          GetOpacity() const { return opacity; }
    int            GetLayer()   const { return layer; }

    // --- Setters ---
    void SetColor(const Vector3& c)  { tint = c; }
    void SetTexture(TextureHandle t) { texture = t; }
    void SetOpacity(float o)         { opacity = o; }
    void SetLayer(int l)             { layer = l; }

private:
    TextureHandle texture = 0;  // 0 = no texture
    Vector3 tint{ 1, 1, 1 };
    float opacity = 1.f;
    int layer = 0;
};