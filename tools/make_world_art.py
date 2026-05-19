"""Generates the pixel-art world: platform tiles and parallax backgrounds.

Outputs (assets/textures/):
  tiles.png        13 rows x [left cap 8 | mid A 16 | mid B 16 | right cap 8], 12px tall.
                   Rows 0-8 follow BiomeType order, then crumbling, spring, ice, ghost.
                   Tile rows 0-8 are the platform body (18 world px at 2x),
                   rows 9-11 hold hanging decoration drawn below the platform.
  bg_<n>.png       per biome, 3 layers of 240x400 side by side:
                   [sky decoration (static) | far layer (tiles vertically) | near layer (tiles vertically)]

Everything is drawn at 1x and scaled 2x in game with point filtering.
Run:  python tools/make_world_art.py [--preview]
"""
import math
import os
import random
import sys

from pixpng import Canvas, mix, rgb, write_png

OUT = os.path.join(os.path.dirname(__file__), "..", "assets", "textures")
TILE_H, BODY_H = 12, 9
CAP, MID = 8, 16
ROW_W = CAP + MID + MID + CAP
BW, BH = 240, 400

# --------------------------------------------------------------------------- tiles


def bricks(x, y, bw, bh, pal, rough=0, rng=None):
    """Offset brick pattern. pal = (hi, mid, dark, mortar)."""
    hi, mid, dark, mortar = pal
    row = y // bh
    off = (bw // 2) if row % 2 else 0
    lx = (x + off) % bw
    ly = y % bh
    if lx == 0 or ly == bh - 1:
        return mortar
    if ly == 0 or lx == 1:
        return hi
    if lx == bw - 1 or ly == bh - 2:
        return dark
    return mid


def planks(x, y, pal):
    hi, mid, dark, seam = pal
    lx = x % 16
    if lx == 0:
        return seam
    if y in (0,) or lx == 1:
        return hi
    if y == BODY_H - 2:
        return dark
    if (lx + y * 3) % 11 == 0:
        return dark
    if lx in (3, 12) and y in (2, 6):
        return seam  # nails
    return mid


STYLES = {}


def style(idx):
    def deco(fn):
        STYLES[idx] = fn
        return fn
    return deco


def base_tile(body_fn, top_fn=None, hang_fn=None, outline=rgb("161c28"), alpha=255, seed=0):
    """Builds one 48x12 row. body_fn(x, y, variant) for the 9 body rows."""
    rng = random.Random(seed)
    c = Canvas(ROW_W, TILE_H)
    segments = [(0, CAP, "L"), (CAP, MID, "A"), (CAP + MID, MID, "B"), (CAP + 2 * MID, CAP, "R")]
    for sx, sw, kind in segments:
        for lx in range(sw):
            gx = sx + lx
            # pattern coordinate: mids tile with period 16, caps continue the pattern
            px = lx if kind in ("A", "B") else (lx if kind == "L" else lx + 8)
            for y in range(BODY_H):
                col = body_fn(px, y, kind)
                if top_fn:
                    tc = top_fn(px, y, kind)
                    if tc is not None:
                        col = tc
                if col is not None and alpha < 255 and col[3] == 255:
                    col = col[:3] + (alpha,)
                c.set(gx, y, col)
            if hang_fn:
                for y in range(BODY_H, TILE_H):
                    c.set(gx, y, hang_fn(px, y - BODY_H, kind, rng))
        # rounded outer corners on caps
        if kind == "L":
            for (x, y) in ((0, 0), (0, BODY_H - 1)):
                c.px[y][sx + x] = None
        if kind == "R":
            for (x, y) in ((sw - 1, 0), (sw - 1, BODY_H - 1)):
                c.px[y][sx + x] = None
    # outline: bottom edge of body + outer cap edges
    for x in range(ROW_W):
        if c.px[BODY_H - 1][x] is not None:
            c.px[BODY_H - 1][x] = outline
    for y in range(BODY_H):
        for x in (0, ROW_W - 1):
            if c.px[y][x] is not None:
                c.px[y][x] = outline
    c.px[1][1] = c.px[1][1] and outline
    c.px[BODY_H - 2][1] = c.px[BODY_H - 2][1] and outline
    c.px[1][ROW_W - 2] = c.px[1][ROW_W - 2] and outline
    c.px[BODY_H - 2][ROW_W - 2] = c.px[BODY_H - 2][ROW_W - 2] and outline
    return c


def hang_drip(colors, chance, max_len=3):
    cache = {}

    def fn(px, y, kind, rng):
        key = (px, kind)
        if key not in cache:
            cache[key] = rng.randint(1, max_len) if rng.random() < chance else 0
        n = cache[key]
        if y < n:
            return colors[min(y, len(colors) - 1)]
        return None
    return fn


def icicles(colors):
    cache = {}

    def fn(px, y, kind, rng):
        key = (px, kind)
        if key not in cache:
            cache[key] = 0
            if px % 5 == 2 and rng.random() < 0.75:
                cache[key] = rng.randint(2, 3)
            elif px % 5 in (1, 3) and rng.random() < 0.5:
                cache[key] = 1
        n = cache[key]
        if y < n:
            return colors[0] if y < n - 1 else colors[1]
        return None
    return fn


@style(0)  # DEFAULT: blue-grey castle stone
def s_default():
    pal = (rgb("8aa0c0"), rgb("63789a"), rgb("465672"), rgb("2c3650"))
    top = lambda x, y, k: rgb("b8cae4") if y == 0 else None
    return base_tile(lambda x, y, k: bricks(x, y + 1, 8, 4, pal), top,
                     hang_drip([rgb("4f7a4a"), rgb("3a5e38")], 0.18, 2), seed=1)


@style(1)  # FOREST: grass over earth, hanging roots
def s_forest():
    dirt = (rgb("8a6040"), rgb("6b4a2e"), rgb("503620"), rgb("3a2616"))

    def body(x, y, k):
        if (x * 7 + y * 13) % 17 == 0:
            return rgb("9a8a7a")  # pebble
        return bricks(x, y, 16, 5, dirt)

    def top(x, y, k):
        edge = 2 + ((x * 5) % 3 == 0) + ((x * 11) % 7 == 0)
        if y == 0:
            return rgb("9be86a")
        if y < edge:
            return rgb("5cbf45") if (x + y) % 3 else rgb("3f9a38")
        if y == edge:
            return rgb("2e6e2a")
        return None
    return base_tile(body, top, hang_drip([rgb("6b4a2e"), rgb("503620"), rgb("3f9a38")], 0.3, 3),
                     outline=rgb("1c140c"), seed=2)


@style(2)  # LAVA: basalt with glowing cracks and lava drips
def s_lava():
    def body(x, y, k):
        crack = (x * 3 + y * 5) % 13 == 0 or ((x + 2 * y) % 16 == 5 and y > 2)
        if crack:
            return rgb("ffb13a") if y > 4 else rgb("ff6a1a")
        return bricks(x, y, 11, 4, (rgb("5a484e"), rgb("3e3236"), rgb("2c2327"), rgb("1c1518")))
    top = lambda x, y, k: rgb("7a6468") if y == 0 else None
    return base_tile(body, top, hang_drip([rgb("ff8a1a"), rgb("ffcf4a"), rgb("ff5a10")], 0.16, 3),
                     outline=rgb("120c0c"), seed=3)


@style(3)  # SNOW: stone under a thick snow cap, icicles
def s_snow():
    pal = (rgb("8aa4c4"), rgb("6882a6"), rgb("4c6488"), rgb("33455f"))

    def top(x, y, k):
        edge = 3 + (1 if (x // 3) % 2 else 0) - (1 if x % 7 == 0 else 0)
        if y == 0:
            return rgb("ffffff")
        if y < edge:
            return rgb("e6f0ff") if y < edge - 1 else rgb("b8cce8")
        return None
    return base_tile(lambda x, y, k: bricks(x, y, 8, 4, pal), top,
                     icicles([rgb("d8eeff"), rgb("8fc0f0")]), outline=rgb("1a2436"), seed=4)


@style(4)  # COSMIC: violet rock with crystal inclusions
def s_cosmic():
    def body(x, y, k):
        v = (x * 7 + y * 3) % 19
        if v == 0 or (k == "B" and x in (6, 7) and y in (3, 4, 5)):
            return rgb("ffd6ff")
        if v == 1 or (k == "B" and x in (5, 8) and y in (4, 5)):
            return rgb("c07cff")
        return bricks(x, y, 12, 3, (rgb("7a50a0"), rgb("5a3a7a"), rgb("442a5e"), rgb("2e1c42")))
    top = lambda x, y, k: rgb("b48ae0") if y == 0 else None
    return base_tile(body, top, hang_drip([rgb("d38cff"), rgb("8a4ad0")], 0.14, 2),
                     outline=rgb("180e24"), seed=5)


@style(5)  # NEON: dark riveted metal with a light strip on top
def s_neon():
    def body(x, y, k):
        lx = x % 16
        if y <= 1:
            return rgb("f4fffc") if y == 0 else rgb("a8c8c0")
        if lx == 0 or y == BODY_H - 2:
            return rgb("0e1116")
        if lx in (2, 13) and y == 4:
            return rgb("5a6476")
        if y == 2:
            return rgb("3a4252")
        return rgb("262b36") if (lx // 4 + y) % 2 else rgb("222731")
    return base_tile(body, None, hang_drip([rgb("3a4252")], 0.0, 1), outline=rgb("08090c"), seed=6)


@style(6)  # DESERT: carved sandstone
def s_desert():
    pal = (rgb("f0c888"), rgb("d8a664"), rgb("b8844a"), rgb("8a5c2e"))

    def body(x, y, k):
        if k == "B" and 5 <= x <= 10 and 3 <= y <= 5:  # glyph
            g = [(6, 3), (9, 3), (7, 4), (8, 4), (6, 5), (9, 5)]
            if (x, y) in g:
                return rgb("8a5c2e")
        if y in (2, 6) and x % 2 == 0:
            return rgb("c89456")
        return bricks(x, y, 16, 9, pal)
    top = lambda x, y, k: rgb("ffe4b0") if y == 0 else None
    return base_tile(body, top, hang_drip([rgb("e8c080"), rgb("d8a664")], 0.1, 3),
                     outline=rgb("3a2410"), seed=7)


@style(7)  # NIGHT: dark wooden planks
def s_night():
    pal = (rgb("8a6c58"), rgb("6a5040"), rgb("4c382c"), rgb("2a1e18"))
    top = lambda x, y, k: rgb("a88a72") if y == 0 else None
    return base_tile(lambda x, y, k: planks(x, y, pal), top,
                     hang_drip([rgb("3a3a44"), rgb("5a5a66")], 0.08, 3), outline=rgb("140e0c"), seed=8)


@style(8)  # MONO: speckled concrete
def s_mono():
    def body(x, y, k):
        if (x * 13 + y * 7) % 23 == 0:
            return rgb("5a5a5a")
        if (x * 5 + y * 11) % 29 == 0:
            return rgb("c0c0c0")
        if x % 16 == 0:
            return rgb("4a4a4a")
        return rgb("8c8c8c") if y > 1 else rgb("a8a8a8")
    top = lambda x, y, k: rgb("d8d8d8") if y == 0 else None
    return base_tile(body, top, None, outline=rgb("1a1a1a"), seed=9)


@style(9)  # CRUMBLING: cracked clay bricks
def s_crumble():
    pal = (rgb("d4935e"), rgb("b0703e"), rgb("8a522a"), rgb("4a2a14"))

    def body(x, y, k):
        if (x * 5 + y * 3) % 11 == 0 or (x + y) % 9 == 0 and y > 2:
            return rgb("3a2010")
        return bricks(x, y, 6, 3, pal)
    top = lambda x, y, k: rgb("e8b080") if y == 0 else None
    return base_tile(body, top, hang_drip([rgb("8a522a")], 0.2, 1), outline=rgb("24140a"), seed=10)


@style(10)  # SPRING: steel with hazard stripes
def s_spring():
    def body(x, y, k):
        if y <= 2:
            return rgb("ffd23a") if ((x + y) // 3) % 2 == 0 else rgb("2a2a2a")
        if x % 16 == 0 or y == BODY_H - 2:
            return rgb("3a4450")
        if y == 3:
            return rgb("9aa8b8")
        return rgb("6a7888")
    return base_tile(body, None, None, outline=rgb("12161a"), seed=11)


@style(11)  # ICE: translucent blocks with a shine
def s_ice():
    def body(x, y, k):
        if (x + y) % 16 in (3, 4) and y < 6:
            return rgb("ffffff", 240)
        if x % 16 == 0 or y == BODY_H - 2:
            return rgb("5a98d8", 230)
        if y == 0:
            return rgb("f0faff", 245)
        return rgb("a8dcff", 215) if y < 4 else rgb("7cc0f4", 215)
    return base_tile(body, None, icicles([rgb("d8f0ff", 230), rgb("8fc8f8", 200)]),
                     outline=rgb("2a5a90", 230), seed=12)


@style(12)  # GHOST: disappearing platform, dotted and translucent
def s_ghost():
    def body(x, y, k):
        if y == 0 or y == BODY_H - 2:
            return rgb("e8f0ff", 230) if x % 2 == 0 else rgb("a8b8e8", 120)
        if (y + x) % 4 == 0:
            return rgb("c8d8ff", 90)
        return rgb("8898d8", 70)
    return base_tile(body, None, None, outline=rgb("6070b0", 200), seed=13)


def make_tiles():
    sheet = Canvas(ROW_W, TILE_H * len(STYLES))
    for i in sorted(STYLES):
        sheet.blit(STYLES[i](), 0, i * TILE_H)
    return sheet


# --------------------------------------------------------------------- backgrounds


def wrap_rng(seed):
    return random.Random(seed)


def stars(c, rng, n, cols, twinkle_cross=4):
    for i in range(n):
        x, y = rng.randrange(BW), rng.randrange(BH)
        col = rng.choice(cols)
        c.set(x, y, col)
        if i < twinkle_cross:
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                c.set(x + dx, y + dy, col[:3] + (120,))


def moon(c, cx, cy, r, body, shade, glow):
    for rr in range(r + 6, r, -2):
        c.disc(cx, cy, rr, glow[:3] + (18,))
    c.disc(cx, cy, r, body)
    for (dx, dy, rr) in ((-r * 0.3, -r * 0.2, r * 0.22), (r * 0.25, r * 0.3, r * 0.18), (-r * 0.1, r * 0.45, r * 0.12)):
        c.disc(cx + dx, cy + dy, rr, shade)


def silhouette_column(c, x0, x1, top_fn, col, rim=None):
    """Fill columns x0..x1 from top_fn(x) downward to the bottom (tileable canvases use wrap)."""
    for x in range(x0, x1):
        t = top_fn(x)
        for y in range(t, t + BH):
            c.set(x, y, col)
        if rim:
            c.set(x, t, rim)


def side_mass(c, rng, width_fn, col, rim=None, side="both"):
    """Irregular vertical walls hugging the screen edges; width_fn(y) -> px from edge."""
    for y in range(BH):
        w = width_fn(y)
        if side in ("both", "left"):
            for x in range(0, w):
                c.set(x, y, col)
            if rim:
                c.set(w - 1, y, rim)
        if side in ("both", "right"):
            wr = width_fn((y + BH // 2) % BH)
            for x in range(BW - wr, BW):
                c.set(x, y, col)
            if rim:
                c.set(BW - wr, y, rim)


def periodic(y, parts):
    """Smooth periodic function of y (period BH) built from sin terms: [(amp, freq, phase)]."""
    return sum(a * math.sin(2 * math.pi * f * y / BH + p) for a, f, p in parts)


def tower(c, x, y, w, h, col, win, win_lit, rng, roof=None):
    c.rect(x, y, w, h, col)
    if roof == "spire":
        for i in range(w // 2 + 1):
            c.rect(x + i, y - i * 2 - 2, w - 2 * i, 2, col)
    elif roof == "crenel":
        for i in range(0, w, 4):
            c.rect(x + i, y - 3, 2, 3, col)
    for wy in range(y + 4, y + h - 4, 9):
        for wx in range(x + 2, x + w - 3, 5):
            if rng.random() < 0.55:
                c.rect(wx, wy, 2, 4, win_lit if rng.random() < 0.6 else win)


BIOMES = {}


def biome(idx):
    def deco(fn):
        BIOMES[idx] = fn
        return fn
    return deco


@biome(0)  # Start: moonlit castle keep
def b_default():
    sky, far, near = Canvas(BW, BH), Canvas(BW, BH, True), Canvas(BW, BH, True)
    rng = wrap_rng(100)
    stars(sky, rng, 70, [rgb("ffffff", 200), rgb("b8c8ff", 160), rgb("8898c8", 140)])
    moon(sky, 170, 90, 18, rgb("e8eeff"), rgb("c0cae8"), rgb("b8c8ff"))
    # far: distant towers climbing both sides
    fcol, wlit, wdim = rgb("2a3858"), rgb("ffd27a", 200), rgb("3a4a6a")
    for i in range(6):
        y = i * 70 + rng.randrange(20)
        tower(far, rng.randrange(0, 30), y, rng.randrange(18, 30), 60, fcol, wdim, wlit, rng, "spire")
        tower(far, BW - rng.randrange(30, 58), y + 35, rng.randrange(18, 28), 60, fcol, wdim, wlit, rng, "crenel")
    side_mass(far, rng, lambda y: 22 + int(periodic(y, [(6, 2, 0), (3, 5, 1)])), fcol)
    # near: stone wall edges with ivy
    ncol, nrim = rgb("1a2236"), rgb("3a4a6e")
    side_mass(near, rng, lambda y: 16 + int(periodic(y, [(4, 3, 0.5), (2, 7, 2)])), ncol, nrim)
    for y in range(0, BH, 8):
        for side in (0, 1):
            x = (2 + (y // 8) % 2 * 4) if side == 0 else BW - 8 - (y // 8) % 2 * 4
            near.rect(x, y, 5, 1, rgb("121828"))
    for i in range(14):
        x = rng.choice([rng.randrange(8, 22), BW - rng.randrange(8, 22)])
        y0 = rng.randrange(BH)
        for j in range(rng.randrange(10, 30)):
            near.set(x + int(2 * math.sin(j * 0.6)), y0 + j, rgb("2f5a3a") if j % 3 else rgb("447a4a"))
    return sky, far, near


@biome(1)  # Forest: giant trunks, canopy, hanging vines
def b_forest():
    sky, far, near = Canvas(BW, BH), Canvas(BW, BH, True), Canvas(BW, BH, True)
    rng = wrap_rng(200)
    stars(sky, rng, 30, [rgb("d8ffd0", 150), rgb("a0e8a0", 120)])
    moon(sky, 60, 70, 14, rgb("f4ffe8"), rgb("d0e8c0"), rgb("c8ffb0"))
    trunk, leaf1, leaf2 = rgb("1e3222"), rgb("1f4a2a"), rgb("173a22")
    for x0, w in ((6, 14), (BW - 26, 16), (40, 8), (BW - 52, 9)):
        far.rect(x0, 0, w, BH, trunk)
        far.rect(x0 + 2, 0, 2, BH, rgb("26402a"))
    for i in range(22):
        side = rng.random() < 0.5
        cx = rng.randrange(0, 60) if side else rng.randrange(BW - 60, BW)
        cy = rng.randrange(BH)
        r = rng.randrange(10, 22)
        far.disc(cx, cy, r, leaf1)
        far.disc(cx + 3, cy + 3, r - 4, leaf2)
    side_mass(near, rng, lambda y: 14 + int(periodic(y, [(5, 2, 0), (3, 6, 1)])), rgb("0e1a10"), rgb("24402a"))
    for i in range(18):
        x = rng.choice([rng.randrange(6, 30), BW - rng.randrange(6, 30)])
        y0 = rng.randrange(BH)
        ln = rng.randrange(20, 60)
        for j in range(ln):
            xx = x + int(3 * math.sin(j * 0.25 + i))
            near.set(xx, y0 + j, rgb("2e6a30") if j % 4 else rgb("4a9a40"))
            if j % 6 == 0:
                near.set(xx + 1, y0 + j, rgb("5ab84a"))
                near.set(xx - 1, y0 + j + 1, rgb("3a8a38"))
    return sky, far, near


@biome(2)  # Lava: volcanic spires with lavafalls
def b_lava():
    sky, far, near = Canvas(BW, BH), Canvas(BW, BH, True), Canvas(BW, BH, True)
    rng = wrap_rng(300)
    stars(sky, rng, 25, [rgb("ff9a5a", 140), rgb("ff5a2a", 120)], 0)
    for r in range(70, 20, -10):
        sky.disc(120, 380, r, rgb("ff4a10", 14))
    rock = rgb("2a1414")
    side_mass(far, rng, lambda y: 44 + int(periodic(y, [(14, 2, 0), (8, 5, 1), (4, 11, 2)])), rock, rgb("5a2418"))
    for x in (30, 38, BW - 36, BW - 44):
        y0 = rng.randrange(BH)
        for j in range(160):
            col = rgb("ff8a1a") if (j + x) % 5 else rgb("ffd24a")
            far.set(x + (1 if (j // 20) % 2 else 0), y0 + j, col)
            far.set(x + 1 + (1 if (j // 20) % 2 else 0), y0 + j, rgb("c83a10"))
    side_mass(near, rng, lambda y: 18 + int(periodic(y, [(6, 3, 0), (3, 8, 1)])), rgb("140a0a"), rgb("3a1a14"))
    for i in range(40):
        x = rng.choice([rng.randrange(2, 16), BW - rng.randrange(2, 16)])
        y = rng.randrange(BH)
        for j in range(rng.randrange(3, 9)):
            near.set(x + (j % 2), y + j, rgb("ff6a1a") if j % 3 else rgb("ffb13a"))
    return sky, far, near


@biome(3)  # Snow: aurora over icy cliffs
def b_snow():
    sky, far, near = Canvas(BW, BH), Canvas(BW, BH, True), Canvas(BW, BH, True)
    rng = wrap_rng(400)
    stars(sky, rng, 60, [rgb("ffffff", 200), rgb("c8e0ff", 150)])
    for band, col in ((0, rgb("5affc0", 40)), (8, rgb("5ad0ff", 36)), (16, rgb("a07aff", 30))):
        for x in range(BW):
            y = int(60 + band + 14 * math.sin(x / 26) + 6 * math.sin(x / 9))
            for k in range(10):
                sky.set(x, y + k, col[:3] + (max(0, col[3] - k * 3),))
    cliff, snow = rgb("2a4060"), rgb("d8e8ff")
    width = lambda y: 40 + int(periodic(y, [(12, 2, 0), (6, 5, 2), (3, 13, 1)]))
    side_mass(far, rng, width, cliff, rgb("4a6890"))
    for y in range(0, BH, 3):
        w0, w1 = width(y), width(y + 1)
        if w1 > w0 + 1:
            for x in range(w0, w1 + 1):
                far.set(x, y, snow)
    side_mass(near, rng, lambda y: 12 + int(periodic(y, [(5, 3, 0), (2, 9, 1)])), rgb("1a2a44"), rgb("8ab0e0"))
    for i in range(30):
        x = rng.choice([rng.randrange(4, 18), BW - rng.randrange(4, 18)])
        y = rng.randrange(BH)
        for j in range(rng.randrange(3, 10)):
            near.set(x, y + j, rgb("c8e4ff") if j < 3 else rgb("8fc0f0"))
    return sky, far, near


@biome(4)  # Cosmic: nebula, planet, floating crystal rocks
def b_cosmic():
    sky, far, near = Canvas(BW, BH), Canvas(BW, BH, True), Canvas(BW, BH, True)
    rng = wrap_rng(500)
    for i in range(14):
        cx, cy, r = rng.randrange(BW), rng.randrange(BH), rng.randrange(20, 50)
        sky.disc(cx, cy, r, rng.choice([rgb("ff5ad0", 12), rgb("8a5aff", 14), rgb("5ac8ff", 10)]))
    stars(sky, rng, 110, [rgb("ffffff", 220), rgb("ffc8ff", 170), rgb("c8b0ff", 150)], 8)
    sky.disc(185, 120, 22, rgb("6a3aa0"))
    sky.disc(180, 115, 18, rgb("8a5ac8"))
    for x in range(150, 222):
        y = int(120 + (x - 185) * 0.25)
        sky.set(x, y, rgb("e8c8ff", 200))
        sky.set(x, y + 1, rgb("a878e0", 160))
    rock, crystal = rgb("2a1a40"), rgb("d38cff")
    for i in range(10):
        side = i % 2
        cx = rng.randrange(8, 50) if side == 0 else rng.randrange(BW - 50, BW - 8)
        cy = rng.randrange(BH)
        w = rng.randrange(18, 34)
        for k in range(w // 2):
            far.rect(cx - w // 2 + k, cy + k, w - 2 * k, 1, rock)
        far.rect(cx - w // 2, cy - 1, w, 1, rgb("4a3070"))
        for k in range(3):
            x = cx - 6 + k * 5
            far.rect(x, cy - 5 - k, 2, 4 + k, crystal)
    side_mass(near, rng, lambda y: 10 + int(periodic(y, [(4, 3, 0), (3, 7, 1)])), rgb("140c22"), rgb("5a3a8a"))
    for i in range(16):
        x = rng.choice([rng.randrange(8, 16), BW - rng.randrange(8, 16)])
        y = rng.randrange(BH)
        h = rng.randrange(6, 14)
        for j in range(h):
            near.rect(x - j // 4, y + j, 1 + j // 2, 1, rgb("c07cff") if j % 3 else rgb("ffd6ff"))
    return sky, far, near


@biome(5)  # Neon: skyscrapers with lit windows, neon tubes
def b_neon():
    sky, far, near = Canvas(BW, BH), Canvas(BW, BH, True), Canvas(BW, BH, True)
    rng = wrap_rng(600)
    for y in range(200, BH, 16):
        for x in range(BW):
            sky.set(x, y, rgb("2affd0", 18))
    for x in range(0, BW, 20):
        for y in range(200, BH):
            sky.set(x, y, rgb("2affd0", 14))
    stars(sky, rng, 30, [rgb("ffffff", 150)], 0)
    b = rgb("10141e")
    for x0, w in ((0, 26), (26, 18), (44, 14), (BW - 24, 24), (BW - 44, 20), (BW - 58, 14)):
        far.rect(x0, 0, w, BH, b if x0 % 2 else rgb("141a26"))
        for y in range(3, BH, 6):
            for x in range(x0 + 2, x0 + w - 2, 4):
                if rng.random() < 0.45:
                    far.rect(x, y, 2, 3, rng.choice([rgb("2affd0", 170), rgb("ff4ad0", 150), rgb("ffe04a", 150), rgb("304058")]))
    side_mass(near, rng, lambda y: 10, rgb("0a0c12"))
    for side in (0, 1):
        x = 7 if side == 0 else BW - 8
        for y in range(BH):
            near.set(x, y, rgb("0affd0") if (y // 40) % 2 == side else rgb("ff3ac8"))
        for y in range(0, BH, 25):
            near.rect(x - 3, y, 7, 2, rgb("2a3040"))
    return sky, far, near


@biome(6)  # Desert: sun over layered mesas
def b_desert():
    sky, far, near = Canvas(BW, BH), Canvas(BW, BH, True), Canvas(BW, BH, True)
    rng = wrap_rng(700)
    for r in range(56, 26, -6):
        sky.disc(160, 110, r, rgb("fff0a0", 16))
    sky.disc(160, 110, 24, rgb("fff4c8"))
    for y in (104, 112, 118):
        sky.rect(130, y, 60, 1, rgb("ffd890", 90))
    width = lambda y: 46 + int(periodic(y, [(12, 2, 0), (6, 4, 1), (3, 9, 2)]))
    strata = [rgb("a0582a"), rgb("b86a34"), rgb("8a4a22")]
    for y in range(BH):
        col = strata[(y // 6) % 3]
        w = width(y)
        for x in range(w):
            far.set(x, y, col)
        wr = width((y + BH // 2) % BH)
        for x in range(BW - wr, BW):
            far.set(x, y, col)
        far.set(w - 1, y, rgb("d88a4a"))
        far.set(BW - wr, y, rgb("d88a4a"))
    side_mass(near, rng, lambda y: 16, rgb("5a3418"), rgb("8a5428"))
    for y in range(0, BH, 20):
        for side in (0, 1):
            x = 4 if side == 0 else BW - 12
            near.rect(x, y + 4, 8, 1, rgb("3a200c"))
            near.rect(x + 2, y + 8, 4, 4, rgb("3a200c"))
            near.set(x + 3, y + 9, rgb("ffb040"))
    return sky, far, near


@biome(7)  # Night: big moon and castle towers
def b_night():
    sky, far, near = Canvas(BW, BH), Canvas(BW, BH, True), Canvas(BW, BH, True)
    rng = wrap_rng(800)
    stars(sky, rng, 90, [rgb("ffffff", 220), rgb("dde6ff", 160)], 6)
    moon(sky, 120, 100, 32, rgb("fff8e0"), rgb("e0d8b8"), rgb("fff0c0"))
    fcol = rgb("141c3a")
    for i in range(5):
        y = i * 80 + rng.randrange(20)
        tower(far, rng.randrange(0, 20), y, rng.randrange(22, 34), 70, fcol, rgb("1e2848"), rgb("ffcf6a", 210), rng, "spire")
        tower(far, BW - rng.randrange(28, 50), y + 40, rng.randrange(22, 30), 70, fcol, rgb("1e2848"), rgb("ffcf6a", 210), rng, "crenel")
    side_mass(far, rng, lambda y: 18, fcol)
    side_mass(near, rng, lambda y: 14, rgb("0a0e1e"), rgb("2a3458"))
    for y in range(0, BH, 100):
        for side in (0, 1):
            x = 14 if side == 0 else BW - 16
            near.rect(x, y + 20, 2, 6, rgb("4a3020"))
            near.disc(x + 1, y + 18, 2, rgb("ffb040"))
            near.set(x + 1, y + 16, rgb("fff0a0"))
            near.disc(x + 1, y + 18, 7, rgb("ffb040", 30))
    return sky, far, near


@biome(8)  # Mono: gothic spires in grey
def b_mono():
    sky, far, near = Canvas(BW, BH), Canvas(BW, BH, True), Canvas(BW, BH, True)
    rng = wrap_rng(900)
    stars(sky, rng, 40, [rgb("ffffff", 140)], 0)
    moon(sky, 70, 80, 16, rgb("e0e0e0"), rgb("b0b0b0"), rgb("ffffff"))
    for i in range(6):
        y = i * 66 + rng.randrange(20)
        tower(far, rng.randrange(0, 26), y, rng.randrange(14, 22), 56, rgb("2a2a2a"), rgb("3a3a3a"), rgb("8a8a8a"), rng, "spire")
        tower(far, BW - rng.randrange(24, 46), y + 33, rng.randrange(14, 22), 56, rgb("2a2a2a"), rgb("3a3a3a"), rgb("8a8a8a"), rng, "spire")
    side_mass(far, rng, lambda y: 14, rgb("2a2a2a"))
    side_mass(near, rng, lambda y: 12, rgb("111111"), rgb("444444"))
    for side in (0, 1):
        x = 8 if side == 0 else BW - 10
        near.rect(x, 0, 2, BH, rgb("333333"))
        for y in range(0, BH, 32):
            near.rect(x - 1, y, 4, 2, rgb("555555"))
    return sky, far, near


def make_bg(idx):
    sky, far, near = BIOMES[idx]()
    out = Canvas(BW * 3, BH)
    out.blit(sky, 0, 0)
    out.blit(far, BW, 0)
    out.blit(near, BW * 2, 0)
    return out



# --------------------------------------------------------------------------- items

ICONS = {
    "jump": ["...#...", "..###..", ".##.##.", "...#...", "..###..", ".##.##.", "......."],
    "shield": [".#####.", "#######", "#######", "#######", ".#####.", "..###..", "...#..."],
    "slow": ["#######", ".#...#.", "..#.#..", "...#...", "..#.#..", ".#####.", "#######"],
    "magnet": ["##...##", "##...##", "##...##", "##...##", "##...##", ".#####.", "..###.."],
}


def make_items():
    """One 16px row: 4 coin spin frames, then power-up orbs (jump, shield, slow, magnet)."""
    c = Canvas(16 * 8, 16)
    rim, gold, light, shine = rgb("a86a10"), rgb("ffc828"), rgb("ffe890"), rgb("ffffff")
    for f, half_w in enumerate((6, 4.5, 2.5, 1)):
        ox = f * 16
        for y in range(2, 14):
            for x in range(16):
                dx, dy = (x + 0.5 - 8) / half_w, (y + 0.5 - 8) / 6
                d = dx * dx + dy * dy
                if d <= 1:
                    col = rim if d > 0.62 or half_w < 2 else gold
                    if d <= 0.62 and dx < -0.1 and dy < -0.1:
                        col = light
                    c.set(ox + x, y, col)
        if half_w > 3:  # embossed diamond
            for k, w in enumerate((1, 3, 1) if half_w < 5 else (1, 3, 5, 3, 1)):
                c.rect(ox + 8 - w // 2, 8 - (1 if half_w < 5 else 2) + k, w, 1, rim)
        c.set(ox + 6, 4, shine)
    orb_cols = {"jump": ("5ac8ff", "1a5a8a"), "shield": ("6aff7a", "1a6a2a"),
                "slow": ("c07aff", "4a1a7a"), "magnet": ("ffb04a", "7a4a0a")}
    for i, name in enumerate(("jump", "shield", "slow", "magnet")):
        ox = (4 + i) * 16
        light_c, dark_c = rgb(orb_cols[name][0]), rgb(orb_cols[name][1])
        for y in range(16):
            for x in range(16):
                d = ((x + 0.5 - 8) ** 2 + (y + 0.5 - 8) ** 2) ** 0.5
                if d <= 7.2:
                    col = dark_c if d > 6.2 else mix(light_c, dark_c, max(0.0, (y - 4) / 14))
                    c.set(ox + x, y, col)
        c.rect(ox + 4, 3, 3, 1, rgb("ffffff", 220))
        c.set(ox + 3, 4, rgb("ffffff", 180))
        for yy, rowp in enumerate(ICONS[name]):
            for xx, ch in enumerate(rowp):
                if ch == "#":
                    c.set(ox + 4 + xx, 4 + yy + 1, rgb("ffffff"))
        c.outline(rgb("0c0c14"))
    return c

def main(preview=False):
    tiles = make_tiles()
    write_png(os.path.join(OUT, "tiles.png"), tiles)
    print("wrote tiles.png", tiles.w, tiles.h)
    write_png(os.path.join(OUT, "items.png"), make_items())
    if preview:
        write_png(os.path.join(os.path.dirname(__file__), "preview_items.png"), make_items(), 8)
    for i in sorted(BIOMES):
        bg = make_bg(i)
        write_png(os.path.join(OUT, f"bg_{i}.png"), bg)
    print("wrote", len(BIOMES), "backgrounds")
    if preview:
        write_png(os.path.join(os.path.dirname(__file__), "preview_tiles.png"), tiles, 6)
        write_png(os.path.join(os.path.dirname(__file__), "preview_scenes.png"), mock_scenes(tiles), 1)


SKY = {0: ("121828", "2d3a5c"), 1: ("0c2816", "1e502e"), 2: ("280c0c", "601010"), 3: ("060e28", "0e2250"),
       4: ("1e0632", "461470"), 5: ("040420", "0c0c48"), 6: ("c06a28", "f0b060"), 7: ("080c3c", "141e6e"),
       8: ("0a0a0a", "2d2d2d")}


def mock_scenes(tiles):
    """Composites every biome like the game would (2x scale) into one wide preview."""
    out = Canvas(BW * 9, BH)
    for i in sorted(BIOMES):
        sky, far, near = BIOMES[i]()
        sc = Canvas(BW, BH)
        top, bot = SKY[i]
        sc.dither_v(0, 0, BW, BH, rgb(top), rgb(bot), 8)
        sc.blit(sky, 0, 0)
        sc.blit(far, 0, 0)
        sc.blit(near, 0, 0)
        rng = random.Random(i)
        for k in range(7):
            w = rng.randrange(30, 90)
            x = rng.randrange(0, BW - w)
            y = 30 + k * 52
            row = [i, i, 9, 10, 11, 12, i][k]
            draw_platform(sc, tiles, row, x, y, w)
        out.blit(sc, i * BW, 0)
    return out


def draw_platform(c, tiles, row, x, y, w):
    ty = row * TILE_H
    def col(src_x, dx):
        for yy in range(TILE_H):
            p = tiles.px[ty + yy][src_x]
            if p is not None:
                c.set(x + dx, y + yy, p)
    for dx in range(w):
        if dx < CAP:
            col(dx, dx)
        elif dx >= w - CAP:
            col(CAP + 2 * MID + (dx - (w - CAP)), dx)
        else:
            m = (dx - CAP) // MID
            col(CAP + (MID if m % 3 == 2 else 0) + (dx - CAP) % MID, dx)


if __name__ == "__main__":
    main("--preview" in sys.argv)
