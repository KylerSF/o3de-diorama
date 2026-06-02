----------------------------------------------------------------------------------------------------
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
----------------------------------------------------------------------------------------------------

-- Tween / easing library for Diorama gameplay scripts.
--
-- A tiny, dependency-free helper for animating a value over time with easing:
-- sprite size/tint/position pops, camera moves, fades, UI slides, the kind of
-- "juice" that otherwise needs hand-rolled per-frame Lua. Pure logic, so it works
-- the same in the launcher and the editor and needs no engine services.
--
-- Usage (drive it from your component's OnTick):
--
--   local Tween = require("scripts.tween")   -- or load however your project loads shared Lua
--
--   function MyThing:OnActivate()
--       self.tweens = Tween.Group()
--       -- animate a number from 1 to 3 over 0.4s, bouncing, calling back each step
--       self.tweens:to(1.0, 3.0, 0.4, Tween.easeOutBack, function(v)
--           DioramaSpriteRequestBus.Event.SetSize(self.entityId, v, v)
--       end)
--       self.tickHandler = TickBus.Connect(self)
--   end
--
--   function MyThing:OnTick(dt) self.tweens:update(dt) end
--
-- Every easing function takes a normalized time t in 0..1 and returns the eased
-- 0..1 progress; `lerp` maps that back onto your value range.

local Tween = {}

----------------------------------------------------------------------------------------------------
-- Easing functions (t in 0..1 -> eased 0..1). Standard Penner-style curves.
----------------------------------------------------------------------------------------------------

local function clamp01(t)
    if t < 0.0 then return 0.0 elseif t > 1.0 then return 1.0 else return t end
end

function Tween.linear(t) return t end

function Tween.easeInQuad(t) return t * t end
function Tween.easeOutQuad(t) return t * (2.0 - t) end
function Tween.easeInOutQuad(t)
    if t < 0.5 then return 2.0 * t * t end
    return -1.0 + (4.0 - 2.0 * t) * t
end

function Tween.easeInCubic(t) return t * t * t end
function Tween.easeOutCubic(t) local f = t - 1.0; return f * f * f + 1.0 end

function Tween.easeInSine(t) return 1.0 - math.cos(t * math.pi * 0.5) end
function Tween.easeOutSine(t) return math.sin(t * math.pi * 0.5) end

-- Overshoot at the end (a satisfying "pop").
function Tween.easeOutBack(t)
    local c1, c3 = 1.70158, 2.70158
    local f = t - 1.0
    return 1.0 + c3 * f * f * f + c1 * f * f
end

-- Spring-like settle.
function Tween.easeOutElastic(t)
    if t <= 0.0 then return 0.0 end
    if t >= 1.0 then return 1.0 end
    local c4 = (2.0 * math.pi) / 3.0
    return math.pow(2.0, -10.0 * t) * math.sin((t * 10.0 - 0.75) * c4) + 1.0
end

-- Bouncing settle.
function Tween.easeOutBounce(t)
    local n1, d1 = 7.5625, 2.75
    if t < 1.0 / d1 then
        return n1 * t * t
    elseif t < 2.0 / d1 then
        t = t - 1.5 / d1; return n1 * t * t + 0.75
    elseif t < 2.5 / d1 then
        t = t - 2.25 / d1; return n1 * t * t + 0.9375
    else
        t = t - 2.625 / d1; return n1 * t * t + 0.984375
    end
end

-- Linear interpolation a..b by eased fraction f.
function Tween.lerp(a, b, f) return a + (b - a) * f end

----------------------------------------------------------------------------------------------------
-- A group of active tweens. Create one per owner, update it each tick, and add
-- tweens with :to(). Each tween calls onUpdate(value) every step and the optional
-- onComplete() once when it finishes. :to returns a handle you can :cancel().
----------------------------------------------------------------------------------------------------

local Group = {}
Group.__index = Group

function Tween.Group()
    return setmetatable({ active = {} }, Group)
end

function Group:to(from, to, duration, easing, onUpdate, onComplete)
    easing = easing or Tween.linear
    duration = (duration and duration > 0.0) and duration or 0.0001
    local t = { from = from, to = to, duration = duration, easing = easing,
                onUpdate = onUpdate, onComplete = onComplete, elapsed = 0.0, done = false }
    table.insert(self.active, t)
    if onUpdate then onUpdate(from) end -- apply the start value immediately
    return t
end

function Group:cancel(handle)
    for i, t in ipairs(self.active) do
        if t == handle then t.done = true; table.remove(self.active, i); return end
    end
end

function Group:clear() self.active = {} end

function Group:isIdle() return #self.active == 0 end

function Group:update(deltaTime)
    local i = 1
    while i <= #self.active do
        local t = self.active[i]
        t.elapsed = t.elapsed + deltaTime
        local f = t.easing(clamp01(t.elapsed / t.duration))
        if t.onUpdate then t.onUpdate(Tween.lerp(t.from, t.to, f)) end
        if t.elapsed >= t.duration then
            if t.onComplete then t.onComplete() end
            table.remove(self.active, i)
        else
            i = i + 1
        end
    end
end

return Tween
