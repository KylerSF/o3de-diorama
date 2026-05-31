----------------------------------------------------------------------------------------------------
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
----------------------------------------------------------------------------------------------------

-- Game controller and HUD for the Diorama twin-stick sample.
--
-- This shows the division of labor in a 2.5D O3DE game: Diorama draws the world
-- (player, enemies, projectiles, tilemap arena) while LyShine draws the HUD. The
-- controller tracks the score, loads the LyShine canvas, and updates its text.
--
-- Kills are decoupled through the standard GameplayNotificationBus: a projectile
-- sends an "EnemyKilled" event addressed to this entity; the controller listens
-- and increments the score. The controller entity should carry the tag "Game" so
-- projectiles can find it.

local TwinStickGame = {
    Properties = {
        HudCanvas = { default = "diorama/twinstick/ui/hud.uicanvas", description = "LyShine HUD canvas path" },
        PointsPerKill = { default = 100, description = "Score awarded per enemy destroyed" },
    },
}

function TwinStickGame:OnActivate()
    self.score = 0
    self.canvasId = nil
    self.scoreElement = nil

    -- Load the HUD canvas. Guarded so the game still runs if the canvas asset is
    -- missing (the HUD simply does not show).
    self.canvasId = UiCanvasManagerBus.Broadcast.LoadCanvas(self.Properties.HudCanvas)
    if self.canvasId ~= nil and self.canvasId:IsValid() then
        local element = UiCanvasBus.Event.FindElementByName(self.canvasId, "ScoreText")
        if element ~= nil then
            self.scoreElement = element:GetId()
        end
    end

    -- Listen for kills addressed to this entity (the "Game" channel).
    self.killBus = GameplayNotificationBus.Connect(self, GameplayNotificationId(self.entityId, "EnemyKilled", "float"))

    self:UpdateHud()
end

function TwinStickGame:OnDeactivate()
    if self.killBus then
        self.killBus:Disconnect()
    end
    if self.canvasId ~= nil and self.canvasId:IsValid() then
        UiCanvasManagerBus.Broadcast.UnloadCanvas(self.canvasId)
    end
end

function TwinStickGame:OnEventBegin(value)
    -- A projectile destroyed an enemy.
    self.score = self.score + self.Properties.PointsPerKill
    self:UpdateHud()
end

function TwinStickGame:UpdateHud()
    if self.scoreElement ~= nil then
        UiTextBus.Event.SetText(self.scoreElement, "Score: " .. tostring(self.score))
    end
end

return TwinStickGame
