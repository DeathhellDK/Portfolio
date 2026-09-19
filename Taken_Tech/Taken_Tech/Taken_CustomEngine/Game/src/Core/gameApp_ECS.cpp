/**
 * @file     gameApp_ECS.cpp
 * @author   Jethro Sung, Woh Kye Le, Tan Wei Liang Terril
 * @email    sung.h , w.kyele, t.weiliangterril
 * 
 * @date     2025-11-01
 *
 * @brief    Implements the Entity-Component-System (ECS) integration for GameApp.
 * 
 * This file provides the concrete ECS logic used by GameApp:
 * - Component creation helpers (Transform, Renderer, Collider, Animator, Controller, etc.)
 * - Safe retrieval and query utilities for component data
 * - Entity lifetime management (creation, destruction, removal)
 * - Prefab instantiation via the factory system
 * - Player-related and texture-management helpers
 *
 * It serves as the bridge between the entity manager, system manager,
 * and component maps within the GameApp runtime.
 *
 * @version 1.0
 * 
 * 
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include "Core/gameApp.h"
#include "Core/ResourceManager.h"
#include "factories.h"
#include <algorithm>
#include <iostream>
#include "Input/DebugConsole.hpp"

namespace{
    inline bool AlmostEqual    (float a, float b, float eps = 1e-3f)                   { return std::fabs(a - b) <= eps; }
    inline bool AlmostEqualVec2(const Vector2& a, const Vector2& b, float eps = 1e-3f) {  return AlmostEqual(a.x, b.x, eps) && AlmostEqual(a.y, b.y, eps); }
    inline bool AlmostEqualVec3(const Vector3& a, const Vector3& b, float eps = 1e-3f) {
        return AlmostEqual(a.x, b.x, eps) &&
            AlmostEqual(a.y, b.y, eps) &&
            AlmostEqual(a.z, b.z, eps);
    }
}

// ============================== Component adders ===============================
// =====================
/*
 * @brief Add a Transform component to an entity.
 *
 * @param e Entity ID to attach the component to.
 * @param pos World-space position.
 * @param scale Local scaling.
 * @param rot Rotation in radians.
 * @return Reference to the newly created Transform component.
*/
Transform& GameApp::AddTransform(Entity e, const Vector2& pos, const Vector2& scale, float rot) {
    // Allocate + construct in pool memory
    Transform* t = transformPool.Create(e, pos, scale, rot);
    transforms[e] = t;

    t->SetLocalPosition(t->GetPosition());
    t->SetLocalScale(t->GetScale());
    t->SetLocalRotation(t->GetRotation());
    t->SetParent(INVALID_ENTITY);

    Signature sig = entityManager.GetSignature(e);
    sig.set(TRANSFORM);
    SetSignature(e, sig);

    return *t;
    /*auto result = transforms.emplace(std::piecewise_construct,
        std::forward_as_tuple(e),
        std::forward_as_tuple(e, pos, scale, rot));
    auto& it = result.first;
    it->second.SetLocalPosition(it->second.GetPosition());
    it->second.SetLocalScale(it->second.GetScale());
    it->second.SetLocalRotation(it->second.GetRotation());
    it->second.SetParent(INVALID_ENTITY);

    Signature sig = entityManager.GetSignature(e);
    sig.set(TRANSFORM);
    entityManager.SetSignature(e, sig);
    return it->second;*/
}


/*
 * @brief Add a MeshRenderer component to an entity.
 *
 * @param e Entity ID to attach the component to.
 * @param mesh Pointer to the mesh to render.
 * @param color RGB color to apply to the mesh.
 * @return Reference to the newly created MeshRenderer component.
*/
MeshRenderer& GameApp::AddRenderer(Entity e, Mesh2D* mesh, const Vector3& color) {
    MeshRenderer* r = rendererPool.Create(e, mesh, color);
    renderers[e] = r;

    Signature sig = entityManager.GetSignature(e);
    sig.set(MESHRENDERER);
    SetSignature(e, sig);

    return *r;
}

/*
 * @brief Add a Collider component to an entity.
 *
 * @param e Entity ID to attach the collider to.
 * @param size Collider box dimensions.
 * @param trigger Whether this collider acts as a trigger.
 * @return Reference to the newly created Collider component.
*/
Collider& GameApp::AddCollider(Entity e, const Vector2& size, bool trigger) {
    Collider* c = colliderPool.Create(e, size, trigger);
    colliders[e] = c;

    Signature sig = entityManager.GetSignature(e);
    sig.set(COLLIDER);
    SetSignature(e, sig);

    return *c;
}

/*
 * @brief Add a PlayerController component to an entity.
 *
 * @param e Entity ID to attach the controller to.
 * @return Reference to the newly created PlayerController component.
*/
PlayerController& GameApp::AddController(Entity e) {
    auto result = controllers.emplace(std::piecewise_construct,
        std::forward_as_tuple(e),
        std::forward_as_tuple(e));
    auto& it = result.first;

    Signature sig = entityManager.GetSignature(e);
    sig.set(PLAYERCONTROLLER);
    SetSignature(e, sig);
    return it->second;
}

/*
 * @brief Add a SpriteAnimator component to an entity.
 *
 * @param e Entity ID to attach the animator to.
 * @return Reference to the newly created SpriteAnimator component.
*/
SpriteAnimator& GameApp::AddAnimator(Entity e) {
    auto result = animators.emplace(std::piecewise_construct,
        std::forward_as_tuple(e),
        std::forward_as_tuple(e));
    auto& it = result.first;

    Signature sig = entityManager.GetSignature(e);
    sig.set(SPRITEANIMATOR);
    SetSignature(e, sig);
    return it->second;
}

/**
 * @brief Adds a temporary physics SpeedComponent to an entity.
 *
 * @param e Entity to attach the component to.
 * @param speedComp Component containing linear velocity data.
 */
void GameApp::AddPhysicsSpeedComponent(Entity e, const SpeedComponent& speedComp){
    temp_spdComponent.emplace(e, speedComp);
    Signature sig = entityManager.GetSignature(e);
    sig.set(SPEED);
    SetSignature(e, sig);
}

/**
 * @brief Adds a temporary physics MassComponent to an entity.
 *
 * @param e Entity to attach the component to.
 * @param massComp Component containing mass data.
 */
