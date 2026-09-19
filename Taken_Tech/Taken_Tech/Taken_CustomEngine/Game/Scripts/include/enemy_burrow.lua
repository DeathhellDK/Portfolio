--[[
* @file     enemy_burrow.lua
* @author   Lim Zhi Jie
* @email    zhijie.lim@digipen.edu
* @date     2026-03-11
*
* @brief Implements the Burrow enemy AI behavior (whack-a-mole underground attack).
*
* This script defines the state machine and attack sequence for the burrow
* enemy type. When the player enters burrow range, the enemy locks onto
* the player's position, telegraphs the attack, disappears underground for a
* travel period, then teleports via SetPos to emerge directly on the player.
* After the strike window expires, the enemy enters a cooldown phase during
* which it slowly retreats from the player before resurfacing. Movement
* during above-ground phases uses axis-separated collision via safeMove.
*
* Burrow phases: ABOVE_GROUND → TELEGRAPHING → BURROWED → STRIKING → COOLDOWN
* States: PATROL → CHASE → ATTACK → SEARCH → RETURN → PATROL
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the
* prior written consent of DigiPen Institute of Technology is prohibited.
*]]

-- =============================================================================
-- enemy_burrow.lua  —  Burrow enemy (whack-a-mole underground attack)
-- Movement uses SetPos directly (same pattern as chase.lua).
-- =============================================================================

BURROW_RANGE_TILES  = 2.0
BURROW_SPEED        = 80.0
TELEGRAPH_DUR       = 0.8
BURROW_TRAVEL_TIME  = 2.0   -- now used as a surface-proximity threshold timer
STRIKE_DUR          = 1.2
COOLDOWN_TIME       = 2.5
HIT_RADIUS_FACTOR   = 0.9
PATROL_SPEED        = 96.0
CHASE_SPEED         = 150.0
UNDERGROUND_SPEED   = 130.0  -- speed while burrowed-chasing
SEARCH_SPEED        = 60.0
VISION_TILES        = 3.0
SURFACE_RANGE_TILES = 1.2    -- how close underground before resurfacing

ANIM_PATROL         = 0
ANIM_ENTER_BURROW   = 1
ANIM_MOVE_BURROWED  = 2
ANIM_EXIT_BURROW    = 3

ST_PATROL = "PATROL"
ST_CHASE  = "CHASE"
ST_SEARCH = "SEARCH"
ST_RETURN = "RETURN"
ST_ATTACK = "ATTACK"

BP_ABOVE  = "ABOVE_GROUND"
BP_TELE   = "TELEGRAPHING"
BP_BURIED = "BURROWED"
BP_STRIKE = "STRIKING"
BP_COOL   = "COOLDOWN"

-- ---------------------------------------------------------------------------
-- Helpers (unchanged)
-- ---------------------------------------------------------------------------
function normDt(dt)
    dt=tonumber(dt) or 0.0;if dt>1.0 then dt=dt*0.001 end;if dt>0.05 then dt=0.05 end;return dt
end
function getTileSize()
    local ts=100.0;if type(AI_TileSize)=="function" then local t=tonumber(AI_TileSize()) or 0.0;if t>0.0 then ts=t end end;return ts
end
function isBlocked(wx,wy) if type(AI_IsBlocked)=="function" then return AI_IsBlocked(wx,wy) end;return false end
function boxBlocked(wx, wy)
    local ts = getTileSize(); local i = 2.0
    if isBlocked(wx+i,    wy+i)    then return true end
    if isBlocked(wx+ts-i, wy+i)    then return true end
    if isBlocked(wx+i,    wy+ts-i) then return true end
    if isBlocked(wx+ts-i, wy+ts-i) then return true end
    -- left and right edge probes only, at mid-height
    if isBlocked(wx,    wy+ts*0.5) then return true end
    if isBlocked(wx+ts, wy+ts*0.5) then return true end
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
    return ex, ey  -- just add this line
