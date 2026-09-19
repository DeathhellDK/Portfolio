#pragma once
/**
 * @file      persistentTag.h
 * @author    Jethro Sung
 * @email     sung.h,
 * @date      2025-11-7
 *
 * @brief     Declares the PersistentTag component
 * 
 * PersistentTag marks an entity as "do not unload" when switching scenes /
 * loading levels. Typical use: player character, global managers, HUD, etc.
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */
#include "Core/component.h"

/**
 * @file persistentTag.h
 * @brief Defines a marker component for entities that persist across levels.
 *
 * Entities with this component are skipped during scene unloads.
 * Typically used for the Player.
 */
class PersistentTag : public Component {
public:
    explicit PersistentTag(Entity ownerId)
        : Component(ownerId) {
    }
};