void GameApp::AddPhysicsMassComponent(Entity e, const MassComponent& massComp){
    temp_massComponent.emplace(e, massComp);
    Signature sig = entityManager.GetSignature(e);
    sig.set(MASS);
    SetSignature(e, sig);
}

/**
 * @brief Adds a PersistentTag component to an entity.
 *
 * Marks an entity to persist between level transitions.
 *
 * @param e Entity to tag.
 * @return Reference to the newly created PersistentTag component.
 */
PersistentTag& GameApp::AddPersistentTag(Entity e) {
    auto result = persistentTags.emplace(std::piecewise_construct,
        std::forward_as_tuple(e),
        std::forward_as_tuple(e));
    auto& it = result.first;

    Signature sig = entityManager.GetSignature(e);
    sig.set(PERSISTENTTAG);
    SetSignature(e, sig);
    return it->second;
}

/**
 * @brief Removes the PersistentTag component from an entity.
 * 
 * @param e Entity to untag.
 */
void GameApp::RemovePersistentTag(Entity e) {
    if (persistentTags.erase(e)) {
        Signature sig = entityManager.GetSignature(e);
        sig.reset(PERSISTENTTAG);
        SetSignature(e, sig);
    }
}

/**
 * @brief Checks if an entity is marked as persistent.
 * 
 * @param e Entity to check.
 * @return true if the entity has a PersistentTag, false otherwise.
 */
bool GameApp::HasPersistentTag(Entity e) const {
    return persistentTags.find(e) != persistentTags.end();
}


/**
 * @brief Adds an EnemiesController component to an entity.
 *
 * Manages AI logic and movement for enemies.
 *
 * @param e Entity to attach the controller to.
 * @return Reference to the newly created EnemiesController component.
 */
EnemiesController& GameApp::AddEnemyController(Entity e) {
    auto result = enemiesControllers.emplace(std::piecewise_construct,
        std::forward_as_tuple(e),
        std::forward_as_tuple(e, &gameEvents));

    auto& it = result.first;

    Signature sig = entityManager.GetSignature(e);
    sig.set(ENEMYCONTROLLER);
    SetSignature(e, sig);

    // Initialize with current grid if available
    if (!aiGrid_.empty()) {
        std::vector<std::string> original = aiGrid_;
        std::reverse(original.begin(), original.end());
        it->second.SetGrid(original, aiTileSize_);
    }

    return it->second;
}

/**
 * @brief Adds a PuzzleObject component to an entity.
 * 
 * @param e Entity to attach the component to.
 * @param kind The type of puzzle element (e.g., Lever, DoorLock).
 * @param groupId The group ID for linking puzzle elements.
 * @return Reference to the newly created PuzzleObject component.
 */
PuzzleObject& GameApp::AddPuzzleObject(Entity e, PuzzleKind kind, int groupId)
{
    auto [it, inserted] = puzzleObjects.emplace(e, PuzzleObject(e));
    PuzzleObject& po = it->second;

    po.kind = kind;
    po.groupId = groupId;

    Signature sig = entityManager.GetSignature(e);
    sig.set(PUZZLEOBJECT);
    SetSignature(e, sig);

    return po;
}

/**
 * @brief Adds a ProjectileComponent to an entity.
 * 
 * @param e Entity to attach the component to.
 * @param source The entity that fired the projectile.
 * @param vel Velocity vector of the projectile.
 * @param lifeSeconds Duration before the projectile is destroyed.
 * @param dmg Damage value.
 * @param fromEnemy True if fired by an enemy, false if by player.
 * @return Reference to the newly created ProjectileComponent.
 */
ProjectileComponent& GameApp::AddProjectile(Entity e, Entity source, const Vector2& vel, float lifeSeconds, int dmg, bool fromEnemy)
{
    ProjectileComponent* p = projectilePool.Create(e, source, vel, lifeSeconds, dmg, fromEnemy);
    projectiles[e] = p;

    Signature sig = entityManager.GetSignature(e);
    sig.set(PROJECTILE);
    SetSignature(e, sig);

    return *p;
}

/**
 * @brief Retrieves the PuzzleObject component for an entity (non-const).
 * 
 * @param e Entity to query.
 * @return Pointer to the PuzzleObject, or nullptr if not found.
 */
PuzzleObject* GameApp::GetPuzzleObject(Entity e)
{
    auto it = puzzleObjects.find(e);
    return (it == puzzleObjects.end()) ? nullptr : &it->second;
}

/**
 * @brief Retrieves the PuzzleObject component for an entity (const).
 * 
 * @param e Entity to query.
 * @return Const pointer to the PuzzleObject, or nullptr if not found.
 */
const PuzzleObject* GameApp::TryGetPuzzleObject(Entity e) const
{
    auto it = puzzleObjects.find(e);
    return (it == puzzleObjects.end()) ? nullptr : &it->second;
}

/**
 * @brief Updates the AI navigation grid for all enemies.
 * 
 * Reverses the grid (y-axis flip) to match internal representation and pushes
 * it to all active EnemyControllers.
 * 
 * @param grid Vector of strings representing the grid layout.
 * @param tileSize Size of each tile in world units.
 */
void GameApp::SetEnemyGrid(const std::vector<std::string>& grid, float tileSize)
{
    std::vector<std::string> reversed = grid;
    std::reverse(reversed.begin(), reversed.end());

    aiGrid_ = std::move(reversed);
    aiTileSize_ = tileSize;

    // Push this grid into every enemy controller in the current scene
    for (auto& [entity, controller] : enemiesControllers)
    {
        controller.SetGrid(grid, tileSize);
    }
}

/**
 * @brief Adds a ParticleEmitter component to an entity.
 *
 * Allows an entity to emit particles for visual effects.
 *
 * @param e Entity to attach the emitter to.
 * @return Reference to the newly created ParticleEmitter component.
 */
ParticleEmitter& GameApp::AddEmitter(Entity e) {
    auto result = emitters.emplace(std::piecewise_construct,
        std::forward_as_tuple(e),
        std::forward_as_tuple(e));
    auto& it = result.first;

    Signature sig = entityManager.GetSignature(e);
    sig.set(PARTICLEEMITTER);
    SetSignature(e, sig);
    return it->second;
}

/**
 * @brief Add a LightComponent and set the LIGHT bit in the entity signature.
 * @param e Entity to attach the light to.
 * @return Reference to the created LightComponent.
 */
