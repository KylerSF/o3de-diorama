"""
2D dynamic lighting demo: a dark scene lit by a colored point light.

Builds a dark backdrop and a row of creature sprites, then adds a Diorama
"2D Light" (point) configured entirely through the typed DioramaLightRequestBus
-- color, intensity, radius -- and confirms the result with GetLightInfo. With
lighting on, the sprites near the light glow in its color and fall off with
distance; sprites far away sit at the ambient floor. Attach light_orbit.lua to
the light (see the closing log) to sweep the glow across the scene.

This is a focused per-feature demo for the 2D lighting feature; see
Docs/howto/07-lighting.md for the step-by-step.

Run in the editor:
  <engine>/bin/Linux/profile/Default/Editor \
    --project-path=/path/to/YourProject \
    --runpython /path/to/o3de-diorama/Docs/examples/lighting_demo.py
"""
import azlmbr
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity
import azlmbr.legacy.general as general
import azlmbr.math as math

diorama = azlmbr.diorama

# This demo builds into its OWN level so it is completely independent of any other
# scene in the project (the twin-stick sample, etc.). Re-running reopens it.
LEVEL_NAME = "DioramaLightingDemo"

# Solid-white textures shipped with the gem; tinted per sprite. The backdrop is a
# large dark quad so the light's glow reads against it.
BACKDROP_TEXTURE = "diorama/textures/white_floor.png"
CREATURE_TEXTURE = "diorama/textures/sample_sprite.png"


def log(msg):
    print("DIORAMA_LIGHTING: {}".format(msg))


def find_type_id(name):
    ids = editor.EditorComponentAPIBus(bus.Broadcast, "FindComponentTypeIdsByEntityType", [name], 0)
    if ids and len(ids) > 0 and not ids[0].IsNull():
        return ids[0]
    return None


def make_entity(name, position, type_id):
    eid = editor.ToolsApplicationRequestBus(
        bus.Broadcast, "CreateNewEntityAtPosition", position, azlmbr.entity.EntityId())
    editor.EditorEntityAPIBus(bus.Event, "SetName", eid, name)
    editor.EditorComponentAPIBus(bus.Broadcast, "AddComponentsOfType", eid, [type_id])
    return eid


def open_demo_level():
    """Open the demo level, creating a fresh one from a built-in template if it does
    not exist, so the demo builds in its OWN level and never lands on top of another
    scene (this project's DefaultLevel is the twin-stick sample)."""
    general.idle_enable(True)
    # Wait for the editor to finish booting and load its startup level FIRST, so our
    # open/create is not clobbered by a late default-level load (which would land our
    # entities in DefaultLevel -- the twin-stick scene -- and, since this demo saves,
    # overwrite it).
    booted = 0
    while general.get_current_level_name() in ("", "Untitled") and booted < 600:
        general.idle_wait_frames(1)
        booted += 1
    general.idle_wait_frames(30)

    def now_in(name):
        return general.get_current_level_name() == name

    try:
        general.open_level_no_prompt(LEVEL_NAME)
    except Exception as e:
        log("open_level raised: {}".format(e))
    waited = 0
    while not now_in(LEVEL_NAME) and waited < 200:
        general.idle_wait_frames(1)
        waited += 1

    if not now_in(LEVEL_NAME):
        # create_level_no_prompt(templateName, levelName, heightmapRes, unitSize,
        # terrainTexSize, useTerrain); Default_Level gives the standard Atom env.
        for template in ("Default_Level", "Empty", "Basic"):
            log("creating new level '{}' from template '{}'".format(LEVEL_NAME, template))
            try:
                general.create_level_no_prompt(template, LEVEL_NAME, 128, 1, 512, False)
            except Exception as e:
                log("create_level('{}') raised: {}".format(template, e))
                continue
            waited = 0
            while not now_in(LEVEL_NAME) and waited < 400:
                general.idle_wait_frames(1)
                waited += 1
            if now_in(LEVEL_NAME):
                break

    # Final guard: ensure our level is current and stayed so (re-open once if a late
    # load drifted us off it).
    general.idle_wait_frames(30)
    if not now_in(LEVEL_NAME):
        try:
            general.open_level_no_prompt(LEVEL_NAME)
        except Exception as e:
            log("reopen raised: {}".format(e))
        waited = 0
        while not now_in(LEVEL_NAME) and waited < 200:
            general.idle_wait_frames(1)
            waited += 1
    return now_in(LEVEL_NAME)


