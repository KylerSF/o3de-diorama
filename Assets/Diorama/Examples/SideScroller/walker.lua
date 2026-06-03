----------------------------------------------------------------------------------------------------
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
----------------------------------------------------------------------------------------------------

-- Side-scroller showcase walker. Attach to the Player entity.
--
-- At game start it registers itself as the camera's follow target and configures
-- the camera for a side-scroll feel (follow X, hold Y), all through the typed
-- DioramaCamera2DRequestBus. Then it walks the player rightward at a constant
-- speed, looping back to the start, so the camera pans, the parallax background
-- layers scroll at their depths, and the player passes through the torches' light
-- and ember particles. One small script drives the whole composed scene.

local Walker = {
    Properties = {
        Camera = { default = EntityId(), description = "The side-scroller camera entity (has the 2D Camera Controller)" },
        Speed = { default = 5.0, description = "Walk speed in world units per second" },
        LoopDistance = { default = 60.0, description = "Distance walked before looping back to the start" },
        FollowOffsetZ = { default = 30.0, description = "Camera pull-back along the out-of-plane axis" },
    },
}

function Walker:OnActivate()
    self.start = TransformBus.Event.GetWorldTranslation(self.entityId)
    self.camera = self.Properties.Camera
    self.speed = self.Properties.Speed or 5.0
    self.loop = self.Properties.LoopDistance or 60.0
    self.x = 0.0

    if self.camera ~= nil and self.camera:IsValid() then
        DioramaCamera2DRequestBus.Event.SetTarget(self.camera, self.entityId)
        DioramaCamera2DRequestBus.Event.SetFollowOffset(self.camera, 0.0, 0.0, self.Properties.FollowOffsetZ or 30.0)
        DioramaCamera2DRequestBus.Event.SetSmoothTime(self.camera, 0.18)
        -- Follow horizontally, hold vertically (large Y deadzone) for a side-scroll feel.
        DioramaCamera2DRequestBus.Event.SetDeadzone(self.camera, 1.0, 100.0)
        DioramaCamera2DRequestBus.Event.SetLookahead(self.camera, 2.0)
    end

    self.tickHandler = TickBus.Connect(self)
end

function Walker:OnDeactivate()
    if self.tickHandler then
        self.tickHandler:Disconnect()
    end
end

function Walker:OnTick(deltaTime, scriptTime)
    self.x = self.x + self.speed * deltaTime
    if self.x > self.loop then
        self.x = 0.0
    end
    TransformBus.Event.SetWorldTranslation(self.entityId, Vector3(self.start.x + self.x, self.start.y, self.start.z))
end

return Walker
