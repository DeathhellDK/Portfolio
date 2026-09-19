--[[
* @file     enemy_heal.lua
* @author   Lim Zhi Jie
* @email    zhijie.lim@digipen.edu
* @date     2026-03-24
*
* @brief Implements the Heal enemy AI behavior (cowardly medic).
*
* This script defines the state machine and combat logic for the Heal enemy
* type. The enemy prioritizes its own survival above attacking, choosing its
* next action based on current HP ratio each time its cooldown expires.
*
* Action priority (evaluated when cooldown expires):
*   Tier 1 -- FLEE    hp < 25% maxHP  ->  back away from player at FLEE_SPEED
*                                         for FLEE_DUR seconds, then heal self
*                                         regardless of distance (no range req)
*   Tier 2 -- HEAL    hp < 50% maxHP  ->  stand still, channel heal, restore HP
*   Tier 3 -- ATTACK  healthy and in range  ->  stand still, melee strike player
*             (healthy and out of range     ->  drift closer at HEAL_MOV_SPEED)
*
* On cooldown (between actions): slow drift toward player at half move speed.
* While fleeing: move away from player at FLEE_SPEED; once FLEE_DUR elapses
* transition directly into a heal regardless of distance.
*
* Messages sent: "HealFlee", "HealSelfStart", "HealSelfDone", "EnemyAttack"
*
* States: PATROL -> CHASE -> ATTACK -> SEARCH -> RETURN -> PATROL
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the
* prior written consent of DigiPen Institute of Technology is prohibited.
*]]

HEAL_ATK_RANGE  = 0.8   -- tiles, melee attack range
HEAL_MOV_SPEED  = 60.0   -- approach speed (px/s)
FLEE_SPEED      = 140.0  -- retreat speed when critically low (px/s)
FLEE_DUR        = 1.8    -- seconds to flee before healing
ACTION_DUR      = 1.0    -- seconds for one heal or attack channel
COOLDOWN_DUR    = 2.0    -- seconds of cooldown after each action
FLEE_THRESHOLD  = 0.25   -- flee + heal below 25% HP  (= 7/30)
HEAL_THRESHOLD  = 0.50   -- heal-in-place below 50% HP (= 15/30)
HEAL_AMOUNT     = 8      -- HP restored per heal tick (scales with EnemiesController maxHp=30)
PATROL_SPEED    = 96.0
CHASE_SPEED     = 150.0
SEARCH_SPEED    = 60.0
VISION_TILES    = 3.0

ST_PATROL = "PATROL"
ST_CHASE  = "CHASE"
ST_SEARCH = "SEARCH"
ST_RETURN = "RETURN"
ST_ATTACK = "ATTACK"

function normDt(dt)
    dt=tonumber(dt) or 0.0; if dt>1.0 then dt=dt*0.001 end; if dt>0.05 then dt=0.05 end; return dt
end
function getTileSize()
    local ts=100.0; if type(AI_TileSize)=="function" then local t=tonumber(AI_TileSize()) or 0.0; if t>0.0 then ts=t end end; return ts
end
function isBlocked(wx,wy) if type(AI_IsBlocked)=="function" then return AI_IsBlocked(wx,wy) end; return false end
function boxBlocked(wx, wy)
    local ts = getTileSize(); local i = 2.0
    if isBlocked(wx+i,    wy+i)    then return true end
    if isBlocked(wx+ts-i, wy+i)    then return true end
    if isBlocked(wx+i,    wy+ts-i) then return true end
    if isBlocked(wx+ts-i, wy+ts-i) then return true end
    if isBlocked(wx,      wy+ts*0.5) then return true end
    if isBlocked(wx+ts,   wy+ts*0.5) then return true end
    return false
end
function safeMove(e, vx, vy, dt)
    local ex, ey = GetPos(e)
    ex = tonumber(ex) or 0.0; ey = tonumber(ey) or 0.0
    local nx = ex + vx * dt
    if not boxBlocked(nx, ey) then ex = nx end
    local ny = ey + vy * dt
    if not boxBlocked(ex, ny) then ey = ny end
    SetPos(e, ex, ey)
    return ex, ey
