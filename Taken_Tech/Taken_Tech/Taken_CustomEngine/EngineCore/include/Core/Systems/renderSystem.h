#pragma once
/**
* @file     renderSystem.h
* @author   Jethro Sung
* @email     w.kyele, sung.h
* @co-author Woh Kye Le
* @date      2025-09-25
*
* @brief Header file for RenderSystem class, declares the RenderSystem class responsible for drawing
 *       entities that contain MeshRenderer, Transform, and
 *       optionally SpriteAnimator components.
 *
 * The RenderSystem queries component ownership, prepares animation
 * data if available, and issues draw calls through the Renderer.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "Graphics/meshrenderer.h"
#include "Core/transform.h"
#include "Graphics/renderer.h"
#include "Graphics/spriteanimator.h"
#include <vector>
#include "Core/Systems/systemManager.h"
#include "Core/componentcontext.h"

/**
 * @class RenderSystem
 * @brief Renders all entities with mesh and transform components,
 *        and optionally applies sprite sheet animations.
 *
 * This system iterates over MeshRenderer, Transform, and SpriteAnimator
 * components. For each renderable entity, it updates shader uniforms,
 * applies animation UVs (if present), and delegates draw calls to
 * the Renderer.
 */
class RenderSystem : public ISystem, public SystemBase {

    IComponentContext& context;  ///< Reference to ECS context (implemented by GameApp)

    public:
        explicit RenderSystem(IComponentContext& ctx);

        void Draw(Renderer& renderer) override;
};