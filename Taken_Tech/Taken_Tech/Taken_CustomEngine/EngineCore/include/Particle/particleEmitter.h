#pragma once
/**
 * @file    particleEmitter.h
 * @author  Woh Kye Le
 * @email   w.kyele
 * @date    2025-10-28
 *
 * @brief   Defines the ParticleEmitter component for spawning particles
 *
 * A ParticleEmitter acts as a configuration blueprint for the ParticleSystem.
 * Each emitter can spawn particles at a set rate, with configurable speed,
 * lifetime, and color/size ranges.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include "Core/component.h"
#include "Math/vect2.h"
#include "Math/vect3.h"
#include "Graphics/mesh2d.h"

class ParticleEmitter : public Component {
public:
    bool enabled = false;            
    float rate = 50.f;               // Particles per second
    float particleLife = 0.6f;       // Lifetime of each particle (s)
    Vector2 offset = { 0.f, 0.f }; ; // Offset from owner' transformation
    Vector2 velMin = { -30.f, 50.f };
    Vector2 velMax = { 30.f, 90.f };
    Vector3 colorStart = { 1.f, 1.f, 1.f };
    Vector3 colorEnd = { 0.8f, 0.8f, 0.8f };
    float sizeStart = 10.f;
    float sizeEnd = 1.f;
    bool  additive = false;          // if true, render with additive blending

    Mesh2D* quad = nullptr;          // Reuse shared quad mesh
    GLuint texture = 0;              // Optional texture ID

    float timeAccumulator = 0.f;     // Tracks time between spawns

    /**
     * @brief Constructs a ParticleEmitter component.
     * @param owner The entity ID that owns this emitter.
     */
    explicit ParticleEmitter(Entity owner) : Component(owner) {}
};