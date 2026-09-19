--[[
* @file     miniboss_channeler.lua
* @brief    Miniboss: Channeler — Ranged mini-boss (3x aimed shot + 8-dir omni burst)
*
* Mirrors C++ updateMinibossChanneler exactly.
*
* Combo sequence:
*   Shots 0-2  (NORMAL phase) : single aimed projectile, 2.0s between shots
*   Shot  3    → CHARGING phase: freeze for 0.6s (wind-up VFX "BossOmniCharge")
*   FIRING phase: one projectile per frame across 8 directions (N/NE/E/SE/S/SW/W/NW)
*   After all 8 fired: 4.0s combo cooldown, then combo resets.
*
* State machine: PATROL → CHASE → ATTACK → SEARCH → RETURN → PATROL
* (identical to enemy_ranged.lua; only ST_ATTACK differs)
*
* Messages sent:
*   "RangedNormalAttack"    — single aimed shot (uses enemy facing direction)
*   "BossOmniCharge"        — burst wind-up started
*   "BossOmniShot_N/NE/E/SE/S/SW/W/NW" — one per burst direction
*
* Lua variable mapping (mirrors C++ member names in comments):
*   s_shotsFired   → channelerShotsFired
*   s_comboPhase   → channelerComboPhase  ("NORMAL" / "CHARGING" / "FIRING")
*   s_burstIndex   → channelerBurstIndex  (0-7 during FIRING)
*   s_attackTimer  → AttackTimer
*]]

-- =============================================================================
-- Constants
-- =============================================================================
NORMAL_SHOT_DELAY = 2.0      -- seconds between aimed shots
BURST_CHARGE_TIME = 0.6      -- freeze duration before burst fires
COMBO_COOLDOWN    = 4.0      -- full-combo cooldown after burst
MIN_RANGE_TILES   = 2.0      -- kite inner boundary (tiles)
MAX_RANGE_TILES   = 5.0      -- kite outer boundary (tiles)
KITE_SPEED        = 80.0
PATROL_SPEED      = 96.0
CHASE_SPEED       = 150.0
SEARCH_SPEED      = 60.0
VISION_TILES      = 3.0

-- Combo phase tokens
CP_NORMAL   = "NORMAL"
CP_CHARGING = "CHARGING"
CP_FIRING   = "FIRING"

-- FSM state tokens
ST_PATROL = "PATROL"
ST_CHASE  = "CHASE"
ST_SEARCH = "SEARCH"
ST_RETURN = "RETURN"
ST_ATTACK = "ATTACK"

-- 8-direction burst: unit vectors + message strings
-- Diagonals pre-normalised (1/sqrt(2) ≈ 0.7071)
-- Lua: BURST_DIRS[i] = { dx, dy, msg }
BURST_DIRS = {
    {  0.0,    1.0,   "BossOmniShot_N"  },
    {  0.7071, 0.7071,"BossOmniShot_NE" },
    {  1.0,    0.0,   "BossOmniShot_E"  },
    {  0.7071,-0.7071,"BossOmniShot_SE" },
    {  0.0,   -1.0,   "BossOmniShot_S"  },
    { -0.7071,-0.7071,"BossOmniShot_SW" },
    { -1.0,    0.0,   "BossOmniShot_W"  },
    { -0.7071, 0.7071,"BossOmniShot_NW" },
}

-- =============================================================================
-- Shared helpers  (identical to enemy_ranged.lua)
-- =============================================================================
function normDt(dt)
    dt = tonumber(dt) or 0.0
    if dt > 1.0 then dt = dt * 0.001 end
    if dt > 0.05 then dt = 0.05 end
    return dt
end
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
    return isBlocked(wx+i,wy+i) or isBlocked(wx+ts-i,wy+i)
        or isBlocked(wx+i,wy+ts-i) or isBlocked(wx+ts-i,wy+ts-i)
end
function safeMove(e, vx, vy, dt)
    local ex, ey = GetPos(e); ex = tonumber(ex) or 0.0; ey = tonumber(ey) or 0.0
    local nx = ex+vx*dt; if not boxBlocked(nx, ey) then ex = nx end
    local ny = ey+vy*dt; if not boxBlocked(ex, ny) then ey = ny end
    SetPos(e, ex, ey)
end
function ai_dist(ax,ay,bx,by)
    local dx=bx-ax; local dy=by-ay; return math.sqrt(dx*dx+dy*dy)
end
function ai_norm(dx,dy)
    local l=math.sqrt(dx*dx+dy*dy); if l<0.001 then return 0,0 end; return dx/l,dy/l
end
function ai_hasLOS(ax,ay,bx,by)
    if type(AI_HasLOS)=="function" then return AI_HasLOS(ax,ay,bx,by) end; return true
