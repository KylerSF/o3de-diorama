# Changelog

All notable changes to Diorama are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/). While in
alpha (the 0.x line), minor releases may include breaking changes.

## [Unreleased]

### Added
- Gem-native 2D collision reachable from gameplay scripts. A `2D Collider` component
  (circle or box, layer/mask matrix, trigger flag) and a collision system that
  detects overlaps each tick and dispatches contact and trigger events through
  buses reflected at `Common` scope, so launcher Lua, Python, and Script Canvas all
  receive them: `Diorama2DCollisionNotificationBus` (OnContactBegin/Stay/End,
  OnTriggerEnter/Exit), the per-collider `Diorama2DColliderRequestBus`, and the
  global `Diorama2DCollisionRequestBus` (OverlapCircle, OverlapBox, Raycast2D). This
  is the launcher-reachable contact path PhysX's collision bus is not (it is
  Automation-scope only). Decoupled from PhysX, with a configurable collision plane
  (XY / XZ / YZ, default the XY screen plane) so it matches whichever plane a 2.5D
  game moves on. The collision math (overlap, raycast, sort-and-sweep broadphase,
  begin/stay/end tracking) is a pure, header-only core; unit tests plus
  component/system integration tests cover it, and the twin-stick sample uses it for
  befriend-on-contact (validated in a running GameLauncher).
- Pure, header-only math cores for the upcoming camera and particle features, the
  parts verifiable without a viewport: frame-rate-independent camera follow,
  deadzone, bounds, lookahead, pixel snap, and trauma-based screen shake; and
  per-particle integration (velocity, gravity, drag), aging, and size/color over
  life with a pooled step. Both unit tested.
- Post-alpha feature roadmap ([Docs/roadmap.md](Docs/roadmap.md)) and a set of
  vetted design docs under [Docs/design/](Docs/design/) for the next 2D/2.5D
  features: dynamic lighting, collision-from-scripts, post-processing, per-sprite
  materials, a 2D camera component, particles, tilemap painting/autotiling,
  skeletal animation, Aseprite import, an in-editor slicer, a starter template, and
  an editor live-preview audit. Each is grounded in engine investigation with a
  chosen approach, security/performance notes, and a phased plan.
- Tween / easing library for gameplay scripts
  (`Assets/Diorama/Scripts/tween.lua`): a dependency-free helper that animates a
  value over time with easing (linear, quad, cubic, sine, back, elastic, bounce),
  with a per-owner tween group you update each tick. Pure logic, so it behaves
  identically in the launcher and editor; useful for size/tint/position pops,
  fades, and camera moves without hand-rolled per-frame code.
- Hello Sprite (rung 1) and Animated Sprite (rung 2) how-to guides with runnable
  examples, completing the written teaching ladder (rungs 1-6). Rung 1 builds a
  single sprite through the typed bus; rung 2 plays the 2x2 sample atlas as a
  four-frame sheet via `PlaySpriteSheet`.
- Parallax and Layers guide (teaching ladder rung 5): how to order overlapping
  sprites and tilemaps into layers with Sort Offset, and a reusable parallax
  script (`Assets/Diorama/Scripts/parallax_layer.lua`) that scrolls a layer at a
  fraction of the camera's motion for a 2.5D depth effect. Includes a runnable
  example that builds background/midground/foreground layers.
- Twin-stick shooter sample game (teaching ladder rung 6): a complete 2.5D "kill
  them with kindness" game. You play Obi, a jolly octopus who fires hearts at
  ocean "haters" (nine species spawned in ramping waves from the arena edges);
  three hearts fill a hater with love and it floats away happy. It ships a tilted
  2.5D camera over an ocean-floor tilemap, a LyShine "Befriended" HUD composited
  over the Diorama world layer, pause (P) / quit (Esc, gamepad Start) controls, a
  colour-shifting octopus (chromatophores), and a pooled heart-burst FX manager.
  Player, chase AI, hearts, and the wave spawner are all transform-driven Lua over
  the Diorama buses (see Changed for why no PhysX), so ordinary gameplay drives the
  same AI-native bus an agent would.
- 2.5D rendering in the sprite feature processor: automatic camera-distance depth
  sort, so nearer sprites draw in front within their sort layer
  (`r_dioramaSpriteDepthSort`); and soft ground shadows under billboarded sprites
  (`r_dioramaSpriteShadows`, opacity `r_dioramaShadowAlpha`), drawn in one batch
  between the floor and the sprites.
