# Twin-Stick Shooter (Diorama sample game)

A small, growable 2.5D twin-stick shooter built with Diorama sprites and standard
O3DE gameplay (PhysX, Lua, input bindings). This is teaching ladder rung 6: it
shows how Diorama (world visuals) fits together with O3DE's input, physics, and
scripting, and with LyShine for the HUD.

It is built incrementally; each step adds a piece and is runnable on its own:

| Step | Adds | Status |
| ---- | ---- | ------ |
| 1 | Player: sprite + twin-stick movement | Done |
| 2 | Enemies, chase AI, waves | This step |
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
- `Assets/Diorama/TwinStick/Scripts/twin_stick_enemy.lua` -- chase AI: each tick
  the enemy finds the entity tagged `Player` and steers toward it through PhysX,
  facing its movement direction by flipping its Diorama sprite.
- `Assets/Diorama/TwinStick/Scripts/twin_stick_spawner.lua` -- wave spawner:
  spawns the enemy prefab on a timer around the arena edge using the standard
  spawnable system (`SpawnableScriptMediator`), ramping the rate up over time.
- `Samples/TwinStick/build_player.py` -- assembles the player entity.
- `Samples/TwinStick/build_enemy.py` -- assembles the enemy entity (save it as a
  spawnable prefab for the spawner to use).

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

## Enemy entity spec

Built by `build_enemy.py`, then **saved as a spawnable prefab** so the spawner
can instantiate copies at runtime:

| Component | Key settings |
| --------- | ------------ |
| Transform | Any (the spawner sets the spawn position) |
| Diorama Sprite | Texture `sample_sprite.png`; Billboard on; reddish tint |
| PhysX Dynamic Rigid Body | Gravity **off**; lock Z motion |
| PhysX Collider | Sphere or box sized to the sprite |
| Lua Script | Script `twin_stick_enemy.lua` |
| Tag | `Enemy` |

To make it spawnable: select the enemy entity and use **Create Prefab** (or save
it as a `.prefab`); the Asset Processor produces a `.spawnable` product the
spawner references.

## Spawner entity spec

A single controller entity that runs the waves:

| Component | Key settings |
| --------- | ------------ |
| Transform | At the arena center |
| Lua Script | Script `twin_stick_spawner.lua` |

Set the script's **EnemyPrefab** property to the enemy prefab. Tune
`SpawnInterval`, `MinInterval`, `RampPerSpawn`, `ArenaRadius`, and `MaxAlive` to
shape the difficulty curve. The spawner uses `SpawnableScriptMediator` -- the
standard O3DE runtime-spawning API -- so this is ordinary O3DE, not a Diorama
special case.

## Running step 1

```bash
<engine>/bin/Linux/profile/Default/Editor \
  --project-path=/path/to/YourProject \
  --runpython /path/to/o3de-diorama/Samples/TwinStick/build_player.py
```

Then enter game mode and drive with WASD; aim with the mouse to flip the player's
facing. (On-screen play requires a working editor/launcher; the script logs the
resolved sprite state via `GetSpriteInfo` so the build can be confirmed headless.)

## Running step 2

Build the enemy, save it as a prefab, then add a spawner:

```bash
<engine>/bin/Linux/profile/Default/Editor \
  --project-path=/path/to/YourProject \
  --runpython /path/to/o3de-diorama/Samples/TwinStick/build_enemy.py
```

Save the resulting `Enemy` entity as a prefab, create an entity with the
`twin_stick_spawner.lua` script, and point its `EnemyPrefab` at that prefab.
Enter game mode: enemies spawn around the edge and chase the player, with the
wave rate ramping up over time.
