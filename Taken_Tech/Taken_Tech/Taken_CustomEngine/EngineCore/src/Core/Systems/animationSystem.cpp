#include "Core/Systems/animationSystem.h"
#include "Graphics/spriteanimator.h"
/**
* @file     animationSystem.cpp
* @author   Woh Kye Le
* @email    w.kyele
* @co-author
* @email
* @date     2025-09-26
*
* @brief Implementation for frame stepping of SpriteAnimator.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

// declares a free function that is defined in spriteanimator.cpp
extern void SpriteAnimator_Update(SpriteAnimator& an, float dt);


AnimationSystem::AnimationSystem(IComponentContext& ctx) : context(ctx)
{
    // System cares only about SpriteAnimator components
    Signature sig;
    sig.set(SPRITEANIMATOR);
    SetSignature(sig);
}

void AnimationSystem::Update(float dt)
{
    const auto& entities = context.GetEntitySignatures();
    const Signature& sysSig = GetSignature(); //Access via getter

    for (auto it = entities.begin(); it != entities.end(); ++it)
    {
        Entity e = it->first;
        const Signature& sig = it->second;

        if ((sig & sysSig) != sysSig)
            continue;

        SpriteAnimator* an = context.GetAnimator(e);
        if (an)
            SpriteAnimator_Update(*an, dt);
    }
}
