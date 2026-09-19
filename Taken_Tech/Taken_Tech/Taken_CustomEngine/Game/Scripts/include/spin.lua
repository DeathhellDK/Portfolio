local player = -1
local t = 0.0

local ALERT_RANGE = 320.0
local TURN_SMOOTH = 10.0
local IDLE_SWAY = 0.25

local function wrapPi(a)
  a = tonumber(a) or 0.0
  while a > 3.14159265 do a = a - 6.2831853 end
  while a < -3.14159265 do a = a + 6.2831853 end
  return a
end

local function lerpAngle(a, b, k, dt)
  a = tonumber(a) or 0.0
  b = tonumber(b) or 0.0
  k = tonumber(k) or 0.0
  dt = tonumber(dt) or 0.0
  local d = wrapPi(b - a)
  return a + d * (1.0 - math.exp(-k * dt))
end

function OnStart(e)
  player = FindPlayer()
  t = 0.0
end

function OnUpdate(e, dt)
  dt = tonumber(dt) or 0.0
  t = t + dt

  local pnum = tonumber(player)
  if player == nil or pnum == nil or pnum < 0 then
    player = FindPlayer()
    pnum = tonumber(player)
  end

  local r = GetRot(e)

  if player ~= nil and pnum ~= nil and pnum >= 0 then
    local ex, ey = GetPos(e)
    local px, py = GetPos(player)

    ex = tonumber(ex) or 0.0
    ey = tonumber(ey) or 0.0
    px = tonumber(px) or 0.0
    py = tonumber(py) or 0.0

    local dx, dy = (px - ex), (py - ey)
    local d2 = dx * dx + dy * dy

    local ar = tonumber(ALERT_RANGE) or 0.0
    if d2 < (ar * ar) then
      local target = math.atan(dy, dx)
      SetRot(e, lerpAngle(r, target, TURN_SMOOTH, dt))
      return
    end
  end

  SetRot(e, r + IDLE_SWAY * dt * math.sin(t * 1.6))
end
