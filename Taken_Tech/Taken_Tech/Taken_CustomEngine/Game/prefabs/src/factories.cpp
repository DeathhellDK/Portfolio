/**
 * @file     factories.cpp
 * @author    Jethro Sung
 * @email     sung.h, w.kyele,t.weiliangterril
 * @co-author Woh Kye Le, Tan Wei Liang Terril
 * @date     2025-11-7
 *
 * @brief    Implements GameObject creation helpers for ECS entity setup.
 *
 * These factory helpers ensure proper ordering of component attachment,
 * correct pivot handling, and prefab tagging used for save/load systems.
 *
 * Responsibilities:
 * - Create Entities in the ECS registry (EntityManager)
 * - Attach required components (Transform, Renderer, Collider, etc.)
 * - Auto-assign Prefab tags for JSON instantiation rules
 * - Assign gameplay behavior controllers (Player, Enemy)
 *
 */
#include "factories.h"
#include "prefab.h"
#include <random>
#include <unordered_set>
#include <algorithm>
#include "Physics/PhysicsSpeedComponent.hpp"
#include "Physics/PhysicsMassComponent.hpp"
#include "Physics/ForceSystem.hpp"
#include "Core/resourceManager.h"
#include "Light/lightComponent.h"
#include "puzzleObject.h"

 /**
 * @brief Helper function to compute navigation map path based on active scene.
 *
 * Determines the appropriate navigation map file path by examining the current scene path:
 * - Defaults to "maps/labyrinth.txt" for labyrinth scenes
 * - Maps scene names to corresponding map files (e.g., "entities_Level1.json" → "maps/entities_Level1.txt")
 * - Provides fallback to labyrinth map if scene can't be determined
 *
 * @param app The game application context containing scene path information
 * @return std::string Path to the navigation map file
 */
static std::string ComputeNavMapPath(const GameApp& app)
{
    std::string scenePath = app.GetActiveScenePath();   // uses playScenePath / editScenePath

    // Default: labyrinth
    if (scenePath.find("labyrinth.json") != std::string::npos)
        return "maps/labyrinth.txt";

    // For entities_LevelX.json, follow the same convention as GameApp::SaveLevel
    // entities_Level1.json -> Assets/maps/entities_Level1.txt, etc.
    std::filesystem::path p(scenePath);
    std::string base = p.stem().string();               // e.g. "entities_Level1"
    if (!base.empty())
        return "maps/" + base + ".txt";

    // Fallback safety
    return "maps/labyrinth.txt";
}

/**
 * @brief Creates a GameObject with Transform + Renderer + Collider.
 *
 * Collider is automatically sized using:
 *     colliderSize = meshExtent * scale
 * ensuring collider matches visible mesh.
 *
 * Also determines prefab tag based on texture name heuristic
 * (`"wall"`, `"crate"`, `"door"`).
 *
 */
Entity MakeGameObject(
    GameApp& app,
    const std::string& name,
    Mesh2D* mesh,
    const Vector2& position,
    const Vector2& scale,
    float rotation,
    const Vector3& color,
    ColliderType colliderType,
    bool isTrigger,
    const Vector2& meshExtent
) {
    Entity e = app.entityManager.CreateEntity(name);//we create the entity and get its id

    // Transform
    app.AddTransform(e, position, scale, rotation);//add the transform component to the entity

    // MeshRenderer
    app.AddRenderer(e, mesh, color);

    // --- Prefab Tagging ---
    std::string tag = "Crate";
    GLuint texID = 0;

    if (auto* mr = app.GetRenderer(e))
        texID = mr->GetTexture();

    std::string texName = ResourceManager::GetTextureNameByID(texID);

    if (texName.find("wall") != std::string::npos)
        tag = "Wall";
    else if (texName.find("door") != std::string::npos)
        tag = "Door";
    else if (texName.find("crate") != std::string::npos)
        tag = "Crate";

    app.SetPrefabTag(e, tag);

    //app.SetPrefabTag(e, "Crate");

    switch (colliderType) {
    case ColliderType::Box: {
        /*
            Because if we have cases where the mesh is not 1:1, we cannot use the scale. Because it would cause a mismatch
            between the collider and the actual mesh size. the mesh multiplied by the scale gives us the actual size of the mesh in world space.
            and because the collider should match the actual size of the mesh in world space, we use meshExtent * scale*/
        Vector2 colliderSize(meshExtent.x * scale.x, meshExtent.y * scale.y);
        app.AddCollider(e, colliderSize, isTrigger);
        break;
    }
    case ColliderType::Circle: {
        float radius = std::max(scale.x * meshExtent.x, scale.y * meshExtent.y) * 0.5f;
        app.AddCollider(e, Vector2(radius, 0.0f), isTrigger);
        //obj->AddComponent<Collider>(scale.x * 0.5f, isTrigger); // Circle radius = half width
        break;
    }//needed cause we declared radius, collider size etc
    case ColliderType::Triangle:
        /*obj->AddComponent<Collider>(
            Vector2(0, meshExtent.y * scale.y * 0.5f),
            Vector2(-meshExtent.x * 0.5f * scale.x, -meshExtent.y * 0.5f * scale.y),
            Vector2(meshExtent.x * 0.5f * scale.x, -meshExtent.y * 0.5f * scale.y),
            isTrigger
        );*/
        break;
    }
    return e;
}

