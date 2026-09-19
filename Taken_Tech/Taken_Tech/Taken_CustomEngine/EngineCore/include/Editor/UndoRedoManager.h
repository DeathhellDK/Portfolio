#pragma once
/**
 * @file UndoRedoManager.h
 * @author    Jethro Sung
 * @co-author Sng Swee Yong Dillon
 * @email     sung.h, sweeyongdillon.sng
 * @brief Central stack-based undo/redo system.
 *
 * Supports:
 * - Transform position/rotation/scale changes
 * - Adding/removing entities
 * - Any editor/manipulation operation
 *
 * Stack size capped at 30.
 */

#include <vector>
#include <optional>
#include <string>
#include "Math/vect2.h"
#include "Math/vect3.h"
#include "Core/component.h"

 // Forward declaration
class IComponentContext;
class Mesh2D;
struct SpriteSheet;

// ---------------------------------------------------------------
// Snapshot describing the transform of one entity
// ---------------------------------------------------------------
/**
 * @struct TransformSnapshot
 * @brief Captures the state of a Transform component.
 */
struct TransformSnapshot
{
    Entity entity = INVALID_ENTITY; ///< The entity ID.
    Vector2 position{};             ///< World position.
    Vector2 scale{};                ///< World scale.
    float rotation = 0.f;           ///< World rotation in degrees.
};

/**
 * @struct ColliderSnapshot
 * @brief Captures the state of a Collider component.
 */
struct ColliderSnapshot
{
    Entity entity = INVALID_ENTITY; ///< The entity ID.
    bool hasCollider = false;       ///< Whether the component existed.
    Vector2 size{ 1.f, 1.f };       ///< Collider size.
    bool isTrigger = false;         ///< Whether it is a trigger.
};

/**
 * @struct MeshRendererSnapshot
 * @brief Captures the state of a MeshRenderer component.
 */
struct MeshRendererSnapshot
{
    Entity entity = INVALID_ENTITY; ///< The entity ID.
    bool hasRenderer = false;       ///< Whether the component existed.

    Vector3 color{ 1.f, 1.f, 1.f }; ///< Tint color.
    unsigned texture = 0;           ///< Texture ID.

    // messh change in editor
    Mesh2D* mesh = nullptr;         ///< Pointer to the mesh.
};

/**
 * @struct SpriteAnimatorSnapshot
 * @brief Captures the state of a SpriteAnimator component.
 */
struct SpriteAnimatorSnapshot
{
    Entity entity = INVALID_ENTITY; ///< The entity ID.
    bool hasAnimator = false;       ///< Whether the component existed.

    int cur = 0;                    ///< Current frame index.
    int startFrame = 0;             ///< Start frame index.
    int endFrame = 0;               ///< End frame index.
    float speed = 0.0f;             ///< Animation speed.

    const SpriteSheet* sheet = nullptr; ///< Pointer to the sprite sheet.
};

/**
 * @struct ParticleEmitterSnapshot
 * @brief Captures the state of a ParticleEmitter component.
 */
struct ParticleEmitterSnapshot
{
    Entity entity = INVALID_ENTITY; ///< The entity ID.
    bool hasEmitter = false;        ///< Whether the component existed.

    bool enabled = false;           ///< Whether the emitter is enabled.
    float rate = 50.f;              ///< Emission rate.
    float particleLife = 0.6f;      ///< Life of each particle.
    Vector2 offset{ 0.f, 0.f };     ///< Emitter offset.
    Vector2 velMin{ -30.f, 50.f };  ///< Minimum velocity.
    Vector2 velMax{ 30.f, 90.f };   ///< Maximum velocity.
    Vector3 colorStart{ 1.f, 1.f, 1.f }; ///< Start color.
    Vector3 colorEnd{ 0.8f, 0.8f, 0.8f }; ///< End color.
    float sizeStart = 10.f;         ///< Start size.
    float sizeEnd = 1.f;            ///< End size.

