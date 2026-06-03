#!/usr/bin/env python3
"""Generate the twin-stick "spread joy" theme textures with PIL.

  heart.png    - red heart (the octopus's harpoon ammo + the float-away FX)
  octopus.png  - jolly purple octopus (the player)
  target.png   - soft white bubble creature (tinted per-entity; grows + floats away)

Rendered at 4x and downscaled for anti-aliasing. These are simple procedural
placeholders -- swap in nicer/AI-generated art by replacing the PNGs (the asset
paths/GUIDs stay the same so no rewiring is needed).
"""
import math
import os
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
SS = 4
N = 512          # exported sprite resolution (was 256); 2x for crisper in-game art
W = N * SS


def canvas():
    return Image.new("RGBA", (W, W), (0, 0, 0, 0))


def save(img, name):
    img.resize((N, N), Image.LANCZOS).save(os.path.join(HERE, name))
    print("wrote", name)


def _heart_shape(d, cx, cy, r, color):
    # a small filled heart centered near (cx, cy) with half-width r (pixels)
    s = r / 16.0
    pts = []
    t = 0.0
    while t < 2.0 * math.pi:
        x = 16.0 * math.sin(t) ** 3
        y = 13.0 * math.cos(t) - 5.0 * math.cos(2 * t) - 2.0 * math.cos(3 * t) - math.cos(4 * t)
        pts.append((cx + x * s, cy - y * s + 2.5 * s))
        t += 0.02
    d.polygon(pts, fill=color)


def heart():
    img = canvas()
    d = ImageDraw.Draw(img)
    cx, cy = W / 2.0, W / 2.0
    scale = W * 0.92 / 34.0
    pts = []
    t = 0.0
    while t < 2.0 * math.pi:
        x = 16.0 * math.sin(t) ** 3
        y = 13.0 * math.cos(t) - 5.0 * math.cos(2 * t) - 2.0 * math.cos(3 * t) - math.cos(4 * t)
        pts.append((cx + x * scale, cy - y * scale + 2.5 * scale))
        t += 0.005
    d.polygon(pts, fill=(230, 46, 84, 255))   # solid heart, no highlight
    save(img, "heart.png")