/**
 * @brief Creates a persistent Player entity with physics + animation components.
 *
 * Components attached:
 *  - Transform
 *  - Renderer
 *  - Collider (solid)
 *  - PlayerController
 *  - SpriteAnimator
 *  - PhysicsSpeed + PhysicsMass
 *  - PersistentTag (prevent duplication on scene reload)
 *
 * @note Loader will reposition existing Player instead of recreating.
 */
Entity MakePlayer(
    GameApp& app,
    const std::string& name,
    Mesh2D* mesh,
    const Vector2& position,
    const Vector2& scale,
    float rotation,
    const Vector3& color,
    const Vector2& meshExtent
) {
    Entity e = app.entityManager.CreateEntity(name);

    // --- Transform ---
    app.AddTransform(e, position, scale, rotation);

    // --- MeshRenderer ---
    app.AddRenderer(e, mesh, color);
    // --- Collider (box by default) ---
    Vector2 colliderSize(meshExtent.x * scale.x, meshExtent.y * scale.y);
    app.AddCollider(e, colliderSize, false);

    // --- PlayerController ---
    app.AddController(e);

    // --- Animation/Sprite
    //app.AddAnimator(e);

    // give the player a texture immediately Edit mode isnt a white quad
    if (auto* mr = app.GetRenderer(e)) {
        if (GLuint tex = ResourceManager::GetTexture("player_idle_right")) {
            mr->SetTexture(tex);
        }
        else {
            DebugConsole::Get().Warning("[Player] Missing texture key 'player_idle_right'\n");
        }
    }

    // --- Animation/Sprite
    SpriteAnimator& an = app.AddAnimator(e);

    // set the sheet immediately so the first frame exists even when not playing
    if (const SpriteSheet* sh = ResourceManager::GetSpriteSheet("player_idle_right")) {
        an.SetSheet(sh);
        an.SetSpeedFromFrameDuration(sh->frameDuration);
        an.Restart();  // ensures frame 0 is selected
    }
    else {
        DebugConsole::Get().Warning("[Player] Missing spritesheet key 'player_idle_right'\n");
    }


    //Persistent Tag
    app.AddPersistentTag(e);

    app.AddPhysicsSpeedComponent(e, SpeedComponent(e, Vector2(0.0f, 0.0f)));

    /// Add speed component into player entity with outside baseline
   /* {
        SpeedComponent spd(e, Vector2(0.0f, 0.0f));
        spd.maxSpeed = 250.0f;
        spd.friction = 625.0f;
        spd.roomSpeed = false;
        spd.outsideSpeed = true;
        app.AddPhysicsSpeedComponent(e, spd);
    }*/
    //app.AddPhysicsSpeedComponent(e, SpeedComponent(e, SpeedComponent::PlayerPresetTag{}));

    /// Add mass component into player entity
    app.AddPhysicsMassComponent(e, MassComponent(e, 2.0f));

    // --- Light (From Prefab) ---
    if (const Prefab* p = app.prefabManager.Get("Player")) {
        if (p->hasLight) {
            LightComponent& l = app.AddLight(e);
            l.enabled = p->light.enabled;
            
            l.source.enabled = p->light.source.enabled;
            l.source.radius = p->light.source.radius;
            l.source.intensity = p->light.source.intensity;
            l.source.opacity = p->light.source.opacity;
            l.source.color = p->light.source.color;
            l.source.offset = p->light.source.offset;
            l.source.attenuation = p->light.source.attenuation;

            l.glow.enabled = p->light.glow.enabled;
            l.glow.radius = p->light.glow.radius;
            l.glow.intensity = p->light.glow.intensity;
            l.glow.opacity = p->light.glow.opacity;
            l.glow.color = p->light.glow.color;
            l.glow.offset = p->light.glow.offset;
            l.glow.softness = p->light.glow.softness;
        }
    }

    app.SetPrefabTag(e, "Player");

    GameApp::playerEntity = e;

    return e;
}

