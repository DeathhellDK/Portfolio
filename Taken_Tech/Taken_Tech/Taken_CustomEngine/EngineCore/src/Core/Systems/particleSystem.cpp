#pragma once
/**
 * @file    particleSystem.cpp
 * @author  Woh Kye Le
 * @email   w.kyele,t.weiliangterril
 * @co-author Tan Wei Liang Terril
 * @date    2025-10-28
 *
 * @brief   ParticleSystem that emits, simulates, and renders particles.
 *          Uses hardware instancing (mat3 per instance) when an instanced
 *          shader is provided; otherwise a non-instanced fallback exists.
 *
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include "Core/Systems/particleSystem.h"
#include "Particle/particleEmitter.h"
#include "Core/transform.h"
#include "Graphics/renderer.h"
#include "Math/matrix3x3.h"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include "Input/DebugConsole.hpp"


 /**
 * @brief Utility function that generates a random float between two values.
 *
 * @param rng Reference to the random number generator.
 * @param a   Lower bound.
 * @param b   Upper bound.
 * @return Random float in [a, b].
 */
static float frand(std::mt19937& rng, float a, float b) {
    std::uniform_real_distribution<float> d(a, b);
    return d(rng);
}


/**
* @brief Constructs the ParticleSystem and defines its ECS signature.
*
* Requires both Transform and ParticleEmitter components. Also ensures
* that a dynamic VBO is allocated for instanced rendering.
*
* @param context Reference to the ECS component context.
*/
ParticleSystem::ParticleSystem(IComponentContext& context)
    : ctx(context), rng(std::random_device{}())
{
    // Require entities to have a Transform and a ParticleEmitter
    Signature sig;
    sig.set(TRANSFORM);
    sig.set(PARTICLEEMITTER);
    SetSignature(sig);
    ensureInstanceVBO();
}


/**
 * @brief Spawns particles from a given emitter entity based on its emission rate.
 *
 * The emission rate (particles per second) is converted into a spawn interval,
 * and new particles are spawned until the accumulator falls below the interval.
 *
 * Each emitted particle inherits initial position, velocity, lifetime,
 * size, and color information from its parent emitter.
 *
 * @param owner Entity that owns the emitter.
 * @param dt    Delta time (seconds) since last frame.
 */
void ParticleSystem::emitFrom(Entity owner, float dt) {
    auto* em = ctx.GetEmitter(owner);
    auto* tr = ctx.GetTransform(owner);

    if (!em || !tr) return;

    if (!em->enabled) return;

    em->timeAccumulator += dt;
    const float interval = (em->rate > 0.f)
        ? (1.f / em->rate)
        : std::numeric_limits<float>::infinity();

    while (em->timeAccumulator >= interval) {
        em->timeAccumulator -= interval;

        Vector2 origin = tr->GetPosition() + em->offset;
        Vector2 vel{ frand(rng, em->velMin.x, em->velMax.x),
                     frand(rng, em->velMin.y, em->velMax.y) };

        Particle p;
        p.pos = { origin.x - em->sizeStart * 0.5f, origin.y }; // center horizontally
        p.vel = vel;
        p.life = em->particleLife;
        p.totalLife = em->particleLife;
        p.size0 = em->sizeStart;
        p.size1 = em->sizeEnd;
        p.col0 = em->colorStart;
        p.col1 = em->colorEnd;
        p.quad = em->quad;
        p.tex = em->texture;
        p.additive = em->additive;

        pool.emplace_back(p);
    }
}


/**
 * @brief Updates all active particles: emission, motion, and lifetime.
 *
 * 1. Emits new particles from all active emitters.
 * 2. Updates positions using simple Euler integration and applies gravity.
 * 3. Removes particles that have expired (life ≤ 0).
 *
 * @param dt Delta time (seconds) since last frame.
 */
void ParticleSystem::Update(float dt) {
    // Emit from all matching entities
    const auto& sigs = ctx.GetEntitySignatures();
    auto owners = ctx.GetSystemManager().GetMatchingEntities(sigs, GetSignature());
    for (Entity e : owners) emitFrom(e, dt);

    // Simulate pool
    for (auto& p : pool) {
        p.life -= dt;
        if (p.life > 0.f) {
            p.pos = p.pos + p.vel * dt;
            p.vel.y -= 120.f * dt;
        }
    }

    pool.erase(std::remove_if(pool.begin(), pool.end(),
        [](const Particle& x) { return x.life <= 0.f; }),
        pool.end());
}


/**
 * @brief Renders all active particles, batching by (mesh, texture) pair.
 *
 * When instanced rendering is enabled:
 * - Groups particles by shared mesh/texture.
 * - Builds one 3×3 transform matrix per instance.
 * - Issues one draw call per batch using glDrawElementsInstanced.
 *
 * If no instanced shader is available, falls back to per-particle draw.
 *
 * @param renderer Reference to the global Renderer.
 */
