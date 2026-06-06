#!/usr/bin/env python3
"""
Layered, normal-mapped cartoon terrain + glowing decor for the solar-system
diorama, so the foreground stops reading as a flat green hill.

Outputs to the gem's Assets/Diorama/Textures:
  terrain_fore.png / terrain_fore_n.png   foreground convex ground + normal map
  terrain_far.png  / terrain_far_n.png    hazy distant ridge (atmospheric depth)
  cartoon_mushroom.png  cartoon_grass.png  cartoon_pebble.png   decor

The normal maps are derived from each layer's heightfield (Sobel gradient ->
tangent-space normal, +Z toward camera) so a directional "sun" 2D Light rakes
across the surface and the flat art reads as 3D.
"""
import os, math, random
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageChops

OUT = os.path.expanduser("~/PROJECTS/o3de-diorama/Assets/Diorama/Textures")
os.makedirs(OUT, exist_ok=True)


def fractal(h, w, octaves, freq, seed, persist=0.55):
    """Value-noise in [0,1] by summing bilinearly-upsampled random grids."""
    rng = np.random.RandomState(seed)
    acc = np.zeros((h, w)); amp = 1.0; tot = 0.0; f = freq
    for _ in range(octaves):
        gh, gw = max(2, int(h * f)), max(2, int(w * f))
        g = Image.fromarray((rng.rand(gh, gw) * 255).astype("uint8")).resize((w, h), Image.BILINEAR)
        acc += amp * np.asarray(g, float) / 255.0
        tot += amp; amp *= persist; f *= 2
    return acc / tot


def normal_from_height(height, mask, strength):
    """Tangent-space normal map (RGBA) from a heightfield. +Z faces the camera.
    Image +y is down, so flip dy to keep 'up' lighting correct."""
    gy, gx = np.gradient(height.astype(float))
    nx = -gx * strength
    ny = gy * strength            # flip: texture-down -> world-up
    nz = np.ones_like(height)
    inv = 1.0 / np.sqrt(nx * nx + ny * ny + nz * nz)
    nx, ny, nz = nx * inv, ny * inv, nz * inv
    rgb = np.dstack([(nx * 0.5 + 0.5), (ny * 0.5 + 0.5), (nz * 0.5 + 0.5)]) * 255
    # flat (0,0,1) outside the mask so unlit margins stay neutral
    flat = np.array([128, 128, 255], float)
    a = mask[..., None]
    rgb = rgb * a + flat * (1 - a)
    return Image.fromarray(np.dstack([rgb, mask * 255]).astype("uint8"), "RGBA")


def convex_mask(W, H, horizon_frac, radius_mul):
    """Lower convex disk (a planet's curved surface) + its (x,y) grids."""
    horizon = int(H * horizon_frac)
    R = int(W * radius_mul); cx = W // 2; cy = horizon + R
    ys, xs = np.mgrid[0:H, 0:W]
    disk = ((xs - cx) ** 2 + (ys - cy) ** 2 <= R * R).astype(float)
    return disk, horizon, R, cx, cy, ys, xs


def foreground(W=2048, H=1024, seed=21):
    disk, horizon, R, cx, cy, ys, xs = convex_mask(W, H, 0.30, 1.5)
    # height = gentle dome (curving away near the top) + multi-scale hills/bumps
    dome = -((xs - cx) ** 2 + (ys - cy) ** 2) / float(R * R)
    hills = fractal(H, W, 5, 0.010, seed)
    bumps = fractal(H, W, 4, 0.045, seed + 1)
    # height drives the normal map; weight the rolling hills + bumps strongly so a
    # raking sun actually shapes the surface (the dome is kept subtle).
    height = 0.3 * dome + 1.4 * hills + 0.7 * bumps
    # ---- colour: grass varied by noise + height, dirt in the hollows, haze far ----
    depth = np.clip((ys - horizon) / (H - horizon), 0, 1)      # 0 at horizon .. 1 near
    grass_lo = np.array([84, 146, 80]); grass_hi = np.array([150, 204, 120])
    dirt = np.array([138, 110, 74])
    t = np.clip(hills * 1.3 - 0.15, 0, 1)[..., None]
    col = grass_lo * (1 - t) + grass_hi * t
    # sparse, soft dirt patches where a second noise dips low
    patch = fractal(H, W, 3, 0.02, seed + 2)
    dmask = np.clip((0.34 - patch) * 4, 0, 1)[..., None] * 0.7
    col = col * (1 - dmask) + dirt * dmask
    # a meandering lighter path
    path_x = cx + (W * 0.22) * np.sin(ys / H * 3.1 + 0.6)
    pd = np.abs(xs - path_x) / (W * 0.045)
    pmask = (np.clip(1 - pd, 0, 1) ** 2)[..., None] * (depth[..., None] > 0.15)
    col = col * (1 - 0.7 * pmask) + np.array([150, 132, 98]) * 0.7 * pmask
    # near-ground brighter, far hazier toward a cool horizon haze
    col = col * (0.78 + 0.30 * depth[..., None])
    haze = np.array([120, 150, 150])
    hz = (np.clip(1 - depth * 4, 0, 1) ** 2)[..., None]
    col = col * (1 - 0.5 * hz) + haze * 0.5 * hz
    rgba = np.zeros((H, W, 4)); rgba[..., :3] = np.clip(col, 0, 255); rgba[..., 3] = disk * 255
    img = Image.fromarray(rgba.astype("uint8"), "RGBA")
    # crisp rim light along the horizon edge
    rim = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    ImageDraw.Draw(rim).ellipse([cx - R, cy - R - 4, cx + R, cy + R - 4], outline=(170, 240, 205, 255), width=8)
    img.alpha_composite(rim.filter(ImageFilter.GaussianBlur(6)))
    img.save(f"{OUT}/terrain_fore.png"); print("terrain_fore", img.size)
    normal_from_height(height, disk, strength=2600.0 / W).save(f"{OUT}/terrain_fore_n.png")
    print("terrain_fore_n")


