#pragma once
/**
 * @file    particleSystem.h
 * @author  Woh Kye Le
 * @email   w.kyele
 * @date    2025-10-28
 *
 * @brief    Defines the ParticleSystem which updates and renders all particles
 *
 *The system updates active particles, spawns new ones from emitters,
 * and fades/cleans them up when their lifetime ends
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include "Core/Systems/systemManager.h"
#include "Core/componentcontext.h"
#include "Math/vect2.h"
#include "Math/vect3.h"
#include <vector>
#include <random>

class Renderer;

/**
 * @class ParticleSystem
 * @brief Spawns, updates, and renders particles for all entities
 *        that have { TRANSFORM, PARTICLEEMITTER }.
 */
class ParticleSystem : public ISystem, public SystemBase {
public:
    /**
     * @brief Constructs the ParticleSystem with a component context.
     * @param ctx Reference to the ECS component context.
     */
    explicit ParticleSystem(IComponentContext& ctx);

    /**
     * @brief Sets the instanced shader used for rendering particles.
     * @param s Pointer to the shader instance.
     */
    void SetShader(Shader* s) { instancedShader = s; }

    /**
     * @brief Simulates and emits particles for the current frame.
     * @param dt Delta time in seconds.
     */
    void Update(float dt) override;

    /**
     * @brief Renders all active particles using instanced rendering.
     * @param renderer Renderer used to issue draw calls.
     */
    void Draw(Renderer& renderer) override;

private:
    IComponentContext& ctx;
    std::mt19937 rng;

    struct Particle {
        Vector2 pos{ 0.f, 0.f };
        Vector2 vel{ 0.f, 0.f };
        float   life = 0.f;             // remaining time 
        float   totalLife = 0.f;        // for interpolation
        float   size0 = 0.f;
        float   size1 = 0.f;
        Vector3 col0{ 1.f, 1.f, 1.f };
        Vector3 col1{ 1.f, 1.f, 1.f };
        Mesh2D* quad = nullptr;        
        GLuint  tex = 0;   
        bool additive = false;
    };

    std::vector<Particle> pool;

    // instancing state
    Shader* instancedShader = nullptr;
    GLuint  instanceVBO = 0;               // stores mat3 per-instance
    std::vector<float> instanceData;       // packed mat3s (9 floats per instance)

    void emitFrom(Entity owner, float dt);

    void ensureInstanceVBO();
};