-- hud_bar_pulse.lua
--
-- Drives a Diorama UI bar element's fill from script, to show that a HUD is
-- updated through the same typed request bus an agent or game uses. Attach it to a
-- 2D UI Element whose Kind is Bar: it oscillates the fill between Min and Max, the
-- way a game would call SetValue as health or a cooldown changes. This is the UI
-- parity in action -- DioramaUIRequestBus.Event.SetValue is exactly how gameplay
-- (or an AI agent, or Script Canvas) would set it.
local HudBarPulse = {
    Properties = {
        Speed = { default = 0.6, description = "Oscillation speed (radians/sec)" },
        Min = { default = 0.1, description = "Lowest fill (0..1)" },
        Max = { default = 1.0, description = "Highest fill (0..1)" },
    },
}

function HudBarPulse:OnActivate()
    self.speed = self.Properties.Speed or 0.6
    self.min = self.Properties.Min or 0.1
    self.max = self.Properties.Max or 1.0
    self.t = 0.0
    self.tickHandler = TickBus.Connect(self)
end

function HudBarPulse:OnDeactivate()
    if self.tickHandler then
        self.tickHandler:Disconnect()
        self.tickHandler = nil
    end
end

function HudBarPulse:OnTick(deltaTime, scriptTime)
    self.t = self.t + deltaTime * self.speed
    local s = 0.5 + 0.5 * math.sin(self.t) -- 0..1
    local value = self.min + (self.max - self.min) * s
    DioramaUIRequestBus.Event.SetValue(self.entityId, value)
end

return HudBarPulse
