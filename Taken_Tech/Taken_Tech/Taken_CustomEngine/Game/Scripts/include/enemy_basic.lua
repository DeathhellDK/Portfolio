--[[
* @file     enemy_basic.lua
* @author   Lim Zhi Jie
* @email    zhijie.lim@digipen.edu
* @date     2026-03-11
*
* @brief Implements the Basic melee enemy AI behavior (dash attack).
*
* This script defines the state machine and movement logic for the basic
* melee enemy type. The enemy patrols a fixed range around its spawn point,
* chases the player on detection, and performs a high-speed dash attack when
* within range. After dashing, the enemy enters a rest state before it can
* dash again. Collision is handled via axis-separated movement using SetPos
* to bypass the physics engine's speed cap.
*
* States: PATROL → CHASE → ATTACK (READY / DASHING / RESTING)
*         CHASE → SEARCH → RETURN → PATROL
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the
* prior written consent of DigiPen Institute of Technology is prohibited.
*]]

-- =============================================================================
-- enemy_basic.lua  —  Basic melee enemy (dash attack)
-- Movement uses SetPos directly (same pattern as chase.lua) to avoid the
-- physics maxSpeed cap. dt is normalised the same way chase.lua does it.
-- =============================================================================

ATTACK_RANGE  = 3.5    -- tiles
DASH_SPEED    = 400.0
DASH_TILES    = 3
DASH_REST_DUR = 4.0
PATROL_SPEED  = 96.0
CHASE_SPEED   = 150.0
SEARCH_SPEED  = 60.0
CHECK_OFFSET  = 5.0
VISION_TILES  = 3.0

ST_PATROL = "PATROL"
ST_CHASE  = "CHASE"
ST_SEARCH = "SEARCH"
ST_RETURN = "RETURN"
ST_ATTACK = "ATTACK"

DASH_READY   = "READY"
DASH_DASHING = "DASHING"
DASH_RESTING = "RESTING"

-- ---------------------------------------------------------------------------
-- dt normalisation (matches chase.lua)
-- ---------------------------------------------------------------------------
function normDt(dt)
    dt = tonumber(dt) or 0.0
    if dt > 1.0 then dt = dt * 0.001 end
    if dt > 0.05 then dt = 0.05 end
    return dt
end

-- ---------------------------------------------------------------------------
-- Collision helpers (matches chase.lua safeBoxBlocked / safeIsBlocked)
-- ---------------------------------------------------------------------------
function getTileSize()
    local ts = 100.0
    if type(AI_TileSize) == "function" then
        local t = tonumber(AI_TileSize()) or 0.0
        if t > 0.0 then ts = t end
    end
    return ts
end

function isBlocked(wx, wy)
    if type(AI_IsBlocked) == "function" then return AI_IsBlocked(wx, wy) end
    return false
end

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

-- Axis-separated move using SetPos (same as chase.lua safeMove)
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

-- ---------------------------------------------------------------------------
-- Math helpers
-- ---------------------------------------------------------------------------
function ai_dist(ax,ay,bx,by)
    local dx=bx-ax; local dy=by-ay; return math.sqrt(dx*dx+dy*dy)
end
function ai_norm(dx,dy)
    local len=math.sqrt(dx*dx+dy*dy); if len<0.001 then return 0,0 end; return dx/len, dy/len
end

-- ---------------------------------------------------------------------------
-- Vision
-- ---------------------------------------------------------------------------
function ai_playerInFront(ecx,ecy,px,py,dirX,dirY)
    local ts = getTileSize()
    local dx=px-ecx; local dy=py-ecy
    local mag=ai_dist(ecx,ecy,px,py)
    if mag<0.001 or mag>VISION_TILES*ts then return false end
    if (dx*dirX+dy*dirY)/mag < 0.7071 then return false end
    if type(AI_HasLOS)=="function" then return AI_HasLOS(ecx,ecy,px,py) end
    return true
end

function ai_hasLOS(ecx,ecy,px,py)
    if type(AI_HasLOS)=="function" then return AI_HasLOS(ecx,ecy,px,py) end
    return true
end

function ai_distToPlayer(entity,px,py)
    local ts=getTileSize(); local ex,ey=GetPos(entity)
    return ai_dist(ex+ts*0.5, ey+ts*0.5, px+ts*0.25, py+ts*0.5)
end

-- ---------------------------------------------------------------------------
-- Patrol  (same wall/range logic, safeMove for movement)
-- ---------------------------------------------------------------------------
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

