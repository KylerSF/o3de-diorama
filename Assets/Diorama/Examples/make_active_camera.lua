-- make_active_camera.lua
--
-- Put this on a camera entity (an Atom Camera component) in any level you want to
-- render in the standalone GameLauncher. It calls CameraRequestBus MakeActiveView,
-- which designates this camera as the active render view. Without it, only the
-- project's startup/default level camera is auto-activated, so a freshly authored or
-- copied level renders empty/gray even though it loaded fine.
--
-- MakeActiveView is called in OnActivate (not deferred to a tick): in a level whose
-- only renderable content is driven by per-tick logic (e.g. a particle emitter that
-- acquires its feature processor on its first tick), nothing bootstraps the render
-- loop until a view is active, and a view never activates if we wait for a tick that
-- never comes. Activating the view up front breaks that deadlock. A short tick
-- handler re-asserts it for the first few frames in case the camera/scene was not
-- fully ready at OnActivate, then disconnects.
--
-- Optional properties let it double as a clean 2D framing helper: tick Orthographic
-- on and set the half-width to frame the scene in a flat, pixel-friendly projection.
local MakeActiveCamera = {
    Properties = {
        Orthographic = { default = false },
        OrthographicHalfWidth = { default = 10.0 },
    },
}

function MakeActiveCamera:Apply()
    if self.Properties.Orthographic then
        CameraRequestBus.Event.SetOrthographic(self.entityId, true)
        CameraRequestBus.Event.SetOrthographicHalfWidth(self.entityId, self.Properties.OrthographicHalfWidth)
    end
    CameraRequestBus.Event.MakeActiveView(self.entityId)
end

function MakeActiveCamera:OnActivate()
    self:Apply()
    self.ticks = 0
    self.tickHandler = TickBus.Connect(self)
end

function MakeActiveCamera:OnDeactivate()
    if self.tickHandler ~= nil then
        self.tickHandler:Disconnect()
        self.tickHandler = nil
    end
end

function MakeActiveCamera:OnTick(deltaTime, timePoint)
    -- Re-assert for a few frames, then stop (the active view persists once it sticks).
    self:Apply()
    self.ticks = self.ticks + 1
    if self.ticks >= 3 then
        self.tickHandler:Disconnect()
        self.tickHandler = nil
    end
end

return MakeActiveCamera
