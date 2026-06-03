# How-To: Sound (SFX and music)

Add sound to a Diorama game. O3DE 26.05 ships the **MiniAudio** gem (a lightweight,
dependency-free audio engine; enabled in this project), with a per-entity playback
component and a typed request bus that is reflected for Lua, Python, and Script
Canvas. So, as with post-processing, Diorama leans on the engine's audio rather than
shipping its own: this guide is the blessed 2D path, and the AI-friendly angle is
that an agent drives the same bus.

> Note: audio output cannot be verified from a headless screen capture. The sound
> asset processing, the components, and the script calls below are real and verified
> to build/load; confirm the audio *plays* by listening in game mode.

## A sound asset

Drop a `.wav` / `.ogg` / `.mp3` / `.flac` into the project; the asset processor turns
it into a `.miniaudio` product. This gem ships a sample blip at
`diorama/audio/blip.wav` (an original generated tone).

## Play a sound

1. Add a **MiniAudio Playback** component to an entity.
2. Set its **Sound** to your asset (e.g. `diorama/audio/blip.wav`). Tick **Looping**
   for music or an ambient loop.
3. Trigger it from script through the typed bus, addressed by the entity id:

```lua
MiniAudioPlaybackRequestBus.Event.Play(self.entityId)
MiniAudioPlaybackRequestBus.Event.Stop(self.entityId)
MiniAudioPlaybackRequestBus.Event.SetLooping(self.entityId, true)
MiniAudioPlaybackRequestBus.Event.SetVolumePercentage(self.entityId, 0.5)
```

The runnable example [`play_sound.lua`](../../Assets/Diorama/Examples/Audio/play_sound.lua)
plays the entity's sound when it activates. Because the bus is reflected `Common`,
the exact same calls work from editor Python, Script Canvas, and an AI agent -- the
same parity as the sprite/HUD buses.

## Music vs SFX vs spatial

- **Music / UI SFX (2D, non-spatial)**: one entity with a looping (music) or
  one-shot (SFX) Playback component. No listener needed for non-spatial playback.
- **Spatial SFX**: add a **MiniAudio Listener** component to the camera entity; the
  Playback component then attenuates with distance and direction. For a top-down or
  side view, place the listener with the camera.
- **Master volume**: the system bus `MiniAudioRequestBus` exposes
  `SetGlobalVolume` / `SetGlobalVolumeInDecibels`.

## Fire-and-forget one-shots (the 2D gap)

MiniAudio playback is per-entity, which is ideal for music and looping sources but
verbose for the many short SFX a game fires (hits, pickups, footsteps). Until a
Diorama `PlayOneShot(path)` convenience lands (a planned thin wrapper over
MiniAudio's engine), the pattern is: keep a small pool of SFX entities each with a
Playback component, set the sound and `Play()` the next free one; or spawn a
play-on-activate SFX prefab and let it finish. See the roadmap audio item.