/**
 * @brief Creates an enemy entity with mesh + physics + AI controller.
 *
 * Implements enemy entity creation with AI navigation capabilities.
 * Configures enemy controller with spawn position and navigation map.
 *
 * @param app          Game context & ECS access point
 * @param name         Debug / inspector identifier
 * @param mesh         Quad mesh for rendering
 * @param position     Initial world-space position (top-left)
 * @param scale        Local scaling applied to mesh
 * @param rotation     Local rotation in radians (pivot at center of mesh)
 * @param color        Tint multiplier for rendering
 * @param meshExtent   Actual mesh size before scaling (pivot correction)
 *
 * @return Entity Newly created enemy entity ID
 *
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
) {
    Entity e = app.entityManager.CreateEntity(name);

    // --- Transform ---
    app.AddTransform(e, position, scale, rotation);

    // --- MeshRenderer ---
    app.AddRenderer(e, mesh, color);

    // --- Collider (stationary solid object) ---
    Vector2 colliderSize(meshExtent.x * scale.x, meshExtent.y * scale.y);
    app.AddCollider(e, colliderSize, false);

    /// Testing Force physics
    //app.AddPhysicsSpeedComponent(e, SpeedComponent(e, Vector2(0.0f, 0.0f)));
    // app.AddPhysicsMassComponent(e, MassComponent(e, 2.0f));

    // --- Tag for save/load and prefab system ---
    app.SetPrefabTag(e, "ranged_mini_boss");

    EnemiesController& ec = app.AddEnemyController(e);
    ec.SetMobType(MobType::RANGED);
    if (Transform* t = app.GetTransform(e)) {
        ec.SetSpawn(t->GetPosition());
    }

    std::string navPath = ComputeNavMapPath(app);
    ec.SetMap(navPath);
    ec.SetLuaDrivenMovement(true);

    std::string scriptPath = "Scripts/include/chase.lua";
    if (name == "ranged_mini_boss") {
        scriptPath = "Scripts/include/enemy_ranged.lua";
    } else if (name == "burrow_mini_boss") {
        scriptPath = "Scripts/include/enemy_burrow.lua";
    } else if (name == "heal_mini_boss") {
        scriptPath = "Scripts/include/enemy_heal.lua";
    } else if (name == "Boss") {
        scriptPath = "Scripts/include/enemy_boss.lua";
    } else if (name == "EnemyContact") {
        scriptPath = "Scripts/include/enemy_basic.lua";
    }
    app.AddLuaScript(e, scriptPath);

    //ec.SetMap("Assets/maps/labyrinth.txt");

    return e;
}

/**
 * @brief Copies a GameObject with new render + physics properties.
 *
 * Simple wrapper around MakeGameObject() for cloning functionality.
 * Used by EditorOverlay drag-clone behavior and scene duplication tools.
 *
 * @param app            Game context & ECS access point
 * @param name           Debug / inspector identifier
 * @param mesh           Quad mesh for rendering
 * @param color          Tint multiplier for rendering
 * @param pos            Initial world-space position
 * @param scale          Local scaling applied to mesh
 * @param rot            Local rotation in radians
 * @param colliderType   Collider shape
 * @param isTrigger      If true: no physical pushback is applied
 * @param meshExtent     Actual mesh size before scaling (pivot correction)
 *
 * @return Entity Newly created cloned entity ID
 *
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
    const Vector2& meshExtent)
{
    // Just forward to MakeGameObject
    return MakeGameObject(app, name, mesh, pos, scale, rot,
        color, colliderType, isTrigger, meshExtent);
}

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
 *
 * @details
 * Prefab instantiation steps:
 * 1. Create base GameObject from prefab data
 * 2. Apply texture if specified
 * 3. Add SpriteAnimator if texture has spritesheet
 * 4. Special handling for enemy prefabs (AI controller + physics)
 * 5. Add ParticleEmitter if configured
 * 6. Set prefab tag for save/load
 *
 */
