--[[
* @file     enemy_boss.lua
* @author   Lim Zhi Jie
* @email    zhijie.lim@digipen.edu
* @date     2026-03-31
*
* @brief Implements the Solo Predator Boss AI behavior.
*
* This script defines the state machine and attack logic for the Solo Predator
* boss. The boss cycles through three abilities in a fixed rotation: a ranged
* kiting volley, an underground burrow strike, and a one-time self-heal that
* only becomes available below 60% HP. It has no allies and is entirely
* self-sufficient.
*
* Ability rotation: RANGED -> BURROW -> HEAL -> RANGED -> BURROW -> RANGED -> ...
* HEAL is skipped if HP is above 60% or has already been used this fight,
* in which case the rotation advances to the next slot (RANGED).
*
* Ranged: The boss kites between MIN_RANGE and MAX_RANGE tiles from the player,
* firing a combo of normal aimed shots followed by 3-projectile spread fans.
* The boss shoots while repositioning and does not pause to stand still.
*
* Burrow: Mirrors enemy_burrow.lua exactly. The boss closes in above ground,
* telegraphs, goes underground (collider disabled), chases the player using
* AI_IsWall collision, then erupts after a travel timer expires (collider
* re-enabled). In Phase 3 (enraged, hp <= 33%) the boss performs a
* double-strike: after the first stun window it immediately re-telegraphs and
* burrows a second time before entering the cooldown.
*
* Heal: One-time use per fight. The boss channels for 1.8 seconds, kiting away
* from the player for the first half of the cast, then restores HP on
* completion. Once used (s_healUsed = true) it is permanently skipped.
*
* Phase escalation (re-evaluated every frame from live HP):
*   Phase 1  hp > 66%   baseline -- 3 normal shots before spread
*   Phase 2  hp <= 66%  normal shot count rises to 4; move speed increases
*   Phase 3  hp <= 33%  enraged -- shot gaps halved; double burrow strike;
*                        fires "BossEnraged" message once on threshold cross
*
* Messages sent: "RangedNormalAttack", "RangedSpreadAttack",
*                "BurrowWarning", "BurrowUnderground",
*                "BossHealStart", "BossHealDone",
*                "BossSequenceAdv", "BossEnraged"
*
* States: PATROL -> CHASE -> BOSS -> SEARCH -> RETURN -> PATROL
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the
* prior written consent of DigiPen Institute of Technology is prohibited.
*]]

-- ---------------------------------------------------------------------------
-- Tuning constants
-- ---------------------------------------------------------------------------
BOSS2_MOVE_SPEED        = 130.0
BOSS2_ENRAGED_SPEED     = 145.0   -- Phase 3 move speed
PATROL_SPEED            = 96.0
VISION_TILES            = 5.0

-- Ranged
B2_MIN_RANGE_TILES      = 2.5     -- stay at least this far from player
B2_MAX_RANGE_TILES      = 5.5     -- chase if farther than this
B2_NORMAL_GAP           = 2.0     -- seconds between normal shots
B2_SPREAD_GAP           = 0.4     -- seconds between spread bursts
B2_COMBO_END_PAUSE      = 1.5     -- rest after full combo before picking next
B2_NORMAL_COUNT_P1      = 3       -- normal shots before spread in Phase 1
B2_NORMAL_COUNT_P2      = 4       -- normal shots before spread in Phase 2+
B2_SPREAD_COUNT         = 3       -- spread shots always
B2_SPREAD_ANGLE         = 25      -- degrees either side of center

-- Burrow
B2_BURROW_RANGE_TILES   = 4.0     -- how close boss must be to trigger burrow
B2_TELEGRAPH_DUR        = 0.8
B2_UNDERGROUND_SPEED    = 200.0   -- speed while burrowed
B2_BURROW_TRAVEL_TIME   = 2.0     -- time spent underground before erupting
B2_STUN_DUR             = 1.2
B2_COOLDOWN_DUR         = 2.0

-- Heal
B2_HEAL_CAST_TIME       = 1.8     -- channel duration
B2_HEAL_AMOUNT_P1       = 12      -- HP restored per heal in Phase 1/2
B2_HEAL_AMOUNT_P3       = 20      -- HP restored per heal in Phase 3
B2_HEAL_KITE_SPEED      = 90.0    -- speed boss backs away while channelling
B2_HEAL_HP_GATE         = 0.60    -- heal unavailable above this ratio
B2_HEAL_PAUSE           = 0.5     -- brief pause after heal completes

