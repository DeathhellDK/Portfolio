#pragma once
/**
 * @file      componentcontext.h
 * @author    Jethro Sung
 * @email    sung.h, sweeyongdillon.sng
 * @co-author Sng Swee Yong Dillon
 * @date      2025-10-29
 *
 * @brief     Declares IComponentContext, the engine-facing interface used to
 *            access ECS data (components, entities, systems, etc.) without
 *            depending directly on the game-layer implementation.
 *
 * Systems like rendering, physics, editor overlay, scripting, etc. can
 * query and mutate components through this interface instead of coupling
 * to GameApp.
 * 
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */

#include <unordered_map>
#include "Core/component.h"
#include "Core/layering.h"
#include <string>

// Forward declaration
class Transform;
class MeshRenderer;
class SpriteAnimator;
class Collider;
class PlayerController;

class EnemiesController;

class ParticleEmitter;
class LightComponent;
class Mesh2D;
class SystemManager;
struct ScriptComponent;
enum class ScriptKind : uint8_t;
class UndoRedoManager;
struct SpeedComponent;
struct MassComponent;
class PersistentTag;

/**
 * @brief Interface used by Engine systems to access ECS component data.
 *
 * Implemented by the game layer (e.g., GameApp) to expose ECS components
 * to generic Engine systems (like RenderSystem, CollisionSystem, etc.)
 * without creating a dependency on the game layer.
 * It is like erm, a default gameapp interface that systems can use to access components
 */
class IComponentContext {
public:
    virtual ~IComponentContext() = default;

    // Access entity signatures
    virtual const std::unordered_map<Entity, Signature>& GetEntitySignatures() const = 0;

    // Component accessors
    virtual Transform*        GetTransform(Entity e)  = 0;
    virtual MeshRenderer*     GetRenderer(Entity e)   = 0;
    virtual SpriteAnimator*   GetAnimator(Entity e)   = 0;
    virtual Collider*         GetCollider(Entity e)   = 0;
    virtual PlayerController* GetController(Entity e) = 0;
	virtual EnemiesController* GetEnemyController(Entity e) = 0;
    virtual ParticleEmitter*  GetEmitter(Entity e)    = 0;
    virtual LightComponent* GetLight(Entity e) = 0;
    virtual Mesh2D* GetMesh(size_t index) = 0;

    // Retrieve a read-only list of all active entities.
    virtual const std::vector<Entity>& GetEntities() const = 0;

    // Access the SystemManager for timing/performance data.
    virtual const SystemManager& GetSystemManager() const = 0;

    // Read-only component getters for EditorOverlay or debug tools.
    virtual const Transform*        TryGetTransform(Entity e)  const = 0;
    virtual const Collider*         TryGetCollider(Entity e)   const = 0;
    virtual const MeshRenderer*     TryGetRenderer(Entity e)   const = 0;
    virtual const SpriteAnimator*   TryGetAnimator(Entity e)   const = 0;
    virtual const PlayerController* TryGetController(Entity e) const = 0;

	virtual const EnemiesController* TryGetEnemyController(Entity e) const = 0;

    virtual const ParticleEmitter*  TryGetEmitter(Entity e)    const = 0;
    virtual const LightComponent* TryGetLight(Entity e)      const = 0;
    virtual const SpeedComponent* TryGetSpeed(Entity e)      const = 0;
    virtual const MassComponent* TryGetMass(Entity e)       const = 0;

    // --- Component creation/removal for editor ---
    // These are used by the editor to toggle components on entities.
    // Implementations should set/clear the correct signature bits.

    virtual MeshRenderer* AddRendererComponent(Entity e) = 0;
    virtual void             RemoveRendererComponent(Entity e) = 0;

    virtual Collider* AddColliderComponent(Entity e) = 0;
    virtual void             RemoveColliderComponent(Entity e) = 0;

    virtual SpriteAnimator* AddAnimatorComponent(Entity e) = 0;
    virtual void             RemoveAnimatorComponent(Entity e) = 0;

    // Persistent Tag
    virtual PersistentTag& AddPersistentTag(Entity e) = 0;
    virtual void RemovePersistentTag(Entity e) = 0;
    virtual bool HasPersistentTag(Entity e) const = 0;

    virtual PlayerController* AddControllerComponent(Entity e) = 0;
    virtual void              RemoveControllerComponent(Entity e) = 0;

