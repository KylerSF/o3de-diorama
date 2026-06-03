#!/usr/bin/env python3
"""Render the "Obi and the haters" key-art poster as a TRUE vector SVG.

All the sprite art in _gen_textures.py is built from primitives (ellipse, polygon,
line, arc, chord, rounded_rectangle). This script imports those same drawing
functions but feeds them an SVG-emitting drawer instead of PIL, so each creature
becomes real vector shapes. It then composes the poster (ocean gradient + bubbles +
Obi centred with the eight predators ringed around him, facing inward) and writes
keyart/obi-and-the-haters.svg.
"""
import importlib.util
import math
import os
import types

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", "..", ".."))   # o3de-diorama
OUT = os.path.join(REPO, "keyart", "obi-and-the-haters.svg")

# import _gen_textures without running its __main__ (so it doesn't regenerate PNGs)
_spec = importlib.util.spec_from_file_location("gentex", os.path.join(HERE, "_gen_textures.py"))
gt = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(gt)
W = gt.W   # native sprite coordinate space (1024)


def _col(c):
    r, g, b = c[0], c[1], c[2]
    a = (c[3] / 255.0) if len(c) == 4 else 1.0
    return "rgb(%d,%d,%d)" % (r, g, b), a


class SvgCanvas:
    """A drop-in for PIL's ImageDraw with the methods the art uses, emitting SVG."""
    def __init__(self):
        self.parts = []

    def _fill(self, fill):
        f, a = _col(fill)
        return 'fill="%s"%s' % (f, "" if a >= 0.999 else ' fill-opacity="%.3f"' % a)

    def ellipse(self, box, fill=None):
        x0, y0, x1, y1 = box
        self.parts.append('<ellipse cx="%.2f" cy="%.2f" rx="%.2f" ry="%.2f" %s/>'
                          % ((x0 + x1) / 2, (y0 + y1) / 2, (x1 - x0) / 2, (y1 - y0) / 2, self._fill(fill)))

    def polygon(self, pts, fill=None):
        p = " ".join("%.2f,%.2f" % (x, y) for x, y in pts)
        self.parts.append('<polygon points="%s" %s/>' % (p, self._fill(fill)))

    def line(self, pts, fill=None, width=1, joint=None):
        p = " ".join("%.2f,%.2f" % (x, y) for x, y in pts)
        f, a = _col(fill)
        op = "" if a >= 0.999 else ' stroke-opacity="%.3f"' % a
        self.parts.append('<polyline points="%s" fill="none" stroke="%s" stroke-width="%g" '
                          'stroke-linejoin="round" stroke-linecap="round"%s/>' % (p, f, width, op))

    def _arc_pts(self, box, start, end):
        x0, y0, x1, y1 = box
        cx, cy, rx, ry = (x0 + x1) / 2, (y0 + y1) / 2, (x1 - x0) / 2, (y1 - y0) / 2
        n = max(6, int(abs(end - start) / 5) + 1)
        return [(cx + rx * math.cos(math.radians(start + (end - start) * i / n)),
                 cy + ry * math.sin(math.radians(start + (end - start) * i / n))) for i in range(n + 1)]

    def arc(self, box, start=0, end=0, fill=None, width=1):
        self.line(self._arc_pts(box, start, end), fill=fill, width=width)

    def chord(self, box, start=0, end=0, fill=None):
        self.polygon(self._arc_pts(box, start, end), fill=fill)

    def rounded_rectangle(self, box, radius=0, fill=None, outline=None, width=1):
        x0, y0, x1, y1 = box
        s = ('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" rx="%.2f" %s'
             % (x0, y0, x1 - x0, y1 - y0, radius, self._fill(fill)))
        if outline is not None:
            of, _ = _col(outline)
            s += ' stroke="%s" stroke-width="%g"' % (of, width)
        self.parts.append(s + "/>")

    def body(self):
        return "\n".join(self.parts)


# Monkeypatch the PIL touch-points so the existing drawing functions render to SVG.
_frags = {}
_cur = {}


def _fake_draw(_img):
    c = SvgCanvas()
    _cur["c"] = c
    return c


def _fake_save(_img, name):
    _frags[name] = _cur["c"].body()


gt.canvas = lambda: None
gt.ImageDraw = types.SimpleNamespace(Draw=_fake_draw)
gt.save = _fake_save

for _fn in ("octopus", "spermwhale", "whale", "dolphin", "human", "moray", "fish", "bird", "penguin"):
    getattr(gt, _fn)()


def frag(tex):
    return _frags[tex + ".png"]


def main():
    P = 1100                      # poster size
    cx = cy = P / 2.0
    R = 360.0                     # ring radius
    HS = 42.0                     # haters scale (px per size-unit)
    OBI = 300.0                   # Obi display size (px)
    haters = [("spermwhale", 6.2), ("whale", 5.2), ("dolphin", 4.0), ("human", 3.4),
              ("moray", 3.1), ("fish", 3.0), ("bird", 2.7), ("penguin", 2.6)]

    out = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%d" height="%d">' % (P, P, P, P)]
    out.append('<defs><linearGradient id="ocean" x1="0" y1="0" x2="0" y2="1">'
               '<stop offset="0" stop-color="rgb(16,48,86)"/>'
               '<stop offset="1" stop-color="rgb(46,120,162)"/></linearGradient></defs>')
    out.append('<rect x="0" y="0" width="%d" height="%d" fill="url(#ocean)"/>' % (P, P))
    for (bx, by, br) in [(140,180,16),(300,90,10),(900,160,18),(980,360,11),(180,520,13),
                         (240,860,15),(560,940,10),(860,820,17),(70,720,9),(1010,640,12),(440,70,8),(760,120,12)]:
        out.append('<circle cx="%d" cy="%d" r="%d" fill="rgb(210,235,245)" fill-opacity="0.23"/>' % (bx, by, br))

    def place(tex, s, px, py, flip):
        sc = s / W
        if flip:
            tf = 'translate(%.2f,%.2f) scale(%.5f,%.5f)' % (px + s / 2, py - s / 2, -sc, sc)
        else:
            tf = 'translate(%.2f,%.2f) scale(%.5f)' % (px - s / 2, py - s / 2, sc)
        return '<g transform="%s">%s</g>' % (tf, frag(tex))

    n = len(haters)
    for i, (tex, sz) in enumerate(haters):
        a = -math.pi / 2 + 2 * math.pi * i / n
        s = sz * HS
        px, py = cx + R * math.cos(a), cy + R * math.sin(a)
        out.append(place(tex, s, px, py, math.cos(a) < 0))     # left side -> flip to face centre
    out.append(place("octopus", OBI, cx, cy, False))            # Obi centrepiece

    out.append('</svg>')
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w") as f:
        f.write("\n".join(out) + "\n")
    print("wrote", OUT, "(%d bytes)" % os.path.getsize(OUT))


if __name__ == "__main__":
    main()
