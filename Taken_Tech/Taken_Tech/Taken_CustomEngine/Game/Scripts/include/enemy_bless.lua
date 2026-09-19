-- =============================================================================
-- enemy_bless.lua  —  Bless mob
--
-- Two modes, matching C++ updateBless / BlessHealAllyState / BlessAttackPlayerState:
--
-- MODE 1: ALLY ALIVE  (s_rangedAllyAlive == true, default)
--   Ignores the player entirely.  Moves toward the ranged ally and casts
--   "BlessHealAlly" on a cooldown when within blessHealRange (3 tiles).
--   Stand still and channel for healCastTime (1.2 s), then 3 s cooldown.
--
-- MODE 2: ALLY DEAD   (s_rangedAllyAlive == false)
--   Switches to aggression.  Chases the player, fires "BlessAttackPlayer"
--   (slow homing projectile) when within blessAttackRange (4 tiles).
--   Stand still and channel for attackCastTime (1.5 s), then 4 s cooldown.
--   Sends "BlessAllyDied" once on mode switch (matches BlessAttackPlayerState::Enter).
--   Transitions to SEARCH if LOS to player is lost.
--
-- How to notify this script that the ally died:
--   From C++/game system, set the Lua global on the entity's _ENV:
--       env["s_rangedAllyAlive"] = false;
--   Or expose a "NotifyAllyDied(entity)" Lua binding that does the same.
--
-- Messages sent (same strings as C++):
--   "BlessHealAlly"     — restore ally HP
--   "BlessAttackPlayer" — fire slow homing projectile at player
--   "BlessAllyDied"     — broadcast once when switching to attack mode
-- =============================================================================

-- Timing / range constants  (match C++ updateBless exactly)
BLESS_HEAL_RANGE    = 3.0   -- tiles, cast heal on ally within this distance
BLESS_ATK_RANGE     = 4.0   -- tiles, fire at player within this distance
HEAL_CAST_TIME      = 1.2   -- seconds to channel the heal
HEAL_COOLDOWN       = 3.0   -- seconds between heals
ATTACK_CAST_TIME    = 1.5   -- seconds to channel the attack projectile
ATTACK_COOLDOWN     = 4.0   -- seconds between attack casts
BLESS_MOVE_SPEED    = 55.0

-- Shared movement constants
PATROL_SPEED        = 96.0
CHASE_SPEED         = 150.0
SEARCH_SPEED        = 60.0
VISION_TILES        = 3.0

-- FSM states
ST_PATROL = "PATROL"
ST_CHASE  = "CHASE"
ST_SEARCH = "SEARCH"
ST_RETURN = "RETURN"
ST_BLESS  = "BLESS"    -- covers both heal-ally and attack-player modes

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
-- Collision / movement helpers
-- ---------------------------------------------------------------------------
function getTileSize()
    local ts = 100.0
    if type(AI_TileSize) == "function" then
        local t = tonumber(AI_TileSize()) or 0.0; if t > 0.0 then ts = t end
    end
    return ts
end
function isBlocked(wx, wy)
    if type(AI_IsBlocked) == "function" then return AI_IsBlocked(wx, wy) end; return false
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

-- ---------------------------------------------------------------------------
-- Math / vision helpers
-- ---------------------------------------------------------------------------
function ai_dist(ax, ay, bx, by)
    local dx = bx-ax; local dy = by-ay; return math.sqrt(dx*dx + dy*dy)
end
function ai_norm(dx, dy)
    local l = math.sqrt(dx*dx + dy*dy); if l < 0.001 then return 0, 0 end; return dx/l, dy/l
end
function ai_hasLOS(ax, ay, bx, by)
    if type(AI_HasLOS) == "function" then return AI_HasLOS(ax, ay, bx, by) end; return true
