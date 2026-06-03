-- camera_pan.lua
--
-- Demo driver for the parallax level. Makes its entity the active render view, then
-- slowly pans it back and forth along X. The DioramaParallaxComponent on the
-- background layers measures their offset against this camera's motion, so panning
-- is what makes the depth separation (parallax) visible.
local CameraPan = {
    Properties = {
        Range = { default = 14.0, description = "Half-distance the camera pans along X" },
        Speed = { default = 0.5, description = "Pan oscillation speed (radians/sec)" },
    },
}

function CameraPan:OnActivate()
    -- Cache properties with fallbacks: when the script is attached without its
    -- properties pushed (e.g. authored by hand), self.Properties.* is nil.
    self.range = self.Properties.Range or 14.0
    self.speed = self.Properties.Speed or 0.5
    CameraRequestBus.Event.MakeActiveView(self.entityId)
    self.start = TransformBus.Event.GetWorldTranslation(self.entityId)
    self.t = 0.0
    self.tickHandler = TickBus.Connect(self)
end

function CameraPan:OnDeactivate()
    if self.tickHandler then
        self.tickHandler:Disconnect()
        self.tickHandler = nil
    end
end

function CameraPan:OnTick(deltaTime, scriptTime)
    self.t = self.t + deltaTime
    local x = math.sin(self.t * self.speed) * self.range
    TransformBus.Event.SetWorldTranslation(self.entityId, Vector3(self.start.x + x, self.start.y, self.start.z))
end

return CameraPan