def octopus():
    # Full-colour plush-style octopus (original art, evoking the reference plush, not
    # copied from it): peach body with bright YELLOW tentacle tips (its signature),
    # darker-peach ribbing on the tentacles, three orange forehead ovals, dark eyes +
    # a smile. The player's baked Tint is white (= these true colours); the mood
    # mechanic multiplies toward red to flush the octopus under threat (multiply can
    # redden but not pale it -- fine for the agitated look). Kept inside the canvas so
    # no tentacle clips.
    img = canvas()
    d = ImageDraw.Draw(img)
    body = (244, 164, 112, 255)   # peach-orange plush body
    tip = (250, 224, 72, 255)     # bright yellow tentacle tips
    rib = (214, 132, 86, 255)     # darker peach ribbing
    spot = (236, 138, 64, 255)    # forehead oval patches
    dark = (38, 30, 52, 255)      # eyes + smile

    def tentacle(bx, by, theta0, curl, length, w0):
        x, y, th = bx, by, theta0
        n = 70
        pts = []
        for i in range(n):
            t = i / (n - 1)
            r = w0 * (1.0 - 0.34 * t)            # stay plump; only a gentle taper
            pts.append((x, y, r, th, t))
            th += curl / n
            x += math.cos(th) * (length / n)
            y += math.sin(th) * (length / n)
        for (px, py, r, _th, t) in pts:                  # peach body (top stays peach)
            d.ellipse([px - r, py - r, px + r, py + r], fill=body)
        for (px, py, r, _th, t) in pts:                  # yellow underside + fully-yellow tip
            if t > 0.84:
                d.ellipse([px - r, py - r, px + r, py + r], fill=tip)   # yellow tip cap
            else:
                yr, oy = r * 0.34, r * 0.70       # thin yellow line along the lower edge only
                d.ellipse([px - yr, py + oy - yr, px + yr, py + oy + yr], fill=tip)

    # Tentacles all around the body (an octopus has 8): two emerge from BEHIND the
    # head's upper sides and curl up/out, the rest fan across the front -- spaced so
    # each reads individually. (base_x, base_y, start_angle, curl). All drawn before
    # the head so their bases tuck under it (the back two then look like they wrap
    # around from behind). Angles are screen-space (0=right, pi/2=down, pi=left).
    # 8 tentacles, ALL attached under the body (no shoulders -- octopuses have none):
    # the bases sit on the body's lower arc and they fan out by angle, the outer ones
    # reaching out to the sides with their tips curling up. (base_x, base_y, angle,
    # curl); angles are screen-space (0=right, pi/2=down, pi=left).
    FAN = [
        (0.36, 0.560, math.pi / 2 + 1.00, 1.2),   # far-left: out + tip curls up
        (0.39, 0.610, math.pi / 2 + 0.62, 1.1),
        (0.44, 0.640, math.pi / 2 + 0.28, 1.0),
        (0.48, 0.655, math.pi / 2 + 0.09, 0.9),
        (0.52, 0.655, math.pi / 2 - 0.09, -0.9),
        (0.56, 0.640, math.pi / 2 - 0.28, -1.0),
        (0.61, 0.610, math.pi / 2 - 0.62, -1.1),
        (0.64, 0.560, math.pi / 2 - 1.00, -1.2),  # far-right: out + tip curls up
    ]
    for (bx, by, th0, cu) in FAN:
        tentacle(bx * W, by * W, th0, cu, 0.26 * W, 0.062 * W)

    # head / mantle
    d.ellipse([W * 0.22, W * 0.14, W * 0.78, W * 0.60], fill=body)
    # three little hearts on the forehead in the plush's orange spot colour, kept high
    # enough that they clear the eyes
    for (sx, sy, sr) in ((0.46, 0.25, 0.048), (0.565, 0.225, 0.055), (0.625, 0.275, 0.044)):
        _heart_shape(d, W * sx, W * sy, W * sr, (236, 138, 64, 255))
    # eyes: dark ovals with a catchlight
    for ex in (0.42, 0.58):
        px, py = W * ex, W * 0.43
        d.ellipse([px - W * 0.040, py - W * 0.052, px + W * 0.040, py + W * 0.052], fill=dark)
        d.ellipse([px - W * 0.028, py - W * 0.044, px - W * 0.009, py - W * 0.024],
                  fill=(255, 255, 255, 235))
    # smile
    d.arc([W * 0.45, W * 0.47, W * 0.55, W * 0.55], start=18, end=162, fill=dark,
          width=int(W * 0.011))
    save(img, "octopus.png")


def target():
    # Soft white bubble; per-entity Tint colors it, and it fades on float-away.
    img = canvas()
    d = ImageDraw.Draw(img)
    cx = W / 2.0
    d.ellipse([W * 0.14, W * 0.14, W * 0.86, W * 0.86], fill=(255, 255, 255, 255))
    # highlight for a soft, happy bubble look
    d.ellipse([W * 0.30, W * 0.24, W * 0.46, W * 0.40], fill=(255, 255, 255, 130))
    save(img, "target.png")


# --- predators (octopus enemies): cute full-colour characters in the same style as
# the octopus -- soft body colour, a big dark dot-eye with a catchlight, a little
# smile. Each is its own texture asset. Side views face LEFT so the sprite flip can
# face the movement direction.
DARK = (40, 32, 54, 255)


def _poly(d, pts, fill):
    d.polygon([(W * x, W * y) for (x, y) in pts], fill=fill)


def _ell(d, x0, y0, x1, y1, fill):
    d.ellipse([W * x0, W * y0, W * x1, W * y1], fill=fill)


def _cute_eye(d, x, y, r):
    rp = r * W
    d.ellipse([W * x - rp, W * y - rp * 1.12, W * x + rp, W * y + rp * 1.12], fill=DARK)
    d.ellipse([W * x - rp * 0.55, W * y - rp * 0.85, W * x - rp * 0.05, W * y - rp * 0.28],
              fill=(255, 255, 255, 235))


def _smile(d, x0, y0, x1, y1):
    d.arc([W * x0, W * y0, W * x1, W * y1], start=20, end=160, fill=DARK, width=max(2, int(W * 0.011)))