    Mesh2D* quad = nullptr;         ///< Particle mesh.
    unsigned texture = 0;           ///< Particle texture ID.
    float timeAccumulator = 0.f;    ///< Internal time accumulator.
};

/**
 * @struct ScriptSnapshot
 * @brief Captures the state of a Script component (script name).
 */
struct ScriptSnapshot
{
    Entity entity = INVALID_ENTITY; ///< The entity ID.
    bool hasScript = false;         ///< Whether a script was assigned.
    std::string scriptName;         ///< Name of the script.
};

/**
 * @struct PlayerControllerSnapshot
 * @brief Captures the existence of a PlayerController component.
 */
struct PlayerControllerSnapshot
{
    Entity entity = INVALID_ENTITY; ///< The entity ID.
    bool hasController = false;     ///< Whether the component existed.
};

/**
 * @struct SpeedComponentSnapshot
 * @brief Captures the existence of a SpeedComponent.
 */
struct SpeedComponentSnapshot
{
    Entity entity = INVALID_ENTITY; ///< The entity ID.
    bool hasSpeed = false;          ///< Whether the component existed.
};

/**
 * @struct MassComponentSnapshot
 * @brief Captures the existence of a MassComponent.
 */
struct MassComponentSnapshot
{
    Entity entity = INVALID_ENTITY; ///< The entity ID.
    bool hasMass = false;           ///< Whether the component existed.
};

/**
 * @struct PersistentTagSnapshot
 * @brief Captures the existence of a PersistentTag.
 */
struct PersistentTagSnapshot
{
    Entity entity = INVALID_ENTITY; ///< The entity ID.
    bool hasPersistentTag = false;  ///< Whether the tag existed.
};

/**
 * @struct LightSnapshot
 * @brief Captures the state of a LightComponent.
 */
struct LightSnapshot
{
    Entity entity = INVALID_ENTITY; ///< The entity ID.
    bool hasLight = false;          ///< Whether the component existed.

    bool enabled = true;            ///< Whether the light is enabled.

    bool glowEnabled = true;        ///< Whether glow effect is enabled.
    float glowRadius = 180.0f;      ///< Glow radius.
    Vector3 glowColor{ 1.0f, 0.85f, 0.55f }; ///< Glow color.
    Vector2 glowOffset{ 0.0f, 0.0f }; ///< Glow offset.
    float glowIntensity = 1.0f;     ///< Glow intensity.
    float glowOpacity = 0.35f;      ///< Glow opacity.
    float glowSoftness = 2.0f;      ///< Glow softness.

    bool sourceEnabled = false;     ///< Whether source light is enabled.
    float sourceRadius = 180.0f;    ///< Source radius.
    Vector3 sourceColor{ 1.0f, 0.85f, 0.55f }; ///< Source color.
    Vector2 sourceOffset{ 0.0f, 0.0f }; ///< Source offset.
    float sourceIntensity = 1.0f;   ///< Source intensity.
    float sourceOpacity = 0.35f;    ///< Source opacity.
    float sourceAttenuation = 1.0f; ///< Source attenuation.

    bool emberEnabled = false;      ///< Whether embers are enabled.
    float emberRate = 20.0f;        ///< Ember emission rate.
    float emberParticleLife = 0.8f; ///< Ember life.
    Vector2 emberVelMin{ -20.f, 30.f }; ///< Ember min velocity.
    Vector2 emberVelMax{ 20.f, 60.f }; ///< Ember max velocity.
    Vector3 emberColorStart{ 1.0f, 0.8f, 0.5f }; ///< Ember start color.
    Vector3 emberColorEnd{ 0.3f, 0.1f, 0.0f }; ///< Ember end color.
    float emberSizeStart = 6.0f;    ///< Ember start size.
    float emberSizeEnd = 1.0f;      ///< Ember end size.
    bool emberAdditive = true;      ///< Ember additive blending.
    Vector2 emberOffset{ 0.0f, 0.0f }; ///< Ember offset.

