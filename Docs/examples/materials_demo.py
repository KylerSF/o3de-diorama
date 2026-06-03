"""
2D materials demo: the hit-flash effect.

Builds a row of sprites in their own level. Attaching flash_pulse.lua makes each
sprite flash a color on a timer (as gameplay would flash an enemy on a hit), via
the sprite's DioramaSpriteRequestBus SetFlash verb. The flash blends in after
lighting, so it reads as a bright hit even on a lit, normal-mapped sprite.

The flash runs at GAME TIME (the script ticks in game mode), so this builds the
scene; attach the script and enter game mode to see it.

Setup after running (see Docs/howto/10-materials.md):
  1. Add a Lua Script component to a Target sprite, script
     diorama/examples/materials/flash_pulse.lua.
  2. Enter game mode: the sprite flashes on the timer.

Run in the editor:
  <engine>/bin/Linux/profile/Default/Editor \
    --project-path=/path/to/YourProject \
    --runpython /path/to/o3de-diorama/Docs/examples/materials_demo.py
"""
import azlmbr
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity
import azlmbr.legacy.general as general
import azlmbr.math as math

diorama = azlmbr.diorama

LEVEL_NAME = "DioramaMaterialsDemo"
CREATURE_TEXTURE = "diorama/textures/o3de_mascot.png"


def log(msg):
    print("DIORAMA_MATERIALS: {}".format(msg))


def find_type_id(name):
    ids = editor.EditorComponentAPIBus(bus.Broadcast, "FindComponentTypeIdsByEntityType", [name], 0)
    if ids and len(ids) > 0 and not ids[0].IsNull():
        return ids[0]
    return None


def open_or_create_level(level_name):
    """Open the named level, creating a fresh one from a built-in template if it
    does not exist, so the example builds in its OWN level and never lands on top
    of another scene (this project's DefaultLevel is the twin-stick sample)."""
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
    except Exception as e:
        log("open_level raised: {}".format(e))
    waited = 0
    while not now_in(level_name) and waited < 200:
        general.idle_wait_frames(1)
        waited += 1

    if not now_in(level_name):
        for template in ("Default_Level", "Empty", "Basic"):
            try:
                general.create_level_no_prompt(template, level_name, 128, 1, 512, False)
            except Exception as e:
                log("create_level('{}') raised: {}".format(template, e))
                continue
            waited = 0
            while not now_in(level_name) and waited < 400:
                general.idle_wait_frames(1)
                waited += 1
            if now_in(level_name):
                break

    general.idle_wait_frames(30)
    if not now_in(level_name):
        try:
            general.open_level_no_prompt(level_name)
        except Exception as e:
            log("reopen raised: {}".format(e))
        waited = 0
        while not now_in(level_name) and waited < 200:
            general.idle_wait_frames(1)
            waited += 1
    return now_in(level_name)


def main():
    log("start")
    if not open_or_create_level(LEVEL_NAME):
        log("FAIL: could not open or create the demo level '{}'".format(LEVEL_NAME))
        return
    general.idle_wait_frames(20)

    sprite_type = find_type_id("Sprite")
    if sprite_type is None:
        log("FAIL: Diorama Sprite component not found (is the Diorama gem enabled?)")
        return

    count = 4
    spacing = 4.0
    targets = []
    for i in range(count):
        x = (i - (count - 1) * 0.5) * spacing
        eid = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", math.Vector3(x, 0.0, 1.0), azlmbr.entity.EntityId())
        editor.EditorEntityAPIBus(bus.Event, "SetName", eid, "Target{}".format(i + 1))
        editor.EditorComponentAPIBus(bus.Broadcast, "AddComponentsOfType", eid, [sprite_type])
        diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", eid, CREATURE_TEXTURE)
        diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", eid, 3.0, 3.85)
        diorama.DioramaSpriteRequestBus(bus.Event, "SetBillboard", eid, True)
        targets.append(eid)

    # Show the material effects from the bus directly (no script needed): pre-flash
    # the last target halfway, and give the first target a cyan silhouette outline.
    diorama.DioramaSpriteRequestBus(bus.Event, "SetFlash", targets[-1], 1.0, 1.0, 1.0, 0.5)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetOutline", targets[0], 0.1, 0.9, 1.0, 1.5)

    general.idle_wait_frames(30)
    if general.get_current_level_name() == LEVEL_NAME:
        try:
            general.save_level()
        except Exception as e:
            log("save_level raised: {}".format(e))
    else:
        log("WARN: current level is '{}', not '{}'; skipping save to avoid overwriting it".format(
            general.get_current_level_name(), LEVEL_NAME))

    log("Built {} targets in level '{}'.".format(count, LEVEL_NAME))
    log("Add a Lua Script (diorama/examples/materials/flash_pulse.lua) to a Target,")
    log("then enter game mode: it flashes on a timer. Drive it from gameplay with")
    log("DioramaSpriteRequestBus.SetFlash(entity, r, g, b, amount) -- 1 on a hit, ease to 0.")
    log("done")


main()