Entity FactoryInstantiatePrefab(
    GameApp& app,
    const std::string& prefabName,
    const Prefab& p,
    const Vector2& worldPos
) {
    Mesh2D* meshPtr = app.GetMesh((size_t)p.meshIndex);

    Entity e = MakeGameObject(
        app,
        prefabName,             // entity name
        meshPtr,
        worldPos,               // position to spawn
        p.scale,
        0.0f,                   // rotation default
        p.color,
        p.collider,
        p.isTrigger,
        { 1.f, 1.f }              // meshExtent (unit quad assumption)
    );

    // 2. Assign texture if there is 
    if (!p.texture.empty()) {
        if (auto* mr = app.GetRenderer(e)) {
            GLuint texID = ResourceManager::GetTexture(p.texture);
            if (texID != 0) {
                mr->SetTexture(texID);
            }
            else {
                DebugConsole::Get().Warning("Prefab texture not found: '" + p.texture + "\n");
            }
        }
    }

    if (!p.texture.empty())
    {
        if (const SpriteSheet* sh = ResourceManager::GetSpriteSheet(p.texture))
        {
            SpriteAnimator& an = app.AddAnimator(e);
            an.SetSheet(sh);
            an.SetSpeedFromFrameDuration(sh->frameDuration);
            an.Restart();
        }
    }

    if (prefabName == "ranged_mini_boss" ||
        prefabName == "EnemyContact" || prefabName == "burrow_mini_boss" ||
        prefabName == "heal_mini_boss" || prefabName == "Boss") {
        /// Add speed component into enemy entity
        app.AddPhysicsSpeedComponent(e, SpeedComponent(e, Vector2(0.0f, 0.0f)));

        /// Add mass component into enemy entity
        app.AddPhysicsMassComponent(e, MassComponent(e, 2.0f));
        EnemiesController& ec = app.AddEnemyController(e);
        if (Transform* t = app.GetTransform(e)) {
            ec.SetSpawn(t->GetPosition());
        }

        MobType mobType = MobType::RANGED;
        if (prefabName == "EnemyContact") {
            mobType = MobType::BASIC;
        }
        else if (prefabName == "burrow_mini_boss") {
            mobType = MobType::BURROW;
        }
        else if (prefabName == "heal_mini_boss") {
            mobType = MobType::HEAL;
        }
        else if (prefabName == "Boss") {
            mobType = MobType::FINALBOSS;
        }
        ec.SetMobType(mobType);

        const char* sheetName = nullptr;
        if (prefabName == "ranged_mini_boss") {
            sheetName = "Idle_ranged_enemy_R";
        }
        else if (prefabName == "EnemyContact") {
            sheetName = "default_enemy_R";
        }
        else if (prefabName == "burrow_mini_boss") {
            sheetName = "burrow_enemy";
        }
        else if (prefabName == "heal_mini_boss") {
            sheetName = "heal_enemy";
        }
        else if (prefabName == "Boss") {
            sheetName = "final_boss";
        }

        if (sheetName) {
            SpriteAnimator* an = app.GetAnimator(e);
            if (!an) {
                an = &app.AddAnimator(e);
            }
            if (const SpriteSheet* sh = ResourceManager::GetSpriteSheet(sheetName)) {
                an->SetSheet(sh);
                an->SetSpeedFromFrameDuration(sh->frameDuration);
                an->Restart();
            }
            else {
                DebugConsole::Get().Warning("[Enemy] Missing spritesheet key '" + std::string(sheetName) + "'\n");
            }
        }

        std::string navPath = ComputeNavMapPath(app);
        ec.SetMap(navPath);
        ec.SetLuaDrivenMovement(true);
        std::string scriptPath = "Scripts/include/chase.lua";
        if (prefabName == "ranged_mini_boss") {
            scriptPath = "Scripts/include/enemy_ranged.lua";
        } else if (prefabName == "burrow_mini_boss") {
            scriptPath = "Scripts/include/enemy_burrow.lua";
        } else if (prefabName == "heal_mini_boss") {
            scriptPath = "Scripts/include/enemy_heal.lua";
        } else if (prefabName == "Boss") {
            scriptPath = "Scripts/include/enemy_boss.lua";
        } else if (prefabName == "EnemyContact") {
            scriptPath = "Scripts/include/enemy_basic.lua";
        }
        app.AddLuaScript(e, scriptPath);
        //ec.SetMap("Assets/maps/labyrinth.txt");

        /// check if the current entity have EnemyController
        if (auto* ec2 = app.GetEnemyController(e)) {
            /// Assign global EntityForceProxy to the enemy controller
            ec2->setForceProxy(g_entityForceProxy);
            DebugConsole::Get().Success("PROXY FOR ENEMY HAVE BEEN CREATED");
        }
    }

    // 3. Attach particle emitter if requested
    if (p.hasEmitter) {
        ParticleEmitter& em = app.AddEmitter(e);

        em.enabled = p.emitter.enabled;
        em.rate = p.emitter.rate;
        em.particleLife = p.emitter.particleLife;
        em.velMin = p.emitter.velMin;
        em.velMax = p.emitter.velMax;
        em.offset = p.emitter.offset;
        em.sizeStart = p.emitter.sizeStart;
        em.sizeEnd = p.emitter.sizeEnd;
        em.colorStart = p.emitter.colorStart;
        em.colorEnd = p.emitter.colorEnd;
        em.timeAccumulator = 0.f;

        // mesh (quad) to use for rendering particles
        em.quad = app.GetMesh((size_t)p.emitter.meshIndex);

        // optional particle texture
        em.texture = 0;
        if (!p.emitter.texture.empty()) {
            GLuint texID = ResourceManager::GetTexture(p.emitter.texture);
            if (texID != 0) {
                em.texture = texID;
            }
            else {
                DebugConsole::Get().Warning("[WARN] Emitter texture not found: '" + p.emitter.texture + "\n");
            }
        }
    }

    if (p.hasLight) {
        LightComponent& l = app.AddLight(e);
        l.enabled = p.light.enabled;

        l.glow.enabled = p.light.glow.enabled;
        l.glow.radius = p.light.glow.radius;
        l.glow.intensity = p.light.glow.intensity;
        l.glow.opacity = p.light.glow.opacity;
        l.glow.color = p.light.glow.color;
        l.glow.offset = p.light.glow.offset;
        l.glow.softness = p.light.glow.softness;

        l.source.enabled = p.light.source.enabled;
        l.source.radius = p.light.source.radius;
        l.source.intensity = p.light.source.intensity;
        l.source.opacity = p.light.source.opacity;
        l.source.color = p.light.source.color;
        l.source.offset = p.light.source.offset;
        l.source.attenuation = p.light.source.attenuation;

        l.ember.enabled = p.light.ember.enabled;
        l.ember.rate = p.light.ember.rate;
        l.ember.particleLife = p.light.ember.particleLife;
        l.ember.velMin = p.light.ember.velMin;
        l.ember.velMax = p.light.ember.velMax;
        l.ember.colorStart = p.light.ember.colorStart;
        l.ember.colorEnd = p.light.ember.colorEnd;
        l.ember.sizeStart = p.light.ember.sizeStart;
        l.ember.sizeEnd = p.light.ember.sizeEnd;
        l.ember.additive = p.light.ember.additive;
        l.ember.offset = p.light.ember.offset;

        l.flicker.enabled = p.light.flicker.enabled;
        l.flicker.intensityMin = p.light.flicker.intensityMin;
        l.flicker.intensityMax = p.light.flicker.intensityMax;
        l.flicker.speed = p.light.flicker.speed;

        if (l.ember.enabled) {
            ParticleEmitter* existing = app.GetEmitter(e);
            ParticleEmitter& em = existing ? *existing : app.AddEmitter(e);
            em.enabled = true;
            em.rate = l.ember.rate;
            em.particleLife = l.ember.particleLife;
            em.velMin = l.ember.velMin;
            em.velMax = l.ember.velMax;
            em.colorStart = l.ember.colorStart;
            em.colorEnd = l.ember.colorEnd;
            em.sizeStart = l.ember.sizeStart;
            em.sizeEnd = l.ember.sizeEnd;
            em.additive = l.ember.additive;
            em.offset = l.ember.offset;
            em.quad = app.GetMesh(1);
        }
    }

    // --- Puzzle Object Component Support ---
    if (prefabName == "BurrowWall") {
        auto& po = app.AddPuzzleObject(e, PuzzleKind::BurrowWall, 2);
        po.active = false;
        po.groupId = 1; // Default groupId
    }
    else if (prefabName == "Lever" || prefabName == "LeverActivated") {
        auto& po = app.AddPuzzleObject(e, PuzzleKind::Lever, 1);
        po.active = (prefabName == "LeverActivated");
        po.groupId = 1; // Default groupId
        po.i0 = 1;      // i0=1 means it's a toggle lever
    }
    else if (prefabName == "PuzzleGenerator" || prefabName == "PuzzleGenerator_On") {
        // PuzzleGenerator is a ShootTarget according to PuzzleSystem
        auto& po = app.AddPuzzleObject(e, PuzzleKind::ShootTarget, 1);
        po.active = (prefabName == "PuzzleGenerator_On");
        po.groupId = 1; // Default groupId
        po.i0 = 1;      // i0 = required hits
        po.i1 = (prefabName == "PuzzleGenerator_On" ? 1 : 0); // i1 = current hits
        po.i2 = 1;      // i2 = 1 requires player projectiles
    }
    else if (prefabName == "healingMemory" || prefabName == "healingMemory_On") {
        auto& po = app.AddPuzzleObject(e, PuzzleKind::HealingMemory, 1);
        po.active = (prefabName == "healingMemory_On");
        po.i0 = 1;      // memoryId
        po.i1 = -1;     // requiredMemoryId (-1 = none)
        po.f0 = 150.0f; // healRadius
        po.f1 = 8.0f;   // memoryDuration
        po.s0 = "A soldier's last memory..."; // narrative
        po.s1 = "MemoryMaps/memory_01.png";   // mapImagePath
    }
    else if (prefabName == "DoorLock") {
        auto& po = app.AddPuzzleObject(e, PuzzleKind::DoorLock, 1);
        po.active = false;
        po.groupId = 1; // Default groupId
        po.i0 = 0;      // i0=0 means it requires only groupId (Legacy mode)
    }
    else if (prefabName == "Floor_Tile_Fragile") {
        auto& po = app.AddPuzzleObject(e, PuzzleKind::BurrowFloor, 1);
        po.active = true;
        po.f0 = 0.8f;   // collapseDelay (aligned with mapGenerator)
        po.f1 = 3.0f;   // respawnDelay (aligned with mapGenerator)
        po.f2 = 0.8f;   // currentTimer (start with collapseDelay)
        po.i0 = 1;      // ignoreWhenBurrowed = true
        po.i1 = 0;      // collapseOnce = false
        po.i2 = 1;      // createFallHazard = true
    }
    else if (prefabName == "MutationHealer") {
        auto& po = app.AddPuzzleObject(e, PuzzleKind::MutationHealer, 1);
        po.active = true;
    }

    // 4. Tag prefab name for saving / editor selection
    app.SetPrefabTag(e, prefabName);

    if (prefabName == "Crate" || prefabName == "BurrowWall" || prefabName == "Floor_Tile_Fragile") {
        if (auto* c = app.GetCollider(e)) {
            c->isPassableWhenBurrowed = true;
        }
    }

    return e;
}

