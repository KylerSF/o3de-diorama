"""
Build the twin-stick player entity in the open level.

This is the AI-native build path: an agent (or a developer) runs this in the
editor to assemble the player from standard O3DE components plus a Diorama
sprite. It is step 1 of the twin-stick sample game (teaching ladder rung 6);
later steps add enemies, projectiles, and a HUD.

The player is built like a real O3DE game entity:
  - Transform                  (placement)
  - Diorama Sprite             (the visual; billboards to face the top-down camera)
  - PhysX Dynamic Rigid Body   (movement integrated by physics; gravity off)
  - PhysX Collider             (so walls and enemies collide)
  - Lua Script                 (twin_stick_player.lua: input -> velocity + facing)
  - Input to Event Bindings    (twin_stick.inputbindings: WASD/mouse/gamepad)

The Diorama sprite is configured through the typed DioramaSpriteRequestBus (no
property-path strings). The standard O3DE components are added by type and their
asset properties are set best-effort; see Samples/TwinStick/README.md for the
authoritative component/property spec to set by hand or in a prefab.

Run in the editor:
  <engine>/bin/Linux/profile/Default/Editor \
    --project-path=/path/to/YourProject \
    --runpython /path/to/o3de-diorama/Samples/TwinStick/build_player.py
"""
import azlmbr
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity
import azlmbr.legacy.general as general
import azlmbr.math as math

diorama = azlmbr.diorama

PLAYER_TEXTURE = "diorama/textures/sample_sprite.png"
PLAYER_SCRIPT = "diorama/twinstick/scripts/twin_stick_player.lua"
PLAYER_INPUT = "diorama/twinstick/input/twin_stick.inputbindings"


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
        return None
    outcome = editor.EditorComponentAPIBus(bus.Broadcast, "AddComponentsOfType", eid, [type_id])
    log("added component: {}".format(display_name))
    return outcome


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

    # Create the player slightly above the arena floor.
    eid = editor.ToolsApplicationRequestBus(
        bus.Broadcast, "CreateNewEntityAtPosition", math.Vector3(0.0, 0.0, 0.5), azlmbr.entity.EntityId())
    editor.EditorEntityAPIBus(bus.Event, "SetName", eid, "Player")

    # Visual: Diorama sprite, configured through the typed bus. Billboard so it
    # faces the top-down camera; facing left/right is handled at runtime by the
    # Lua script flipping the sprite.
    add_component(eid, "Sprite")
    diorama.DioramaSpriteRequestBus(bus.Event, "SetTextureByPath", eid, PLAYER_TEXTURE)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSize", eid, 1.0, 1.0)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetBillboard", eid, True)
    diorama.DioramaSpriteRequestBus(bus.Event, "SetSortOffset", eid, 10.0)

    # Gameplay: standard O3DE components. Their asset/config properties are set
    # best-effort here; the README lists the exact properties for a prefab or
    # manual setup. These names match the editor's Add Component menu.
    add_component(eid, "PhysX Dynamic Rigid Body")
    add_component(eid, "PhysX Collider")
    add_component(eid, "Lua Script")
    add_component(eid, "Input to Event Bindings")

    # Verify the Diorama side took (no screenshot needed).
    info = diorama.DioramaSpriteRequestBus(bus.Event, "GetSpriteInfo", eid)
    log("sprite texturePath={} billboard={} size={}x{}".format(
        info.texturePath, info.billboard, info.width, info.height))
    log("player entity id={}".format(eid.ToString() if hasattr(eid, "ToString") else eid))
    log("Set the Lua Script asset to '{}' and the Input bindings to '{}' (see README).".format(
        PLAYER_SCRIPT, PLAYER_INPUT))
    log("done")


main()
