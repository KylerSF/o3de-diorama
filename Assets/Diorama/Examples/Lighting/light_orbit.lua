----------------------------------------------------------------------------------------------------
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
----------------------------------------------------------------------------------------------------

-- Orbiting light for the Diorama 2D lighting demo.
--
-- Attach this to an entity that also has a Diorama "2D Light" component (Point).
-- Each tick it moves the entity in a circle in the world XY plane, so the light's
-- glow sweeps across the sprites in the scene. Diorama lighting follows the light
-- entity's transform (the light component updates its position whenever the entity
-- moves), so animating the light is just moving the entity, the same transform-
-- driven model the rest of the gem uses. Nothing here is lighting-specific beyond
-- "move the entity"; that is the whole point of the demo.

local LightOrbit = {
    Properties = {
        -- Radius of the orbit, in world units. The light circles the entity's
        -- starting position at this distance.
        OrbitRadius = { default = 6.0, description = "Orbit radius in world units" },
        -- Orbit speed in radians per second (about 1.0 = one loop every ~6s).
        OrbitSpeed = { default = 1.0, description = "Orbit speed (radians per second)" },
        -- Optional vertical bob amplitude, in world units (0 = orbit flat).
        BobAmplitude = { default = 0.0, description = "Vertical bob height in world units (0 = none)" },
    },
}

function LightOrbit:OnActivate()
    -- The orbit is centered on wherever the entity is placed, so drop the light at
    -- the middle of the sprite cluster and it sweeps around them.
    self.center = TransformBus.Event.GetWorldTranslation(self.entityId)

    -- Guard every property with a default: a runtime (baked) ScriptComponent does
    -- NOT apply the .lua Property defaults (those are editor-time only), so an
    -- unbaked property reads nil here. Same rule as the twin-stick scripts.
    self.radius = self.Properties.OrbitRadius or 6.0
    self.speed = self.Properties.OrbitSpeed or 1.0
    self.bob = self.Properties.BobAmplitude or 0.0

    self.angle = 0.0
    self.tickHandler = TickBus.Connect(self)
end

function LightOrbit:OnDeactivate()
    if self.tickHandler then
        self.tickHandler:Disconnect()
    end
end

function LightOrbit:OnTick(deltaTime, scriptTime)
    self.angle = self.angle + self.speed * deltaTime

    local x = self.center.x + math.cos(self.angle) * self.radius
    local y = self.center.y + math.sin(self.angle) * self.radius
    local z = self.center.z + math.sin(self.angle * 2.0) * self.bob

    TransformBus.Event.SetWorldTranslation(self.entityId, Vector3(x, y, z))
end

return LightOrbit