def fish():
    # GROUPER: a chunky, big-mouthed, mottled reef fish (file kept as fish.png).
    img = canvas()
    d = ImageDraw.Draw(img)
    c, belly, fin, spot = (158, 134, 98, 255), (198, 180, 144, 255), (128, 106, 74, 255), (110, 90, 62, 255)
    _poly(d, [(0.64, 0.50), (0.98, 0.26), (0.98, 0.74)], fin)             # tail
    _poly(d, [(0.28, 0.30), (0.46, 0.10), (0.62, 0.30)], fin)            # big dorsal
    _ell(d, 0.06, 0.24, 0.72, 0.80, c)                                   # chunky tall body
    _ell(d, 0.14, 0.55, 0.62, 0.79, belly)                              # belly
    _poly(d, [(0.34, 0.76), (0.48, 0.93), (0.58, 0.76)], fin)           # ventral fin
    for (sx, sy, sr) in ((0.36, 0.34, 0.05), (0.50, 0.44, 0.06), (0.42, 0.62, 0.045), (0.58, 0.34, 0.04)):
        d.ellipse([W * sx - W * sr, W * sy - W * sr, W * sx + W * sr, W * sy + W * sr], fill=spot)  # mottling
    d.arc([W * 0.04, W * 0.44, W * 0.30, W * 0.66], start=200, end=340, fill=DARK, width=max(3, int(W * 0.02)))  # big mouth
    _cute_eye(d, 0.25, 0.38, 0.06)
    save(img, "fish.png")


def bird():
    # cute albatross, SIDE view facing left: round head + chubby white body, a darker
    # grey folded wing, and a LONG pale beak with an orange ridge + a hooked tip.
    img = canvas()
    d = ImageDraw.Draw(img)
    c, wing = (248, 248, 252, 255), (148, 158, 176, 255)
    beakc, beaky, beako = (198, 196, 194, 255), (240, 198, 56, 255), (234, 138, 58, 255)
    _poly(d, [(0.80, 0.52), (0.99, 0.40), (0.96, 0.64)], wing)   # tail (behind)
    _poly(d, [(0.50, 0.72), (0.48, 0.88), (0.56, 0.73)], beako)  # leg
    _poly(d, [(0.62, 0.72), (0.60, 0.88), (0.68, 0.73)], beako)  # leg
    _ell(d, 0.36, 0.42, 0.88, 0.80, c)                           # chubby body
    _ell(d, 0.22, 0.22, 0.58, 0.58, c)                           # round head
    _ell(d, 0.48, 0.46, 0.84, 0.70, wing)                        # folded wing (darker)
    # long beak: grey base, a yellow top ridge, and an orange hooked tip
    _poly(d, [(0.30, 0.40), (0.05, 0.43), (0.02, 0.49), (0.07, 0.515), (0.30, 0.515)], beakc)
    _poly(d, [(0.30, 0.40), (0.05, 0.43), (0.07, 0.455), (0.30, 0.425)], beaky)   # yellow culmen
    _poly(d, [(0.05, 0.43), (0.015, 0.49), (0.085, 0.505), (0.085, 0.45)], beako)  # orange hooked tip
    _cute_eye(d, 0.40, 0.39, 0.05)
    save(img, "bird.png")


def penguin():
    # cute EMPEROR penguin, front view: a TALL narrow black body, cream belly, yellow
    # ear patches, slim flippers hanging at the sides (no broad shoulders), orange
    # beak + feet, two eyes.
    img = canvas()
    d = ImageDraw.Draw(img)
    dk, cream, beak = (44, 50, 66, 255), (250, 246, 214, 255), (248, 152, 60, 255)
    _poly(d, [(0.39, 0.90), (0.32, 0.985), (0.52, 0.94)], beak)  # left foot
    _poly(d, [(0.61, 0.90), (0.68, 0.985), (0.48, 0.94)], beak)  # right foot
    # thin flippers tucked low + tight against the (now rounder) body
    _poly(d, [(0.30, 0.52), (0.25, 0.58), (0.27, 0.84), (0.34, 0.78)], dk)
    _poly(d, [(0.70, 0.52), (0.75, 0.58), (0.73, 0.84), (0.66, 0.78)], dk)
    # chubby silhouette: a fat round body with a NARROWER head on top (no shoulders)
    _ell(d, 0.23, 0.30, 0.77, 0.94, dk)                          # fat body
    _ell(d, 0.31, 0.10, 0.69, 0.40, dk)                          # rounder, less-tall head (wider than before)
    _ell(d, 0.31, 0.38, 0.69, 0.90, cream)                       # cream belly (only cream shape)
    for ex in (0.43, 0.57):                                       # white-based eyes, set wider apart
        d.ellipse([W * ex - W * 0.032, W * 0.21 - W * 0.038, W * ex + W * 0.032, W * 0.21 + W * 0.038], fill=(250, 250, 248, 255))
        d.ellipse([W * ex - W * 0.016, W * 0.215 - W * 0.022, W * ex + W * 0.008, W * 0.215 + W * 0.022], fill=DARK)
    _poly(d, [(0.46, 0.25), (0.54, 0.25), (0.50, 0.34)], beak)   # beak (points down)
    save(img, "penguin.png")


