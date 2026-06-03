"""
Drive a Diorama sprite entirely through the AI-native DioramaSpriteRequestBus,
then verify the result with GetSpriteInfo (no screenshot needed).

This is both a how-to for agents and a self-checking example: it creates an
entity, adds the Sprite component, configures it through typed verbs, and asserts
the resolved state. Contrast with the brittle pre-bus approach, which poked
nested EditContext property-path strings (Config|Atlas / UV Region|UV Min) with
no confirmation.

Run in the editor:
  <engine>/bin/Linux/profile/Default/Editor \
    --project-path=/path/to/YourProject \
    --runpython /path/to/o3de-diorama/Docs/examples/sprite_via_ai_bus.py
"""
import azlmbr
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity
import azlmbr.legacy.general as general
import azlmbr.math as math

# The Diorama buses are reflected under azlmbr.diorama (the bus's Module
# attribute). Access it as an attribute of azlmbr rather than `import
# azlmbr.diorama`, since azlmbr exposes submodules as attributes.
diorama = azlmbr.diorama


def log(msg):
    print("DIORAMA_AIBUS: {}".format(msg))


def find_type_id(name):
    ids = editor.EditorComponentAPIBus(bus.Broadcast, "FindComponentTypeIdsByEntityType", [name], 0)
    if ids and len(ids) > 0 and not ids[0].IsNull():
        return ids[0]
    return None


LEVEL_NAME = "DioramaSpriteViaAiBus"


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
    # A level must be open before creating entities (creating an entity with no
    # level/root prefab open asserts in the engine's prefab system). Build into our
    # own level so this never lands on top of another scene.
    if not open_or_create_level(LEVEL_NAME):
        log("FAIL: could not open or create level '{}'".format(LEVEL_NAME))
        return
    general.idle_wait_frames(20)

    sprite_type = find_type_id("Sprite")
    if sprite_type is None:
        log("FAIL: Sprite component type not found (is the Diorama gem enabled?)")
        return

    # Create an entity and add the Sprite component (entity/component creation is
    # still an editor op; everything after is the typed bus).
    eid = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntityAtPosition", math.Vector3(0.0, 0.0, 1.0), azlmbr.entity.EntityId())
    editor.EditorEntityAPIBus(bus.Event, "SetName", eid, "AiBusSprite")
    editor.EditorComponentAPIBus(bus.Broadcast, "AddComponentsOfType", eid, [sprite_type])

    # Configure the sprite through typed, forgiving verbs. No property-path
    # strings, no math types to construct, no silent no-ops.
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", eid, "diorama/textures/atlas_2x2.png")
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", eid, 3.0, 3.0)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSortOffset", eid, 5.0)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetDoubleSided", eid, True)
    # Show just the bottom-right (yellow) atlas cell.
    diorama.DioramaSpriteRequestBus(bus.Event, "SetUVRegion", eid, 0.5, 0.5, 1.0, 1.0)

    # Close the loop: query resolved state instead of eyeballing a screenshot.
    info = diorama.DioramaSpriteRequestBus(bus.Event, "GetSpriteInfo", eid)
    log("texturePath={}".format(info.texturePath))
    log("width={} height={} sortOffset={}".format(info.width, info.height, info.sortOffset))
    log("doubleSided={} animEnabled={} frameCount={}".format(info.doubleSided, info.animEnabled, info.frameCount))
    log("textureLoaded={} visible={}".format(info.textureLoaded, info.visible))

    ok = (abs(info.width - 3.0) < 1e-4 and abs(info.sortOffset - 5.0) < 1e-4 and info.doubleSided)
    log("config readback {}".format("OK" if ok else "MISMATCH"))
    log("done")


main()
