/**
    * @file UndoRedoManager.cpp
    * @author  Jethro Sung, Sng Swee Yong Dillon
    * @email   sung.h, sweeyongdillon.sng
	* @date	2025/11/29
    * @brief implements the editor�s full stack-based undo/redo system, allowing every editing action�such as moving objects, scaling them, rotating them, creating new entities, 
    * or deleting existing ones�to be reversed or re-applied at any time. The system records each operation as an UndoRecord containing before/after snapshots of transform values or
    * entity-lifecycle data.
*/
#include "Editor/UndoRedoManager.h"
#include "Core/componentcontext.h"
#include "Core/entitymanager.h"
#include "Core/transform.h"
#include "Graphics/meshrenderer.h"
#include "Graphics/spriteanimator.h"
#include "Physics/collider.h"
#include "Particle/particleEmitter.h"
#include "Light/lightComponent.h"
#include <iostream>

/**
    * @brief Captures the current Transform state of an entity.
    *
    * Builds a TransformSnapshot containing position, scale, and rotation
    * for the specified entity. If the entity does not have a Transform,
    * the snapshot contains default values.
    *
    * @param ctx Component context used to query the Transform.
    * @param e   Entity ID to snapshot.
    * @return TransformSnapshot containing the entity�s transform data.
*/
//static TransformSnapshot CaptureSnapshot(IComponentContext& ctx, Entity e)
//{
//    TransformSnapshot snap;
//    snap.entity = e;
//
//    if (const Transform* t = ctx.TryGetTransform(e))
//    {
//        snap.position = t->GetPosition();
//        snap.scale = t->GetScale();
//        snap.rotation = t->GetRotation();
//    }
//    return snap;
//}

/**
    * @brief Constructs an UndoRedoManager bound to a component context.
    *
    * This manager uses IComponentContext to:
    * - Read and apply transform changes
    * - Destroy and recreate entities
    * - Maintain stable entity IDs for undoable deletion/creation
    *
    * @param ctx Reference to the ECS component context.
*/
UndoRedoManager::UndoRedoManager(IComponentContext& ctx)
    : context(ctx)
{
}

/*
static MeshRendererSnapshot CaptureMeshRendererSnapshot(IComponentContext& ctx, Entity e)
{
    MeshRendererSnapshot snap{};
    snap.entity = e;

    if (const MeshRenderer* mr = ctx.TryGetRenderer(e))
    {
        snap.hasRenderer = true;
        snap.color = mr->GetColor();
        snap.texture = static_cast<unsigned>(mr->GetTexture());
        snap.mesh = mr->GetMesh();
    }

    return snap;
}
*/

/**
 * @brief Captures the current state of a LightComponent for undo/redo purposes.
 * 
 * This function retrieves all properties of a LightComponent attached to the specified entity,
 * including general settings, glow, source, ember, and flicker effects.
 * 
 * @param ctx Reference to the IComponentContext.
 * @param e The entity ID to capture the light snapshot from.
 * @return LightSnapshot containing the captured light data.
 */
LightSnapshot CaptureLightSnapshot(IComponentContext& ctx, Entity e)
{
    LightSnapshot snap{};
    snap.entity = e;

    if (const LightComponent* l = ctx.TryGetLight(e))
    {
        snap.hasLight = true;

        snap.enabled = l->enabled;

        snap.glowEnabled = l->glow.enabled;
        snap.glowRadius = l->glow.radius;
        snap.glowColor = l->glow.color;
        snap.glowOffset = l->glow.offset;
        snap.glowIntensity = l->glow.intensity;
        snap.glowOpacity = l->glow.opacity;
        snap.glowSoftness = l->glow.softness;

        snap.sourceEnabled = l->source.enabled;
        snap.sourceRadius = l->source.radius;
        snap.sourceColor = l->source.color;
        snap.sourceOffset = l->source.offset;
        snap.sourceIntensity = l->source.intensity;
        snap.sourceOpacity = l->source.opacity;
        snap.sourceAttenuation = l->source.attenuation;

        snap.emberEnabled = l->ember.enabled;
        snap.emberRate = l->ember.rate;
        snap.emberParticleLife = l->ember.particleLife;
        snap.emberVelMin = l->ember.velMin;
        snap.emberVelMax = l->ember.velMax;
        snap.emberColorStart = l->ember.colorStart;
        snap.emberColorEnd = l->ember.colorEnd;
        snap.emberSizeStart = l->ember.sizeStart;
        snap.emberSizeEnd = l->ember.sizeEnd;
        snap.emberAdditive = l->ember.additive;
        snap.emberOffset = l->ember.offset;

        snap.flickerEnabled = l->flicker.enabled;
        snap.flickerIntensityMin = l->flicker.intensityMin;
        snap.flickerIntensityMax = l->flicker.intensityMax;
        snap.flickerSpeed = l->flicker.speed;
        snap.flickerTime = l->flicker.time;
    }

    return snap;
}

/**
 * @brief Applies a captured LightSnapshot to an entity.
 * 
 * This function restores the state of a LightComponent on the specified entity based on the
 * provided snapshot. If the snapshot indicates the light component should exist, it is created
 * or updated. If not, it is removed if present.
 * 
 * @param ctx Reference to the IComponentContext.
 * @param snap The LightSnapshot containing the state to apply.
 */
void ApplyLightSnapshot(IComponentContext& ctx, const LightSnapshot& snap)
{
    if (snap.hasLight)
    {
        LightComponent* l = ctx.GetLight(snap.entity);
        if (!l)
            l = ctx.AddLightComponent(snap.entity);

        if (!l) return;

        l->enabled = snap.enabled;

        l->glow.enabled = snap.glowEnabled;
        l->glow.radius = snap.glowRadius;
        l->glow.color = snap.glowColor;
        l->glow.offset = snap.glowOffset;
        l->glow.intensity = snap.glowIntensity;
        l->glow.opacity = snap.glowOpacity;
        l->glow.softness = snap.glowSoftness;

        l->source.enabled = snap.sourceEnabled;
        l->source.radius = snap.sourceRadius;
        l->source.color = snap.sourceColor;
        l->source.offset = snap.sourceOffset;
        l->source.intensity = snap.sourceIntensity;
        l->source.opacity = snap.sourceOpacity;
        l->source.attenuation = snap.sourceAttenuation;

        l->ember.enabled = snap.emberEnabled;
        l->ember.rate = snap.emberRate;
        l->ember.particleLife = snap.emberParticleLife;
        l->ember.velMin = snap.emberVelMin;
        l->ember.velMax = snap.emberVelMax;
        l->ember.colorStart = snap.emberColorStart;
        l->ember.colorEnd = snap.emberColorEnd;
        l->ember.sizeStart = snap.emberSizeStart;
        l->ember.sizeEnd = snap.emberSizeEnd;
        l->ember.additive = snap.emberAdditive;
        l->ember.offset = snap.emberOffset;

        l->flicker.enabled = snap.flickerEnabled;
        l->flicker.intensityMin = snap.flickerIntensityMin;
        l->flicker.intensityMax = snap.flickerIntensityMax;
        l->flicker.speed = snap.flickerSpeed;
        l->flicker.time = snap.flickerTime;
    }
    else
    {
        if (ctx.TryGetLight(snap.entity))
            ctx.RemoveLightComponent(snap.entity);
    }
}

