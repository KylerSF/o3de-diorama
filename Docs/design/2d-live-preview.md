# Design: editor live-preview (verify and close the gaps)

Status: design / audit (Tier-3 roadmap item). This one is mostly already done; the
work is to verify it and correct the record.

## The claim, re-examined

The README and an old phase-2a note say the editor preview "does not yet
live-update to every runtime property change." Reading the current code, that is
largely **stale**:

- Both editor components wire it. `EditorSpriteComponent` (and
  `EditorTilemapComponent`) attach `ChangeNotify -> OnConfigChanged` on the config
  element, and `OnConfigChanged` calls `m_presenter.SetConfig(m_config)`
  (`Code/Source/Tools/EditorSpriteComponent.cpp:123`,
  `EditorTilemapComponent.cpp:122`).
- `SpritePresenter::SetConfig` re-queues the texture if it changed, resets the
  animation, refreshes the tick connection, and calls `Push`, which forwards the
  whole config to the feature processor via `UpdateSprite`
  (`Code/Source/Clients/SpritePresenter.cpp:98-114,152-171`).
- The feature processor reads the stored config every frame in `Render`/`AppendQuad`,
  so any render-affecting property (size, UVs, flips, billboard, double-sided, sort
  offset, tint, animation grid/fps) takes effect immediately.
- The config has no per-field `ChangeNotify` that would shadow the parent handler, so
  nested-field edits bubble up to `OnConfigChanged`.
- Request-bus (agent/script) edits also call `SetConfig` and broadcast
  `InvalidatePropertyDisplay`, so the inspector and preview stay in sync both ways.

So the basic live-preview path exists and is correct by construction for both
components.

## The actual remaining work

Not a redesign, a **verification pass plus targeted close-out**:

1. **Verify on screen** that each Sprite and Tilemap property updates the viewport
   live when edited in the inspector: texture, size, pivot, UV min/max, flips,
   billboard, double-sided, sort offset, tint, and the animation fields; and for the
   tilemap, grid size, atlas grid, tile size, tint, sort offset.
2. **Close any specific gap found.** Candidates, if any property does not update:
   - a property whose change needs more than `UpdateSprite` (none is apparent today,
     since the processor re-reads the full config each frame);
   - a nested field that does not bubble its change to `OnConfigChanged` (would need
     its own `ChangeNotify` forwarding to the parent);
   - an asset field whose async reload is not re-queued (the texture path already is;
     a normal map / dissolve map added by the materials and lighting work must follow
     the same `textureChanged -> QueueTextureLoad` pattern in `SetConfig`).
3. **Correct the record.** Update the README's known-limitation line and the phase-2a
   memory note to reflect verified behavior (it live-updates; list any genuinely
   remaining exception, if one is found).

## Forward-looking guardrail

As lighting (normal map), materials (dissolve map, effect params), and skeletal
meshes add new config fields, each new **asset reference** must extend
`SpritePresenter::SetConfig`'s change detection (re-queue load on change), and each
new field must be covered by the verification checklist above. This keeps the
live-preview promise true as the config grows, rather than silently regressing it.

## Verification plan (needs the editor)

- Edit every Sprite and Tilemap property in the inspector and confirm the viewport
  updates without re-selecting the entity or re-opening the level.
- Drive the same properties through the request bus and confirm both the viewport and
  the inspector values update.
- Add a new asset field (when lighting/materials land) and confirm it live-reloads,
  per the guardrail.

## Open questions

- Whether any property genuinely fails to live-update today (only on-screen
  verification can confirm); the code path suggests none for the current fields.