- Tilemap component (teaching ladder rung 4): a world-space grid of cells, each
  drawing one cell of a shared atlas, rendered through the same batched sprite
  feature processor so a whole layer collapses into one draw call. Authored in
  the editor inspector (atlas/grid/appearance) and driven by an AI-native
  `DioramaTilemapRequestBus` (SetGridSize, SetAtlasGrid, SetTile, Fill, Clear,
  SetTileSize, SetTint, SetSortOffset, GetTilemapInfo) with named, documented,
  clamped verbs. Runtime and editor components share one config and presenter,
  so it draws identically in game and in the editor viewport. Unit-tested tile
  UV / grid layout math and EBus dispatch; how-to guide and runnable example.
- Sprite-sheet (flipbook) animation on the Sprite component: a uniform frame
  grid (columns, rows, frame count) played on a timer, with frames-per-second,
  looping or hold-on-last, and a start frame. Frames compose with the existing
  atlas UV sub-region and flips. Driven through the shared presenter so it plays
  identically at runtime and in the editor viewport.
- Unit tests for per-frame UV regions and the frame-advance logic.
- How-to guide and runnable example for the Sprite Atlas (teaching ladder rung
  3): sharing one atlas texture across sprites via UV sub-regions, with a
  scripted four-cell example scene and a how-to index.
- AI-native sprite API (`DioramaSpriteRequestBus`): a typed, reflected,
  queryable EBus so an agent can drive a sprite with plain scalar verbs
  (SetTextureByPath, SetSize, SetUVRegion, SetSortOffset, SetDoubleSided,
  PlaySpriteSheet, ...) instead of brittle EditContext property-path strings,
  plus a GetSpriteInfo query and a notification bus. Reflected to the
  BehaviorContext (callable from Python as `azlmbr.diorama`), with a runnable
  example (`Docs/examples/sprite_via_ai_bus.py`). See
  `Docs/design/ai-sprite-api.md`. Verb logic and GetSpriteInfo are unit tested;
  Python reachability and the GetSpriteInfo readback are confirmed. End-to-end
  editor-scripted verb application still depends on the sprite component being
  active, which the current editor `--runpython` flow does not guarantee; that
  activation path is the remaining follow-up.
- Per-argument names and descriptions on every `DioramaSpriteRequestBus` verb,
  reflected to the BehaviorContext. These name and document each argument for
  Script Canvas node pins and for any tool that introspects the reflection to
  build an agent-facing API schema. (The editor's generated Python stub lists
  EBus arguments by type only, so the names live in the C++ reflection rather
  than the `.pyi`.)
- Per-sprite "Double Sided" toggle (default on): a flat sprite is visible from
  both sides, matching how 2D sprite renderers behave elsewhere. Implemented
  geometrically (the renderer emits back-facing triangles for double-sided,
  non-billboard sprites) so it needs no separate shader and does not affect
  batching. Turn it off to hide a sprite when viewed from behind.
- Build/test CI: a self-hosted `build-test` workflow that compiles the gem
  through a host O3DE project and runs the unit tests, plus a reusable
  `scripts/ci_build_test.sh` and a self-hosted runner setup guide. Complements
  the always-on lint workflow.

### Changed
- Twin-stick gameplay reworked from PhysX to transform-driven movement with
  distance-based hit detection. Two engine facts drove it: a dynamic PhysX rigid
  body ignores the entity's authored editor transform at simulation start (it
  initializes at the world origin), and PhysX collision callbacks are reflected
  only in the Automation script scope, so a launcher Lua script never receives
  `OnCollisionBegin`. Entities now move via `TransformBus`, stay in bounds by a
  clamp, and detect hits by distance through a shared live-entity table. Kills are
  replaced by a "befriend" mechanic (three hearts and a hater floats away); the
  HUD counts befriended haters via a shared global rather than a cross-entity
  `GameplayNotification`.
- The sprite shader disables the depth test and back-face culling, so a flat floor
  renders correctly under a tilted 2.5D camera and draw order (painter's, by sort
  layer then camera distance) is authoritative.
- Sprite rendering now goes through a proper Atom scene feature processor
  (`SpriteFeatureProcessor`) instead of an immediate-mode per-sprite draw loop.
  Sprites that share a texture and sort layer are batched into a single draw call
  with one shader resource group, which scales to large sprite counts. The
  immediate-mode `SpriteRenderer` and its request bus are removed; components
  register with the scene's feature processor through the shared presenter, which
  retries scene resolution per tick so sprites whose scene is not ready at
  activation still appear once it is.