/**
    * @brief Records a transform modification for Undo/Redo.
    *
    * Pushes a TransformEdit record containing the old and new values for
    * position, scale, and rotation. Any new change invalidates the redo stack.
    *
    * @param e        The entity being modified.
    * @param oldPos   Previous world position.
    * @param newPos   New world position.
    * @param oldScale Previous scale value.
    * @param newScale New scale value.
    * @param oldRot   Previous rotation (degrees).
    * @param newRot   New rotation (degrees).
*/
void UndoRedoManager::Push_TransformUpdate(
    Entity e,
    const Vector2& oldPos, const Vector2& newPos,
    const Vector2& oldScale, const Vector2& newScale,
    float oldRot, float newRot)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::TransformEdit;

    rec.before.entity = e;
    rec.before.position = oldPos;
    rec.before.scale = oldScale;
    rec.before.rotation = oldRot;

    rec.after.entity = e;
    rec.after.position = newPos;
    rec.after.scale = newScale;
    rec.after.rotation = newRot;

    undoStack.push_back(rec);

    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

/**
 * @brief Records a collider update for undo/redo.
 * 
 * This function creates an UndoRecord for a modification to a Collider component, storing
 * the previous and new size and trigger status.
 * 
 * @param e The entity ID.
 * @param oldSize The previous size of the collider.
 * @param newSize The new size of the collider.
 * @param oldTrigger The previous trigger status.
 * @param newTrigger The new trigger status.
 */
void UndoRedoManager::Push_ColliderUpdate(
    Entity e,
    const Vector2& oldSize, const Vector2& newSize,
    bool oldTrigger, bool newTrigger)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::ColliderEdit;

    rec.beforeCollider.entity = e;
    rec.beforeCollider.hasCollider = true;
    rec.beforeCollider.size = oldSize;
    rec.beforeCollider.isTrigger = oldTrigger;

    rec.afterCollider.entity = e;
    rec.afterCollider.hasCollider = true;
    rec.afterCollider.size = newSize;
    rec.afterCollider.isTrigger = newTrigger;

    undoStack.push_back(rec);
    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

/**
 * @brief Records a collider toggle operation for undo/redo.
 * 
 * This function creates an UndoRecord for adding or removing a Collider component, or changing
 * its properties significantly, storing the state before and after the operation.
 * 
 * @param e The entity ID.
 * @param beforeHasCollider Whether the entity had a collider before.
 * @param beforeSize The size of the collider before.
 * @param beforeTrigger The trigger status before.
 * @param afterHasCollider Whether the entity has a collider after.
 * @param afterSize The size of the collider after.
 * @param afterTrigger The trigger status after.
 */
void UndoRedoManager::Push_ColliderToggle(
    Entity e,
    bool beforeHasCollider,
    const Vector2& beforeSize,
    bool beforeTrigger,
    bool afterHasCollider,
    const Vector2& afterSize,
    bool afterTrigger)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::ColliderToggle;

    rec.beforeCollider.entity = e;
    rec.beforeCollider.hasCollider = beforeHasCollider;
    rec.beforeCollider.size = beforeSize;
    rec.beforeCollider.isTrigger = beforeTrigger;

    rec.afterCollider.entity = e;
    rec.afterCollider.hasCollider = afterHasCollider;
    rec.afterCollider.size = afterSize;
    rec.afterCollider.isTrigger = afterTrigger;

    undoStack.push_back(rec);
    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

/**
 * @brief Records a transform update along with collider state for undo/redo.
 * 
 * This function captures changes to both the Transform and Collider components simultaneously,
 * useful for operations that might affect both (e.g., scaling an object which affects collider size).
 * 
 * @param e The entity ID.
 * @param oldPos Previous position.
 * @param newPos New position.
 * @param oldScale Previous scale.
 * @param newScale New scale.
 * @param oldRot Previous rotation.
 * @param newRot New rotation.
 * @param hadColliderBefore Whether the entity had a collider before.
 * @param oldColSize Previous collider size.
 * @param newColSize New collider size.
 * @param oldColTrigger Previous collider trigger status.
 * @param newColTrigger New collider trigger status.
 */
void UndoRedoManager::Push_TransformUpdateWithCollider(
    Entity e,
    const Vector2& oldPos, const Vector2& newPos,
    const Vector2& oldScale, const Vector2& newScale,
    float oldRot, float newRot,
    bool hadColliderBefore,
    const Vector2& oldColSize, const Vector2& newColSize,
    bool oldColTrigger, bool newColTrigger)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::TransformEdit;

    rec.before.entity = e;
    rec.before.position = oldPos;
    rec.before.scale = oldScale;
    rec.before.rotation = oldRot;

    rec.after.entity = e;
    rec.after.position = newPos;
    rec.after.scale = newScale;
    rec.after.rotation = newRot;

    rec.beforeCollider.entity = e;
    rec.beforeCollider.hasCollider = hadColliderBefore;
    rec.beforeCollider.size = oldColSize;
    rec.beforeCollider.isTrigger = oldColTrigger;

    rec.afterCollider.entity = e;
    rec.afterCollider.hasCollider = hadColliderBefore;
    rec.afterCollider.size = newColSize;
    rec.afterCollider.isTrigger = newColTrigger;

    undoStack.push_back(rec);
    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

/**
 * @brief Records a mesh renderer edit for undo/redo.
 * 
 * This function stores the previous and new state of a MeshRenderer component, including color,
 * texture, and the mesh reference itself.
 * 
 * @param e The entity ID.
 * @param oldColor Previous color.
 * @param newColor New color.
 * @param oldTexture Previous texture ID.
 * @param newTexture New texture ID.
 * @param oldMesh Previous Mesh2D pointer.
 * @param newMesh New Mesh2D pointer.
 */
void UndoRedoManager::Push_MeshRendererEdit(
    Entity e,
    const Vector3& oldColor, const Vector3& newColor,
    unsigned oldTexture, unsigned newTexture,
    Mesh2D* oldMesh, Mesh2D* newMesh)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::MeshRendererEdit;

    rec.beforeRenderer.entity = e;
    rec.beforeRenderer.hasRenderer = true;
    rec.beforeRenderer.color = oldColor;
    rec.beforeRenderer.texture = oldTexture;
    rec.beforeRenderer.mesh = oldMesh;

    rec.afterRenderer.entity = e;
    rec.afterRenderer.hasRenderer = true;
    rec.afterRenderer.color = newColor;
    rec.afterRenderer.texture = newTexture;
    rec.afterRenderer.mesh = newMesh;

    undoStack.push_back(rec);
    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

/**
 * @brief Records a mesh renderer toggle for undo/redo.
 * 
 * This function handles the addition or removal of a MeshRenderer component, as well as
 * significant property changes that might accompany a toggle.
 * 
 * @param e The entity ID.
 * @param hadRendererBefore Whether the entity had a renderer before.
 * @param oldColor Previous color.
 * @param oldTexture Previous texture ID.
 * @param oldMesh Previous Mesh2D pointer.
 * @param hasRendererAfter Whether the entity has a renderer after.
 * @param newColor New color.
 * @param newTexture New texture ID.
 * @param newMesh New Mesh2D pointer.
 */
