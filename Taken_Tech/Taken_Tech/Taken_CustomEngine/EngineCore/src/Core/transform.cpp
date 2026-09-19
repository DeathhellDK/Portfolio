/**
 * @file     transform.cpp
 * @author   Jethro Sung
 * @email    sung.h, sweeyongdillon.sng
 * @co-author Sng Swee Yong Dillon
 * @date      2025-11-07
 *
 * @brief    Implements Transform component world/local TRS composition and matrix utilities.
 *
 * Transform defines the spatial state of an entity in the 2D world:
 *  - World position (top-left origin)
 *  - Scale (width, height before pivot offset)
 *  - Rotation (radians, clockwise, center-pivoted)
 *  - Parent/child hierarchy for attachment (e.g., Sword to Player hand)
 *
 * Key features:
 *  - Dirty-flagged matrix caching for performance
 *  - Local vs world transforms, supporting nested gameplay rigs
 *  - Backup/rollback state for CollisionSystem safety
 *  - Interpolated model matrix for smooth rendering when physics is discrete
 *
 * Matrix pipeline:
 *    T * PivotBack * R * PivotToCenter * S
 * ensures rotation occurs around the visual center instead of top-left
 *
 * @see Transform
 * @see CollisionSystem
 * @see ScriptSystem
 */
#include "Core/transform.h"
#include "Editor/UndoRedoManager.h"
#include "Core/componentcontext.h"

/**
 * @brief Returns this transform's cached world matrix.
 *
 * Rebuilds the matrix only when the `dirty` flag is set.
 * Uses TRS composition
 *
 * @return Cached Matrix3x3 representation of the transform.
 */
Matrix3x3 Transform::GetMatrix() {
    if (dirty) {
        Matrix3x3 t = Matrix3x3::BuildTranslation(position.x, position.y);
        Matrix3x3 r = Matrix3x3::BuildRotation(rotation);
        Matrix3x3 s = Matrix3x3::BuildScaling(scale.x, scale.y);

        cachedMatrix = t * r * s;
        dirty = false;
    }
    return cachedMatrix;
}

/**
 * @brief Builds and returns the full world-space model matrix.
 *
 * This matrix rotates and scales **around the sprite center** by translating
 * the pivot to the center before rotation, then restoring the offset.
 *
 * @code
 * M = T * PivotBack * R * PivotToCenter * S
 * @endcode
 *
 * @return Center-pivoted Matrix3x3 for rendering.
 */
Matrix3x3 Transform::GetModelMatrix() const {
    // Build scaling
    Matrix3x3 S = Matrix3x3::BuildScaling(scale.x, scale.y);

    // Build rotation
    Matrix3x3 R = Matrix3x3::BuildRotation(rotation);

    // Build translation
    Matrix3x3 T = Matrix3x3::BuildTranslation(position.x, position.y);
    Matrix3x3 pivotToCenter = Matrix3x3::BuildTranslation(-0.5f * scale.x, -0.5f * scale.y);
    Matrix3x3 pivotBack = Matrix3x3::BuildTranslation(0.5f * scale.x, 0.5f * scale.y);

    // Final = T * R * S (scale, then rotate, then move)
	return T * pivotBack * R * pivotToCenter * S;// this is so that we move the mesh around its center before rotating, and then move it backe
}

/**
 * @brief Saves current world-space position as safe rollback position.
 *
 * Used by collision system to restore a valid state if movement must be undone.
 */
void Transform::BackupPosition() {
    lastSafePosition = position;
}

/**
 * @brief Updates world-space transform based on local values relative to a parent.
 *
 * Composition rules:
 * - Rotation: parentRotation + localRotation
 * - Position: parentPosition + rotated(localPosition)
 * - Scale: child scale unaffected by parent scale in current implementation
 *
 * @param parentT Pointer to parent's Transform, or nullptr if root.
 */
void Transform::ComposeFromParent(const Transform* parentT)
{
    if (parentT) {
        // rotation: add
        rotation = parentT->GetRotation() + localRot;

        // position: parentPos + rotate(localPos, parentRot)
        float c = std::cos(parentT->GetRotation()), s = std::sin(parentT->GetRotation());
        Vector2 r{ c * localPos.x - s * localPos.y, s * localPos.x + c * localPos.y };
        position = parentT->GetPosition() + r;

        // scale: keep child size independent of parent
        scale = localScale;
    }
    else {
        // root: world == local
        position = localPos;
        scale = localScale;
        rotation = localRot;
    }
    dirty = true;

}

// --- Root-safe setters: keep locals in sync when there is no parent ---
/**
 * @brief Sets world-space position and updates local if root.
 */
void Transform::SetPosition(const Vector2& pos) {

    position = pos;
    // If this transform is a ROOT (no parent), mirror world -> local so
    // ComposeFromParent() (root branch) won't snap us back next frame.
    if (GetParent() == INVALID_ENTITY) {
        // If you have SetLocalPosition(...), prefer calling it:
        // SetLocalPosition(pos);
        // else write directly:
        localPos = pos;
    }
    dirty = true;
}

/**
 * @brief Sets world-space scale and updates local if root.
 */
void Transform::SetScale(const Vector2& scl) {
    scale = scl;
    if (GetParent() == INVALID_ENTITY) {
        // SetLocalScale(scl);
        localScale = scl;
    }
    dirty = true;
}

/**
 * @brief Sets world-space rotation and updates local if root.
 */
void Transform::SetRotation(float r) {
   
    rotation = r;
    if (GetParent() == INVALID_ENTITY) {
        // SetLocalRotation(r);
        localRot = r;
    }
    dirty = true;
}

/**
 * @brief Stores previous frame state for interpolation support.
 *
 * Required by GetInterpolatedModel() when fixed-timestep physics is used.
 */
void Transform::SavePreviousState() {
    prevPosition = position;
    prevScale = scale;
    prevRotation = rotation;
}

/**
    * @brief Interpolates between previous and current transform.
    * @param alpha Fraction [0,1) between previous (0) and current (1).
    * @return Interpolated 3×3 model matrix for smooth rendering.
    */
Matrix3x3 Transform::GetInterpolatedModel(float alpha) const {
    auto Lerp = [](float a, float b, float t) { return a + (b - a) * t; };
    Vector2 interpPos{
        Lerp(prevPosition.x, position.x, alpha),
        Lerp(prevPosition.y, position.y, alpha)
    };
    Vector2 interpScale{
        Lerp(prevScale.x, scale.x, alpha),
        Lerp(prevScale.y, scale.y, alpha)
    };
    float interpRot = Lerp(prevRotation, rotation, alpha);

    Matrix3x3 S = Matrix3x3::BuildScaling(interpScale.x, interpScale.y);
    Matrix3x3 R = Matrix3x3::BuildRotation(interpRot);
    Matrix3x3 T = Matrix3x3::BuildTranslation(interpPos.x, interpPos.y);
    Matrix3x3 pivotToCenter = Matrix3x3::BuildTranslation(-0.5f * interpScale.x, -0.5f * interpScale.y);
    Matrix3x3 pivotBack = Matrix3x3::BuildTranslation(0.5f * interpScale.x, 0.5f * interpScale.y);

    return T * pivotBack * R * pivotToCenter * S;
}
