#!/usr/bin/env python3
"""
Author the DioramaSolarSystem level prefab offline: the cartoon solar-system
diorama (view from a planet's surface), built to actually SHOWCASE the gem:

  - world-space sprites + 2.5D depth sort (sky < bodies < ridge < ground < props)
  - billboarding
  - 2D dynamic lighting: a directional "sun" 2D Light raking a normal-mapped,
    layered terrain so the ground reads as 3D rolling hills (not a flat disk)
  - glow/bloom: a 2D Look (Atom bloom + vignette) on the camera, plus emissive
    sun / crystals / comet / glowing flora pushed into the bloom range
  - drop shadows under the billboarded props (r_dioramaSpriteShadows)

Captured headless with capture_level_noap.sh (pass the lighting cvars via
CAP_EXTRA). Motion features (animation, parallax, camera pan) are layered on in a
follow-up pass for the video.
"""
import hashlib, json, os
from PIL import Image

PROJ = os.path.expanduser("~/PROJECTS/DioramaSandbox")
GEMTX = os.path.expanduser("~/PROJECTS/o3de-diorama/Assets/Diorama/Textures")
LEVEL = "DioramaSolarSystem"
HW = 8.2; FW = 2 * HW; FH = FW * 9 / 16    # ortho frame 16.4 x 9.225


def guid(p):
    h = hashlib.sha1(p.lower().replace("\\", "/").encode()).digest()
    b = bytearray(h[:16]); b[6] = (b[6] & 0xF) | 0x50; b[8] = (b[8] & 0x3F) | 0x80
    s = b.hex().upper()
    return "{%s-%s-%s-%s-%s}" % (s[0:8], s[8:12], s[12:16], s[16:20], s[20:32])


def tex(name):
    src = "diorama/textures/%s.png" % name
    return {"assetId": {"guid": guid(src), "subId": 1000}, "assetHint": src + ".streamingimage"}


def aspect(name):
    im = Image.open("%s/%s.png" % (GEMTX, name)); return im.width / im.height


_cid = [992000000000]
def cid():
    _cid[0] += 1; return _cid[0]


CONTAINER = "Entity_[992000000000]"
entities, order = {}, []


def sprite(label, texname, x, y, h, sort, tint=(1, 1, 1, 1), billboard=True, nmap=None, emissive=None):
    """One world-space sprite. nmap = normal-map texture name (enables shaped
    lighting); emissive = (r, g, b, intensity) to make it glow + bloom."""
    w = h * aspect(texname)
    cfg = {"Texture": tex(texname), "Size": [w, h], "Billboard": billboard,
           "Tint": list(tint), "SortOffset": float(sort)}
    if nmap:
        cfg["NormalMap"] = tex(nmap)
    if emissive:
        cfg["EmissiveColor"] = [emissive[0], emissive[1], emissive[2], 1.0]
        cfg["EmissiveIntensity"] = float(emissive[3])
    eid = "Entity_[%d]" % cid()
    entities[eid] = {
        "Id": eid, "Name": label,
        "Components": {
            "T": {"$type": "{27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0} TransformComponent", "Id": cid(),
                  "Parent Entity": CONTAINER, "Transform Data": {"Translate": [x, y, 1.0]}},
            "S": {"$type": "Diorama::EditorSpriteComponent", "Id": cid(), "Config": cfg},
        },
    }
    order.append(eid)


# --- sky backdrop (fills frame, farthest) ---
sprite("Sky", "cartoon_sky", 0.0, 0.0, FH + 1.0, -100)

# --- bodies in the sky, by APPARENT size from Earth's surface ---
# The Moon is by far the closest object, so it dominates; the Sun is the only
# other large disc (~same angular size). Every planet is millions of km farther,
# so they read as small distant discs/points sorted BEHIND the Moon.
# (name, x, y, height, sort)  -- lower sort = farther back
BODIES = [
    ("sun", -5.8, 2.1, 2.9, -58),      # largest disc, bigger than the Moon
    ("moon", 3.6, 2.5, 1.9, -40),      # closest body, dominant; right side of frame
    ("mercury", -7.4, 3.9, 0.15, -58), ("venus", -4.0, 4.1, 0.30, -58),
    ("mars", 0.6, 4.05, 0.22, -58), ("jupiter", 2.5, 3.9, 0.55, -58),
    ("saturn", 4.9, 4.05, 0.5, -58), ("uranus", 6.3, 3.45, 0.30, -58),
    ("neptune", 7.4, 4.05, 0.27, -58), ("pluto", 8.0, 3.3, 0.12, -58),
    ("comet", 1.3, 3.0, 0.45, -52), ("asteroid", -3.1, 2.0, 0.26, -52),
]
# the Moon "emits light" -> soft white emissive glow, like the sun/comet
# Moon emissive kept low so the rabbit maria + craters stay visible (a high
# uniform emissive add flattens the surface contrast); bloom still gives a halo.
EMISSIVE_BODY = {"sun": (1.0, 0.62, 0.22, 2.0), "comet": (0.5, 0.85, 1.0, 1.8),
                 "moon": (0.8, 0.85, 1.0, 0.45)}
