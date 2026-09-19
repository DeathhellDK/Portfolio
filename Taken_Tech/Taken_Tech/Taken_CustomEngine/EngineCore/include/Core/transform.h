#pragma once
/**
 * @file    transform.h
 * @author  Jethro Sung
 * @email    sung.h, sweeyongdillon.sng
 * @co-author Sng Swee Yong Dillon
 * @date    2025-11-7
 *
 * @brief   Declares the Transform component for 2D entities.
 *
 * The Transform component stores spatial information: world-space position,
 * scale, and rotation. It can also generate transformation matrices required
 * for rendering. Additionally, it supports:
 * - Parent-child hierarchical transforms (local vs world transforms)
 * - Caching of model matrices to avoid redundant matrix rebuilds
 * - Storage of previous and safe positions for interpolation and rollback
 *
 * Used by systems such as Movement, Collision, and Rendering.
 *
 * @version 1.1
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#include "Math/matrix3x3.h"
#include "Math/vect2.h"
#include "Core/component.h"
#include <cmath>

class IComponentContext;
 /**
  * @class Transform
  * @brief Represents the spatial transformation (position, scale, rotation)
  *        of an entity in world space.
  *
  * Inherits from Component to ensure association with an owning entity.
  * Provides:
  * - Local and world transform support
  * - Model matrix generation (T * R * S order)
  * - Caching and dirty-flag system for performance optimization
  * - Backup of previous and safe states for physics and interpolation
  */
class Transform : public Component { // we hv it inherit from component, so that it carries the entity id
public:
    static IComponentContext* context;
    Vector2 lastSafePosition; //prev frame loc 

    /**
     * @brief Constructs a Transform component.
     *
     * @param ownerId Entity ID that owns this Transform.
     * @param pos Initial position (default: origin).
     * @param scl Initial scale (default: 1,1).
     * @param rot Initial rotation in radians (default: 0).
     */
    Transform(Entity ownerId,
        const Vector2& pos = Vector2(0.0f, 0.0f),
        const Vector2& scl = Vector2(1.0f, 1.0f),
        float rot = 0.0f)
        : Component(ownerId), position(pos), scale(scl), rotation(rot), dirty(true) {
    }///we init, pos, scl, rot and set the owner id for the component

    // Setters
    /**
     * @brief Sets the world position.
     * @param pos The new position vector.
     */
    void SetPosition(const Vector2& pos);

    /**
     * @brief Sets the world scale.
     * @param scl The new scale vector.
     */
    void SetScale(const Vector2& scl);

    /**
     * @brief Sets the world rotation.
     * @param r The new rotation in radians.
     */
    void SetRotation(float r);

    // Getters
    /**
     * @brief Gets the world position.
     * @return A constant reference to the position vector.
     */
    const Vector2& GetPosition() const { return position; }

    /**
     * @brief Gets the world scale.
     * @return A constant reference to the scale vector.
     */
    const Vector2& GetScale() const { return scale; }

    /**
     * @brief Gets the world rotation.
     * @return The rotation in radians.
     */
    float GetRotation() const { return rotation; }

    // Get final transformation matrix
    /**
     * @brief Returns the cached transformation matrix, rebuilding if needed.
     * @return Matrix3x3 representing the transform.
     */
    Matrix3x3 GetMatrix();
    /**
    * @brief Builds the full 2D affine model matrix (T * R * S).
    * @return Matrix3x3 model transform.
    */
    Matrix3x3 GetModelMatrix() const; //build the 2d aff trans mtx

    /**
     * @brief Stores the current position into lastSafePosition.
     */
    void BackupPosition();

    // Components can override Update, but here we don�t need per-frame logic
    //void Update(float dt) override {}

    // ------------------------------------------------------------------------
    // Hierarchical transform support
    // ------------------------------------------------------------------------

    void SetParent(Entity p) { parent = p; }
    Entity GetParent() const { return parent; }

    void SetLocalPosition(const Vector2& p) { localPos = p; dirty = true; }
    void SetLocalScale(const Vector2& s) { localScale = s; dirty = true; }
    void SetLocalRotation(float r) { localRot = r; dirty = true; }

    const Vector2& GetLocalPosition() const { return localPos; }
    const Vector2& GetLocalScale()    const { return localScale; }
    float          GetLocalRotation() const { return localRot; }

    /**
     * @brief Rebuilds world-space transform based on parent transform.
     *
     * Must be called if this transform is part of a hierarchy and the
     * parent's world transform has changed.
     *
     * @param parentT Pointer to the parent's Transform component.
     */
    void ComposeFromParent(const Transform* parentT);

    /**
     * @brief Stores current world position into previous state for interpolation.
     */
    void SavePreviousState();

    Matrix3x3 GetInterpolatedModel(float alpha) const;

private:
    Vector2 position;
    Vector2 scale;
    float   rotation; // radians

    Vector2 prevPosition;
    Vector2 prevScale;
    float   prevRotation = 0.f;

    Matrix3x3 cachedMatrix;
    bool dirty; // mark when recalculation is needed

    Entity  parent = INVALID_ENTITY;
    Vector2 localPos = { 0.f, 0.f };
    Vector2 localScale = { 1.f, 1.f };
    float   localRot = 0.f;
};