# Diorama Examples and How-To Outline

This is the documentation and sample ladder for Diorama. Each step builds on the
previous one and maps to a how-to guide plus a runnable example. The goal is that
a newcomer can follow the ladder from a single sprite to a complete game.

## Teaching ladder (core)

1. Hello Sprite
   - One Sprite component, one texture, drawn in world space.
   - Teaches: enabling the gem, adding the component, assigning a texture,
     world placement, the runtime vs editor component split.

2. Animated Sprite
   - Play frames from a sprite sheet on a timer.
   - Teaches: sprite-sheet sampling, UV regions, frame timing.

3. Sprite Atlas
   - Multiple sprites sharing one atlas texture, batched.
   - Teaches: atlas authoring, UV regions at scale, draw batching and why it
     matters for performance.

4. Tilemap
   - A tilemap asset built from a source file and rendered by a Tilemap component.
   - Teaches: the custom asset and builder pipeline, compact product assets,
     bounded and validated asset data, batched tile rendering.

5. Parallax and Layers (2.5D)
   - Several sprite and tilemap layers at different depths with parallax.
   - Teaches: depth sorting, transparency ordering, the 2.5D camera setup,
     mixing flat content with a 3D background.

6. Twin-Stick Shooter (capstone)
   - A complete small 2.5D game: player movement, aiming, projectiles, enemies,
     a tilemap arena, and a HUD (LyShine for UI, Diorama for world content).
   - Teaches: how all the pieces fit, integration with input, physics, and
     scripting, and the clean division of labor between Diorama and LyShine.

## Advanced and bonus

- Custom Sprite Material
  - Drive a Diorama quad with a custom shader and live parameters instead of a
    static texture. A Mandelbrot or Julia fractal with zoom, center, and
    iteration controls is a good, eye-catching demo here.
  - Teaches: the extensibility seam (your shader on a Diorama quad), shader
    parameters via the per-draw resource group.
  - Note: this is a bonus, not a core teaching example. It showcases the custom
    shader path rather than the gem's primary 2D and 2.5D workflows.

- Pixel-Perfect Camera
  - Orthographic, integer-scaled rendering for crisp pixel art.

- Thousands of Sprites
  - A stress scene that exercises the batched feature processor and documents
    the performance characteristics.

## How-to guides (cross-cutting)

- Install and enable the Diorama gem in a project.
- Author and import sprite textures (formats, alpha, filtering).
- Build a tilemap from source to product asset.
- Set up a 2D or 2.5D camera.
- Performance tips: batching, atlases, draw order, when the feature processor
  path kicks in.
- Contributing: coding conventions and how Diorama relates to LyShine and Atom.