end
function ai_dist(ax,ay,bx,by) local dx=bx-ax;local dy=by-ay;return math.sqrt(dx*dx+dy*dy) end
function ai_norm(dx,dy) local l=math.sqrt(dx*dx+dy*dy);if l<0.001 then return 0,0 end;return dx/l,dy/l end
function ai_hasLOS(ax,ay,bx,by) if type(AI_HasLOS)=="function" then return AI_HasLOS(ax,ay,bx,by) end;return true end
function ai_playerInFront(ecx,ecy,px,py,dX,dY)
    local ts=getTileSize();local dx=px-ecx;local dy=py-ecy;local mag=ai_dist(ecx,ecy,px,py)
    if mag<0.001 or mag>VISION_TILES*ts then return false end
    if (dx*dX+dy*dY)/mag<0.7071 then return false end;return ai_hasLOS(ecx,ecy,px,py)
end
function ai_distToPlayer(entity,px,py)
    local ts=getTileSize();local ex,ey=GetPos(entity);return ai_dist(ex+ts*0.5,ey+ts*0.5,px+ts*0.25,py+ts*0.5)
end
function ai_updatePatrol(entity, dt)
    local ts = getTileSize()
    local ex, ey = GetPos(entity)  -- read ONCE
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
    local ts=getTileSize();local ex,ey=GetPos(entity);s_lastKnownPX=px;s_lastKnownPY=py
    local cx=ex+ts*0.5;local cy=ey+ts*0.5;local dx=(px+ts*0.5)-cx;local dy=(py+ts*0.5)-cy
    local dist=math.sqrt(dx*dx+dy*dy);if dist<1e-3 then return end
    local dX=dx/dist;local dY=dy/dist;local speed=CHASE_SPEED
    if math.abs(dx)>math.abs(dy) then s_dirX=dx>0 and 1 or -1;s_dirY=0 else s_dirX=0;s_dirY=dy>0 and 1 or -1 end
    local sx=dX*speed*dt;local sy=dY*speed*dt
    if boxBlocked(ex+sx,ey+sy) then
        if not boxBlocked(ex+sx,ey) then safeMove(entity,dX*speed,0,dt)
        elseif not boxBlocked(ex,ey+sy) then safeMove(entity,0,dY*speed,dt) end
    else safeMove(entity,dX*speed,dY*speed,dt) end
end
function ai_updateSearch(entity,dt)
    local ts=getTileSize();local ex,ey=GetPos(entity)
    local geX=math.floor(ex/ts);local geY=math.floor(ey/ts)
    local gtX=math.floor((s_lastKnownPX or ex)/ts);local gtY=math.floor((s_lastKnownPY or ey)/ts)
    if s_hasLastSeen then
        if geX==gtX and geY==gtY then s_hasLastSeen=false;s_searchTimer=0;s_searchPhase=0;return end
        local ddx=gtX-geX;local ddy=gtY-geY;local nX=geX;local nY=geY
        if math.abs(ddx)>=math.abs(ddy) then nX=geX+(ddx>0 and 1 or -1) else nY=geY+(ddy>0 and 1 or -1) end
        if not isBlocked(nX*ts+ts*0.5,nY*ts+ts*0.5) then
            local nx2,ny2=ai_norm(nX*ts+ts*0.5-(ex+ts*0.5),nY*ts+ts*0.5-(ey+ts*0.5));safeMove(entity,nx2*SEARCH_SPEED,ny2*SEARCH_SPEED,dt)
        else
            local aX=geX;local aY=geY
            if math.abs(ddx)>=math.abs(ddy) then aY=geY+(ddy>0 and 1 or -1) else aX=geX+(ddx>0 and 1 or -1) end
            if not isBlocked(aX*ts+ts*0.5,aY*ts+ts*0.5) then
                local nx2,ny2=ai_norm(aX*ts+ts*0.5-(ex+ts*0.5),aY*ts+ts*0.5-(ey+ts*0.5));safeMove(entity,nx2*SEARCH_SPEED,ny2*SEARCH_SPEED,dt)
            else
                s_hasLastSeen=false;s_searchTimer=0;s_searchPhase=0
            end
        end
        return
    end
    s_searchTimer=(s_searchTimer or 0)+dt
    local dirs={{1,0},{0,1},{-1,0},{0,-1}};local ph=s_searchPhase or 0
    if s_searchTimer>(ph+1)*0.5 then ph=ph+1;s_searchPhase=ph;if ph>=4 then s_searchDone=true end end
    if ph<4 then s_dirX=dirs[ph+1][1];s_dirY=dirs[ph+1][2] end
