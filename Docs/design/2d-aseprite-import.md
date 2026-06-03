# Design: Aseprite / sprite-sheet import

Status: design (Tier-3 roadmap item). No implementation yet.

## Goal

Let artists author in Aseprite (the de-facto indie pixel-art tool) and drop a
`.aseprite` / `.ase` file into the project, getting a ready sprite sheet plus
animation clips with no manual slicing. The `.ase` *file format* is openly
documented, so reading it carries no license constraint (we are not shipping
Aseprite, only importing its open format).

## Key findings (O3DE 26.05)

- A gem adds a source-asset importer by implementing
  `AssetBuilderSDK::AssetBuilderCommandBus::Handler` (`CreateJobs`, `ProcessJob`,
  `ShutDown`) and registering it from a `BuilderComponent` in the tools module
  (pattern: `Gems/OpenParticleSystem/.../Builder/`). The builder declares the source
  extension via `AssetBuilderPattern` and bumps `m_version` to force re-analysis.
- `ProcessJob` emits `JobProduct`s with a stable `m_productSubID`, an asset type, and
  `ProductDependency`s.
- A builder can **delegate image compression** to the existing image pipeline:
  `ImageBuilderRequestBus::ConvertImageObject(...)`
  (`Gems/Atom/Asset/ImageProcessingAtom/.../ImageBuilderComponent.h`) turns our
  composed RGBA into a proper `StreamingImageAsset`, so we never reimplement texture
  compression.
- There is a `TextureAtlas` gem but it is a runtime lookup class with no importer, so
  it does not do this for us; our sprite UV model is independent and fine.

## Approach

A `DioramaAsepriteBuilder` (tools module) that:

1. **Parses the `.aseprite` binary** ourselves (the format is simple: a header, then
   frames, each a list of chunks: layers `0x2004`, cels `0x2005`, tags `0x2018`,
   palette). Cel pixel data uses zlib, which AzCore already provides. We treat the
   file as untrusted and validate every offset and length before reading.
2. **Composites** each frame's visible layers into an RGBA image and **packs** the
   frames into a sheet (v1: a uniform grid, so it drops straight into the existing
   sprite-sheet animation path; v2: a shelf/rect pack for tight atlases).
3. **Emits two products**: (a) the packed sheet, handed to `ConvertImageObject` so it
   becomes a `StreamingImageAsset`; and (b) a small **Diorama sprite-sheet metadata**
   product (grid dimensions or per-frame rects, per-frame durations as fps, and tags
   as named clips) that a Sprite component references. The metadata product carries a
   `ProductDependency` on the streaming image.

## Integration with existing features

- v1's uniform-grid output plugs directly into the current flipbook animation
  (`PlaySpriteSheet`, the grid + fps + loop config), so import works end to end with
  no new runtime code.
- v2's named tags become named clips, which pairs with the clip authoring in
  [Docs/design/2d-editor-authoring.md](2d-editor-authoring.md), and non-uniform frame
  rects pair with free-form atlas slicing there.

## Security and performance

- Every header field, chunk size, frame/layer/cel count, and pixel-data length is
  bounded and validated before use; a malformed file fails the job cleanly rather
  than over-reading (the VISION untrusted-asset criterion, enforced in the builder).
- Compression is delegated to the proven image pipeline; we add no per-frame runtime
  cost (output is an ordinary streaming image + a tiny metadata asset).

## Phasing

1. **v1**: flattened frames -> uniform sheet + grid/fps metadata (works with current
   animation).
2. **v2**: tags -> named clips, per-frame durations, non-uniform rect packing.
3. **v3**: layer and slice import (expose Aseprite slices as named sub-sprites).

## Verification plan

- Import a multi-frame, tagged `.aseprite`: the sheet builds, a Sprite plays the
  frames at the authored fps, and a named tag plays as a clip.
- A truncated/garbage `.ase` fails the job with a clear error and no crash.

## Open questions

- Whether the metadata product is a new reflected asset type or reuses an existing
  serialized struct on the Sprite config; lean toward a small dedicated asset so it
  can be referenced and shared.
- Multi-layer blend-mode fidelity (v3): match Aseprite's normal/multiply/etc. or
  flatten with normal blending in v1/v2.
