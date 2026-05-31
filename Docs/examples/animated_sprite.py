"""
Animated Sprite: play frames from a sprite sheet.

Treats the 2x2 sample atlas as a four-frame sprite sheet and plays it on a loop
through the Sprite component's animation verbs, then confirms with GetSpriteInfo.
This is teaching ladder rung 2.

Run in the editor:
  <engine>/bin/Linux/profile/Default/Editor \
    --project-path=/path/to/YourProject \
    --runpython /path/to/o3de-diorama/Docs/examples/animated_sprite.py
"""
import azlmbr
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity
import azlmbr.legacy.general as general
import azlmbr.math as math

diorama = azlmbr.diorama


def log(msg):
    print("DIORAMA_ANIM: {}".format(msg))


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

    sprite_type = find_type_id("Sprite")
    if sprite_type is None:
        log("FAIL: Sprite component type not found (is the Diorama gem enabled?)")
        return

    eid = editor.ToolsApplicationRequestBus(
        bus.Broadcast, "CreateNewEntityAtPosition", math.Vector3(0.0, 0.0, 1.0), azlmbr.entity.EntityId())
    editor.EditorEntityAPIBus(bus.Event, "SetName", eid, "AnimatedSprite")
    editor.EditorComponentAPIBus(bus.Broadcast, "AddComponentsOfType", eid, [sprite_type])

    diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", eid, "diorama/textures/atlas_2x2.png")
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", eid, 3.0, 3.0)

    # Play the 2x2 sheet as 4 frames at 4 fps, looping. PlaySpriteSheet sets the
    # grid, the playback rate, and enables animation in one call.
    columns, rows, frame_count, fps, loop = 2, 2, 4, 4.0, True
    diorama.DioramaSpriteRequestBus(bus.Event, "PlaySpriteSheet", eid, columns, rows, frame_count, fps, loop)

    info = diorama.DioramaSpriteRequestBus(bus.Event, "GetSpriteInfo", eid)
    log("animEnabled={} frameCount={} currentFrame={}".format(
        info.animEnabled, info.frameCount, info.currentFrame))
    log("done")


main()
