"""
One-shot on-screen test scene for this session's features. Run via --runpython so it
never touches the (crash-prone) Python console panel.

Builds level "DioramaTestAll":
  * Aseprite row (PASSIVE -- just look): three sprites textured with the generated
    indexed / grayscale / multiply atlases. Indexed = colored blocks on transparent;
    grayscale = horizontal gradient; multiply = orange with a darker left half.
  * Player (INTERACTIVE): Sprite + 2D Input Actions + 2D Animation State Machine +
    a Lua Script slot, with both configs baked into the prefab. After it runs, set the
    Lua Script to diorama/examples/animsm/input_to_anim.lua and TestCamera -> Be this
    camera, enter game mode, and move (WASD/space): walks + tints green/white/orange.

The script logs, per Aseprite sprite, whether its texture product resolved, and how
many component configs it baked -- that is the headless pass/fail signal.

  <engine>/bin/Linux/profile/Default/Editor --project-path=<proj> \
    --runpython <gem>/Docs/examples/test_all_features.py
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

LEVEL_NAME = "DioramaTestAll"
GROUND_TEXTURE = "diorama/textures/white_floor.png"
PLAYER_TEXTURE = "diorama/textures/o3de_mascot.png"
LUA_SCRIPT = "diorama/examples/animsm/input_to_anim.lua"
ASEPRITE = [
    ("Indexed", "diorama/examples/aseprite/indexed_test.streamingimage", -7.0),
    ("Grayscale", "diorama/examples/aseprite/grayscale_test.streamingimage", 0.0),
    ("Multiply", "diorama/examples/aseprite/multiply_test.streamingimage", 7.0),
]

INPUT_CONFIG = {
    "actions": [
        {"name": "move", "kind": 2, "deadZone": 0.2, "pressThreshold": 0.2, "bindings": [
            {"channel": "keyboard_key_alphanumeric_D", "scale": 1.0, "axis": 0},
            {"channel": "keyboard_key_alphanumeric_A", "scale": -1.0, "axis": 0},
            {"channel": "keyboard_key_alphanumeric_W", "scale": 1.0, "axis": 1},
            {"channel": "keyboard_key_alphanumeric_S", "scale": -1.0, "axis": 1},
            {"channel": "keyboard_key_navigation_arrow_right", "scale": 1.0, "axis": 0},
            {"channel": "keyboard_key_navigation_arrow_left", "scale": -1.0, "axis": 0},
            {"channel": "keyboard_key_navigation_arrow_up", "scale": 1.0, "axis": 1},
            {"channel": "keyboard_key_navigation_arrow_down", "scale": -1.0, "axis": 1},
            {"channel": "gamepad_thumbstick_l_x", "scale": 1.0, "axis": 0},
            {"channel": "gamepad_thumbstick_l_y", "scale": 1.0, "axis": 1},
        ]},
        {"name": "jump", "kind": 0, "bindings": [
            {"channel": "keyboard_key_edit_space", "scale": 1.0, "axis": 0},
            {"channel": "gamepad_button_a", "scale": 1.0, "axis": 0},
        ]},
    ]
}
ANIM_CONFIG = {
    "parameters": [{"name": "speed", "kind": 1}, {"name": "jump", "kind": 2}],
    "states": [
        {"name": "idle", "frameCount": 0, "fps": 6.0, "loop": True, "duration": 0.5},
        {"name": "run", "frameCount": 0, "fps": 12.0, "loop": True, "duration": 0.4},
        {"name": "jump", "frameCount": 0, "loop": False, "duration": 0.4},
    ],
    "transitions": [
        {"from": "idle", "to": "run", "conditions": [{"param": "speed", "compare": 0, "threshold": 0.1}]},
        {"from": "run", "to": "idle", "conditions": [{"param": "speed", "compare": 1, "threshold": 0.1}]},
        {"from": "", "to": "jump", "conditions": [{"param": "jump"}]},
        {"from": "jump", "to": "idle", "hasExitTime": True, "exitTime": 1.0},
    ],
    "defaultState": "idle",
}


def log(msg):
    print("DIORAMA_TESTALL: {}".format(msg))


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
    return now_in(level_name)


def make_entity(name, position, type_ids):
    eid = editor.ToolsApplicationRequestBus(
        bus.Broadcast, "CreateNewEntityAtPosition", position, azlmbr.entity.EntityId())
    editor.EditorEntityAPIBus(bus.Event, "SetName", eid, name)
    editor.EditorComponentAPIBus(bus.Broadcast, "AddComponentsOfType", eid, type_ids)
    return eid


def patch_prefab():
    try:
        proj = str(azlmbr.paths.projectroot)
    except Exception as e:
        log("no project root ({}); configure by hand".format(e))
        return
    pf = "{}/Levels/{}/{}.prefab".format(proj, LEVEL_NAME, LEVEL_NAME)
    try:
        with open(pf) as f:
            doc = json.load(f)
    except Exception as e:
        log("read prefab failed ({}); configure by hand".format(e))
        return
    cam, inp, anim = 0, 0, 0
    for entity in doc.get("Entities", {}).values():
        name = entity.get("Name")
        for comp in entity.get("Components", {}).values():
            ctype = comp.get("$type", "")
            if name == "TestCamera" and "TransformComponent" in ctype:
                comp.setdefault("Transform Data", {})["Rotate"] = [-90.0, 0.0, 0.0]
                cam += 1
            elif "EditorDioramaInputComponent" in ctype:
                comp["Config"] = INPUT_CONFIG
                inp += 1
            elif "EditorDioramaAnimStateMachineComponent" in ctype:
                comp["Config"] = ANIM_CONFIG
                anim += 1
    try:
        with open(pf, "w") as f:
            json.dump(doc, f, indent=4)
        general.open_level_no_prompt(LEVEL_NAME)
        general.idle_wait_frames(20)
        log("Prefab patched: camera={}, inputConfig={}, animConfig={}".format(cam, inp, anim))
    except Exception as e:
        log("prefab patch failed ({}); configure by hand".format(e))


def main():
    log("start")
    if not open_or_create_level(LEVEL_NAME):
        log("FAIL: could not open/create level")
        return
    general.idle_wait_frames(20)

    sprite_type = find_type_id("Sprite")
    input_type = find_type_id("2D Input Actions")
    anim_type = find_type_id("2D Animation State Machine")
    cam_ctrl_type = find_type_id("2D Camera Controller")
    atom_camera_type = find_type_id("Camera")
    lua_type = find_type_id("Lua Script")
    if sprite_type is None or input_type is None or anim_type is None:
        log("FAIL: Diorama components missing (rebuild/enable gem). sprite={} input={} anim={}".format(
            sprite_type is not None, input_type is not None, anim_type is not None))
        return

    ground = make_entity("Ground", math.Vector3(0.0, 0.0, 0.0), [sprite_type])
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", ground, GROUND_TEXTURE)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", ground, 40.0, 24.0)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetBillboard", ground, True)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTint", ground, 0.15, 0.18, 0.24, 1.0)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSortOffset", ground, -10.0)

    # Aseprite fixtures -- passive visual check (Feature 4, indexed/grayscale/blend).
    for label, product, x in ASEPRITE:
        e = make_entity("Aseprite_" + label, math.Vector3(x, 4.0, 1.0), [sprite_type])
        diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", e, 5.0, 5.0)
        diorama.DioramaSpriteRequestBus(bus.Event, "SetBillboard", e, True)
        diorama.DioramaSpriteRequestBus(bus.Event, "SetPointFilter", e, True)  # crisp 16x16
        ok = diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", e, product)
        log("Aseprite {} texture resolved = {}".format(label, ok))

    # Player -- interactive input -> anim state machine.
    ptypes = [sprite_type, input_type, anim_type] + ([lua_type] if lua_type is not None else [])
    if lua_type is None:
        log("WARN: 'Lua Script' component not found; add it to Player by hand.")
    player = make_entity("Player", math.Vector3(0.0, -5.0, 1.0), ptypes)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", player, PLAYER_TEXTURE)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", player, 3.0, 3.9)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetBillboard", player, True)

    cam_types = ([cam_ctrl_type] if cam_ctrl_type is not None else []) + \
        ([atom_camera_type] if atom_camera_type is not None else [])
    make_entity("TestCamera", math.Vector3(0.0, 0.0, 26.0), cam_types)

    general.idle_wait_frames(40)
    if general.get_current_level_name() == LEVEL_NAME:
        try:
            general.save_level()
            patch_prefab()
        except Exception as e:
            log("save_level raised: {}".format(e))

    log("Scene built. Aseprite row = look now (fly the editor camera up to the 3 sprites).")
    log("MANUAL for input/anim: Player -> Lua Script -> {} ; TestCamera -> Be this camera ; Ctrl+G ; move.".format(LUA_SCRIPT))
    log("done")


main()
