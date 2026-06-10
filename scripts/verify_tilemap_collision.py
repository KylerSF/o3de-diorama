"""
In-editor verification that per-tile collision boxes land on the visible tiles.

Builds an UN-ROTATED 5x5 tilemap at the origin whose border tiles are solid (index 0)
and interior empty (-1), enters game mode (so the runtime TilemapComponent activates
and registers its static colliders), then queries the 2D collision world at the exact
world positions where each cell's center should be. For an un-rotated tilemap at the
origin, a cell's world center equals GetTileLocalPosition (the same convention the
renderer uses), so a hit at a solid cell proves the collider sits on the visible tile.

PASS = every solid probe hits and every empty probe misses. Run via --runpython
(the console panel is avoided). Results print to user/log/Editor.log as
DIORAMA_COLCHECK lines.
"""
import json

import azlmbr
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity
import azlmbr.legacy.general as general
import azlmbr.math as math
import azlmbr.paths

diorama = azlmbr.diorama

LEVEL_NAME = "DioramaColCheck"
COLS, ROWS = 5, 5
TILE = 1.0  # default tile size


def log(msg):
    print("DIORAMA_COLCHECK: {}".format(msg))


def find_type_id(name):
    ids = editor.EditorComponentAPIBus(bus.Broadcast, "FindComponentTypeIdsByEntityType", [name], 0)
    if ids and len(ids) > 0 and not ids[0].IsNull():
        return ids[0]
    return None


def open_or_create_level(level_name):
    general.idle_enable(True)
    booted = 0
    while general.get_current_level_name() in ("", "Untitled") and booted < 600:
        general.idle_wait_frames(1)
        booted += 1
    general.idle_wait_frames(30)

    def now_in(name):
        return general.get_current_level_name() == name

    try:
        general.open_level_no_prompt(level_name)
    except Exception:
        pass
    waited = 0
    while not now_in(level_name) and waited < 200:
        general.idle_wait_frames(1)
        waited += 1
    if not now_in(level_name):
        for template in ("Default_Level", "Empty", "Basic"):
            try:
                general.create_level_no_prompt(template, level_name, 128, 1, 512, False)
            except Exception:
                continue
            waited = 0
            while not now_in(level_name) and waited < 400:
                general.idle_wait_frames(1)
                waited += 1
            if now_in(level_name):
                break
    general.idle_wait_frames(30)
    return now_in(level_name)


def border_tiles():
    # Row-major 5x5: 0 = solid on the border, -1 = empty interior.
    tiles = []
    for r in range(ROWS):
        for c in range(COLS):
            edge = (c == 0 or r == 0 or c == COLS - 1 or r == ROWS - 1)
            tiles.append(0 if edge else -1)
    return tiles


def cell_world(col, row):
    # GetTileLocalPosition convention: columns along +X, rows along -Z, centered.
    x = (col - (COLS - 1) / 2.0) * TILE
    z = -(row - (ROWS - 1) / 2.0) * TILE
    return x, z


def main():
    log("start")
    if not open_or_create_level(LEVEL_NAME):
        log("FAIL: could not open/create level")
        return
    general.idle_wait_frames(20)

    tilemap_type = find_type_id("Tilemap")
    if tilemap_type is None:
        log("FAIL: Tilemap component not found (rebuild gem?)")
        return

    eid = editor.ToolsApplicationRequestBus(
        bus.Broadcast, "CreateNewEntityAtPosition", math.Vector3(0.0, 0.0, 0.0), azlmbr.entity.EntityId())
    editor.EditorEntityAPIBus(bus.Event, "SetName", eid, "Walls")
    editor.EditorComponentAPIBus(bus.Broadcast, "AddComponentsOfType", eid, [tilemap_type])

    general.idle_wait_frames(20)
    general.save_level()

    # Bake the tilemap config (grid + tiles + solid set) into the prefab; the nested
    # arrays cannot be set through the editor component API.
    try:
        proj = str(azlmbr.paths.projectroot)
        pf = "{}/Levels/{}/{}.prefab".format(proj, LEVEL_NAME, LEVEL_NAME)
        with open(pf) as f:
            doc = json.load(f)
        patched = 0
        for entity in doc.get("Entities", {}).values():
            if entity.get("Name") != "Walls":
                continue
            for comp in entity.get("Components", {}).values():
                if "EditorTilemapComponent" in comp.get("$type", ""):
                    comp["Config"] = {
                        "columns": COLS,
                        "rows": ROWS,
                        "atlasColumns": 1,
                        "atlasRows": 1,
                        "tiles": border_tiles(),
                        "solidTiles": [0],
                        "collisionLayer": 1,
                    }
                    patched += 1
        with open(pf, "w") as f:
            json.dump(doc, f, indent=4)
        general.open_level_no_prompt(LEVEL_NAME)
        general.idle_wait_frames(20)
        log("prefab patched: {} tilemap config(s)".format(patched))
    except Exception as e:
        log("FAIL: prefab patch failed ({})".format(e))
        return

    # Enter game mode so the runtime TilemapComponent activates + registers colliders.
    general.enter_game_mode()
    general.idle_wait_frames(60)

    probes = [
        ("corner(0,0) solid", cell_world(0, 0), True),
        ("top-mid(2,0) solid", cell_world(2, 0), True),
        ("left-mid(0,2) solid", cell_world(0, 2), True),
        ("center(2,2) empty", cell_world(2, 2), False),
        ("inner(2,3) empty", cell_world(2, 3), False),
    ]
    passed = 0
    for label, (x, z), expect_hit in probes:
        hits = diorama.Diorama2DCollisionRequestBus(bus.Broadcast, "OverlapBox", x, z, 0.3, 0.3, 1)
        n = len(hits) if hits is not None else 0
        ok = (n > 0) == expect_hit
        passed += 1 if ok else 0
        log("probe {} at ({:.1f},{:.1f}) hits={} expect_hit={} -> {}".format(label, x, z, n, expect_hit, "OK" if ok else "MISMATCH"))

    general.exit_game_mode()
    general.idle_wait_frames(10)

    log("RESULT: {}/{} probes correct -> {}".format(passed, len(probes), "PASS" if passed == len(probes) else "FAIL"))
    log("done")


main()