-- Phase thresholds
B2_PHASE2_HP            = 0.66
B2_PHASE3_HP            = 0.33

-- Ability sequence indices
SEQ_RANGED  = 0
SEQ_BURROW  = 1
SEQ_HEAL    = 2

-- Burrow sub-phases
BP_ABOVE    = "ABOVE"
BP_TELE     = "TELE"
BP_BURIED   = "BURIED"
BP_STRIKE   = "STRIKE"
BP_COOL     = "COOL"

-- Animation row indices (mirrors enemy_burrow.lua)
ANIM_PATROL       = 0
ANIM_ENTER_BURROW = 1
ANIM_MOVE_BURROWED= 2
ANIM_EXIT_BURROW  = 3

-- Outer FSM states
ST_PATROL = "PATROL"
ST_CHASE  = "CHASE"
ST_BOSS   = "BOSS"
ST_SEARCH = "SEARCH"
ST_RETURN = "RETURN"

-- ---------------------------------------------------------------------------
-- dt normalisation
-- ---------------------------------------------------------------------------
function normDt(dt)
    dt = tonumber(dt) or 0.0
    if dt > 1.0 then dt = dt * 0.001 end
    if dt > 0.05 then dt = 0.05 end
    return dt
end

-- ---------------------------------------------------------------------------
-- Engine helpers
-- ---------------------------------------------------------------------------
function getTileSize()
    local ts = 100.0
    if type(AI_TileSize) == "function" then
        local t = tonumber(AI_TileSize()) or 0.0; if t > 0.0 then ts = t end
    end
    return ts
end

function isBlocked(wx, wy)
    if type(AI_IsBlocked) == "function" then return AI_IsBlocked(wx, wy) end
    return false
end

function boxBlocked(wx, wy)
    local ts = getTileSize(); local i = 2.0
    return isBlocked(wx+i,wy+i)     or isBlocked(wx+ts-i,wy+i)
        or isBlocked(wx+i,wy+ts-i) or isBlocked(wx+ts-i,wy+ts-i)
end

function safeMove(e, vx, vy, dt)
    local ex, ey = GetPos(e)
    ex = tonumber(ex) or 0.0; ey = tonumber(ey) or 0.0
    local nx = ex + vx*dt; if not boxBlocked(nx, ey) then ex = nx end
    local ny = ey + vy*dt; if not boxBlocked(ex, ny) then ey = ny end
    SetPos(e, ex, ey)
end

function ai_dist(ax, ay, bx, by)
    local dx = bx-ax; local dy = by-ay; return math.sqrt(dx*dx+dy*dy)
end

function ai_norm(dx, dy)
    local l = math.sqrt(dx*dx+dy*dy); if l < 0.001 then return 0, 0 end
    return dx/l, dy/l
end

function ai_hasLOS(ax, ay, bx, by)
    if type(AI_HasLOS) == "function" then return AI_HasLOS(ax, ay, bx, by) end
    return true
end

function ai_playerInFront(ecx, ecy, px, py, dX, dY)
    local ts  = getTileSize()
    local dx  = px-ecx; local dy = py-ecy
    local mag = ai_dist(ecx, ecy, px, py)
    if mag < 0.001 or mag > VISION_TILES*ts then return false end
    if (dx*dX+dy*dY)/mag < 0.7071 then return false end
    return ai_hasLOS(ecx, ecy, px, py)
end

function ai_distToPlayer(entity, px, py)
    local ts = getTileSize(); local ex, ey = GetPos(entity)
    return ai_dist(ex+ts*0.5, ey+ts*0.5, px+ts*0.25, py+ts*0.5)
end

function sendMsg(id)
    if type(SendMessage) == "function" then SendMessage(id) end
end

-- ---------------------------------------------------------------------------
-- Animation helpers (mirrors enemy_burrow.lua)
-- ---------------------------------------------------------------------------
function anim_setBurrowPatrol(entity)
    if type(SetAnimationRow) == "function" then SetAnimationRow(entity, ANIM_PATROL) end
end

function anim_setEnterBurrow(entity)
    if type(SetAnimationRow) == "function" then SetAnimationRow(entity, ANIM_ENTER_BURROW) end
