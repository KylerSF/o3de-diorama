#!/usr/bin/env python3
"""
Clean flat-design / cartoon versions of the solar-system bodies (Sun + planets +
Pluto + asteroid + comet) and a cartoon starry-sky background, for the diorama.
Stylized but based on the real planets' colours/features (Jupiter bands, Saturn
rings, Earth continents, Mars cap, Pluto's heart, etc.). Cohesive style so any
other generated set dressing matches. Output to the gem's Assets/Diorama/Textures
as cartoon_<name>.png.
"""
import os
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

OUT = os.path.expanduser("~/PROJECTS/o3de-diorama/Assets/Diorama/Textures")
os.makedirs(OUT, exist_ok=True)
SS = 4
L = np.array([-0.45, -0.5, 0.74]); L /= np.linalg.norm(L)


def grid(n):
    c = (np.arange(n) + 0.5) / n * 2 - 1
    return np.meshgrid(c, c)


def ball(size, surface_fn, glow=None, ring=None, emissive=False, outline=(0, 0, 0, 0)):
    n = size * SS
    x, y = grid(n)
    r2 = x * x + y * y; inside = r2 <= 1.0
    z = np.sqrt(np.clip(1 - r2, 0, 1))
    surf = surface_fn(x, y)
    if emissive:
        fac = np.ones(x.shape)
        spec = 0
    else:
        diff = np.clip(x * L[0] + y * L[1] + z * L[2], 0, 1)
        fac = 0.74 + 0.26 * diff          # gentle, cartoon
        spec = (np.clip(diff, 0, 1) ** 10) * 70
    rgb = np.clip(surf * fac[..., None] + (spec if np.isscalar(spec) else spec[..., None]), 0, 255)
    alpha = np.clip((1 - r2) * 7, 0, 1) * inside
    img = Image.fromarray(np.dstack([rgb, alpha * 255]).astype("uint8"), "RGBA").resize((size, size), Image.LANCZOS)
    pad = int(size * (0.5 if ring else 0.22))
    canvas = Image.new("RGBA", (size + 2 * pad, size + 2 * pad), (0, 0, 0, 0))
    if ring:
        _ring(canvas, img, ring, size, pad)
    else:
        canvas.alpha_composite(img, (pad, pad))
    if glow:
        a = canvas.split()[3]
        g = Image.composite(Image.new("RGBA", canvas.size, glow + (255,)), Image.new("RGBA", canvas.size, (0, 0, 0, 0)), a).filter(ImageFilter.GaussianBlur(size // 9))
        out = Image.new("RGBA", canvas.size, (0, 0, 0, 0)); out.alpha_composite(g); out.alpha_composite(canvas)
        # Feather the alpha radially to 0 so the blurred glow can NEVER leave a hard
        # square edge at the texture bound (the in-engine bloom supplies the soft
        # round halo). Disc + ring sit well inside r=0.82, so they're untouched.
        W2 = out.size[0]; yy, xx = np.mgrid[0:W2, 0:W2]; cc = (W2 - 1) / 2.0
        rr = np.sqrt((xx - cc) ** 2 + (yy - cc) ** 2) / cc
        fade = np.clip((1.0 - rr) / 0.18, 0.0, 1.0)
        arr = np.array(out); arr[..., 3] = (arr[..., 3] * fade).astype("uint8")
        canvas = Image.fromarray(arr, "RGBA")
    return canvas


def _ring(canvas, sphere, ring, size, pad):
    W = canvas.size[0]; cx = cy = W // 2
    rx, ry, th = int(ring["rx"] * size), int(ring["ry"] * size), max(4, int(ring["th"] * size))
    ringimg = Image.new("RGBA", (W, W), (0, 0, 0, 0)); d = ImageDraw.Draw(ringimg)
    for i in range(th):
        a = 0 if abs(i - th * 0.5) < th * 0.12 else 240         # gap
        col = ring["color"]
        d.ellipse([cx - rx, cy - ry + i, cx + rx, cy + ry - i], outline=col + (a,), width=2)
    canvas.alpha_composite(ringimg.crop((0, 0, W, cy)), (0, 0))
    canvas.alpha_composite(sphere, (pad, pad))
    canvas.alpha_composite(ringimg.crop((0, cy, W, W)), (0, cy))


def solid(rgb):
    return lambda x, y: np.ones(x.shape + (3,)) * np.array(rgb, float)


def bands(base, stripes):
    cols = [np.array(c, float) for c in stripes]
    def fn(x, y):
        out = np.ones(x.shape + (3,)) * np.array(base, float)
        idx = ((y * 0.5 + 0.5) * len(cols)).astype(int).clip(0, len(cols) - 1)
        for k, c in enumerate(cols):
            out[idx == k] = c
        return out
    return fn


def blobs(base, items):
    def fn(x, y):
        out = np.ones(x.shape + (3,)) * np.array(base, float)
        for (cx, cy, rx, ry, col) in items:
            m = ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1
            out[m] = col
        return out
    return fn


def earth(x, y):
    out = blobs([46, 110, 190], [(-0.25, -0.05, 0.4, 0.32, [80, 160, 90]),
                                 (0.32, 0.28, 0.34, 0.42, [90, 165, 95]),
                                 (0.15, -0.5, 0.28, 0.16, [110, 175, 105])])(x, y)
    out[np.abs(y) > 0.82] = [240, 245, 250]
    for (cx, cy, r) in [(0.45, -0.2, 0.18), (-0.4, 0.4, 0.2)]:
        out[(x - cx) ** 2 + (y - cy) ** 2 <= r * r] = [245, 248, 252]
    return out


def mars(x, y):
    out = blobs([200, 95, 55], [(-0.2, 0.1, 0.42, 0.26, [165, 70, 45]), (0.3, -0.25, 0.28, 0.2, [170, 75, 48])])(x, y)
    out[y > 0.82] = [240, 240, 245]; out[y < -0.85] = [240, 240, 245]
    return out


def pluto(x, y):
    out = np.ones(x.shape + (3,)) * np.array([205, 180, 150], float)
    out[((x - 0.05) / 0.4) ** 2 + ((y + 0.25) / 0.45) ** 2 <= 1] = [238, 225, 205]   # heart-ish bright region
    out[((x + 0.35) / 0.3) ** 2 + ((y - 0.2) / 0.4) ** 2 <= 1] = [150, 110, 88]
    return out


def moon(x, y):
    # light grey disc; +y is DOWN. Dark maria laid out as the "moon rabbit"
    # pounding mochi (ears up, body centred, mortar lower-right) + crater texture.
    out = np.ones(x.shape + (3,)) * np.array([198, 200, 208], float)
    # subtle surface mottle (low-freq noise upscaled)
    n = x.shape[0]
    rng = np.random.RandomState(7)
    small = (rng.rand(20, 20) * 255).astype("uint8")
    mottle = np.asarray(Image.fromarray(small).resize((n, n), Image.BILINEAR), float) / 255.0
    out *= (0.94 + 0.10 * mottle)[..., None]

    maria = np.array([128, 136, 156], float)

    def ellipse(cx, cy, rx, ry, ang):
        ca, sa = np.cos(ang), np.sin(ang)
        xr = (x - cx) * ca + (y - cy) * sa
        yr = -(x - cx) * sa + (y - cy) * ca
        return (xr / rx) ** 2 + (yr / ry) ** 2 <= 1.0

    rabbit = np.zeros(x.shape, bool)
    for (cx, cy, rx, ry, ang) in [
        (-0.20, -0.46, 0.12, 0.34, 0.26),   # left ear
        (0.08, -0.50, 0.11, 0.33, -0.18),   # right ear
        (-0.06, 0.02, 0.40, 0.38, 0.0),     # head/body
        (0.30, 0.18, 0.26, 0.30, 0.3),      # haunch
        (0.20, 0.40, 0.12, 0.14, 0.0),      # front paws reaching down
    ]:
        rabbit |= ellipse(cx, cy, rx, ry, ang)
    out[rabbit] = out[rabbit] * 0.35 + maria * 0.65
    # the mortar (usu) of rice cakes, a separate dark patch lower-right
    out[ellipse(0.46, 0.46, 0.17, 0.13, -0.2)] = maria * 0.85

    craters = [(-0.38, 0.50, 0.10), (0.40, -0.30, 0.07), (0.58, 0.10, 0.06),
               (-0.55, -0.10, 0.06), (-0.10, 0.62, 0.08), (0.62, -0.42, 0.05),
               (-0.50, 0.30, 0.05), (0.10, -0.66, 0.05), (-0.68, 0.18, 0.04),
               (0.30, 0.62, 0.05), (-0.30, -0.62, 0.045)]
    d2 = lambda cx, cy: (x - cx) ** 2 + (y - cy) ** 2
    for (cx, cy, r) in craters:
        ring = (d2(cx, cy) <= (r * 1.15) ** 2) & (d2(cx, cy) >= (r * 0.78) ** 2)
        out[ring] = np.clip(out[ring] * 1.18, 0, 255)        # bright rim
        out[d2(cx, cy) < (r * 0.78) ** 2] *= 0.84            # darker floor
    return out


def jupiter(x, y):
    out = bands([222, 192, 150], [[228, 206, 170], [185, 140, 100], [232, 212, 178],
                                  [175, 128, 92], [225, 200, 165], [195, 150, 110]])(x, y)
    out[((x - 0.22) / 0.16) ** 2 + ((y - 0.18) / 0.11) ** 2 <= 1] = [205, 95, 70]
    return out


def craters(base, spots):
    def fn(x, y):
        out = np.ones(x.shape + (3,)) * np.array(base, float)
        for (cx, cy, r) in spots:
            out[(x - cx) ** 2 + (y - cy) ** 2 <= r * r] = np.array(base, float) * 0.8
        return out
    return fn


PLANETS = [
    ("sun", solid([255, 200, 60]), 360, dict(glow=(255, 150, 30), emissive=True)),
    ("mercury", craters([150, 140, 128], [(-0.2, 0.15, 0.18), (0.25, -0.2, 0.14), (0.05, 0.4, 0.1), (-0.4, -0.3, 0.12)]), 240, {}),
    ("venus", bands([232, 205, 150], [[236, 212, 160], [222, 192, 138]]), 300, {}),
    ("earth", earth, 320, dict(glow=(70, 130, 210))),
    ("mars", mars, 280, {}),
    ("jupiter", jupiter, 380, {}),
    ("saturn", bands([226, 204, 156], [[232, 214, 170], [212, 188, 140]]), 320, dict(ring=dict(color=(214, 196, 150), rx=0.98, ry=0.32, th=0.16))),
    ("uranus", bands([165, 218, 224], [[176, 226, 230], [156, 210, 218]]), 300, dict(glow=(120, 205, 215))),
    ("neptune", bands([56, 96, 205], [[68, 110, 214], [46, 84, 192]]), 300, dict(glow=(50, 90, 205))),
    ("pluto", pluto, 220, {}),
    ("moon", moon, 300, dict(glow=(205, 210, 235))),
]

if __name__ == "__main__":
    for name, fn, size, kw in PLANETS:
        ball(size, fn, **kw).save(f"{OUT}/cartoon_{name}.png")
        print("cartoon_" + name)