void ParticleSystem::Draw(Renderer& renderer)
{
    if (pool.empty())
        return;

    Shader* prevShader = renderer.GetShader();
    const float prevAlpha = renderer.GetGlobalAlpha();

    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLint prevSrc = 0;
    GLint prevDst = 0;
    glGetIntegerv(GL_BLEND_SRC_RGB, &prevSrc);
    glGetIntegerv(GL_BLEND_DST_RGB, &prevDst);

    if (!instancedShader) {
        DebugConsole::Get().Error("Instancing not active — using slow path!\n");

        glEnable(GL_BLEND);
        for (const auto& p : pool) {
            if (!p.quad) continue;

            if (p.additive) glBlendFunc(GL_ONE, GL_ONE);
            else glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            const float age01 = 1.f - (p.life / p.totalLife);
            const float size = p.size0 + (p.size1 - p.size0) * age01;

            Vector3 color{
                p.col0.x + (p.col1.x - p.col0.x) * age01,
                p.col0.y + (p.col1.y - p.col0.y) * age01,
                p.col0.z + (p.col1.z - p.col0.z) * age01
            };

            Transform tmp(INVALID_ENTITY, p.pos, { size, size }, 0.f);
            renderer.DrawMesh(*p.quad, tmp.GetModelMatrix(), color, p.tex);
        }

        renderer.SetGlobalAlpha(prevAlpha);
        renderer.SetShader(prevShader);
        glBlendFunc(prevSrc, prevDst);
        if (!blendEnabled) glDisable(GL_BLEND);
        return;
    }

    struct Key { Mesh2D* quad; GLuint tex; bool additive; };
    struct KeyHash {
        size_t operator()(const Key& k) const {
            size_t h = std::hash<void*>()(k.quad) ^ (std::hash<GLuint>()(k.tex) << 1);
            h ^= (std::hash<bool>()(k.additive) << 2);
            return h;
        }
    };
    struct KeyEq {
        bool operator()(const Key& a, const Key& b) const {
            return a.quad == b.quad && a.tex == b.tex && a.additive == b.additive;
        }
    };

    std::unordered_map<Key, std::vector<size_t>, KeyHash, KeyEq> buckets;
    buckets.reserve(8);

    for (size_t i = 0; i < pool.size(); ++i) {
        const auto& p = pool[i];
        if (!p.quad) continue;
        buckets[{ p.quad, p.tex, p.additive }].push_back(i);
    }

    renderer.SetShader(instancedShader);

    renderer.SetVPForInstanced();
    instancedShader->SetVec3("u_Tint", { 1.f, 1.f, 1.f });
    instancedShader->SetFloat("u_Opacity", 1.0f);

    glEnable(GL_BLEND);

    for (auto& [key, idxList] : buckets) {
        Mesh2D* quad = key.quad;
        GLuint tex = key.tex;

        if (key.additive) glBlendFunc(GL_ONE, GL_ONE);
        else glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        renderer.BindTexture(tex);

        instanceData.clear();
        instanceData.reserve(idxList.size() * 16);

        for (size_t i : idxList) {
            const auto& p = pool[i];

            const float age01 = 1.f - (p.life / p.totalLife);
            const float size = p.size0 + (p.size1 - p.size0) * age01;

            const float tx = p.pos.x;
            const float ty = p.pos.y;

            instanceData.push_back(size);
            instanceData.push_back(0.0f);
            instanceData.push_back(0.0f);
            instanceData.push_back(0.0f);
            instanceData.push_back(size);
            instanceData.push_back(0.0f);
            instanceData.push_back(tx);
            instanceData.push_back(ty);
            instanceData.push_back(1.0f);

            instanceData.push_back(p.col0.x);
            instanceData.push_back(p.col0.y);
            instanceData.push_back(p.col0.z);
            instanceData.push_back(p.col1.x);
            instanceData.push_back(p.col1.y);
            instanceData.push_back(p.col1.z);
            instanceData.push_back(age01);
        }

        glBindVertexArray(quad->GetVAO());
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(instanceData.size() * sizeof(float)),
            instanceData.data(),
            GL_STREAM_DRAW);

        const GLsizei stride = static_cast<GLsizei>(sizeof(float) * 16);
        const std::size_t offMat0 = sizeof(float) * 0;
        const std::size_t offMat1 = sizeof(float) * 3;
        const std::size_t offMat2 = sizeof(float) * 6;
        const std::size_t offCol0 = sizeof(float) * 9;
        const std::size_t offCol1 = sizeof(float) * 12;
        const std::size_t offAge = sizeof(float) * 15;

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(uintptr_t)offMat0);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)(uintptr_t)offMat1);
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, stride, (void*)(uintptr_t)offMat2);
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, stride, (void*)(uintptr_t)offCol0);
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, stride, (void*)(uintptr_t)offCol1);
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, stride, (void*)(uintptr_t)offAge);

        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);
        glVertexAttribDivisor(7, 1);
        glVertexAttribDivisor(8, 1);

        quad->DrawInstanced(*renderer.GetShader(), static_cast<GLsizei>(idxList.size()));

        glDisableVertexAttribArray(3);
        glDisableVertexAttribArray(4);
        glDisableVertexAttribArray(5);
        glDisableVertexAttribArray(6);
        glDisableVertexAttribArray(7);
        glDisableVertexAttribArray(8);
        glBindVertexArray(0);
    }

    renderer.BindTexture(0);
    renderer.SetGlobalAlpha(prevAlpha);
    renderer.SetShader(prevShader);
    glBlendFunc(prevSrc, prevDst);
    if (!blendEnabled) glDisable(GL_BLEND);
}

/**
 * @brief Allocates an OpenGL VBO for per-instance data if not already created.
 *
 * The buffer stores one mat3 (9 floats) , 2 vec3 (6 floats) per instance and is reused every frame.
 */
void ParticleSystem::ensureInstanceVBO() {
    if (instanceVBO) return;
    glGenBuffers(1, &instanceVBO);
}