    bool flickerEnabled = false;    ///< Whether flicker is enabled.
    float flickerIntensityMin = 0.8f; ///< Min flicker intensity.
    float flickerIntensityMax = 1.0f; ///< Max flicker intensity.
    float flickerSpeed = 5.0f;      ///< Flicker speed.
    float flickerTime = 0.0f;       ///< Flicker time accumulator.
};

/**
 * @brief Captures the current state of a LightComponent for an entity.
 * @param ctx The component context.
 * @param e The entity ID.
 * @return A LightSnapshot containing the light data.
 */
LightSnapshot CaptureLightSnapshot(IComponentContext& ctx, Entity e);

/**
 * @brief Applies a LightSnapshot to an entity, adding/removing/updating the component.
 * @param ctx The component context.
 * @param snap The snapshot to apply.
 */
void ApplyLightSnapshot(IComponentContext& ctx, const LightSnapshot& snap);


/**
 * @struct EnemyControllerSnapshot
 * @brief Captures the existence of an EnemyControllerComponent.
 */
struct EnemyControllerSnapshot
{
    Entity entity = INVALID_ENTITY; ///< The entity ID.
    bool hasEnemyController = false; ///< Whether the component existed.
};

// ---------------------------------------------------------------
// Snapshot used when entities are created or deleted
// ---------------------------------------------------------------
/**
 * @struct EntitySnapshot
 * @brief Captures the full state of an entity for creation/deletion undo/redo.
 */
struct EntitySnapshot
{
    Entity entity = INVALID_ENTITY; ///< The entity ID.
    bool existedBefore = false;     ///< If true, this entity existed before the operation (so undo should restore it).
    TransformSnapshot transform;    ///< Its transform state.

    // Component presence flags at the time of snapshot
    bool hasTransform = false;
    bool hasRenderer = false;
    bool hasAnimator = false;
    bool hasCollider = false;
    bool hasController = false;
    bool hasEmitter = false;
    bool hasSpeed = false;
    bool hasMass = false;
    bool hasScript = false;
    bool hasPersistentTag = false;
    bool hasLight = false;
    bool hasEnemyController = false;

    // Renderer state snapshot
    Vector3 rendererColor{ 1.f, 1.f, 1.f };
	unsigned rendererTexture = 0; // hv to store tex ID

    // Animator state snapshot
    const SpriteSheet* animSheet = nullptr;
    int animStart = 0;
    int animEnd = 0;
    int animCur = 0;
    float animSpeed = 0.f;

    std::string scriptName;

    Vector2 colliderSize{ 1.f, 1.f };
    bool    colliderIsTrigger = false;

    LightSnapshot light;
};

// ---------------------------------------------------------------
// Unified undo/redo record
// ---------------------------------------------------------------
/**
 * @struct UndoRecord
 * @brief Represents a single undoable/redoable action in the history stack.
 */
struct UndoRecord
{
    /**
     * @enum Type
     * @brief Identifies the type of operation performed.
     */
    enum class Type {
        TransformEdit,
        ColliderEdit,
        ColliderToggle,
        EntityCreated,
        EntityDeleted,
        MeshRendererEdit,
        MeshRendererToggle,
        SpriteAnimatorEdit,
        SpriteAnimatorToggle,
        ParticleEmitterEdit,
        ParticleEmitterToggle,
        ScriptEdit,
        PlayerControllerToggle,
        SpeedComponentToggle,
        MassComponentToggle,
        PersistentTagToggle,
        LightEdit,
        LightToggle,
        EnemyControllerToggle
    };

    Type action; ///< The type of action recorded.

    // Transform change
    TransformSnapshot before; ///< State before the action.
    TransformSnapshot after;  ///< State after the action.

    ColliderSnapshot beforeCollider;
    ColliderSnapshot afterCollider;

    MeshRendererSnapshot beforeRenderer;
    MeshRendererSnapshot afterRenderer;

    SpriteAnimatorSnapshot beforeAnimator;
    SpriteAnimatorSnapshot afterAnimator;

    ParticleEmitterSnapshot beforeEmitter;
    ParticleEmitterSnapshot afterEmitter;