-- ---------------------------------------------------------------------------
-- Chase  (direct SetPos toward player, axis-separated collision)
-- ---------------------------------------------------------------------------
function ai_updateChase(entity, dt, px, py)
    local ts=getTileSize(); local ex,ey=GetPos(entity)
    s_lastKnownPX=px; s_lastKnownPY=py

    local cx=ex+ts*0.5; local cy=ey+ts*0.5
    local pcx=px+ts*0.5; local pcy=py+ts*0.5
    local dx=pcx-cx; local dy=pcy-cy
    local dist=math.sqrt(dx*dx+dy*dy)
    if dist < 1e-3 then return end

    local dirX=dx/dist; local dirY=dy/dist
    local speed=CHASE_SPEED
    local stepX=dirX*speed*dt; local stepY=dirY*speed*dt

    -- Update facing
    if math.abs(dx)>math.abs(dy) then s_dirX=dx>0 and 1 or -1; s_dirY=0
    else s_dirX=0; s_dirY=dy>0 and 1 or -1 end

    -- Axis-separated collision (same pattern as chase.lua)
    if boxBlocked(ex+stepX, ey+stepY) then
        if not boxBlocked(ex+stepX, ey) then
            safeMove(entity, dirX*speed, 0, dt)
        elseif not boxBlocked(ex, ey+stepY) then
            safeMove(entity, 0, dirY*speed, dt)
        end
    else
        safeMove(entity, dirX*speed, dirY*speed, dt)
    end
end

-- ---------------------------------------------------------------------------
-- Search
-- ---------------------------------------------------------------------------
function ai_updateSearch(entity, dt)
    local ts=getTileSize(); local ex,ey=GetPos(entity)
    local geX=math.floor(ex/ts); local geY=math.floor(ey/ts)
    local gtX=math.floor((s_lastKnownPX or ex)/ts)
    local gtY=math.floor((s_lastKnownPY or ey)/ts)

    if s_hasLastSeen then
        if geX==gtX and geY==gtY then
            s_hasLastSeen=false; s_searchTimer=0; s_searchPhase=0; return
        end
        local ddx=gtX-geX; local ddy=gtY-geY
        local nX=geX; local nY=geY
        if math.abs(ddx)>=math.abs(ddy) then nX=geX+(ddx>0 and 1 or -1)
        else nY=geY+(ddy>0 and 1 or -1) end
        local wx=nX*ts+ts*0.5; local wy=nY*ts+ts*0.5
        if not isBlocked(wx,wy) then
            local ndx,ndy=ai_norm(wx-(ex+ts*0.5), wy-(ey+ts*0.5))
            safeMove(entity, ndx*SEARCH_SPEED, ndy*SEARCH_SPEED, dt)
        else
            local aX=geX; local aY=geY
            if math.abs(ddx)>=math.abs(ddy) then aY=geY+(ddy>0 and 1 or -1)
            else aX=geX+(ddx>0 and 1 or -1) end
            local ax=aX*ts+ts*0.5; local ay=aY*ts+ts*0.5
            if not isBlocked(ax,ay) then
                local ndx,ndy=ai_norm(ax-(ex+ts*0.5), ay-(ey+ts*0.5))
                safeMove(entity, ndx*SEARCH_SPEED, ndy*SEARCH_SPEED, dt)
            else
                s_hasLastSeen=false;s_searchTimer=0;s_searchPhase=0
            end
        end
        return
    end

    -- 360 scan
    s_searchTimer=(s_searchTimer or 0)+dt
    local dirs={{1,0},{0,1},{-1,0},{0,-1}}
    local ph=s_searchPhase or 0
    if s_searchTimer>(ph+1)*0.5 then
        ph=ph+1; s_searchPhase=ph
        if ph>=4 then s_searchDone=true end
    end
    if ph<4 then s_dirX=dirs[ph+1][1]; s_dirY=dirs[ph+1][2] end
end

-- ---------------------------------------------------------------------------
-- Return to spawn
-- ---------------------------------------------------------------------------
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
    s_atSpawn=false; s_movingToTile=false; s_targetGX=0; s_targetGY=0
    s_dashPhase=DASH_READY; s_dashRemaining=0; s_dashRestTimer=0
    s_dashDirX=0; s_dashDirY=0; s_tileSize=ts
end

