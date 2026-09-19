local player = nil

local ALERT_RANGE = 480.0
local CHASE_SPEED = 100.0
local PATROL_SPEED = 96.0
local PATROL_INTERVAL = 2.0

local state = "patrol"
local patrolDirX = 1.0
local patrolDirY = 0.0
local patrolTimer = 0.0
local lastX = 0.0
local lastY = 0.0
local stuckTimer = 0.0

local function wouldOverlapPlayer(nextX, nextY, px, py, ts)
    local enemyCx = nextX + ts * 0.5
    local enemyCy = nextY + ts * 0.5
    local playerCx = px + ts * 0.5
    local playerCy = py + ts * 0.5

    local minDist = ts * 0.5
    local dx = enemyCx - playerCx
    local dy = enemyCy - playerCy
    return (dx * dx + dy * dy) < (minDist * minDist)
end

local function safeHasLOS(ax, ay, bx, by)
    if type(AI_TileSize) == "function" then
        local ts = tonumber(AI_TileSize()) or 0.0
        if ts <= 0.0 then
            return true
        end
    end

    if type(AI_HasLOS) == "function" then
        return AI_HasLOS(ax, ay, bx, by)
    end
    return true
end

local function safeIsBlocked(wx, wy)
    if type(AI_TileSize) == "function" then
        local ts = tonumber(AI_TileSize()) or 0.0
        if ts <= 0.0 then
            return false
        end
    end

    if type(AI_IsBlocked) == "function" then
        return AI_IsBlocked(wx, wy)
    end
    return false
end

local function getTileSize()
    local ts = 32.0
    if type(AI_TileSize) == "function" then
        local t = tonumber(AI_TileSize()) or 0.0
        if t > 0.0 then
            ts = t
        end
    end
    return ts
end

local function safeBoxBlocked(wx, wy)
    local ts = getTileSize()
    local inset = 2.0
    local x1 = wx + inset
    local y1 = wy + inset
    local x2 = wx + ts - inset
    local y2 = wy + ts - inset

    if safeIsBlocked(x1, y1) then return true end
    if safeIsBlocked(x2, y1) then return true end
    if safeIsBlocked(x1, y2) then return true end
    if safeIsBlocked(x2, y2) then return true end
    return false
end

-- Moves by SetPos only (no AI_Move). Axis-separated collision.
local function safeMove(e, vx, vy, dt)
    vx = tonumber(vx) or 0.0
    vy = tonumber(vy) or 0.0

    -- dt normalization inline (NO helper function calls)
    dt = tonumber(dt) or 0.0
    if dt > 1.0 then dt = dt * 0.001 end
    if dt > 0.05 then dt = 0.05 end

    local ex, ey = GetPos(e)
    ex = tonumber(ex) or 0.0
    ey = tonumber(ey) or 0.0

    local nx = ex + vx * dt
    if not safeBoxBlocked(nx, ey) then
        ex = nx
    end

    local ny = ey + vy * dt
    if not safeBoxBlocked(ex, ny) then
        ey = ny
    end

    SetPos(e, ex, ey)
end

function OnStart(e)
    player = FindPlayer()
    state = "patrol"
    patrolDirX = 1.0
    patrolDirY = 0.0
    patrolTimer = 0.0
    local ex, ey = GetPos(e)
    lastX = tonumber(ex) or 0.0
    lastY = tonumber(ey) or 0.0
    stuckTimer = 0.0
end

local function ensurePlayer()
    if player == nil then
        player = FindPlayer()
    end
    return player ~= nil
end