void UndoRedoManager::Push_MeshRendererToggle(
    Entity e,
    bool hadRendererBefore,
    const Vector3& oldColor,
    unsigned oldTexture,
    Mesh2D* oldMesh,
    bool hasRendererAfter,
    const Vector3& newColor,
    unsigned newTexture,
    Mesh2D* newMesh)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::MeshRendererToggle;

    rec.beforeRenderer.entity = e;
    rec.beforeRenderer.hasRenderer = hadRendererBefore;
    rec.beforeRenderer.color = oldColor;
    rec.beforeRenderer.texture = oldTexture;
    rec.beforeRenderer.mesh = oldMesh;

    rec.afterRenderer.entity = e;
    rec.afterRenderer.hasRenderer = hasRendererAfter;
    rec.afterRenderer.color = newColor;
    rec.afterRenderer.texture = newTexture;
    rec.afterRenderer.mesh = newMesh;

    undoStack.push_back(rec);
    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

/**
 * @brief Records a sprite animator edit for undo/redo.
 * 
 * This function captures changes to a SpriteAnimator component, including current frame,
 * animation range, speed, and the sprite sheet used.
 * 
 * @param e The entity ID.
 * @param oldCur Previous current frame.
 * @param newCur New current frame.
 * @param oldStart Previous start frame.
 * @param newStart New start frame.
 * @param oldEnd Previous end frame.
 * @param newEnd New end frame.
 * @param oldSpeed Previous animation speed.
 * @param newSpeed New animation speed.
 * @param oldSheet Previous sprite sheet.
 * @param newSheet New sprite sheet.
 */
void UndoRedoManager::Push_SpriteAnimatorEdit(
    Entity e,
    int oldCur, int newCur,
    int oldStart, int newStart,
    int oldEnd, int newEnd,
    float oldSpeed, float newSpeed,
    const SpriteSheet* oldSheet, const SpriteSheet* newSheet)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::SpriteAnimatorEdit;

    rec.beforeAnimator.entity = e;
    rec.beforeAnimator.hasAnimator = true;
    rec.beforeAnimator.cur = oldCur;
    rec.beforeAnimator.startFrame = oldStart;
    rec.beforeAnimator.endFrame = oldEnd;
    rec.beforeAnimator.speed = oldSpeed;
    rec.beforeAnimator.sheet = oldSheet;

    rec.afterAnimator.entity = e;
    rec.afterAnimator.hasAnimator = true;
    rec.afterAnimator.cur = newCur;
    rec.afterAnimator.startFrame = newStart;
    rec.afterAnimator.endFrame = newEnd;
    rec.afterAnimator.speed = newSpeed;
    rec.afterAnimator.sheet = newSheet;

    undoStack.push_back(rec);
    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

/**
 * @brief Records a sprite animator toggle for undo/redo.
 * 
 * This function handles the addition or removal of a SpriteAnimator component, storing
 * the complete state of the animator before and after the operation.
 * 
 * @param e The entity ID.
 * @param beforeHasAnimator Whether the entity had an animator before.
 * @param beforeCur Previous current frame.
 * @param beforeStart Previous start frame.
 * @param beforeEnd Previous end frame.
 * @param beforeSpeed Previous animation speed.
 * @param beforeSheet Previous sprite sheet.
 * @param afterHasAnimator Whether the entity has an animator after.
 * @param afterCur New current frame.
 * @param afterStart New start frame.
 * @param afterEnd New end frame.
 * @param afterSpeed New animation speed.
 * @param afterSheet New sprite sheet.
 */
void UndoRedoManager::Push_SpriteAnimatorToggle(
    Entity e,
    bool beforeHasAnimator,
    int beforeCur,
    int beforeStart,
    int beforeEnd,
    float beforeSpeed,
    const SpriteSheet* beforeSheet,
    bool afterHasAnimator,
    int afterCur,
    int afterStart,
    int afterEnd,
    float afterSpeed,
    const SpriteSheet* afterSheet)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::SpriteAnimatorToggle;

    rec.beforeAnimator.entity = e;
    rec.beforeAnimator.hasAnimator = beforeHasAnimator;
    rec.beforeAnimator.cur = beforeCur;
    rec.beforeAnimator.startFrame = beforeStart;
    rec.beforeAnimator.endFrame = beforeEnd;
    rec.beforeAnimator.speed = beforeSpeed;
    rec.beforeAnimator.sheet = beforeSheet;

    rec.afterAnimator.entity = e;
    rec.afterAnimator.hasAnimator = afterHasAnimator;
    rec.afterAnimator.cur = afterCur;
    rec.afterAnimator.startFrame = afterStart;
    rec.afterAnimator.endFrame = afterEnd;
    rec.afterAnimator.speed = afterSpeed;
    rec.afterAnimator.sheet = afterSheet;

    undoStack.push_back(rec);
    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

/**
 * @brief Records a particle emitter edit for undo/redo.
 * 
 * This function captures detailed changes to a ParticleEmitter component, including rate,
 * lifetime, velocity ranges, colors, sizes, and texture properties.
 * 
 * @param e The entity ID.
 * @param oldEnabled Previous enabled state.
 * @param newEnabled New enabled state.
 * @param oldRate Previous emission rate.
 * @param newRate New emission rate.
 * @param oldParticleLife Previous particle lifetime.
 * @param newParticleLife New particle lifetime.
 * @param oldOffset Previous offset.
 * @param newOffset New offset.
 * @param oldVelMin Previous min velocity.
 * @param newVelMin New min velocity.
 * @param oldVelMax Previous max velocity.
 * @param newVelMax New max velocity.
 * @param oldColorStart Previous start color.
 * @param newColorStart New start color.
 * @param oldColorEnd Previous end color.
 * @param newColorEnd New end color.
 * @param oldSizeStart Previous start size.
 * @param newSizeStart New start size.
 * @param oldSizeEnd Previous end size.
 * @param newSizeEnd New end size.
 * @param oldQuad Previous mesh quad.
 * @param newQuad New mesh quad.
 * @param oldTexture Previous texture ID.
 * @param newTexture New texture ID.
 * @param oldTimeAccumulator Previous time accumulator.
 * @param newTimeAccumulator New time accumulator.
 */
