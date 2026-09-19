#pragma once
/**
 * @file     scriptcomponent.h
 * @author   Jethro Sung
 * @email    sung.h
 * @date     2025-11-7
 * @brief    Holds a single script instance and execution metadata.
 *
 * Attached to an entity to enable ScriptSystem to:
 *  - Track script lifecycle (OnStart / OnUpdate)
 *  - Manage script polymorphism through std::unique_ptr<IScript>
 *  - Identify script behavior type for reloading or editor UI
 *
 * @note Only one script is currently supported per entity.
*/
#include <cstdint>
#include <memory>
#include <string>
#include "Scripting/scripts.h"

 // What runtime executes the script?
enum class ScriptBackend : uint8_t { None = 0, NativeCpp, Lua };

// Still useful for editor dropdown when using Native C++ scripts
enum class ScriptKind : uint8_t { None = 0 };

struct ScriptComponent
{
    // Which backend is active
    ScriptBackend backend = ScriptBackend::None;

    // ----------------------
    // Native C++ scripting
    // ----------------------
    std::unique_ptr<IScript> native;
    ScriptKind kind = ScriptKind::None;

    // ----------------------
    // Lua scripting
    // ----------------------
    std::string luaFile; // e.g. sin.lua
    int luaEnvRef = -1;    // registry ref to per entity env table
    int luaStartRef = -1;    // registry ref to OnStart optional
    int luaUpdateRef = -1;    // registry ref to OnUpdate req

    // Shared lifecycle
    bool started = false;
};