local function updatePatrol(e, dt)
    -- dt normalization inline (NO helper function calls)
    dt = tonumber(dt) or 0.0
    if dt > 1.0 then dt = dt * 0.001 end
    if dt > 0.05 then dt = 0.05 end

    patrolTimer = (tonumber(patrolTimer) or 0.0) + dt
    local interval = tonumber(PATROL_INTERVAL) or 0.0
    if interval > 0.0 and patrolTimer >= interval then
        patrolTimer = patrolTimer - interval
        patrolDirX = -patrolDirX
        patrolDirY = -patrolDirY
    end

    local ex, ey = GetPos(e)
    ex = tonumber(ex) or 0.0
    ey = tonumber(ey) or 0.0

    local speed = tonumber(PATROL_SPEED) or 0.0
    local stepX = patrolDirX * speed * dt
    local stepY = patrolDirY * speed * dt

    local nx = ex + stepX
    local ny = ey + stepY

    if safeBoxBlocked(nx, ny) then
        local alt1x = -patrolDirY
        local alt1y = patrolDirX
        local ax1 = ex + alt1x * speed * dt
        local ay1 = ey + alt1y * speed * dt
        if not safeBoxBlocked(ax1, ay1) then
            patrolDirX = alt1x
            patrolDirY = alt1y
            safeMove(e, alt1x * speed, alt1y * speed, dt)
            return
        end

        local alt2x = patrolDirY
        local alt2y = -patrolDirX
        local ax2 = ex + alt2x * speed * dt
        local ay2 = ey + alt2y * speed * dt
        if not safeBoxBlocked(ax2, ay2) then
            patrolDirX = alt2x
            patrolDirY = alt2y
            safeMove(e, alt2x * speed, alt2y * speed, dt)
            return
        end

        patrolDirX = -patrolDirX
        patrolDirY = -patrolDirY
    else
        safeMove(e, patrolDirX * speed, patrolDirY * speed, dt)
    end
end

function OnUpdate(e, dt)
    -- dt normalization inline (NO helper function calls)
    dt = tonumber(dt) or 0.0
    if dt > 1.0 then dt = dt * 0.001 end
    if dt > 0.05 then dt = 0.05 end

    if not ensurePlayer() then
        state = "patrol"
        updatePatrol(e, dt)
        return
    end

    local ts = getTileSize()

    local ex, ey = GetPos(e)
    ex = tonumber(ex) or 0.0
    ey = tonumber(ey) or 0.0

    local cx = ex + ts * 0.5
    local cy = ey + ts * 0.5

    local px, py = GetPos(player)
    px = tonumber(px) or 0.0
    py = tonumber(py) or 0.0

    local pcx = px + ts * 0.5
    local pcy = py + ts * 0.5

    local dx = pcx - cx
    local dy = pcy - cy
    local dist2 = dx * dx + dy * dy

    local alert = tonumber(ALERT_RANGE) or 0.0
    local alert2 = alert * alert

    local hasLos = safeHasLOS(cx, cy, pcx, pcy)

    local dxMove = ex - lastX
    local dyMove = ey - lastY
    local moveSq = dxMove * dxMove + dyMove * dyMove
    if moveSq < 1.0 then
        stuckTimer = stuckTimer + dt
    else
        stuckTimer = 0.0
    end
    lastX = ex
    lastY = ey

    if stuckTimer > 1.0 then
        local choice = math.random(4)
        if choice == 1 then
            patrolDirX, patrolDirY = 1.0, 0.0
        elseif choice == 2 then
            patrolDirX, patrolDirY = -1.0, 0.0
        elseif choice == 3 then
            patrolDirX, patrolDirY = 0.0, 1.0
        else
            patrolDirX, patrolDirY = 0.0, -1.0
        end
        state = "patrol"
        stuckTimer = 0.0
    end

    if state == "patrol" then
        if dist2 <= alert2 and hasLos then
            state = "chase"
        else
            updatePatrol(e, dt)
            return
        end
    end

    if state == "chase" then
        if dist2 > alert2 or not hasLos then
            state = "patrol"
            updatePatrol(e, dt)
            return
        end

        local dist = math.sqrt(dist2)
        if dist < 1e-3 then
            return
        end

        local dirX = dx / dist
        local dirY = dy / dist

        local speed = tonumber(CHASE_SPEED) or 0.0
        local stepX = dirX * speed * dt
        local stepY = dirY * speed * dt

        local nx = ex + stepX
        local ny = ey + stepY

        -- Removed wouldOverlapPlayer to allow the enemy to continuously walk into the player and trigger collision damage.
        -- The physics system now handles dynamic pushback to prevent wall pinning.

        if safeBoxBlocked(nx, ny) then
            if not safeBoxBlocked(ex + stepX, ey) then
                safeMove(e, dirX * speed, 0.0, dt)
            elseif not safeBoxBlocked(ex, ey + stepY) then
                safeMove(e, 0.0, dirY * speed, dt)
            else
                state = "patrol"
                updatePatrol(e, dt)
                return
            end
        else
            safeMove(e, dirX * speed, dirY * speed, dt)
        end

        -- local r = math.atan(dy, dx)
        -- SetRot(e, r)
    end
end