void UndoRedoManager::Push_ParticleEmitterEdit(
    Entity e,
    bool oldEnabled,
    bool newEnabled,
    float oldRate,
    float newRate,
    float oldParticleLife,
    float newParticleLife,
    const Vector2& oldOffset,
    const Vector2& newOffset,
    const Vector2& oldVelMin,
    const Vector2& newVelMin,
    const Vector2& oldVelMax,
    const Vector2& newVelMax,
    const Vector3& oldColorStart,
    const Vector3& newColorStart,
    const Vector3& oldColorEnd,
    const Vector3& newColorEnd,
    float oldSizeStart,
    float newSizeStart,
    float oldSizeEnd,
    float newSizeEnd,
    Mesh2D* oldQuad,
    Mesh2D* newQuad,
    unsigned oldTexture,
    unsigned newTexture,
    float oldTimeAccumulator,
    float newTimeAccumulator)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::ParticleEmitterEdit;

    rec.beforeEmitter.entity = e;
    rec.beforeEmitter.hasEmitter = true;
    rec.beforeEmitter.enabled = oldEnabled;
    rec.beforeEmitter.rate = oldRate;
    rec.beforeEmitter.particleLife = oldParticleLife;
    rec.beforeEmitter.offset = oldOffset;
    rec.beforeEmitter.velMin = oldVelMin;
    rec.beforeEmitter.velMax = oldVelMax;
    rec.beforeEmitter.colorStart = oldColorStart;
    rec.beforeEmitter.colorEnd = oldColorEnd;
    rec.beforeEmitter.sizeStart = oldSizeStart;
    rec.beforeEmitter.sizeEnd = oldSizeEnd;
    rec.beforeEmitter.quad = oldQuad;
    rec.beforeEmitter.texture = oldTexture;
    rec.beforeEmitter.timeAccumulator = oldTimeAccumulator;

    rec.afterEmitter.entity = e;
    rec.afterEmitter.hasEmitter = true;
    rec.afterEmitter.enabled = newEnabled;
    rec.afterEmitter.rate = newRate;
    rec.afterEmitter.particleLife = newParticleLife;
    rec.afterEmitter.offset = newOffset;
    rec.afterEmitter.velMin = newVelMin;
    rec.afterEmitter.velMax = newVelMax;
    rec.afterEmitter.colorStart = newColorStart;
    rec.afterEmitter.colorEnd = newColorEnd;
    rec.afterEmitter.sizeStart = newSizeStart;
    rec.afterEmitter.sizeEnd = newSizeEnd;
    rec.afterEmitter.quad = newQuad;
    rec.afterEmitter.texture = newTexture;
    rec.afterEmitter.timeAccumulator = newTimeAccumulator;

    undoStack.push_back(rec);
    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

/**
 * @brief Records a particle emitter toggle for undo/redo.
 * 
 * This function handles the addition or removal of a ParticleEmitter component, storing
 * all properties to ensure the emitter can be perfectly restored or removed.
 * 
 * @param e The entity ID.
 * @param beforeHasEmitter Whether the entity had an emitter before.
 * @param beforeEnabled Previous enabled state.
 * @param beforeRate Previous emission rate.
 * @param beforeParticleLife Previous particle lifetime.
 * @param beforeOffset Previous offset.
 * @param beforeVelMin Previous min velocity.
 * @param beforeVelMax Previous max velocity.
 * @param beforeColorStart Previous start color.
 * @param beforeColorEnd Previous end color.
 * @param beforeSizeStart Previous start size.
 * @param beforeSizeEnd Previous end size.
 * @param beforeQuad Previous mesh quad.
 * @param beforeTexture Previous texture ID.
 * @param beforeTimeAccumulator Previous time accumulator.
 * @param afterHasEmitter Whether the entity has an emitter after.
 * @param afterEnabled New enabled state.
 * @param afterRate New emission rate.
 * @param afterParticleLife New particle lifetime.
 * @param afterOffset New offset.
 * @param afterVelMin New min velocity.
 * @param afterVelMax New max velocity.
 * @param afterColorStart New start color.
 * @param afterColorEnd New end color.
 * @param afterSizeStart New start size.
 * @param afterSizeEnd New end size.
 * @param afterQuad New mesh quad.
 * @param afterTexture New texture ID.
 * @param afterTimeAccumulator New time accumulator.
 */
void UndoRedoManager::Push_ParticleEmitterToggle(
    Entity e,
    bool beforeHasEmitter,
    bool beforeEnabled,
    float beforeRate,
    float beforeParticleLife,
    const Vector2& beforeOffset,
    const Vector2& beforeVelMin,
    const Vector2& beforeVelMax,
    const Vector3& beforeColorStart,
    const Vector3& beforeColorEnd,
    float beforeSizeStart,
    float beforeSizeEnd,
    Mesh2D* beforeQuad,
    unsigned beforeTexture,
    float beforeTimeAccumulator,
    bool afterHasEmitter,
    bool afterEnabled,
    float afterRate,
    float afterParticleLife,
    const Vector2& afterOffset,
    const Vector2& afterVelMin,
    const Vector2& afterVelMax,
    const Vector3& afterColorStart,
    const Vector3& afterColorEnd,
    float afterSizeStart,
    float afterSizeEnd,
    Mesh2D* afterQuad,
    unsigned afterTexture,
    float afterTimeAccumulator)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::ParticleEmitterToggle;

    rec.beforeEmitter.entity = e;
    rec.beforeEmitter.hasEmitter = beforeHasEmitter;
    rec.beforeEmitter.enabled = beforeEnabled;
    rec.beforeEmitter.rate = beforeRate;
    rec.beforeEmitter.particleLife = beforeParticleLife;
    rec.beforeEmitter.offset = beforeOffset;
    rec.beforeEmitter.velMin = beforeVelMin;
    rec.beforeEmitter.velMax = beforeVelMax;
    rec.beforeEmitter.colorStart = beforeColorStart;
    rec.beforeEmitter.colorEnd = beforeColorEnd;
    rec.beforeEmitter.sizeStart = beforeSizeStart;
    rec.beforeEmitter.sizeEnd = beforeSizeEnd;
    rec.beforeEmitter.quad = beforeQuad;
    rec.beforeEmitter.texture = beforeTexture;
    rec.beforeEmitter.timeAccumulator = beforeTimeAccumulator;

    rec.afterEmitter.entity = e;
    rec.afterEmitter.hasEmitter = afterHasEmitter;
    rec.afterEmitter.enabled = afterEnabled;
    rec.afterEmitter.rate = afterRate;
    rec.afterEmitter.particleLife = afterParticleLife;
    rec.afterEmitter.offset = afterOffset;
    rec.afterEmitter.velMin = afterVelMin;
    rec.afterEmitter.velMax = afterVelMax;
    rec.afterEmitter.colorStart = afterColorStart;
    rec.afterEmitter.colorEnd = afterColorEnd;
    rec.afterEmitter.sizeStart = afterSizeStart;
    rec.afterEmitter.sizeEnd = afterSizeEnd;
    rec.afterEmitter.quad = afterQuad;
    rec.afterEmitter.texture = afterTexture;
    rec.afterEmitter.timeAccumulator = afterTimeAccumulator;

    undoStack.push_back(rec);
    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

/**
 * @brief Records a script edit for undo/redo.
 * 
 * This function tracks changes to the script attached to an entity, storing the
 * script name before and after the change.
 * 
 * @param e The entity ID.
 * @param beforeHasScript Whether the entity had a script before.
 * @param beforeName Previous script name.
 * @param afterHasScript Whether the entity has a script after.
 * @param afterName New script name.
 */
