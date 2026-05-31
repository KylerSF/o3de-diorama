"""
Build a 2.5D scene of depth-sorted parallax layers with Diorama sprites.

Layering in Diorama is just the Sort Offset (draw order within transparent
rendering) plus where a sprite sits in depth. Parallax -- background layers
moving slower than the camera -- is a small reusable script (parallax_layer.lua)
on top of that. This example creates a far background, a midground, and a
foreground layer, each with its own sort offset, and explains how to attach the
parallax script.

Run in the editor:
  <engine>/bin/Linux/profile/Default/Editor \
    --project-path=/path/to/YourProject \
    --runpython /path/to/o3de-diorama/Docs/examples/parallax_layers.py
"""
import azlmbr
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity
import azlmbr.legacy.general as general
import azlmbr.math as math

diorama = azlmbr.diorama

TEXTURE = "diorama/textures/atlas_2x2.png"

# (name, sortOffset, depthY, tint, parallaxFactor). Lower sort draws first
# (behind); higher draws on top. Far background follows the camera most
# (factor 0.1), the foreground is fixed in the world (factor 1.0).
LAYERS = [
    ("Background", 0.0, 6.0, (0.4, 0.5, 0.9, 1.0), 0.1),
    ("Midground", 5.0, 3.0, (0.6, 0.8, 0.6, 1.0), 0.5),
    ("Foreground", 10.0, 0.0, (1.0, 1.0, 1.0, 1.0), 1.0),
]


def log(msg):
    print("DIORAMA_PARALLAX: {}".format(msg))


def find_type_id(name):
    ids = editor.EditorComponentAPIBus(bus.Broadcast, "FindComponentTypeIdsByEntityType", [name], 0)
    if ids and len(ids) > 0 and not ids[0].IsNull():
        return ids[0]
    return None


def main():
    log("start")
    general.idle_enable(True)
    try:
        general.open_level_no_prompt("DefaultLevel")
    except Exception as e:
        log("open_level raised: {}".format(e))
    waited = 0
    while general.get_current_level_name() in ("", "Untitled") and waited < 400:
        general.idle_wait_frames(1)
        waited += 1
    general.idle_wait_frames(20)

    if find_type_id("Sprite") is None:
        log("FAIL: Diorama Sprite component not found (is the Diorama gem enabled?)")
        return
    sprite_type = find_type_id("Sprite")

    for name, sort_offset, depth_y, tint, parallax in LAYERS:
        eid = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", math.Vector3(0.0, depth_y, 2.0), azlmbr.entity.EntityId())
        editor.EditorEntityAPIBus(bus.Event, "SetName", eid, name)
        editor.EditorComponentAPIBus(bus.Broadcast, "AddComponentsOfType", eid, [sprite_type])

        diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", eid, TEXTURE)
        diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", eid, 8.0, 8.0)
        diorama.DioramaSpriteRequestBus(bus.Event, "SetTint", eid, tint[0], tint[1], tint[2], tint[3])
        diorama.DioramaSpriteRequestBus(bus.Event, "SetSortOffset", eid, sort_offset)

        info = diorama.DioramaSpriteRequestBus(bus.Event, "GetSpriteInfo", eid)
        log("layer '{}' sortOffset={} (parallax factor {} -> attach parallax_layer.lua)".format(
            name, info.sortOffset, parallax))

    log("Sprites and sort order are set. For parallax: add a Lua Script component")
    log("to each layer with diorama/scripts/parallax_layer.lua, set its Camera to")
    log("your camera entity, and set ParallaxFactor per layer (0.1 / 0.5 / 1.0).")
    log("done")


main()
