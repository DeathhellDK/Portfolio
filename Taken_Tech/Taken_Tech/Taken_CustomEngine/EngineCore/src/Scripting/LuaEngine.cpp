/**
 * @file      luaEngine.cpp
 * @author    Jethro Sung
 * @email     sung.h
 * @date      2026-01-30
 *
 * @brief     Implements LuaEngine, the engine-side Lua scripting bridge used
 *            to load, sandbox, and execute per-entity ScriptComponent logic.
 *
 * LuaEngine owns a Lua state (lua_State*) and exposes a small C API to Lua
 * scripts. The API is registered into the global Lua environment and routes
 * calls back into the engine via IComponentContext (stored in the Lua registry).
 *
 * Core responsibilities:
 *  - Context bridging:
 *      Stores an IComponentContext* in the Lua registry (kCtxPtrKey) so that
 *      static C functions (l_GetPos, l_SetPos, AI_* helpers, etc.) can access
 *      ECS/game state without depending on GameApp directly.
 *
 *  - API binding (BindAPI):
 *      Registers engine utility functions into Lua (Transform getters/setters,
 *      AI grid queries, movement, and player lookup).
 *
 *  - Per-entity script environments (LoadForEntity):
 *      Creates a fresh Lua table as the entity's _ENV, sets its metatable
 *      __index to _G so registered API functions remain visible, then loads
 *      and runs the Lua chunk under that environment.
 *      After execution, OnStart and OnUpdate function references are extracted
 *      and stored in the ScriptComponent as registry references.
 *
 *  - Script execution (CallStart / CallUpdate):
 *      Executes OnStart(entity) and OnUpdate(entity, dt) under the entity's
 *      environment using CallRef(). Any Lua errors are caught, the message is
 *      popped from the stack, and false is returned to the caller.
 *
 *  - Resource cleanup (Unload):
 *      Unreferences the per-entity environment and function refs stored in
 *      ScriptComponent to prevent registry leaks when scripts are changed or
 *      entities are destroyed.
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */
#include "Scripting/LuaEngine.h"
#include <iostream>
#include "Input/DebugConsole.hpp"

static const char* kCtxPtrKey = "__ctx_ptr";

LuaEngine::LuaEngine() { L = luaL_newstate(); luaL_openlibs(L); }
LuaEngine::~LuaEngine() { if (L) lua_close(L); }

void LuaEngine::SetCtx(lua_State* L, IComponentContext* ctx)
{
    lua_pushlightuserdata(L, ctx);
    lua_setfield(L, LUA_REGISTRYINDEX, kCtxPtrKey);
}

IComponentContext* LuaEngine::GetCtx(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, kCtxPtrKey);
    auto* ctx = static_cast<IComponentContext*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return ctx;
}

void LuaEngine::BindAPI(IComponentContext& ctx)
{
    SetCtx(L, &ctx);
    luaL_dostring(L, "package.path = package.path .. ';Scripts/include/?.lua'");

    lua_register(L, "GetPos", &LuaEngine::l_GetPos);
    lua_register(L, "SetPos", &LuaEngine::l_SetPos);
    lua_register(L, "GetRot", &LuaEngine::l_GetRot);
    lua_register(L, "SetRot", &LuaEngine::l_SetRot);
    lua_register(L, "AI_TileSize", &LuaEngine::l_AI_TileSize);
    lua_register(L, "AI_IsBlocked", &LuaEngine::l_AI_IsBlocked);
    lua_register(L, "AI_HasLOS", &LuaEngine::l_AI_HasLOS);
    lua_register(L, "AI_Move", &LuaEngine::l_AI_Move);
    lua_register(L, "FindPlayer", &LuaEngine::l_FindPlayer);
    lua_register(L, "SpawnProjectile", &LuaEngine::l_SpawnProjectile);
    lua_register(L, "SetColliderEnabled", &LuaEngine::l_SetColliderEnabled);
    lua_register(L, "SetAnimationRow", &LuaEngine::l_SetAnimationRow);
    lua_register(L, "AI_IsWall", &LuaEngine::l_AI_IsWall);
    lua_register(L, "GetEnemyHp", &LuaEngine::l_GetEnemyHp);
    lua_register(L, "GetEnemyMaxHp", &LuaEngine::l_GetEnemyMaxHp);
    lua_register(L, "HealEnemyHp", &LuaEngine::l_HealEnemyHp);
    lua_register(L, "SendMessage", &LuaEngine::l_SendMessage);
}

/**
 * @brief Lua binding: GetEntityPos(entityId) -> x, y
 * 
 * Retrieves the world position of the specified entity.
 * Usage in Lua: local x, y = GetPos(entityId)
 */
