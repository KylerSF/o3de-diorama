"""
Sprite Atlas example (teaching ladder rung 3).

Builds a scene with four Diorama sprites in a row, all sharing the single
texture atlas Assets/Diorama/Textures/atlas_2x2.png, each sampling a different
cell through the Atlas / UV Region fields. The result is a red, green, blue, and
yellow quad side by side, drawn from one shared texture.

Run in the editor:
  <engine>/bin/Linux/profile/Default/Editor \
    --project-path=/path/to/YourProject \
    --runpython /path/to/o3de-diorama/Docs/examples/sprite_atlas.py

This is a documentation example, not a test. It pairs with
Docs/howto/03-sprite-atlas.md.
"""
import azlmbr
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity
import azlmbr.legacy.general as general
import azlmbr.math as math
import azlmbr.asset


# Each atlas cell: a display name, world X offset, and its UV sub-rectangle.
# UV Min is the cell's top-left, UV Max its bottom-right, normalized 0..1.
CELLS = [
    ("Red", -3.0, (0.0, 0.0), (0.5, 0.5)),
    ("Green", -1.0, (0.5, 0.0), (1.0, 0.5)),
    ("Blue", 1.0, (0.0, 0.5), (0.5, 1.0)),
    ("Yellow", 3.0, (0.5, 0.5), (1.0, 1.0)),
]

ATLAS_PRODUCT_PATH = "diorama/textures/atlas_2x2.png.streamingimage"


def log(msg):
    print("DIORAMA_ATLAS_EXAMPLE: {}".format(msg))


def find_type_id(name):
    ids = editor.EditorComponentAPIBus(bus.Broadcast, "FindComponentTypeIdsByEntityType", [name], 0)
    if ids and len(ids) > 0 and not ids[0].IsNull():
        return ids[0]
    return None


def make_entity(name, pos):
    eid = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntityAtPosition", pos, azlmbr.entity.EntityId())
    if eid is not None and eid.IsValid():
        editor.EditorEntityAPIBus(bus.Event, "SetName", eid, name)
    return eid


def add_component(eid, type_id):
    outcome = editor.EditorComponentAPIBus(bus.Broadcast, "AddComponentsOfType", eid, [type_id])
    if outcome.IsSuccess():
        return outcome.GetValue()[0]
    return None


def set_prop(comp, path, value):
    editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", comp, path, value)


def main():
    log("start")

    sprite_type = find_type_id("Sprite")
    if sprite_type is None:
        log("FAIL: Sprite component type not found (is the Diorama gem enabled?)")
        return

    atlas_id = azlmbr.asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", ATLAS_PRODUCT_PATH, math.Uuid(), False)

    for name, x, uv_min, uv_max in CELLS:
        entity = make_entity("AtlasSprite_{}".format(name), math.Vector3(x, 0.0, 0.0))
        comp = add_component(entity, sprite_type)
        if comp is None:
            log("FAIL: could not add Sprite component to {}".format(name))
            continue
        set_prop(comp, "Config|Texture", atlas_id)
        set_prop(comp, "Config|Atlas / UV Region|UV Min", math.Vector2(uv_min[0], uv_min[1]))
        set_prop(comp, "Config|Atlas / UV Region|UV Max", math.Vector2(uv_max[0], uv_max[1]))
        log("placed {} sprite at x={}".format(name, x))

    log("done: four sprites share {} (one texture, four regions)".format(ATLAS_PRODUCT_PATH))


main()
