--[[
* @file     enemy_ranged.lua
* @author   Lim Zhi Jie
* @email    zhijie.lim@digipen.edu
* @date     2026-03-11
*
* @brief Implements the Ranged enemy AI behavior (kite + 6-shot combo).
*
* This script defines the state machine and attack logic for the ranged
* enemy type. The enemy maintains a preferred distance from the player
* (sweet spot between MIN_RANGE_TILES and MAX_RANGE_TILES), retreating or
* advancing to stay within it. When in position, it executes a repeating
* 6-shot combo: shots 0–2 are single projectiles (NORMAL phase, 2.0s delay),
* and shots 3–5 are 3-projectile spread fans (SPREAD phase, 0.4s delay),
* followed by a 3.0s full-combo cooldown before repeating.
*
* Messages sent: "RangedNormalAttack", "RangedSpreadAttack"
* States: PATROL → CHASE → ATTACK → SEARCH → RETURN → PATROL
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the
* prior written consent of DigiPen Institute of Technology is prohibited.
*]]

-- =============================================================================
-- enemy_ranged.lua  —  Ranged enemy (kite + 6-shot combo)
--
-- Combo sequence matching C++ updateRanged exactly:
--   Shots 0-2  (NORMAL phase) : single projectile,  2.0s between shots
--   Shots 3-5  (SPREAD phase) : 3-projectile fan,   0.4s between bursts
--   After 6 shots: 3.0s full-combo cooldown, then repeat.
--
-- Messages sent (same strings as C++):
--   "RangedNormalAttack"  — fire one projectile at player
--   "RangedSpreadAttack"  — fire 3-projectile fan
-- =============================================================================

NORMAL_SHOT_DELAY = 2.0
SPREAD_SHOT_DELAY = 0.4
COMBO_COOLDOWN    = 3.0
MIN_RANGE_TILES   = 2.0
MAX_RANGE_TILES   = 4.0
RANGE_SPEED       = 80.0
PATROL_SPEED      = 96.0
CHASE_SPEED       = 150.0
SEARCH_SPEED      = 60.0
VISION_TILES      = 3.0

ST_PATROL = "PATROL"
ST_CHASE  = "CHASE"
ST_SEARCH = "SEARCH"
ST_RETURN = "RETURN"
ST_ATTACK = "ATTACK"

function normDt(dt)
    dt = tonumber(dt) or 0.0
    if dt > 1.0 then dt = dt * 0.001 end
    if dt > 0.05 then dt = 0.05 end
    return dt
end
function getTileSize()
    local ts = 100.0
    if type(AI_TileSize) == "function" then local t = tonumber(AI_TileSize()) or 0.0; if t > 0.0 then ts = t end end
    return ts
end
function isBlocked(wx, wy)
    if type(AI_IsBlocked) == "function" then return AI_IsBlocked(wx, wy) end; return false
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
    s_shotsFired=0      -- counter 0-5 within the current combo
    s_attackTimer=0.0   -- counts down to next shot / end of cooldown
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
        if ai_distToPlayer(entity,px,py)<=MAX_RANGE_TILES*ts then
            s_fsm=ST_ATTACK; s_attackTimer=0; s_shotsFired=0; return
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
        if ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) then s_fsm=ST_CHASE; return end
        ai_moveToSpawn(entity,dt); if s_atSpawn then s_fsm=ST_PATROL end

    elseif s_fsm==ST_ATTACK then
        local dx=pcx-ecx; local dy=pcy-ecy
        local dist=math.sqrt(dx*dx+dy*dy)
        local minR=MIN_RANGE_TILES*ts; local maxR=MAX_RANGE_TILES*ts

        -- Lose sight → search
        if not ai_hasLOS(ecx,ecy,px,py) then
            s_fsm=ST_SEARCH; s_searchTimer=0; s_searchPhase=0; s_searchDone=false; s_hasLastSeen=true; return
        end
        -- Escaped max range → chase
        if dist>maxR then s_fsm=ST_CHASE; return end

        -- Face player
        if math.abs(dx)>math.abs(dy) then s_dirX=dx>0 and 1 or -1; s_dirY=0
        else s_dirX=0; s_dirY=dy>0 and 1 or -1 end

        -- Reposition to stay in sweet spot  (mirrors C++ moveDir logic)
        if dist<minR then
            local ndx,ndy=ai_norm(-dx,-dy)
            if boxBlocked(ex+ndx*RANGE_SPEED*dt,ey+ndy*RANGE_SPEED*dt) then
                if not boxBlocked(ex+ndx*RANGE_SPEED*dt,ey) then safeMove(entity,ndx*RANGE_SPEED,0,dt)
                elseif not boxBlocked(ex,ey+ndy*RANGE_SPEED*dt) then safeMove(entity,0,ndy*RANGE_SPEED,dt) end
            else safeMove(entity,ndx*RANGE_SPEED,ndy*RANGE_SPEED,dt) end
        elseif dist>maxR then
            local ndx,ndy=ai_norm(dx,dy)
            if boxBlocked(ex+ndx*RANGE_SPEED*dt,ey+ndy*RANGE_SPEED*dt) then
                if not boxBlocked(ex+ndx*RANGE_SPEED*dt,ey) then safeMove(entity,ndx*RANGE_SPEED,0,dt)
                elseif not boxBlocked(ex,ey+ndy*RANGE_SPEED*dt) then safeMove(entity,0,ndy*RANGE_SPEED,dt) end
            else safeMove(entity,ndx*RANGE_SPEED,ndy*RANGE_SPEED,dt) end
        end

        -- Only fire in sweet spot
        if dist<minR or dist>maxR then return end

        -- Tick attack timer down
        s_attackTimer=s_attackTimer-dt
        if s_attackTimer>0 then return end

        -- Fire: shots 0-2 = normal, shots 3-5 = spread
        if s_shotsFired < 3 then
    local nx, ny = ai_norm(pcx - ecx, pcy - ecy)
        SpawnProjectile(entity, nx, ny)
        s_attackTimer = NORMAL_SHOT_DELAY
    else
        -- Spread: 3-projectile fan (center + ±25 degrees)
        local angle = math.atan(pcy - ecy, pcx - ecx)
        local spread = math.rad(25)
        for _, offset in ipairs({-spread, 0, spread}) do
            local a = angle + offset
            SpawnProjectile(entity, math.cos(a), math.sin(a))
        end
        s_attackTimer = SPREAD_SHOT_DELAY
    end

        s_shotsFired=s_shotsFired+1

        -- Full 6-shot combo → combo cooldown + reset
        if s_shotsFired>=6 then
            s_shotsFired=0
            s_attackTimer=COMBO_COOLDOWN
        end
    end
end