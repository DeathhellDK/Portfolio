#pragma once
/**
 * @file    vertex2d.h
 * @author  Jethro
 * @email   sung.h
 * @date    2025-09-29
 *
 * @brief   Declares the Vertex2D struct for 2D rendering.
 *
 * Vertex2D defines the memory layout for a single vertex in a
 * 2D mesh: position, texture coordinates, and color. It is
 * tightly packed (28 bytes) to ensure predictable layout when
 * passed to OpenGL through a VBO.
 *
 * @version 1.0
 */
#include "Math/vect2.h"
#include "Math/vect3.h"

#pragma pack(push, 1) // force no padding
 /**
  * @struct Vertex2D
  * @brief Represents a single 2D vertex (position, UV, color).
  *
  * Layout:
  * - pos[2]:   x, y coordinates in object space.
  * - texCoord[2]: u, v texture coordinates.
  * - color[3]: r, g, b per-vertex color.
  *
  * Used in Mesh2D to build vertex buffers. Must remain tightly
  * packed for OpenGL attribute pointers to work correctly.
  */
struct Vertex2D {
	float pos[2];      // x, y
	float texCoord[2]; // u, v
	float color[3];    // r, g, b

	// default constructor
	Vertex2D();
	/**
	 * @brief Constructs a vertex with given attributes.
	 * @param p 2D position vector.
	 * @param uv 2D texture coordinates.
	 * @param c 3D color vector (r, g, b).
	 */
	Vertex2D(const Vector2& p, const Vector2& uv, const Vector3& c);
};
#pragma pack(pop)

// compile-time check: must remain 28 bytes (2+2+3 floats).
//static_assert(sizeof(Vertex2D) == 28, "Vertex2D must be tightly packed (28 bytes).");
