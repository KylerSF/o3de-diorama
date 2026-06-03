-- one_shot.lua
--
-- Fire-and-forget SFX demo/test for DioramaAudioRequestBus.PlayOneShot. Attach to
-- any entity (no MiniAudio component needed -- PlayOneShot manages its own voice
-- pool). It plays the sound on a timer, the way a game fires a pickup or hit SFX.
-- One global call, no per-entity audio wiring; the same line works from Script
-- Canvas or an AI agent.
local OneShot = {
    Properties = {
        Sound = { default = "diorama/audio/blip.wav", description = "Sound product path" },
        Interval = { default = 1.0, description = "Seconds between plays" },
        Volume = { default = 1.0, description = "Volume 0..1" },
    },
}

function OneShot:OnActivate()
    self.sound = self.Properties.Sound or "diorama/audio/blip.wav"
    self.interval = self.Properties.Interval or 1.0
    self.volume = self.Properties.Volume or 1.0
    self.timer = 0.0
    self.tickHandler = TickBus.Connect(self)
end

function OneShot:OnDeactivate()
    if self.tickHandler then
        self.tickHandler:Disconnect()
        self.tickHandler = nil
    end
end

function OneShot:OnTick(deltaTime, scriptTime)
    self.timer = self.timer + deltaTime
    if self.timer >= self.interval then
        self.timer = self.timer - self.interval
        DioramaAudioRequestBus.Broadcast.PlayOneShot(self.sound, self.volume)
    end
end

return OneShot