end

function anim_setMoveBurrowed(entity)
    if type(SetAnimationRow) == "function" then SetAnimationRow(entity, ANIM_MOVE_BURROWED) end
end

function anim_setExitBurrow(entity)
    if type(SetAnimationRow) == "function" then SetAnimationRow(entity, ANIM_EXIT_BURROW) end
end

-- ---------------------------------------------------------------------------
-- Phase helper — returns 1/2/3 based on current HP ratio
-- ---------------------------------------------------------------------------
function getPhase(entity)
    if type(GetEnemyHp) ~= "function" or type(GetEnemyMaxHp) ~= "function" then
        return 1
    end
    local hp    = tonumber(GetEnemyHp(entity))    or 1
    local maxHp = tonumber(GetEnemyMaxHp(entity)) or 1
    local ratio = hp / maxHp
    if ratio <= B2_PHASE3_HP then return 3 end
    if ratio <= B2_PHASE2_HP then return 2 end
    return 1
end

-- ---------------------------------------------------------------------------
-- Patrol
-- ---------------------------------------------------------------------------
function ai_updatePatrol(entity, dt)
    local ts = getTileSize(); local ex, ey = GetPos(entity)
    local dX = s_dirX or 1; local dY = s_dirY or 0
    local nx = ex+dX*PATROL_SPEED*dt; local ny = ey+dY*PATROL_SPEED*dt
    local sgx = math.floor((s_spawnX or ex)/ts)
    local sgy = math.floor((s_spawnY or ey)/ts)
    local pR  = s_patrolRange or 5
    local out = (dX>0 and math.floor(nx/ts)>sgx+pR) or (dX<0 and math.floor(nx/ts)<sgx-pR)
             or (dY>0 and math.floor(ny/ts)>sgy+pR) or (dY<0 and math.floor(ny/ts)<sgy-pR)
    if boxBlocked(nx, ny) or out then
        s_dirX = -dX; s_dirY = -dY
    else
        safeMove(entity, dX*PATROL_SPEED, dY*PATROL_SPEED, dt)
    end
end

-- ---------------------------------------------------------------------------
-- Chase
-- ---------------------------------------------------------------------------
function ai_updateChase(entity, dt, px, py)
    local ts = getTileSize(); local ex, ey = GetPos(entity)
    s_lastKnownPX = px; s_lastKnownPY = py
    local cx = ex+ts*0.5; local cy = ey+ts*0.5
    local dx = (px+ts*0.25)-cx; local dy = (py+ts*0.5)-cy
    local dist = math.sqrt(dx*dx+dy*dy); if dist < 1e-3 then return end
    local dX = dx/dist; local dY = dy/dist
    if math.abs(dx)>math.abs(dy) then s_dirX=dx>0 and 1 or -1; s_dirY=0
    else s_dirX=0; s_dirY=dy>0 and 1 or -1 end
    local speed = (s_phase == 3) and B2_ENRAGED_SPEED or BOSS2_MOVE_SPEED
    local sx = dX*speed*dt; local sy = dY*speed*dt
    if boxBlocked(ex+sx, ey+sy) then
        if not boxBlocked(ex+sx, ey) then safeMove(entity, dX*speed, 0, dt)
        elseif not boxBlocked(ex, ey+sy) then safeMove(entity, 0, dY*speed, dt) end
    else
        safeMove(entity, dX*speed, dY*speed, dt)
    end
end