LightComponent& GameApp::AddLight(Entity e) {
    auto [it, inserted] = lights.emplace(e, LightComponent(e));
    Signature sig = entityManager.GetSignature(e);
    sig.set(LIGHT);
    SetSignature(e, sig);
    return it->second;
}
// =====================
// ================================================================================



// ============================== Component getters ===============================
// =====================

EnemiesController*   GameApp::GetEnemyController(Entity e) { return FindComponent(enemiesControllers, e); }
Transform*           GameApp::GetTransform(Entity e)       { return FindComponent(transforms, e); }
MeshRenderer*        GameApp::GetRenderer(Entity e)        { return FindComponent(renderers, e); }
Collider*            GameApp::GetCollider(Entity e)        { return FindComponent(colliders, e); }
PlayerController*    GameApp::GetController(Entity e)      { return FindComponent(controllers, e); }
SpriteAnimator*      GameApp::GetAnimator(Entity e)        { return FindComponent(animators, e); }
ParticleEmitter*     GameApp::GetEmitter(Entity e)         { return FindComponent(emitters, e); }
LightComponent* GameApp::GetLight(Entity e) { return FindComponent(lights, e); }

//bridging for editor to add/remove components

/**
 * @brief Editor bridge: Adds a default MeshRenderer to an entity.
 * @param e Entity to attach the component to.
 * @return Pointer to the new MeshRenderer.
 */
MeshRenderer* GameApp::AddRendererComponent(Entity e)
{
    // Basic defaults – editor will tweak color/texture later
    Mesh2D* mesh = nullptr;
    if (!meshes.empty())
    {
        // Use first mesh as a default quad / sprite mesh
        mesh = meshes[0].get();
    }

    Vector3 color{ 1.f, 1.f, 1.f };

    MeshRenderer& mr = AddRenderer(e, mesh, color);
    return &mr;
}

/**
 * @brief Editor bridge: Adds a default Collider to an entity.
 * @param e Entity to attach the component to.
 * @return Pointer to the new Collider.
 */
Collider* GameApp::AddColliderComponent(Entity e)
{
    Vector2 size{ 1.0f, 1.0f };
    bool trigger = false;
    Collider& c = AddCollider(e, size, trigger);
    return &c;
}

/**
 * @brief Editor bridge: Adds a default SpriteAnimator to an entity.
 * @param e Entity to attach the component to.
 * @return Pointer to the new SpriteAnimator.
 */
SpriteAnimator* GameApp::AddAnimatorComponent(Entity e)
{
    SpriteAnimator& a = AddAnimator(e);
    return &a;
}

/**
 * @brief Editor bridge: Adds a PlayerController to an entity.
 * Also hooks up physics force proxy if applicable.
 * @param e Entity to attach the component to.
 * @return Pointer to the new PlayerController.
 */
PlayerController* GameApp::AddControllerComponent(Entity e)
{
    PlayerController& c = AddController(e);
    // If this is the player entity, hook up the force proxy
    if (e == playerEntity && g_entityForceProxy)
    {
        c.setForceProxy(g_entityForceProxy);
    }

    // If an emitter already exists on this entity, bind it
    if (auto* em = GetEmitter(e))
    {
        c.BindEmitter(em);
        em->enabled = false; // start disabled; controller will toggle it
    }

    return &c;
}

/**
 * @brief Editor bridge: add a ParticleEmitter with sensible defaults.
 * @param e Entity to attach the component to.
 * @return Pointer to the created ParticleEmitter.
 */
ParticleEmitter* GameApp::AddEmitterComponent(Entity e)
{
    ParticleEmitter& em = AddEmitter(e);

    // Start with generic, visible defaults that work for any
    em.enabled = true;         // let it spawn immediately
    em.rate = 40.0f;
    em.particleLife = 0.5f;
    em.velMin = Vector2{ -20.f, 40.f };
    em.velMax = Vector2{ 20.f, 80.f };
    em.offset = Vector2{ 0.f, 0.f };
    em.sizeStart = 8.f;
    em.sizeEnd = 1.f;
    em.colorStart = Vector3{ 0.8f, 0.8f, 0.8f };
    em.colorEnd = Vector3{ 0.f, 0.f, 0.f };
    em.timeAccumulator = 0.f;

    // basic mesh: use default quad so it’s actually drawable
    em.quad = GetMesh(0);
    em.texture = 0;

    // If this entity is the player, override with Player prefab data
    if (e == playerEntity)
    {
        const Prefab* p = prefabManager.Get("Player");
        if (p && p->hasEmitter)
        {
            em.enabled = p->emitter.enabled;
            em.rate = p->emitter.rate;
            em.particleLife = p->emitter.particleLife;
            em.velMin = p->emitter.velMin;
            em.velMax = p->emitter.velMax;
            em.offset = p->emitter.offset;
            em.sizeStart = p->emitter.sizeStart;
            em.sizeEnd = p->emitter.sizeEnd;
            em.colorStart = p->emitter.colorStart;
            em.colorEnd = p->emitter.colorEnd;
            em.timeAccumulator = 0.f;

            em.quad = GetMesh(static_cast<size_t>(p->emitter.meshIndex));
            em.texture = 0;
            if (!p->emitter.texture.empty())
            {
                if (GLuint texID = ResourceManager::GetTexture(p->emitter.texture))
                    em.texture = texID;
            }
        }
    }

    // If this entity happens to have a PlayerController, bind it.
    if (auto* ctrl = GetController(e))
    {
        ctrl->BindEmitter(&em);

        // start disabled and let movement switch it on.
        em.enabled = false;
    }

    return &em;
}

/**
 * @brief Editor bridge: add a LightComponent with default torch-like settings.
 * @param e Entity to attach the component to.
 * @return Pointer to the created LightComponent.
 */
LightComponent* GameApp::AddLightComponent(Entity e)
{
    LightComponent& l = AddLight(e);
    l.enabled = true;
    l.glow.enabled = true;
    l.glow.radius = 180.0f;
    l.glow.intensity = 1.0f;
    l.glow.opacity = 0.35f;
    l.glow.color = Vector3{ 1.0f, 0.85f, 0.55f };
    l.glow.offset = Vector2{ 0.0f, 0.0f };
    return &l;
}

/**
 * @brief Editor bridge: Adds a SpeedComponent (physics) to an entity.
 * 
 * @param e Entity to attach the component to.
 * @return Pointer to the new SpeedComponent.
 */
