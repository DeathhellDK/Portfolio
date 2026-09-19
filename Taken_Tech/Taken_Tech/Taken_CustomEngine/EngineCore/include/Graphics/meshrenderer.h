#pragma once
/**
 * @file    meshrenderer.h
 * @author  Jethro Sung
 * @email    sung.h, w.kyele
 * @Co-author Woh Kye Le
 * @date    2025-09-29
 *
 * @brief   Declares the MeshRenderer component for drawable entities.
 *
 * MeshRenderer is an ECS component that associates an entity with a
 * Mesh2D (geometry), a color tint, and an optional texture ID.
 * It does not perform rendering directly; instead, the RenderSystem
 * queries MeshRenderer instances and issues draw calls through Renderer.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#include "Graphics/mesh2d.h"
#include "Math/matrix3x3.h"
#include "Math/vect3.h"
#include "Core/component.h"
#include "Graphics/material.h"
#include <glad/glad.h>

class Mesh2D;

/**
* @class MeshRenderer
* @brief ECS component storing mesh, color, and texture for rendering.
*
* Provides setter/getter methods for texture, color, and mesh.
* Actual rendering is deferred to RenderSystem, which reads from
* MeshRenderer and associated Transform components.
*/
class MeshRenderer : public Component {
public:
    /**
    * @brief Constructs a MeshRenderer component.
    *
    * @param ownerId Entity ID that owns this component.
    * @param mesh Pointer to a Mesh2D object (geometry).
    * @param color Base color tint applied when drawing.
    * @param texture OpenGL texture ID (0 = no texture).
    */
    MeshRenderer(Entity ownerId, Mesh2D* mesh, const Vector3& color, GLuint texture = 0) : Component(ownerId), mesh(mesh) {
        mat.SetColor(color);
        mat.SetTexture(texture);
    }

    // no draw funcion here as rendering is handled by renderSystems

    // setters
    /**
     * @brief Assigns a texture to this renderer.
     * @param id OpenGL texture handle (0 = no texture).
     */
    void SetTexture(GLuint id) { mat.SetTexture(id); }
    /**
     * @brief Updates the base tint color.
     * @param c New RGB color vector.
     */
    void SetColor(const Vector3& c) { mat.SetColor(c); }
    /**
     * @brief Replaces the mesh pointer.
     * @param m Pointer to a Mesh2D object.
     */
    void SetMesh(Mesh2D* m) { mesh = m; }

    // --- getters used by RenderSystem ---
    // @return Pointer to associated Mesh2D (may be null).
    Mesh2D* GetMesh()    const { return mesh; }

    // @return Constant ref to current tint color.
    const Vector3& GetColor()   const { return mat.GetColor(); }

    // @return OpenGL texture ID (0 = no texture).
    GLuint  GetTexture() const { return mat.GetTexture(); }

    /** @return Const reference to material. */
    const Material& GetMaterial() const { return mat; }

    /** @return Mutable reference to material. */
    Material& GetMaterial() { return mat; }

    // public so RenderSystem can access directly
	Mesh2D* mesh = nullptr; // Pointer to the mesh to draw (may be null)
    Material mat;
};