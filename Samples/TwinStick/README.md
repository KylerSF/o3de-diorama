# Twin-Stick Shooter (Diorama sample game)

A small, growable 2.5D twin-stick shooter built with Diorama sprites and standard
O3DE gameplay (PhysX, Lua, input bindings). This is teaching ladder rung 6: it
shows how Diorama (world visuals) fits together with O3DE's input, physics, and
scripting, and with LyShine for the HUD.

It is built incrementally; each step adds a piece and is runnable on its own:

| Step | Adds | Status |
| ---- | ---- | ------ |
| 1 | Player: sprite + twin-stick movement | This step |
| 2 | Enemies, chase AI, waves | Planned |
| 3 | Projectiles and collision | Planned |
| 4 | Scoring and HUD (LyShine) | Planned |

The arena floor is a Diorama [Tilemap](../../Docs/howto/04-tilemap.md).

## Layout

- `Assets/Diorama/TwinStick/Scripts/twin_stick_player.lua` -- player movement and
  aim. WASD / left-stick move the player through PhysX; the mouse / right-stick
  set the facing direction, which the script expresses by flipping the Diorama
  sprite through `DioramaSpriteRequestBus` (the same AI-native bus an agent uses,
  called here from gameplay).
- `Assets/Diorama/TwinStick/Input/twin_stick.inputbindings` -- input asset
  mapping devices to the `move_x`, `move_y`, `aim_x`, `aim_y`, and `fire` events.
- `Samples/TwinStick/build_player.py` -- assembles the player entity in the editor.

## Player entity spec

A real O3DE entity built from these components (what `build_player.py` creates,
and what to set in a prefab or by hand):

| Component | Key settings |
| --------- | ------------ |
| Transform | Position near the arena floor |
| Diorama Sprite | Texture `sample_sprite.png`; Billboard on; size ~1x1 |
| PhysX Dynamic Rigid Body | Gravity **off**; linear damping ~2; lock Z motion |
| PhysX Collider | Sphere or box sized to the sprite |
| Lua Script | Script `twin_stick_player.lua` |
| Input to Event Bindings | Bindings `twin_stick.inputbindings` |
| Tag | `Player` |

Movement is velocity-driven through PhysX so collisions with arena walls and
(later) enemies work. Gravity is off because this is a top-down game in the XY
plane; the camera looks down the Z axis.

## Running step 1

```bash
<engine>/bin/Linux/profile/Default/Editor \
  --project-path=/path/to/YourProject \
  --runpython /path/to/o3de-diorama/Samples/TwinStick/build_player.py
```

Then enter game mode and drive with WASD; aim with the mouse to flip the player's
facing. (On-screen play requires a working editor/launcher; the script logs the
resolved sprite state via `GetSpriteInfo` so the build can be confirmed headless.)
