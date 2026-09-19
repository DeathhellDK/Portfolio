/**
* @file     renderSystem.cpp
* @author   Jethro Sung
* @email     w.kyele, sung.h
* @co-author Woh Kye Le
* @date      2025-09-25
*
* @brief Implementation of RenderSystem's draw pass
*
* @details
*   Converts O(n) owner lookups into O(1) via hash maps, then for each
*   MeshRenderer: finds Transform (required) and SpriteAnimator (optional),
*   uploads per-entity animation uniforms (or defaults), and delegates the draw call to the Renderer.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "Core/Systems/renderSystem.h"
#include "Core/engine.hpp"
#include <unordered_map>

RenderSystem::RenderSystem(IComponentContext& ctx)
    : context(ctx)
{
    // Define which components are required
    Signature sig;
    sig.set(TRANSFORM);
    sig.set(MESHRENDERER);
    SetSignature(sig);
}

/**
 * @brief Executes the RenderSystem's main draw pass.
 *
 * Iterates over all entities that match the system signature and:
 *   1. Performs camera-based view culling to skip off-screen entities.
 *   2. Prepares shader uniforms for transformation, texture, and animation.
 *   3. Delegates drawing to the Renderer.
 *
 * @param renderer Reference to the active Renderer used for issuing draw calls.
 */
void RenderSystem::Draw(Renderer& renderer)
{
    Shader* sh = renderer.GetShader();
    const auto& entities = context.GetEntitySignatures();
    const Signature& sysSig = GetSignature();
    const eng::Layering& layering = context.GetLayering();

    const float alpha = static_cast<float>(eng::interpolationAlpha());

    Camera2D* cam = renderer.getCamera();
    if (!cam)
        return;

    // --- Camera bounds (in world space) ---
    Vector2 camPos = cam->GetPosition();
    float zoom = cam->GetZoom();

    // widen culling area �3 so that far objects remain visible
    float halfW = (cam->viewportWidth * 1.5f) / zoom;
    float halfH = (cam->viewportHeight * 1.5f) / zoom;

    float camLeft = camPos.x - halfW;
    float camRight = camPos.x + halfW;
    float camBottom = camPos.y - halfH;
    float camTop = camPos.y + halfH;

    auto drawBucket = [&](eng::LayerId layerId)
        {
            const auto& bucket = context.GetEntitiesInLayer(layerId);

            for (Entity e : bucket)
            {
                auto it = entities.find(e);
                if (it == entities.end()) continue;

                const Signature& sig = it->second;
                if ((sig & sysSig) != sysSig)
                    continue;

                Transform* tr = context.GetTransform(e);
                MeshRenderer* mr = context.GetRenderer(e);
                SpriteAnimator* an = context.GetAnimator(e);
                if (!tr || !mr)
                    continue;

                Mesh2D* mesh = mr->GetMesh();
                if (!mesh)
                    continue;

                // Skip large batches (walls)
                bool skipCulling = (mesh->GetIndexCount() > 600);
                if (!skipCulling)
                {
                    const Vector2& pos = tr->GetPosition();
                    const Vector2& scale = tr->GetScale();

                    float left = pos.x;
                    float right = pos.x + scale.x;
                    float bottom = pos.y;
                    float top = pos.y + scale.y;

                    if (right < camLeft || left > camRight ||
                        top   < camBottom || bottom > camTop)
                        continue;
                }

                // --- Shader setup ---
                const Material& mat = mr->GetMaterial();
                GLuint texId = mat.GetTexture();
                Vector3 color = mat.GetColor();
                sh->Use();

                if (an && an->sheet)
                {
                    sh->SetInt("u_Frame", an->GetFrame());
                    sh->SetInt("u_Cols", an->GetCols());
                    sh->SetVec2("u_FrameSize", { an->FrameU(), an->FrameV() });
                    texId = an->sheet->texture.ID();
                }
                else
                {
                    sh->SetInt("u_Frame", 0);
                    sh->SetInt("u_Cols", 1);
                    sh->SetVec2("u_FrameSize", { 1.f, 1.f });
                }

                Matrix3x3 interpModel = tr->GetInterpolatedModel(alpha);

                renderer.DrawMesh(*mesh, tr->GetModelMatrix(), mr->GetColor(), texId);
            }
        };

    // One flag check per layer
    if (layering.RenderEnabled(eng::LayerId::Background))
        drawBucket(eng::LayerId::Background);

    if (layering.RenderEnabled(eng::LayerId::World))
        drawBucket(eng::LayerId::World);

    //for (auto& [e, sig] : entities)
    //{
    //    if ((sig & sysSig) != sysSig)
    //        continue;

    //    Transform* tr = context.GetTransform(e);
    //    MeshRenderer* mr = context.GetRenderer(e);
    //    SpriteAnimator* an = context.GetAnimator(e);
    //    if (!tr || !mr)
    //        continue;

    //    Mesh2D* mesh = mr->GetMesh();
    //    if (!mesh)
    //        continue;

    //    // Skip large batches (walls)
    //    bool skipCulling = (mesh->GetIndexCount() > 600);
    //    if (!skipCulling)
    //    {
    //        const Vector2& pos = tr->GetPosition();
    //        const Vector2& scale = tr->GetScale();

    //        float left = pos.x;
    //        float right = pos.x + scale.x;
    //        float bottom = pos.y;
    //        float top = pos.y + scale.y;

    //        if (right < camLeft || left > camRight ||
    //            top   < camBottom || bottom > camTop)
    //            continue;
    //    }

    //    // --- Shader setup ---
    //    const Material& mat = mr->GetMaterial();
    //    GLuint texId = mat.GetTexture();
    //    Vector3 color = mat.GetColor();
    //    sh->Use();

    //    if (an && an->sheet)
    //    {
    //        sh->SetInt("u_Frame", an->GetFrame());
    //        sh->SetInt("u_Cols", an->GetCols());
    //        sh->SetVec2("u_FrameSize", { an->FrameU(), an->FrameV() });
    //        texId = an->sheet->texture.ID();
    //    }
    //    else
    //    {
    //        sh->SetInt("u_Frame", 0);
    //        sh->SetInt("u_Cols", 1);
    //        sh->SetVec2("u_FrameSize", { 1.f, 1.f });
    //    }

    //    Matrix3x3 interpModel = tr->GetInterpolatedModel(alpha);

    //    renderer.DrawMesh(*mesh, tr->GetModelMatrix(), mr->GetColor(), texId);
    //}
}