/**
 * @brief Creates a wall tile entity from variant data.
 *
 * Implements wall tile creation from variant data including grid-to-world
 * coordinate conversion, texture application, and collider configuration.
 *
 * @param app         Game context & ECS access point
 * @param variant     Variant placement data (grid pos, texture, solid flag)
 * @param tileSize    Size of each grid cell in world units
 * @param meshExtent  Actual mesh size before scaling (default unit quad)
 *
 * @return Entity Newly created wall entity ID
 */
Entity MakeWallTileFromVariant(
    GameApp& app,
    const VariantPlacement& variant,
    float tileSize,
    const Vector2& meshExtent)
{
    // --- grid → world ---
    const Vector2 worldPos(
        variant.gx * tileSize,
        variant.gy * tileSize
    );
    const Vector2 worldSize(tileSize, tileSize);

    // --- name ---
    std::string entityName = variant.name.empty()
        ? ("WallTile_" + std::to_string(variant.gx) + "_" + std::to_string(variant.gy))
        : variant.name;

    // --- mesh ---
    Mesh2D* mesh = app.GetMesh(1);

    // --- create base entity ---
    Entity wallEntity = MakeGameObject(
        app,
        entityName,
        mesh,
        worldPos,
        worldSize,
        0.0f,
        Vector3(variant.color[0], variant.color[1], variant.color[2]),
        ColliderType::Box,
        false,
        meshExtent
    );

    // --- texture resolve ---
    if (auto* mr = app.GetRenderer(wallEntity)) {

        GLuint texID = ResourceManager::GetTexture(variant.textureKey);

        // fallback if missing or invalid
        if (texID == 0 || glIsTexture(texID) == GL_FALSE) {
            texID = ResourceManager::GetTexture("wall_B&T");
        }

        if (texID != 0) {
            mr->SetTexture(texID);
        }
    }


    // --- collider override ---
    if (auto* collider = app.GetCollider(wallEntity)) {
        if (!variant.solid) {
            collider->isTrigger = true;
        }
        if (variant.colliderSize[0] > 0.0f || variant.colliderSize[1] > 0.0f) {
            collider->size = Vector2(variant.colliderSize[0], variant.colliderSize[1]);
        }
    }

    // --- prefab tag ---
    app.SetPrefabTag(wallEntity, "WallVariant");
    return wallEntity;
}

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
 * @return std::vector<Entity> Vector of created entity IDs
 *
 */