end
function ai_playerInFront(ecx,ecy,px,py,dX,dY)
    local ts=getTileSize(); local dx=px-ecx; local dy=py-ecy
    local mag=ai_dist(ecx,ecy,px,py)
    if mag<0.001 or mag>VISION_TILES*ts then return false end
    if (dx*dX+dy*dY)/mag<0.7071 then return false end
    return ai_hasLOS(ecx,ecy,px,py)
end
function ai_distToPlayer(entity,px,py)
    local ts=getTileSize(); local ex,ey=GetPos(entity)
    return ai_dist(ex+ts*0.5,ey+ts*0.5,px+ts*0.25,py+ts*0.5)
end
function ai_updatePatrol(entity,dt)
    local ts=getTileSize(); local ex,ey=GetPos(entity)
    local dX=s_dirX or 1; local dY=s_dirY or 0
    local nx=ex+dX*PATROL_SPEED*dt; local ny=ey+dY*PATROL_SPEED*dt
    local sgx=math.floor((s_spawnX or ex)/ts); local sgy=math.floor((s_spawnY or ey)/ts)
    local pR=s_patrolRange or 5
    local out=(dX>0 and math.floor(nx/ts)>sgx+pR)
           or (dX<0 and math.floor(nx/ts)<sgx-pR)
           or (dY>0 and math.floor(ny/ts)>sgy+pR)
           or (dY<0 and math.floor(ny/ts)<sgy-pR)
    if boxBlocked(nx,ny) or out then s_dirX=-dX; s_dirY=-dY
    else safeMove(entity, dX*PATROL_SPEED, dY*PATROL_SPEED, dt) end
end
function ai_updateChase(entity,dt,px,py)
    local ts=getTileSize(); local ex,ey=GetPos(entity)
    s_lastKnownPX=px; s_lastKnownPY=py
    local cx=ex+ts*0.5; local cy=ey+ts*0.5
    local dx=(px+ts*0.5)-cx; local dy=(py+ts*0.5)-cy
    local dist=math.sqrt(dx*dx+dy*dy); if dist<1e-3 then return end
    local dX=dx/dist; local dY=dy/dist
    if math.abs(dx)>math.abs(dy) then s_dirX=dx>0 and 1 or -1; s_dirY=0
    else s_dirX=0; s_dirY=dy>0 and 1 or -1 end
    local sx=dX*CHASE_SPEED*dt; local sy=dY*CHASE_SPEED*dt
    if boxBlocked(ex+sx,ey+sy) then
        if   not boxBlocked(ex+sx,ey) then safeMove(entity,dX*CHASE_SPEED,0,dt)
        elseif not boxBlocked(ex,ey+sy) then safeMove(entity,0,dY*CHASE_SPEED,dt) end
    else safeMove(entity,dX*CHASE_SPEED,dY*CHASE_SPEED,dt) end
end
function ai_updateSearch(entity,dt)
    local ts=getTileSize(); local ex,ey=GetPos(entity)
    local geX=math.floor(ex/ts); local geY=math.floor(ey/ts)
    local gtX=math.floor((s_lastKnownPX or ex)/ts)
    local gtY=math.floor((s_lastKnownPY or ey)/ts)
    if s_hasLastSeen then
        if geX==gtX and geY==gtY then
            s_hasLastSeen=false; s_searchTimer=0; s_searchPhase=0; return
        end
        local ddx=gtX-geX; local ddy=gtY-geY; local nX=geX; local nY=geY
        if math.abs(ddx)>=math.abs(ddy) then nX=geX+(ddx>0 and 1 or -1)
        else nY=geY+(ddy>0 and 1 or -1) end
        if not isBlocked(nX*ts+ts*0.5, nY*ts+ts*0.5) then
            local nx2,ny2=ai_norm(nX*ts+ts*0.5-(ex+ts*0.5), nY*ts+ts*0.5-(ey+ts*0.5))
            safeMove(entity, nx2*SEARCH_SPEED, ny2*SEARCH_SPEED, dt)
        else
            local aX=geX; local aY=geY
            if math.abs(ddx)>=math.abs(ddy) then aY=geY+(ddy>0 and 1 or -1)
            else aX=geX+(ddx>0 and 1 or -1) end
            if not isBlocked(aX*ts+ts*0.5, aY*ts+ts*0.5) then
                local nx2,ny2=ai_norm(aX*ts+ts*0.5-(ex+ts*0.5), aY*ts+ts*0.5-(ey+ts*0.5))
                safeMove(entity, nx2*SEARCH_SPEED, ny2*SEARCH_SPEED, dt)
            end
        end
        return
    end
    s_searchTimer=(s_searchTimer or 0)+dt
    local dirs={{1,0},{0,1},{-1,0},{0,-1}}; local ph=s_searchPhase or 0
    if s_searchTimer>(ph+1)*0.5 then ph=ph+1; s_searchPhase=ph
        if ph>=4 then s_searchDone=true end
    end
    if ph<4 then s_dirX=dirs[ph+1][1]; s_dirY=dirs[ph+1][2] end
