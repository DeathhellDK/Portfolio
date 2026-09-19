#pragma once
/**
 * @file      lightSystem.h
 * @author    Woh Kye Le
 * @email     w.kyele
 * @date      2026-03-02
 *
 * @brief     Declares LightSystem, a draw-only ECS system that renders
 *            additive glow meshes for entities with { TRANSFORM, LIGHT }.
 *
 * LightSystem builds a reusable unit-circle mesh and, for each light owner,
 * scales and tints it according to LightComponent data. It uses additive
 * blending to accumulate light contributions over the scene.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2026 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#pragma once

#include "Core/Systems/systemManager.h"
#include "Core/componentcontext.h"
#include "Graphics/mesh2d.h"
#include "Graphics/shader.h"

/**
 * @class LightSystem
 * @brief Renders additive glow circles for entities that have a Transform and LightComponent.
 */
class LightSystem : public ISystem, public SystemBase {
public:
		/**
		 * @brief Construct the LightSystem with a component context.
		 * @param ctx Reference to the component context for ECS access.
		 */
		explicit LightSystem(IComponentContext& ctx);

		/**
		 * @brief Sets the shader used for rendering lights.
		 * @param s Pointer to the shader instance.
		 */
		void SetShader(Shader* s) { lightShader = s; }

		/**
		 * @brief Update light system logic 
		 * @param dt Delta time (seconds).
		 */
		void Update(float dt) override;

		/**
		 * @brief Draw additive glow meshes for all matching entities.
		 * @param renderer Renderer used to issue draw calls.
		 */
		void Draw(Renderer& renderer) override;

		/**
		 * @brief Toggles the global darkening overlay.
		 * @param shouldEnable True to enable darkening, false for normal brightness.
		 */
		void SetDarkeningEnabled(bool shouldEnable) { darkeningEnabled = shouldEnable; }

		/**
		 * @brief Returns whether the global darkening overlay is enabled.
		 * @return True if darkening is enabled.
		 */
		bool IsDarkeningEnabled() const { return darkeningEnabled; }

private:
	IComponentContext& context;
	Mesh2D lightMesh;
	Shader* lightShader = nullptr;
	bool darkeningEnabled = true;

		/**
		 * @brief Build a unit-circle mesh used for rendering glows.
		 *
		 * The mesh is centered at (0.5, 0.5) with radius 0.5 and is then
		 * scaled and translated per light instance.
		 */
		void BuildLightMesh();
};