    ScriptSnapshot beforeScript;
    ScriptSnapshot afterScript;

    PlayerControllerSnapshot beforeController;
    PlayerControllerSnapshot afterController;

    SpeedComponentSnapshot beforeSpeed;
    SpeedComponentSnapshot afterSpeed;

    MassComponentSnapshot beforeMass;
    MassComponentSnapshot afterMass;

    PersistentTagSnapshot beforePersistentTag;
    PersistentTagSnapshot afterPersistentTag;

    LightSnapshot beforeLight;
    LightSnapshot afterLight;

    EnemyControllerSnapshot beforeEnemyController;
    EnemyControllerSnapshot afterEnemyController;

    // Entity create/delete
    EntitySnapshot entityInfo; ///< Full entity state for creation/deletion.
};

// ===============================================================
// UndoRedoManager
// ===============================================================
/**
 * @class UndoRedoManager
 * @brief Manages the undo/redo stack and operations for the editor.
 */
class UndoRedoManager
{
public:

    /**
     * @brief Constructs the manager with a reference to the component context.
     * @param ctx The component context to operate on.
     */
    explicit UndoRedoManager(IComponentContext& ctx);

    // ---------- High-level operations ----------
    
    /**
     * @brief Pushes a transform update operation onto the undo stack.
     * @param e The entity ID.
     * @param oldPos Previous position.
     * @param newPos New position.
     * @param oldScale Previous scale.
     * @param newScale New scale.
     * @param oldRot Previous rotation.
     * @param newRot New rotation.
     */
    void Push_TransformUpdate(Entity e,
        const Vector2& oldPos, const Vector2& newPos,
        const Vector2& oldScale, const Vector2& newScale,
        float oldRot, float newRot);

    /**
     * @brief Pushes a collider update operation onto the undo stack.
     * @param e The entity ID.
     * @param oldSize Previous size.
     * @param newSize New size.
     * @param oldTrigger Previous trigger state.
     * @param newTrigger New trigger state.
     */
    void Push_ColliderUpdate(Entity e,
        const Vector2& oldSize, const Vector2& newSize,
        bool oldTrigger, bool newTrigger);

    /**
     * @brief Pushes a collider toggle (add/remove) operation onto the undo stack.
     */
    void Push_ColliderToggle(
        Entity e,
        bool beforeHasCollider,
        const Vector2& beforeSize,
        bool beforeTrigger,
        bool afterHasCollider,
        const Vector2& afterSize,
        bool afterTrigger);

    /**
     * @brief Pushes a combined transform and collider update.
     */
    void Push_TransformUpdateWithCollider(Entity e,
        const Vector2& oldPos, const Vector2& newPos,
        const Vector2& oldScale, const Vector2& newScale,
        float oldRot, float newRot,
        bool hadColliderBefore,
        const Vector2& oldColSize, const Vector2& newColSize,
        bool oldColTrigger, bool newColTrigger);

    /**
     * @brief Pushes an entity creation event.
     * @param e The created entity ID.
     */
    void Push_EntityCreated(Entity e);

    /**
     * @brief Pushes an entity deletion event.
     * @param e The deleted entity ID.
     * @param pos Last position.
     * @param scale Last scale.
     * @param rot Last rotation.
     */
    void Push_EntityDeleted(Entity e,
        const Vector2& pos, const Vector2& scale, float rot);

    /**
     * @brief Pushes a MeshRenderer edit operation.
     */
    void Push_MeshRendererEdit(
        Entity e,
        const Vector3& oldColor, const Vector3& newColor,
        unsigned oldTexture, unsigned newTexture,
        Mesh2D* oldMesh, Mesh2D* newMesh);

    /**
     * @brief Pushes a MeshRenderer toggle (add/remove) operation.
     */
    void Push_MeshRendererToggle(
        Entity e,
        bool hadRendererBefore,
        const Vector3& oldColor,
        unsigned oldTexture,
        Mesh2D* oldMesh,
        bool hasRendererAfter,
        const Vector3& newColor,
        unsigned newTexture,
        Mesh2D* newMesh);

