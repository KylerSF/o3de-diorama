#!/usr/bin/env python3
"""Generate a thousands-of-sprites stress level for the Diorama gem, offline.

Authors a level prefab full of sprites that all share one texture. Because the batched
feature processor groups same-texture sprites into one batch, thousands of them collapse
into a handful of draw calls. Measured at runtime: 2500 same-texture sprites render as a
single draw batch (confirmed by temporarily instrumenting the feature processor's batch
count; the batching is also covered by the SpriteBatchPlan unit tests). Authored as pure
prefab JSON, no editor required (the robust path for tooling).

Reading the actual draw-call count: the "Total Draw Item Count" stat is shown by the
EDITOR viewport overlay (open the level in the editor), not the GameLauncher. Do NOT use
a headless Xvfb capture as a perf benchmark: under Xvfb the swapchain presents into a
software framebuffer, which caps throughput at ~48 FPS on this setup regardless of scene
load, so the FPS there reflects the present path, not the GPU cost of the sprites.

Usage:
  gen_stress_level.py [count] [--project PATH] [--level NAME]

Defaults: 2500 sprites, project ~/PROJECTS/DioramaSandbox, level DioramaStress.
"""
import argparse
import copy
import json
import math
import os

DEFAULT_PROJECT = os.path.expanduser("~/PROJECTS/DioramaSandbox")
# Bundled shared texture every stress sprite uses (so they all batch). guid is the
# v5 asset id of the lowercased product path; subId 1000 is the streaming image.
TEXTURE = {"guid": "{23D59181-9740-51AA-8BB4-4F793E26749A}", "subId": 1000}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("count", nargs="?", type=int, default=2500)
    ap.add_argument("--project", default=DEFAULT_PROJECT)
    ap.add_argument("--level", default="DioramaStress")
    a = ap.parse_args()

    n = a.count
    cols = int(math.ceil(math.sqrt(n)))
    spacing = 0.9
    span = (cols - 1) * spacing
    half = span / 2.0

    container_id = "Entity_[100000000000]"
    doc = {
        "ContainerEntity": {
            "Id": container_id,
            "Name": "Level",
            "Components": {
                "EditorEntitySortComponent": {
                    "$type": "EditorEntitySortComponent",
                    "Id": 100000000001,
                    "Child Entity Order": [],
                },
            },
        },
        "Entities": {},
    }
    order = doc["ContainerEntity"]["Components"]["EditorEntitySortComponent"]["Child Entity Order"]

    cid = 200000000000  # running component-id counter (unique across the prefab)

    def new_cid():
        nonlocal cid
        cid += 1
        return cid

    for i in range(n):
        r, c = divmod(i, cols)
        x = -half + c * spacing
        y = half - r * spacing
        # rainbow tint by index so the field reads as content, not a white blob
        hue = (i / max(1, n)) * 6.0
        k = hue - math.floor(hue)
        ramp = [(1, k, 0), (1 - k, 1, 0), (0, 1, k), (0, 1 - k, 1), (k, 0, 1), (1, 0, 1 - k)]
        rr, gg, bb = ramp[int(hue) % 6]

        key = "Entity_[%d]" % (300000000000 + i)
        doc["Entities"][key] = {
            "Id": key,
            "Name": "S%d" % i,
            "Components": {
                "Diorama::EditorSpriteComponent": {
                    "$type": "Diorama::EditorSpriteComponent",
                    "Id": new_cid(),
                    "Config": {
                        "Texture": dict(TEXTURE),
                        "Size": [0.6, 0.6],
                        "Tint": [rr, gg, bb],
                        "Billboard": True,
                    },
                },
                "TransformComponent": {
                    "$type": "{27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0} TransformComponent",
                    "Id": new_cid(),
                    "Parent Entity": container_id,
                    "Transform Data": {"Translate": [x, y, 1.0]},
                },
                "EditorEntitySortComponent": {"$type": "EditorEntitySortComponent", "Id": new_cid(), "Child Entity Order": []},
                "EditorInspectorComponent": {"$type": "EditorInspectorComponent", "Id": new_cid()},
                "EditorLockComponent": {"$type": "EditorLockComponent", "Id": new_cid()},
                "EditorVisibilityComponent": {"$type": "EditorVisibilityComponent", "Id": new_cid()},
                "EditorOnlyEntityComponent": {"$type": "EditorOnlyEntityComponent", "Id": new_cid()},
                "EditorEntityIconComponent": {"$type": "EditorEntityIconComponent", "Id": new_cid()},
                "EditorPendingCompositionComponent": {"$type": "EditorPendingCompositionComponent", "Id": new_cid()},
                "EditorDisabledCompositionComponent": {"$type": "EditorDisabledCompositionComponent", "Id": new_cid()},
            },
        }
        order.append(key)

    lvl_dir = os.path.join(a.project, "Levels", a.level)
    os.makedirs(lvl_dir, exist_ok=True)
    out = os.path.join(lvl_dir, a.level + ".prefab")
    with open(out, "w") as f:
        json.dump(doc, f, indent=4)
    print("wrote %s: %d sprites (%dx%d grid, span %.1f units), shared texture" % (out, n, cols, cols, span))


if __name__ == "__main__":
    main()
