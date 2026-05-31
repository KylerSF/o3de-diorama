# Diorama How-To Guides

Step-by-step guides that follow the teaching ladder in
[../examples-outline.md](../examples-outline.md), from a single sprite to a
complete game. Each rung has a written guide and, where applicable, a runnable
example under [../examples](../examples).

## Core ladder

| Rung | Guide | Status |
| ---- | ----- | ------ |
| 1 | Hello Sprite | Planned |
| 2 | Animated Sprite | Planned |
| 3 | [Sprite Atlas](03-sprite-atlas.md) | Written |
| 4 | [Tilemap](04-tilemap.md) | Written |
| 5 | Parallax and Layers (2.5D) | Planned |
| 6 | [Twin-Stick Shooter (capstone)](06-twin-stick.md) | Written |

Guides are being written alongside the features they teach. Rungs 1 and 2 cover
functionality that already ships (the Sprite component and sprite-sheet
animation); their written guides are still to come.

## Runnable examples

| Example | Script |
| ------- | ------ |
| Sprite Atlas | [../examples/sprite_atlas.py](../examples/sprite_atlas.py) |
| Tilemap | [../examples/tilemap.py](../examples/tilemap.py) |

Run an example in the editor:

```bash
<engine>/bin/Linux/profile/Default/Editor \
  --project-path=/path/to/YourProject \
  --runpython /path/to/o3de-diorama/Docs/examples/<script>.py
```
