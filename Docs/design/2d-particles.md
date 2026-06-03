# Design: 2D particle system

Status: **v1 implemented** (Tier-2 roadmap item). The `ParticleEmitterComponent`
(+ editor twin + `DioramaParticleRequestBus`) ships, built on the pure unit-tested
`Particles2D` core: continuous + burst emission, point/cone/radial shape with
spread, randomized lifetime/speed, gravity/drag, and size + color over life,
rendered as billboarded quads through the existing sprite batch (one draw per
emitter) via a pre-acquired handle pool. The pool is fixed-capacity and all ranges
are clamped. Deferred: additive/multiply blend (v2, pairs with materials),
per-particle atlas animation, trails/sub-emitters (v3), and porting the
twin-stick heart-burst onto it. The design notes below are kept for context.

## Goal

A real particle emitter component for sparks, smoke, dust, hearts, explosions, and
trails, rendered through the existing batched sprite path so a burst is cheap. The
sample already hand-rolls a heart-burst pool; this turns that pattern into a reusable,
authorable feature.

## Approach

Particles are just many short-lived billboarded sprites that share a texture, which
is exactly what the `SpriteFeatureProcessor` already batches well. So the emitter
simulates particles on the CPU and feeds their quads through the existing batch path
(same texture + sort layer = one draw call per emitter). No new render path.

### Emitter configuration

- **Texture / atlas frame** (and optional per-particle flipbook animation in v2).
- **Emission**: continuous `rate` (particles/sec) and/or `burst` counts at times;
  `maxParticles` (a hard, bounded pool size).
- **Shape**: point, cone, circle/radial, box, or along a direction, with spread.
- **Lifetime**: per-particle, with randomized range.
- **Motion**: initial speed range, `gravity`, `drag`; optional radial/tangential
  acceleration; rotation + angular velocity.
- **Over-life curves**: size start->end, color/alpha start->end, reusing the
  [tween easings](../../Assets/Diorama/Scripts/tween.lua) for the interpolation shape.
- **Blend mode**: alpha or additive (additive for fire/sparks/magic); ties into the
  materials blend-mode work ([Docs/design/2d-materials.md](2d-materials.md)).
- **Simulation space**: world (particles stay put as the emitter moves, for trails) or
  local (particles follow the emitter).

### Simulation and rendering

- A fixed-capacity, **pooled** particle array (`maxParticles`); spawn reuses dead
  slots. No per-frame heap allocation, matching the render-loop rule.
- Per tick: integrate velocity/gravity/drag, age particles, evaluate the over-life
  curves, retire the dead. O(live particles).
- The per-tick delta is clamped first (`ClampTimeStep`): a non-finite or non-positive
  delta becomes 0 and large deltas are capped. The first frame after a level load can
  hand the component a non-finite delta; without the clamp, continuous emission
  (`accumulator += rate * dt`) would accumulate an unbounded spawn count and the drain
  loop would spin forever, hanging the game thread. Emission is also bounded to the
  pool capacity per tick (`EmitCountForTick`), so the spawn loop is always finite.
  Both are pure helpers in `Particles2D.h` and unit-tested.
- Each live particle emits a billboarded quad (with its current size/color/rotation)
  into the sprite batch for the emitter's texture, so the whole burst is one draw.

### AI-native bus

`DioramaParticleRequestBus` (per entity), reflected `Common`: `Emit(count)`,
`Burst()`, `SetRate(r)`, `Play()`, `Stop()`, `SetTexture(path)`, `SetLifetime(...)`,
`SetGravity(...)`, `SetStartColor/EndColor`, `SetStartSize/EndSize`,
`SetBlendMode(mode)` - scalar, documented, clamped verbs in the established style, so
an agent (or Script Canvas) drives effects the same way it drives sprites.

## Security and performance

- `maxParticles`, `rate`, and all ranges are validated and clamped at load and on
  every setter; the pool is fixed-size, so no asset/script value can size a buffer or
  spawn unboundedly.
- Pooled storage and scratch buffers mean no per-frame allocation.
- One draw call per emitter texture (additive particles batch among themselves).

## Phasing

1. **v1**: continuous + burst emission, point/cone/radial shapes,
   velocity/gravity/drag, size + color over life, billboarded, alpha blend, the bus.
   Retire the sample's bespoke heart-burst onto this (a real dogfood + a code
   deletion in the sample).
2. **v2**: additive/multiply blend (with materials), per-particle atlas-frame
   animation, more shapes, rotation curves.
3. **v3**: trails, sub-emitters (a particle spawns an emitter on death, for
   fireworks), and a simple curve editor for over-life ramps.

## Verification plan (needs the monitor)

- A burst emits N particles that fall under gravity, fade and shrink over life, and
  retire cleanly; the pool never grows past `maxParticles`.
- A continuous emitter holds a steady particle count at `rate * lifetime`.
- Additive mode (v2) brightens the background; confirm one draw call per emitter and
  no per-frame allocation under a stress burst.
- The sample's heart effect, ported to the emitter, looks the same as before with
  less code.

## Open questions

- Whether the emitter registers particles as transient entries in the existing
  `SpriteFeatureProcessor` or owns a thin parallel batch using the same shader/draw
  context; leaning on reusing the feature processor to avoid a second code path.
- Over-life curve representation: start/end + easing enum (compact, v1) vs a small
  keyed curve (v3).
