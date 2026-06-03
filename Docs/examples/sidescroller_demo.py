"""
Side-scroller vertical slice: the whole 2.5D stack in one scene.

Composes the gem's 2D features into a side-scroll layout in its own level:
- parallax background layers (far/mid/near) that scroll at their depths,
- a follow camera,
- torches (2D point lights) the player passes through,
- an ember particle emitter,
- a walking player sprite.

It is both a portfolio sample and an integration test of the features built out in
this sprint. The motion (walk + camera follow + parallax + particles) runs at game
time; this builds the scene, then you attach the walker script and enter game mode.

Setup after running (see Docs/howto/11-sidescroller.md):
  1. Add a Lua Script to the Player entity, script
     diorama/examples/sidescroller/walker.lua, and set its Camera property to
     the SideCamera entity.
  2. Select SideCamera -> Be this camera, then enter game mode.

Run in the editor:
  <engine>/bin/Linux/profile/Default/Editor \
    --project-path=/path/to/YourProject \
    --runpython /path/to/o3de-diorama/Docs/examples/sidescroller_demo.py
"""
import azlmbr
import azlmbr.asset
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity
import azlmbr.legacy.general as general
import azlmbr.math as math

diorama = azlmbr.diorama

LEVEL_NAME = "DioramaSideScroller"
QUAD_TEXTURE = "diorama/textures/white_sprite.png"
QUAD_TEXTURE_PRODUCT = "diorama/textures/white_sprite.png.streamingimage"
PLAYER_TEXTURE = "diorama/textures/o3de_mascot.png"


def log(msg):
    print("DIORAMA_SIDESCROLLER: {}".format(msg))


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


def make_entity(name, position, type_ids):
    eid = editor.ToolsApplicationRequestBus(
        bus.Broadcast, "CreateNewEntityAtPosition", position, azlmbr.entity.EntityId())
    editor.EditorEntityAPIBus(bus.Event, "SetName", eid, name)
    outcome = editor.EditorComponentAPIBus(bus.Broadcast, "AddComponentsOfType", eid, type_ids)
    comps = outcome.GetValue() if outcome.IsSuccess() else []
    return eid, comps


def set_prop(comp, path, value):
    editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", comp, path, value)