SpeedComponent* GameApp::AddSpeedComponent(Entity e)
{
    // Use the explicit ctor so the component owner is correct
    SpeedComponent speed(e);        // use default for most ent

    // If this is the player entity, mirror MakePlayer
    if (e == playerEntity)
    {
        // In case you later switch back to PlayerPresetTag
        speed.speed = Vector2{ 0.0f, 0.0f };
    }


    AddPhysicsSpeedComponent(e, speed);

    auto it = temp_spdComponent.find(e);
    return (it != temp_spdComponent.end()) ? &it->second : nullptr;
}

/**
 * @brief Editor bridge: Adds a MassComponent (physics) to an entity.
 * 
 * @param e Entity to attach the component to.
 * @return Pointer to the new MassComponent.
 */
MassComponent* GameApp::AddMassComponent(Entity e)
{
	float initialMass = 2.0f;//default mass

	//mirror MakePlayer
    if (e == playerEntity || TryGetEnemyController(e) != nullptr)
    {
        initialMass = 2.0f;
    }
    MassComponent mass(e, initialMass);
    AddPhysicsMassComponent(e, mass);

    auto it = temp_massComponent.find(e);
    return (it != temp_massComponent.end()) ? &it->second : nullptr;
}

/**
 * @brief Removes the MeshRenderer component from an entity.
 * 
 * @param e Entity to modify.
 */
void GameApp::RemoveRendererComponent(Entity e)
{
    auto it = renderers.find(e);
    if (it == renderers.end())
        return;

    rendererPool.Destroy(it->second);
    renderers.erase(it);

    Signature sig = entityManager.GetSignature(e);
    sig.reset(MESHRENDERER);
    SetSignature(e, sig);
}

/**
 * @brief Removes the Collider component from an entity.
 * 
 * @param e Entity to modify.
 */
void GameApp::RemoveColliderComponent(Entity e)
{
    auto it = colliders.find(e);
    if (it == colliders.end())
        return;

    colliderPool.Destroy(it->second);
    colliders.erase(it);

    Signature sig = entityManager.GetSignature(e);
    sig.reset(COLLIDER);
    SetSignature(e, sig);
}

/**
 * @brief Removes the SpriteAnimator component from an entity.
 * 
 * @param e Entity to modify.
 */
void GameApp::RemoveAnimatorComponent(Entity e)
{
    auto it = animators.find(e);
    if (it == animators.end())
        return;

    animators.erase(it);

    Signature sig = entityManager.GetSignature(e);
    sig.reset(SPRITEANIMATOR);
    SetSignature(e, sig);
}

/**
 * @brief Removes the PlayerController component from an entity.
 * 
 * @param e Entity to modify.
 */
void GameApp::RemoveControllerComponent(Entity e)
{
    auto it = controllers.find(e);
    if (it == controllers.end())
        return;

    controllers.erase(it);

    Signature sig = entityManager.GetSignature(e);
    sig.reset(PLAYERCONTROLLER);
    SetSignature(e, sig);
}

/**
 * @brief Removes the ParticleEmitter component from an entity.
 * 
 * Unbinds the emitter from any associated controller before removal.
 * 
 * @param e Entity to modify.
 */
void GameApp::RemoveEmitterComponent(Entity e)
{
    auto it = emitters.find(e);
    if (it == emitters.end())
        return;

    // If this entity has a PlayerController, unbind the emitter first
    if (auto* ctrl = GetController(e))
    {
        // Allow nullptr here – this just means "no emitter"
        ctrl->BindEmitter(nullptr);
    }

    emitters.erase(it);

    Signature sig = entityManager.GetSignature(e);
    sig.reset(PARTICLEEMITTER);
    SetSignature(e, sig);
}

/**
 * @brief Remove a LightComponent and clear the LIGHT bit in the signature.
 * @param e Entity to detach the component from.
 */
void GameApp::RemoveLightComponent(Entity e)
{
    auto it = lights.find(e);
    if (it == lights.end())
        return;

    lights.erase(it);

    Signature sig = entityManager.GetSignature(e);
    sig.reset(LIGHT);
    SetSignature(e, sig);
}


/**
 * @brief Removes the SpeedComponent (physics) from an entity.
 * 
 * @param e Entity to modify.
 */
void GameApp::RemoveSpeedComponent(Entity e)
{
    auto it = temp_spdComponent.find(e);
    if (it == temp_spdComponent.end())
        return;

    temp_spdComponent.erase(it);

    Signature sig = entityManager.GetSignature(e);
    sig.reset(SPEED);
    SetSignature(e, sig);
}

/**
 * @brief Removes the MassComponent (physics) from an entity.
 * 
 * @param e Entity to modify.
 */
void GameApp::RemoveMassComponent(Entity e)
{
    auto it = temp_massComponent.find(e);
    if (it == temp_massComponent.end())
        return;

    temp_massComponent.erase(it);

    Signature sig = entityManager.GetSignature(e);
    sig.reset(MASS);
    SetSignature(e, sig);
}

/**
 * @brief Removes the EnemiesController component from an entity.
 * 
 * @param e Entity to modify.
 */
void GameApp::RemoveEnemyControllerComponent(Entity e)
{
	auto it = enemiesControllers.find(e);
	if (it == enemiesControllers.end())
		return;

	enemiesControllers.erase(it);

	Signature sig = entityManager.GetSignature(e);
	sig.reset(ENEMYCONTROLLER);
	SetSignature(e, sig);
}

/**
 * @brief Editor bridge: Adds an EnemiesController to an entity.
 * 
 * @param e Entity to attach the component to.
 * @return Pointer to the new EnemiesController.
 */
EnemiesController* GameApp::AddEnemyControllerComponent(Entity e)
{
    EnemiesController& ec = AddEnemyController(e);
    return &ec;
}

// --- const versions / TryGet ---
/*
 * @brief Try to retrieve a Transform component for an entity.
 *
 * @param e Entity ID to query.
 * @return Pointer to the Transform component, or nullptr if not found.
*/
const Transform* GameApp::TryGetTransform(Entity e) const {
    auto it = transforms.find(e);
    return (it != transforms.end()) ? it->second : nullptr;
}

/**
 * @brief Attempts to retrieve a ParticleEmitter component.
 * @param e Entity to query.
 * @return Pointer to ParticleEmitter or nullptr if not found.
 */