void UndoRedoManager::Push_ScriptEdit(
    Entity e,
    bool beforeHasScript,
    const std::string& beforeName,
    bool afterHasScript,
    const std::string& afterName)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::ScriptEdit;

    rec.beforeScript.entity = e;
    rec.beforeScript.hasScript = beforeHasScript;
    rec.beforeScript.scriptName = beforeName;

    rec.afterScript.entity = e;
    rec.afterScript.hasScript = afterHasScript;
    rec.afterScript.scriptName = afterName;

    undoStack.push_back(std::move(rec));
    redoStack.clear();

    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());
}

/**
 * @brief Records a player controller toggle for undo/redo.
 * 
 * This function tracks the addition or removal of a PlayerController component.
 * 
 * @param e The entity ID.
 * @param beforeHasController Whether the entity had a controller before.
 * @param afterHasController Whether the entity has a controller after.
 */
void UndoRedoManager::Push_PlayerControllerToggle(
    Entity e,
    bool beforeHasController,
    bool afterHasController)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::PlayerControllerToggle;

    rec.beforeController.entity = e;
    rec.beforeController.hasController = beforeHasController;

    rec.afterController.entity = e;
    rec.afterController.hasController = afterHasController;

    undoStack.push_back(std::move(rec));
    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

/**
 * @brief Records a speed component toggle for undo/redo.
 * 
 * This function tracks the addition or removal of a SpeedComponent.
 * 
 * @param e The entity ID.
 * @param beforeHasSpeed Whether the entity had a speed component before.
 * @param afterHasSpeed Whether the entity has a speed component after.
 */
void UndoRedoManager::Push_SpeedComponentToggle(
    Entity e,
    bool beforeHasSpeed,
    bool afterHasSpeed)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::SpeedComponentToggle;

    rec.beforeSpeed.entity = e;
    rec.beforeSpeed.hasSpeed = beforeHasSpeed;

    rec.afterSpeed.entity = e;
    rec.afterSpeed.hasSpeed = afterHasSpeed;

    undoStack.push_back(std::move(rec));
    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

/**
 * @brief Records a mass component toggle for undo/redo.
 * 
 * This function tracks the addition or removal of a MassComponent.
 * 
 * @param e The entity ID.
 * @param beforeHasMass Whether the entity had a mass component before.
 * @param afterHasMass Whether the entity has a mass component after.
 */
void UndoRedoManager::Push_MassComponentToggle(
    Entity e,
    bool beforeHasMass,
    bool afterHasMass)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::MassComponentToggle;

    rec.beforeMass.entity = e;
    rec.beforeMass.hasMass = beforeHasMass;

    rec.afterMass.entity = e;
    rec.afterMass.hasMass = afterHasMass;

    undoStack.push_back(std::move(rec));
    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

/**
 * @brief Records a persistent tag toggle for undo/redo.
 * 
 * This function tracks the addition or removal of the persistent tag on an entity.
 * 
 * @param e The entity ID.
 * @param beforeHasPersistentTag Whether the entity had the persistent tag before.
 * @param afterHasPersistentTag Whether the entity has the persistent tag after.
 */
void UndoRedoManager::Push_PersistentTagToggle(
    Entity e,
    bool beforeHasPersistentTag,
    bool afterHasPersistentTag)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::PersistentTagToggle;

    rec.beforePersistentTag.entity = e;
    rec.beforePersistentTag.hasPersistentTag = beforeHasPersistentTag;

    rec.afterPersistentTag.entity = e;
    rec.afterPersistentTag.hasPersistentTag = afterHasPersistentTag;

    undoStack.push_back(std::move(rec));
    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

/**
 * @brief Records a light edit for undo/redo.
 * 
 * This function captures changes to a LightComponent using full snapshots of the
 * light's state before and after the modification.
 * 
 * @param e The entity ID.
 * @param beforeSnap The light snapshot before the change.
 * @param afterSnap The light snapshot after the change.
 */
void UndoRedoManager::Push_LightEdit(
    Entity e,
    const LightSnapshot& beforeSnap,
    const LightSnapshot& afterSnap)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::LightEdit;
    rec.beforeLight = beforeSnap;
    rec.afterLight = afterSnap;
    rec.beforeLight.entity = e;
    rec.afterLight.entity = e;

    undoStack.push_back(std::move(rec));
    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());
    redoStack.clear();
}

/**
 * @brief Records a light toggle for undo/redo.
 * 
 * This function tracks the addition or removal of a LightComponent, using snapshots
 * to store the state.
 * 
 * @param e The entity ID.
 * @param beforeSnap The light snapshot before the operation.
 * @param afterSnap The light snapshot after the operation.
 */
void UndoRedoManager::Push_LightToggle(
    Entity e,
    const LightSnapshot& beforeSnap,
    const LightSnapshot& afterSnap)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::LightToggle;
    rec.beforeLight = beforeSnap;
    rec.afterLight = afterSnap;
    rec.beforeLight.entity = e;
    rec.afterLight.entity = e;

    undoStack.push_back(std::move(rec));
    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());
    redoStack.clear();
}

/**
 * @brief Records an enemy controller toggle for undo/redo.
 * 
 * This function tracks the addition or removal of an EnemyController component.
 * 
 * @param e The entity ID.
 * @param beforeHasEnemyController Whether the entity had an enemy controller before.
 * @param afterHasEnemyController Whether the entity has an enemy controller after.
 */
void UndoRedoManager::Push_EnemyControllerToggle(
    Entity e,
    bool beforeHasEnemyController,
    bool afterHasEnemyController)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::EnemyControllerToggle;

    rec.beforeEnemyController.entity = e;
    rec.beforeEnemyController.hasEnemyController = beforeHasEnemyController;

    rec.afterEnemyController.entity = e;
    rec.afterEnemyController.hasEnemyController = afterHasEnemyController;

    undoStack.push_back(rec);
    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