int LuaEngine::l_GetPos(lua_State* L)
{
    auto* ctx = GetCtx(L);
    Entity e = (Entity)luaL_checkinteger(L, 1);

    float x = 0.f, y = 0.f;
    if (ctx) ctx->TryGetWorldPos(e, x, y);

    lua_pushnumber(L, (lua_Number)x);
    lua_pushnumber(L, (lua_Number)y);
    return 2;
}

/**
 * @brief Lua binding: SetEntityPos(entityId, x, y)
 * 
 * Sets the world position of the specified entity.
 * Usage in Lua: SetPos(entityId, x, y)
 */
int LuaEngine::l_SetPos(lua_State* L)
{
    auto* ctx = GetCtx(L);
    Entity e = (Entity)luaL_checkinteger(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);

    if (ctx) ctx->SetWorldPos(e, x, y);
    return 0;
}

/**
 * @brief Lua binding: GetEntityRot(entityId) -> rotation
 * 
 * Retrieves the rotation of the specified entity.
 * Usage in Lua: local r = GetRot(entityId)
 */
int LuaEngine::l_GetRot(lua_State* L)
{
    auto* ctx = GetCtx(L);
    Entity e = (Entity)luaL_checkinteger(L, 1);

    float r = 0.f;
    if (ctx) ctx->TryGetRotation(e, r);

    lua_pushnumber(L, (lua_Number)r);
    return 1;
}

/**
 * @brief Lua binding: SetEntityRot(entityId, rotation)
 * 
 * Sets the rotation of the specified entity.
 * Usage in Lua: SetRot(entityId, r)
 */
int LuaEngine::l_SetRot(lua_State* L)
{
    auto* ctx = GetCtx(L);
    Entity e = (Entity)luaL_checkinteger(L, 1);
    float r = (float)luaL_checknumber(L, 2);

    if (ctx) ctx->SetRotation(e, r);
    return 0;
}

/**
 * @brief Lua binding: FindPlayer() -> entityId
 * 
 * Returns the entity ID of the player character.
 * Usage in Lua: local playerID = FindPlayer()
 */
int LuaEngine::l_FindPlayer(lua_State* L)
{
    auto* ctx = GetCtx(L);
    Entity p = ctx ? ctx->GetPlayerEntity() : INVALID_ENTITY;
    lua_pushinteger(L, (lua_Integer)p);
    return 1;
}

/**
 * @brief Lua binding: AI_TileSize() -> float
 * 
 * Returns the size of a single tile in the AI navigation grid.
 * Usage in Lua: local size = AI_TileSize()
 */
int LuaEngine::l_AI_TileSize(lua_State* L)
{
    IComponentContext* ctx = GetCtx(L);
    lua_pushnumber(L, ctx ? (lua_Number)ctx->GetTileSize() : (lua_Number)0.0);
    return 1;
}

/**
 * @brief Lua binding: AI_Move(entityId, vx, vy, dt)
 * 
 * Moves an entity using the AI movement logic (velocity-based).
 * Usage in Lua: AI_Move(entityId, vx, vy, dt)
 */
int LuaEngine::l_AI_Move(lua_State* L)
{
    IComponentContext* ctx = GetCtx(L);
    Entity e = (Entity)luaL_checkinteger(L, 1);
    float vx = (float)luaL_checknumber(L, 2);
    float vy = (float)luaL_checknumber(L, 3);
    float dt = (float)luaL_checknumber(L, 4);

    if (ctx) ctx->MoveEntityWorld(e, vx, vy, dt);
    return 0;
}


/**
 * @brief Lua binding: SpawnProjectile(shooterId, dirX, dirY)
 * 
 * Spawns a projectile from the shooter in the specified direction.
 * Usage in Lua: SpawnProjectile(shooterId, dx, dy)
 */
int LuaEngine::l_SpawnProjectile(lua_State* L)
{
    IComponentContext* ctx = GetCtx(L);
    Entity shooter = (Entity)luaL_checkinteger(L, 1);
    float dirX = (float)luaL_checknumber(L, 2);
    float dirY = (float)luaL_checknumber(L, 3);

    if (ctx) ctx->SpawnEnemyProjectile(shooter, dirX, dirY);
    return 0;
}

/**
 * @brief Lua binding: AI_HasLOS(x1, y1, x2, y2) -> bool
 * 
 * Checks for Line of Sight between two points in the world.
 * Usage in Lua: local visible = AI_HasLOS(x1, y1, x2, y2)
 */
int LuaEngine::l_AI_HasLOS(lua_State* L)
{
    IComponentContext* ctx = GetCtx(L);
    float ax = (float)luaL_checknumber(L, 1);
    float ay = (float)luaL_checknumber(L, 2);
    float bx = (float)luaL_checknumber(L, 3);
    float by = (float)luaL_checknumber(L, 4);

    bool los = false;
    if (ctx) {
        (void)ctx->GetController(ctx->GetPlayerEntity());
        los = ctx->HasLineOfSightWorld(ax, ay, bx, by, ctx->GetPlayerEntity());
    }

    lua_pushboolean(L, los ? 1 : 0);
    return 1;
}