def main():
    log("start")
    if not open_demo_level():
        log("FAIL: could not open or create the demo level '{}'".format(LEVEL_NAME))
        return
    general.idle_wait_frames(20)

    sprite_type = find_type_id("Sprite")
    light_type = find_type_id("2D Light")
    if sprite_type is None:
        log("FAIL: Diorama Sprite component not found (is the Diorama gem enabled?)")
        return
    if light_type is None:
        log("FAIL: Diorama '2D Light' component not found (rebuild the gem with lighting?)")
        return

    # A large dark backdrop quad (billboard so it faces the camera), behind the
    # creatures (lower sort offset). Tinted dark so the light pool stands out.
    backdrop = make_entity("Backdrop", math.Vector3(0.0, 0.0, 0.0), sprite_type)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", backdrop, BACKDROP_TEXTURE)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", backdrop, 40.0, 24.0)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetBillboard", backdrop, True)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTint", backdrop, 0.22, 0.22, 0.30, 1.0)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSortOffset", backdrop, -10.0)

    # A row of five creatures spread across the backdrop, at a nearer depth.
    count = 5
    spacing = 6.0
    for i in range(count):
        x = (i - (count - 1) * 0.5) * spacing
        eid = make_entity("Creature{}".format(i + 1), math.Vector3(x, 0.0, 1.0), sprite_type)
        diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", eid, CREATURE_TEXTURE)
        diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", eid, 3.0, 3.0)
        diorama.DioramaSpriteRequestBus(bus.Event, "SetBillboard", eid, True)
        diorama.DioramaSpriteRequestBus(bus.Event, "SetSortOffset", eid, 0.0)

    # The light: a warm point light at the center, just in front of the scene.
    # Configured through the typed bus -- no property-path strings, no math types.
    light = make_entity("OrbitLight", math.Vector3(0.0, 0.0, 3.0), light_type)
    diorama.DioramaLightRequestBus(bus.Event, "SetDirectional", light, False)  # point light
    diorama.DioramaLightRequestBus(bus.Event, "SetColor", light, 1.0, 0.55, 0.15)  # warm orange
    diorama.DioramaLightRequestBus(bus.Event, "SetIntensity", light, 2.5)
    diorama.DioramaLightRequestBus(bus.Event, "SetRadius", light, 8.0)
    diorama.DioramaLightRequestBus(bus.Event, "SetEnabled", light, True)

    # Close the loop: read resolved light state instead of eyeballing a screenshot.
    info = diorama.DioramaLightRequestBus(bus.Event, "GetLightInfo", light)
    log("light: directional={} color=({:.2f},{:.2f},{:.2f}) intensity={:.2f} radius={:.2f} enabled={}".format(
        info.isDirectional, info.r, info.g, info.b, info.intensity, info.radius, info.enabled))
    ok = (not info.isDirectional) and abs(info.radius - 8.0) < 1e-3 and abs(info.intensity - 2.5) < 1e-3
    log("light config readback {}".format("OK" if ok else "MISMATCH"))

    # Let the sprite textures stream in before we settle, so the scene is fully
    # drawn (otherwise sprites briefly show the magenta fallback tint).
    general.idle_wait_frames(60)

    # Save so the demo level persists and can be reopened without re-running. Guard:
    # only save if we are actually in the demo level, never another scene -- a
    # misfire here would overwrite whatever level is open (e.g. the twin-stick).
    if general.get_current_level_name() == LEVEL_NAME:
        try:
            general.save_level()
        except Exception as e:
            log("save_level raised: {}".format(e))
    else:
        log("WARN: current level is '{}', not '{}'; skipping save to avoid overwriting it".format(
            general.get_current_level_name(), LEVEL_NAME))

    log("Scene built in its own level '{}'. To animate: add a Lua Script".format(LEVEL_NAME))
    log("component to 'OrbitLight' with diorama/examples/lighting/light_orbit.lua")
    log("and the glow will sweep the scene.")
    log("Toggle the whole effect with the console var: r_dioramaSpriteLighting 0 / 1")
    log("done")


main()
