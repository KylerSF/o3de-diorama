"""
On-screen test scene for the 2D Input Actions + 2D Animation State Machine features.

Builds (in its own level): a ground quad, a framed camera, and a Player sprite
carrying three components -- Sprite, 2D Input Actions, 2D Animation State Machine,
and a Lua Script slot. It then bakes the Input + Anim-State-Machine *configs* into
the saved prefab (they are nested struct lists the editor Python API cannot set
reliably) and reopens the level.

After it runs you do TWO manual clicks (the editor cannot wire these from Python):
  1. Select Player -> the Lua Script component -> set Script to
     diorama/examples/animsm/input_to_anim.lua
  2. Select TestCamera -> "Be this camera", then enter game mode (Ctrl+G).

Then move with WASD / arrows / left stick and tap Space (or gamepad A):
  * the sprite walks  -> the 2D Input Actions "move" axis works,
  * it turns white while moving and green when idle, orange on a jump
    -> the Animation State Machine switched state from the input-fed "speed" param
       and the "jump" trigger (watch the Console for "Diorama anim state -> ...").

If the auto-config did not take (the script prints how many components it patched),
fill the values printed at the end into the Inspector by hand.

Run in the editor:
  <engine>/bin/Linux/profile/Default/Editor \
    --project-path=/path/to/YourProject \
    --runpython /path/to/o3de-diorama/Docs/examples/anim_input_demo.py
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

LEVEL_NAME = "DioramaAnimInput"
GROUND_TEXTURE = "diorama/textures/white_floor.png"
PLAYER_TEXTURE = "diorama/textures/o3de_mascot.png"
LUA_SCRIPT = "diorama/examples/animsm/input_to_anim.lua"

# The configs to bake into the prefab. Enum fields serialize as their integer value:
#   InputMap::ActionKind  Button=0, Axis1D=1, Axis2D=2
#   InputMap::Axis        X=0, Y=1
#   AnimSM::ParamKind     Bool=0, Float=1, Trigger=2
#   AnimSM::Compare       Greater=0, Less=1, GreaterEqual=2, LessEqual=3
INPUT_CONFIG = {
    "actions": [
        {
            "name": "move",
            "kind": 2,  # Axis2D
            "deadZone": 0.2,
            "pressThreshold": 0.2,
            "bindings": [
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
            ],
        },
        {
            "name": "jump",
            "kind": 0,  # Button
            "bindings": [
                {"channel": "keyboard_key_edit_space", "scale": 1.0, "axis": 0},
                {"channel": "gamepad_button_a", "scale": 1.0, "axis": 0},
            ],
        },
    ]
}

ANIM_CONFIG = {
    "parameters": [
        {"name": "speed", "kind": 1},   # Float
        {"name": "jump", "kind": 2},    # Trigger
    ],
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
    print("DIORAMA_ANIM_INPUT: {}".format(msg))


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
    """Bake the camera rotation + the Input/Anim configs into the saved prefab, then
    reopen. The nested config lists cannot be set through the editor component API, and
    a runtime transform set does not persist, so we edit the serialized file directly --
    the same approach quickstart_demo.py uses to frame its camera."""
    try:
        proj = str(azlmbr.paths.projectroot)
    except Exception as e:
        log("could not resolve project root ({}); configure components by hand".format(e))
        return
    pf = "{}/Levels/{}/{}.prefab".format(proj, LEVEL_NAME, LEVEL_NAME)
    try:
        with open(pf) as f:
            doc = json.load(f)
    except Exception as e:
        log("could not read prefab ({}); configure components by hand".format(e))
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
        log("Prefab patched: camera={}, input config={}, anim config={}.".format(cam, inp, anim))
        if inp == 0 or anim == 0:
            log("WARN: a component config was not patched -- fill it in the Inspector (values below).")
    except Exception as e:
        log("prefab patch failed ({}); configure components by hand".format(e))


def main():
    log("start")
    if not open_or_create_level(LEVEL_NAME):
        log("FAIL: could not open or create level '{}'".format(LEVEL_NAME))
        return
    general.idle_wait_frames(20)

    sprite_type = find_type_id("Sprite")
    input_type = find_type_id("2D Input Actions")
    anim_type = find_type_id("2D Animation State Machine")
    cam_ctrl_type = find_type_id("2D Camera Controller")
    atom_camera_type = find_type_id("Camera")
    lua_type = find_type_id("Lua Script")

    missing = [n for n, t in (
        ("Sprite", sprite_type),
        ("2D Input Actions", input_type),
        ("2D Animation State Machine", anim_type),
    ) if t is None]
    if missing:
        log("FAIL: components not found (rebuild/enable the gem?): {}".format(", ".join(missing)))
        return

    ground = make_entity("Ground", math.Vector3(0.0, 0.0, 0.0), [sprite_type])
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", ground, GROUND_TEXTURE)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", ground, 30.0, 20.0)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetBillboard", ground, True)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTint", ground, 0.18, 0.22, 0.30, 1.0)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSortOffset", ground, -10.0)

    player_types = [sprite_type, input_type, anim_type]
    if lua_type is not None:
        player_types.append(lua_type)
    else:
        log("WARN: 'Lua Script' component not found; add it to Player by hand.")
    player = make_entity("Player", math.Vector3(0.0, 0.0, 1.0), player_types)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", player, PLAYER_TEXTURE)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", player, 3.0, 3.9)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetBillboard", player, True)

    cam_types = ([cam_ctrl_type] if cam_ctrl_type is not None else []) + \
        ([atom_camera_type] if atom_camera_type is not None else [])
    make_entity("TestCamera", math.Vector3(0.0, 0.0, 14.0), cam_types)

    general.idle_wait_frames(40)
    if general.get_current_level_name() == LEVEL_NAME:
        try:
            general.save_level()
            patch_prefab()
        except Exception as e:
            log("save_level raised: {}".format(e))

    log("Scene built in level '{}'.".format(LEVEL_NAME))
    log("MANUAL: (1) Player -> Lua Script -> Script = {}".format(LUA_SCRIPT))
    log("MANUAL: (2) TestCamera -> Be this camera, then Ctrl+G and move (WASD/space).")
    log("EXPECT: sprite walks; tint green=idle, white=run, orange=jump; Console logs state changes.")
    log("--- Inspector fallback if a config did not auto-apply ---")
    log("2D Input Actions: action 'move' (Axis 2D, dead zone 0.2) bound to A/D->X +/-1,")
    log("  W/S->Y +/-1, left stick X/Y; action 'jump' (Button) bound to space + gamepad A.")
    log("2D Animation State Machine: params speed(Float), jump(Trigger); states idle/run/jump;")
    log("  transitions idle->run [speed>0.1], run->idle [speed<0.1], AnyState->jump [jump],")
    log("  jump->idle [exit time 1.0]; default state idle.")
    log("done")


main()
