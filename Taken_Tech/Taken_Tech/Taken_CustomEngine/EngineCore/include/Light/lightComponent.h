#pragma once
/**
 * @file      lightComponent.h
 * @author    Woh Kye Le
 * @email     w.kyele
 * @date      2026-03-02
 *
 * @brief     Declares LightComponent, a 2D lighting component that stores
 *            radius, intensity, opacity, tint color, and positional offset.
 *
 * LightComponent is consumed by LightSystem to render additive glows around
 * entities. It carries only data and does not issue rendering commands or
 * allocate GPU resources on its own.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2026 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#pragma once

#include "Core/component.h"
#include "Math/vect2.h"
#include "Math/vect3.h"

enum class LightType {
	Source, // radial falloff
	Glow,   // soft halo
	Ember   // particle embers (uses ParticleSystem)
};

struct GlowDesc {
	bool    enabled = true;
	float   radius = 180.0f;
	Vector3 color{ 1.0f, 0.85f, 0.55f };
	Vector2 offset{ 0.0f, 0.0f };
	float   intensity = 1.0f;
	float   opacity = 0.35f;
	float   softness = 2.0f;
};

struct SourceDesc {
	bool    enabled = false;
	float   radius = 180.0f;
	Vector3 color{ 1.0f, 0.85f, 0.55f };
	Vector2 offset{ 0.0f, 0.0f };
	float   intensity = 1.0f;
	float   opacity = 0.35f;
	float   attenuation = 1.0f;
};

struct EmberDesc {
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
};

struct FlickerDesc {
	bool    enabled = false;
	float   intensityMin = 0.8f;
	float   intensityMax = 1.0f;
	float   speed = 5.0f;
	float   time = 0.0f; // Internal tracker
};

class LightComponent : public Component {
public:
	bool enabled = true;
	GlowDesc glow;
	SourceDesc source;
	EmberDesc ember;
	FlickerDesc flicker;

		/**
		 * @brief Construct a LightComponent owned by an entity.
		 * @param ownerId The entity that owns this component.
		 */
	explicit LightComponent(Entity ownerId) : Component(ownerId) {}
};

