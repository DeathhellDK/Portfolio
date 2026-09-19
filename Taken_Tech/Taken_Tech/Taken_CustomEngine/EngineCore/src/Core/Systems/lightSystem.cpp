#pragma once
/**
 * @file      lightSystem.cpp
 * @author    Woh Kye Le
 * @email     w.kyele
 * @date      2026-03-02
 *
 * @brief     Implementation of LightSystem's additive glow rendering.
 *
 * The draw pass filters entities by { TRANSFORM, LIGHT } signature, then
 * for each owner:
 *  - Computes a temporary transform centered at the light's world position
 *  - Scales a unit-circle mesh to the requested radius
 *  - Sets global alpha based on opacity × intensity
 *  - Renders with additive blending to accumulate light
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2026 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#include "Core/Systems/lightSystem.h"

#include "Core/transform.h"
#include "Graphics/renderer.h"
#include "Graphics/shader.h"
#include "Graphics/vertex2d.h"
#include "Light/lightComponent.h"
#include "Particle/particleEmitter.h"

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>

#include <cmath>
#include <vector>

/**
 * @brief Construct a LightSystem and configure its signature.
 * @param ctx Component context providing ECS accessors and system manager.
 */
LightSystem::LightSystem(IComponentContext& ctx)
	: context(ctx)
{
	Signature sig;
	sig.set(TRANSFORM);
	sig.set(LIGHT);
	SetSignature(sig);

	BuildLightMesh();
}

/**
 * @brief Update light system logic 
 * @param dt Delta time (seconds).
 */
void LightSystem::Update(float dt)
{
	const auto& sigs = context.GetEntitySignatures();
	auto entities = context.GetSystemManager().GetMatchingEntities(sigs, GetSignature());

	for (Entity e : entities)
	{
		LightComponent* light = context.GetLight(e);
		if (!light || !light->enabled)
			continue;

		// 1. Handle flickering
		if (light->flicker.enabled) {
			light->flicker.time += dt * light->flicker.speed;
			// multiple sine graph for natural flickering
			float noise = std::sin(light->flicker.time) * 0.5f + 0.5f;
			noise += std::sin(light->flicker.time * 2.3f) * 0.2f;
			noise += std::sin(light->flicker.time * 0.7f) * 0.3f;
			noise /= 1.5f; // Normalize to [0, 1]

			float factor = light->flicker.intensityMin + (light->flicker.intensityMax - light->flicker.intensityMin) * noise;
			
			// Apply factor to intensity
			if (light->glow.enabled) light->glow.intensity = factor;
			if (light->source.enabled) light->source.intensity = factor;
		}

		// 2. Handle ember particles
		if (light->ember.enabled)
		{
			ParticleEmitter* em = context.GetEmitter(e);
			if (!em) {
				em = context.AddEmitterComponent(e);
			}

			if (em) {
				em->enabled = true;
				em->rate = light->ember.rate;
				em->particleLife = light->ember.particleLife;
				em->velMin = light->ember.velMin;
				em->velMax = light->ember.velMax;
				em->colorStart = light->ember.colorStart;
				em->colorEnd = light->ember.colorEnd;
				em->sizeStart = light->ember.sizeStart;
				em->sizeEnd = light->ember.sizeEnd;
				em->additive = light->ember.additive;
				em->offset = light->ember.offset;

				if (!em->quad) {
					em->quad = context.GetMesh(1);
				}
			}
		}
		else
		{
			if (ParticleEmitter* em = context.GetEmitter(e)) {
				em->enabled = false;
			}
		}
	}
}

/**
 * @brief Render additive glow circles for all entities with { TRANSFORM, LIGHT }.
 * @param renderer Renderer used for issuing draw calls.
 */
