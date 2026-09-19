#pragma once
/**
 * @file    scriptSystem.h
 * @author  Jethro Sung
 * @email   sung.h
 * @date    2025-11-07
 *
 * @brief   Declares the ScriptSystem class for handling entity scripts.
 *
 * The ScriptSystem is part of the ECS architecture and is responsible for
 * updating all entities that have an associated script component. It handles
 * both initialization (`Start`) and per-frame updates (`Update`) of scripts
 * attached to entities.
 *
 * The system only performs updates when scripting is globally enabled within
 * the active component context. If disabled, it will skip all operations for
 * the frame.
 *
 * @version 1.0
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without prior
 * written consent of DigiPen Institute of Technology is prohibited.
 */
#include "Core/Systems/systemManager.h"
#include "Core/componentcontext.h"

 /*
  * @class ScriptSystem
  * @brief Executes initialization and per-frame updates for all script components.
  *
  * This system coordinates the script lifecycle for all entities that contain
  * a script component, invoking `ScriptStartIfNeeded()` to ensure proper setup
  * and `ScriptUpdate()` each frame to run game logic defined in user scripts.
  *
  * Typical use:
  * - Managed by SystemManager as part of the ECS pipeline.
  * - Called once per frame via `UpdateAll()`.
  * - Relies on the `IComponentContext` interface for accessing script data.
  */
class ScriptSystem : public ISystem {
    // Reference to the ECS component context providing access to script operations.
    IComponentContext& ctx;
public:
    /*
         * @brief Constructs a ScriptSystem bound to a component context.
         
         * @param context Reference to the active component context responsible for managing script components.
     */
    explicit ScriptSystem(IComponentContext& context) : ctx(context) {}

    /*
         * @brief Updates all scripted entities for the current frame.
         *
         * If scripting is disabled globally (`AreScriptsEnabled()` returns false),
         * the update step is skipped entirely. Otherwise, it:
         * 1. Retrieves all entities containing script components.
         * 2. Ensures each entity’s script has been started via `ScriptStartIfNeeded()`.
         * 3. Invokes `ScriptUpdate()` for each entity, passing the delta time.
         *
         * @param dt Delta time (in seconds) since the last frame.
     */
    void Update(float dt) override {
        if (!ctx.AreScriptsEnabled()) return;
        const auto entities = ctx.GetScriptedEntities();
        for (Entity e : entities) {
            ctx.ScriptStartIfNeeded(e);
            ctx.ScriptUpdate(e, dt);
        }
    }
};