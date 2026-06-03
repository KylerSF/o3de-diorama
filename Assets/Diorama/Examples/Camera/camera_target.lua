----------------------------------------------------------------------------------------------------
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
----------------------------------------------------------------------------------------------------

-- Camera demo driver. Attach to the Target entity in the 2D camera demo.
--
-- At game start it wires the demo camera through the typed DioramaCamera2DRequestBus
-- (register this entity as the follow target, set the framing offset, smoothing,
-- deadzone, and lookahead), then each tick it patrols the target left/right so the
-- camera follows, and periodically adds shake trauma so the camera kicks. This is
-- all the camera feature's verbs exercised from one small script; set the Camera
-- property to the demo's camera entity before entering game mode.

local CameraTarget = {
    Properties = {
        Camera = { default = EntityId(), description = "The demo camera entity (the one with the 2D Camera Controller)" },
        Range = { default = 12.0, description = "How far the target patrols left/right of its start, in world units" },
        Speed = { default = 3.0, description = "Patrol speed in world units per second" },
        FollowOffsetZ = { default = 28.0, description = "Camera pull-back along the out-of-plane axis (frames the scene)" },
        SmoothTime = { default = 0.25, description = "Camera follow smoothing time in seconds" },
        DeadzoneHalf = { default = 4.0, description = "Half-size of the camera deadzone (target moves this far before the camera pans)" },
        Lookahead = { default = 3.0, description = "How far the camera leads the target's motion" },
        ShakeInterval = { default = 2.5, description = "Seconds between automatic shake kicks" },
        ShakeAmount = { default = 0.6, description = "Trauma added per kick (0..1)" },
    },
}

function CameraTarget:OnActivate()
    self.start = TransformBus.Event.GetWorldTranslation(self.entityId)
    -- Guard every property with a default (a baked ScriptComponent does not apply
    -- the .lua Property defaults at runtime).
    self.camera = self.Properties.Camera
    self.range = self.Properties.Range or 12.0
    self.speed = self.Properties.Speed or 3.0
    self.shakeInterval = self.Properties.ShakeInterval or 2.5
    self.shakeAmount = self.Properties.ShakeAmount or 0.6

    self.t = 0.0
    self.shakeTimer = 0.0

    -- Configure the camera through its typed request bus. Doing it here (game time)
    -- means the runtime controller is live and connected, so no editor property
    -- wiring is needed -- the same verbs an agent or gameplay script would call.
    if self.camera ~= nil and self.camera:IsValid() then
        DioramaCamera2DRequestBus.Event.SetTarget(self.camera, self.entityId)
        DioramaCamera2DRequestBus.Event.SetFollowOffset(self.camera, 0.0, 0.0, self.Properties.FollowOffsetZ or 28.0)
        DioramaCamera2DRequestBus.Event.SetSmoothTime(self.camera, self.Properties.SmoothTime or 0.25)
        local dz = self.Properties.DeadzoneHalf or 4.0
        DioramaCamera2DRequestBus.Event.SetDeadzone(self.camera, dz, dz)
        DioramaCamera2DRequestBus.Event.SetLookahead(self.camera, self.Properties.Lookahead or 3.0)
    end

    self.tickHandler = TickBus.Connect(self)
end

function CameraTarget:OnDeactivate()
    if self.tickHandler then
        self.tickHandler:Disconnect()
    end
end

function CameraTarget:OnTick(deltaTime, scriptTime)
    -- Patrol left/right with a smooth (sine) sweep so the camera pans back and forth.
    self.t = self.t + deltaTime * self.speed * 0.15
    local x = self.start.x + math.sin(self.t) * self.range
    TransformBus.Event.SetWorldTranslation(self.entityId, Vector3(x, self.start.y, self.start.z))

    -- Periodic shake kick so the camera's trauma shake is visible.
    self.shakeTimer = self.shakeTimer + deltaTime
    if self.shakeTimer >= self.shakeInterval then
        self.shakeTimer = 0.0
        if self.camera ~= nil and self.camera:IsValid() then
            DioramaCamera2DRequestBus.Event.AddTrauma(self.camera, self.shakeAmount)
        end
    end
end

return CameraTarget
