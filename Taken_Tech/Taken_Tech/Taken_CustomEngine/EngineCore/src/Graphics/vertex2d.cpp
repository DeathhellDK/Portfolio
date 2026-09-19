/**
 * @file    vertex2d.cpp
 * @author  Jethro Sung
 * @email   sung.h
 * @date    2025-09-29
 *
 * @brief   Implements the Vertex2D struct.
 *
 * Provides constructors to initialize vertex attributes
 * (position, UV, color). Ensures GPU-friendly packing.
 *
 * @version 1.0
 */
#include "Graphics/vertex2d.h"

 /**
  * @brief Default-initializes the vertex.
  *
  * Position = (0,0), TexCoord = (0,0), Color = (1,1,1).
  * This is useful for zero-cost construction in arrays.
  */
Vertex2D::Vertex2D() {
    pos[0] = pos[1] = 0.f;
    texCoord[0] = texCoord[1] = 0.f;
    color[0] = color[1] = color[2] = 1.f;
}

/**
 * @brief Initializes the vertex with explicit position, UV, and color.
 *
 * @param p  Local-space position.
 * @param uv Texture coordinates.
 * @param c  RGB color.
 */
Vertex2D::Vertex2D(const Vector2& p, const Vector2& uv, const Vector3& c) {
    pos[0] = p.x; pos[1] = p.y;
    texCoord[0] = uv.x; texCoord[1] = uv.y;
    color[0] = c.x; color[1] = c.y; color[2] = c.z;
}