end
function ai_playerInFront(ecx, ecy, px, py, dX, dY)
    local ts = getTileSize()
    local dx = px-ecx; local dy = py-ecy; local mag = ai_dist(ecx, ecy, px, py)
    if mag < 0.001 or mag > VISION_TILES*ts then return false end
    if (dx*dX + dy*dY)/mag < 0.7071 then return false end
    return ai_hasLOS(ecx, ecy, px, py)
end

-- ---------------------------------------------------------------------------
-- Shared FSM movement helpers
-- ---------------------------------------------------------------------------
function ai_updatePatrol(entity, dt)
    local ts = getTileSize(); local ex, ey = GetPos(entity)
    local dX = s_dirX or 1; local dY = s_dirY or 0
    local nx = ex+dX*PATROL_SPEED*dt; local ny = ey+dY*PATROL_SPEED*dt
    local sgx = math.floor((s_spawnX or ex)/ts); local sgy = math.floor((s_spawnY or ey)/ts)
    local pR = s_patrolRange or 5
    local out = (dX>0 and math.floor(nx/ts)>sgx+pR) or (dX<0 and math.floor(nx/ts)<sgx-pR)
             or (dY>0 and math.floor(ny/ts)>sgy+pR) or (dY<0 and math.floor(ny/ts)<sgy-pR)
    if boxBlocked(nx, ny) or out then s_dirX = -dX; s_dirY = -dY
    else safeMove(entity, dX*PATROL_SPEED, dY*PATROL_SPEED, dt) end
end

function ai_updateChase(entity, dt, px, py)
    local ts = getTileSize(); local ex, ey = GetPos(entity)
    s_lastKnownPX = px; s_lastKnownPY = py
    local cx = ex+ts*0.5; local cy = ey+ts*0.5
    local dx = (px+ts*0.5)-cx; local dy = (py+ts*0.5)-cy
    local dist = math.sqrt(dx*dx + dy*dy); if dist < 1e-3 then return end
    local dX = dx/dist; local dY = dy/dist
    if math.abs(dx) > math.abs(dy) then s_dirX = dx>0 and 1 or -1; s_dirY = 0
    else s_dirX = 0; s_dirY = dy>0 and 1 or -1 end
    local sx = dX*CHASE_SPEED*dt; local sy = dY*CHASE_SPEED*dt
    if boxBlocked(ex+sx, ey+sy) then
        if not boxBlocked(ex+sx, ey) then safeMove(entity, dX*CHASE_SPEED, 0, dt)
        elseif not boxBlocked(ex, ey+sy) then safeMove(entity, 0, dY*CHASE_SPEED, dt) end
    else safeMove(entity, dX*CHASE_SPEED, dY*CHASE_SPEED, dt) end
end

function ai_updateSearch(entity, dt)
    local ts = getTileSize(); local ex, ey = GetPos(entity)
    local geX = math.floor(ex/ts); local geY = math.floor(ey/ts)
    local gtX = math.floor((s_lastKnownPX or ex)/ts)
    local gtY = math.floor((s_lastKnownPY or ey)/ts)
    if s_hasLastSeen then
        if geX == gtX and geY == gtY then
            s_hasLastSeen = false; s_searchTimer = 0; s_searchPhase = 0; return
        end
        local ddx = gtX-geX; local ddy = gtY-geY; local nX = geX; local nY = geY
        if math.abs(ddx) >= math.abs(ddy) then nX = geX+(ddx>0 and 1 or -1)
        else nY = geY+(ddy>0 and 1 or -1) end
        if not isBlocked(nX*ts+ts*0.5, nY*ts+ts*0.5) then
            local nx2, ny2 = ai_norm(nX*ts+ts*0.5-(ex+ts*0.5), nY*ts+ts*0.5-(ey+ts*0.5))
            safeMove(entity, nx2*SEARCH_SPEED, ny2*SEARCH_SPEED, dt)
        else
            local aX = geX; local aY = geY
            if math.abs(ddx) >= math.abs(ddy) then aY = geY+(ddy>0 and 1 or -1)
            else aX = geX+(ddx>0 and 1 or -1) end
            if not isBlocked(aX*ts+ts*0.5, aY*ts+ts*0.5) then
                local nx2, ny2 = ai_norm(aX*ts+ts*0.5-(ex+ts*0.5), aY*ts+ts*0.5-(ey+ts*0.5))
                safeMove(entity, nx2*SEARCH_SPEED, ny2*SEARCH_SPEED, dt)
            end
        end
        return
    end
    s_searchTimer = (s_searchTimer or 0) + dt
    local dirs = {{1,0},{0,1},{-1,0},{0,-1}}; local ph = s_searchPhase or 0
    if s_searchTimer > (ph+1)*0.5 then
        ph = ph+1; s_searchPhase = ph; if ph >= 4 then s_searchDone = true end
    end
    if ph < 4 then s_dirX = dirs[ph+1][1]; s_dirY = dirs[ph+1][2] end
