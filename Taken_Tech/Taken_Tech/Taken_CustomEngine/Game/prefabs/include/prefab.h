#pragma once
/**
 * @file     prefab.h
 * @author   Jethro Sung
 * @co-author Woh kye Le
 * @email    sung.h, w.kyele
 * @date     2025-10-30
 *
 * @brief    Defines Prefab descriptors for game object instantiation.
 *
 * Prefabs describe the visual and physical defaults for entities created
 * by the loader or factory system (e.g., crates, doors, torches, enemies).
 *
 * They provide a clear separation between:
 *  - Asset appearance (texture / mesh / tint / scale)
 *  - Physics representation (collider shape + trigger behavior)
 *  - Optional particle emission effects
 *
 * @note Prefabs are data-only. They do not build components directly;
 *       instead, Loader / Factories convert these fields into actual ECS components.
 */
#include "Math/vect2.h"
#include "Math/vect3.h"
#include "Physics/collider.h"
#include <string>

 /**
     * @struct EmitterDesc
     * @brief Describes particle emission behavior for an entity.
     *
     * When 'enabled == true', the Prefab builder attaches a particle emitter
     * configured using these parameters:
     *
     * Properties:
     *  - 'rate'          Emission frequency (particles per second)
     *  - 'particleLife'  Lifetime of each spawned particle (seconds)
     *  - 'velMin/velMax' Randomized initial velocity range
     *  - 'offset'        Emitter origin offset from entity center
     *  - 'sizeStart/End' Particle size progression
     *  - 'colorStart/End' Color transition across lifetime
     *  - 'texture'       Optional override for particle appearance
     *  - 'meshIndex'     Which quad mesh index to use for rendering
     *
 */
struct EmitterDesc {
    bool   enabled = false;
    float  rate = 0.f;
    float  particleLife = 0.4f;
    Vector2 velMin{ -50.f,  80.f };
    Vector2 velMax{ 50.f, 120.f };
    Vector2 offset{ 0.f, 0.f };
    float  sizeStart = 10.f;
    float  sizeEnd = 2.f;
    Vector3 colorStart{ 1.f, 1.f, 1.f };
    Vector3 colorEnd{ 1.f, 1.f, 1.f };
    std::string texture;      // optional texture 
    int   meshIndex = 1;      // which quad mesh to use 
};

/**
    * @struct LightDesc
    * @brief Multi-channel light configuration used by prefabs.
    *
    * Provides three independent channels that can be enabled simultaneously:
    * - Glow: soft halo light with a power-based falloff (softness exponent).
    * - Source: radial source light with quadratic attenuation (1/(1+k*r^2)).
    * - Ember: particle-based embers; when enabled, a ParticleEmitter is configured.
    *
    * All offsets are specified relative to the entity's visual center so that
    * lights and particles can be aligned to features on the sprite (e.g., flame tip).
    */
struct LightDesc {
    bool   enabled = false;
    /**
        * @struct Glow
        * @brief Soft halo parameters for the glow channel.
        *
        * @param enabled    Toggles the glow channel.
        * @param radius     World-space radius of the glow.
        * @param intensity  Brightness multiplier applied to the glow.
        * @param opacity    Final alpha multiplier for the glow contribution.
        * @param color      RGB tint applied to the glow.
        * @param offset     Offset from the entity center used for placement.
        * @param softness   Exponent controlling falloff sharpness (higher = tighter).
        */
    struct Glow {
        bool    enabled = false;
        float   radius = 0.0f;
        float   intensity = 1.0f;
        float   opacity = 0.35f;
        Vector3 color{ 1.f, 1.f, 1.f };
        Vector2 offset{ 0.f, 0.f };
        float   softness = 2.0f;
    } glow;

    /**
        * @struct Source
        * @brief Radial source light parameters.
        *
        * @param enabled      Toggles the source channel.
        * @param radius       World-space radius of the source light.
        * @param intensity    Brightness multiplier for the source light.
        * @param opacity      Final alpha multiplier for the source contribution.
        * @param color        RGB tint applied to the source light.
        * @param offset       Offset from the entity center used for placement.
        * @param attenuation  Falloff coefficient k in 1/(1 + k*r^2).
        */
    struct Source {
        bool    enabled = false;
        float   radius = 0.0f;
        float   intensity = 1.0f;
        float   opacity = 0.35f;
        Vector3 color{ 1.f, 1.f, 1.f };
        Vector2 offset{ 0.f, 0.f };
        float   attenuation = 1.0f;
    } source;

    /**
        * @struct Ember
        * @brief Particle ember parameters mapped to a ParticleEmitter.
        *
        * @param enabled      Toggles ember emission.
        * @param rate         Particles per second spawned.
        * @param particleLife Lifetime of each ember in seconds.
        * @param velMin/velMax Randomized initial velocity range (x,y).
        * @param colorStart/End Start and end RGB colors across lifetime.
        * @param sizeStart/End Start and end particle sizes.
        * @param additive     If true, renders embers with additive blending.
        * @param offset       Emitter origin offset from the entity center.
        */
    struct Ember {
        bool    enabled = false;
        float   rate = 20.0f;
        float   particleLife = 0.8f;
        Vector2 velMin{ -20.f, 30.f };
        Vector2 velMax{ 20.f, 60.f };
        Vector3 colorStart{ 1.0f, 0.8f, 0.5f };
        Vector3 colorEnd{ 0.3f, 0.1f, 0.0f };
        float   sizeStart = 6.0f;
        float   sizeEnd = 1.0f;
        bool    additive = true;
        Vector2 offset{ 0.0f, 0.0f };
    } ember;

    /**
     * @struct Flicker
     * @brief Flicker parameters for light sources.
     *
     * @param enabled      Toggles light flickering.
     * @param intensityMin Minimum intensity multiplier.
     * @param intensityMax Maximum intensity multiplier.
     * @param speed        Flicker speed multiplier.
     */
    struct Flicker {
        bool    enabled = false;
        float   intensityMin = 0.8f;
        float   intensityMax = 1.0f;
        float   speed = 5.0f;
    } flicker;
};

/**
    * @struct Prefab
    * @brief High-level descriptor for creating an ECS entity.
    *
    * Prefabs define the default component state of the spawned object:
    *
    * - Rendering:
    *    - 'meshIndex' -> Which quad to render
    *    - 'texture'   -> Texture resource to bind
    *    - 'color'     -> Tint multiplier
    *    - 'scale'     -> Sprite scaling in world units
    *
    * - Physics:
    *    - 'collider'  Shape selector for collision system
    *    - 'isTrigger' If true, collider does not resolve physically
    *
    * - Effects:
    *    - 'hasEmitter' Enables particle emission
    *    - 'emitter'    Particle emission configuration
    *
    * @note Used by Factories and Loader to spawn gameplay objects.
    * @note JSON scene files override these defaults where specified.
*/
struct Prefab {
    int meshIndex = 0;
    Vector2 scale = { 1,1 };
    Vector3 color = { 1,1,1 };
    ColliderType collider = ColliderType::Box;
    bool isTrigger = false;
    std::string texture;
    bool hasEmitter = false;
    EmitterDesc emitter;
    bool hasLight = false;
    LightDesc light;
};