-- ===========================================================================
-- OnUpdate
-- ===========================================================================
function OnUpdate(entity, dt)
    dt = normDt(dt)
    if s_fsm==nil then OnStart(entity) end

    local player=FindPlayer(); if player==nil then return end
    local px,py=GetPos(player)
    px=tonumber(px) or 0.0; py=tonumber(py) or 0.0

    local ex,ey=GetPos(entity)
    ex=tonumber(ex) or 0.0; ey=tonumber(ey) or 0.0
    local ts=s_tileSize or getTileSize()
    local ecx=ex+ts*0.5; local ecy=ey+ts*0.5

    -- -----------------------------------------------------------------------
    if s_fsm==ST_PATROL then
        local distToPlayer = ai_distToPlayer(entity,px,py)
        if ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) or (distToPlayer < VISION_TILES*ts and ai_hasLOS(ecx,ecy,px,py)) then
            s_fsm=ST_CHASE;return
        end
        ai_updatePatrol(entity, dt)

    -- -----------------------------------------------------------------------
    elseif s_fsm==ST_CHASE then
        if not ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) and not ai_hasLOS(ecx,ecy,px,py) then
            s_fsm=ST_SEARCH; s_searchTimer=0; s_searchPhase=0
            s_searchDone=false; s_hasLastSeen=true; return
        end
        if ai_distToPlayer(entity,px,py) <= ATTACK_RANGE*ts then
            s_fsm=ST_ATTACK; s_dashPhase=DASH_READY; return
        end
        ai_updateChase(entity, dt, px, py)

    -- -----------------------------------------------------------------------
    elseif s_fsm==ST_SEARCH then
        local distToPlayer = ai_distToPlayer(entity,px,py)
        if ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) or ai_hasLOS(ecx,ecy,px,py) or distToPlayer < VISION_TILES*ts then
            s_fsm=ST_CHASE;return
        end
        ai_updateSearch(entity, dt)
        if s_searchDone then
            s_searchDone=false; s_fsm=ST_RETURN; s_movingToTile=false; s_atSpawn=false
        end

    -- -----------------------------------------------------------------------
    elseif s_fsm==ST_RETURN then
        if ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) then
            s_fsm=ST_CHASE; return
        end
        ai_moveToSpawn(entity, dt)
        if s_atSpawn then s_fsm=ST_PATROL end

    -- -----------------------------------------------------------------------
    elseif s_fsm==ST_ATTACK then
        if s_dashPhase==DASH_READY then
            if not ai_hasLOS(ecx,ecy,px,py) then
                s_fsm=ST_SEARCH; s_searchTimer=0; s_searchPhase=0
                s_searchDone=false; s_hasLastSeen=true; return
            end
            if ai_distToPlayer(entity,px,py) > ATTACK_RANGE*ts then
                s_fsm=ST_CHASE; return
            end
            -- Lock dash direction toward player and begin
            local pcx=px+ts*0.5; local pcy=py+ts*0.5
            local ddx,ddy=ai_norm(pcx-ecx, pcy-ecy)
            s_dashDirX=ddx; s_dashDirY=ddy
            s_dashRemaining=DASH_TILES*ts
            s_dashPhase=DASH_DASHING
            if math.abs(ddx)>math.abs(ddy) then s_dirX=ddx>0 and 1 or -1; s_dirY=0
            else s_dirX=0; s_dirY=ddy>0 and 1 or -1 end

        elseif s_dashPhase==DASH_DASHING then
            if s_dashRemaining<=0 then
                s_dashPhase=DASH_RESTING; s_dashRestTimer=0; return
            end
            local step=math.min(DASH_SPEED*dt, s_dashRemaining)
            -- Wall probe at leading edge
            local lx=ecx+s_dashDirX*(step+ts*0.5+2)
            local ly=ecy+s_dashDirY*(step+ts*0.5+2)
            if isBlocked(lx,ly) then
                s_dashPhase=DASH_RESTING; s_dashRestTimer=0; return
            end
            s_dashRemaining=s_dashRemaining-step
            -- Direct SetPos — bypasses physics maxSpeed cap entirely
            SetPos(entity, ex+s_dashDirX*step, ey+s_dashDirY*step)

        elseif s_dashPhase==DASH_RESTING then
            -- Frozen during rest
            s_dashRestTimer=s_dashRestTimer+dt
            if s_dashRestTimer>=DASH_REST_DUR then
                s_dashPhase=DASH_READY; s_dashRestTimer=0
            end
        end
    end
end