def whale():
    # ORCA (killer whale): black body, white belly + eye patch, tall dorsal fin (file
    # kept as whale.png).
    img = canvas()
    d = ImageDraw.Draw(img)
    black, white = (40, 44, 56, 255), (246, 248, 250, 255)
    _poly(d, [(0.74, 0.46), (0.99, 0.30), (0.93, 0.48), (0.99, 0.64)], black)  # fluke
    _poly(d, [(0.42, 0.30), (0.52, 0.10), (0.62, 0.32)], black)              # tall dorsal fin
    _ell(d, 0.04, 0.30, 0.80, 0.68, black)                                   # body
    _ell(d, 0.12, 0.55, 0.66, 0.665, white)                                  # white belly
    d.ellipse([W * 0.15, W * 0.37, W * 0.28, W * 0.46], fill=white)          # white eye patch
    _cute_eye(d, 0.205, 0.455, 0.045)
    _smile(d, 0.06, 0.49, 0.24, 0.58)
    save(img, "whale.png")


def seal():
    img = canvas()
    d = ImageDraw.Draw(img)
    c, belly = (196, 178, 158, 255), (222, 208, 192, 255)
    _poly(d, [(0.76, 0.50), (0.99, 0.40), (0.95, 0.52), (0.99, 0.66)], c)   # tail flippers
    _ell(d, 0.18, 0.40, 0.82, 0.84, c)                                       # body
    _ell(d, 0.05, 0.22, 0.43, 0.58, c)                                       # head
    _ell(d, 0.30, 0.60, 0.72, 0.82, belly)                                  # belly
    _poly(d, [(0.46, 0.78), (0.42, 0.95), (0.60, 0.82)], c)                 # fore flipper
    _cute_eye(d, 0.16, 0.36, 0.046)
    _cute_eye(d, 0.31, 0.37, 0.046)
    _smile(d, 0.17, 0.44, 0.31, 0.50)
    # nose + whiskers
    d.ellipse([W * 0.215, W * 0.405, W * 0.265, W * 0.45], fill=DARK)
    wc, ww = (120, 104, 88, 255), max(1, int(W * 0.006))
    for dx, dy in ((-0.15, -0.02), (-0.16, 0.02), (-0.14, 0.06)):
        d.line([(W * 0.235, W * 0.435), (W * (0.235 + dx), W * (0.435 + dy))], fill=wc, width=ww)
    for dx, dy in ((0.15, -0.02), (0.16, 0.02), (0.14, 0.06)):
        d.line([(W * 0.235, W * 0.435), (W * (0.235 + dx), W * (0.435 + dy))], fill=wc, width=ww)
    save(img, "seal.png")


