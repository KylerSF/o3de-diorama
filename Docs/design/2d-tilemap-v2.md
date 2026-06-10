# Tilemap v2: rule tiles, animated tiles, per-tile collision

Status: rule tiles **shipped**; per-tile collision **shipped** (greedy-mesh core +
runtime collider registration); animated tiles **designed**.

The base tilemap (atlas grid, multi-layer asset + builder, Tiled import, paint
tool, 4-bit and 47-blob autotiling, per-tile flip/rotate) is in. v2 closes the
three remaining items the roadmap called out.

## 1. Custom rule tiles (shipped)

The built-in autotiling assumes the art block is laid out in the gem's canonical
order (16 cells in edge-mask order, or 47 in blob order). An imported tileset is
often laid out differently, so the canonical index points at the wrong art. Rule
tiles let an author map each meaningful neighborhood to its own cell.

- A pure `RuleEntry { mask, offset }` and `RuleSetOffset(mask8, rules)` in
  [`TilemapAutotile.h`](../../Code/Source/Clients/TilemapAutotile.h): the neighbor
  mask is normalized the same way the blob scheme normalizes it (so an author lists
  only the 47 meaningful neighborhoods, not 256 raw masks), then matched against the
  rules in order; the first match's offset wins, else it falls back to the canonical
  blob index. Pure and unit tested.
- The rules live on the tilemap config (`m_autotileRules`, authored in the
  Inspector under "Autotiling"), and the `AutotileRules(baseTileIndex)` bus verb
  applies them, rewriting the integer grid exactly like `Autotile`/`AutotileBlob`.
  No rendering change, so it is fully exercised by the unit tests and the verify
  loop (read cells back through `GetTilemapInfo` / the tile getters).

## 2. Per-tile collision (core shipped; runtime pending)

A tile world should collide without hand-placing colliders. The expensive naive
approach is one collider per solid cell; the right approach is to merge runs of
solid cells into a few large boxes.

- The merge is a pure greedy mesh in
  [`TilemapCollision.h`](../../Code/Source/Clients/TilemapCollision.h):
  `MergeSolidCells(columns, rows, isSolid)` grows each unclaimed solid cell right
  then down into a maximal rectangle, producing a small, deterministic set of
  `CellBox`es; `ToWorldBox` turns a cell box into a centered world AABB. Pure and
  unit tested (a full grid collapses to one box; an L-shape covers exactly the solid
  cells with no overlap).
- **Runtime (shipped):** the config gains a **Solid Tiles** set (atlas indices that
  collide) and a **Collision Layer**. At activate (and whenever the tiles change) the
  runtime tilemap greedy-meshes its solid cells, converts each merged `CellBox` to a
  box `Collision2D::Collider` in the world X,Z plane (`BuildTilemapColliders`, unit
  tested), and registers the set with the collision world via the new
  `SetStaticColliders(owner, boxes)` path. These are **query-only static geometry**:
  they participate in overlap / raycast / push-out (so a moving collider blocks against
  the map) but do not generate per-box contact-begin/stay/end notifications, which keeps
  the change small and avoids spawning one entity per region. Assumes the tilemap is
  axis-aligned in the X,Z collision plane (its default orientation). A future option:
  opt-in contact events, and an oriented-plane mode for rotated maps.

## 3. Animated tiles (designed)

Some cells should cycle through frames (water, torches, coins). The plan:

- A config list of animated-tile definitions: a key atlas index -> a sequence of
  atlas indices + an fps (and optional per-frame durations), plus a loop flag.
- A pure frame-selection core (given elapsed time and the sequence, pick the cell),
  unit testable like the rest.
- The presenter, each tick, overrides the UV of any cell whose value is an animated
  key with the current frame's atlas cell. This is a render-path change, so it lands
  with the on-screen verification pass.

## Why split this way

Rule tiles are pure grid math, so they ship and verify headlessly today, exactly
like the existing autotile verbs. Per-tile collision's geometry is equally pure and
shipped/tested; only the collider *spawning* is a runtime behaviour. Animated tiles
are inherently a render-path feature. The pure cores land now (green, tested); the
runtime/visual wiring lands with a monitor, consistent with how every visual
Diorama feature reaches "fully done."