end
function ai_moveToSpawn(entity,dt)
    local ts=getTileSize();local ex,ey=GetPos(entity)
    local ecx=ex+ts*0.5;local ecy=ey+ts*0.5
    local scx=(s_spawnX or ex)+ts*0.5;local scy=(s_spawnY or ey)+ts*0.5
    local dX=scx-ecx;local dY=scy-ecy;s_atSpawn=false
    if dX*dX+dY*dY<(ts*0.5)*(ts*0.5) then
        s_atSpawn=true;s_movingToTile=false
        s_dirX=1;s_dirY=0  -- reset to default patrol direction
        SetPos(entity,s_spawnX or ex,s_spawnY or ey);return
    end
    local dist=math.sqrt(dX*dX+dY*dY);local ndx=dX/dist;local ndy=dY/dist;local speed=PATROL_SPEED
    if math.abs(dX)>math.abs(dY) then s_dirX=dX>0 and 1 or -1;s_dirY=0 else s_dirX=0;s_dirY=dY>0 and 1 or -1 end

    local ax, ay = safeMove(entity,ndx*speed,ndy*speed,dt)
    local stuck = (math.abs(dX)>math.abs(dY) and math.abs(ax-ex)<0.5)
               or (math.abs(dY)>math.abs(dX) and math.abs(ay-ey)<0.5)
    if stuck then
        -- try sliding along the other axis
        if math.abs(dX)>math.abs(dY) then
            safeMove(entity,0,ndy*speed,dt)
        else
            safeMove(entity,ndx*speed,0,dt)
        end
    end
end

-- ---------------------------------------------------------------------------
-- Sprite helpers  ← INSERT THESE 4 FUNCTIONS HERE
-- ---------------------------------------------------------------------------

-- Called while patrolling / above-ground walking
function anim_setPatrol(entity)
    if type(SetAnimationRow)=="function" then SetAnimationRow(entity, ANIM_PATROL) end
end

-- Called at the START of BP_TELE (enemy crouches / digs in)
function anim_setEnterBurrow(entity)
    if type(SetAnimationRow)=="function" then SetAnimationRow(entity, ANIM_ENTER_BURROW) end
end

-- Called at the START of BP_BURIED (fin moving underground)
function anim_setMoveBurrowed(entity)
    if type(SetAnimationRow)=="function" then SetAnimationRow(entity, ANIM_MOVE_BURROWED) end
end

-- Called at the START of BP_STRIKE (enemy erupts from ground)
function anim_setExitBurrow(entity)
    if type(SetAnimationRow)=="function" then SetAnimationRow(entity, ANIM_EXIT_BURROW) end
end

-- ===========================================================================
-- OnStart
-- ===========================================================================
function OnStart(entity)
    local ts=getTileSize();local ex,ey=GetPos(entity)
    s_fsm=ST_PATROL;s_dirX=1;s_dirY=0
    s_spawnX=ex;s_spawnY=ey;s_patrolRange=5
    s_lastKnownPX=ex;s_lastKnownPY=ey
    s_hasLastSeen=false;s_searchTimer=0;s_searchPhase=0;s_searchDone=false
    s_atSpawn=false;s_movingToTile=false
    s_burrowPhase=BP_ABOVE;s_burrowTimer=0
    s_strikeX=0;s_strikeY=0;s_isBurrowed=false;s_burrowCooldown=0
    s_tileSize=ts

    -- ► SPRITE: start in patrol animation, collider on
    anim_setPatrol(entity)
    if type(SetColliderEnabled)=="function" then SetColliderEnabled(entity, true) end
end

