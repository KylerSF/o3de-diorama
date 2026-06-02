# Design: in-editor sprite/atlas slicer and animation clip editor

Status: design (Tier-3 roadmap item). No implementation yet.

## Goal

Author atlas sub-regions and animation clips visually, instead of typing UV numbers
and grid parameters into the inspector. Lower the friction of the common authoring
loop: slice a sheet, define frames and fps, preview the result.

## What already exists to build on

- `SpriteComponentConfig` already holds everything these tools edit: `uvMin`/`uvMax`
  (atlas sub-region), the sprite-sheet grid (columns/rows/count), `framesPerSecond`,
  `loop`, and `startFrame`. The editor already pushes config edits to the live
  preview (`ChangeNotify -> OnConfigChanged -> SetConfig`, see
  [Docs/design/2d-live-preview.md](2d-live-preview.md)).
- So these tools are a **visual front-end over existing fields**, not a new data
  model. They write the same config through the same path, so the preview updates for
  free.

## Approach

An editor-module (Qt) tool, reachable from the Sprite component inspector (a
"Slice..." / "Edit Clips..." button via a UI element handler) or a Diorama tools
menu:

- **Atlas slicer.**
  - v1: an **auto-grid** helper. Show the texture, let the user set columns/rows (or
    a cell pixel size), preview the cell lines over the image, and write the
    sprite-sheet grid into the config. Covers the uniform-sheet case, which is the
    common one and which the runtime animation already consumes.
  - v2: **free-form rectangles** for tightly packed atlases (draw/drag rects, name
    them), producing per-frame rects rather than a uniform grid. This pairs with the
    non-uniform output of the Aseprite importer
    ([Docs/design/2d-aseprite-import.md](2d-aseprite-import.md)).
- **Animation clip editor.**
  - List the frames (from the grid/rects), set `framesPerSecond`, `loop`, and
    `startFrame`, and **preview playback** in the panel (reusing the existing
    `SpriteAnimation::Advance` logic so the preview matches runtime exactly).
  - v2: multiple **named clips** over frame ranges (idle/walk/...), saved as a small
    reusable clip asset that a Sprite can select, again matching importer output.

## Why not a big custom canvas first

The uniform-grid + frame-range case (v1) covers most 2D content and reuses fields and
playback logic that already exist, so it ships fast and correct. The heavier
free-form rect canvas and named-clip asset (v2) are added once there is demand and
once the importer produces packed atlases to edit.

## Security and performance

- The tools only edit existing, already-validated config fields and a small clip
  asset; all values are clamped on write (grid >= 1, fps >= 0, frame range in
  bounds), exactly as the inspector path does.
- Editor-only: no runtime cost, and the tools live in the Qt tools module, keeping the
  runtime client free of editor code (the two-module rule).

## Phasing

1. **v1**: auto-grid slicer (write the sprite-sheet grid) + a frame-range/fps clip
   preview, on top of the existing fields and playback logic.
2. **v2**: free-form rect slicing + named-clip asset (pairs with Aseprite import).
3. **v3**: per-frame events (call a callback on a frame) and pivot/origin editing.

## Verification plan (needs the editor)

- Auto-grid: set columns/rows on a sheet, see the cell overlay, and the Sprite plays
  the grid live without typing UVs.
- Clip preview: change fps/loop and watch the panel preview match the viewport.
- Free-form (v2): draw rects on a packed atlas, name frames, and a clip plays them.

## Open questions

- Qt dockable widget vs a reflected property-editor custom UI handler embedded in the
  inspector; lean on a modal/dockable Qt tool for the canvas, a reflected control for
  the simple auto-grid.
- Whether clips live on the Sprite config or as a standalone shared asset; v2 leans
  standalone so multiple sprites reuse a clip set (consistent with importer output).