/**
 * @brief Lua binding: AI_IsBlocked(x, y) -> bool
 * 
 * Checks if a specific world position is blocked by an obstacle.
 * Usage in Lua: local blocked = AI_IsBlocked(x, y)
 */
int LuaEngine::l_AI_IsBlocked(lua_State* L)
{
    IComponentContext* ctx = GetCtx(L);
    float wx = (float)luaL_checknumber(L, 1);
    float wy = (float)luaL_checknumber(L, 2);

    bool blocked = true;
    if (ctx) blocked = ctx->IsBlockedWorld(wx, wy);

    lua_pushboolean(L, blocked ? 1 : 0);
    return 1;
}

/**
 * @brief Helper to call a Lua function reference with arguments.
 * 
 * Sets up the Lua stack, pushes the entity ID and optional delta time,
 * and executes the function safely with error handling.
 * 
 * @param e Entity context for the call.
 * @param funcRef Registry reference to the Lua function.
 * @param envRef Registry reference to the Lua environment (table).
 * @param dt Delta time (optional).
 * @param passDt Whether to pass delta time to the Lua function.
 * @return true if the call succeeded, false on error.
 */
bool LuaEngine::CallRef(Entity e, int funcRef, int envRef, float dt, bool passDt)
{
    if (funcRef < 0 || envRef < 0) return false;

    lua_rawgeti(L, LUA_REGISTRYINDEX, funcRef); // func

    // If func to run under the env, set it as the first upvalue _ENV
    // funcs created with _ENV use upval; set it here:
    lua_rawgeti(L, LUA_REGISTRYINDEX, envRef);
    lua_setupvalue(L, -2, 1);

    // pass only the args Lua expects
    lua_pushinteger(L, (lua_Integer)e);
    if (passDt) lua_pushnumber(L, (lua_Number)dt);

    int nargs = passDt ? 2 : 1;

    // Push debug.traceback as error handler (msgh)
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_remove(L, -2); // remove 'debug' table, keep traceback func

    // Stack right now (top to bottom):
    int funcIndex = lua_gettop(L) - nargs - 1; // func is below args; traceback is on top
    lua_insert(L, funcIndex); // move traceback below func+args so becomes msgh

    int msgh = funcIndex;

    if (lua_pcall(L, nargs, 0, msgh) != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);

        DebugConsole::Get().AddFormattedMessage(
            LogLevel::Error,
            "[Lua][pcall] entity=", (int)e,
            " err=", (err ? err : "(unknown)")
        );

        lua_pop(L, 1);        // pop error string
        lua_remove(L, msgh);  // remove traceback func
        return false;
    }

    // remove traceback func on success
    lua_remove(L, msgh);
    return true;
}

bool LuaEngine::LoadForEntity(IComponentContext& /*ctx*/, Entity e, ScriptComponent& sc)
{
    (void)e;

    Unload(sc);
    if (sc.luaFile.empty()) return false;

    // Create per-entity env
    lua_newtable(L); // env

    int envIndex = lua_gettop(L);

    // env._G = _G
    lua_getglobal(L, "_G");
    lua_setfield(L, envIndex, "_G");

    // env metatable: __index = _G   GetRot/SetRot/etc visible
    lua_newtable(L);            // mt
    lua_getglobal(L, "_G");     // push _G
    lua_setfield(L, -2, "__index"); // mt.__index = _G
    lua_setmetatable(L, envIndex);  // setmetatable(env, mt)

    lua_getglobal(L, "require");
    lua_setfield(L, envIndex, "require");
    lua_getglobal(L, "package");
    lua_setfield(L, envIndex, "package");

    sc.luaEnvRef = luaL_ref(L, LUA_REGISTRYINDEX); // pops env

    // Load file
    if (luaL_loadfile(L, sc.luaFile.c_str()) != LUA_OK)
    {
       /* const char* err = lua_tostring(l, -1);
        if (err) debugconsole::get().error(std::string("[lua][loadfile] ") + sc.luafile + " : " + err + "\n");*/
        const char* err = lua_tostring(L, -1);
        DebugConsole::Get().AddFormattedMessage(
            LogLevel::Error,
            "[Lua][loadfile] entity=", (int)e,
            " file=", sc.luaFile,
            " err=", (err ? err : "(unknown)")
        );
        lua_pop(L, 1);
        return false;
    }

    // Set chunk _ENV first upval
    lua_rawgeti(L, LUA_REGISTRYINDEX, sc.luaEnvRef);
    lua_setupvalue(L, -2, 1);

    // Run chunk
    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);
        DebugConsole::Get().AddFormattedMessage(
            LogLevel::Error,
            "[Lua][chunk-run] entity=", (int)e,
            " file=", sc.luaFile,
            " err=", (err ? err : "(unknown)")
        );
        lua_pop(L, 1);
        return false;
    }

    // Pull OnStart/OnUpdate from env
    lua_rawgeti(L, LUA_REGISTRYINDEX, sc.luaEnvRef); // env on stack

    lua_getfield(L, -1, "OnStart");
    if (lua_isfunction(L, -1)) sc.luaStartRef = luaL_ref(L, LUA_REGISTRYINDEX);
    else { lua_pop(L, 1); sc.luaStartRef = -1; }

    lua_getfield(L, -1, "OnUpdate");
    if (lua_isfunction(L, -1)) sc.luaUpdateRef = luaL_ref(L, LUA_REGISTRYINDEX);
    else
    {
        DebugConsole::Get().AddFormattedMessage(
            LogLevel::Error,
            "[Lua][missing] entity=", (int)e,
            " file=", sc.luaFile,
            " missing or non-function: OnUpdate"
        );
        lua_pop(L, 1); // pop non-func
        lua_pop(L, 1); // pop env
        return false;
    }

    lua_pop(L, 1); // pop env
    return true;
}

