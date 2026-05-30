# Changelog

All notable changes to Diorama are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/). While in
alpha (the 0.x line), minor releases may include breaking changes.

## [Unreleased]

### Added
- Sprite-sheet (flipbook) animation on the Sprite component: a uniform frame
  grid (columns, rows, frame count) played on a timer, with frames-per-second,
  looping or hold-on-last, and a start frame. Frames compose with the existing
  atlas UV sub-region and flips. Driven through the shared presenter so it plays
  identically at runtime and in the editor viewport.
- Unit tests for per-frame UV regions and the frame-advance logic.
- How-to guide and runnable example for the Sprite Atlas (teaching ladder rung
  3): sharing one atlas texture across sprites via UV sub-regions, with a
  scripted four-cell example scene and a how-to index.
- Build/test CI: a self-hosted `build-test` workflow that compiles the gem
  through a host O3DE project and runs the unit tests, plus a reusable
  `scripts/ci_build_test.sh` and a self-hosted runner setup guide. Complements
  the always-on lint workflow.

### Changed
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
- Deadlock that froze the editor (and would freeze a runtime client) the first
  frame a sprite became drawable. `SpriteFeatureProcessor::Render` runs as a
  render job the main thread blocks on, and it loaded the sprite shader with a
  blocking call (`LoadShader` -> `BlockUntilLoadComplete`); the main thread
  waited for the job while the job waited for the asset. The shader now loads
  asynchronously in `Activate`, and the per-frame init only polls readiness, so
  nothing blocks inside the render job. Verified on screen: sprites render and
  the editor stays responsive.

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
