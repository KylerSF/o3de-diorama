----------------------------------------------------------------------------------------------------
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
----------------------------------------------------------------------------------------------------

-- Wave spawner for the Diorama twin-stick sample game.
--
-- Spawns the enemy prefab on a timer at random points around the edge of the
-- arena, and ramps the spawn rate up over time so waves get harder. Uses the
-- standard O3DE spawnable system (SpawnableScriptMediator), so the enemy is an
-- ordinary spawnable prefab; this is exactly how a developer wires runtime
-- spawning in O3DE.

local TwinStickSpawner = {
    Properties = {
        EnemyPrefab = { default = SpawnableScriptAssetRef(), description = "Enemy prefab to spawn" },
        SpawnInterval = { default = 2.0, description = "Seconds between spawns at the start" },
        MinInterval = { default = 0.5, description = "Fastest spawn interval as waves ramp up" },
        RampPerSpawn = { default = 0.05, description = "Interval reduction after each spawn" },
        ArenaRadius = { default = 12.0, description = "Distance from the origin enemies spawn at" },
        MaxAlive = { default = 30, description = "Cap on concurrently spawned enemies" },
    },
}

function TwinStickSpawner:OnActivate()
    self.mediator = SpawnableScriptMediator()
    self.timer = 0.0
    self.interval = self.Properties.SpawnInterval
    self.tickets = {}
    self.tickHandler = TickBus.Connect(self)
end

function TwinStickSpawner:OnDeactivate()
    if self.tickHandler then
        self.tickHandler:Disconnect()
    end
    self.tickets = {}
end

function TwinStickSpawner:SpawnOne()
    local ticket = self.mediator:CreateSpawnTicket(self.Properties.EnemyPrefab)

    -- A random point on the arena edge.
    local angle = math.random() * 2.0 * math.pi
    local r = self.Properties.ArenaRadius
    local position = Vector3(math.cos(angle) * r, math.sin(angle) * r, 0.5)

    -- No parent (world space), no rotation, unit scale.
    self.mediator:SpawnAndParentAndTransform(ticket, EntityId(), position, Vector3(0.0, 0.0, 0.0), 1.0)
    table.insert(self.tickets, ticket)
end

function TwinStickSpawner:OnTick(deltaTime, scriptTime)
    self.timer = self.timer + deltaTime
    if self.timer < self.interval then
        return
    end
    if #self.tickets >= self.Properties.MaxAlive then
        return
    end

    self.timer = 0.0
    self:SpawnOne()

    -- Ramp difficulty: each spawn shortens the interval down to MinInterval.
    self.interval = math.max(self.Properties.MinInterval, self.interval - self.Properties.RampPerSpawn)
end

return TwinStickSpawner