void LuaEngine::CallStart(IComponentContext& ctx, Entity e, ScriptComponent& sc)
{
    SetCtx(L, &ctx);
    CallRef(e, sc.luaStartRef, sc.luaEnvRef, 0.0f, false);
}

void LuaEngine::CallUpdate(IComponentContext& ctx, Entity e, ScriptComponent& sc, float dt)
{
    SetCtx(L, &ctx);
    CallRef(e, sc.luaUpdateRef, sc.luaEnvRef, dt, true);
}

void LuaEngine::Unload(ScriptComponent& sc)
{
    if (!L) return;

    if (sc.luaEnvRef >= 0)    luaL_unref(L, LUA_REGISTRYINDEX, sc.luaEnvRef);
    if (sc.luaStartRef >= 0)  luaL_unref(L, LUA_REGISTRYINDEX, sc.luaStartRef);
    if (sc.luaUpdateRef >= 0) luaL_unref(L, LUA_REGISTRYINDEX, sc.luaUpdateRef);

    sc.luaEnvRef = sc.luaStartRef = sc.luaUpdateRef = -1;
}

int LuaEngine::l_SetColliderEnabled(lua_State* L)
{
    IComponentContext* ctx = GetCtx(L);
    Entity e = (Entity)luaL_checkinteger(L, 1);
    bool active = lua_toboolean(L, 2) != 0;
    if (ctx) ctx->SetColliderActive(e, active);
    return 0;
}

int LuaEngine::l_SetAnimationRow(lua_State* L)
{
    IComponentContext* ctx = GetCtx(L);
    Entity e = (Entity)luaL_checkinteger(L, 1);
    int row = (int)luaL_checkinteger(L, 2);
    if (ctx) ctx->SetAnimationRow(e, row);
    return 0;
}

int LuaEngine::l_AI_IsWall(lua_State* L)
{
    IComponentContext* ctx = GetCtx(L);
    float wx = (float)luaL_checknumber(L, 1);
    float wy = (float)luaL_checknumber(L, 2);
    bool blocked = true;
    if (ctx) blocked = ctx->IsWallWorld(wx, wy);
    lua_pushboolean(L, blocked ? 1 : 0);
    return 1;
}

int LuaEngine::l_GetEnemyHp(lua_State* L)
{
    IComponentContext* ctx = GetCtx(L);
    Entity e = (Entity)luaL_checkinteger(L, 1);
    int hp = ctx ? ctx->GetEnemyHp(e) : 0;
    lua_pushinteger(L, hp);
    return 1;
}

int LuaEngine::l_GetEnemyMaxHp(lua_State* L)
{
    IComponentContext* ctx = GetCtx(L);
    Entity e = (Entity)luaL_checkinteger(L, 1);
    int maxhp = ctx ? ctx->GetEnemyMaxHp(e) : 1;
    lua_pushinteger(L, maxhp);
    return 1;
}

int LuaEngine::l_HealEnemyHp(lua_State* L)
{
    IComponentContext* ctx = GetCtx(L);
    Entity e = (Entity)luaL_checkinteger(L, 1);
    int amount = (int)luaL_checkinteger(L, 2);
    if (ctx) ctx->HealEnemyHp(e, amount);
    return 0;
}

int LuaEngine::l_SendMessage(lua_State* L)
{
    IComponentContext* ctx = GetCtx(L);
    const char* msg = luaL_checkstring(L, 1);
    if (ctx && msg) ctx->BroadcastMessage(msg);
    return 0;
}