const ParticleEmitter* GameApp::TryGetEmitter(Entity e) const {
    auto it = emitters.find(e);
    return (it != emitters.end()) ? &it->second : nullptr;
}

/**
 * @brief Attempts to retrieve a LightComponent.
 * @param e Entity to query.
 * @return Pointer to LightComponent or nullptr if not found.
 */
const LightComponent* GameApp::TryGetLight(Entity e) const {
    auto it = lights.find(e);
    return (it != lights.end()) ? &it->second : nullptr;
}

/*
 * @brief Try to retrieve a Collider component for an entity.
 *
 * @param e Entity ID to query.
 * @return Pointer to the Collider component, or nullptr if not found.
*/
const Collider* GameApp::TryGetCollider(Entity e) const {
    auto it = colliders.find(e);
    return (it != colliders.end()) ? it->second : nullptr;
}

/*
 * @brief Try to retrieve a MeshRenderer component for an entity.
 *
 * @param e Entity ID to query.
 * @return Pointer to the MeshRenderer component, or nullptr if not found.
*/
const MeshRenderer* GameApp::TryGetRenderer(Entity e) const {
    auto it = renderers.find(e);
    return (it != renderers.end()) ? it->second : nullptr;
}

/**
 * @brief Attempts to retrieve a SpriteAnimator component.
 */
const SpriteAnimator* GameApp::TryGetAnimator(Entity e) const {
    auto it = animators.find(e);
    return (it != animators.end()) ? &it->second : nullptr;
}

/**
 * @brief Attempts to retrieve a PlayerController component.
 */
const PlayerController* GameApp::TryGetController(Entity e) const {
    auto it = controllers.find(e);
    return (it != controllers.end()) ? &it->second : nullptr;
}

/**
 * @brief Attempts to retrieve an EnemiesController component.
 */
const EnemiesController* GameApp::TryGetEnemyController(Entity e) const {
    auto it = enemiesControllers.find(e);
    return (it != enemiesControllers.end()) ? &it->second : nullptr;
}

/**
 * @brief Attempts to retrieve a SpeedComponent (const).
 * @param e Entity to query.
 * @return Pointer to SpeedComponent or nullptr if not found.
 */
const SpeedComponent* GameApp::TryGetSpeed(Entity e) const
{
    auto it = temp_spdComponent.find(e);
    return (it != temp_spdComponent.end()) ? &it->second : nullptr;
}

/**
 * @brief Attempts to retrieve a MassComponent (const).
 * @param e Entity to query.
 * @return Pointer to MassComponent or nullptr if not found.
 */
const MassComponent* GameApp::TryGetMass(Entity e) const
{
    auto it = temp_massComponent.find(e);
    return (it != temp_massComponent.end()) ? &it->second : nullptr;
}

// =====================
// ================================================================================



// ========================== Entity lifetime / queries ===========================
// =====================

/*
 * @brief Destroy an entity and all of its components.
 *
 * Removes components from their respective arrays and marks the entity
 * as destroyed in the entity manager.
 *
 * @param e Entity to destroy.
 */
void GameApp::DestroyEntity(Entity e) {
    if (auto it = transforms.find(e); it != transforms.end()) {
        transformPool.Destroy(it->second);
        transforms.erase(it);
    }
    if (auto it = renderers.find(e); it != renderers.end()) {
        rendererPool.Destroy(it->second);
        renderers.erase(it);
    }

    if (auto it = colliders.find(e); it != colliders.end()) {
        colliderPool.Destroy(it->second);
        colliders.erase(it);
    }
    controllers.erase(e);
    animators.erase(e);
    persistentTags.erase(e);
    emitters.erase(e);
    lights.erase(e);
    enemiesControllers.erase(e);
    scriptComponents.erase(e);

    temp_spdComponent.erase(e);      
    temp_massComponent.erase(e);

    prefabTags.erase(e);            
    prefabDefaults.erase(e);
    puzzleObjects.erase(e);
    if (auto it = projectiles.find(e); it != projectiles.end()) {
        projectilePool.Destroy(it->second);
        projectiles.erase(it);
    }

    layerManager_.Remove(e);

    // Finally mark as destroyed in entityManager
    entityManager.DestroyEntity(e);
    /*entityManager.SetSignature(e, Signature());*/
}

/**
 * @brief Remove all components belonging to doomed entities
 *
 * Iterates through all entities in the provided set, erasing their components
 * from the ECS component maps and removing them from the entity manager.
 *
 * @param doomed Set of entities to remove.
 */
void GameApp::RemoveEntities(const std::unordered_set<Entity>& doomed) {
    for (Entity e : doomed)
    {
        if (auto it = transforms.find(e); it != transforms.end()) {
            transformPool.Destroy(it->second);
            transforms.erase(it);
        }
        if (auto it = renderers.find(e); it != renderers.end()) {
            rendererPool.Destroy(it->second);
            renderers.erase(it);
        }

        if (auto it = colliders.find(e); it != colliders.end()) {
            colliderPool.Destroy(it->second);
            colliders.erase(it);
        }
        controllers.erase(e);
        animators.erase(e);
        persistentTags.erase(e);
        emitters.erase(e);
        lights.erase(e);
        enemiesControllers.erase(e);
        scriptComponents.erase(e);

        temp_spdComponent.erase(e);      
        temp_massComponent.erase(e);

        prefabTags.erase(e);            
        prefabDefaults.erase(e);

        layerManager_.Remove(e);

        entityManager.DestroyEntity(e);
        puzzleObjects.erase(e);
        if (auto it = projectiles.find(e); it != projectiles.end()) {
            projectilePool.Destroy(it->second);
            projectiles.erase(it);
        }
        /*entityManager.SetSignature(e, Signature());*/

    }
}


/*
 * @brief Retrieve all active entities in the ECS.
 *
 * @return Constant reference to a vector of entity IDs.
 */
const std::vector<Entity>& GameApp::GetEntities() const {
    /*static std::vector<Entity> ids;
    ids.clear();
    ids.reserve(entityManager.GetEntities().size());

    for (const auto& obj : entityManager.GetEntities())
        ids.push_back(obj.id);

    return ids;*/

    entityIdCache.clear();
    entityIdCache.reserve(entityManager.GetEntities().size());

    for (const auto& obj : entityManager.GetEntities()) {
        entityIdCache.push_back(obj.id);
    }
    return entityIdCache;
}