def human():
    # DIVER: a clear human snorkeler, FRONT view -- skin face under a wetsuit hood and
    # a dive mask (light lens), dark wetsuit body + arms, flippers, snorkel.
    img = canvas()
    d = ImageDraw.Draw(img)
    suit, skin, lens, frame, fin, snork, cheek, bub = (40, 44, 54, 255), (246, 202, 166, 255), (188, 224, 238, 255), (20, 22, 30, 255), (28, 31, 40, 255), (238, 112, 92, 255), (242, 158, 158, 255), (206, 232, 242, 205)
    for (bx, by, br) in ((0.85, 0.17, 0.03), (0.91, 0.27, 0.022), (0.80, 0.29, 0.017)):  # rising bubbles
        d.ellipse([W * bx - W * br, W * by - W * br, W * bx + W * br, W * by + W * br], fill=bub)
    _poly(d, [(0.34, 0.82), (0.26, 0.99), (0.44, 0.99), (0.48, 0.86)], fin)   # left swim fin
    _poly(d, [(0.66, 0.82), (0.74, 0.99), (0.56, 0.99), (0.52, 0.86)], fin)   # right swim fin
    aw = max(4, int(W * 0.05))                          # bent arms (shoulder -> elbow -> hand)
    d.line([(W * 0.33, W * 0.55), (W * 0.15, W * 0.62), (W * 0.22, W * 0.74)], fill=suit, width=aw, joint="curve")
    d.line([(W * 0.67, W * 0.55), (W * 0.85, W * 0.62), (W * 0.78, W * 0.74)], fill=suit, width=aw, joint="curve")
    d.ellipse([W * 0.22 - W * 0.042, W * 0.74 - W * 0.042, W * 0.22 + W * 0.042, W * 0.74 + W * 0.042], fill=skin)  # left hand
    d.ellipse([W * 0.78 - W * 0.042, W * 0.74 - W * 0.042, W * 0.78 + W * 0.042, W * 0.74 + W * 0.042], fill=skin)  # right hand
    _ell(d, 0.26, 0.46, 0.74, 0.92, suit)               # rounded wetsuit body
    _ell(d, 0.33, 0.10, 0.67, 0.50, skin)               # head (skin)
    d.chord([W * 0.32, W * 0.07, W * 0.68, W * 0.48], start=180, end=360, fill=suit)  # wetsuit hood
    d.ellipse([W * 0.355, W * 0.40, W * 0.415, W * 0.45], fill=cheek)   # rosy cheeks
    d.ellipse([W * 0.585, W * 0.40, W * 0.645, W * 0.45], fill=cheek)
    d.line([(W * 0.34, W * 0.43), (W * 0.295, W * 0.16), (W * 0.37, W * 0.11)], fill=snork, width=max(3, int(W * 0.022)), joint="curve")  # bent snorkel
    d.ellipse([W * 0.31, W * 0.41, W * 0.375, W * 0.47], fill=snork)    # mouthpiece
    d.rounded_rectangle([W * 0.36, W * 0.22, W * 0.64, W * 0.39], radius=W * 0.06, fill=lens, outline=frame, width=max(2, int(W * 0.014)))  # dive mask
    _cute_eye(d, 0.45, 0.305, 0.045)                    # big cute eyes in the lens
    _cute_eye(d, 0.55, 0.305, 0.045)
    _smile(d, 0.45, 0.41, 0.55, 0.47)
    save(img, "human.png")


def moray():
    # MORAY EEL: a long green sinuous body with a gaping jaw (upper + lower), small
    # teeth, a big prominent eye + nostril -- facing left.
    img = canvas()
    d = ImageDraw.Draw(img)
    c, c2, spot, mouth, tooth, iris = (118, 146, 86, 255), (102, 130, 74, 255), (92, 118, 64, 255), (86, 44, 52, 255), (238, 236, 222, 255), (150, 192, 202, 255)
    n = 80
    for i in range(n):                                  # wavy body, tail to the right
        t = i / (n - 1)
        x = 0.24 + 0.72 * t
        y = 0.50 + 0.13 * math.sin(t * 2.3 * math.pi)
        r = (0.11 * (1.0 - 0.68 * t)) * W
        d.ellipse([W * x - r, W * y - r, W * x + r, W * y + r], fill=c)
    for (sx, sy, sr) in ((0.44, 0.46, 0.038), (0.58, 0.55, 0.032), (0.72, 0.48, 0.028)):
        d.ellipse([W * sx - W * sr, W * sy - W * sr, W * sx + W * sr, W * sy + W * sr], fill=spot)  # spots
    _poly(d, [(0.02, 0.47), (0.34, 0.42), (0.36, 0.52), (0.06, 0.60)], mouth)        # open mouth cavity
    _poly(d, [(0.02, 0.46), (0.10, 0.33), (0.40, 0.31), (0.48, 0.52), (0.34, 0.47), (0.10, 0.47)], c)  # upper jaw + head
    _poly(d, [(0.06, 0.58), (0.36, 0.53), (0.46, 0.62), (0.14, 0.66)], c2)           # lower jaw (dropped)
    for tx in (0.10, 0.155, 0.21, 0.265):               # upper teeth
        d.polygon([(W * tx, W * 0.475), (W * (tx + 0.022), W * 0.475), (W * (tx + 0.011), W * 0.515)], fill=tooth)
    ex, ey = 0.30, 0.40                                 # big prominent eye
    d.ellipse([W * ex - W * 0.052, W * ey - W * 0.052, W * ex + W * 0.052, W * ey + W * 0.052], fill=iris)
    d.ellipse([W * ex - W * 0.03, W * ey - W * 0.03, W * ex + W * 0.03, W * ey + W * 0.03], fill=DARK)
    d.ellipse([W * ex - W * 0.02, W * ey - W * 0.02, W * ex - W * 0.004, W * ey - W * 0.004], fill=(255, 255, 255, 235))
    d.ellipse([W * 0.05, W * 0.42, W * 0.078, W * 0.448], fill=(72, 92, 54, 255))    # nostril
    save(img, "moray.png")


