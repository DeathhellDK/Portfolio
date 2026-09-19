#pragma once
/**
* @file     debugDraw.h
* @author   Woh Kye Le
* @email    w.kyele
* @co-author
* @date     2025-10-01
*
* @brief Header file for DebugDraw namespace - Physics visualization and debugging utilities
* This namespace provides runtime debugging tools for visualizing collision boundaries
* and physics components during development and testing.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
#include "Graphics/renderer.h"

class GameApp; // forward declaration of GameApp

namespace DebugDraw {
    // draws AABB outlines for ColliderType::Box while PhysicsDebug is active
    void ColliderOutlines(GameApp& app, Renderer& renderer, int outlineMeshIndex = 4);
}