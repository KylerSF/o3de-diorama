"""
2D camera demo: a follow camera with deadzone, lookahead, and screen shake.

Builds a row of reference posts (so panning is visible), a Target sprite, and a
camera entity carrying an Atom Camera plus the Diorama "2D Camera Controller".
The controller follows whatever target it is given and applies deadzone, lookahead,
and trauma-based shake -- all from the unit-tested Camera2D math core. The follow
is seen at GAME TIME (the controller ticks in game mode, not in the editor
viewport), so this script builds the scene and the camera_target.lua script wires
+ drives it once you enter game mode.

Setup after running (see Docs/howto/08-camera.md):
  1. Add a Lua Script component to the Target entity, script
     diorama/examples/camera/camera_target.lua, and set its Camera property to the
     DemoCamera entity.
  2. Select DemoCamera and use Be this camera (or right-click -> Be this camera).
  3. Enter game mode: the camera pans to follow the patrolling target and kicks
     with shake on a timer.

Run in the editor:
  <engine>/bin/Linux/profile/Default/Editor \
    --project-path=/path/to/YourProject \
    --runpython /path/to/o3de-diorama/Docs/examples/camera_demo.py
"""
import azlmbr
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity
import azlmbr.legacy.general as general
import azlmbr.math as math

diorama = azlmbr.diorama

LEVEL_NAME = "DioramaCameraDemo"
POST_TEXTURE = "diorama/textures/white_sprite.png"
TARGET_TEXTURE = "diorama/textures/sample_sprite.png"


def log(msg):
    print("DIORAMA_CAMERA: {}".format(msg))


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
    # Wait for the editor to finish booting and load its startup level FIRST, so our
    # open/create is not clobbered by a late default-level load (which would land
    # our entities in DefaultLevel -- the twin-stick scene -- and, if we then saved,
    # overwrite it).
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
        # create_level_no_prompt(templateName, levelName, heightmapRes, unitSize,
        # terrainTexSize, useTerrain); Default_Level gives the standard Atom env.
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

    # Final guard: ensure our level is current and stayed so (re-open once if a late
    # load drifted us off it).
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


def make_entity(name, position, type_ids):
    eid = editor.ToolsApplicationRequestBus(
        bus.Broadcast, "CreateNewEntityAtPosition", position, azlmbr.entity.EntityId())
    editor.EditorEntityAPIBus(bus.Event, "SetName", eid, name)
    editor.EditorComponentAPIBus(bus.Broadcast, "AddComponentsOfType", eid, type_ids)
    return eid


def main():
    log("start")
    general.idle_enable(True)
    if not open_or_create_level(LEVEL_NAME):
        log("FAIL: could not open or create the demo level '{}'".format(LEVEL_NAME))
        return
    general.idle_wait_frames(20)

    sprite_type = find_type_id("Sprite")
    camera_type = find_type_id("Camera")
    controller_type = find_type_id("2D Camera Controller")
    if sprite_type is None:
        log("FAIL: Diorama Sprite component not found (is the Diorama gem enabled?)")
        return
    if controller_type is None:
        log("FAIL: '2D Camera Controller' not found (rebuild the gem with the camera component?)")
        return
    if camera_type is None:
        log("WARN: Atom 'Camera' component not found; creating the controller entity")
        log("      without a camera -- add a Camera component to DemoCamera by hand.")

    # A row of reference posts across X so the camera's panning is obvious, plus a
    # couple of rows in Y for depth. Alternating tints make individual posts read.
    tints = [(0.9, 0.3, 0.3, 1.0), (0.3, 0.9, 0.4, 1.0), (0.4, 0.5, 0.95, 1.0), (0.95, 0.85, 0.3, 1.0)]
    index = 0
    for gx in range(-30, 31, 6):
        for gy in (-4.0, 4.0):
            eid = make_entity("Post_{}".format(index), math.Vector3(float(gx), gy, 1.0), [sprite_type])
            diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", eid, POST_TEXTURE)
            diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", eid, 2.0, 4.0)
            diorama.DioramaSpriteRequestBus(bus.Event, "SetBillboard", eid, True)
            t = tints[index % len(tints)]
            diorama.DioramaSpriteRequestBus(bus.Event, "SetTint", eid, t[0], t[1], t[2], t[3])
            index += 1

    # The follow target: a sprite at the center. camera_target.lua patrols it.
    target = make_entity("Target", math.Vector3(0.0, 0.0, 1.0), [sprite_type])
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", target, TARGET_TEXTURE)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", target, 3.0, 3.0)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetBillboard", target, True)

    # The camera entity: an Atom Camera + the Diorama 2D Camera Controller. Placed
    # back along +Z; the controller keeps it framed on the target by following in
    # the plane and holding the out-of-plane distance. The controller only moves
    # position and preserves the camera's rotation, so you set a good viewing angle
    # once (Be this camera) and the follow keeps it. See Docs/howto/08-camera.md.
    cam_types = [controller_type] + ([camera_type] if camera_type is not None else [])
    camera = make_entity("DemoCamera", math.Vector3(0.0, 0.0, 28.0), cam_types)

    general.idle_wait_frames(60)
    # Guard: only save if we are actually in the demo level, never another scene --
    # a misfire here would overwrite whatever level is open (e.g. the twin-stick).
    if general.get_current_level_name() == LEVEL_NAME:
        try:
            general.save_level()
        except Exception as e:
            log("save_level raised: {}".format(e))
    else:
        log("WARN: current level is '{}', not '{}'; skipping save to avoid overwriting it".format(
            general.get_current_level_name(), LEVEL_NAME))

    log("Scene built in its own level '{}'.".format(LEVEL_NAME))
    log("Next: add a Lua Script (diorama/examples/camera/camera_target.lua) to 'Target',")
    log("set its Camera property to 'DemoCamera', select DemoCamera -> Be this camera,")
    log("then enter game mode. The camera follows the patrolling target and shakes.")
    log("If the framing is off, adjust DemoCamera's transform (it only needs to look")
    log("at the XY plane; the controller preserves its rotation).")
    log("done")


main()