end

function ai_moveToSpawn(entity, dt)
    local ts = getTileSize(); local ex, ey = GetPos(entity)
    local ecx = ex+ts*0.5; local ecy = ey+ts*0.5
    local scx = (s_spawnX or ex)+ts*0.5; local scy = (s_spawnY or ey)+ts*0.5
    local dX = scx-ecx; local dY = scy-ecy; s_atSpawn = false
    if dX*dX + dY*dY < 4.0 then
        s_atSpawn = true; s_movingToTile = false
        SetPos(entity, s_spawnX or ex, s_spawnY or ey); return
    end
    local dist = math.sqrt(dX*dX + dY*dY); local ndx = dX/dist; local ndy = dY/dist
    if math.abs(dX) > math.abs(dY) then s_dirX = dX>0 and 1 or -1; s_dirY = 0
    else s_dirX = 0; s_dirY = dY>0 and 1 or -1 end
    if boxBlocked(ex+ndx*PATROL_SPEED*dt, ey+ndy*PATROL_SPEED*dt) then
        if not boxBlocked(ex+ndx*PATROL_SPEED*dt, ey) then safeMove(entity, ndx*PATROL_SPEED, 0, dt)
        elseif not boxBlocked(ex, ey+ndy*PATROL_SPEED*dt) then safeMove(entity, 0, ndy*PATROL_SPEED, dt) end
    else safeMove(entity, ndx*PATROL_SPEED, ndy*PATROL_SPEED, dt) end
end

-- ---------------------------------------------------------------------------
-- Move toward a world-space centre point (used in bless modes)
-- ---------------------------------------------------------------------------
function moveToward(entity, dt, tx, ty, speed)
    local ex, ey = GetPos(entity); ex = tonumber(ex) or 0.0; ey = tonumber(ey) or 0.0
    local ts = s_tileSize or getTileSize()
    local ecx = ex+ts*0.5; local ecy = ey+ts*0.5
    local dx = tx-ecx; local dy = ty-ecy; local dist = math.sqrt(dx*dx + dy*dy)
    if dist < 0.001 then return end
    local ndx, ndy = dx/dist, dy/dist
    if boxBlocked(ex+ndx*speed*dt, ey+ndy*speed*dt) then
        if not boxBlocked(ex+ndx*speed*dt, ey) then safeMove(entity, ndx*speed, 0, dt)
        elseif not boxBlocked(ex, ey+ndy*speed*dt) then safeMove(entity, 0, ndy*speed, dt) end
    else safeMove(entity, ndx*speed, ndy*speed, dt) end
    -- Update facing direction
    if math.abs(dx) > math.abs(dy) then s_dirX = dx>0 and 1 or -1; s_dirY = 0
    else s_dirX = 0; s_dirY = dy>0 and 1 or -1 end
end