-- ===========================================================================
-- OnUpdate
-- ===========================================================================
function OnUpdate(entity, dt)
    dt=normDt(dt)
    if s_fsm==nil then OnStart(entity) end
    local player=FindPlayer();if player==nil then return end
    local px,py=GetPos(player);px=tonumber(px) or 0.0;py=tonumber(py) or 0.0
    local ex,ey=GetPos(entity);ex=tonumber(ex) or 0.0;ey=tonumber(ey) or 0.0
    local ts=s_tileSize or getTileSize()
    local ecx=ex+ts*0.5;local ecy=ey+ts*0.5
    local pcx=px+ts*0.25;local pcy=py+ts*0.5
    local bRange=BURROW_RANGE_TILES*ts
    local surfaceRange=SURFACE_RANGE_TILES*ts

    -- -----------------------------------------------------------------------
    if s_fsm==ST_PATROL then
        -- ► SPRITE: patrol walk animation
        anim_setPatrol(entity)
        local distToPlayer = ai_distToPlayer(entity,px,py)
        if ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) or (distToPlayer < VISION_TILES*ts and ai_hasLOS(ecx,ecy,px,py)) then
            s_fsm=ST_CHASE;return
        end
        ai_updatePatrol(entity,dt)

    elseif s_fsm==ST_CHASE then
        -- ► SPRITE: patrol/chase animation (same above-ground row)
        anim_setPatrol(entity)
        if not ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) and not ai_hasLOS(ecx,ecy,px,py) then
            s_fsm=ST_SEARCH;s_searchTimer=0;s_searchPhase=0;s_searchDone=false;s_hasLastSeen=true;return
        end
        if ai_distToPlayer(entity,px,py)<=bRange then
            s_fsm=ST_ATTACK;s_burrowPhase=BP_ABOVE;s_burrowTimer=0;s_isBurrowed=false;return
        end
        ai_updateChase(entity,dt,px,py)

    elseif s_fsm==ST_SEARCH then
        anim_setPatrol(entity)
        local distToPlayer = ai_distToPlayer(entity,px,py)
        if ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) or ai_hasLOS(ecx,ecy,px,py) or distToPlayer < VISION_TILES*ts then
            s_fsm=ST_CHASE;return
        end
        ai_updateSearch(entity,dt)
        if s_searchDone then s_searchDone=false;s_fsm=ST_RETURN;s_movingToTile=false;s_atSpawn=false end

    elseif s_fsm==ST_RETURN then
        anim_setPatrol(entity)
        if ai_playerInFront(ecx,ecy,px,py,s_dirX,s_dirY) then s_fsm=ST_CHASE;return end
        ai_moveToSpawn(entity,dt);if s_atSpawn then s_fsm=ST_PATROL end

    elseif s_fsm==ST_ATTACK then
        local dist=ai_distToPlayer(entity,px,py)
        local dx=pcx-ecx;local dy=pcy-ecy

        -- -------------------------------------------------------------------
        if s_burrowPhase==BP_ABOVE then
            -- ► SPRITE: above-ground idle/approach
            anim_setPatrol(entity)

            if not ai_hasLOS(ecx,ecy,px,py) then
                s_fsm=ST_SEARCH;s_searchTimer=0;s_searchPhase=0;s_searchDone=false;s_hasLastSeen=true;return
            end
            if s_burrowCooldown>0 then s_burrowCooldown=s_burrowCooldown-dt end
            if dist>bRange then
                local ndx,ndy=ai_norm(dx,dy);local speed=BURROW_SPEED
                if boxBlocked(ecx+ndx*speed*dt,ecy+ndy*speed*dt) then
                    if not boxBlocked(ecx+ndx*speed*dt,ecy) then safeMove(entity,ndx*speed,0,dt)
                    elseif not boxBlocked(ecx,ecy+ndy*speed*dt) then safeMove(entity,0,ndy*speed,dt) end
                else safeMove(entity,ndx*speed,ndy*speed,dt) end
                if math.abs(dx)>math.abs(dy) then s_dirX=dx>0 and 1 or -1;s_dirY=0 else s_dirX=0;s_dirY=dy>0 and 1 or -1 end
            elseif s_burrowCooldown<=0 then
                -- Transition into telegraph (dig-in animation)
                s_burrowTimer=TELEGRAPH_DUR
                s_burrowPhase=BP_TELE
                s_isBurrowed=false
                -- ► SPRITE: play enter-burrow animation
                anim_setEnterBurrow(entity)
            end

        -- -------------------------------------------------------------------
        elseif s_burrowPhase==BP_TELE then
            s_burrowTimer=s_burrowTimer-dt
            if s_burrowTimer<=0 then
                s_burrowPhase=BP_BURIED
                s_burrowTimer=BURROW_TRAVEL_TIME  -- ← ADD THIS LINE
                s_isBurrowed=true
                anim_setMoveBurrowed(entity)
                if type(SetColliderEnabled)=="function" then SetColliderEnabled(entity, false) end
            end

        -- -------------------------------------------------------------------
        elseif s_burrowPhase==BP_BURIED then
            anim_setMoveBurrowed(entity)
            s_burrowTimer = s_burrowTimer - dt

            local bex, bey = GetPos(entity)
            bex = tonumber(bex) or 0.0; bey = tonumber(bey) or 0.0
            local bdx = (px + ts*0.5) - (bex + ts*0.5)
            local bdy = (py + ts*0.5) - (bey + ts*0.5)
            local bDist = math.sqrt(bdx*bdx + bdy*bdy)

            if bDist > 1.0 then
                local ndx, ndy = ai_norm(bdx, bdy)
                local nx = bex + ndx * UNDERGROUND_SPEED * dt
                if not AI_IsWall(nx + ts*0.5, bey + ts*0.5) then bex = nx end
                local ny = bey + ndy * UNDERGROUND_SPEED * dt
                if not AI_IsWall(bex + ts*0.5, ny + ts*0.5) then bey = ny end
                SetPos(entity, bex, bey)
            end

            if s_burrowTimer <= 0 then
                s_isBurrowed = false
                s_burrowTimer = STRIKE_DUR
                s_burrowPhase = BP_STRIKE
                anim_setExitBurrow(entity)
                if type(SetColliderEnabled)=="function" then SetColliderEnabled(entity, true) end
            end

        -- -------------------------------------------------------------------
        elseif s_burrowPhase==BP_STRIKE then
            -- ► SPRITE: exit-burrow / strike animation (already playing)
            s_burrowTimer=s_burrowTimer-dt
            if s_burrowTimer<=0 then
                s_burrowTimer=COOLDOWN_TIME
                s_burrowPhase=BP_COOL
                -- ► SPRITE: back to patrol/idle row during cooldown
                anim_setPatrol(entity)
            end

        -- -------------------------------------------------------------------
        elseif s_burrowPhase==BP_COOL then
            -- ► SPRITE: patrol/idle while retreating
            anim_setPatrol(entity)

            s_burrowTimer=s_burrowTimer-dt
            if s_burrowTimer<=0 then
                s_burrowPhase=BP_ABOVE;s_burrowCooldown=0;return
            end
            if dist<bRange*0.5 and dist>0.001 then
                local ndx,ndy=ai_norm(-dx,-dy);local slow=BURROW_SPEED*0.3
                if not boxBlocked(ecx+ndx*slow*dt, ecy+ndy*slow*dt) then safeMove(entity,ndx*slow,ndy*slow,dt) end
            end
        end
    end
end

function burrowMove(e, vx, vy, dt)
    local ts = getTileSize()
    local ex, ey = GetPos(e)
    ex = tonumber(ex) or 0.0; ey = tonumber(ey) or 0.0

    -- Get player position directly inside the function
    local player = FindPlayer()
    if player == nil then
        safeMove(e, vx, vy, dt)
        return
    end
    local lpx, lpy = GetPos(player)
    lpx = tonumber(lpx) or 0.0; lpy = tonumber(lpy) or 0.0

    local ptx = math.floor(lpx / ts)
    local pty = math.floor(lpy / ts)

    local function burrowBlocked(wx, wy)
        local cx = math.floor((wx + ts*0.5) / ts)
        local cy = math.floor((wy + ts*0.5) / ts)
        if cx == ptx and cy == pty then return false end
        return boxBlocked(wx, wy)
    end

    local nx = ex + vx * dt
    if not burrowBlocked(nx, ey) then ex = nx end
    local ny = ey + vy * dt
    if not burrowBlocked(ex, ny) then ey = ny end
    SetPos(e, ex, ey)
end