    /**
     * @brief Pushes a SpriteAnimator edit operation.
     */
    void Push_SpriteAnimatorEdit(
        Entity e,
        int oldCur, int newCur,
        int oldStart, int newStart,
        int oldEnd, int newEnd,
        float oldSpeed, float newSpeed,
        const SpriteSheet* oldSheet, const SpriteSheet* newSheet
    );

    /**
     * @brief Pushes a SpriteAnimator toggle (add/remove) operation.
     */
    void Push_SpriteAnimatorToggle(
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
        const SpriteSheet* afterSheet
    );

    /**
     * @brief Pushes a ParticleEmitter edit operation.
     */
    void Push_ParticleEmitterEdit(
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
        float newTimeAccumulator
    );

    /**
     * @brief Pushes a ParticleEmitter toggle (add/remove) operation.
     */
    void Push_ParticleEmitterToggle(
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
        float afterTimeAccumulator
    );

    /**
     * @brief Pushes a script edit (change script name) operation.
     */
    void Push_ScriptEdit(
        Entity e,
        bool beforeHasScript,
        const std::string& beforeName,
        bool afterHasScript,
        const std::string& afterName);

    /**
     * @brief Pushes a PlayerController toggle operation.
     */
    void Push_PlayerControllerToggle(
        Entity e,
        bool beforeHasController,
        bool afterHasController);

    /**
     * @brief Pushes a SpeedComponent toggle operation.
     */
    void Push_SpeedComponentToggle(
        Entity e,
        bool beforeHasSpeed,
        bool afterHasSpeed);

    /**
     * @brief Pushes a MassComponent toggle operation.
     */
    void Push_MassComponentToggle(
        Entity e,
        bool beforeHasMass,
        bool afterHasMass);

    /**
     * @brief Pushes a PersistentTag toggle operation.
     */
    void Push_PersistentTagToggle(
        Entity e,
        bool beforeHasPersistentTag,
        bool afterHasPersistentTag);

    /**
     * @brief Pushes a LightComponent edit operation.
     */
    void Push_LightEdit(
        Entity e,
        const LightSnapshot& beforeSnap,
        const LightSnapshot& afterSnap);

    /**
     * @brief Pushes a LightComponent toggle operation.
     */
    void Push_LightToggle(
        Entity e,
        const LightSnapshot& beforeSnap,
        const LightSnapshot& afterSnap);

    /**
     * @brief Pushes an EnemyController toggle operation.
     */
    void Push_EnemyControllerToggle(
        Entity e,
        bool beforeHasEnemyController,
        bool afterHasEnemyController);

    /**
     * @brief Performs the undo operation (reverts the last action).
     */
    void Undo();

    /**
     * @brief Performs the redo operation (reapplies the last undone action).
     */
    void Redo();

    /** @return True if there are actions to undo. */
    bool CanUndo() const { return !undoStack.empty(); }

    /** @return True if there are actions to redo. */
    bool CanRedo() const { return !redoStack.empty(); }

    /** @return The current undo stack. */
    const std::vector<UndoRecord>& GetUndoStack() const { return undoStack; }

    /** @return The current redo stack. */
    const std::vector<UndoRecord>& GetRedoStack() const { return redoStack; }

    /** @brief Clears both undo and redo stacks. */
    void Clear()
    {
        undoStack.clear();
        redoStack.clear();
    }

    /** @brief Clears only the redo stack. */
    void ClearRedo() { redoStack.clear(); }

private:
    /**
     * @brief Applies an undo or redo record.
     * @param rec The record to apply.
     * @param undoing True if undoing, false if redoing.
     */
    void ApplyRecord(const UndoRecord& rec, bool undoing);

    IComponentContext& context; ///< Reference to the component context.

    std::vector<UndoRecord> undoStack; ///< Stack of undoable actions.
    std::vector<UndoRecord> redoStack; ///< Stack of redoable actions.

    static constexpr size_t MAX_STACK = 30; ///< Maximum stack size.
};