-- ===========================================================================
-- OnStart
-- ===========================================================================
function OnStart(entity)
    local ts = getTileSize(); local ex, ey = GetPos(entity)
    s_fsm             = ST_PATROL
    s_dirX            = 1; s_dirY = 0
    s_spawnX          = ex; s_spawnY = ey; s_patrolRange = 5
    s_lastKnownPX     = ex; s_lastKnownPY = ey
    s_hasLastSeen     = false; s_searchTimer = 0; s_searchPhase = 0; s_searchDone = false
    s_atSpawn         = false; s_movingToTile = false
    -- Bless state  (mirrors C++ isCastingBless / blessActionTimer / blessCooldownTimer)
    s_isCasting       = false
    s_actionTimer     = 0.0
    s_cooldownTimer   = 0.0
    -- Ally alive flag — set to false by game system when ranged ally dies
    -- e.g. from C++: env["s_rangedAllyAlive"] = false
    if s_rangedAllyAlive == nil then s_rangedAllyAlive = true end
    s_allyDiedNotified = false   -- guard so "BlessAllyDied" fires only once
    s_tileSize        = ts
end

-- ===========================================================================
-- OnUpdate
-- ===========================================================================
function OnUpdate(entity, dt)
    dt = normDt(dt)
    if s_fsm == nil then OnStart(entity) end

    local player = FindPlayer(); if player == nil then return end
    local px, py = GetPos(player); px = tonumber(px) or 0.0; py = tonumber(py) or 0.0
    local ex, ey = GetPos(entity); ex = tonumber(ex) or 0.0; ey = tonumber(ey) or 0.0
    local ts = s_tileSize or getTileSize()
    local ecx = ex+ts*0.5; local ecy = ey+ts*0.5
    local pcx = px+ts*0.25; local pcy = py+ts*0.5

    -- -----------------------------------------------------------------------
    -- PATROL — spot player (or ally dies) to enter BLESS state
    -- -----------------------------------------------------------------------
    if s_fsm == ST_PATROL then
        -- Ally died while patrolling — immediately switch to attack mode
        if not s_rangedAllyAlive then
            s_fsm = ST_BLESS; s_isCasting = false; s_actionTimer = 0; s_cooldownTimer = 0
            if not s_allyDiedNotified then
                if type(SendMessage) == "function" then SendMessage("BlessAllyDied") end
                s_allyDiedNotified = true
            end
            return
        end
        if ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) and ai_hasLOS(ecx,ecy,px,py) then
            s_fsm = ST_BLESS; s_isCasting = false; s_actionTimer = 0; s_cooldownTimer = 0; return
        end
        ai_updatePatrol(entity, dt)

    -- -----------------------------------------------------------------------
    -- CHASE — only used in MODE 2 (ally dead) when returning after search
    -- -----------------------------------------------------------------------
    elseif s_fsm == ST_CHASE then
        if not ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) and not ai_hasLOS(ecx,ecy,px,py) then
            s_fsm = ST_SEARCH; s_searchTimer=0; s_searchPhase=0; s_searchDone=false; s_hasLastSeen=true; return
        end
        local dist = ai_dist(ecx, ecy, pcx, pcy)
        if dist <= BLESS_ATK_RANGE*ts then
            s_fsm = ST_BLESS; s_isCasting = false; s_actionTimer = 0; s_cooldownTimer = 0; return
        end
        ai_updateChase(entity, dt, px, py)

    -- -----------------------------------------------------------------------
    -- SEARCH
    -- -----------------------------------------------------------------------
    elseif s_fsm == ST_SEARCH then
        if ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) or ai_hasLOS(ecx,ecy,px,py) then
            s_fsm = ST_CHASE; return
        end
        ai_updateSearch(entity, dt)
        if s_searchDone then
            s_searchDone = false; s_fsm = ST_RETURN; s_movingToTile = false; s_atSpawn = false
        end

    -- -----------------------------------------------------------------------
    -- RETURN TO SPAWN
    -- -----------------------------------------------------------------------
    elseif s_fsm == ST_RETURN then
        if ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) and ai_hasLOS(ecx,ecy,px,py) then
            s_fsm = ST_CHASE; return
        end
        ai_moveToSpawn(entity, dt)
        if s_atSpawn then s_fsm = ST_PATROL end

    -- -----------------------------------------------------------------------
    -- BLESS — two modes inside one state, matching C++ updateBless exactly
    -- -----------------------------------------------------------------------
    elseif s_fsm == ST_BLESS then

        -- Tick cooldown every frame regardless of mode (matches C++)
        if s_cooldownTimer > 0 then s_cooldownTimer = s_cooldownTimer - dt end

        -- ===================================================================
        -- MODE 1: Ally still alive — support behaviour
        -- ===================================================================
        if s_rangedAllyAlive then
            -- In C++, BlessHealAllyState passes the ally's world pos as
            -- "playerPos". Since Lua can't do that substitution, we use
            -- s_allyX / s_allyY which the game system should keep updated,
            -- falling back to the player pos if not set.
            local ax = s_allyX or pcx
            local ay = s_allyY or pcy

            local dax = ax-ecx; local day = ay-ecy
            local dist = math.sqrt(dax*dax + day*day)
            local healRange = BLESS_HEAL_RANGE * ts

            -- Currently channelling the heal cast — stand still
            if s_isCasting then
                s_actionTimer = s_actionTimer + dt
                if s_actionTimer >= HEAL_CAST_TIME then
                    if type(SendMessage) == "function" then SendMessage("BlessHealAlly") end
                    s_isCasting     = false
                    s_actionTimer   = 0.0
                    s_cooldownTimer = HEAL_COOLDOWN
                end
                -- stand still — no movement
                return
            end

            -- In range and cooldown ready — begin channelling
            if dist <= healRange and s_cooldownTimer <= 0 then
                s_isCasting   = true
                s_actionTimer = 0.0
                -- stand still
            elseif dist > healRange then
                -- Move toward ally
                moveToward(entity, dt, ax, ay, BLESS_MOVE_SPEED)
            end
            -- (if in range but on cooldown — just stand still and wait)
            return
        end

        -- ===================================================================
        -- MODE 2: Ally dead — attack the player
        -- ===================================================================

        -- Broadcast the mode-switch message exactly once (mirrors BlessAttackPlayerState::Enter)
        if not s_allyDiedNotified then
            if type(SendMessage) == "function" then SendMessage("BlessAllyDied") end
            s_allyDiedNotified = true
            s_isCasting = false; s_actionTimer = 0.0; s_cooldownTimer = 0.0
        end

        -- Lose sight of player → search
        if not ai_hasLOS(ecx, ecy, px, py) then
            s_fsm = ST_SEARCH; s_searchTimer=0; s_searchPhase=0; s_searchDone=false; s_hasLastSeen=true; return
        end

        local pdx = pcx-ecx; local pdy = pcy-ecy
        local pdist = math.sqrt(pdx*pdx + pdy*pdy)
        local atkRange = BLESS_ATK_RANGE * ts

        -- Currently channelling the attack cast — stand still
        if s_isCasting then
            s_actionTimer = s_actionTimer + dt
            if s_actionTimer >= ATTACK_CAST_TIME then
                if type(SendMessage) == "function" then SendMessage("BlessAttackPlayer") end
                s_isCasting     = false
                s_actionTimer   = 0.0
                s_cooldownTimer = ATTACK_COOLDOWN
            end
            -- stand still
            return
        end

        -- In range and cooldown ready — begin channelling attack
        if pdist <= atkRange and s_cooldownTimer <= 0 then
            s_isCasting   = true
            s_actionTimer = 0.0
            -- stand still
        elseif pdist > atkRange then
            -- Move toward player
            moveToward(entity, dt, pcx, pcy, BLESS_MOVE_SPEED)
        end
        -- (if in range but on cooldown — stand still and wait)
    end
end