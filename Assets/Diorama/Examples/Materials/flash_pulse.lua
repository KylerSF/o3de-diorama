----------------------------------------------------------------------------------------------------
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
----------------------------------------------------------------------------------------------------

-- Hit-flash demo driver. Attach to a sprite entity in the 2D materials demo.
--
-- Every Interval seconds it "hits" the sprite: snaps the flash to full, then eases
-- it back to zero over FadeTime, using the sprite's DioramaSpriteRequestBus SetFlash
-- verb. This is exactly how gameplay would flash an enemy on a hit -- call SetFlash
-- with amount 1 on the event, then decay it.

local FlashPulse = {
    Properties = {
        FlashR = { default = 1.0, description = "Flash color red (0..1)" },
        FlashG = { default = 1.0, description = "Flash color green (0..1)" },
        FlashB = { default = 1.0, description = "Flash color blue (0..1)" },
        Interval = { default = 1.5, description = "Seconds between hits" },
        FadeTime = { default = 0.35, description = "Seconds for the flash to ease back to zero" },
    },
}

function FlashPulse:OnActivate()
    self.r = self.Properties.FlashR or 1.0
    self.g = self.Properties.FlashG or 1.0
    self.b = self.Properties.FlashB or 1.0
    self.interval = self.Properties.Interval or 1.5
    self.fade = self.Properties.FadeTime or 0.35

    self.timer = 0.0
    self.amount = 0.0
    self.tickHandler = TickBus.Connect(self)
end

function FlashPulse:OnDeactivate()
    if self.tickHandler then
        self.tickHandler:Disconnect()
    end
end

function FlashPulse:OnTick(deltaTime, scriptTime)
    self.timer = self.timer + deltaTime
    if self.timer >= self.interval then
        self.timer = self.timer - self.interval
        self.amount = 1.0 -- "hit": snap to full flash
    end

    -- Ease the flash back toward zero.
    if self.amount > 0.0 and self.fade > 0.0 then
        self.amount = self.amount - deltaTime / self.fade
        if self.amount < 0.0 then
            self.amount = 0.0
        end
    end

    DioramaSpriteRequestBus.Event.SetFlash(self.entityId, self.r, self.g, self.b, self.amount)
end

return FlashPulse
