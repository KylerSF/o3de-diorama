# How-To: 2D Materials (hit-flash)

Diorama's 2D materials add per-sprite shader effects on top of the lit sprite.
The first one is the **hit-flash**: blend a sprite toward a color after lighting,
for the classic damage/hit pop. It is the material backbone the next effects
(outline, dissolve) build on.

If you just want to see it, run the example, attach the script, and enter game
mode. It builds its own level (`DioramaMaterialsDemo`):

```bash
<engine>/bin/Linux/profile/Default/Editor \
  --project-path=/path/to/YourProject \
  --runpython /path/to/o3de-diorama/Docs/examples/materials_demo.py
```

## Important: the flash animation runs at game time

A constant flash can be set in the inspector (the Sprite component's **Flash
Color** / **Flash Amount** fields), but the *animated* hit-flash is driven by a
script at game time. The demo's `flash_pulse.lua` does this.

## Flash from the inspector

Select a sprite, and on its Sprite component set **Flash Amount** above 0 and pick
a **Flash Color**. The sprite blends toward that color (after lighting). This is a
static flash, useful to preview the look.

## Flash from script (the hit pop)

The real use is transient: on a hit, snap the flash to 1, then ease it back to 0.
Use the sprite's `DioramaSpriteRequestBus`:

```lua
-- On a hit:
DioramaSpriteRequestBus.Event.SetFlash(enemy, 1.0, 1.0, 1.0, 1.0)  -- full white flash
-- Each tick after, ease back:
amount = math.max(0.0, amount - deltaTime / fadeTime)
DioramaSpriteRequestBus.Event.SetFlash(enemy, 1.0, 1.0, 1.0, amount)
```

`flash_pulse.lua` (in the demo) wraps exactly this on a timer. The flash blends in
*after* lighting, so it reads as a bright pop even on a lit or normal-mapped
sprite. Channels and amount are clamped to 0..1.

## 3. Run the demo

After running `materials_demo.py`:

1. Select a **Target** entity, **Add Component -> Lua Script**, set the script to
   `diorama/examples/materials/flash_pulse.lua`.
2. **Enter game mode** (Ctrl+G): the sprite flashes on the timer. Tune the
   script's `FlashR/G/B`, `Interval`, and `FadeTime` properties.

## How it works

A flashing sprite carries its flash color + amount in its batch key, so it draws
in its own batch with the flash set as a per-draw constant; the shader does
`lerp(litColor, flashColor, amount)` as the last step. Sprites with amount 0 (the
default) are unaffected and batch together as before. See
[Docs/design/2d-materials.md](../design/2d-materials.md) for the design and the
roadmap to outline/dissolve/additive blend.