for name, x, y, h, sort in BODIES:
    sprite(name.capitalize(), "cartoon_" + name, x, y, h, sort, emissive=EMISSIVE_BODY.get(name))

# --- layered terrain (atmospheric depth) + dynamic lighting via normal maps ---
# far hazy ridge peeking above the foreground horizon
hf = FW / aspect("terrain_far")
sprite("RidgeFar", "terrain_far", 0.0, 1.3, hf, -20, nmap="terrain_far_n")
# foreground convex ground (transparent above its horizon -> sky/ridge show through)
sprite("Ground", "terrain_fore", 0.0, -1.7, 11.0, 0, nmap="terrain_fore_n")

# --- hero foreground props: a gnarled oak, one purple crystal, one glowing
# mushroom, one left-leaning rock. y raised so each base sits ON the grass. ---
# (texname, x, y, height, emissive)
PROPS = [
    ("oak", -2.2, 0.1, 3.5, None),
    ("rock", -5.0, -1.4, 1.4, None),
    ("crystal", 6.6, -0.9, 1.1, (0.55, 0.35, 0.95, 1.6)),   # purple glow, far right, smaller
    ("mushroom", 3.0, -2.2, 1.7, (1.0, 0.86, 0.6, 1.6)),    # warm glowing toadstool cluster
]
for name, x, y, h, em in PROPS:
    label = name.replace("_", " ").title().replace(" ", "")
    sprite(label, "cartoon_" + name, x, y, h, 10, emissive=em)

# --- directional "sun" 2D Light raking in from the upper-left (where the sun is) ---
LIGHT = "Entity_[880088008800]"
entities[LIGHT] = {
    "Id": LIGHT, "Name": "SunLight",
    "Components": {
        "T": {"$type": "{27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0} TransformComponent", "Id": cid(),
              "Parent Entity": CONTAINER, "Transform Data": {"Translate": [-7.0, 1.6, 2.0]}},
        "L": {"$type": "Diorama::EditorDioramaLightComponent", "Id": cid(),
              "Config": {"type": 0, "color": [1.0, 0.78, 0.5, 1.0], "intensity": 1.6,
                         "direction": [0.8, -0.5, -0.55], "enabled": True}},
    },
}
order.append(LIGHT)

# --- capture camera (orthographic) + 2D Look (bloom + vignette) ---
CAM = "Entity_[770077007700]"
SCRIPT = {"assetId": {"guid": "{99AB7C02-81D9-5D52-918E-7BE26AFB4DF6}", "subId": 1},
          "assetHint": "diorama/examples/make_active_camera.luac"}
entities[CAM] = {
    "Id": CAM, "Name": "CaptureCam",
    "Components": {
        "EditorCameraComponent": {"$type": "{CA11DA46-29FF-4083-B5F6-E02C3A8C3A3D} EditorCameraComponent", "Id": cid()},
        "EditorOnlyEntityComponent": {"$type": "EditorOnlyEntityComponent", "Id": cid()},
        "Look": {"$type": "Diorama::EditorDioramaLookComponent", "Id": cid(),
                 "Config": {"bloomEnabled": True, "bloomThreshold": 1.0, "bloomKnee": 0.5,
                            "bloomIntensity": 0.6, "vignetteEnabled": True, "vignetteIntensity": 0.18}},
        "TransformComponent": {"$type": "{27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0} TransformComponent", "Id": cid(),
                               "Parent Entity": CONTAINER,
                               "Transform Data": {"Translate": [0.0, 0.0, 30.0], "Rotate": [-90.0, 0.0, 0.0]}},
        "ActiveCamScript": {"$type": "ScriptEditorComponent", "Id": cid(),
            "ScriptComponent": {"Properties": {"Properties": [
                {"$type": "AzFramework::ScriptPropertyBoolean", "id": 1, "name": "Orthographic", "value": True},
                {"$type": "AzFramework::ScriptPropertyNumber", "id": 2, "name": "OrthographicHalfWidth", "value": HW},
            ]}, "Script": SCRIPT}, "ScriptAsset": SCRIPT},
    },
}
order.append(CAM)

container = {
    "Id": CONTAINER, "Name": "Level",
    "Components": {
        "EditorEntitySortComponent": {"$type": "EditorEntitySortComponent", "Id": cid(), "Child Entity Order": order},
        "EditorPrefabComponent": {"$type": "EditorPrefabComponent", "Id": cid()},
        "TransformComponent": {"$type": "{27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0} TransformComponent", "Id": cid(), "Parent Entity": ""},
    },
}
out_dir = "%s/Levels/%s" % (PROJ, LEVEL); os.makedirs(out_dir, exist_ok=True)
out = "%s/%s.prefab" % (out_dir, LEVEL)
json.dump({"ContainerEntity": container, "Entities": entities}, open(out, "w"), indent=4)
print("wrote %s (%d entities)" % (out, len(entities)))