end
function ai_dist(ax,ay,bx,by) local dx=bx-ax; local dy=by-ay; return math.sqrt(dx*dx+dy*dy) end
function ai_norm(dx,dy) local l=math.sqrt(dx*dx+dy*dy); if l<0.001 then return 0,0 end; return dx/l,dy/l end
function ai_hasLOS(ax,ay,bx,by) if type(AI_HasLOS)=="function" then return AI_HasLOS(ax,ay,bx,by) end; return true end
function ai_playerInFront(ecx,ecy,px,py,dX,dY)
    local ts=getTileSize(); local dx=px-ecx; local dy=py-ecy; local mag=ai_dist(ecx,ecy,px,py)
    if mag<0.001 or mag>VISION_TILES*ts then return false end
    if (dx*dX+dy*dY)/mag<0.7071 then return false end; return ai_hasLOS(ecx,ecy,px,py)
end
function ai_distToPlayer(entity,px,py)
    local ts=getTileSize(); local ex,ey=GetPos(entity); return ai_dist(ex+ts*0.5,ey+ts*0.5,px+ts*0.25,py+ts*0.5)
end
function ai_updatePatrol(entity, dt)
    local ts = getTileSize()
    local ex, ey = GetPos(entity)
    local dX = s_dirX or 1
    local dY = s_dirY or 0
    local speed = PATROL_SPEED
    local nx = ex + dX * speed * dt
    local ny = ey + dY * speed * dt
    local sgx = math.floor((s_spawnX or ex) / ts)
    local sgy = math.floor((s_spawnY or ey) / ts)
    local pR  = s_patrolRange or 5
    local out = (dX > 0 and math.floor(nx / ts) > sgx + pR)
             or (dX < 0 and math.floor(nx / ts) < sgx - pR)
             or (dY > 0 and math.floor(ny / ts) > sgy + pR)
             or (dY < 0 and math.floor(ny / ts) < sgy - pR)
    if out then
        s_dirX = -dX; s_dirY = -dY
        return
    end
    local ax, ay = safeMove(entity, dX * speed, dY * speed, dt)
    local stuck = (dX ~= 0 and math.abs(ax - ex) < 0.5)
               or (dY ~= 0 and math.abs(ay - ey) < 0.5)
    if stuck then
        s_dirX = -dX; s_dirY = -dY
    end
end
function ai_updateChase(entity,dt,px,py)
    local ts=getTileSize(); local ex,ey=GetPos(entity); s_lastKnownPX=px; s_lastKnownPY=py
    local cx=ex+ts*0.5; local cy=ey+ts*0.5; local dx=(px+ts*0.5)-cx; local dy=(py+ts*0.5)-cy
    local dist=math.sqrt(dx*dx+dy*dy); if dist<1e-3 then return end
    local dX=dx/dist; local dY=dy/dist
    if math.abs(dx)>math.abs(dy) then s_dirX=dx>0 and 1 or -1; s_dirY=0 else s_dirX=0; s_dirY=dy>0 and 1 or -1 end
    local sx=dX*CHASE_SPEED*dt; local sy=dY*CHASE_SPEED*dt
    if boxBlocked(ex+sx,ey+sy) then
        if not boxBlocked(ex+sx,ey) then safeMove(entity,dX*CHASE_SPEED,0,dt)
        elseif not boxBlocked(ex,ey+sy) then safeMove(entity,0,dY*CHASE_SPEED,dt) end
    else safeMove(entity,dX*CHASE_SPEED,dY*CHASE_SPEED,dt) end