/*
 * @brief Retrieve the ECS SystemManager reference.
 *
 * @return Constant reference to the SystemManager.
*/
const SystemManager& GameApp::GetSystemManager() const {
    return systemManager;
}

/**
 * @brief Retrieves all GameObject entries from the EntityManager.
 */
const std::vector<GameObject>& GameApp::GetAllEntities() const {
    return entityManager.GetEntities();
}

/**
 * @brief Finds an entity�s name string by its ID.
 * @param e Entity ID.
 * @return Name of the entity, or empty string if not found.
 */
std::string GameApp::GetEntityNameByEntity(Entity e) const {
    const auto& entities = entityManager.GetEntities();
    for (const auto& obj : entities) {
        if (obj.id == e)
            return obj.name;
    }
    return "";
}

/**
 * @brief Sets the name of an entity in the EntityManager.
 * @param e Entity ID.
 * @param name New name string.
 */
void GameApp::SetEntityName(Entity e, const std::string& name){
    entityManager.SetEntityName(e, name);
}
// =====================
// ================================================================================



// ====================================  Prefab  ==================================
// =====================
/*
 * @brief Instantiate a prefab at a given position.
 *
 * Loads prefab data from PrefabManager, calls function FactoryInstantiatePrefab to
 * creates a new entity, applies its transform, color, collider, and texture settings.
 *
 *
 * @param name Prefab identifier string.
 * @param pos World-space spawn position.
 * @return Entity ID of the created instance, or 0 on failure.
*/
Entity GameApp::InstantiatePrefab(const std::string& name, const Vector2& pos)
{
    DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[DEBUG] Instantiating prefab: " + name, "\n");

    const Prefab* p = prefabManager.Get(name);
    if (!p) {
        DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[ERROR] Prefab not found: " + name, "\n");
        return INVALID_ENTITY;
    }

    // Delegate work to the factory
    Entity e = FactoryInstantiatePrefab(*this, name, *p, pos);

    if (e != INVALID_ENTITY) {
        RegisterPrefabDefaults(e);
    }

    return e;
}

/**
 * @brief Tags an entity with its originating prefab name.
 * @param e Entity ID.
 * @param name Prefab name string.
 */
void GameApp::SetPrefabTag(Entity e, const std::string& name) {
    prefabTags[e] = name;
}
// =====================
// ================================================================================

/**
    * @brief Stores the initial color/rotation/scale values for an instance.
    *
    * Used to determine whether fields are overridden when applying prefab updates.
    *
    * @param e Entity whose defaults to store.
*/
void GameApp::RegisterPrefabDefaults(Entity e)
{
    PrefabInstanceDefaults defs{};

    if (Transform* t = GetTransform(e))
    {
        defs.scale = t->GetScale();
        defs.rotation = t->GetRotation();
    }

    if (MeshRenderer* mr = GetRenderer(e))
    {
        defs.color = mr->GetColor();
    }

    prefabDefaults[e] = defs;
}

/**
    * @brief Restores an instance’s properties to the saved prefab defaults.
    *
    * Applies default scale, rotation, and color unless the component is missing.
    *
    * @param e Entity to reset.
*/
void GameApp::RevertInstanceToPrefab(Entity e)
{
    auto it = prefabDefaults.find(e);
    if (it == prefabDefaults.end())
        return;

    const PrefabInstanceDefaults& defs = it->second;

    if (Transform* t = GetTransform(e))
    {
        t->SetScale(defs.scale);
        t->SetRotation(defs.rotation);
    }

    if (MeshRenderer* mr = GetRenderer(e))
    {
        mr->SetColor(defs.color);
    }
}

