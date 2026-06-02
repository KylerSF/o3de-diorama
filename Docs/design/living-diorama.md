# Design: The Living Diorama (flagship showcase)

Status: design / concept. The flagship visual showcase, chosen with the user. It is
the hero scene the engine is sold on, complementing the twin-stick (gameplay /
scripting / collision capstone). Most of what it needs is designed but not yet
built and needs on-screen verification, so this is the target the visual features
aim at, not a buildable-today task.

## Concept

Lean all the way into the gem's name. The flagship literally *is* a diorama: a paper
**pop-up shadow box** the camera gently orbits. Layers of paper cutouts stand at
different depths inside a little box; a warm lamp rakes 2D light across them;
paper-puppet characters animate; dust motes and fireflies drift; an emissive moon
glows behind. The payoff lands when the camera tilts: the flat cutouts visibly
separate into layered cards, so the 2.5D idea (flat art in a real lit 3D world) is
the entire point of the piece, not a footnote.

It reads as craft, not a tech demo, which is exactly the first impression a 2D/2.5D
engine wants: "look how beautiful, and it is all flat sprites lit in 3D."

## What it showcases (and why it is the right hero)

It is the one scene that naturally exercises the differentiators twin-stick cannot:

- **Parallax depth** - the cutout layers at different Z, separating under camera tilt.
- **2D dynamic lighting** - the lamp is a point light raking normal-mapped paper;
  the headline feature, shown at its most flattering (warm light on textured cards).
- **Cutout / skeletal animation** - paper puppets (a swaying tree, a walking figure).
- **Per-sprite materials + post** - an emissive moon and lamp glow that bloom.
- **Particles** - dust motes in the light shaft, fireflies.
- **Depth sort + soft shadows** - cards cast soft contact shadows on the box floor.
- **The buses** - the whole scene is assembled and animated through the Diorama
  buses in Lua, so it doubles as an AI-native authoring showcase.

## Build plan (phased; each phase already looks like something)

1. **v0 - the box (buildable with today's features + tween).** Static paper cutouts
   on layers (sort offsets / real Z), depth-sorted, with a gentle auto-orbit and
   tween-driven sway. Even unlit, layered cutouts under a tilting camera already read
   as a diorama. No new engine features required beyond what ships now.
2. **v1 - the lamp (needs 2D lighting v1+).** Add a warm point light and normal maps
   on the cutouts. This is the hero moment; the scene goes from flat to lit paper.
3. **v2 - life (needs particles + cutout animation + emissive/bloom).** Dust motes in
   the light shaft, fireflies, a swaying tree and a walking puppet, an emissive moon
   that blooms through stock PostFx.
4. **v3 - delight.** The [Magic Eye toggle](magic-eye-autostereogram.md), gentle
   interactivity (mouse nudge / parallax-on-look), and an optional page turn that
   swaps the scene for another "page."

Dependency order matches the roadmap: lighting, then particles + cutout animation
(v1 of [skeletal](2d-skeletal-animation.md)), then materials/post for the glow.

## Art direction

A paper-craft / construction-paper aesthetic: flat cutouts with subtle paper grain
and torn edges, warm key light, cool ambient, a few saturated accents (the lamp, the
moon, fireflies). Layers: sky/moon, far trees, mid scenery, the character stage, and
a foreground frame (the box edge). All assets must be **original or open-licensed**
(the project rule); the paper-craft look is cheap to author originally and forgiving,
which suits that constraint. The existing ocean art could even become an alternate
"reef diorama" page, reusing [twinstick] creatures.

## Scripting / AI-native angle

The scene is assembled and animated entirely through the Diorama buses from Lua
(place layers, set the light, drive the sway/orbit with the tween library), so it is
also a worked example that an agent could author the same way. This keeps the
flagship aligned with the project's AI-friendly goal rather than being a one-off
hand-built scene.

## Security and performance

- A handful of sprites, one or two lights, one small particle emitter: trivially
  cheap; the batched feature processor draws the layers in a few calls.
- All asset-sourced values (layer counts, light/particle params) are bounded as in
  the rest of the gem.

## Verification (needs the monitor)

This is a visual showcase, so every phase needs on-screen confirmation: the cutouts
separate convincingly under tilt; the lamp lights the paper as intended; particles
and glow read well; the Magic Eye toggle resolves into a 3D image. It is the natural
place to confirm the lighting, parallax, particle, and material features look right
once they are built.

## Open questions

- Interactive (orbit + nudge) vs. a looping cinematic vignette, or both via a toggle.
  Lean: gentle auto-orbit by default, mouse nudge for interactivity, recordable as a
  loop for trailers.
- Whether the cutout layers use real world-Z (so the scene depth buffer has them,
  which also feeds the Magic Eye pass) or pure sort offsets; real Z is preferred here
  precisely because it gives the autostereogram a depth source (see the Magic Eye doc).
