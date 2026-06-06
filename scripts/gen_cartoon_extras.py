#!/usr/bin/env python3
"""
Cartoon asteroid, comet, and starry-sky background to match gen_cartoon_planets,
for the solar-system diorama. Output cartoon_asteroid.png, cartoon_comet.png,
cartoon_sky.png to the gem's Assets/Diorama/Textures.
"""
import os, math, random
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

OUT = os.path.expanduser("~/PROJECTS/o3de-diorama/Assets/Diorama/Textures")
os.makedirs(OUT, exist_ok=True)
L = np.array([-0.45, -0.5, 0.74]); L /= np.linalg.norm(L)


def asteroid(size=300, seed=5):
    random.seed(seed); SS = 4; n = size * SS
    # lumpy closed blob (radius wobbles with angle)
    cx = cy = n / 2
    pts = []
    base = n * 0.36
    waves = [(random.uniform(0.06, 0.16), random.randint(2, 6), random.uniform(0, 6.28)) for _ in range(4)]
    for a in np.linspace(0, 2 * math.pi, 90, endpoint=False):
        r = base * (1 + sum(amp * math.sin(k * a + ph) for amp, k, ph in waves))
        pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    mask = Image.new("L", (n, n), 0); ImageDraw.Draw(mask).polygon(pts, fill=255)
    arr = np.zeros((n, n, 3)) + np.array([150, 142, 130], float)
    # craters
    ys, xs = np.mgrid[0:n, 0:n]
    for _ in range(10):
        ang = random.uniform(0, 6.28); rad = random.uniform(0, base * 0.7)
        ox, oy = cx + rad * math.cos(ang), cy + rad * math.sin(ang); cr = random.uniform(n * 0.02, n * 0.06)
        arr[(xs - ox) ** 2 + (ys - oy) ** 2 <= cr * cr] *= 0.8
    # gentle shading via a fake normal from distance-to-edge gradient
    x = (xs - cx) / base; y = (ys - cy) / base
    fac = np.clip(0.78 - 0.25 * (x * 0.6 + y * 0.6), 0.55, 1.05)
    arr = np.clip(arr * fac[..., None], 0, 255)
    out = Image.fromarray(np.dstack([arr, np.asarray(mask)]).astype("uint8"), "RGBA").resize((size, size), Image.LANCZOS)
    pad = size // 6; canvas = Image.new("RGBA", (size + 2 * pad, size + 2 * pad), (0, 0, 0, 0)); canvas.alpha_composite(out, (pad, pad))
    canvas.save(f"{OUT}/cartoon_asteroid.png"); print("cartoon_asteroid", canvas.size)


def comet(W=720, H=360):
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    nx, ny = int(W * 0.78), H // 2; nr = int(H * 0.16)
    # tail: glowing cone fading to the left
    tail = Image.new("RGBA", (W, H), (0, 0, 0, 0)); d = ImageDraw.Draw(tail)
    for i in range(nx):
        t = i / nx                       # 0 at left .. 1 at nucleus
        half = int((H * 0.02) + (H * 0.30) * t)
        a = int(150 * (t ** 1.6))
        d.line([(i, ny - half), (i, ny + half)], fill=(120, 200, 255, a))
    tail = tail.filter(ImageFilter.GaussianBlur(6))
    img.alpha_composite(tail)
    # coma glow + icy nucleus
    glow = Image.new("RGBA", (W, H), (0, 0, 0, 0)); ImageDraw.Draw(glow).ellipse([nx - nr * 2, ny - nr * 2, nx + nr * 2, ny + nr * 2], fill=(150, 220, 255, 150))
    img.alpha_composite(glow.filter(ImageFilter.GaussianBlur(14)))
    nuc = Image.new("RGBA", (W, H), (0, 0, 0, 0)); ImageDraw.Draw(nuc).ellipse([nx - nr, ny - nr, nx + nr, ny + nr], fill=(225, 245, 255, 255))
    img.alpha_composite(nuc)
    img.save(f"{OUT}/cartoon_comet.png"); print("cartoon_comet", img.size)


def sky(W=1600, H=900, seed=11):
    rng = np.random.RandomState(seed)
    # vertical gradient deep indigo -> near black
    top = np.array([22, 16, 44]); bot = np.array([6, 5, 16])
    g = np.linspace(0, 1, H)[:, None, None]
    arr = (top * (1 - g) + bot * g) * np.ones((H, W, 1))
    img = Image.fromarray(arr.astype("uint8"), "RGB").convert("RGBA")
    # soft nebula clouds
    neb = Image.new("RGBA", (W, H), (0, 0, 0, 0)); d = ImageDraw.Draw(neb)
    for _ in range(6):
        cx, cy = rng.randint(0, W), rng.randint(0, H); r = rng.randint(120, 300)
        col = random.choice([(80, 50, 140), (40, 70, 150), (120, 50, 110)])
        d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=col + (40,))
    img.alpha_composite(neb.filter(ImageFilter.GaussianBlur(80)))
    # stars
    d = ImageDraw.Draw(img)
    for _ in range(420):
        x, y = rng.randint(0, W), rng.randint(0, H); s = int(rng.choice([1, 1, 1, 2, 2, 3])); b = int(rng.randint(150, 255))
        tint = random.choice([(b, b, b), (b, b, 255), (255, b, b)])
        d.ellipse([x, y, x + s, y + s], fill=tint + (255,))
    # a couple of cartoon galaxies (soft spiral hints)
    for _ in range(2):
        gx, gy = rng.randint(W // 5, 4 * W // 5), rng.randint(H // 5, 3 * H // 5)
        gimg = Image.new("RGBA", (W, H), (0, 0, 0, 0)); gd = ImageDraw.Draw(gimg)
        for r in range(60, 6, -6):
            gd.ellipse([gx - r, gy - r // 2, gx + r, gy + r // 2], fill=(200, 180, 230, 18))
        img.alpha_composite(gimg.filter(ImageFilter.GaussianBlur(6)))
    img.convert("RGB").save(f"{OUT}/cartoon_sky.png"); print("cartoon_sky", (W, H))


if __name__ == "__main__":
    import random as _r; _r.seed(1)
    asteroid(); comet(); sky()
