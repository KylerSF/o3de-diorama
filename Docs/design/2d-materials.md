# Design: per-sprite materials and effects (flash, tint, dissolve, outline, blend)

Status: design (Tier-1 roadmap item). No implementation yet. Builds directly on the
existing sprite shader (`DioramaSprite.azsl`) and the batched feature processor.

## Goal

Make the common sprite effects a property, not hand-rolled Lua: hit **flash**,
**tint**/fill, **dissolve** (burn/spawn transitions), **hue-shift**, **outline**, and
**blend modes** (additive for glows/fire, multiply for shadows). Flash and outline
alone replace most of the "juice" code a game would otherwise write per project, and
the emissive path feeds bloom ([Docs/design/2d-post-processing.md](2d-post-processing.md)).

## The batching constraint (it shapes the design)

Sprites batch by texture + sort layer into one draw with one shader resource group.
A per-sprite *uniform* would break batching (one draw per sprite). So we split
effects by how they are fed:

1. **Per-vertex effects (stay batched).** Cheap, common effects are packed into the
   vertex stream, so every sprite in a batch can differ while still drawing in one
   call. We already pack a per-sprite color into the vertex. We add a small amount of
   per-vertex effect data:
   - **flash** (color + amount): lerp the sampled texel toward the flash color by
     amount. The hit-flash everyone needs.
   - **fill/tint** (already have tint; extend to a fill amount): solid-color fill for
     silhouettes/damage.
   These ride in extra vertex attributes (or packed bits), computed per sprite on the
   CPU in `AppendQuad`, and applied in the pixel shader. No batch split.

2. **Material-variant effects (may form their own batch).** Richer effects need extra
   textures or a different pipeline state, so they key into the batch key and group
   separately:
   - **dissolve**: sample a dissolve/noise map, `clip()` where noise > amount, with an
     emissive edge band for a burn look. Needs a dissolve texture (a shared one is
     fine, so it still batches across sprites that use it).
   - **outline**: detect alpha edges by sampling the texture a texel out in 4-8
     directions (needs the atlas texel size) and draw an outline color where the
     center is transparent but a neighbor is opaque. Width/color are per-vertex
     params; the taps are shader work.
   - **hue-shift / color grading**: a per-vertex hue angle, applied in HSV.

3. **Blend mode (extends the batch key).** Alpha, additive, and multiply are
   different pipeline blend states. The DynamicDrawContext binds the shader's blend
   state, so additive/multiply need a shader/pipeline variant. We add a blend-mode
   enum to the batch key so additive sprites batch among themselves, and select the
   matching draw-context variant. Default stays the current alpha blend, so nothing
   changes unless asked.

## Shader and data

- Extend `SpriteVertex` with a compact effect block (flash rgba-as-color + amount,
  fill amount, hue angle, outline width) packed to keep the vertex small; computed
  per sprite in `AppendQuad`.
- `DioramaSprite.azsl` `MainPS` applies, in order: sample -> hue-shift -> dissolve
  clip -> fill/flash lerp -> outline composite -> emissive add -> `* vertex color`.
  Each stage is a cheap no-op when its amount is 0, so unused effects cost nearly
  nothing.
- Material-variant effects (dissolve map, outline taps, blend mode) select a shader
  variant / draw-context so they do not burden the common path.
- AI-native surface on `DioramaSpriteRequestBus`: `SetFlash(color, amount)`,
  `SetFill(color, amount)`, `SetDissolve(amount)`, `SetHueShift(angle)`,
  `SetOutline(color, width)`, `SetBlendMode(mode)` - scalar, documented, clamped
  verbs, the same style as the existing sprite API. A hit flash becomes
  `SetFlash(white, 1)` then a tween of amount to 0 (the [tween lib](../../Assets/Diorama/Scripts/tween.lua)).

## Security and performance

- All effect amounts/widths/angles are clamped on the CPU before they reach a vertex
  or constant buffer; no asset value sizes a buffer.
- Per-vertex effects add no draws and no allocation; they ride the existing batch.
- Variant/blend-mode effects add at most a bounded number of extra batches (one per
  distinct variant + blend mode in use), logged if a cap is ever hit.

## Phasing

1. **v1**: flash + fill (per-vertex, stays batched) + the bus verbs. Covers the bulk
   of gameplay juice (hit flash, damage tint, spawn pop with the tween lib).
2. **v2**: dissolve + hue-shift (material params / shared dissolve map) and the
   emissive term (shared with lighting v3 and post bloom).
3. **v3**: outline (edge taps) and additive/multiply blend modes (batch-key variant).

## Verification plan (needs the monitor)

- Flash: `SetFlash(white,1)` whitens a sprite; tweening amount to 0 fades it back; many
  flashing sprites still draw in one batch (no batch-count spike).
- Dissolve: ramping amount burns the sprite away with a glowing edge.
- Outline: an alpha sprite gets a clean 1-2px outline that tracks its silhouette.
- Additive: a glow sprite brightens what is behind it; confirm it batches separately
  from alpha sprites and the counts are as expected.

## Open questions

- Vertex-size budget: how many effects to pack per-vertex before moving some to a
  per-batch constant; measure against the batch throughput goal.
- Outline quality from a packed atlas: edge taps must respect the sprite's UV
  sub-region so they do not sample a neighbor atlas cell; clamp taps to the region.