/**
    * @brief Records that an entity has been created.
    *
    * Undoing this event deletes the entity.
    * Redoing this event recreates the entity with the same ID and
    * initial Transform values.
    *
    * @param e ID of the newly created entity.
*/
void UndoRedoManager::Push_EntityCreated(Entity e)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::EntityCreated;

    rec.entityInfo.entity = e;
    rec.entityInfo.existedBefore = false; // undo = delete

    const Transform* t = context.TryGetTransform(e);
    const MeshRenderer* mr = context.TryGetRenderer(e);
    const SpriteAnimator* anim = context.TryGetAnimator(e);
    const Collider* col = context.TryGetCollider(e);
    const PlayerController* pc = context.TryGetController(e);
    const ParticleEmitter* em = context.TryGetEmitter(e);
    const SpeedComponent* spd = context.TryGetSpeed(e);
    const MassComponent* mass = context.TryGetMass(e);

    rec.entityInfo.hasTransform = (t != nullptr);
    rec.entityInfo.hasRenderer = (mr != nullptr);
    rec.entityInfo.hasAnimator = (anim != nullptr);
    rec.entityInfo.hasCollider = (col != nullptr);
    rec.entityInfo.hasController = (pc != nullptr);
    rec.entityInfo.hasEmitter = (em != nullptr);
    rec.entityInfo.hasSpeed = (spd != nullptr);
    rec.entityInfo.hasMass = (mass != nullptr);
    rec.entityInfo.hasPersistentTag = context.HasPersistentTag(e);
    rec.entityInfo.hasLight = (context.TryGetLight(e) != nullptr);
    rec.entityInfo.light = CaptureLightSnapshot(context, e);
    rec.entityInfo.hasEnemyController = (context.TryGetEnemyController(e) != nullptr);

    std::string sName = context.GetEntityScriptName(e);
    if (!sName.empty() && sName != "None") {
        rec.entityInfo.hasScript = true;
        rec.entityInfo.scriptName = sName;
    }
    else {
        rec.entityInfo.hasScript = false;
        rec.entityInfo.scriptName.clear();
    }

    // Capture transform at creation time (if any)
    if (t)
    {
        rec.entityInfo.transform.entity = e;
        rec.entityInfo.transform.position = t->GetPosition();
        rec.entityInfo.transform.scale = t->GetScale();
        rec.entityInfo.transform.rotation = t->GetRotation();
    }

    if (mr)
    {
        rec.entityInfo.rendererColor = mr->GetColor();
        rec.entityInfo.rendererTexture = static_cast<unsigned>(mr->GetTexture());
    }

    if (anim)
    {
        rec.entityInfo.animSheet = anim->sheet;
        rec.entityInfo.animStart = anim->startFrame;
        rec.entityInfo.animEnd = anim->endFrame;
        rec.entityInfo.animCur = anim->cur;
        rec.entityInfo.animSpeed = anim->speed;
    }

    if (col)
    {
        rec.entityInfo.colliderSize = col->size;
        rec.entityInfo.colliderIsTrigger = col->isTrigger;
    }

    undoStack.push_back(rec);
    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