def spermwhale():
    # SPERM WHALE: a huge blocky/square head (left) and a body tapering to the fluke.
    img = canvas()
    d = ImageDraw.Draw(img)
    c, belly = (104, 112, 128, 255), (150, 158, 172, 255)
    _poly(d, [(0.72, 0.48), (0.99, 0.34), (0.94, 0.50), (0.99, 0.66)], c)   # fluke
    _poly(d, [(0.06, 0.30), (0.58, 0.30), (0.86, 0.42), (0.86, 0.58), (0.58, 0.70), (0.06, 0.70)], c)  # blocky head+body
    _ell(d, 0.03, 0.30, 0.28, 0.70, c)                  # rounded front of the big head
    _ell(d, 0.20, 0.54, 0.72, 0.685, belly)             # belly
    d.line([(W * 0.06, W * 0.62), (W * 0.42, W * 0.64)], fill=(74, 80, 94, 255), width=max(2, int(W * 0.012)))  # jaw
    _cute_eye(d, 0.32, 0.45, 0.045)
    save(img, "spermwhale.png")


def dolphin():
    # BOTTLENOSE DOLPHIN: sleek grey body, a rostrum (beak), a swept-back dorsal fin,
    # lighter belly, and the classic dolphin smile.
    img = canvas()
    d = ImageDraw.Draw(img)
    c, belly = (124, 150, 176, 255), (208, 222, 234, 255)
    _poly(d, [(0.78, 0.46), (0.99, 0.32), (0.94, 0.48), (0.99, 0.62)], c)   # fluke
    _poly(d, [(0.46, 0.32), (0.60, 0.12), (0.66, 0.34)], c)                # swept dorsal fin
    _ell(d, 0.12, 0.34, 0.84, 0.66, c)                  # sleek body
    _poly(d, [(0.02, 0.50), (0.18, 0.43), (0.18, 0.55)], c)                # rostrum (beak)
    _ell(d, 0.18, 0.50, 0.62, 0.64, belly)              # belly
    _cute_eye(d, 0.26, 0.43, 0.042)
    _smile(d, 0.04, 0.48, 0.24, 0.57)                   # dolphin smile
    save(img, "dolphin.png")


def ocean():
    # Ocean-gradient floor: the same deep-to-light blue as the key-art background.
    # Drawn as a plain opaque vertical gradient (no supersampling needed); the arena
    # uses it as ONE big stretched tile so it reads as a single smooth gradient.
    H = 512
    img = Image.new("RGBA", (H, H), (0, 0, 0, 255))
    d = ImageDraw.Draw(img)
    top, bot = (16, 48, 86), (46, 120, 162)
    for y in range(H):
        t = y / (H - 1)
        d.line([(0, y), (H, y)], fill=(int(top[0] + (bot[0] - top[0]) * t),
                                       int(top[1] + (bot[1] - top[1]) * t),
                                       int(top[2] + (bot[2] - top[2]) * t), 255))
    img.save(os.path.join(HERE, "ocean.png"))
    print("wrote ocean.png")


if __name__ == "__main__":
    ocean()
    heart()
    octopus()
    target()
    fish()        # grouper
    bird()        # albatross
    whale()       # orca
    human()       # diver
    penguin()
    moray()
    spermwhale()
    dolphin()
    seal()        # back in the cast by request
    print("done")
