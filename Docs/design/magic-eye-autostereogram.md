# Design: Magic Eye (autostereogram) toggle

Status: design / concept. A novelty showcase toggle, chosen with the user as the
"surprise" companion to the [Living Diorama](living-diorama.md). Needs GPU work and
on-screen verification (by a human who can free-view a Magic Eye image), so it is a
target, not a buildable-today task.

## Concept

A toggle (`r_dioramaMagicEye`, or a key in the sample) that re-renders the current
scene as a single-image random-dot autostereogram, the classic Magic Eye picture
that resolves into a 3D shape when you diverge your eyes. It is pure share-bait
novelty, and crucially it is only possible *because* a Diorama scene has real depth:
flat sprites arranged in a 3D world produce a depth field, and that field is exactly
what an autostereogram encodes. It turns the engine's core idea (2.5D depth) into a
party trick that demonstrates the depth is genuinely there.

## How an autostereogram works (and the algorithm)

A single-image stereogram repeats a horizontal pattern across each row; the *amount*
each pixel is shifted relative to one pattern-width is proportional to scene depth.
Nearer points shift more, so when the viewer's eyes diverge by one pattern width, the
brain fuses neighbouring repeats and perceives the shift pattern as depth.

The standard per-row algorithm (SIRDS):

```
for each row y:
  for each pixel x left-to-right:
    separation = baseSeparation * (1 - depthScale * depth(x, y))   // nearer -> smaller sep -> more "pop"
    if x < separation:
      out(x, y) = patternOrRandom(x, y)                            // seed the leftmost column
    else:
      out(x, y) = out(x - separation, y)                          // copy from one separation left
```

This is a fragment/compute fullscreen pass: input a depth texture (plus an optional
tiling pattern texture or per-row RNG), output the stereogram to the color target.
The left-to-right dependency is per-row serial, so it runs as a compute pass with one
thread per row (or a fragment pass that walks the row), which is fine at screen
widths.

## The real design wrinkle: where does the depth come from?

Diorama's sprite shader **disables the depth test and does not write depth** (it
orders by sort layer + camera distance, the painter's model). So a pure-sprite scene
has *no* depth buffer for the autostereogram to read. Three ways to supply one:

1. **Real world-Z layers (preferred for the Living Diorama).** If the cutout layers
   sit at real, distinct Z, the scene's normal depth pre-pass (from any opaque
   geometry, or a cheap Diorama depth write) has them. The Living Diorama already
   wants real-Z layers, so the Magic Eye gets a depth source for free there.
2. **A Diorama depth pass (general solution).** Add an opt-in pass that writes each
   sprite's camera distance (already computed for depth sort,
   `SpriteFeatureProcessor`) into a single-channel depth target, enabled only while
   Magic Eye is on. This makes the toggle work in *any* Diorama scene, not just ones
   with real-Z content.
3. **Sort-layer-as-depth (cheapest, coarse).** Map each sprite's sort offset to a
   quantized depth. Crude (stair-stepped), but trivial and enough for a first proof.

Decision: prototype with option 1 in the Living Diorama (real-Z layers give a clean
depth map immediately), then generalize with option 2 so the toggle is universal.

## Integration

A `DioramaMagicEyePass` fullscreen pass injected into the running pipeline via the
pass system (the same pattern as the CRT pass in
[2d-post-processing.md](2d-post-processing.md)), reading the depth source and writing
the stereogram. Gated by `r_dioramaMagicEye` so it is off by default and adds no cost
until toggled. Parameters: pattern (random dots vs. a tiling texture), base
separation (eye-divergence width), depth scale (pop strength).

## Phasing

1. **v1**: the autostereogram pass over a provided depth map (the Living Diorama's
   real-Z layers), random-dot pattern, tuned separation/scale. Prove it resolves.
2. **v2**: the opt-in Diorama depth pass (option 2) so any scene can toggle it.
3. **v3**: animated autostereogram (it updates as the scene moves) and a tiling-texture
   pattern option for a prettier surface.

## Security and performance

- One extra fullscreen pass, only while toggled; removed cleanly when off.
- All parameters clamped before reaching the shader; the depth read is bounded to the
  target size.

## Verification (needs the monitor and a human)

Uniquely, this needs a person who can free-view a stereogram: toggle it on a scene
with clear depth (the Living Diorama) and confirm the hidden shape resolves, that
nearer content pops more, and that toggling off cleanly restores the normal image.
Capture a still for sharing.

## Open questions

- Compute vs. fragment implementation of the per-row serial copy; compute with one
  thread per row is the clean fit.
- Whether to ship it only in the Living Diorama (v1) or expose the universal toggle
  (v2) for the alpha; lean v1 first as the proof, v2 as a follow-up.