/**
    * @brief Records that an entity was deleted.
    *
    * Stores the entity ID and its Transform values so that undo can
    * recreate it in the exact state it was in before deletion.
    * Redoing the event deletes the entity again.
    *
    * @param e     Entity that was deleted.
    * @param pos   Last known world position.
    * @param scale Last known scale.
    * @param rot   Last known rotation (degrees).
*/
void UndoRedoManager::Push_EntityDeleted(Entity e,
    const Vector2& pos, const Vector2& scale, float rot)
{
    UndoRecord rec{};
    rec.action = UndoRecord::Type::EntityDeleted;

    rec.entityInfo.entity = e;
    rec.entityInfo.existedBefore = true; // undo = recreate
    rec.entityInfo.transform.entity = e;
    rec.entityInfo.transform.position = pos;
    rec.entityInfo.transform.scale = scale;
    rec.entityInfo.transform.rotation = rot;

    // --- Capture which components exist just before deletion ---
    const Transform* t = context.TryGetTransform(e);
    const MeshRenderer* mr = context.TryGetRenderer(e);
    const SpriteAnimator* anim = context.TryGetAnimator(e);
    const Collider* col = context.TryGetCollider(e);
    const PlayerController* pc = context.TryGetController(e);
    const ParticleEmitter* em = context.TryGetEmitter(e);
    const SpeedComponent* spd = context.TryGetSpeed(e);
    const MassComponent* mass = context.TryGetMass(e);

    rec.entityInfo.hasTransform = (t != nullptr);
    rec.entityInfo.hasRenderer = (mr != nullptr);
    rec.entityInfo.hasAnimator = (anim != nullptr);
    rec.entityInfo.hasCollider = (col != nullptr);
    rec.entityInfo.hasController = (pc != nullptr);
    rec.entityInfo.hasEmitter = (em != nullptr);
    rec.entityInfo.hasSpeed = (spd != nullptr);
    rec.entityInfo.hasMass = (mass != nullptr);
    rec.entityInfo.hasPersistentTag = context.HasPersistentTag(e);
    rec.entityInfo.hasLight = (context.TryGetLight(e) != nullptr);
    rec.entityInfo.light = CaptureLightSnapshot(context, e);
    rec.entityInfo.hasEnemyController = (context.TryGetEnemyController(e) != nullptr);

    std::string sName = context.GetEntityScriptName(e);
    if (!sName.empty() && sName != "None") {
        rec.entityInfo.hasScript = true;
        rec.entityInfo.scriptName = sName;
    }
    else {
        rec.entityInfo.hasScript = false;
        rec.entityInfo.scriptName.clear();
    }

    if (mr)
    {
        rec.entityInfo.rendererColor = mr->GetColor();
        rec.entityInfo.rendererTexture = static_cast<unsigned>(mr->GetTexture());
    }

    if (anim)
    {
        rec.entityInfo.animSheet = anim->sheet;
        rec.entityInfo.animStart = anim->startFrame;
        rec.entityInfo.animEnd = anim->endFrame;
        rec.entityInfo.animCur = anim->cur;
        rec.entityInfo.animSpeed = anim->speed;
    }

    if (col)
    {
        rec.entityInfo.colliderSize = col->size;
        rec.entityInfo.colliderIsTrigger = col->isTrigger;
    }

    undoStack.push_back(rec);
    if (undoStack.size() > MAX_STACK)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

/**
    * @brief Applies a single UndoRecord in either undo or redo direction.
    *
    * Behavior depends on record type:
    * - TransformEdit: restores previous or next Transform state.
    * - EntityCreated: undo deletes; redo recreates the entity.
    * - EntityDeleted: undo recreates; redo deletes the entity.
    *
    * @param rec      The record to apply.
    * @param undoing  True if applying an Undo; false if applying a Redo.
*/
void UndoRedoManager::ApplyRecord(const UndoRecord& rec, bool undoing)
{
    //Entity e = rec.before.entity;

    switch (rec.action)
    {
        // ----------------------------------------------------------
        // TRANSFORM EDIT
        // ----------------------------------------------------------
    case UndoRecord::Type::TransformEdit:
    {
        const TransformSnapshot& snap = undoing ? rec.before : rec.after;
        const ColliderSnapshot& colSnap = undoing ? rec.beforeCollider : rec.afterCollider;

        if (Transform* t = context.GetTransform(snap.entity))
        {
            t->SetPosition(snap.position);
            t->SetScale(snap.scale);
            t->SetRotation(snap.rotation);
        }

        if (colSnap.hasCollider)
        {
            if (Collider* c = context.GetCollider(colSnap.entity))
            {
                c->size = colSnap.size;
                c->isTrigger = colSnap.isTrigger;
            }
        }
        break;
    }
    case UndoRecord::Type::ColliderEdit:
    {
        const ColliderSnapshot& snap = undoing ? rec.beforeCollider : rec.afterCollider;

        if (snap.hasCollider)
        {
            if (Collider* c = context.GetCollider(snap.entity))
            {
                c->size = snap.size;
                c->isTrigger = snap.isTrigger;
            }
        }
        break;
    }
    case UndoRecord::Type::ColliderToggle:
    {
        const ColliderSnapshot& snap = undoing ? rec.beforeCollider : rec.afterCollider;

        if (snap.hasCollider)
        {
            Collider* c = context.GetCollider(snap.entity);
            if (!c)
                c = context.AddColliderComponent(snap.entity);

            if (c)
            {
                c->size = snap.size;
                c->isTrigger = snap.isTrigger;
            }
        }
        else
        {
            if (context.GetCollider(snap.entity))
                context.RemoveColliderComponent(snap.entity);
        }
        break;
    }
    case UndoRecord::Type::MeshRendererEdit:
    {
        const MeshRendererSnapshot& snap = undoing ? rec.beforeRenderer : rec.afterRenderer;

        if (snap.hasRenderer)
        {
            if (MeshRenderer* mr = context.GetRenderer(snap.entity))
            {
                mr->SetColor(snap.color);
                mr->SetTexture(static_cast<unsigned>(snap.texture));
                mr->SetMesh(snap.mesh);
            }
        }
        break;
    }

    case UndoRecord::Type::MeshRendererToggle:
    {
        const MeshRendererSnapshot& snap = undoing ? rec.beforeRenderer : rec.afterRenderer;

        if (snap.hasRenderer)
        {
            MeshRenderer* mr = context.GetRenderer(snap.entity);
            if (!mr)
                mr = context.AddRendererComponent(snap.entity);

            if (mr)
            {
                mr->SetColor(snap.color);
                mr->SetTexture(static_cast<unsigned>(snap.texture));
                mr->SetMesh(snap.mesh);
            }
        }
        else
        {
            if (context.GetRenderer(snap.entity))
                context.RemoveRendererComponent(snap.entity);
        }
        break;
    }

    case UndoRecord::Type::SpriteAnimatorEdit:
    {
        const auto& snap = undoing ? rec.beforeAnimator : rec.afterAnimator;

        if (snap.hasAnimator)
        {
            if (SpriteAnimator* a = context.GetAnimator(snap.entity))
            {
                a->cur = snap.cur;
                a->startFrame = snap.startFrame;
                a->endFrame = snap.endFrame;
                a->speed = snap.speed;
                a->sheet = snap.sheet;
            }
        }
        break;
    }

    case UndoRecord::Type::SpriteAnimatorToggle:
    {
        const auto& snap = undoing ? rec.beforeAnimator : rec.afterAnimator;

        if (snap.hasAnimator)
        {
            SpriteAnimator* a = context.GetAnimator(snap.entity);
            if (!a)
                a = context.AddAnimatorComponent(snap.entity);

            if (a)
            {
                a->cur = snap.cur;
                a->startFrame = snap.startFrame;
                a->endFrame = snap.endFrame;
                a->speed = snap.speed;
                a->sheet = snap.sheet;
            }
        }
        else
        {
            if (context.GetAnimator(snap.entity))
                context.RemoveAnimatorComponent(snap.entity);
        }
        break;
    }

    case UndoRecord::Type::ParticleEmitterEdit:
    {
        const auto& snap = undoing ? rec.beforeEmitter : rec.afterEmitter;

        if (snap.hasEmitter)
        {
            if (ParticleEmitter* em = context.GetEmitter(snap.entity))
            {
                em->enabled = snap.enabled;
                em->rate = snap.rate;
                em->particleLife = snap.particleLife;
                em->offset = snap.offset;
                em->velMin = snap.velMin;
                em->velMax = snap.velMax;
                em->colorStart = snap.colorStart;
                em->colorEnd = snap.colorEnd;
                em->sizeStart = snap.sizeStart;
                em->sizeEnd = snap.sizeEnd;
                em->quad = snap.quad;
                em->texture = snap.texture;
                em->timeAccumulator = snap.timeAccumulator;
            }
        }
        break;
    }

    case UndoRecord::Type::ParticleEmitterToggle:
    {
        const auto& snap = undoing ? rec.beforeEmitter : rec.afterEmitter;

        if (snap.hasEmitter)
        {
            ParticleEmitter* em = context.GetEmitter(snap.entity);
            if (!em)
                em = context.AddEmitterComponent(snap.entity);

            if (em)
            {
                em->enabled = snap.enabled;
                em->rate = snap.rate;
                em->particleLife = snap.particleLife;
                em->offset = snap.offset;
                em->velMin = snap.velMin;
                em->velMax = snap.velMax;
                em->colorStart = snap.colorStart;
                em->colorEnd = snap.colorEnd;
                em->sizeStart = snap.sizeStart;
                em->sizeEnd = snap.sizeEnd;
                em->quad = snap.quad;
                em->texture = snap.texture;
                em->timeAccumulator = snap.timeAccumulator;
            }
        }
        else
        {
            if (context.GetEmitter(snap.entity))
                context.RemoveEmitterComponent(snap.entity);
        }
        break;
    }

    case UndoRecord::Type::ScriptEdit:
    {
        const ScriptSnapshot& snap = undoing ? rec.beforeScript : rec.afterScript;

        if (!snap.hasScript || snap.scriptName.empty() || snap.scriptName == "None")
        {
            context.SetEntityScriptName(snap.entity, "None");
        }
        else
        {
            context.SetEntityScriptName(snap.entity, snap.scriptName);
        }
        break;
    }

    case UndoRecord::Type::PlayerControllerToggle:
    {
        const PlayerControllerSnapshot& snap =
            undoing ? rec.beforeController : rec.afterController;

        if (snap.hasController)
        {
            if (!context.GetController(snap.entity))
                context.AddControllerComponent(snap.entity);
        }
        else
        {
            if (context.GetController(snap.entity))
                context.RemoveControllerComponent(snap.entity);
        }
        break;
    }

    case UndoRecord::Type::SpeedComponentToggle:
    {
        const SpeedComponentSnapshot& snap =
            undoing ? rec.beforeSpeed : rec.afterSpeed;

        if (snap.hasSpeed)
        {
            if (context.TryGetSpeed(snap.entity) == nullptr)
                context.AddSpeedComponent(snap.entity);
        }
        else
        {
            if (context.TryGetSpeed(snap.entity) != nullptr)
                context.RemoveSpeedComponent(snap.entity);
        }
        break;
    }

    case UndoRecord::Type::MassComponentToggle:
    {
        const MassComponentSnapshot& snap =
            undoing ? rec.beforeMass : rec.afterMass;

        if (snap.hasMass)
        {
            if (context.TryGetMass(snap.entity) == nullptr)
                context.AddMassComponent(snap.entity);
        }
        else
        {
            if (context.TryGetMass(snap.entity) != nullptr)
                context.RemoveMassComponent(snap.entity);
        }
        break;
    }
    case UndoRecord::Type::PersistentTagToggle:
    {
        const PersistentTagSnapshot& snap =
            undoing ? rec.beforePersistentTag : rec.afterPersistentTag;

        if (snap.hasPersistentTag)
        {
            if (!context.HasPersistentTag(snap.entity))
                context.AddPersistentTag(snap.entity);
        }
        else
        {
            if (context.HasPersistentTag(snap.entity))
                context.RemovePersistentTag(snap.entity);
        }
        break;
    }

    case UndoRecord::Type::LightEdit:
    {
        const LightSnapshot& snap = undoing ? rec.beforeLight : rec.afterLight;
        ApplyLightSnapshot(context, snap);
        break;
    }

    case UndoRecord::Type::LightToggle:
    {
        const LightSnapshot& snap = undoing ? rec.beforeLight : rec.afterLight;
        ApplyLightSnapshot(context, snap);
        break;
    }

    case UndoRecord::Type::EnemyControllerToggle:
    {
        const EnemyControllerSnapshot& snap =
            undoing ? rec.beforeEnemyController : rec.afterEnemyController;

        if (snap.hasEnemyController)
        {
            if (!context.TryGetEnemyController(snap.entity))
                context.AddEnemyControllerComponent(snap.entity);
        }
        else
        {
            if (context.TryGetEnemyController(snap.entity))
                context.RemoveEnemyControllerComponent(snap.entity);
        }
        break;
    }

    // ----------------------------------------------------------
    // ENTITY CREATED undo = delete, redo = recreate
    // ----------------------------------------------------------
    case UndoRecord::Type::EntityCreated:
    {
        const EntitySnapshot& es = rec.entityInfo;
        if (undoing)
        {
            context.DestroyEntityImmediate(es.entity);
        }
        else
        {
            // Redo creation, recreate entity with same ID and components
            Entity re = context.CreateEntityWithFixedID(es.entity);

            // Recreate components via IComponentContext bridge
            if (es.hasRenderer)   context.AddRendererComponent(re);
            if (es.hasCollider)   context.AddColliderComponent(re);
            if (es.hasAnimator)   context.AddAnimatorComponent(re);
            if (es.hasController) context.AddControllerComponent(re);
            if (es.hasEmitter)    context.AddEmitterComponent(re);
            if (es.hasSpeed)      context.AddSpeedComponent(re);
            if (es.hasMass)       context.AddMassComponent(re);
            if (es.hasPersistentTag) context.AddPersistentTag(re);
            if (es.hasEnemyController) context.AddEnemyControllerComponent(re);
            
            if (es.hasScript && !es.scriptName.empty()) {
                context.SetEntityScriptName(re, es.scriptName);
            }

			// Restore renderer state
            if (es.hasRenderer)
            {
                if (MeshRenderer* mr = context.GetRenderer(re))
                {
                    mr->SetColor(es.rendererColor);
                    mr->SetTexture(static_cast<unsigned>(es.rendererTexture));
                }
            }

            if (es.hasAnimator)
            {
                if (SpriteAnimator* a = context.GetAnimator(re))
                {
                    a->SetSheet(es.animSheet, false); // false = don't reset cur
                    a->startFrame = es.animStart;
                    a->endFrame = es.animEnd;
                    a->cur = es.animCur;
                    a->speed = es.animSpeed;
                }
            }

            // Restore transform
            if (Transform* t = context.GetTransform(re))
            {
                t->SetPosition(es.transform.position);
                t->SetScale(es.transform.scale);
                t->SetRotation(es.transform.rotation);
            }

            if (es.hasCollider)
            {
                if (Collider* c = context.GetCollider(re))
                {
                    c->size = es.colliderSize;
                    c->isTrigger = es.colliderIsTrigger;
                }
            }
            if (es.hasLight)
                ApplyLightSnapshot(context, es.light);
        }
        break;
    }

    // ----------------------------------------------------------
    // ENTITY DELETED undo = recreate entity, redo = delete
    // ----------------------------------------------------------
    case UndoRecord::Type::EntityDeleted:
    {
        const EntitySnapshot& es = rec.entityInfo;
        if (undoing)
        {
            // Undo deletion -> recreate entity with previous components
            Entity re = context.CreateEntityWithFixedID(es.entity);

            if (es.hasRenderer)   context.AddRendererComponent(re);
            if (es.hasCollider)   context.AddColliderComponent(re);
            if (es.hasAnimator)   context.AddAnimatorComponent(re);
            if (es.hasController) context.AddControllerComponent(re);
            if (es.hasEmitter)    context.AddEmitterComponent(re);
            if (es.hasSpeed)      context.AddSpeedComponent(re);
            if (es.hasMass)       context.AddMassComponent(re);
            if (es.hasPersistentTag) context.AddPersistentTag(re);
            if (es.hasEnemyController) context.AddEnemyControllerComponent(re);

            if (es.hasScript && !es.scriptName.empty()) {
                context.SetEntityScriptName(re, es.scriptName);
            }

            // restore renderer state
            if (es.hasRenderer)
            {
                if (MeshRenderer* mr = context.GetRenderer(re))
                {
                    mr->SetColor(es.rendererColor);
                    mr->SetTexture(static_cast<unsigned>(es.rendererTexture));
                }
            }

            if (es.hasAnimator)
            {
                if (SpriteAnimator* a = context.GetAnimator(re))
                {
                    a->SetSheet(es.animSheet, false);
                    a->startFrame = es.animStart;
                    a->endFrame = es.animEnd;
                    a->cur = es.animCur;
                    a->speed = es.animSpeed;
                }
            }

            if (Transform* t = context.GetTransform(re))
            {
                t->SetPosition(es.transform.position);
                t->SetScale(es.transform.scale);
                t->SetRotation(es.transform.rotation);
            }

            if (es.hasCollider)
            {
                if (Collider* c = context.GetCollider(re))
                {
                    c->size = es.colliderSize;
                    c->isTrigger = es.colliderIsTrigger;
                }
            }
            if (es.hasLight)
                ApplyLightSnapshot(context, es.light);
        }
        else
        {
            context.DestroyEntityImmediate(es.entity);
        }
        break;
    }

    } // switch
}

/**
    * @brief Performs an undo operation.
    *
    * Pops the most recent record from the undo stack, applies its undo
    * behavior via ApplyRecord(), and then updates the redo stack with an
    * appropriate forward snapshot.
    *
    * @note If the undo stack is empty, nothing happens.
*/
void UndoRedoManager::Undo()
{
    if (undoStack.empty())
        return;

    UndoRecord rec = undoStack.back();
    undoStack.pop_back();

    // Apply undo side
    ApplyRecord(rec, true);

    // ---- Update redo snapshot ----
    /*if (rec.action == UndoRecord::Type::TransformEdit)
    {
        rec.after = CaptureSnapshot(context, rec.before.entity);
    }*/
  /*  else
    {
        rec.entityInfo.transform = CaptureSnapshot(context, rec.entityInfo.entity);
    }*/

    redoStack.push_back(rec);
}

/**
    * @brief Performs a redo operation.
    *
    * Pops the most recent record from the redo stack, reapplies its forward
    * state using ApplyRecord(), then pushes a refreshed snapshot back into
    * the undo stack.
    *
    * @note If the redo stack is empty, nothing happens.
*/
void UndoRedoManager::Redo()
{
    if (redoStack.empty())
        return;

    UndoRecord rec = redoStack.back();
    redoStack.pop_back();

    ApplyRecord(rec, false);

    // ---- Update undo snapshot ----
    /*if (rec.action == UndoRecord::Type::TransformEdit)
    {
        rec.after = CaptureSnapshot(context, rec.before.entity);
    }*/
    //else
    //{
    //    // Refresh recreated or deleted entity state
    //    rec.entityInfo.transform = CaptureSnapshot(context, rec.entityInfo.entity);
    //}

    undoStack.push_back(rec);
}

