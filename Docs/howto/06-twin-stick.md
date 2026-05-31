# How-To: Twin-Stick Shooter (capstone)

Teaching ladder rung 6. A small but complete 2.5D twin-stick shooter that ties
the whole ladder together: a tilemap arena, a sprite player with twin-stick
controls, chasing enemies spawned in waves, projectiles, and a HUD. It shows the
division of labor in an O3DE game -- **Diorama draws the world, LyShine draws the
UI, and standard O3DE (PhysX, Lua, input bindings, spawnables) runs the
gameplay.**

The game lives under [`Samples/TwinStick`](../../Samples/TwinStick); this guide
explains how the pieces fit. It is a real game built the way a developer (or an
agent) would build one, not a contrived demo.

## What Diorama provides, and what it does not

Diorama is a renderer. It draws the player, enemies, projectiles, and the tilemap
arena as world-space sprites, and exposes typed buses (`DioramaSpriteRequestBus`,
`DioramaTilemapRequestBus`) to configure them. Everything else is ordinary O3DE:

| Concern | Handled by |
| ------- | ---------- |
| Sprites, tilemap arena | **Diorama** |
| Movement, collision, spawning | PhysX + the spawnable system |
| Input | Input bindings + `InputEventNotificationBus` |
| Game logic | Lua scripts |
| HUD | **LyShine** |

A nice consequence: gameplay Lua drives the same Diorama buses an agent would.
The player and enemies set their facing with `DioramaSpriteRequestBus.SetFlip`
from ordinary `OnTick` code.

## The pieces

| Piece | Script / asset | Role |
| ----- | -------------- | ---- |
| Arena | Tilemap component | The floor, a batched grid of atlas tiles (rung 4) |
| Player | `twin_stick_player.lua` | WASD move (PhysX), mouse aim, fire |
| Enemy | `twin_stick_enemy.lua` | Chases the `Player`-tagged entity |
| Spawner | `twin_stick_spawner.lua` | Spawns the enemy prefab in ramping waves |
| Projectile | `twin_stick_projectile.lua` | Launches, lives briefly, kills enemies |
| Game / HUD | `twin_stick_game.lua` | Score + LyShine HUD |

## How a kill flows through the game

1. The player holds fire; `twin_stick_player.lua` spawns the projectile prefab
   toward the aim direction via `SpawnableScriptMediator`.
2. The projectile launches along its forward and, on
   `CollisionNotificationBus.OnCollisionBegin`, checks the other body's tag.
3. On an `Enemy`, it sends an `EnemyKilled` event (`GameplayNotificationBus`) to
   the `Game`-tagged controller, then destroys the enemy and itself.
4. `twin_stick_game.lua` receives the event, adds to the score, and updates the
   LyShine HUD text.

Each step is decoupled through standard O3DE buses, so pieces can be developed and
tested independently.

## Building it

Run the capstone assembler in the editor:

```bash
<engine>/bin/Linux/profile/Default/Editor \
  --project-path=/path/to/YourProject \
  --runpython /path/to/o3de-diorama/Samples/TwinStick/build_game.py
```

It creates the arena, player, camera, game controller, and spawner, and
configures every Diorama part through the typed buses. Then finish the standard
O3DE wiring listed in [`Samples/TwinStick/README.md`](../../Samples/TwinStick/README.md):
the Lua script and input assets, the enemy and projectile **spawnable prefabs**,
the spawner's `EnemyPrefab` and player's `ProjectilePrefab`, and the LyShine HUD
canvas (a `hud.uicanvas` with a text element named `ScoreText`).

## Why it is built this way

Runtime spawning, collision, input, and UI all use the documented O3DE
interfaces, so the sample is a faithful starting point you can grow: add enemy
types, pickups, multiple weapons, or a wave UI. Diorama keeps the visual layer
small and forgiving; the rest is the O3DE you already know.