end
function ai_updateSearch(entity,dt)
    local ts=getTileSize(); local ex,ey=GetPos(entity)
    local geX=math.floor(ex/ts); local geY=math.floor(ey/ts)
    local gtX=math.floor((s_lastKnownPX or ex)/ts); local gtY=math.floor((s_lastKnownPY or ey)/ts)
    if s_hasLastSeen then
        if geX==gtX and geY==gtY then s_hasLastSeen=false; s_searchTimer=0; s_searchPhase=0; return end
        local ddx=gtX-geX; local ddy=gtY-geY; local nX=geX; local nY=geY
        if math.abs(ddx)>=math.abs(ddy) then nX=geX+(ddx>0 and 1 or -1) else nY=geY+(ddy>0 and 1 or -1) end
        if not isBlocked(nX*ts+ts*0.5,nY*ts+ts*0.5) then
            local nx2,ny2=ai_norm(nX*ts+ts*0.5-(ex+ts*0.5),nY*ts+ts*0.5-(ey+ts*0.5)); safeMove(entity,nx2*SEARCH_SPEED,ny2*SEARCH_SPEED,dt)
        else
            local aX=geX; local aY=geY
            if math.abs(ddx)>=math.abs(ddy) then aY=geY+(ddy>0 and 1 or -1) else aX=geX+(ddx>0 and 1 or -1) end
            if not isBlocked(aX*ts+ts*0.5,aY*ts+ts*0.5) then
                local nx2,ny2=ai_norm(aX*ts+ts*0.5-(ex+ts*0.5),aY*ts+ts*0.5-(ey+ts*0.5)); safeMove(entity,nx2*SEARCH_SPEED,ny2*SEARCH_SPEED,dt)
            else
                s_hasLastSeen=false;s_searchTimer=0;s_searchPhase=0
            end
        end
        return
    end
    s_searchTimer=(s_searchTimer or 0)+dt
    local dirs={{1,0},{0,1},{-1,0},{0,-1}}; local ph=s_searchPhase or 0
    if s_searchTimer>(ph+1)*0.5 then ph=ph+1; s_searchPhase=ph; if ph>=4 then s_searchDone=true end end
    if ph<4 then s_dirX=dirs[ph+1][1]; s_dirY=dirs[ph+1][2] end
end
function ai_moveToSpawn(entity,dt)
    local ts=getTileSize();local ex,ey=GetPos(entity)
    local ecx=ex+ts*0.5;local ecy=ey+ts*0.5
    local scx=(s_spawnX or ex)+ts*0.5;local scy=(s_spawnY or ey)+ts*0.5
    local dX=scx-ecx;local dY=scy-ecy;s_atSpawn=false
    if dX*dX+dY*dY<(ts*0.5)*(ts*0.5) then
        s_atSpawn=true;s_movingToTile=false
        s_dirX=1;s_dirY=0
        SetPos(entity,s_spawnX or ex,s_spawnY or ey);return
    end
    local dist=math.sqrt(dX*dX+dY*dY);local ndx=dX/dist;local ndy=dY/dist;local speed=PATROL_SPEED
    if math.abs(dX)>math.abs(dY) then s_dirX=dX>0 and 1 or -1;s_dirY=0 else s_dirX=0;s_dirY=dY>0 and 1 or -1 end
    local ax,ay=safeMove(entity,ndx*speed,ndy*speed,dt)
    local stuck=(math.abs(dX)>math.abs(dY) and math.abs(ax-ex)<0.5)
             or(math.abs(dY)>math.abs(dX) and math.abs(ay-ey)<0.5)
    if stuck then
        if math.abs(dX)>math.abs(dY) then safeMove(entity,0,ndy*speed,dt)
        else safeMove(entity,ndx*speed,0,dt) end
    end
end

-- ===========================================================================
-- OnStart
-- ===========================================================================
function OnStart(entity)
    local ts=getTileSize(); local ex,ey=GetPos(entity)
    s_fsm=ST_PATROL; s_dirX=1; s_dirY=0
    s_spawnX=ex; s_spawnY=ey; s_patrolRange=5
    s_lastKnownPX=ex; s_lastKnownPY=ey
    s_hasLastSeen=false; s_searchTimer=0; s_searchPhase=0; s_searchDone=false
    s_atSpawn=false; s_movingToTile=false
    s_isHealing=false
    s_isFleeing=false    -- true while backing away before a critical heal
    s_fleeTimer=0.0      -- counts up to FLEE_DUR
    s_actionTimer=0.0    -- > 0 means currently in heal/attack action
    s_cooldownTimer=0.0  -- counts down after each action
    s_tileSize=ts
end

