----------------------------------------------------------------------------------------------------
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
----------------------------------------------------------------------------------------------------

-- Twin-stick player movement for the Diorama sample game.
--
-- Top-down movement in the world XY plane: WASD / left-stick drive the player
-- through PhysX (so collisions with arena walls and enemies work), and the mouse
-- / right-stick aim sets the facing direction. The player is a Diorama sprite, so
-- "facing" is expressed by flipping the sprite horizontally through the
-- DioramaSpriteRequestBus -- the same AI-native bus an agent would call, used
-- here from ordinary gameplay code.
--
-- The aim direction is published on the entity so the projectile script (added in
-- a later step) can fire toward where the player is aiming.

local TwinStickPlayer = {
    Properties = {
        MoveSpeed = { default = 8.0, description = "Movement speed in world units per second" },
        AimDeadZone = { default = 0.05, description = "Ignore aim input below this magnitude" },
    },
}

function TwinStickPlayer:OnActivate()
    self.moveX = 0.0
    self.moveY = 0.0
    self.aimX = 0.0
    self.aimY = 0.0
    -- Last non-zero aim, kept so firing has a direction even between flicks.
    self.aimDir = Vector3(1.0, 0.0, 0.0)
    self.facingLeft = false

    self.tickHandler = TickBus.Connect(self)

    self.idMoveX = InputEventNotificationId("move_x")
    self.idMoveY = InputEventNotificationId("move_y")
    self.idAimX = InputEventNotificationId("aim_x")
    self.idAimY = InputEventNotificationId("aim_y")

    self.hMoveX = InputEventNotificationBus.Connect(self, self.idMoveX)
    self.hMoveY = InputEventNotificationBus.Connect(self, self.idMoveY)
    self.hAimX = InputEventNotificationBus.Connect(self, self.idAimX)
    self.hAimY = InputEventNotificationBus.Connect(self, self.idAimY)
end

function TwinStickPlayer:OnDeactivate()
    if self.tickHandler then self.tickHandler:Disconnect() end
    if self.hMoveX then self.hMoveX:Disconnect() end
    if self.hMoveY then self.hMoveY:Disconnect() end
    if self.hAimX then self.hAimX:Disconnect() end
    if self.hAimY then self.hAimY:Disconnect() end
end

function TwinStickPlayer:OnPressed(value)
    local id = InputEventNotificationBus.GetCurrentBusId()
    if id == self.idMoveX then
        self.moveX = value
    elseif id == self.idMoveY then
        self.moveY = value
    elseif id == self.idAimX then
        self.aimX = value
    elseif id == self.idAimY then
        self.aimY = value
    end
end

function TwinStickPlayer:OnHeld(value)
    self:OnPressed(value)
end

function TwinStickPlayer:OnReleased(value)
    local id = InputEventNotificationBus.GetCurrentBusId()
    if id == self.idMoveX then
        self.moveX = 0.0
    elseif id == self.idMoveY then
        self.moveY = 0.0
    elseif id == self.idAimX then
        self.aimX = 0.0
    elseif id == self.idAimY then
        self.aimY = 0.0
    end
end

function TwinStickPlayer:OnTick(deltaTime, scriptTime)
    -- Movement: normalize so diagonals are not faster, then drive PhysX velocity
    -- in the XY plane (gravity is disabled on the player rigid body).
    local moveLen = math.sqrt(self.moveX * self.moveX + self.moveY * self.moveY)
    if moveLen > 0.1 then
        local speed = self.Properties.MoveSpeed
        local velocity = Vector3((self.moveX / moveLen) * speed, (self.moveY / moveLen) * speed, 0.0)
        RigidBodyRequestBus.Event.SetLinearVelocity(self.entityId, velocity)
    else
        RigidBodyRequestBus.Event.SetLinearVelocity(self.entityId, Vector3(0.0, 0.0, 0.0))
    end

    -- Aim: when the player aims, remember the direction and face the sprite that
    -- way. Mouse deltas arrive as one-shot per-frame values; gamepad stick values
    -- are re-sent each tick.
    local aimLen = math.sqrt(self.aimX * self.aimX + self.aimY * self.aimY)
    if aimLen > self.Properties.AimDeadZone then
        self.aimDir = Vector3(self.aimX / aimLen, self.aimY / aimLen, 0.0)
        local shouldFaceLeft = self.aimX < 0.0
        if shouldFaceLeft ~= self.facingLeft then
            self.facingLeft = shouldFaceLeft
            -- Express facing through the Diorama sprite bus (horizontal flip).
            DioramaSpriteRequestBus.Event.SetFlip(self.entityId, self.facingLeft, false)
        end
    end
    -- Mouse deltas do not "release", so clear them each frame.
    self.aimX = 0.0
    self.aimY = 0.0
end

return TwinStickPlayer