/**
    * @brief Uses a selected instance as the new default for its prefab,
    *        and propagates changes to all other non-overridden instances.
    *
    * Determines which fields are overridden per instance and only updates
    * values that match previous defaults.
    *
    * @param source Entity whose values define the new defaults.
*/
void GameApp::ApplyInstanceAsPrefab(Entity source)
{
    // Need to know which prefab this came from
    auto itTag = prefabTags.find(source);
    if (itTag == prefabTags.end())
        return;

    const std::string& prefabName = itTag->second;

    // Read current values from the source instance
    PrefabInstanceDefaults newDefs{};

    if (Transform* t = GetTransform(source))
    {
        newDefs.scale = t->GetScale();
        newDefs.rotation = t->GetRotation();
    }

    if (MeshRenderer* mr = GetRenderer(source))
    {
        newDefs.color = mr->GetColor();
    }

    std::string newTextureKey;      // empty = Texture untouched
    bool        newHasEmitter = false;
    EmitterDesc newEmitterDesc{};

    // Tex name from renderer
    if (MeshRenderer* mr = GetRenderer(source))
    {
        GLuint texID = mr->GetTexture();
        if (texID != 0)
        {
            // Convert GL id back to resource key
            newTextureKey = ResourceManager::GetTextureNameByID(texID);
        }
    }

    // Emitter settings from ParticleEmitter
    if (ParticleEmitter* em = GetEmitter(source))
    {
        newHasEmitter = true;

        // Start from old prefab emitter
        if (const Prefab* baseConst = prefabManager.Get(prefabName))
        {
            newEmitterDesc = baseConst->emitter;
        }

        // Copy fields that edit in the editor (mirror FactoryInstantiatePrefab)
        newEmitterDesc.enabled = em->enabled;
        newEmitterDesc.rate = em->rate;
        newEmitterDesc.particleLife = em->particleLife;
        newEmitterDesc.velMin = em->velMin;
        newEmitterDesc.velMax = em->velMax;
        newEmitterDesc.offset = em->offset;
        newEmitterDesc.sizeStart = em->sizeStart;
        newEmitterDesc.sizeEnd = em->sizeEnd;
        newEmitterDesc.colorStart = em->colorStart;
        newEmitterDesc.colorEnd = em->colorEnd;
    }

    // Also update the underlying Prefab blueprint so future instantiations use these defaults
    if (const Prefab* baseConst = prefabManager.Get(prefabName))
    {
        // PrefabManager stores Prefab objects so we just want to tweak its defaults.
        Prefab* base = const_cast<Prefab*>(baseConst);

        // Only update the fields we actually sampled
        base->scale = newDefs.scale;
        base->color = newDefs.color;

        if (!newTextureKey.empty())
        {
            base->texture = newTextureKey;
        }

        base->hasEmitter = newHasEmitter;
        if (newHasEmitter)
        {
            base->emitter = newEmitterDesc;
        }
    }

    const Prefab* p = prefabManager.Get(prefabName);

    // For every entity that uses this prefab tag...
    for (auto& [e, tag] : prefabTags)
    {
        if (tag != prefabName)
            continue;

        auto itDef = prefabDefaults.find(e);
        if (itDef == prefabDefaults.end())
            continue;

        PrefabInstanceDefaults& defs = itDef->second;

        Transform* t = GetTransform(e);
        MeshRenderer* mr = GetRenderer(e);
        ParticleEmitter* em = GetEmitter(e);

        // Check which fields were overridden BEFORE we change defs
        bool overrideScale = false;
        bool overrideRot = false;
        bool overrideColor = false;

        if (t)
        {
            overrideScale = !AlmostEqualVec2(t->GetScale(), defs.scale);
            overrideRot = !AlmostEqual(t->GetRotation(), defs.rotation);
        }

        if (mr)
        {
            overrideColor = !AlmostEqualVec3(mr->GetColor(), defs.color);
        }

        // Update stored defaults to new shared baseline
        defs = newDefs;

        // Now push new defaults into any instance that was NOT overriding
        if (t)
        {
            if (!overrideScale)
                t->SetScale(newDefs.scale);
            if (!overrideRot)
                t->SetRotation(newDefs.rotation);
        }

        if (mr)
        {
            if (!overrideColor)
            {
				mr->SetColor(newDefs.color);
            }

            // push prefab texture to all instances
            if (p && !p->texture.empty())
            {
                if (GLuint texID = ResourceManager::GetTexture(p->texture))
                {
                    mr->SetTexture(texID);
                }
            }
        }
        if (p)
        {
            if (p->hasEmitter)
            {
                // Ensure instance has an emitter
                if (!em)
                {
                    em = &AddEmitter(e);
                }

                if (em)
                {
                    em->enabled = p->emitter.enabled;
                    em->rate = p->emitter.rate;
                    em->particleLife = p->emitter.particleLife;
                    em->velMin = p->emitter.velMin;
                    em->velMax = p->emitter.velMax;
                    em->offset = p->emitter.offset;
                    em->sizeStart = p->emitter.sizeStart;
                    em->sizeEnd = p->emitter.sizeEnd;
                    em->colorStart = p->emitter.colorStart;
                    em->colorEnd = p->emitter.colorEnd;
                    em->timeAccumulator = 0.f;

                    // Mesh & texture
                    em->quad = GetMesh((size_t)p->emitter.meshIndex);
                }
            }
            else
            {
                // Prefab no longer has an emitter.
                // can REMOVE emitters from all instances here if add on a RemoveEmitter(e) helper.
            }
        }
    }

    DebugConsole::Get().Info("[Prefab] Applied instance " + std::to_string(source)
        + " as new defaults for prefab '" + prefabName + "'");
}

/**
    * @brief Writes the current prefab definitions to disk as prefabs.json.
*/
void GameApp::SavePrefabsToJson() const
{
    const std::string path = AssetPath("prefabs.json");
    if (!prefabManager.SavePrefabs(path))
    {
        std::cerr << "[GameApp] Failed to save prefabs to " << path << "\n";
    }
}

// =============================  Player spawn helpers  ===========================
// =====================
/**
 * @brief Sets the player�s spawn position in the world.
 * @param pos World-space position.
 */
void GameApp::SetPlayerSpawn(const Vector2& pos) {
    playerSpawn = pos;
    DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[GameApp] Player spawn set to (" + std::to_string(pos.x) + ", " + std::to_string(pos.y) + ")\n", "\n");
}

/**
 * @brief Retrieves the saved player spawn position.
 */
Vector2 GameApp::GetPlayerSpawn() const {
    return playerSpawn;
}

/**
 * @brief Returns the current position of the player entity.
 *
 * Searches all entities for one named "Player" and returns its Transform position.
 *
 * @return World-space position of the player, or (0,0) if not found.
 */
Vector2 GameApp::GetPlayerPos() const {
    for (auto e : GetEntities()) {
        if (GetEntityNameByEntity(e) == "Player") {
            if (const Transform* t = TryGetTransform(e)) {
                return t->GetPosition();
            }
        }
    }
    return Vector2{ 0,0 };
}

/**
    * @brief Finds the entity ID of the player based on cached ID or name search.
    *
    * @return Player entity or INVALID_ENTITY.
*/
Entity GameApp::FindPlayer() const {
    if (playerEntity != INVALID_ENTITY) return playerEntity;
    for (auto e : GetEntities()) {
        if (GetEntityNameByEntity(e) == "Player")
            return e;
    }
    return INVALID_ENTITY;
}
// =====================
// ================================================================================



// ================================  Texture helpers  =============================
// =====================
/**
 * @brief Returns a list of all texture resource names registered in the ResourceManager.
 * @return Reference to a temporary list of texture names.
 */
const std::vector<std::string>& GameApp::GetTextureList() const {
    static std::vector<std::string> names;
    names.clear();
    for (auto& [name, _] : ResourceManager::GetAllTextures())
        names.push_back(name);
    return names;
}

/**
 * @brief Applies a texture to a renderable entity by name.
 * @param e Entity ID.
 * @param name Texture resource name.
 */
void GameApp::ApplyTexture(Entity e, const std::string& name) {
    if (auto* mr = GetRenderer(e)) {
        mr->SetTexture(ResourceManager::GetTexture(name));
    }
}

/**
 * @brief Imports a texture from an absolute file path into the ResourceManager.
 * @param abs Absolute file path.
 * @param name Resource name to register the texture under.
 * @return true on success, false on failure.
 */
bool GameApp::ImportTexture(const std::string& abs, const std::string& name) {
    return ResourceManager::ImportTexture(abs, name);
}
// =====================
// ================================================================================