- Batching keys on the full texture asset id (no lossy hash) and a float sort
  offset (fractional 2.5D layers stay distinct), uses 32-bit indices (batches
  may exceed 16384 sprites), reuses per-frame scratch buffers, and caches each
  sprite's batch key, so the render loop performs no per-frame heap allocation.
- The batch plan is dirty-tracked: the grouping and sort are rebuilt only when a
  sprite is added, removed, or its texture/sort layer changes, so a static scene
  skips the per-frame scan and sort (vertex packing and submission still run each
  frame). Transform, animation, and texture-streaming changes do not trigger a
  rebuild.
- Unit tests for the batch-planning logic (grouping, sort-offset ordering,
  fractional-layer separation, invalid-texture handling, stable in-batch order,
  and buffer reuse).

### Fixed
- The 2.5D floor vanished under a pitched camera. Root-caused (by reading the
  engine source) to the sprite shader's depth test, not back-face culling and not
  frustum culling; disabling the depth test is the correct model for this
  draw-order-ordered sprite renderer, and the floor now renders under the tilt.
- The score HUD never updated. The reflected Lua name `UiCanvasBus.FindElementByName`
  is bound to `FindElementEntityIdByName` and returns the element's EntityId
  directly (so `:GetId()` on it was wrong); the count now also flows through a
  shared global polled by the HUD controller, matching how the rest of the sample
  communicates.
- Per-spawn "property not found" log spam from reading `.lua` properties that the
  runtime `ScriptComponent` has no baked value for (it does not apply `.lua`
  defaults). Uniform game constants moved to script locals; only genuinely
  per-instance values stay as baked, tunable properties.
- Deadlock that froze the editor (and would freeze a runtime client) the first
  frame a sprite became drawable. `SpriteFeatureProcessor::Render` runs as a
  render job the main thread blocks on, and it loaded the sprite shader with a
  blocking call (`LoadShader` -> `BlockUntilLoadComplete`); the main thread
  waited for the job while the job waited for the asset. The shader now loads
  asynchronously in `Activate`, and the per-frame init only polls readiness, so
  nothing blocks inside the render job. Verified on screen: sprites render and
  the editor stays responsive.

### Removed
- Orphaned pre-theme creature prefabs (Shark, Bird, Fish, Whale, and a generic
  Enemy base) and the unused shark texture, left over from before the ocean-hater
  cast. Nothing referenced them; the live cast is generated from one creature list.

### Contributed upstream
- Filed o3de/o3de#19804 with a fix in PR #19805: a use-after-free in
  `AzFramework::ScriptComponent::DestroyEntityTable` when a Lua-scripted entity is
  torn down during shutdown after the script context (and its `lua_State`) has been
  freed, found while testing the sample's quit path. Diorama complements the engine
  and contributes such findings back rather than patching around them.

## [0.1.0-alpha] - 2026-05-29

First alpha. World-space sprite rendering through Atom, with editor tooling and
the runtime/editor module split in place. The `gem.json` version tracks the
`0.1.0` release line; this git tag carries the `-alpha` pre-release marker.

### Added
- Diorama gem: world-space 2D/2.5D sprite rendering through Atom, with a
  lightweight runtime client module and a separate Qt editor module.
- Sprite component that renders a world-space quad via Atom dynamic draw.
- Editor viewport preview of sprites through a shared presenter.
- Atlas UV sub-regions (`UV Min` / `UV Max`), horizontal and vertical flip, and a
  2.5D sort offset for draw-order control.
- Unit tests covering sprite UV region and flip mapping (`SpriteUVTest`).
- Project documentation: README, VISION, examples/how-to outline, and license.

### Fixed
- Static `AZ::Name` initialization-order crash in `SpriteRenderer`. A
  namespace-scope `AZ::Name` constructed during shared-library load, before the
  AzCore `NameDictionary` existed, causing a SIGSEGV when the test library was
  loaded ahead of engine bootstrap. It is now constructed lazily on first use.

[Unreleased]: https://github.com/nickschuetz/o3de-diorama/compare/v0.1.0-alpha...HEAD
[0.1.0-alpha]: https://github.com/nickschuetz/o3de-diorama/releases/tag/v0.1.0-alpha