std::vector<Entity> CreateVariantEntities(
    GameApp& app,
    const std::vector<VariantPlacement>& variants,
    float tileSize) {
    std::vector<Entity> createdEntities;
    createdEntities.reserve(variants.size());

    for (const auto& variant : variants) {
        Entity entity = INVALID_ENTITY;

        if (variant.type == "WallTile") {
            if (variant.removed) {
                createdEntities.push_back(INVALID_ENTITY);
                continue;
            }
            entity = MakeWallTileFromVariant(app, variant, tileSize);
        }
        else {
            if (variant.removed) {
                createdEntities.push_back(INVALID_ENTITY);
                continue;
            }

            const Prefab* p = app.prefabManager.Get(variant.type);
            Vector2 scale = p ? p->scale : Vector2(tileSize, tileSize);

            // Prefabs use top-left origin. Align center of prefab with center of tile.
            Vector2 worldPos(
                (variant.gx + 0.5f) * tileSize - scale.x * 0.5f,
                (variant.gy + 0.5f) * tileSize - scale.y * 0.5f
            );

            entity = app.InstantiatePrefab(variant.type, worldPos);

            if (entity == INVALID_ENTITY) {
                DebugConsole::Get().Warning("[Factories] Unknown variant type or prefab not found: '" + variant.type + "', skipping\n");
            }
        }

        if (entity != INVALID_ENTITY) {
            // Apply overrides
            if (auto* mr = app.GetRenderer(entity)) {
                mr->SetColor(Vector3(variant.color[0], variant.color[1], variant.color[2]));
            }
            if (auto* col = app.GetCollider(entity)) {
                if (variant.colliderSize[0] > 0.0f || variant.colliderSize[1] > 0.0f) {
                    col->size = Vector2(variant.colliderSize[0], variant.colliderSize[1]);
                }
            }
            if (variant.hasSignature) {
                app.AddPersistentTag(entity);
            }

            if (variant.type == "ranged_mini_boss" ||
                variant.type == "EnemyContact" ||
                variant.type == "burrow_mini_boss" ||
                variant.type == "heal_mini_boss" ||
                variant.type == "Boss")
            {
                if (auto* ec = app.GetEnemyController(entity)) {
                    if (Transform* t = app.GetTransform(entity)) {
                        ec->SetSpawn(t->GetPosition());
                    }

                    ec->SetLuaDrivenMovement(true);
                    ec->setForceProxy(g_entityForceProxy);

                    // std::string navPath = ComputeNavMapPath(app);
                    // ec->SetMap(navPath);

                    std::string scriptPath = "Scripts/include/chase.lua";
                    if (variant.type == "ranged_mini_boss") {
                        scriptPath = "Scripts/include/enemy_ranged.lua";
                    } else if (variant.type == "burrow_mini_boss") {
                        scriptPath = "Scripts/include/enemy_burrow.lua";
                    } else if (variant.type == "heal_mini_boss") {
                        scriptPath = "Scripts/include/enemy_heal.lua";
                    } else if (variant.type == "Boss") {
                        scriptPath = "Scripts/include/enemy_boss.lua";
                    } else if (variant.type == "EnemyContact") {
                        scriptPath = "Scripts/include/enemy_basic.lua";
                    }
                    app.AddLuaScript(entity, scriptPath);
                }
            }

            // --- Puzzle override restore ---
            if (variant.hasPuzzleData) {
                if (PuzzleObject* po = app.GetPuzzleObject(entity)) {
                    po->kind = static_cast<PuzzleKind>(variant.puzzleKind);
                    po->groupId = variant.puzzleGroupId;
                    po->active = variant.puzzleActive;

                    po->i0 = variant.pi0;
                    po->i1 = variant.pi1;
                    po->i2 = variant.pi2;
                    po->i3 = variant.pi3;

                    po->f0 = variant.pf0;
                    po->f1 = variant.pf1;
                    po->f2 = variant.pf2;
                    po->f3 = variant.pf3;

                    po->s0 = variant.ps0;
                    po->s1 = variant.ps1;
                }
            }

            createdEntities.push_back(entity);
        }
    }
    return createdEntities;
}

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
 * @return std::vector<Entity> Vector of created entity IDs (empty if file not found)
 */