end
function ai_moveToSpawn(entity,dt)
    local ts=getTileSize(); local ex,ey=GetPos(entity)
    local ecx=ex+ts*0.5; local ecy=ey+ts*0.5
    local scx=(s_spawnX or ex)+ts*0.5; local scy=(s_spawnY or ey)+ts*0.5
    local dX=scx-ecx; local dY=scy-ecy; s_atSpawn=false
    if dX*dX+dY*dY<4.0 then
        s_atSpawn=true; s_movingToTile=false
        SetPos(entity, s_spawnX or ex, s_spawnY or ey); return
    end
    local dist=math.sqrt(dX*dX+dY*dY); local ndx=dX/dist; local ndy=dY/dist
    if math.abs(dX)>math.abs(dY) then s_dirX=dX>0 and 1 or -1; s_dirY=0
    else s_dirX=0; s_dirY=dY>0 and 1 or -1 end
    if boxBlocked(ex+ndx*PATROL_SPEED*dt, ey+ndy*PATROL_SPEED*dt) then
        if   not boxBlocked(ex+ndx*PATROL_SPEED*dt, ey) then safeMove(entity,ndx*PATROL_SPEED,0,dt)
        elseif not boxBlocked(ex, ey+ndy*PATROL_SPEED*dt) then safeMove(entity,0,ndy*PATROL_SPEED,dt) end
    else safeMove(entity, ndx*PATROL_SPEED, ndy*PATROL_SPEED, dt) end
end

-- =============================================================================
-- OnStart
-- =============================================================================
function OnStart(entity)
    local ts = getTileSize()
    local ex, ey = GetPos(entity)
    -- FSM
    s_fsm         = ST_PATROL
    s_dirX        = 1;  s_dirY = 0
    -- Spawn / patrol
    s_spawnX      = ex; s_spawnY = ey; s_patrolRange = 5
    -- Search
    s_lastKnownPX = ex; s_lastKnownPY = ey
    s_hasLastSeen = false; s_searchTimer = 0; s_searchPhase = 0; s_searchDone = false
    s_atSpawn     = false; s_movingToTile = false
    -- Attack combo  (maps to C++ channelerShotsFired / channelerComboPhase / etc.)
    s_shotsFired  = 0           -- channelerShotsFired : 0-2 during NORMAL
    s_comboPhase  = CP_NORMAL   -- channelerComboPhase
    s_burstIndex  = 0           -- channelerBurstIndex : 0-7 during FIRING
    s_attackTimer = 0.0         -- AttackTimer
    s_tileSize    = ts
end