    virtual ParticleEmitter* AddEmitterComponent(Entity e) = 0;
    virtual void              RemoveEmitterComponent(Entity e) = 0;

    virtual LightComponent* AddLightComponent(Entity e) = 0;
    virtual void            RemoveLightComponent(Entity e) = 0;

    virtual SpeedComponent* AddSpeedComponent(Entity e) = 0;
    virtual void              RemoveSpeedComponent(Entity e) = 0;

    virtual MassComponent* AddMassComponent(Entity e) = 0;
    virtual void              RemoveMassComponent(Entity e) = 0;

    virtual EnemiesController* AddEnemyControllerComponent(Entity e) = 0;
    virtual void               RemoveEnemyControllerComponent(Entity e) = 0;

    virtual std::string GetEntityDisplayName(Entity e) const {
        return "Entity " + std::to_string(e);
    }

    // --- Scripting bridge (engine-agnostic) ---
    virtual std::vector<Entity> GetScriptedEntities() const = 0;
    virtual void ScriptStartIfNeeded(Entity e) = 0;
    virtual void ScriptUpdate(Entity e, float dt) = 0;
    virtual bool AreScriptsEnabled() const { return true; }

    // --- Script Editor Bridge (game-agnostic) ---
    // List of script identifiers the editor can present in a combo box.
    // Example (game side): { "None", "Spin", "ChasePlayer" }.
    virtual const std::vector<std::string>& GetScriptNameList() const {
        static const std::vector<std::string> empty;
        return empty;
    }

    // Name of script currently assigned to entity ("" or "None" = no script).
    virtual std::string GetEntityScriptName(Entity e) const {
        (void)e;
        return {};
    }

    // Assign script by name; engine/editor do not care how it maps to ScriptComponent.
    virtual void SetEntityScriptName(Entity e, const std::string& name) {
        (void)e; (void)name;
    }

    virtual void SpawnEnemyProjectile(Entity shooter, float dirX, float dirY) = 0;

    // --- Asset Editor Bridge: does NOT expose ResourceManager ---
    virtual const std::vector<std::string>& GetTextureList() const = 0;
    virtual void ApplyTexture(Entity e, const std::string& textureName) = 0;
    virtual bool ImportTexture(const std::string& absPath, const std::string& name) = 0;

    // =============================================================
    // UndoRedo 
    // =============================================================

    virtual void DestroyEntityImmediate(Entity e) = 0;
    virtual Entity CreateEntityWithFixedID(Entity id) = 0;
    virtual UndoRedoManager& GetUndoRedo() = 0;
    virtual bool GetIsPlaying() const = 0;

    //Lua/Runtime Scripting API
    virtual bool TryGetWorldPos(Entity e, float& outX, float& outY) const = 0;
    virtual bool SetWorldPos(Entity e, float x, float y) = 0;
    virtual bool TryGetRotation(Entity e, float& outR) const = 0;
    virtual bool SetRotation(Entity e, float r) = 0;

    // AI helpers
    virtual float GetTileSize() const = 0;
    virtual bool  IsBlockedWorld(float wx, float wy) const = 0;
    virtual bool  IsWallWorld(float wx, float wy) const = 0;
    virtual bool  HasLineOfSightWorld(float ax, float ay, float bx, float by, Entity e) const = 0;

    // lets Lua utilize physics
    virtual void  MoveEntityWorld(Entity e, float vx, float vy, float dt) = 0;

    // for FindPlayer in script.
    // Game decides what player means like PlayerController, tag, etc
    virtual Entity GetPlayerEntity() const = 0;

    // Layering flags (single flag per layer)
    virtual const eng::Layering& GetLayering() const = 0;

    // Efficient layer buckets (arrays of handles)
    virtual const std::vector<Entity>& GetEntitiesInLayer(eng::LayerId id) const = 0;

    // Functions for lua to call
    virtual void SetColliderActive(Entity e, bool active) = 0;
    virtual void SetAnimationRow(Entity e, int row) = 0;
    virtual int  GetEnemyHp(Entity e) const = 0;
    virtual int  GetEnemyMaxHp(Entity e) const = 0;
    virtual void HealEnemyHp(Entity e, int amount) = 0;
    virtual void BroadcastMessage(const std::string& id) = 0;
};