#!/usr/bin/env python3
"""
Generate real, spec-correct .aseprite test fixtures for the native importer.

Writes three small .aseprite files exercising the parser/compositor paths that the
RGBA happy-path does not:
  * indexed_test.aseprite    -- 8-bpp indexed (palette chunk + transparent index 0)
  * grayscale_test.aseprite  -- 16-bpp grayscale (value + alpha)
  * multiply_test.aseprite   -- 32-bpp RGBA, two layers, the top one Multiply

Cels are ZLIB-compressed (cel type 2), so the inflate path is exercised too. The
files follow the open .ase spec, so the Aseprite app can open them as well -- but the
point here is to feed the DioramaAsepriteBuilder through the AssetProcessor and view
the result, which the unit tests (raw in-memory cels, no AP, no render) do not cover.

Usage:
  python3 scripts/gen_aseprite_fixtures.py [output_dir]

Default output_dir is Assets/Diorama/Examples/Aseprite/ under this gem, which an
enabled-gem asset scan folder picks up, producing
  diorama/examples/aseprite/<name>.dioramasheet (+ .streamingimage)
that you assign to a Sprite's Aseprite Sheet field.
"""
import os
import struct
import sys
import zlib

W = 16
H = 16

FILE_MAGIC = 0xA5E0
FRAME_MAGIC = 0xF1FA
CHUNK_LAYER = 0x2004
CHUNK_CEL = 0x2005
CHUNK_TAGS = 0x2018
CHUNK_PALETTE = 0x2019


def u8(v):
    return struct.pack("<B", v & 0xFF)


def u16(v):
    return struct.pack("<H", v & 0xFFFF)


def s16(v):
    return struct.pack("<h", v)


def u32(v):
    return struct.pack("<I", v & 0xFFFFFFFF)


def ase_string(s):
    b = s.encode("utf-8")
    return u16(len(b)) + b


def chunk(ctype, body):
    return u32(len(body) + 6) + u16(ctype) + body


def layer_chunk(name, blend_mode=0, visible=True):
    body = (
        u16(1 if visible else 0)  # flags: visible
        + u16(0)  # type: normal
        + u16(0)  # child level
        + u16(0)  # default width (ignored)
        + u16(0)  # default height (ignored)
        + u16(blend_mode)
        + u8(255)  # opacity
        + b"\x00\x00\x00"  # reserved
        + ase_string(name)
    )
    return chunk(CHUNK_LAYER, body)


def cel_chunk(layer_index, raw_pixels):
    comp = zlib.compress(raw_pixels)
    body = (
        u16(layer_index)
        + s16(0)  # x
        + s16(0)  # y
        + u8(255)  # opacity
        + u16(2)  # cel type: compressed image
        + b"\x00" * 7  # z-index (s16) + reserved (5)
        + u16(W)
        + u16(H)
        + comp
    )
    return chunk(CHUNK_CEL, body)


def palette_chunk(entries):
    # entries: list of (r,g,b,a); index = position.
    body = u32(len(entries)) + u32(0) + u32(len(entries) - 1) + b"\x00" * 8
    for (r, g, b, a) in entries:
        body += u16(0) + u8(r) + u8(g) + u8(b) + u8(a)  # flags=0, RGBA
    return chunk(CHUNK_PALETTE, body)


def tags_chunk(name, frm, to):
    body = u16(1) + b"\x00" * 8
    body += u16(frm) + u16(to) + u8(0) + b"\x00" * 12 + ase_string(name)
    return chunk(CHUNK_TAGS, body)


def build_file(color_depth, transparent_index, chunks):
    header_after_depth = (
        u32(1)  # flags: layer opacity valid
        + u16(0)  # speed (deprecated)
        + u32(0)
        + u32(0)
        + u8(transparent_index)
        + b"\x00\x00\x00"  # ignore
        + u16(256)  # number of colors
        + u8(1)  # pixel width
        + u8(1)  # pixel height
        + s16(0)  # grid x
        + s16(0)  # grid y
        + u16(16)  # grid width
        + u16(16)  # grid height
        + b"\x00" * 84  # reserved
    )
    header_pre = u16(FILE_MAGIC) + u16(1) + u16(W) + u16(H) + u16(color_depth)
    header = header_pre + header_after_depth
    assert len(header) == 124, len(header)  # everything after the u32 file-size field

    body = b"".join(chunks)
    frame = (
        u16(FRAME_MAGIC)
        + u16(len(chunks))  # old chunk count
        + u16(120)  # duration ms
        + b"\x00\x00"  # reserved
        + u32(len(chunks))  # new chunk count
        + body
    )
    frame = u32(len(frame) + 4) + frame  # prepend frame size (incl this field)

    payload = header + frame
    return u32(len(payload) + 4) + payload  # prepend total file size


def make_indexed():
    # Palette: 0 = transparent, 1 = red, 2 = green, 3 = blue.
    palette = [(0, 0, 0, 0), (220, 60, 60, 255), (60, 200, 80, 255), (70, 120, 230, 255)]
    pixels = bytearray(W * H)
    for y in range(H):
        for x in range(W):
            border = x == 0 or y == 0 or x == W - 1 or y == H - 1
            pixels[y * W + x] = 0 if border else (1 + ((x // 4 + y // 4) % 3))
    return build_file(8, 0, [layer_chunk("indexed"), palette_chunk(palette), cel_chunk(0, bytes(pixels))])


def make_grayscale():
    # 16-bpp: value, alpha. Horizontal value gradient, full alpha.
    pixels = bytearray(W * H * 2)
    for y in range(H):
        for x in range(W):
            v = int(255 * x / (W - 1))
            i = (y * W + x) * 2
            pixels[i] = v
            pixels[i + 1] = 255
    return build_file(16, 0, [layer_chunk("gray"), cel_chunk(0, bytes(pixels))])


def make_multiply():
    # 32-bpp RGBA, two layers. Base = solid orange; top = Multiply, gray on the left
    # half (darkens) and white on the right half (no change). The left half should come
    # out darker than the right if Multiply composites correctly.
    def rgba_fill(rgba):
        out = bytearray(W * H * 4)
        for p in range(W * H):
            out[p * 4 : p * 4 + 4] = bytes(rgba)
        return bytes(out)

    base = rgba_fill((255, 140, 0, 255))  # orange
    top = bytearray(W * H * 4)
    for y in range(H):
        for x in range(W):
            c = (128, 128, 128, 255) if x < W // 2 else (255, 255, 255, 255)
            top[(y * W + x) * 4 : (y * W + x) * 4 + 4] = bytes(c)
    chunks = [
        layer_chunk("base", blend_mode=0),
        layer_chunk("shade", blend_mode=1),  # Multiply
        cel_chunk(0, base),
        cel_chunk(1, bytes(top)),
    ]
    return build_file(32, 0, chunks)


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        "Assets", "Diorama", "Examples", "Aseprite")
    os.makedirs(out_dir, exist_ok=True)

    files = {
        "indexed_test.aseprite": make_indexed(),
        "grayscale_test.aseprite": make_grayscale(),
        "multiply_test.aseprite": make_multiply(),
    }
    for name, data in files.items():
        path = os.path.join(out_dir, name)
        with open(path, "wb") as f:
            f.write(data)
        print("wrote {} ({} bytes)".format(path, len(data)))
    print("Done. The AssetProcessor will build these to diorama/examples/aseprite/*.dioramasheet")


if __name__ == "__main__":
    main()
