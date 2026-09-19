#pragma once
/**
 * @file    particle.h
 * @author  Woh Kye Le 
 * @email   w.kyele
 * @date    2025-10-28
 *
 * @brief   Defines the Particle component representing a single particle
 * 
 * Each particle is a lightweight, short-lived component owned by an emitter
 * It stores its position, velocity, remaining life, color, and size range
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

class Particle : public Component {
public:
    float life = 0.f;                // Remaining lifetime (seconds)
    Vector2 velocity;                // Velocity per second
    Vector3 colorStart{ 1, 1, 1 };   // Starting color
    Vector3 colorEnd{ 1, 1, 1 };     // End color
    float sizeStart = 1.f;           // Initial size
    float sizeEnd = 0.f;             // Final size (when faded)
    float age = 0.f;                 // Time since spawn

    /**
     * @brief Constructs a new Particle component.
     * @param ownerId The entity ID that owns this particle.
     * @param life Initial lifetime of the particle in seconds.
     * @param vel Initial velocity of the particle.
     * @param cStart Starting color of the particle.
     * @param cEnd Ending color of the particle.
     * @param sStart Starting size of the particle.
     * @param sEnd Ending size of the particle.
     */
    Particle(Entity ownerId,
            float life,
            const Vector2& vel,
            const Vector3& cStart,
            const Vector3& cEnd,
            float sStart,
            float sEnd)
        : Component(ownerId),
        life(life),
        velocity(vel),
        colorStart(cStart),
        colorEnd(cEnd),
        sizeStart(sStart),
        sizeEnd(sEnd) {
    }
};
