"""
2.5D quick-start: the smallest playable Diorama scene.

Builds an instant-start starter in its own level: a ground quad, a player sprite
(the O3DE mascot), and a framed 2D camera. You then drop the reusable
player_move_2d.lua controller and an Input component on the player and walk it
around with WASD / arrows / left stick. This is the "new 2.5D game" starting point
the rest of the how-tos build on; see Docs/howto/17-quickstart.md.

It deliberately stays minimal: no parallax, lights, or particles. Add those from
the per-feature how-tos once you have something moving.

Setup after running (see Docs/howto/17-quickstart.md):
  1. Select Player -> Add Component -> Lua Script, set the script to
     diorama/examples/quickstart/player_move_2d.lua.
  2. Select Player -> Add Component -> Input, set its Input to bind to
     diorama/input/diorama_move.inputbindings.
  3. Select QuickCamera -> Be this camera, then enter game mode (Ctrl+G) and move.

Run in the editor:
  <engine>/bin/Linux/profile/Default/Editor \
    --project-path=/path/to/YourProject \
    --runpython /path/to/o3de-diorama/Docs/examples/quickstart_demo.py
"""
import json

import azlmbr
import azlmbr.asset
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity
import azlmbr.legacy.general as general
import azlmbr.math as math
import azlmbr.paths

diorama = azlmbr.diorama

LEVEL_NAME = "DioramaQuickStart"
GROUND_TEXTURE = "diorama/textures/white_floor.png"
PLAYER_TEXTURE = "diorama/textures/o3de_mascot.png"


def log(msg):
    print("DIORAMA_QUICKSTART: {}".format(msg))


def find_type_id(name):
    ids = editor.EditorComponentAPIBus(bus.Broadcast, "FindComponentTypeIdsByEntityType", [name], 0)
    if ids and len(ids) > 0 and not ids[0].IsNull():
        return ids[0]
    return None


def open_or_create_level(level_name):
    """Open the named level, creating a fresh one from a built-in template if it
    does not exist, so the quick-start builds in its OWN level and never lands on
    top of another scene (this project's DefaultLevel is the twin-stick sample)."""
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


def make_entity(name, position, type_ids):
    eid = editor.ToolsApplicationRequestBus(
        bus.Broadcast, "CreateNewEntityAtPosition", position, azlmbr.entity.EntityId())
    editor.EditorEntityAPIBus(bus.Event, "SetName", eid, name)
    editor.EditorComponentAPIBus(bus.Broadcast, "AddComponentsOfType", eid, type_ids)
    return eid


def frame_quickcamera():
    """Aim QuickCamera at the XY plane by patching the saved prefab, then reopen.

    A runtime TransformBus.SetLocalRotation does NOT persist: it updates the live
    component (so the viewport shows it) but never patches the prefab template DOM that
    save serializes, so the rotation is dropped on save. Instead we edit the serialized
    prefab directly -- the file the loader reads -- and reopen the level so the editor's
    live template absorbs the rotation (and a later save by the user preserves it).
    Rotate -90 about X makes the camera's +Y forward point -Z, a front view of the scene.
    """
    proj = None
    try:
        proj = str(azlmbr.paths.projectroot)
    except Exception as e:
        log("could not resolve project root ({}); set QuickCamera Rotate X=-90 by hand".format(e))
        return
    pf = "{}/Levels/{}/{}.prefab".format(proj, LEVEL_NAME, LEVEL_NAME)
    try:
        with open(pf) as f:
            doc = json.load(f)
        patched = 0
        for entity in doc.get("Entities", {}).values():
            if entity.get("Name") != "QuickCamera":
                continue
            for comp in entity.get("Components", {}).values():
                if "TransformComponent" in comp.get("$type", ""):
                    comp.setdefault("Transform Data", {})["Rotate"] = [-90.0, 0.0, 0.0]
                    patched += 1
        with open(pf, "w") as f:
            json.dump(doc, f, indent=4)
        general.open_level_no_prompt(LEVEL_NAME)
        general.idle_wait_frames(20)
        log("Camera framed (Rotate X=-90) via prefab patch + reload; patched {} transform(s).".format(patched))
    except Exception as e:
        log("camera-frame patch failed ({}); set QuickCamera Rotate X=-90 by hand".format(e))


def main():
    log("start")
    if not open_or_create_level(LEVEL_NAME):
        log("FAIL: could not open or create the quick-start level '{}'".format(LEVEL_NAME))
        return
    general.idle_wait_frames(20)

    sprite_type = find_type_id("Sprite")
    controller_type = find_type_id("2D Camera Controller")
    atom_camera_type = find_type_id("Camera")
    if sprite_type is None:
        log("FAIL: 'Sprite' component not found (is the Diorama gem enabled and built?)")
        return
    if controller_type is None:
        log("FAIL: '2D Camera Controller' not found (rebuild the gem with the camera component?)")
        return

    # Ground: a wide, soft quad behind the player so movement reads against it.
    ground = make_entity("Ground", math.Vector3(0.0, 0.0, 0.0), [sprite_type])
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", ground, GROUND_TEXTURE)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", ground, 30.0, 20.0)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetBillboard", ground, True)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTint", ground, 0.22, 0.30, 0.40, 1.0)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSortOffset", ground, -10.0)

    # Player: the O3DE mascot. The movement script + Input component are attached by
    # hand (the editor does not reliably wire a Lua script asset from Python).
    player = make_entity("Player", math.Vector3(0.0, 0.0, 1.0), [sprite_type])
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", player, PLAYER_TEXTURE)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", player, 3.0, 3.9)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetBillboard", player, True)

    # Camera: an Atom Camera + the Diorama 2D Camera Controller, placed back along
    # +Z to look at the XY plane. Make it the active view with Be this camera.
    cam_types = [controller_type] + ([atom_camera_type] if atom_camera_type is not None else [])
    if atom_camera_type is None:
        log("WARN: Atom 'Camera' component not found; add a Camera to QuickCamera by hand.")
    make_entity("QuickCamera", math.Vector3(0.0, 0.0, 14.0), cam_types)
    # The camera's rotation is set after the save, by patching the prefab directly --
    # see frame_quickcamera(). O3DE camera forward is entity +Y, so it needs a -90 X
    # rotation (forward -> -Z, a front view of the XY plane) or the level renders empty.

    general.idle_wait_frames(40)
    if general.get_current_level_name() == LEVEL_NAME:
        try:
            general.save_level()
            frame_quickcamera()
        except Exception as e:
            log("save_level raised: {}".format(e))
    else:
        log("WARN: current level is '{}', not '{}'; skipping save to avoid overwriting it".format(
            general.get_current_level_name(), LEVEL_NAME))

    log("Quick-start scene built in level '{}'.".format(LEVEL_NAME))
    log("Add diorama/examples/quickstart/player_move_2d.lua and an Input component")
    log("(bind diorama/input/diorama_move.inputbindings) to 'Player', select")
    log("QuickCamera -> Be this camera, then enter game mode and move with WASD.")
    log("done")


main()
