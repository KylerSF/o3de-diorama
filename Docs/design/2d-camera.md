# Design: 2D camera component (follow, deadzone, bounds, pixel-perfect, shake)

Status: **v2 implemented** (Tier-2 roadmap item, task #27). The reusable C++
`DioramaCamera2DComponent` (+ editor twin + `DioramaCamera2DRequestBus`) ships,
built on the pure unit-tested `Camera2D` math core: follow, deadzone, bounds,
lookahead, trauma shake, and position pixel-snap, in any of the XY/XZ/YZ planes.
This absorbs the deferred gameplay-juice work (camera follow + screen shake, task
#22). Deferred to v3: orthographic projection switch for true pixel-perfect (the
component snaps position today but does not yet drive the ortho clip matrix).

The original design notes below are kept for context.

## Goal

A drop-on camera that does what every 2D game needs: smoothly follow a target, hold
a deadzone so small motions do not jitter the view, stay inside world bounds, look
ahead in the direction of motion, support a pixel-perfect orthographic mode, and
shake on impact. The twin-stick sample's static camera becomes a following one, and
hits feel impactful.

## Approach

An O3DE camera is an entity with a Camera component (an Atom View); we steer it by
writing the **camera entity's transform** through `TransformBus`, which is exactly
the transform-driven model Diorama already uses. So the camera controller is a
component (or, for v1, a sample script) that each tick computes a desired camera
transform and applies it. The existing 2.5D tilt (the sample's pitched camera) is
preserved as a fixed orientation offset; only the position tracks.

### Follow + smoothing

```
desired = targetPos + followOffset            // offset keeps the 2.5D framing
camera  = damp(camera, desired, smoothing, dt) // critically-damped lerp, frame-rate independent
```

A critically-damped spring (or `lerp` with `1 - exp(-k*dt)`) gives smooth,
overshoot-free follow that is independent of frame rate.

### Deadzone

The target may move within a rectangle (in view space) without moving the camera;
the camera only chases the component of motion that would push the target outside the
deadzone. This kills micro-jitter and is the standard 2D feel.

### Bounds

Clamp the final camera position to a world-space rectangle (the level extents minus
half the view) so the camera never shows outside the playfield. Bounds are optional
(off = free follow).

### Lookahead

Offset the desired position by `lookaheadDistance * normalize(targetVelocity)` so the
view leads the target's motion, giving the player more room ahead. Smoothed so it
eases in/out as the target starts/stops.

### Pixel-perfect / orthographic

For pixel-art: put the camera in **orthographic** projection and size it so one world
unit maps to an integer number of pixels (`pixelsPerUnit`), then snap the camera
position to the texel grid (`round(pos * ppu) / ppu`) so sprites land on whole
pixels and do not shimmer. Set ortho via the Camera component if it exposes it,
otherwise set the Atom View's view-to-clip matrix directly (confirm the available
surface at implementation). Pixel-perfect and the 2.5D tilt are mutually exclusive
modes (tilt implies perspective); the component picks one.

### Screen shake (trauma model)

Trauma-based shake reads better than a fixed decay: events add `trauma` (0..1);
each frame `shake = maxShake * trauma^2` drives a positional (and small rotational)
offset from smooth noise, and `trauma` decays linearly. `trauma^2` makes small hits
subtle and big hits punchy, and the offset is applied *after* follow so it never
fights the tracking. Pairs naturally with the [tween library](../../Assets/Diorama/Scripts/tween.lua).

## Delivery

- **v1 (ships now, verifiable as soon as there is a monitor)**: a sample camera
  controller script (`twin_stick_camera.lua` or similar) doing follow + deadzone +
  shake over `TransformBus`, wired to the player and to hit events. This delivers the
  juice (#22) and proves the feel.
- **v2**: promote to a reusable C++ `DioramaCamera2DComponent` with editor config
  (target, offset, smoothing, deadzone rect, bounds rect, lookahead, shake limits)
  and a small request bus (`SetTarget`, `AddTrauma`, `SetBounds`) in the AI-native
  style, so any project gets it without scripting.
- **v3**: pixel-perfect/orthographic mode and a pixel-art sample.

## Security and performance

- All per-frame work is O(1); no allocation.
- Bounds, deadzone, and shake magnitudes are clamped; `AddTrauma` clamps to 0..1 so
  no event can drive an unbounded offset.

## Verification plan (needs the monitor)

- Follow: the camera tracks the player smoothly; stopping does not overshoot.
- Deadzone: small wiggles do not move the camera; crossing the deadzone edge does.
- Bounds: at a level edge the camera stops, never showing past the playfield.
- Shake: a hit produces a brief punchy shake that decays; rapid hits accumulate but
  stay bounded; shake does not break follow.
- Pixel-perfect (v3): a pixel-art sprite shows no shimmer while the camera moves.

## Open questions

- Confirm whether the stock Camera component exposes orthographic projection in
  26.05, or whether we set the Atom View clip matrix directly for pixel-perfect mode.
- Whether v1 lives as a sample script or goes straight to the C++ component; leaning
  script-first to get the feel verified, then promote.
