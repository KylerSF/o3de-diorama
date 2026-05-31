# Twin-Stick Shooter (Diorama sample game)

A small, growable 2.5D twin-stick shooter built with Diorama sprites and standard
O3DE gameplay (PhysX, Lua, input bindings). This is teaching ladder rung 6: it
shows how Diorama (world visuals) fits together with O3DE's input, physics, and
scripting, and with LyShine for the HUD.

It is built incrementally; each step adds a piece and is runnable on its own:

| Step | Adds | Status |
| ---- | ---- | ------ |
| 1 | Player: sprite + twin-stick movement | Done |
| 2 | Enemies, chase AI, waves | Done |
| 3 | Projectiles and collision | Done |
| 4 | Scoring and HUD (LyShine) | Done |

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
- `Assets/Diorama/TwinStick/Scripts/twin_stick_projectile.lua` -- launches along
  its spawn direction, lives briefly, and destroys any `Enemy`-tagged body it
  hits (and itself) via PhysX collision notifications.
- `Samples/TwinStick/build_player.py` -- assembles the player entity.
- `Samples/TwinStick/build_enemy.py` -- assembles the enemy entity (save it as a
  spawnable prefab for the spawner to use).
- `Assets/Diorama/TwinStick/Scripts/twin_stick_game.lua` -- game/HUD controller:
  tracks the score, loads the LyShine HUD canvas, and updates it when it receives
  `EnemyKilled` events.
- `Samples/TwinStick/build_projectile.py` -- assembles the projectile entity
  (save it as a spawnable prefab for the player to fire).
- `Samples/TwinStick/build_game.py` -- the capstone assembler: builds the whole
  scene (arena, player, camera, game controller, spawner) in one run.

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

## Projectile entity spec

Built by `build_projectile.py`, then **saved as a spawnable prefab** the player
fires:

| Component | Key settings |
| --------- | ------------ |
| Transform | Any (the player sets position and aim yaw at spawn) |
| Diorama Sprite | Texture `sample_sprite.png`; Billboard on; small; bright tint |
| PhysX Dynamic Rigid Body | Gravity **off** |
| PhysX Collider | Small; **Report collisions** enabled |
| Lua Script | Script `twin_stick_projectile.lua` |
| Tag | `Projectile` |

The player gets two new properties on `twin_stick_player.lua`: set
**ProjectilePrefab** to this prefab and tune **FireCooldown**. The `fire` input
(left mouse / right trigger) spawns a projectile facing the current aim; the
projectile launches itself along that direction and destroys the first enemy it
touches.

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

## Running step 3

Build the projectile, save it as a prefab, and wire it to the player:

```bash
<engine>/bin/Linux/profile/Default/Editor \
  --project-path=/path/to/YourProject \
  --runpython /path/to/o3de-diorama/Samples/TwinStick/build_projectile.py
```

Save the `Projectile` entity as a prefab and set the player's `ProjectilePrefab`
property to it. Enter game mode: aim and hold fire to shoot; projectiles destroy
enemies on contact.

## Game controller and HUD spec

A single controller entity runs the score and HUD:

| Component | Key settings |
| --------- | ------------ |
| Transform | Anywhere |
| Lua Script | Script `twin_stick_game.lua` |
| Tag | `Game` (so projectiles can find it) |

Set the script's `HudCanvas` to a LyShine canvas (e.g. `hud.uicanvas`) that
contains a UI **Text** element named `ScoreText`. Create the canvas in the UI
Editor; the controller loads it with `UiCanvasManagerBus.LoadCanvas` and updates
the text with `UiTextBus.SetText`, guarding gracefully if the canvas is absent.

Score is decoupled: a projectile sends an `EnemyKilled` event on
`GameplayNotificationBus` to the `Game`-tagged entity, and the controller adds
`PointsPerKill` and refreshes the HUD. This is the Diorama (world) vs LyShine
(UI) split in action.

## Building the whole game at once

`build_game.py` assembles the arena, player, camera, game controller, and spawner
in one run, configuring all Diorama parts through the typed buses:

```bash
<engine>/bin/Linux/profile/Default/Editor \
  --project-path=/path/to/YourProject \
  --runpython /path/to/o3de-diorama/Samples/TwinStick/build_game.py
```

It logs the remaining standard-O3DE wiring checklist (Lua/input/prefab/canvas
references). See [the rung 6 guide](../../Docs/howto/06-twin-stick.md) for how the
pieces fit together.
