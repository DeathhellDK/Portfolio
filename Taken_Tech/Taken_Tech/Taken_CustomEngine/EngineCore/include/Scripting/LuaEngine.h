/**
 * @file      luaEngine.h
 * @author    Jethro Sung
 * @email     sung.h
 * @date      2026-01-30
 *
 * @brief     Declares LuaEngine, the scripting bridge that embeds Lua and
 *            exposes engine/gameplay functionality to ScriptComponent-driven
 *            entities.
 *
 * LuaEngine owns a Lua state (lua_State*) and provides helpers to:
 *  - Bind a C API into Lua (BindAPI) using an IComponentContext so scripts can
 *    query/mutate ECS data without directly depending on GameApp.
 *  - Load per-entity scripts into isolated environments (LoadForEntity).
 *  - Invoke script lifecycle callbacks (CallStart, CallUpdate).
 *  - Unload scripts and release Lua references (Unload).
 *
 * The l_* functions are C-callable Lua bindings registered into the Lua state.
 * They act as wrappers around engine operations (e.g., transform getters/setters,
 * finding player entity, and basic AI/grid queries and movement).
 *
 * A context pointer (IComponentContext*) is stored in the Lua registry to allow
 * static binding functions to access the ECS at runtime (GetCtx/SetCtx).
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */
#pragma once
#include <string>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include "Core/component.h"
#include "Core/componentcontext.h"
#include "Scripting/scriptcomponent.h"

class LuaEngine
{
public:
    /** @brief Constructs the LuaEngine and initializes the Lua state. */
    LuaEngine();
    /** @brief Destroys the LuaEngine and closes the Lua state. */
    ~LuaEngine();

    /**
     * @brief Binds C++ API functions to the Lua state.
     * @param ctx The ECS component context to expose to scripts.
     */
    void BindAPI(IComponentContext& ctx);

    /**
     * @brief Loads a script file for a specific entity.
     * @param ctx The ECS component context.
     * @param e The entity ID.
     * @param sc The ScriptComponent holding the script path.
     * @return True if the script was successfully loaded and initialized.
     */
    bool LoadForEntity(IComponentContext& ctx, Entity e, ScriptComponent& sc);
    
    /**
     * @brief Calls the Start() function defined in the entity's Lua script.
     * @param ctx The ECS component context.
     * @param e The entity ID.
     * @param sc The ScriptComponent holding the script references.
     */
    void CallStart(IComponentContext& ctx, Entity e, ScriptComponent& sc);
    
    /**
     * @brief Calls the Update(dt) function defined in the entity's Lua script.
     * @param ctx The ECS component context.
     * @param e The entity ID.
     * @param sc The ScriptComponent holding the script references.
     * @param dt Delta time in seconds.
     */
    void CallUpdate(IComponentContext& ctx, Entity e, ScriptComponent& sc, float dt);
    
    /**
     * @brief Unloads a script and releases its Lua references.
     * @param sc The ScriptComponent to clean up.
     */
    void Unload(ScriptComponent& sc);

private:
    lua_State* L = nullptr;

    static int l_GetPos(lua_State* L);
    static int l_SetPos(lua_State* L);
    static int l_GetRot(lua_State* L);
    static int l_SetRot(lua_State* L);
    static int l_FindPlayer(lua_State* L);
    static int l_AI_TileSize(lua_State* L);
    static int l_AI_IsBlocked(lua_State* L);
    static int l_AI_HasLOS(lua_State* L);
    static int l_AI_Move(lua_State* L);
    static int l_SpawnProjectile(lua_State* L);
    static int l_SetColliderEnabled(lua_State* L);
    static int l_SetAnimationRow(lua_State* L);
    static int l_AI_IsWall(lua_State* L);
    static int l_GetEnemyHp(lua_State* L);
    static int l_GetEnemyMaxHp(lua_State* L);
    static int l_HealEnemyHp(lua_State* L);
    static int l_SendMessage(lua_State* L);

    static IComponentContext* GetCtx(lua_State* L);
    static void SetCtx(lua_State* L, IComponentContext* ctx);

    bool CallRef(Entity e, int funcRef, int envRef, float dt, bool passDt);
};