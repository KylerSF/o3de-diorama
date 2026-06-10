# 2D animation state machine

Status: shipped (v1).

## Why

Diorama ships several ways to play a single animation clip: sprite-sheet
(flipbook) playback on the Sprite, named Aseprite tags on the Aseprite component,
and keyframed cutout poses on the Skeletal component. What it lacked is the layer
*above* a clip: deciding *which* clip should play right now from game state. Every
real character needs that (idle to run when speed rises, jump on a button,
attack on a trigger, land when grounded). Without it, games hand-roll the same
if/else clip-switching in Lua every time, which is exactly the boilerplate the
typed buses exist to remove.

This is also a parity feature. A human authors the graph in the Inspector; an
agent or script drives it through a typed bus by pushing parameters, the same way
it drives sprites, the HUD, and audio. The graph is the durable, declarative
artifact both can read.

## Model

The design mirrors the controller graphs in Unity/Godot, reduced to the minimum a
2D game needs:

- **Parameters** are a small named blackboard. Three kinds:
  - `Bool` (latched true/false, e.g. `grounded`),
  - `Float` (continuous, e.g. `speed`, tested with a comparison),
  - `Trigger` (a one-shot pulse: set it, it fires one transition, then auto-clears).
- **States** are nodes, each bound to one animation clip. A state names an
  Aseprite tag (preferred when set) or a sprite-sheet clip
  (columns/rows/frameCount/fps/loop). A clip duration (explicit, or derived from
  `frameCount / fps`) feeds exit-time transitions.
- **Transitions** are directed edges with a list of ANDed **conditions** on
  parameters, plus an optional **exit time** (fire only once the source clip is a
  given fraction played). A transition whose `from` is empty is an **Any-State**
  edge, eligible from every state. Definition order is priority: the first
  eligible transition wins.

Each tick the machine advances the time in the current state, evaluates the
eligible transitions, and on a change plays the new state's clip on the target
entity and emits `OnStateChanged(from, to)`.

## Architecture

The whole decision is a pure, dependency-free core
([`AnimStateMachine.h`](../../Code/Source/Clients/AnimStateMachine.h)): a state is
an index, a parameter is a `ParamValue`, a transition is a guard, and `Step`
advances the runtime (timing, exit-time normalization, priority selection, and
trigger consumption) with no engine, asset, or render dependency. That is the
same pattern as `Collision2D.h` / `Camera2D.h` / `SkeletalClip.h` /
`TilemapPaint.h`, and it means the entire behaviour is unit tested headlessly
(see [`AnimStateMachineTest.cpp`](../../Code/Tests/Clients/AnimStateMachineTest.cpp)).

`DioramaAnimStateMachineComponent` is a thin shell over the core: it resolves the
authored, name-based config into the index-based pure definition
(`BuildAnimStateMachineDefinition`), holds the runtime, calls `Step` each tick,
and on a state change drives the bound animation through the existing
`DioramaSpriteRequestBus` / `DioramaAsepriteRequestBus`. It owns no rendering; it
composes with the Sprite/Aseprite components that do. The target defaults to the
same entity, or a named target entity, so the graph can live on a controller
entity that drives a child sprite.

Malformed graphs degrade rather than crash: an edge to an unknown state is
dropped, a condition on an unknown parameter is dropped, and an exit time is
clamped to `[0,1]`. An unconditional edge with no exit time is ignored by the
core so a graph can never livelock.

## API

`DioramaAnimStateMachineRequestBus` (Common scope, addressed by entity):

- `SetBool(name, value)`, `SetFloat(name, value)` set the blackboard.
- `SetTrigger(name)` pulses a trigger; `ResetTrigger(name)` cancels a pending one.
- `Play(stateName)` forces a state immediately, bypassing transitions.
- `GetCurrentState()`, `GetBool(name)`, `GetFloat(name)` read back state for the
  verify loop.

`DioramaAnimStateMachineNotificationBus` fires `OnStateChanged(from, to)` on every
state entry, so logic can be tied to a state without rendering (spawn a hitbox on
entering `attack`, play a footstep on entering `run`). This is also how a headless
test or agent confirms the graph moved.

## Decisions and non-goals

- **One active state, no blend trees (v1).** 2D animation is overwhelmingly
  discrete clip switching; cross-fades and 1D/2D blend spaces are a possible v2
  but add real complexity (per-clip weights, synchronized time) for a payoff that
  cutout/flipbook art rarely needs.
- **Triggers stay pending until consumed.** Unlike Unity (which resets triggers
  at end of frame), a pulse here waits for a transition that uses it. That is more
  forgiving for scripting ("request a jump, it fires when reachable"); call
  `ResetTrigger` to cancel. Documented so it is not surprising.
- **No in-editor graph canvas (v1).** Authoring is the Inspector list of
  parameters/states/transitions, which is enough to be useful and keeps the
  feature to the gem's existing twin pattern. A node-graph editor is a future
  nicety, not a blocker.