std::vector<Entity> ApplyRoomVariants(
    GameApp& app,
    const std::string& scenePath,
    int gridHeight,
    float tileSize) {

    std::filesystem::path p(scenePath);
    const std::string stem = p.stem().string();
    const std::string variantPath = AssetPath("variants/" + stem + "_variants.json");

    // Load variants
    std::vector<VariantPlacement> variants;
    if (!Variant::LoadFile(variantPath, gridHeight, variants)) {
        return {}; // Return empty vector
    }

    // Create entities from variants
    return CreateVariantEntities(app, variants, tileSize);
}

/**
 * @brief Spawns a projectile entity fired by a shooter (player or enemy).
 *
 * Creates a bullet GameObject via MakeGameObject() (Transform + Renderer + Collider),
 * then attaches a Projectile component with velocity, lifetime, and damage.
 *
 * @param app       Game context & ECS access point.
 * @param shooter   Entity that fired the projectile (used for Transform and ownership).
 * @param dir       Normalized firing direction in world space.
 * @param fromEnemy True if this projectile was fired by an enemy; false if fired by player.
 *
 * @return Entity ID of the spawned projectile, or INVALID_ENTITY if creation fails.
 */
Entity MakeProjectile(
    GameApp& app,
    Entity shooter,
    const Vector2& dir,
    bool fromEnemy)
{
    Transform* et = app.GetTransform(shooter);
    if (!et) return INVALID_ENTITY;

    Mesh2D* mesh = app.GetMesh(1);
    if (!mesh) return INVALID_ENTITY;

    const Vector2 scale{ 16.0f, 16.0f };

    const Vector2 shooterPos = et->GetPosition();
    Vector2 pos{
        shooterPos.x + dir.x * 20.0f - scale.x * 0.5f,
        shooterPos.y + dir.y * 20.0f - scale.y * 0.5f
    };

    PlayerController* pc = app.GetController(shooter);
    if (!fromEnemy)
    {
        Vector2 centerPos = et->GetPosition() + et->GetScale() * 0.5f;

        ResourceManager::PlaySfx("Projectile", 0.2f);

        float spawnHeightOffset = static_cast<float>(et->GetScale().y * 0.35);
        pos.y = centerPos.y - spawnHeightOffset;

        if (dir.x < 0.0f)
        {
            Vector2 spawnOffset = et->GetScale() * 0.5f;
            float distance = std::max(spawnOffset.Length(), spawnOffset.Length());
            pos.x = centerPos.x - distance;
        }
        else
        {
            Vector2 spawnOffset = et->GetScale() * 0.39f;
            float distance = std::max(spawnOffset.Length(), spawnOffset.Length());
            pos.x = centerPos.x + distance;
        }
    }

    const Vector3 color{ 1.0f, 1.0f, 1.0f };
    const Vector2 unit{ 1.0f, 1.0f };

    static int sBulletId = 0;
    int dmg =0;
    if (pc) {
        if (pc->godmode == true) {
            dmg = 9999;
        }
        else
        {
            dmg = pc->getPlayerProjectileDmg();
        }
    }

    Entity bullet = MakeGameObject(
        app,
        (fromEnemy ? "EnemyBullet_" : "PlayerBullet_") + std::to_string(sBulletId++),
        mesh,
        pos,
        scale,
        0.0f,
        color,
        ColliderType::Box,
        true,
        unit
    );
    const float speed = fromEnemy ? 300.0f : pc->getPlayerProjectileSpeed();
    const float life = 2.0f;
    const int damage = fromEnemy ? 5 : dmg;

    app.AddProjectile(bullet, shooter, Vector2{ dir.x * speed, dir.y * speed }, life, damage, fromEnemy);

    // --- Projectile Animation ---
    SpriteAnimator& an = app.AddAnimator(bullet);
    const std::string sheetName = (dir.x >= 0) ? "projectile_Bullet_R" : "projectile_Bullet_L";
    if (const SpriteSheet* sh = ResourceManager::GetSpriteSheet(sheetName)) {
        an.SetSheet(sh);
        an.SetSpeedFromFrameDuration(sh->frameDuration);
        an.Restart();
    }

    return bullet;
}
