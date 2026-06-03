"""
2D particle demo: a continuous emitter fountain.

Builds a single emitter entity with the Diorama "2D Particle Emitter" component,
configured (texture, rate, direction, colors) so that, in game mode, it sprays a
fountain of particles that arc under gravity and fade over their life. Each
particle is a billboarded sprite quad drawn through the existing batched sprite
path, so the whole fountain is one draw call. The simulation is the unit-tested
Particles2D core; the emission shape and randomness live in the component.

Particles run at GAME TIME (the emitter ticks in game mode, not the editor
viewport), so this builds the scene; enter game mode to see it.

Run in the editor:
  <engine>/bin/Linux/profile/Default/Editor \
    --project-path=/path/to/YourProject \
    --runpython /path/to/o3de-diorama/Docs/examples/particles_demo.py
"""
import azlmbr
import azlmbr.asset
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity
import azlmbr.legacy.general as general
import azlmbr.math as math

diorama = azlmbr.diorama

LEVEL_NAME = "DioramaParticleDemo"
# Solid-white particle texture, tinted per particle by the start/end color.
PARTICLE_TEXTURE = "diorama/textures/white_sprite.png.streamingimage"


def log(msg):
    print("DIORAMA_PARTICLES: {}".format(msg))


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


def set_prop(comp, path, value):
    editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", comp, path, value)


def main():
    log("start")
    if not open_or_create_level(LEVEL_NAME):
        log("FAIL: could not open or create the demo level '{}'".format(LEVEL_NAME))
        return
    general.idle_wait_frames(20)

    emitter_type = find_type_id("2D Particle Emitter")
    if emitter_type is None:
        log("FAIL: '2D Particle Emitter' not found (rebuild the gem with the particle component?)")
        return

    eid = editor.ToolsApplicationRequestBus(
        bus.Broadcast, "CreateNewEntityAtPosition", math.Vector3(0.0, 0.0, 1.0), azlmbr.entity.EntityId())
    editor.EditorEntityAPIBus(bus.Event, "SetName", eid, "Fountain")
    outcome = editor.EditorComponentAPIBus(bus.Broadcast, "AddComponentsOfType", eid, [emitter_type])
    if not outcome.IsSuccess():
        log("FAIL: could not add the 2D Particle Emitter component")
        return
    comp = outcome.GetValue()[0]

    # Configure a fountain through the component's authored config. The texture is
    # an asset reference (set by id); the rest are scalars with sensible defaults,
    # so we just tune a few for a continuous upward spray.
    tex_id = azlmbr.asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", PARTICLE_TEXTURE, math.Uuid(), False)
    set_prop(comp, "Config|Texture", tex_id)
    set_prop(comp, "Config|Rate", 80.0)            # continuous spray
    set_prop(comp, "Config|Burst Count", 0)        # no one-shot burst; steady stream
    set_prop(comp, "Config|Direction", 90.0)       # up (+Y)
    set_prop(comp, "Config|Spread", 40.0)          # a tight-ish cone
    set_prop(comp, "Config|Speed Min", 4.0)
    set_prop(comp, "Config|Speed Max", 7.0)
    set_prop(comp, "Config|Gravity", math.Vector2(0.0, -6.0))
    set_prop(comp, "Config|Start Size", 0.5)
    set_prop(comp, "Config|End Size", 0.0)
    set_prop(comp, "Config|Max Particles", 400)

    general.idle_wait_frames(30)
    if general.get_current_level_name() == LEVEL_NAME:
        try:
            general.save_level()
        except Exception as e:
            log("save_level raised: {}".format(e))
    else:
        log("WARN: current level is '{}', not '{}'; skipping save to avoid overwriting it".format(
            general.get_current_level_name(), LEVEL_NAME))

    log("Fountain emitter built in its own level '{}'.".format(LEVEL_NAME))
    log("Enter game mode to see the fountain spray, arc under gravity, and fade.")
    log("Drive it from script via DioramaParticleRequestBus (Burst, SetRate, SetGravity,")
    log("SetStartColor/EndColor, ...). See Docs/howto/09-particles.md.")
    log("done")


main()
