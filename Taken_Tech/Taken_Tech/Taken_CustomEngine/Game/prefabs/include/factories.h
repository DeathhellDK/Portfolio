#pragma once
/**
 * @file     factories.h
 * @author   Jethro Sung
 * @co-author Woh Kye Le, Lim Zhi Jie
 * @email    sung.h, w.kyele, zhijie.lim
 * @date     2025-10-24
 *
 * @brief    Declares GameObject factory helper functions.
 *
 * Factories create properly-wired ECS entities based on high-level
 * parameters or Prefab data. They handle common setup such as:
 *  - Transform initialization
 *  - MeshRenderer + texture binding
 *  - Collider assignment (solid vs trigger)
 *  - Optional controllers (player / AI enemy)
 *  - Persistent player handling for scene transitions
 *
 * @details
 * These helpers ensure consistent entity creation for:
 *  - Loader (scene JSON instantiation)
 *  - Editor (drag-spawn runtime previews)
 *  - Gameplay logic (enemy spawners, dropped objects, etc.)
 *
 * @note Mesh extent is used to ensure correct pivot (center-based rotation).
 */

#include "Core/gameobj.h"
#include "Graphics/meshrenderer.h"
#include "Core/transform.h"
#include "playerController.h"
#include "Physics/collider.h"
#include "Core/persistentTag.h"
#include "Core/gameApp.h"
#include "Editor/EditorShared.h"

#include <vector>
#include <memory>

class Mesh2D; // forward declare



/**
    * @brief Creates a fully-wired GameObject entity with visual + physics setup.
    *
    * Components created:
    *  - Transform
    *  - MeshRenderer (mesh + color)
    *  - Collider (optional trigger behavior)
    *
    * @param app          Game context & ECS access point
    * @param name         Debug / inspector identifier
    * @param mesh         Quad mesh for rendering
    * @param position     Initial world-space position (top-left)
    * @param scale        Local scaling applied to mesh
    * @param rotation     Local rotation in radians (pivot at center of mesh)
    * @param color        Tint multiplier for rendering
    * @param colliderType Collider shape (default: Box)
    * @param isTrigger    If true: no physical pushback is applied
    * @param meshExtent   Actual mesh size before scaling (pivot correction)
    *
    * @return Newly created entity ID
*/
Entity MakeGameObject(
    GameApp& app,
    const std::string& name,
    Mesh2D* mesh,
    const Vector2& position,
    const Vector2& scale = { 100.f, 100.f },
    float rotation = 0.f,
    const Vector3& color = { 1.f, 1.f, 1.f },
    ColliderType colliderType = ColliderType::Box,//default to box collider
    bool isTrigger = false,
    const Vector2& meshExtent = { 1.0f, 1.0f } //actual local mesh size
);

/**
 * @brief Creates the player entity if not already persistent.
 *
 * Adds:
 *  - PlayerController
 *  - PersistentTag (avoids duplicate creation across level loads)
 *
 * @note Loader repositions existing persistent player rather than cloning.
 *
 * @return Entity referencing the controlled player
 */
Entity MakePlayer(
    GameApp& app,
    const std::string& name,
    Mesh2D* mesh,
    const Vector2& position,
    const Vector2& scale = { 80.f, 80.f },
    float rotation = 0.f,
    const Vector3& color = { 0.2f, 0.7f, 1.f },
    const Vector2& meshExtent = { 1.0f, 1.0f } //actual local mesh size
);

/**
 * @brief Creates an enemy entity with mesh + physics, to be driven by
 *        Enemy systems once runtime AI initialization occurs.
 *
 * @note AI controller binding may occur later (after nav data ready).
 */
Entity MakeEnemy(
    GameApp& app,
    const std::string& name,
    Mesh2D* mesh,
    const Vector2& position,
    const Vector2& scale,
    float rotation,
    const Vector3& color,
    const Vector2& meshExtent
);

/**
 * @brief Copies a GameObject with new render + physics properties.
 *
 * @details
 * Used by EditorOverlay drag-clone behavior and scene duplication tools.
 */
Entity CloneGameObject(GameApp& app,
    const std::string& name,
    Mesh2D* mesh,
    const Vector3& color,
    const Vector2& pos,
    const Vector2& scale,
    float rot,
    ColliderType colliderType,
    bool isTrigger,
    const Vector2& meshExtent);


/**
 * @brief Instantiates a prefab entity with all configured components.
 *
 * Implements prefab instantiation with support for multiple component types
 * including animation, particle effects, and special handling for enemy prefabs.
 *
 * @param app          Game context & ECS access point
 * @param prefabName   Name of the prefab type (e.g., "ranged_mini_boss", "Crate")
 * @param prefabData   Prefab configuration data
 * @param worldPos     World position to spawn the entity
 *
 * @return Entity Newly created prefab entity ID
 */
Entity FactoryInstantiatePrefab(GameApp& app,
    const std::string& prefabName,
    const Prefab& prefabData,
    const Vector2& worldPos
);



/**
 * @brief Creates a wall tile entity from variant placement data.
 *
 * Components created:
 *  - Transform (positioned at grid coordinates converted to world space)
 *  - MeshRenderer (with specified texture)
 *  - Collider (if solid is true)
 *
 * @param app         Game context & ECS access point
 * @param variant     Variant placement data (grid pos, texture, solid flag)
 * @param tileSize    Size of each grid cell in world units
 * @param meshExtent  Actual mesh size before scaling (default unit quad)
 *
 * @return Newly created wall entity ID
 */
Entity MakeWallTileFromVariant(
    GameApp& app,
    const VariantPlacement& variant,
    float tileSize,
    const Vector2& meshExtent = { 1.0f, 1.0f }
);


/**
 * @brief Creates all variant entities from a list of variant placements.
 *
 * Processes each variant in the list and creates the appropriate entity type.
 * Currently supports "WallTile" variants, can be extended for other types.
 *
 * @param app         Game context & ECS access point
 * @param variants    Vector of variant placement data
 * @param tileSize    Size of each grid cell in world units
 *
 * @return Vector of created entity IDs
 */
std::vector<Entity> CreateVariantEntities(
    GameApp& app,
    const std::vector<VariantPlacement>& variants,
    float tileSize
);


/**
 * @brief Loads variant file and creates all variant entities.
 *
 * Convenience function that combines loading and entity creation.
 * Resolves the variant file path from scene path automatically.
 *
 * @param app         Game context & ECS access point
 * @param scenePath   Path to the scene (used to resolve variant file)
 * @param tileSize    Size of each grid cell in world units
 *
 * @return Vector of created entity IDs (empty if file not found)
 */
std::vector<Entity> ApplyRoomVariants(
    GameApp& app,
    const std::string& scenePath,
    int gridHeight,
    float tileSize);

/**
 * @brief Spawns a projectile fired by a shooter entity (player or enemy).
 *
 * Creates a bullet GameObject using MakeGameObject() and attaches a
 * Projectile component with configured velocity, lifetime, and damage.
 *
 * @param app        Game context & ECS access point.
 * @param shooter    Entity that fired the projectile (must have Transform).
 * @param dir        Normalized world-space firing direction.
 * @param fromEnemy  True if projectile is fired by an enemy.
 *
 * @return Newly created projectile entity ID,
 *         or INVALID_ENTITY if creation fails.
 */
Entity MakeProjectile(
    GameApp& app,
    Entity shooter,
    const Vector2& dir,
    bool fromEnemy);