-- ===========================================================================
-- OnUpdate
-- ===========================================================================
function OnUpdate(entity, dt)
    dt=normDt(dt)
    if s_fsm==nil then OnStart(entity) end

    local player=FindPlayer(); if player==nil then return end
    local px,py=GetPos(player); px=tonumber(px) or 0.0; py=tonumber(py) or 0.0
    local ex,ey=GetPos(entity); ex=tonumber(ex) or 0.0; ey=tonumber(ey) or 0.0
    local ts=s_tileSize or getTileSize()
    local ecx=ex+ts*0.5; local ecy=ey+ts*0.5
    local pcx=px+ts*0.25; local pcy=py+ts*0.5

    if s_fsm==ST_PATROL then
        local distToPlayer = ai_distToPlayer(entity,px,py)
        if ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) or (distToPlayer < VISION_TILES*ts and ai_hasLOS(ecx,ecy,px,py)) then
            s_fsm=ST_CHASE;return
        end
        ai_updatePatrol(entity,dt)

    elseif s_fsm==ST_CHASE then
        if not ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) and not ai_hasLOS(ecx,ecy,px,py) then
            s_fsm=ST_SEARCH; s_searchTimer=0; s_searchPhase=0; s_searchDone=false; s_hasLastSeen=true; return
        end
        if ai_distToPlayer(entity,px,py)<=HEAL_ATK_RANGE*ts then
            s_fsm=ST_ATTACK; s_isHealing=false; s_actionTimer=0; s_cooldownTimer=0; return
        end
        ai_updateChase(entity,dt,px,py)

    elseif s_fsm==ST_SEARCH then
        local distToPlayer = ai_distToPlayer(entity,px,py)
        if ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) or ai_hasLOS(ecx,ecy,px,py) or distToPlayer < VISION_TILES*ts then
            s_fsm=ST_CHASE;return
        end
        ai_updateSearch(entity,dt)
        if s_searchDone then s_searchDone=false; s_fsm=ST_RETURN; s_movingToTile=false; s_atSpawn=false end

    elseif s_fsm==ST_RETURN then
        if ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) and ai_hasLOS(ecx,ecy,px,py) then s_fsm=ST_CHASE; return end
        ai_moveToSpawn(entity,dt); if s_atSpawn then s_fsm=ST_PATROL end

    elseif s_fsm==ST_ATTACK then
        -- Sight/range checks — skip while fleeing or healing so those states
        -- are not cancelled the moment the enemy moves away from the player
        if not s_isFleeing and not s_isHealing then
            if not ai_hasLOS(ecx,ecy,px,py) then
                s_fsm=ST_SEARCH; s_searchTimer=0; s_searchPhase=0; s_searchDone=false; s_hasLastSeen=true; return
            end
            if ai_distToPlayer(entity,px,py)>HEAL_ATK_RANGE*ts then s_fsm=ST_CHASE; return end
        end

        local dx=pcx-ecx; local dy=pcy-ecy
        local dist=math.sqrt(dx*dx+dy*dy)

        -- Tick cooldown every frame
        if s_cooldownTimer>0 then s_cooldownTimer=s_cooldownTimer-dt end

        -- ===================================================================
        -- FLEE: critically low HP — back away from player for FLEE_DUR,
        --       then transition directly into a heal regardless of distance.
        -- ===================================================================
        if s_isFleeing then
            s_fleeTimer=s_fleeTimer+dt
            if s_fleeTimer>=FLEE_DUR then
                -- Done fleeing: start healing immediately (no range required)
                s_isFleeing=false; s_fleeTimer=0.0
                s_isHealing=true; s_actionTimer=dt
                if type(SendMessage)=="function" then SendMessage("HealSelfStart") end
                return
            end
            -- Move away from player (negate direction)
            if dist>0.001 then
                local ndx,ndy=ai_norm(dx,dy)
                -- Flee in the opposite direction
                local fx=-ndx; local fy=-ndy
                if boxBlocked(ex+fx*FLEE_SPEED*dt,ey+fy*FLEE_SPEED*dt) then
                    if not boxBlocked(ex+fx*FLEE_SPEED*dt,ey) then safeMove(entity,fx*FLEE_SPEED,0,dt)
                    elseif not boxBlocked(ex,ey+fy*FLEE_SPEED*dt) then safeMove(entity,0,fy*FLEE_SPEED,dt) end
                else safeMove(entity,fx*FLEE_SPEED,fy*FLEE_SPEED,dt) end
                -- Face away from player (mirrors retreat direction)
                if math.abs(dx)>math.abs(dy) then s_dirX=dx>0 and -1 or 1; s_dirY=0
                else s_dirX=0; s_dirY=dy>0 and -1 or 1 end
            end
            return
        end

        -- ===================================================================
        -- HEALING: stand still, channel heal, restore HP on completion
        -- ===================================================================
        if s_isHealing then
            s_actionTimer=s_actionTimer+dt
            if s_actionTimer>=ACTION_DUR then
                HealEnemyHp(entity, HEAL_AMOUNT)
                if type(SendMessage)=="function" then SendMessage("HealSelfDone") end
                s_isHealing=false; s_actionTimer=0.0; s_cooldownTimer=COOLDOWN_DUR
            end
            return  -- stand still
        end

        -- ===================================================================
        -- ATTACK ACTION: stand still, channel melee, reset on completion
        -- ===================================================================
        if s_actionTimer>0 then
            s_actionTimer=s_actionTimer+dt
            if s_actionTimer>=ACTION_DUR then
                if type(SendMessage)=="function" then SendMessage("EnemyAttack") end
                s_actionTimer=0.0; s_cooldownTimer=COOLDOWN_DUR
            end
            return  -- stand still
        end

        -- ===================================================================
        -- DECISION (cooldown expired): choose next action by priority
        -- ===================================================================
        if s_cooldownTimer<=0 then
            local cur_hp  = GetEnemyHp(entity)
            local max_hp  = GetEnemyMaxHp(entity)
            local hp_ratio = cur_hp / max_hp

            if hp_ratio<FLEE_THRESHOLD then
                -- TIER 1: critically low — flee then heal
                s_isFleeing=true; s_fleeTimer=0.0
                if type(SendMessage)=="function" then SendMessage("HealFlee") end

            elseif hp_ratio<HEAL_THRESHOLD then
                -- TIER 2: moderately hurt — heal in place
                s_isHealing=true; s_actionTimer=dt
                if type(SendMessage)=="function" then SendMessage("HealSelfStart") end

            elseif dist<=HEAL_ATK_RANGE*ts then
                -- TIER 3a: healthy and in range — attack
                s_actionTimer=dt
                -- (EnemyAttack message sent when action completes above)

            else
                -- TIER 3b: healthy but out of range — move closer
                local ndx,ndy=ai_norm(dx,dy)
                if boxBlocked(ex+ndx*HEAL_MOV_SPEED*dt,ey+ndy*HEAL_MOV_SPEED*dt) then
                    if not boxBlocked(ex+ndx*HEAL_MOV_SPEED*dt,ey) then safeMove(entity,ndx*HEAL_MOV_SPEED,0,dt)
                    elseif not boxBlocked(ex,ey+ndy*HEAL_MOV_SPEED*dt) then safeMove(entity,0,ndy*HEAL_MOV_SPEED,dt) end
                else safeMove(entity,ndx*HEAL_MOV_SPEED,ndy*HEAL_MOV_SPEED,dt) end
                if math.abs(dx)>math.abs(dy) then s_dirX=dx>0 and 1 or -1; s_dirY=0
                else s_dirX=0; s_dirY=dy>0 and 1 or -1 end
            end
        else
            -- === On cooldown: stand still if healthy, drift toward player if still hurt ===
            local cur_hp = GetEnemyHp(entity)
            local max_hp = GetEnemyMaxHp(entity)
            if cur_hp / max_hp < HEAL_THRESHOLD and dist>0.001 then
                -- Still hurt — drift slowly closer so next heal decision is in range
                local ndx,ndy=ai_norm(dx,dy); local slow=HEAL_MOV_SPEED*0.5
                if boxBlocked(ex+ndx*slow*dt,ey+ndy*slow*dt) then
                    if not boxBlocked(ex+ndx*slow*dt,ey) then safeMove(entity,ndx*slow,0,dt)
                    elseif not boxBlocked(ex,ey+ndy*slow*dt) then safeMove(entity,0,ndy*slow,dt) end
                else safeMove(entity,ndx*slow,ndy*slow,dt) end
                if math.abs(dx)>math.abs(dy) then s_dirX=dx>0 and 1 or -1; s_dirY=0
                else s_dirX=0; s_dirY=dy>0 and 1 or -1 end
            end
            -- If healthy, just stand still and wait for cooldown to expire
        end
    end
end