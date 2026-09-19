#pragma once
/**
 * @file      proximityPromptText.h
 * @author    Woh Kye Le
 * @email     w.kyele
 * @date      2026-04-03
 *
 * @brief     Declares UI proximity prompt specification and lookup helpers.
 *
 * @version   1.0
 * @copyright
 * Copyright (C) 2026 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include <cstdint>
#include <string>

#include "Core/component.h"
#include "puzzleObject.h"

class GameApp;

namespace UI
{
    struct ProximityPromptSpec
    {
        std::string text;
        std::string firstText;
        std::string repeatText;
        std::string font;
        float sizePx = 0.0f;
        float range = 0.0f;
        uint32_t rgba = 0;
    };

    struct ProximityPromptResult
    {
        bool show = false;
        std::string text;
        std::string font;
        float sizePx = 0.0f;
        uint32_t rgba = 0;
    };

    /**
     * @brief Returns proximity prompt specification for entities kind.
     * @param kind Entities classification used as a key.
     * @return ProximityPromptSpec loaded from json or empty if not configured.
     */
    ProximityPromptSpec GetPromptForEntitiesKind(PuzzleKind kind);

    /**
     * @brief Returns proximity prompt specification for the first ability absorption prompt.
     * @return ProximityPromptSpec loaded from json or empty if not configured.
     */
    ProximityPromptSpec GetPromptForFirstAbsorb();

    /**
     * @brief Returns active proximity prompt for the player based on current world state.
     * @param app The game application used to query entities and puzzle state.
     * @param player The player entity.
     * @return ProximityPromptResult containing text style when active.
     */
    ProximityPromptResult GetActivePromptForPlayer(GameApp& app, Entity player);

    /**
     * @brief Resets proximity prompt state when restarting gameplay without the intro cutscene.
     */
    void ResetPromptsForRetry();

    /**
     * @brief Resets proximity prompt state when starting a new run that includes the intro cutscene.
     */
    void ResetPromptsForCutsceneIntro();

    bool ConsumeEndCutsceneRequest();
}
