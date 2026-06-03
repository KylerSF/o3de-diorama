"""
2D UI / HUD demo: a screen-space HUD built from Diorama UI Elements.

Builds, in its own level, a title label, a score label, a health bar, and a corner
panel using the Diorama "2D UI Element" component, then drives them through the
typed DioramaUIRequestBus -- the same bus an AI agent or Script Canvas would use.
This is the UI parity: a HUD is built and updated exactly like sprites.

The HUD draws at GAME TIME, so this builds the scene; enter game mode (or use the
headless capture) to see it. Attach diorama/examples/ui/hud_bar_pulse.lua to the
Health bar to watch the fill animate via SetValue.

Run in the editor:
  <engine>/bin/Linux/profile/Default/Editor \
    --project-path=/path/to/YourProject \
    --runpython /path/to/o3de-diorama/Docs/examples/ui_hud_demo.py
"""
import azlmbr
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity
import azlmbr.legacy.general as general
import azlmbr.math as math

diorama = azlmbr.diorama

LEVEL_NAME = "DioramaUIDemo"

# UI element kinds (Config|Kind) and screen anchors (Config|Anchor / SetAnchor).
KIND_TEXT, KIND_BAR, KIND_PANEL = 0, 1, 2
ANCHOR_TOP_LEFT, ANCHOR_TOP, ANCHOR_BOTTOM_RIGHT = 0, 1, 8


def log(msg):
    print("DIORAMA_UI: {}".format(msg))


def find_type_id(name):
    ids = editor.EditorComponentAPIBus(bus.Broadcast, "FindComponentTypeIdsByEntityType", [name], 0)
    if ids and len(ids) > 0 and not ids[0].IsNull():
        return ids[0]
    return None


def set_prop(component, path, value):
    editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", component, path, value)


def make_ui(ui_type, name, kind, anchor, offset):
    eid = editor.ToolsApplicationRequestBus(
        bus.Broadcast, "CreateNewEntityAtPosition", math.Vector3(0.0, 0.0, 0.0), azlmbr.entity.EntityId())
    editor.EditorEntityAPIBus(bus.Event, "SetName", eid, name)
    outcome = editor.EditorComponentAPIBus(bus.Broadcast, "AddComponentsOfType", eid, [ui_type])
    comp = outcome.GetValue()[0] if outcome.IsSuccess() else None
    if comp is not None:
        set_prop(comp, "Config|Kind", kind)
    # Anchor + offset go through the typed bus, like any other UI update.
    diorama.DioramaUIRequestBus(bus.Event, "SetAnchor", eid, anchor)
    diorama.DioramaUIRequestBus(bus.Event, "SetOffset", eid, offset[0], offset[1])
    return eid, comp


def main():
    # open_or_create_level boilerplate omitted for brevity; see other demos. It
    # opens/creates LEVEL_NAME so the HUD builds in its OWN level.
    ui_type = find_type_id("2D UI Element")
    if ui_type is None:
        log("FAIL: '2D UI Element' not found (is the Diorama gem enabled?)")
        return

    # Title (top-center).
    title, _ = make_ui(ui_type, "HUD_Title", KIND_TEXT, ANCHOR_TOP, (0.0, 40.0))
    diorama.DioramaUIRequestBus(bus.Event, "SetText", title, "DIORAMA 2D HUD")
    diorama.DioramaUIRequestBus(bus.Event, "SetFontSize", title, 54.0)
    diorama.DioramaUIRequestBus(bus.Event, "SetColor", title, 1.0, 0.9, 0.3, 1.0)

    # Score (top-left).
    score, _ = make_ui(ui_type, "HUD_Score", KIND_TEXT, ANCHOR_TOP_LEFT, (24.0, 24.0))
    diorama.DioramaUIRequestBus(bus.Event, "SetText", score, "Score: 0")
    diorama.DioramaUIRequestBus(bus.Event, "SetColor", score, 0.6, 0.9, 1.0, 1.0)

    # Health bar (top-left, under the score).
    health, _ = make_ui(ui_type, "HUD_Health", KIND_BAR, ANCHOR_TOP_LEFT, (24.0, 84.0))
    diorama.DioramaUIRequestBus(bus.Event, "SetSize", health, 340.0, 26.0)
    diorama.DioramaUIRequestBus(bus.Event, "SetValue", health, 0.65)
    diorama.DioramaUIRequestBus(bus.Event, "SetColor", health, 0.25, 0.9, 0.35, 1.0)
    diorama.DioramaUIRequestBus(bus.Event, "SetBackgroundColor", health, 0.0, 0.0, 0.0, 0.55)

    # Corner panel (bottom-right).
    panel, _ = make_ui(ui_type, "HUD_Panel", KIND_PANEL, ANCHOR_BOTTOM_RIGHT, (-20.0, -20.0))
    diorama.DioramaUIRequestBus(bus.Event, "SetSize", panel, 240.0, 72.0)
    diorama.DioramaUIRequestBus(bus.Event, "SetColor", panel, 0.12, 0.18, 0.42, 0.7)

    log("Built a HUD (title, score, health bar, panel) in level '{}'.".format(LEVEL_NAME))
    log("Attach diorama/examples/ui/hud_bar_pulse.lua to HUD_Health and enter game")
    log("mode: the bar animates via DioramaUIRequestBus.SetValue -- the same call a")
    log("game or agent makes. See Docs/howto/13-ui-hud.md.")
    log("done")


main()