-- =============================================================================
-- OnUpdate
-- =============================================================================
function OnUpdate(entity, dt)
    dt = normDt(dt)
    if s_fsm == nil then OnStart(entity) end

    local player = FindPlayer(); if player == nil then return end
    local px, py = GetPos(player); px = tonumber(px) or 0.0; py = tonumber(py) or 0.0
    local ex, ey = GetPos(entity); ex = tonumber(ex) or 0.0; ey = tonumber(ey) or 0.0
    local ts  = s_tileSize or getTileSize()
    local ecx = ex + ts*0.5;  local ecy = ey + ts*0.5
    local pcx = px + ts*0.25; local pcy = py + ts*0.5

    -- -------------------------------------------------------------------------
    if s_fsm == ST_PATROL then
        if ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) then s_fsm=ST_CHASE; return end
        ai_updatePatrol(entity, dt)

    -- -------------------------------------------------------------------------
    elseif s_fsm == ST_CHASE then
        if not ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY)
           and not ai_hasLOS(ecx,ecy,px,py) then
            s_fsm=ST_SEARCH; s_searchTimer=0; s_searchPhase=0
            s_searchDone=false; s_hasLastSeen=true; return
        end
        if ai_distToPlayer(entity,px,py) <= MAX_RANGE_TILES*ts then
            s_fsm=ST_ATTACK; s_attackTimer=0; s_shotsFired=0
            s_comboPhase=CP_NORMAL; s_burstIndex=0; return
        end
        ai_updateChase(entity, dt, px, py)

    -- -------------------------------------------------------------------------
    elseif s_fsm == ST_SEARCH then
        if ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY)
           or ai_hasLOS(ecx,ecy,px,py) then s_fsm=ST_CHASE; return end
        ai_updateSearch(entity, dt)
        if s_searchDone then
            s_searchDone=false; s_fsm=ST_RETURN; s_movingToTile=false; s_atSpawn=false
        end

    -- -------------------------------------------------------------------------
    elseif s_fsm == ST_RETURN then
        if ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) then s_fsm=ST_CHASE; return end
        ai_moveToSpawn(entity, dt)
        if s_atSpawn then s_fsm=ST_PATROL end

    -- -------------------------------------------------------------------------
    elseif s_fsm == ST_ATTACK then
        local dx   = pcx - ecx; local dy = pcy - ecy
        local dist = math.sqrt(dx*dx + dy*dy)
        local minR = MIN_RANGE_TILES * ts
        local maxR = MAX_RANGE_TILES * ts

        -- Lose LOS → search
        if not ai_hasLOS(ecx,ecy,px,py) then
            s_fsm=ST_SEARCH; s_searchTimer=0; s_searchPhase=0
            s_searchDone=false; s_hasLastSeen=true; return
        end

        -- Always face player
        if math.abs(dx)>math.abs(dy) then s_dirX=dx>0 and 1 or -1; s_dirY=0
        else s_dirX=0; s_dirY=dy>0 and 1 or -1 end

        -- -------------------------------------------------------------------
        -- CHARGING: frozen wind-up — count down then switch to FIRING
        -- Lua: if s_comboPhase == CP_CHARGING then
        -- -------------------------------------------------------------------
        if s_comboPhase == CP_CHARGING then
            SetPos(entity, ex, ey)   -- freeze in place
            s_attackTimer = s_attackTimer - dt
            if s_attackTimer <= 0.0 then
                s_comboPhase = CP_FIRING
                s_burstIndex = 0
            end
            return
        end

        -- -------------------------------------------------------------------
        -- FIRING: one direction per frame until all 8 are sent
        -- Lua: if s_comboPhase == CP_FIRING then
        -- -------------------------------------------------------------------
        if s_comboPhase == CP_FIRING then
            SetPos(entity, ex, ey)   -- freeze in place
            if s_burstIndex < 8 then
                local d = BURST_DIRS[s_burstIndex + 1]
                -- SpawnProjectile uses the direction vector directly
                SpawnProjectile(entity, d[1], d[2])
                SendMessage(d[3])
                s_burstIndex = s_burstIndex + 1
            else
                -- All 8 fired — combo cooldown then full reset
                s_comboPhase  = CP_NORMAL
                s_shotsFired  = 0
                s_attackTimer = COMBO_COOLDOWN
            end
            return
        end

        -- -------------------------------------------------------------------
        -- NORMAL: kite into sweet spot, then fire aimed shots
        -- Lua: if s_comboPhase == CP_NORMAL then (implicit — fall through)
        -- -------------------------------------------------------------------

        -- Kite repositioning — mirrors C++ kiteSpeed block
        if dist < minR and dist > 0.001 then
            local ndx, ndy = ai_norm(-dx, -dy)
            if boxBlocked(ex+ndx*KITE_SPEED*dt, ey+ndy*KITE_SPEED*dt) then
                if   not boxBlocked(ex+ndx*KITE_SPEED*dt, ey) then safeMove(entity,ndx*KITE_SPEED,0,dt)
                elseif not boxBlocked(ex, ey+ndy*KITE_SPEED*dt) then safeMove(entity,0,ndy*KITE_SPEED,dt) end
            else safeMove(entity, ndx*KITE_SPEED, ndy*KITE_SPEED, dt) end
        elseif dist > maxR then
            local ndx, ndy = ai_norm(dx, dy)
            if boxBlocked(ex+ndx*KITE_SPEED*dt, ey+ndy*KITE_SPEED*dt) then
                if   not boxBlocked(ex+ndx*KITE_SPEED*dt, ey) then safeMove(entity,ndx*KITE_SPEED,0,dt)
                elseif not boxBlocked(ex, ey+ndy*KITE_SPEED*dt) then safeMove(entity,0,ndy*KITE_SPEED,dt) end
            else safeMove(entity, ndx*KITE_SPEED, ndy*KITE_SPEED, dt) end
        end

        -- Only fire when inside the sweet spot
        if dist < minR or dist > maxR then return end

        -- Tick attack timer
        s_attackTimer = s_attackTimer - dt
        if s_attackTimer > 0.0 then return end

        if s_shotsFired < 3 then
            -- Normal aimed single shot
            local ndx, ndy = ai_norm(dx, dy)
            SpawnProjectile(entity, ndx, ndy)
            SendMessage("RangedNormalAttack")
            s_attackTimer = NORMAL_SHOT_DELAY
            s_shotsFired  = s_shotsFired + 1
        else
            -- 3 shots done — begin burst charge
            -- Lua: s_comboPhase = CP_CHARGING, s_attackTimer = BURST_CHARGE_TIME
            s_comboPhase  = CP_CHARGING
            s_attackTimer = BURST_CHARGE_TIME
            SendMessage("BossOmniCharge")
        end
    end
end