-- ---------------------------------------------------------------------------
-- Search
-- ---------------------------------------------------------------------------
function ai_updateSearch(entity, dt)
    local ts  = getTileSize(); local ex, ey = GetPos(entity)
    local geX = math.floor(ex/ts); local geY = math.floor(ey/ts)
    local gtX = math.floor((s_lastKnownPX or ex)/ts)
    local gtY = math.floor((s_lastKnownPY or ey)/ts)
    if s_hasLastSeen then
        if geX==gtX and geY==gtY then s_hasLastSeen=false; s_searchTimer=0; s_searchPhase=0; return end
        local ddx=gtX-geX; local ddy=gtY-geY; local nX=geX; local nY=geY
        if math.abs(ddx)>=math.abs(ddy) then nX=geX+(ddx>0 and 1 or -1)
        else nY=geY+(ddy>0 and 1 or -1) end
        local wx=nX*ts+ts*0.5; local wy=nY*ts+ts*0.5
        if not isBlocked(wx, wy) then
            local ndx, ndy = ai_norm(wx-(ex+ts*0.5), wy-(ey+ts*0.5))
            safeMove(entity, ndx*PATROL_SPEED, ndy*PATROL_SPEED, dt)
        else
            local aX=geX; local aY=geY
            if math.abs(ddx)>=math.abs(ddy) then aY=geY+(ddy>0 and 1 or -1)
            else aX=geX+(ddx>0 and 1 or -1) end
            local ax=aX*ts+ts*0.5; local ay=aY*ts+ts*0.5
            if not isBlocked(ax, ay) then
                local ndx, ndy = ai_norm(ax-(ex+ts*0.5), ay-(ey+ts*0.5))
                safeMove(entity, ndx*PATROL_SPEED, ndy*PATROL_SPEED, dt)
            end
        end
        return
    end
    s_searchTimer = (s_searchTimer or 0) + dt
    local dirs = {{1,0},{0,1},{-1,0},{0,-1}}
    local ph   = s_searchPhase or 0
    if s_searchTimer > (ph+1)*0.5 then ph=ph+1; s_searchPhase=ph; if ph>=4 then s_searchDone=true end end
    if ph < 4 then s_dirX=dirs[ph+1][1]; s_dirY=dirs[ph+1][2] end
end

-- ---------------------------------------------------------------------------
-- Return to spawn
-- ---------------------------------------------------------------------------
function ai_moveToSpawn(entity, dt)
    local ts  = getTileSize(); local ex, ey = GetPos(entity)
    local scx = (s_spawnX or ex)+ts*0.5; local scy = (s_spawnY or ey)+ts*0.5
    local ecx = ex+ts*0.5;               local ecy = ey+ts*0.5
    local dX  = scx-ecx; local dY = scy-ecy; s_atSpawn = false
    if dX*dX+dY*dY < 4.0 then
        s_atSpawn = true; SetPos(entity, s_spawnX or ex, s_spawnY or ey); return
    end
    local dist = math.sqrt(dX*dX+dY*dY)
    local ndx, ndy = dX/dist, dY/dist
    if math.abs(dX)>math.abs(dY) then s_dirX=dX>0 and 1 or -1; s_dirY=0
    else s_dirX=0; s_dirY=dY>0 and 1 or -1 end
    if boxBlocked(ex+ndx*PATROL_SPEED*dt, ey+ndy*PATROL_SPEED*dt) then
        if not boxBlocked(ex+ndx*PATROL_SPEED*dt, ey) then safeMove(entity, ndx*PATROL_SPEED, 0, dt)
        elseif not boxBlocked(ex, ey+ndy*PATROL_SPEED*dt) then safeMove(entity, 0, ndy*PATROL_SPEED, dt) end
    else
        safeMove(entity, ndx*PATROL_SPEED, ndy*PATROL_SPEED, dt)
    end
end