def far_ridge(W=2048, H=512, seed=31):
    # a low, hazy ridgeline: midpoint-ish silhouette via summed sines + noise
    rng = np.random.RandomState(seed)
    base = H * 0.45
    prof = base + (fractal(H, W, 4, 0.012, seed) - 0.5) * H * 0.5
    prof = np.clip(prof[0] if prof.ndim == 2 else prof.mean(0), H * 0.2, H * 0.9)
    xs = np.arange(W)
    profile = (H * 0.55 + H * 0.18 * np.sin(xs / W * 6.0) + H * 0.10 * np.sin(xs / W * 17.0 + 1.0)
               + (np.asarray(Image.fromarray((rng.rand(1, W // 8) * 255).astype("uint8")).resize((W, 1), Image.BILINEAR), float)[0] - 128) / 128 * H * 0.06)
    ys, xg = np.mgrid[0:H, 0:W]
    mask = (ys >= profile[None, :]).astype(float)
    # hazy bluish ridge, lighter toward its top edge
    top = np.array([96, 120, 138]); bot = np.array([60, 82, 104])
    g = np.clip((ys - profile[None, :]) / (H * 0.5), 0, 1)[..., None]
    col = top * (1 - g) + bot * g
    height = (1 - g[..., 0]) * mask + fractal(H, W, 3, 0.03, seed + 5) * 0.25 * mask
    rgba = np.zeros((H, W, 4)); rgba[..., :3] = np.clip(col, 0, 255); rgba[..., 3] = mask * 210
    Image.fromarray(rgba.astype("uint8"), "RGBA").save(f"{OUT}/terrain_far.png"); print("terrain_far", (W, H))
    normal_from_height(height, mask, strength=1400.0 / W).save(f"{OUT}/terrain_far_n.png"); print("terrain_far_n")


def _one_mushroom(s, body, bodydk, hi, gillc, gilldk, stemc):
    """One smooth glowing toadstool on its own tile: rounded dome cap, visible
    radial gills, a thick tapered stem with a bulbous foot. A dark rim outline
    keeps it distinct when clustered. Returns (tile, base_x, base_y)."""
    TW, TH = int(2.5 * s), int(3.0 * s)
    m = Image.new("RGBA", (TW, TH), (0, 0, 0, 0)); md = ImageDraw.Draw(m)
    lx, lby = TW / 2.0, TH * 0.95
    # WIDE, shallow umbrella cap (widest at the rim) that overhangs a thinner stem
    # -> reads as a mushroom, not a muffin.
    capw, caph = s * 1.14, s * 0.46
    stemh, stemw = s * 0.92, s * 0.145
    st = lby - stemh; rim_y = st + s * 0.04
    # stem (tapered) + bulbous foot
    md.polygon([(lx - stemw, lby), (lx + stemw, lby), (lx + stemw * 0.72, st), (lx - stemw * 0.72, st)], fill=stemc + (255,))
    md.ellipse([lx - stemw * 1.4, lby - s * 0.12, lx + stemw * 1.4, lby + s * 0.05], fill=stemc + (255,))
    # gills: a band narrower than the cap (so the cap overhangs it) + radial lines
    gw = capw * 0.34
    md.ellipse([lx - gw, rim_y - s * 0.02, lx + gw, rim_y + s * 0.16], fill=gillc + (255,))
    for k in range(13):
        gx = lx + (k / 12.0 - 0.5) * gw * 1.9
        md.line([(lx, rim_y - s * 0.04), (gx, rim_y + s * 0.14)], fill=gilldk + (235,), width=max(2, int(s * 0.016)))

    # cap = top half of a wide ellipse (widest at the rim) + a rounded overhang lip;
    # an expanded dark copy underneath gives a clean separating outline.
    def cap_shapes(draw, exp, col):
        draw.pieslice([lx - capw / 2 - exp, rim_y - caph - exp, lx + capw / 2 + exp, rim_y + caph + exp], 180, 360, fill=col)
        draw.ellipse([lx - capw / 2 - s * 0.02 - exp, rim_y - s * 0.10 - exp, lx + capw / 2 + s * 0.02 + exp, rim_y + s * 0.06 + exp], fill=col)
    cap_shapes(md, s * 0.022, bodydk + (255,))     # outline ring
    cap_shapes(md, 0.0, body + (255,))             # cap body
    # lower-cap shading (masked to the cap so the blur leaves no halo) -> convex belly
    sh = Image.new("RGBA", (TW, TH), (0, 0, 0, 0))
    ImageDraw.Draw(sh).ellipse([lx - capw / 2, rim_y - caph * 0.18, lx + capw / 2, rim_y + s * 0.06], fill=bodydk + (150,))
    sh = sh.filter(ImageFilter.GaussianBlur(s * 0.045)); sh.putalpha(ImageChops.multiply(sh.split()[3], m.split()[3])); m.alpha_composite(sh)
    # soft top sheen (masked)
    sn = Image.new("RGBA", (TW, TH), (0, 0, 0, 0))
    ImageDraw.Draw(sn).ellipse([lx - capw * 0.26, rim_y - caph * 0.92, lx + capw * 0.02, rim_y - caph * 0.42], fill=hi + (175,))
    sn = sn.filter(ImageFilter.GaussianBlur(s * 0.035)); sn.putalpha(ImageChops.multiply(sn.split()[3], m.split()[3])); m.alpha_composite(sn)
    return m, lx, lby


def mushroom(size=1200, seed=41):
    # A cluster of 7 distinct glowing toadstools (smooth domes + radial gills +
    # thick stems), arranged back-to-front so they overlap cleanly without mushing.
    SS = 2; W, H = size * SS, int(size * 0.6) * SS
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    body, bodydk, hi = (247, 244, 234), (203, 197, 180), (255, 254, 248)
    gillc, gilldk, stemc = (234, 230, 216), (192, 186, 168), (240, 237, 226)
    base = H * 0.40
    # (cx_frac, base_y_frac, size_px); drawn back (small y) first for clean overlap
    items = [
        (0.15, 0.93, base * 1.06), (0.29, 0.99, base * 0.64), (0.42, 0.77, base * 0.82),
        (0.53, 0.71, base * 1.06), (0.63, 0.79, base * 0.60), (0.76, 0.86, base * 0.92),
        (0.88, 0.94, base * 1.02),
    ]
    for fx, fy, s in sorted(items, key=lambda t: t[1]):
        tile, lx, lby = _one_mushroom(s, body, bodydk, hi, gillc, gilldk, stemc)
        img.alpha_composite(tile, (int(fx * W - lx), int(fy * H - lby)))
    img = img.resize((size, int(size * 0.6)), Image.LANCZOS)
    img = img.crop(img.getbbox()); img.save(f"{OUT}/cartoon_mushroom.png"); print("cartoon_mushroom", img.size)


def oak(size=460, seed=3):
    # a characterful single oak: a thick trunk with crooked, tapering limbs and a
    # broad lumpy canopy. Supersampled for smooth edges.
    SS = 4; W = H = size * SS; rng = random.Random(seed)
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0)); d = ImageDraw.Draw(img)
    bark, barkdk = (104, 72, 46), (74, 50, 32)
    tips = []

    def limb(x, y, ang, length, width, depth):
        # crooked: walk the limb in short segments with angle jitter so it zig-zags,
        # but clamp every segment to the UPPER hemisphere so a limb never droops to
        # the ground (the tree should sweep up and out, not melt into the grass).
        segs = 4
        for i in range(segs):
            ang += rng.uniform(-0.18, 0.18)                       # the crook
            ang = min(max(ang, 0.28), math.pi - 0.28)            # stay above horizontal (no droop) but allow spread
            seg = length / segs
            x2 = x + seg * math.cos(ang); y2 = y - seg * math.sin(ang)
            w = max(2.0, width * (1 - 0.16 * i))
            d.line([(x, y), (x2, y2)], fill=bark + (255,), width=int(w))
            d.ellipse([x2 - w / 2, y2 - w / 2, x2 + w / 2, y2 + w / 2], fill=bark + (255,))   # smooth joint
            x, y = x2, y2
        if depth == 0 or length < W * 0.05:
            tips.append((x, y))
            return
        nb = rng.randint(2, 3) + (1 if depth >= 3 else 0)   # extra limbs up high -> fuller
        for _ in range(nb):
            da = rng.uniform(0.45, 1.05) * rng.choice([-1, 1])   # wider forks -> spreading, visible limbs
            limb(x, y, ang + da, length * rng.uniform(0.62, 0.78), width * 0.66, depth - 1)

    limb(W * 0.5 + rng.uniform(-W * 0.03, W * 0.03), H * 0.95,
         math.pi / 2 + rng.uniform(-0.08, 0.08), H * 0.28, W * 0.122, 4)   # thicker trunk, forks lower
    # broad lumpy canopy: a cluster centred ON each (upper) tip so the leaves cover
    # the branch ends -- no bare twigs poking out.
    canopy = Image.new("RGBA", (W, H), (0, 0, 0, 0)); cd = ImageDraw.Draw(canopy)
    greens = [(66, 124, 58), (92, 158, 78), (52, 104, 50)]
    M = W * 0.13                        # keep every blob fully inside -> no hard edge clip
    clamp = lambda v, hi: min(max(v, M), hi - M)
    for (x, y) in tips:
        for k in range(8):
            if k == 0:
                ox, oy, rr = 0.0, 0.0, W * 0.085          # covers the branch tip itself
            else:
                ox, oy = rng.uniform(-W * 0.07, W * 0.07), rng.uniform(-W * 0.07, W * 0.05)
                rr = rng.uniform(W * 0.05, W * 0.08)
            cx, cy = clamp(x + ox, W), clamp(y + oy, H)
            cd.ellipse([cx - rr, cy - rr, cx + rr, cy + rr], fill=rng.choice(greens) + (255,))
    canopy = canopy.filter(ImageFilter.GaussianBlur(W * 0.004))
    img.alpha_composite(canopy)
    # sunlit dabs on the crown
    hi = Image.new("RGBA", (W, H), (0, 0, 0, 0)); hd = ImageDraw.Draw(hi)
    for (x, y) in tips:
        if y < H * 0.45:
            cx, cy = clamp(x, W), clamp(y, H)
            hd.ellipse([cx - W * 0.05, cy - W * 0.06, cx + W * 0.04, cy + W * 0.02], fill=(150, 200, 110, 150))
    img.alpha_composite(hi.filter(ImageFilter.GaussianBlur(W * 0.01)))
    img = img.resize((size, size), Image.LANCZOS)
    img = img.crop(img.getbbox()); img.save(f"{OUT}/cartoon_oak.png"); print("cartoon_oak", img.size)


def grass(size=256, seed=42):
    W = H = size; img = Image.new("RGBA", (W, H), (0, 0, 0, 0)); d = ImageDraw.Draw(img)
    rng = random.Random(seed)
    for _ in range(11):
        x = rng.uniform(W*0.2, W*0.8); h = rng.uniform(H*0.4, H*0.85); lean = rng.uniform(-W*0.12, W*0.12)
        col = rng.choice([(72, 140, 70), (96, 170, 84), (58, 120, 62)])
        d.polygon([(x-4, H*0.95), (x+4, H*0.95), (x+lean, H*0.95-h)], fill=col + (255,))
    img = img.crop(img.getbbox()); img.save(f"{OUT}/cartoon_grass.png"); print("cartoon_grass", img.size)


def pebble(size=192, seed=43):
    W = H = size; rng = random.Random(seed)
    pts = []
    for i in range(10):
        a = 2*math.pi*i/10; r = size*0.34*(1+rng.uniform(-0.22, 0.22))
        pts.append((W/2 + r*math.cos(a), H*0.6 + r*math.sin(a)*0.7))
    mask = Image.new("L", (W, H), 0); ImageDraw.Draw(mask).polygon(pts, fill=255)
    ys, xs = np.mgrid[0:H, 0:W]
    arr = np.zeros((H, W, 3)) + np.array([118, 116, 122], float)
    arr *= np.clip(1.08 - 0.5*(ys/H), 0.6, 1.1)[..., None]
    out = Image.fromarray(np.dstack([np.clip(arr, 0, 255), np.asarray(mask)]).astype("uint8"), "RGBA")
    out = out.crop(out.getbbox()); out.save(f"{OUT}/cartoon_pebble.png"); print("cartoon_pebble", out.size)


if __name__ == "__main__":
    random.seed(1)
    foreground(); far_ridge(); mushroom(); oak(); grass(); pebble()
