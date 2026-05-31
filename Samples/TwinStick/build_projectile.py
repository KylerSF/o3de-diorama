"""
Build the twin-stick projectile entity in the open level.

Step 3 of the twin-stick sample game: projectiles. Create the entity here, then
save it as a spawnable prefab and point the player's ProjectilePrefab property at
it; the player spawns one toward the aim direction on the fire input.

The projectile is a real O3DE entity:
  - Transform                  (the player sets position and aim yaw at spawn)
  - Diorama Sprite             (the visual; billboards to face the camera)
  - PhysX Dynamic Rigid Body   (launched along its forward; gravity off)
  - PhysX Collider             (reports collisions; report on the body)
  - Lua Script                 (twin_stick_projectile.lua: launch, lifetime, hit)
  - Tag                        ("Projectile")

Run in the editor:
  <engine>/bin/Linux/profile/Default/Editor \
    --project-path=/path/to/YourProject \
    --runpython /path/to/o3de-diorama/Samples/TwinStick/build_projectile.py
"""
import azlmbr
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity
import azlmbr.legacy.general as general
import azlmbr.math as math

diorama = azlmbr.diorama

PROJECTILE_TEXTURE = "diorama/textures/sample_sprite.png"
PROJECTILE_SCRIPT = "diorama/twinstick/scripts/twin_stick_projectile.lua"


def log(msg):
    print("DIORAMA_TWINSTICK: {}".format(msg))


def find_type_id(name):
    ids = editor.EditorComponentAPIBus(bus.Broadcast, "FindComponentTypeIdsByEntityType", [name], 0)
    if ids and len(ids) > 0 and not ids[0].IsNull():
        return ids[0]
    return None


def add_component(eid, display_name):
    type_id = find_type_id(display_name)
    if type_id is None:
        log("WARN: component type not found: {}".format(display_name))
        return
    editor.EditorComponentAPIBus(bus.Broadcast, "AddComponentsOfType", eid, [type_id])
    log("added component: {}".format(display_name))


def open_default_level():
    general.idle_enable(True)
    try:
        general.open_level_no_prompt("DefaultLevel")
    except Exception as e:
        log("open_level raised: {}".format(e))
    waited = 0
    while general.get_current_level_name() in ("", "Untitled") and waited < 400:
        general.idle_wait_frames(1)
        waited += 1
    general.idle_wait_frames(20)


def main():
    log("start")
    open_default_level()

    if find_type_id("Sprite") is None:
        log("FAIL: Diorama Sprite component not found (is the Diorama gem enabled?)")
        return

    eid = editor.ToolsApplicationRequestBus(
        bus.Broadcast, "CreateNewEntityAtPosition", math.Vector3(0.0, 0.0, 0.5), azlmbr.entity.EntityId())
    editor.EditorEntityAPIBus(bus.Event, "SetName", eid, "Projectile")

    add_component(eid, "Sprite")
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", eid, PROJECTILE_TEXTURE)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", eid, 0.3, 0.3)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetBillboard", eid, True)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTint", eid, 1.0, 1.0, 0.5, 1.0)  # bright, reads as a shot
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSortOffset", eid, 11.0)

    add_component(eid, "PhysX Dynamic Rigid Body")
    add_component(eid, "PhysX Collider")
    add_component(eid, "Lua Script")
    add_component(eid, "Tag")

    log("Set the Lua Script asset to '{}', add the 'Projectile' tag, enable the".format(PROJECTILE_SCRIPT))
    log("collider's 'Report collisions' option, then SAVE AS a spawnable prefab.")
    log("Point the player's ProjectilePrefab property at that prefab. See README.")
    log("done")


main()