void LightSystem::Draw(Renderer& renderer)
{
	const auto& sigs = context.GetEntitySignatures();
	auto entities = context.GetSystemManager().GetMatchingEntities(sigs, GetSignature());

	const float prevAlpha = renderer.GetGlobalAlpha();
	Shader* prevShader = renderer.GetShader();

	// 1. set global darkening overlay to cover the whole viewport in screen space
	if (darkeningEnabled) {
		Camera2D* cam = renderer.getCamera();
		renderer.setCamera(nullptr);
		Shader* defShader = renderer.GetDefaultShader();
		renderer.SetShader(defShader);
		if (defShader) {
			defShader->Use();
			defShader->SetInt("u_Frame", 0);
			defShader->SetVec2("u_FrameSize", { 1.0f, 1.0f });
			defShader->SetInt("u_Cols", 1);
		}

		GLboolean blendEnabled = glIsEnabled(GL_BLEND);
		GLint prevSrc = 0, prevDst = 0;
		glGetIntegerv(GL_BLEND_SRC_RGB, &prevSrc);
		glGetIntegerv(GL_BLEND_DST_RGB, &prevDst);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		Matrix3x3 ndcMatrix = Matrix3x3::BuildTranslation(-1.0f, -1.0f) * Matrix3x3::BuildScaling(2.0f, 2.0f);
		renderer.SetGlobalAlpha(0.5f);
		renderer.DrawMesh(*context.GetMesh(0), ndcMatrix, Vector3{ 0.f, 0.f, 0.f }, 0);
		renderer.SetGlobalAlpha(1.0f);

		glBlendFunc(prevSrc, prevDst);
		if (!blendEnabled) glDisable(GL_BLEND);

		renderer.setCamera(cam);
	}

	// 2. Render additive lights
	if (lightShader && lightShader->GetID() != 0) {
		renderer.SetShader(lightShader);
		lightShader->Use();
		lightShader->SetInt("u_Frame", 0);
		lightShader->SetVec2("u_FrameSize", { 1.0f, 1.0f });
		lightShader->SetInt("u_Cols", 1);
	}

	GLboolean blendEnabled = glIsEnabled(GL_BLEND);
	GLint prevSrc = 0;
	GLint prevDst = 0;
	glGetIntegerv(GL_BLEND_SRC_RGB, &prevSrc);
	glGetIntegerv(GL_BLEND_DST_RGB, &prevDst);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	auto drawOne = [&](const Transform& tr, const Vector2& offset, float radius, const Vector3& color, float intensity, float opacity, float attenuation, float softness, int mode)
	{
		const Vector2 center = tr.GetPosition() + (tr.GetScale() * 0.5f) + offset;
		const float diameter = std::max(0.0f, radius * 2.0f);
		const Vector2 pos{ center.x - radius, center.y - radius };

		Transform tmp(INVALID_ENTITY, pos, Vector2{ diameter, diameter }, 0.0f);
		if (lightShader && lightShader->GetID() != 0) {
			lightShader->Use();
			lightShader->SetVec3("u_LightColor", color);
			lightShader->SetFloat("u_Intensity", intensity);
			lightShader->SetFloat("u_Opacity", opacity);
			lightShader->SetFloat("u_Attenuation", attenuation);
			lightShader->SetFloat("u_Softness", softness);
			lightShader->SetInt("u_Mode", mode);
		}
		renderer.SetGlobalAlpha(1.0f);
		renderer.DrawMesh(lightMesh, tmp.GetModelMatrix(), Vector3{1.f,1.f,1.f}, 0);
	};

	for (Entity e : entities)
	{
		Transform* tr = context.GetTransform(e);
		LightComponent* light = context.GetLight(e);
		if (!tr || !light || !light->enabled)
			continue;

		if (light->glow.enabled) {
			drawOne(
				*tr,
				light->glow.offset,
				light->glow.radius,
				light->glow.color,
				light->glow.intensity,
				light->glow.opacity,
				1.0f,
				light->glow.softness,
				0
			);
		}
		if (light->source.enabled) {
			drawOne(
				*tr,
				light->source.offset,
				light->source.radius,
				light->source.color,
				light->source.intensity,
				light->source.opacity,
				light->source.attenuation,
				2.0f,
				1
			);
		}
	}

	renderer.SetGlobalAlpha(prevAlpha);
	if (prevShader) {
		renderer.SetShader(prevShader);
	}

	glBlendFunc(prevSrc, prevDst);
	if (!blendEnabled)
		glDisable(GL_BLEND);
}

/**
 * @brief Build a unit-circle triangle fan mesh used by the light renderer.
 *
 * The mesh consists of a center vertex and a ring of vertices forming
 * a triangle fan. Texture coordinates are arranged to support radial
 * falloff in shaders if needed.
 */
void LightSystem::BuildLightMesh()
{
	constexpr int segments = 48;

	std::vector<Vertex2D> verts;
	std::vector<unsigned int> indices;
	verts.reserve(segments + 1);
	indices.reserve(segments * 3);

	const Vector3 white{ 1.0f, 1.0f, 1.0f };
	verts.emplace_back(Vector2(0.5f, 0.5f), Vector2(0.5f, 0.5f), white);

	const float cx = 0.5f;
	const float cy = 0.5f;
	const float r = 0.5f;
	const float twoPi = 6.2831853071795864769f;

	for (int i = 0; i < segments; ++i)
	{
		float t = static_cast<float>(i) / static_cast<float>(segments);
		float angle = t * twoPi;
		float x = cx + r * std::cos(angle);
		float y = cy + r * std::sin(angle);
		verts.emplace_back(Vector2(x, y), Vector2((x - cx) / (2.0f * r) + 0.5f, (y - cy) / (2.0f * r) + 0.5f), white);
	}

	for (int i = 0; i < segments; ++i)
	{
		int next = (i + 1) % segments;
		indices.push_back(0);
		indices.push_back(1 + i);
		indices.push_back(1 + next);
	}

	lightMesh.Reserve(verts.size(), indices.size());
	lightMesh.SetVertices(verts);
	lightMesh.SetIndices(indices);
}