/**
    * @brief Builds and applies hierarchical transforms for all entities.
    *
    * Performs a DFS from parentless roots, calling ComposeFromParent() to
    * compute world transforms. The hierarchy is generated dynamically each
    * frame from parent pointers, avoiding persistent tree storage.
*/
void GameApp::ComposeHierarchy()
{
    if (isShuttingDown)
        return;
    // Build a child list per parent on the fly (avoids storing persistent graph)
    std::unordered_map<Entity, std::vector<Entity>> children;
    children.reserve(transforms.size());

    // Seed roots and child links
    std::vector<Entity> roots; roots.reserve(transforms.size());
    for (auto& [e, t] : transforms) {
        Entity p = t->GetParent();
        if (p == INVALID_ENTITY || !transforms.count(p))
            roots.push_back(e);
        else
            children[p].push_back(e);
    }

    // DFS compose, parents before children
    std::function<void(Entity)> dfs = [&](Entity e) {
        Transform* me = GetTransform(e);
        const Transform* parent = (me && me->GetParent() != INVALID_ENTITY) ? TryGetTransform(me->GetParent()) : nullptr;
        if (me) me->ComposeFromParent(parent);
        for (Entity c : children[e]) dfs(c);
        };

    for (Entity r : roots) dfs(r);
}

// ---------------------------------------------
// Script editor bridge (IComponentContext)
// ---------------------------------------------

/**
    * @brief Returns the list of script names shown in the editor dropdown.
*/
const std::vector<std::string>& GameApp::GetScriptNameList() const
{
    // Visible names in the editor combo box
  /*  static const std::vector<std::string> names = {
        "None",
        "Spin",
        "ChasePlayer"
    };
    return names;*/

    return scriptNameList;
}

/**
    * @brief Converts an entity’s ScriptComponent to a human-readable name.
    *
    * @param e Entity ID.
    * @return Script name ("None", "Spin", "ChasePlayer").
*/
std::string GameApp::GetEntityScriptName(Entity e) const
{
    auto it = scriptComponents.find(e);
    if (it == scriptComponents.end())
        return "None";

    const ScriptComponent& sc = it->second;

    if (sc.backend == ScriptBackend::NativeCpp)
    {
        return "None";
    }

    if (sc.backend == ScriptBackend::Lua)
    {
        if (sc.luaFile == "Scripts/include/spin.lua")  return "LuaSpin";
        if (sc.luaFile == "Scripts/include/chase.lua") return "LuaChasePlayer";
        if (sc.luaFile == "Scripts/include/enemy_basic.lua") return "LuaBasic";
        if (sc.luaFile == "Scripts/include/enemy_burrow.lua") return "LuaBurrow";
        if (sc.luaFile == "Scripts/include/enemy_heal.lua") return "LuaHeal";
        if (sc.luaFile == "Scripts/include/enemy_ranged.lua") return "LuaRanged";
        if (sc.luaFile == "Scripts/include/enemy_bless.lua") return "LuaBless";
        if (sc.luaFile == "Scripts/include/enemy_boss.lua") return "LuaBoss";
        if (sc.luaFile == "Scripts/include/miniboss_channeler.lua") return "LuaChanneler";
        return sc.luaFile.empty() ? "Lua" : sc.luaFile;
    }

    return "None";
}

/**
    * @brief Assigns a script to an entity or removes it if "None" is selected.
    *
    * Recreates a fresh script instance and marks it unstarted so the ScriptSystem
    * will call OnStart() next frame.
    *
    * @param e    Entity ID.
    * @param name Script name chosen in the editor.
*/
void GameApp::SetEntityScriptName(Entity e, const std::string& name)
{
    if (name == "None")
    {
        auto it = scriptComponents.find(e);
        if (it != scriptComponents.end())
        {
            if (it->second.backend == ScriptBackend::Lua)
                luaEngine.Unload(it->second);

            scriptComponents.erase(it);
        }
        return;
    }

    // ----- Lua choices -----
    if (name == "LuaSpin")
    {
        AddLuaScript(e, "Scripts/include/spin.lua");
        return;
    }
    if (name == "LuaChasePlayer")
    {
        AddLuaScript(e, "Scripts/include/chase.lua");
        return;
    }

    if (name == "LuaBasic")
    {
        AddLuaScript(e, "Scripts/include/enemy_basic.lua");
        return;
    }
    if (name == "LuaBurrow")
    {
        AddLuaScript(e, "Scripts/include/enemy_burrow.lua");
        return;
    }
    if (name == "LuaHeal")
    {
        AddLuaScript(e, "Scripts/include/enemy_heal.lua");
        return;
    }
    if (name == "LuaRanged")
    {
        AddLuaScript(e, "Scripts/include/enemy_ranged.lua");
        return;
    }
    if (name == "LuaBless")
    {
        AddLuaScript(e, "Scripts/include/enemy_bless.lua");
        return;
    }

    if (name == "LuaBoss")
    {
        AddLuaScript(e, "Scripts/include/enemy_boss.lua");
        return;
    }

    if (name == "LuaChanneler")
    {
        AddLuaScript(e, "Scripts/include/miniboss_channeler.lua");
        return;
    }


    ScriptKind kind = ScriptKind::None;

    if (kind == ScriptKind::None)
    {
        /*auto it = scriptComponents.find(e);
        if (it != scriptComponents.end())
            scriptComponents.erase(it);*/
        return;
    }
    //if (name == "None")
    //{
    //    auto it = scriptComponents.find(e);
    //    if (it != scriptComponents.end())
    //    {
    //        if (it->second.backend == ScriptBackend::Lua)
    //            luaEngine.Unload(it->second);

    //        scriptComponents.erase(it);
    //    }
    //    return;
    //}
    //if (name == "LuaChasePlayer")
    //{
    //    AddLuaScript(e, "Scripts/chase_player.lua");
    //    return;
    //}

    ScriptComponent& sc = scriptComponents[e];
    sc.backend = ScriptBackend::NativeCpp;
    sc.kind = kind;
    sc.luaFile.clear();
    sc.started = false;

    sc.native.reset();
}

/**
 * @brief Updates an entity's layer membership based on its components.
 * 
 * Moves the entity between World and Background layers depending on whether
 * it has active components (PlayerController, EnemyController, Projectile).
 * 
 * @param e Entity to update.
 */
void GameApp::RefreshLayerMembership(Entity e)
{
    Signature sig = entityManager.GetSignature(e);

    // - Background: walls/background objects
    // - World (live): player/enemies/projectiles
    const bool isLive =
        sig.test(PLAYERCONTROLLER) ||
        sig.test(ENEMYCONTROLLER) ||
        sig.test(PROJECTILE);

    layerManager_.Move(isLive ? eng::LayerId::World : eng::LayerId::Background, e);
}
