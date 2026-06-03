"""
Hello Sprite: the smallest Diorama scene -- one world-space sprite.

Creates an entity, adds the Sprite component, points it at a texture, and sizes
it, all through the typed DioramaSpriteRequestBus, then confirms the result with
GetSpriteInfo. This is teaching ladder rung 1.

Run in the editor:
  <engine>/bin/Linux/profile/Default/Editor \
    --project-path=/path/to/YourProject \
    --runpython /path/to/o3de-diorama/Docs/examples/hello_sprite.py
"""
import azlmbr
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity
import azlmbr.legacy.general as general
import azlmbr.math as math

diorama = azlmbr.diorama


def log(msg):
    print("DIORAMA_HELLO: {}".format(msg))


def find_type_id(name):
    ids = editor.EditorComponentAPIBus(bus.Broadcast, "FindComponentTypeIdsByEntityType", [name], 0)
    if ids and len(ids) > 0 and not ids[0].IsNull():
        return ids[0]
    return None


LEVEL_NAME = "DioramaHelloSprite"


def open_or_create_level(level_name):
    """Open the named level, creating a fresh one from a built-in template if it
    does not exist, so the example builds in its OWN level and never lands on top
    of another scene (this project's DefaultLevel is the twin-stick sample)."""
    general.idle_enable(True)
    try:
        general.open_level_no_prompt(level_name)
    except Exception as e:
        log("open_level raised: {}".format(e))
    waited = 0
    while general.get_current_level_name() != level_name and waited < 200:
        general.idle_wait_frames(1)
        waited += 1
    if general.get_current_level_name() == level_name:
        return True
    # create_level_no_prompt(templateName, levelName, heightmapRes, unitSize,
    # terrainTexSize, useTerrain); Default_Level gives the standard Atom env.
    for template in ("Default_Level", "Empty", "Basic"):
        try:
            general.create_level_no_prompt(template, level_name, 128, 1, 512, False)
        except Exception as e:
            log("create_level('{}') raised: {}".format(template, e))
            continue
        waited = 0
        while general.get_current_level_name() != level_name and waited < 400:
            general.idle_wait_frames(1)
            waited += 1
        if general.get_current_level_name() == level_name:
            return True
    return False


def main():
    log("start")
    if not open_or_create_level(LEVEL_NAME):
        log("FAIL: could not open or create level '{}'".format(LEVEL_NAME))
        return
    general.idle_wait_frames(20)

    sprite_type = find_type_id("Sprite")
    if sprite_type is None:
        log("FAIL: Sprite component type not found (is the Diorama gem enabled?)")
        return

    eid = editor.ToolsApplicationRequestBus(
        bus.Broadcast, "CreateNewEntityAtPosition", math.Vector3(0.0, 0.0, 1.0), azlmbr.entity.EntityId())
    editor.EditorEntityAPIBus(bus.Event, "SetName", eid, "HelloSprite")
    editor.EditorComponentAPIBus(bus.Broadcast, "AddComponentsOfType", eid, [sprite_type])

    diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", eid, "diorama/textures/sample_sprite.png")
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", eid, 2.0, 2.0)

    info = diorama.DioramaSpriteRequestBus(bus.Event, "GetSpriteInfo", eid)
    log("texturePath={} size={}x{} textureLoaded={}".format(
        info.texturePath, info.width, info.height, info.textureLoaded))
    log("done")


main()
