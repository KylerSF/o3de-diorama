----------------------------------------------------------------------------------------------------
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
----------------------------------------------------------------------------------------------------

-- Parallax layer helper for 2.5D Diorama scenes.
--
-- Attach to a layer entity (a sprite, a tilemap, or a parent of several) and set
-- its Camera target and a ParallaxFactor. As the camera moves, the layer is
-- offset by the camera's movement scaled by (1 - factor):
--
--   factor = 1  -> offset 0       : the layer sits in the world (foreground)
--   factor = 0  -> offset = delta : the layer follows the camera (far background,
--                                    appears static on screen)
--   0 < f < 1   -> partial follow : midground, moves slower than the foreground
--
-- Layering itself (which sprite draws on top within a depth) is handled by the
-- Diorama sprite/tilemap Sort Offset; parallax is this camera-relative motion on
-- top of that.

local ParallaxLayer = {
    Properties = {
        Camera = { default = EntityId(), description = "Entity the parallax is relative to (usually the camera)" },
        ParallaxFactor = { default = 0.5, description = "0 = far background (follows camera), 1 = foreground (fixed in world)" },
    },
}

function ParallaxLayer:OnActivate()
    -- Capture the layer's authored position and the camera's starting position so
    -- offsets are measured as deltas from the initial framing.
    self.basePosition = TransformBus.Event.GetWorldTranslation(self.entityId)
    self.haveCameraStart = false
    self.cameraStart = Vector3(0.0, 0.0, 0.0)
    self.tickHandler = TickBus.Connect(self)
end

function ParallaxLayer:OnDeactivate()
    if self.tickHandler then
        self.tickHandler:Disconnect()
    end
end

function ParallaxLayer:OnTick(deltaTime, scriptTime)
    local camera = self.Properties.Camera
    if camera == nil or not camera:IsValid() then
        return
    end

    local cameraPos = TransformBus.Event.GetWorldTranslation(camera)
    if not self.haveCameraStart then
        self.cameraStart = cameraPos
        self.haveCameraStart = true
    end

    local factor = self.Properties.ParallaxFactor
    local follow = 1.0 - factor
    local offsetX = (cameraPos.x - self.cameraStart.x) * follow
    local offsetY = (cameraPos.y - self.cameraStart.y) * follow
    local offsetZ = (cameraPos.z - self.cameraStart.z) * follow

    TransformBus.Event.SetWorldTranslation(
        self.entityId,
        Vector3(self.basePosition.x + offsetX, self.basePosition.y + offsetY, self.basePosition.z + offsetZ))
end

return ParallaxLayer