-- ===========================================================================
-- Ability picker  — fixed rotation: RANGED → BURROW → HEAL → RANGED → …
-- HEAL is skipped (replaced by RANGED) if HP is above the gate threshold.
-- ===========================================================================
function pickNextAbility(dist, ts, entity)
    -- Determine the next slot in the fixed rotation
    local rotation = { SEQ_RANGED, SEQ_BURROW, SEQ_HEAL }
    local last = s_seqPhase or SEQ_RANGED

    -- Find current index in rotation
    local curIdx = 1
    for i, v in ipairs(rotation) do
        if v == last then curIdx = i; break end
    end

    -- Advance to next (wrap around)
    local nextIdx = (curIdx % #rotation) + 1
    local next    = rotation[nextIdx]

    -- Skip HEAL if already used or HP is above the gate
    if next == SEQ_HEAL then
        local canHeal = false
        if not s_healUsed and type(GetEnemyHp) == "function" and type(GetEnemyMaxHp) == "function" then
            local hp    = tonumber(GetEnemyHp(entity))    or 1
            local maxHp = tonumber(GetEnemyMaxHp(entity)) or 1
            canHeal = (hp / maxHp) <= B2_HEAL_HP_GATE
        end
        if not canHeal then
            -- Skip heal, take the slot after it in the rotation
            nextIdx = (nextIdx % #rotation) + 1
            next    = rotation[nextIdx]
        end
    end

    return next
end

-- ---------------------------------------------------------------------------
-- advanceTo  — clean transition to next ability
-- ---------------------------------------------------------------------------
function advanceTo(next)
    s_seqPhase         = next
    s_abilityTimer     = 0.0
    s_abilityDone      = false
    s_attackTimer      = 0.0
    if next == SEQ_RANGED then
        s_rangedShots  = 0
    end
    if next == SEQ_BURROW then
        s_burrowPhase    = BP_ABOVE
        s_burrowTimer    = 0.0
        s_isBurrowed     = false
        s_doubleStruck   = false
        s_burrowCooldown = 0.0
    end
    if next == SEQ_HEAL then
        s_healKiting   = true      -- boss will kite away during cast
    end
    sendMsg("BossSequenceAdv")
end

-- ===========================================================================
-- updateBoss  — one ability tick per frame
-- ===========================================================================
function updateBoss(entity, dt, px, py)
    local ts    = getTileSize()
    local ex, ey = GetPos(entity)
    ex = tonumber(ex) or 0.0; ey = tonumber(ey) or 0.0

    local ecx = ex + ts*0.5;   local ecy = ey + ts*0.5
    local pcx = px + ts*0.25;  local pcy = py + ts*0.5
    local dx  = pcx - ecx;     local dy  = pcy - ecy
    local dist = math.sqrt(dx*dx + dy*dy)

    -- Update phase & fire enrage message on first Phase 3 tick
    local newPhase = getPhase(entity)
    if newPhase == 3 and (s_phase or 1) < 3 then
        sendMsg("BossEnraged")
    end
    s_phase = newPhase

    -- Face player (for animation / direction state)
    if math.abs(dx) > math.abs(dy) then
        s_dirX = dx > 0 and 1 or -1; s_dirY = 0
    else
        s_dirX = 0; s_dirY = dy > 0 and 1 or -1
    end

    local minR = B2_MIN_RANGE_TILES * ts
    local maxR = B2_MAX_RANGE_TILES * ts
    local speed = (s_phase == 3) and B2_ENRAGED_SPEED or BOSS2_MOVE_SPEED

    -- =========================================================================
    -- SEQ_RANGED
    -- =========================================================================
    if s_seqPhase == SEQ_RANGED then

        -- ── End-of-combo pause ───────────────────────────────────────────────
        if s_abilityDone then
            s_abilityTimer = s_abilityTimer + dt
            -- Kite while waiting
            local ndx, ndy = ai_norm(dx, dy)
            if dist < minR then
                safeMove(entity, -ndx*speed, -ndy*speed, dt)
            elseif dist > maxR then
                safeMove(entity, ndx*speed, ndy*speed, dt)
            end
            if s_abilityTimer >= B2_COMBO_END_PAUSE then
                advanceTo(pickNextAbility(dist, ts, entity))
            end
            return
        end

        -- ── Kite repositioning (non-blocking — boss still shoots while moving) ──
        if dist < minR then
            local ndx, ndy = ai_norm(dx, dy)
            if boxBlocked(ex-ndx*speed*dt, ey-ndy*speed*dt) then
                if not boxBlocked(ex-ndx*speed*dt, ey) then safeMove(entity, -ndx*speed, 0, dt)
                elseif not boxBlocked(ex, ey-ndy*speed*dt) then safeMove(entity, 0, -ndy*speed, dt) end
            else safeMove(entity, -ndx*speed, -ndy*speed, dt) end
        elseif dist > maxR then
            local ndx, ndy = ai_norm(dx, dy)
            if boxBlocked(ex+ndx*speed*dt, ey+ndy*speed*dt) then
                if not boxBlocked(ex+ndx*speed*dt, ey) then safeMove(entity, ndx*speed, 0, dt)
                elseif not boxBlocked(ex, ey+ndy*speed*dt) then safeMove(entity, 0, ndy*speed, dt) end
            else safeMove(entity, ndx*speed, ndy*speed, dt) end
        end

        -- ── Shoot timer ──────────────────────────────────────────────────────
        s_attackTimer = s_attackTimer - dt
        if s_attackTimer > 0.0 then return end

        -- Determine how many normal shots before spread (phase-dependent)
        local normalCount = (s_phase >= 2) and B2_NORMAL_COUNT_P2 or B2_NORMAL_COUNT_P1
        local totalShots  = normalCount + B2_SPREAD_COUNT

        -- Phase 3 halves shot gaps
        local normalGap = (s_phase == 3) and (B2_NORMAL_GAP*0.5)  or B2_NORMAL_GAP
        local spreadGap = (s_phase == 3) and (B2_SPREAD_GAP*0.5) or B2_SPREAD_GAP

        if s_rangedShots < normalCount then
            -- Normal shot
            local ndx, ndy = ai_norm(pcx-ecx, pcy-ecy)
            SpawnProjectile(entity, ndx, ndy)
            sendMsg("RangedNormalAttack")
            s_attackTimer = normalGap
        else
            -- Spread shot
            local angle  = math.atan(pcy-ecy, pcx-ecx)
            local spread = math.rad(B2_SPREAD_ANGLE)
            for _, offset in ipairs({-spread, 0, spread}) do
                local a = angle + offset
                SpawnProjectile(entity, math.cos(a), math.sin(a))
            end
            sendMsg("RangedSpreadAttack")
            s_attackTimer = spreadGap
        end

        s_rangedShots = s_rangedShots + 1
        if s_rangedShots >= totalShots then
            s_abilityDone  = true
            s_abilityTimer = 0.0
        end

    -- =========================================================================
    -- SEQ_BURROW
    -- =========================================================================
    elseif s_seqPhase == SEQ_BURROW then

        local burrowRange  = B2_BURROW_RANGE_TILES * ts

        -- ── ABOVE: close in until in burrow range, then telegraph ────────────
        if s_burrowPhase == BP_ABOVE then

            anim_setBurrowPatrol(entity)

            if s_burrowCooldown > 0 then s_burrowCooldown = s_burrowCooldown - dt end

            if dist > burrowRange then
                local ndx, ndy = ai_norm(dx, dy)
                if boxBlocked(ex+ndx*speed*dt, ey+ndy*speed*dt) then
                    if not boxBlocked(ex+ndx*speed*dt, ey) then safeMove(entity, ndx*speed, 0, dt)
                    elseif not boxBlocked(ex, ey+ndy*speed*dt) then safeMove(entity, 0, ndy*speed, dt) end
                else safeMove(entity, ndx*speed, ndy*speed, dt) end
                if math.abs(dx)>math.abs(dy) then s_dirX=dx>0 and 1 or -1; s_dirY=0
                else s_dirX=0; s_dirY=dy>0 and 1 or -1 end
            elseif s_burrowCooldown <= 0 then
                -- Enter telegraph
                s_burrowTimer = B2_TELEGRAPH_DUR
                s_burrowPhase = BP_TELE
                s_isBurrowed  = false
                anim_setEnterBurrow(entity)
                sendMsg("BurrowWarning")
            end

        -- ── TELE: count down telegraph window, then go underground ────────────
        elseif s_burrowPhase == BP_TELE then

            s_burrowTimer = s_burrowTimer - dt
            if s_burrowTimer <= 0.0 then
                s_burrowPhase = BP_BURIED
                s_burrowTimer = B2_BURROW_TRAVEL_TIME
                s_isBurrowed  = true
                anim_setMoveBurrowed(entity)
                if type(SetColliderEnabled) == "function" then SetColliderEnabled(entity, false) end
                sendMsg("BurrowUnderground")
            end

        -- ── BURIED: move underground toward player; erupt when timer expires ────
        elseif s_burrowPhase == BP_BURIED then

            anim_setMoveBurrowed(entity)
            s_burrowTimer = s_burrowTimer - dt

            local bex, bey = GetPos(entity)
            bex = tonumber(bex) or 0.0; bey = tonumber(bey) or 0.0
            local bdx = (px + ts*0.5) - (bex + ts*0.5)
            local bdy = (py + ts*0.5) - (bey + ts*0.5)
            local bDist = math.sqrt(bdx*bdx + bdy*bdy)

            if bDist > 1.0 then
                local ndx, ndy = ai_norm(bdx, bdy)
                local nx = bex + ndx * B2_UNDERGROUND_SPEED * dt
                if not AI_IsWall(nx + ts*0.5, bey + ts*0.5) then bex = nx end
                local ny = bey + ndy * B2_UNDERGROUND_SPEED * dt
                if not AI_IsWall(bex + ts*0.5, ny + ts*0.5) then bey = ny end
                SetPos(entity, bex, bey)
            end

            if s_burrowTimer <= 0 then
                s_isBurrowed  = false
                s_burrowTimer = B2_STUN_DUR
                s_burrowPhase = BP_STRIKE
                anim_setExitBurrow(entity)
                if type(SetColliderEnabled) == "function" then SetColliderEnabled(entity, true) end
            end

        -- ── STRIKE: stun window — player can punish ───────────────────────────
        elseif s_burrowPhase == BP_STRIKE then

            s_burrowTimer = s_burrowTimer - dt
            if s_burrowTimer <= 0.0 then
                -- Phase 3: double-strike — re-burrow if not done yet
                if s_phase == 3 and not s_doubleStruck then
                    s_doubleStruck = true
                    s_burrowPhase  = BP_TELE
                    s_burrowTimer  = B2_TELEGRAPH_DUR * 0.6
                    s_isBurrowed   = false
                    anim_setEnterBurrow(entity)
                    sendMsg("BurrowWarning")
                else
                    anim_setBurrowPatrol(entity)
                    s_burrowPhase    = BP_COOL
                    s_burrowTimer    = B2_COOLDOWN_DUR
                    s_burrowCooldown = 0
                end
            end

        -- ── COOL: surface cooldown before picking next ability ────────────────
        elseif s_burrowPhase == BP_COOL then

            anim_setBurrowPatrol(entity)
            s_burrowTimer = s_burrowTimer - dt

            -- Slowly retreat from player while cooling down
            if dist < burrowRange * 0.5 and dist > 0.001 then
                local ndx, ndy = ai_norm(-dx, -dy)
                local slow = speed * 0.3
                if not boxBlocked(ex+ndx*slow*dt, ey+ndy*slow*dt) then
                    safeMove(entity, ndx*slow, ndy*slow, dt)
                end
            end

            if s_burrowTimer <= 0.0 then
                s_burrowPhase = BP_ABOVE
                s_isBurrowed  = false
                advanceTo(pickNextAbility(dist, ts, entity))
            end
        end

    -- =========================================================================
    -- SEQ_HEAL
    -- =========================================================================
    elseif s_seqPhase == SEQ_HEAL then

        -- ── Kite away from player while channelling ───────────────────────────
        if s_healKiting and dist > 0.001 then
            local ndx, ndy = ai_norm(dx, dy)
            local kiteV = B2_HEAL_KITE_SPEED
            if boxBlocked(ex-ndx*kiteV*dt, ey-ndy*kiteV*dt) then
                if not boxBlocked(ex-ndx*kiteV*dt, ey) then safeMove(entity, -ndx*kiteV, 0, dt)
                elseif not boxBlocked(ex, ey-ndy*kiteV*dt) then safeMove(entity, 0, -ndy*kiteV, dt) end
            else safeMove(entity, -ndx*kiteV, -ndy*kiteV, dt) end
        end

        if not s_abilityDone then
            -- ── Channelling ──────────────────────────────────────────────────
            if s_abilityTimer == 0.0 then
                sendMsg("BossHealStart")
            end
            s_abilityTimer = s_abilityTimer + dt

            -- Stop kiting after half the cast so it's not too easy to punish
            if s_abilityTimer >= B2_HEAL_CAST_TIME * 0.5 then
                s_healKiting = false
            end

            if s_abilityTimer >= B2_HEAL_CAST_TIME then
                local healAmt = (s_phase == 3) and B2_HEAL_AMOUNT_P3 or B2_HEAL_AMOUNT_P1
                if type(HealEnemyHp) == "function" then HealEnemyHp(entity, healAmt) end
                sendMsg("BossHealDone")
                s_healUsed     = true
                s_abilityDone  = true
                s_abilityTimer = 0.0
                s_healKiting   = false
            end
        else
            -- ── Post-heal pause, then pick next ──────────────────────────────
            s_abilityTimer = s_abilityTimer + dt
            if s_abilityTimer >= B2_HEAL_PAUSE then
                advanceTo(pickNextAbility(dist, ts, entity))
            end
        end

    end -- end seqPhase switch
end

-- ===========================================================================
-- OnStart
-- ===========================================================================
function OnStart(entity)
    local ts = getTileSize(); local ex, ey = GetPos(entity)

    -- Outer FSM
    s_fsm         = ST_PATROL
    s_dirX        = 1; s_dirY = 0
    s_spawnX      = ex; s_spawnY = ey
    s_patrolRange = 5

    -- Search / return state
    s_lastKnownPX = ex; s_lastKnownPY = ey
    s_hasLastSeen = false; s_searchTimer = 0; s_searchPhase = 0; s_searchDone = false
    s_atSpawn     = false

    -- Ability state
    s_seqPhase     = SEQ_RANGED
    s_abilityTimer = 0.0
    s_abilityDone  = false
    s_attackTimer  = 0.0
    s_rangedShots  = 0

    -- Burrow sub-state
    s_burrowPhase    = BP_ABOVE
    s_burrowTimer    = 0.0
    s_isBurrowed     = false
    s_doubleStruck   = false
    s_burrowCooldown = 0.0

    -- Heal state
    s_healKiting    = false
    s_healUsed      = false   -- heal can only fire once per fight

    -- Phase tracking
    s_phase         = 1

    s_tileSize = ts

    -- Start in above-ground patrol animation with collider enabled
    anim_setBurrowPatrol(entity)
    if type(SetColliderEnabled) == "function" then SetColliderEnabled(entity, true) end
end

-- ===========================================================================
-- OnUpdate
-- ===========================================================================
function OnUpdate(entity, dt)
    dt = normDt(dt)
    if s_fsm == nil then OnStart(entity) end

    local player = FindPlayer(); if player == nil then return end
    local px, py = GetPos(player)
    px = tonumber(px) or 0.0; py = tonumber(py) or 0.0

    local ex, ey = GetPos(entity)
    ex = tonumber(ex) or 0.0; ey = tonumber(ey) or 0.0
    local ts  = s_tileSize or getTileSize()
    local ecx = ex + ts*0.5; local ecy = ey + ts*0.5

    -- =======================================================================
    -- PATROL
    -- =======================================================================
    if s_fsm == ST_PATROL then
        if ai_playerInFront(ecx, ecy, px, py, s_dirX, s_dirY)
        or ai_hasLOS(ecx, ecy, px, py) then
            s_fsm = ST_CHASE; return
        end
        ai_updatePatrol(entity, dt)

    -- =======================================================================
    -- CHASE  — close to max ranged range, then enter ability loop
    -- =======================================================================
    elseif s_fsm == ST_CHASE then
        if not ai_hasLOS(ecx, ecy, px, py) then
            s_fsm = ST_SEARCH; s_searchTimer = 0; s_searchPhase = 0
            s_searchDone = false; s_hasLastSeen = true; return
        end
        if ai_distToPlayer(entity, px, py) <= B2_MAX_RANGE_TILES * ts then
            advanceTo(SEQ_RANGED)   -- always open with a ranged volley
            s_fsm = ST_BOSS; return
        end
        ai_updateChase(entity, dt, px, py)

    -- =======================================================================
    -- BOSS  — run ability sequence
    -- =======================================================================
    elseif s_fsm == ST_BOSS then
        if not ai_hasLOS(ecx, ecy, px, py) then
            -- Only interrupt if NOT currently burrowed (underground movement
            -- doesn't require LOS)
            if not s_isBurrowed then
                s_fsm = ST_SEARCH; s_searchTimer = 0; s_searchPhase = 0
                s_searchDone = false; s_hasLastSeen = true; return
            end
        end
        updateBoss(entity, dt, px, py)

    -- =======================================================================
    -- SEARCH
    -- =======================================================================
    elseif s_fsm == ST_SEARCH then
        if ai_playerInFront(ecx, ecy, px, py, s_dirX, s_dirY)
        or ai_hasLOS(ecx, ecy, px, py) then
            s_fsm = ST_CHASE; return
        end
        ai_updateSearch(entity, dt)
        if s_searchDone then
            s_searchDone = false; s_fsm = ST_RETURN; s_atSpawn = false
        end

    -- =======================================================================
    -- RETURN
    -- =======================================================================
    elseif s_fsm == ST_RETURN then
        if ai_playerInFront(ecx, ecy, px, py, s_dirX, s_dirY)
        or ai_hasLOS(ecx, ecy, px, py) then
            s_fsm = ST_CHASE; return
        end
        ai_moveToSpawn(entity, dt)
        if s_atSpawn then s_fsm = ST_PATROL end
    end
end