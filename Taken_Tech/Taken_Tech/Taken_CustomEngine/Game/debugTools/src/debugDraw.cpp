/**
* @file     debugDraw.cpp
* @author   Woh Kye Le
* @email    w.kyele
* @co-author Jethro Sung, Tan Wei Liang Terril
* @email    sung.h,t.weiliangterril
* @date     2025-10-01
*
*@brief Implementation of DebugDraw namespace - Physics collision visualization system
*
* The DebugDraw namespace provides real-time physics visualization used to
* assist gameplay debugging and spatial alignment checks during development.
*
* Responsibilities:
* - Render Box Collider AABBs (non-rotated)
* - Integrate toggle control via PhysicsDebug (e.g., key 'P')
* - Use Renderer's mesh pipeline for outline drawing
* - Support ECS-driven entity/component management
*
* @version 1.0
* @copyright Copyright(C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "DebugDraw.h"
#include "Core/gameApp.h"                 // needs access to component stores & Matrix3x3
#include "Physics/PhysicsDebugger.h" // PhysicsDebug::IsDebugActive()
#include "Physics/collider.h"
#include "Core/transform.h"

/**
 * @brief Draws Box Collider AABB outlines for all active entities.
 *
 * @param app      Reference to the main GameApp for ECS component access.
 * @param renderer Active Renderer used to submit debug drawing calls.
 * @param outlineMeshIndex Index referencing the outline quad mesh (default 4).
 *
 *  1) Abort early if physics debugging is disabled
 *  2) Validate outline mesh index, fallback if unavailable
 *  3) Iterate through collider components in GameApp
 *  4) For each valid Transform+Collider pair:
 *      - Place outline mesh at the collider's world-space area
 *      - Scale to size of collider AABB
 *      - Draw in white, with line width temporarily thickened
 *
 */
void DebugDraw::ColliderOutlines(GameApp& app, Renderer& renderer, int outlineMeshIndex)
{
    if (!PhysicsDebug::IsDebugActive()) return;

    // safety on index
    if (outlineMeshIndex < 0 || static_cast<size_t>(outlineMeshIndex) >= app.meshes.size()) return;

    Mesh2D* outline = app.meshes[outlineMeshIndex].get(); // Outline rect

    if (outline->GetIndexCount() == 0)
    {
        DebugConsole::Get().Error("[DebugDraw] Outline mesh has zero indices.\n");
        return;
    }

    // thicker lines for visibility
    GLfloat prevWidth = 10.0f;
    glGetFloatv(GL_LINE_WIDTH, &prevWidth);
    glLineWidth(2.0f);

    const Vector3 outlineColor = { 1.f, 1.f, 1.f };

    const auto& entities = app.GetEntitySignatures();

    // --- Iterate through colliders ---
    const auto& allColliders = app.GetAllColliders();
    for (const auto& [entity, colPtr] : allColliders)
    {
        if (!colPtr) continue;               // safety
        const Collider& col = *colPtr;

        // Skip if entity no longer has a valid signature
        auto sigIt = entities.find(entity);
        if (sigIt == entities.end())
            continue;

        const Signature& sig = sigIt->second;

        // Skip if ent doesn't have both Transform and Collider
        if (!(sig.test(TRANSFORM) && sig.test(COLLIDER)))
            continue;
        // AABB debug outline (no rotation)
        if (col.type == ColliderType::Box)
        {
            Transform* tr = app.GetTransform(entity);
            
            Vector2 min = tr->GetPosition();        // top-left of collider in world space
            Vector2 size = col.size;                // width/height of collider in world units

            Matrix3x3 model =
                Matrix3x3::BuildTranslation(min.x, min.y) *
                Matrix3x3::BuildScaling(size.x, size.y);   // no rotation

            renderer.DrawMesh(*outline, model, outlineColor, 0);
        }
    }

    glLineWidth(prevWidth);

}