def main():
    log("start")
    if not open_or_create_level(LEVEL_NAME):
        log("FAIL: could not open or create the demo level '{}'".format(LEVEL_NAME))
        return
    general.idle_wait_frames(20)

    sprite_type = find_type_id("Sprite")
    camera_ctrl_type = find_type_id("2D Camera Controller")
    atom_camera_type = find_type_id("Camera")
    light_type = find_type_id("2D Light")
    parallax_type = find_type_id("2D Parallax Layer")
    emitter_type = find_type_id("2D Particle Emitter")
    if sprite_type is None or camera_ctrl_type is None or parallax_type is None:
        log("FAIL: required Diorama components not found (rebuild the gem?)")
        return

    tex_id = azlmbr.asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", QUAD_TEXTURE_PRODUCT, math.Uuid(), False)

    # Camera: an Atom camera + the follow controller. The walker wires its target +
    # framing at game time; just place it back along +Z here.
    cam_types = [camera_ctrl_type] + ([atom_camera_type] if atom_camera_type is not None else [])
    camera, _ = make_entity("SideCamera", math.Vector3(0.0, 0.0, 30.0), cam_types)

    # Parallax background layers: wide quads at increasing parallax factors (far
    # layers follow the camera most). Each references the camera.
    # (name, tint, size, y, sort, factor)
    layers = [
        ("BgFar", (0.20, 0.24, 0.40, 1.0), (120.0, 36.0), 4.0, -30.0, 0.15),
        ("BgMid", (0.24, 0.38, 0.32, 1.0), (100.0, 22.0), 0.0, -22.0, 0.40),
        ("BgNear", (0.30, 0.26, 0.20, 1.0), (90.0, 12.0), -3.0, -16.0, 0.70),
    ]
    for name, tint, size, y, sort, factor in layers:
        eid, comps = make_entity(name, math.Vector3(0.0, y, 1.0), [sprite_type, parallax_type])
        diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", eid, QUAD_TEXTURE)
        diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", eid, size[0], size[1])
        diorama.DioramaSpriteRequestBus(bus.Event, "SetBillboard", eid, True)
        diorama.DioramaSpriteRequestBus(bus.Event, "SetTint", eid, tint[0], tint[1], tint[2], tint[3])
        diorama.DioramaSpriteRequestBus(bus.Event, "SetSortOffset", eid, sort)
        if len(comps) >= 2:
            set_prop(comps[1], "Config|Camera", camera)
            set_prop(comps[1], "Config|Factor", factor)

    # Ground strip the player walks along (foreground, world-fixed: no parallax).
    ground, _ = make_entity("Ground", math.Vector3(0.0, -6.0, 1.0), [sprite_type])
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", ground, QUAD_TEXTURE)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", ground, 160.0, 4.0)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetBillboard", ground, True)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTint", ground, 0.15, 0.13, 0.12, 1.0)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSortOffset", ground, -8.0)

    # Torches the player passes: a warm point light each (lights via property since
    # the editor light component does not connect its request bus).
    if light_type is not None:
        for tx in (15.0, 40.0):
            eid, comps = make_entity("Torch_{}".format(int(tx)), math.Vector3(tx, -2.0, 2.0), [light_type])
            if comps:
                set_prop(comps[0], "Config|Color", math.Color(1.0, 0.6, 0.2, 1.0))
                set_prop(comps[0], "Config|Intensity", 2.5)
                set_prop(comps[0], "Config|Radius", 9.0)

    # An ember emitter at the first torch (particles via property; the editor emitter
    # does not connect its request bus either).
    if emitter_type is not None:
        eid, comps = make_entity("Embers", math.Vector3(15.0, -3.0, 2.0), [emitter_type])
        if comps:
            set_prop(comps[0], "Config|Texture", tex_id)
            set_prop(comps[0], "Config|Rate", 30.0)
            set_prop(comps[0], "Config|Burst Count", 0)
            set_prop(comps[0], "Config|Direction", 90.0)
            set_prop(comps[0], "Config|Spread", 35.0)
            set_prop(comps[0], "Config|Speed Min", 1.5)
            set_prop(comps[0], "Config|Speed Max", 3.0)
            set_prop(comps[0], "Config|Gravity", math.Vector2(0.0, 1.0))
            set_prop(comps[0], "Config|Start Size", 0.25)
            set_prop(comps[0], "Config|End Size", 0.0)
            set_prop(comps[0], "Config|Start Color", math.Color(1.0, 0.7, 0.2, 1.0))
            set_prop(comps[0], "Config|End Color", math.Color(1.0, 0.2, 0.0, 0.0))

    # A couple of lit props along the path.
    for i, px in enumerate((25.0, 50.0)):
        eid, _ = make_entity("Prop{}".format(i + 1), math.Vector3(px, -3.0, 1.0), [sprite_type])
        diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", eid, PLAYER_TEXTURE)
        diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", eid, 2.5, 3.2)
        diorama.DioramaSpriteRequestBus(bus.Event, "SetBillboard", eid, True)

    # The player. The walker (attached by hand) moves it and wires the camera.
    player, _ = make_entity("Player", math.Vector3(0.0, -3.0, 1.5), [sprite_type])
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", player, PLAYER_TEXTURE)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", player, 3.4, 4.4)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetBillboard", player, True)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetOutline", player, 0.1, 0.9, 1.0, 1.2)

    general.idle_wait_frames(40)
    if general.get_current_level_name() == LEVEL_NAME:
        try:
            general.save_level()
        except Exception as e:
            log("save_level raised: {}".format(e))
    else:
        log("WARN: current level is '{}', not '{}'; skipping save to avoid overwriting it".format(
            general.get_current_level_name(), LEVEL_NAME))

    log("Side-scroller scene built in level '{}'.".format(LEVEL_NAME))
    log("Add diorama/examples/sidescroller/walker.lua to 'Player', set its Camera to")
    log("'SideCamera', select SideCamera -> Be this camera, then enter game mode:")
    log("the player walks, the camera follows, the parallax layers scroll, and the")
    log("player passes through the torch light + embers.")
    log("done")


main()
