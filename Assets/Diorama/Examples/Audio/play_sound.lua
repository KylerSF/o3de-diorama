-- play_sound.lua
--
-- Plays the entity's sound through O3DE's MiniAudio playback bus. Attach it to an
-- entity that also has a MiniAudio Playback component with a Sound set (e.g.
-- diorama/audio/blip.wav). On activate it plays once; flip Loop on for music.
--
-- This is the AI-friendly audio path: MiniAudioPlaybackRequestBus is reflected
-- Common, so an agent, Script Canvas, or game code all trigger sound the same way
-- they drive sprites and the HUD. Call Play from any game event (a hit, a pickup,
-- a menu open) rather than OnActivate.
local PlaySound = {
    Properties = {
        Loop = { default = false, description = "Loop the sound (music / ambience)" },
        Volume = { default = 1.0, description = "Volume 0..1" },
    },
}

function PlaySound:OnActivate()
    MiniAudioPlaybackRequestBus.Event.SetLooping(self.entityId, self.Properties.Loop or false)
    MiniAudioPlaybackRequestBus.Event.SetVolumePercentage(self.entityId, self.Properties.Volume or 1.0)
    MiniAudioPlaybackRequestBus.Event.Play(self.entityId)
end

function PlaySound:OnDeactivate()
    MiniAudioPlaybackRequestBus.Event.Stop(self.entityId)
end

return PlaySound
