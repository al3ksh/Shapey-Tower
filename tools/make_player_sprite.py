"""Generates assets/textures/player_sheet.png - the Shapey Tower hero.

Pixel-art masked climber: dark hood + coverall, pale mask with geometric
eye holes (triangle + circle), orange scarf that trails behind in motion.

Frames (32x40 each, laid out horizontally, character faces right):
  0-1  idle (breathing)
  2-5  run cycle
  6    jump (rising)
  7    fall
  8    wall slide (hand on the wall to the right)
  9    hurt / death (X eyes, limbs flung out)

Each row of the sheet is one skin: a palette plus accessories drawn on top of
the shared poses (crowns, capes, gas masks, skull faces, glitch effects...).
See SKINS - keep the order in sync with include/skins.h.

No external dependencies - writes the PNG with zlib + struct.
Run:  python tools/make_player_sprite.py [--preview]
"""
import os
import struct
import zlib

FW, FH = 32, 40

PAL = {
    "K": (18, 16, 24, 255),     # outline
    "H": (40, 38, 54, 255),     # hood
    "h": (64, 62, 84, 255),     # hood highlight
    "M": (236, 230, 212, 255),  # mask
    "m": (192, 184, 164, 255),  # mask shade
    "E": (10, 8, 12, 255),      # eye holes
    "S": (46, 56, 74, 255),     # suit
    "s": (30, 36, 50, 255),     # suit shade / back limbs
    "t": (70, 84, 106, 255),    # suit highlight
    "a": (84, 100, 126, 255),   # front arm (reads over the torso)
    "O": (232, 124, 44, 255),   # scarf
    "o": (168, 78, 28, 255),    # scarf shade
    "G": (96, 64, 46, 255),     # gloves
    "B": (58, 40, 32, 255),     # boots
    "b": (36, 24, 20, 255),     # boot sole
    # accessory colours
    "Y": (255, 206, 72, 255),   # gold
    "y": (186, 124, 36, 255),   # gold shade
    "R": (222, 38, 62, 255),    # ruby
    "F": (255, 232, 110, 255),  # flame core
    "f": (255, 140, 40, 255),   # flame
    "r": (196, 48, 28, 255),    # flame base
    "I": (214, 244, 255, 255),  # ice
    "i": (120, 190, 236, 255),  # ice shade
    "W": (242, 236, 218, 255),  # bone
    "w": (176, 166, 142, 255),  # bone shade
    "P": (118, 46, 156, 255),   # cape
    "p": (74, 26, 104, 255),    # cape shade
    "C": (90, 255, 230, 255),   # cyan glow
    "c": (255, 60, 200, 255),   # magenta glow
    "L": (150, 255, 90, 255),   # toxic glow
    "l": (48, 96, 30, 255),     # dark lens
    "g": (126, 132, 140, 255),  # metal
    "n": (66, 70, 80, 255),     # dark metal / rubber
    "X": (240, 200, 40, 255),   # hazard yellow
}


class Frame:
    def __init__(self):
        self.px = [[None] * FW for _ in range(FH)]

    def set(self, x, y, c):
        if 0 <= x < FW and 0 <= y < FH:
            self.px[y][x] = c

    def rect(self, x, y, w, h, c):
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                self.set(xx, yy, c)

    def limb(self, x0, y0, x1, y1, width, c):
        steps = max(abs(x1 - x0), abs(y1 - y0), 1)
        for i in range(steps + 1):
            t = i / steps
            x = round(x0 + (x1 - x0) * t)
            y = round(y0 + (y1 - y0) * t)
            self.rect(x - width // 2, y, width, 1, c)
            self.rect(x - width // 2, y - 1 if i else y, width, 1, c)

    def outline(self):
        add = []
        for y in range(FH):
            for x in range(FW):
                if self.px[y][x] is not None:
                    continue
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < FW and 0 <= ny < FH and self.px[ny][nx] not in (None, "K"):
                        add.append((x, y))
                        break
        for x, y in add:
            self.px[y][x] = "K"


def draw_head(f, bob, hurt=False):
    y = 3 + bob
    # hood silhouette
    f.rect(12, y, 8, 1, "H")
    f.rect(10, y + 1, 12, 1, "H")
    f.rect(9, y + 2, 14, 9, "H")
    f.rect(10, y + 11, 12, 1, "H")
    f.rect(10, y + 2, 1, 7, "h")
    f.rect(12, y + 1, 5, 1, "h")
    # mask (offset to the facing side)
    f.rect(14, y + 3, 7, 1, "M")
    f.rect(13, y + 4, 9, 6, "M")
    f.rect(14, y + 10, 7, 1, "M")
    f.rect(13, y + 4, 1, 6, "m")
    f.rect(14, y + 10, 7, 1, "m")
    if hurt:
        for ex in (14, 18):  # X eyes
            f.set(ex, y + 5, "E"); f.set(ex + 2, y + 5, "E")
            f.set(ex + 1, y + 6, "E")
            f.set(ex, y + 7, "E"); f.set(ex + 2, y + 7, "E")
        f.set(20, y + 3, "m"); f.set(19, y + 4, "m")  # crack
        return
    # triangle eye
    f.set(15, y + 5, "E")
    f.rect(14, y + 6, 3, 1, "E")
    # round eye
    f.rect(18, y + 5, 2, 2, "E")
    f.set(18, y + 5, "m")
    # stitched mouth
    f.set(16, y + 8, "m")
    f.set(18, y + 8, "m")
    f.set(20, y + 8, "m")


def draw_scarf(f, bob, tail):
    y = 15 + bob
    f.rect(11, y, 11, 2, "O")
    f.rect(11, y + 1, 11, 1, "o")
    for i, (x, yy) in enumerate(tail):
        f.set(x, yy + bob, "o" if i % 2 else "O")
        f.set(x, yy + bob + 1, "o")


def draw_torso(f, bob):
    y = 17 + bob
    f.rect(11, y, 11, 10, "S")
    f.rect(11, y, 2, 10, "s")
    f.rect(19, y + 1, 2, 7, "t")
    f.rect(16, y, 1, 9, "s")   # zipper
    f.rect(11, y + 8, 11, 1, "s")  # belt
    f.set(16, y + 8, "O")


def draw_leg(f, hip, foot, back):
    col = "s" if back else "S"
    f.limb(hip[0], hip[1], foot[0], foot[1] - 1, 4, col)
    fx, fy = foot
    f.rect(fx - 2, fy - 1, 5, 2, "B")
    f.rect(fx - 2, fy + 1, 5, 1, "b")


def draw_arm(f, shoulder, hand, back):
    col = "s" if back else "a"
    f.limb(shoulder[0], shoulder[1], hand[0], hand[1], 3, col)
    f.rect(hand[0] - 1, hand[1], 3, 2, "G")


def _noop(f, ctx):
    pass


def make(pose, skin=0):
    bob, legs, arms, tail, hurt, idx = pose
    ctx = {"bob": bob, "tail": tail, "hurt": hurt, "idx": idx, "head_y": 3 + bob, "torso_y": 17 + bob}
    kit = ACCESSORIES.get(SKINS[skin][0], {})
    f = Frame()
    (bh, bf), (fh, ff) = legs
    (bs, bhd), (fs, fhd) = arms
    kit.get("back", _noop)(f, ctx)
    draw_arm(f, bs, bhd, True)
    draw_leg(f, bh, bf, True)
    draw_torso(f, bob)
    kit.get("torso", _noop)(f, ctx)
    draw_leg(f, fh, ff, False)
    draw_scarf(f, bob, tail)
    kit.get("scarf", _noop)(f, ctx)
    draw_head(f, bob, hurt)
    kit.get("head", _noop)(f, ctx)
    draw_arm(f, fs, fhd, False)
    f.outline()
    kit.get("post", _noop)(f, ctx)
    return f


# ---------------------------------------------------------------------------------------------
# Accessories. Coordinates follow draw_head/draw_torso: the hood spans x 9..22 from head_y,
# the mask x 13..21 at head_y+3..+10, the torso x 11..21 from torso_y. Facing right.
# ---------------------------------------------------------------------------------------------
def ember_head(f, c):
    y, i = c["head_y"], c["idx"]
    # flickering flame crest replacing the top of the hood
    for x in range(10, 22):
        h = 2 + (x * 5 + i * 3) % 4
        if x in (10, 21):
            h -= 1
        for k in range(h):
            f.set(x, y + 1 - k, "r" if k == 0 else ("f" if k < h - 1 else "F"))
    if not c["hurt"]:  # glowing eyes
        f.set(15, y + 5, "F"); f.rect(14, y + 6, 3, 1, "f")
        f.rect(18, y + 5, 2, 2, "F")


def ember_post(f, c):
    y, i = c["head_y"], c["idx"]
    for k in range(3):  # sparks drifting above the flames (no outline)
        sx = 9 + (i * 7 + k * 5) % 14
        sy = max(0, y - 3 - (i + k * 2) % 2)
        f.px[sy][sx] = "F" if k % 2 else "f"


def frost_head(f, c):
    y = c["head_y"]
    for x, h in ((10, 2), (12, 3), (14, 4), (16, 5), (18, 4), (20, 3)):  # icicle crown
        for k in range(h):
            f.set(x, y - k, "I" if k else "i")
        f.set(x + 1, y, "i")
    if not c["hurt"]:
        f.set(15, y + 5, "i"); f.rect(14, y + 6, 3, 1, "i")
        f.rect(18, y + 5, 2, 2, "I")


def frost_scarf(f, c):
    y = c["torso_y"]
    for x, ln in ((12, 2), (14, 1), (17, 2), (20, 1)):  # icicles hanging off the scarf
        f.rect(x, y, 1, ln, "I")


def frost_torso(f, c):
    cx, cy = 19, c["torso_y"] + 4  # snowflake emblem
    f.set(cx, cy, "I")
    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        f.set(cx + dx, cy + dy, "i")


def toxic_back(f, c):
    y = c["torso_y"]
    f.rect(6, y, 5, 10, "g")       # air tank on the back
    f.rect(6, y, 1, 10, "n")
    f.rect(7, y + 3, 2, 5, "L")    # glowing liquid window
    f.rect(7, y - 1, 3, 1, "n")    # valve


def toxic_torso(f, c):
    y = c["torso_y"] + 8
    for x in range(11, 22):        # hazard-striped belt
        f.set(x, y, "X" if (x // 2) % 2 else "b")


def toxic_head(f, c):
    y, hurt = c["head_y"], c["hurt"]
    f.rect(13, y + 3, 9, 8, "n")   # rubber mask over the face
    f.rect(14, y + 3, 7, 1, "g")
    for ex in (14, 18):            # round goggles
        f.rect(ex - 1, y + 4, 4, 4, "g")
        f.rect(ex, y + 5, 2, 2, "l" if hurt else "L")
    if hurt:
        f.set(14, y + 5, "E"); f.set(19, y + 6, "E")
    f.rect(19, y + 9, 4, 3, "g")   # filter canister
    f.rect(20, y + 10, 3, 1, "n")
    f.set(22, y + 9, "n")
    for k in range(5):             # hose from the tank to the mask
        f.set(9 + k, y + 13 - k // 2, "n")


def royal_back(f, c):
    tail, bob = c["tail"], c["bob"]
    sx, sy = 11, 16 + bob
    dx = tail[-1][0] - tail[0][0]
    dy = tail[-1][1] - tail[0][1]
    # the cape follows the scarf's motion but always hangs mostly downward
    ex = sx + dx * 1.2 - 2
    ey = max(sy + 6, sy + 16 + dy * 0.8)
    n = 16
    for k in range(n + 1):
        t = k / n
        px = round(sx + (ex - sx) * t)
        py = round(sy + (ey - sy) * t)
        w = 3 + round(t * 5)
        f.rect(px - w // 2, py, w, 2, "Y" if k == n else ("p" if (k // 3) % 2 else "P"))


def royal_head(f, c):
    y = c["head_y"]
    f.rect(11, y, 10, 1, "Y")      # crown band
    f.rect(11, y + 1, 10, 1, "y")
    for x in (11, 12, 15, 16, 19, 20):
        f.set(x, y - 1, "Y")
    for x in (11, 15, 16, 20):
        f.set(x, y - 2, "Y")
    f.set(15, y - 1, "R"); f.set(16, y - 1, "R")  # ruby
    f.set(13, y, "R"); f.set(18, y, "R")


def royal_scarf(f, c):
    y = 16 + c["bob"]
    for x in range(12, 22, 2):     # gold chain over the collar
        f.set(x, y, "Y")


def bone_head(f, c):
    y, hurt = c["head_y"], c["hurt"]
    f.rect(14, y + 3, 7, 1, "W")   # skull face
    f.rect(13, y + 4, 9, 5, "W")
    f.rect(14, y + 9, 7, 2, "W")
    f.rect(13, y + 4, 1, 5, "w")
    if hurt:
        for ex in (14, 18):
            f.set(ex, y + 5, "E"); f.set(ex + 2, y + 5, "E"); f.set(ex + 1, y + 6, "E")
            f.set(ex, y + 7, "E"); f.set(ex + 2, y + 7, "E")
    else:
        f.rect(14, y + 5, 2, 2, "E"); f.rect(18, y + 5, 2, 2, "E")  # sockets
        f.set(17, y + 7, "E")                                       # nose
    for x in range(14, 21):                                          # teeth
        f.set(x, y + 9, "E" if x % 2 else "W")
    f.rect(14, y + 10, 7, 1, "w")


def bone_torso(f, c):
    y = c["torso_y"]
    f.rect(16, y, 1, 8, "w")       # spine
    for r in (1, 3, 5):            # ribs
        f.rect(12, y + r, 4, 1, "W")
        f.rect(17, y + r, 4, 1, "W")


def glitch_head(f, c):
    y, i = c["head_y"], c["idx"]
    f.rect(13, y + 4, 9, 3, "C")   # visor band with scrolling bits
    for k in range(3):
        f.set(13 + (i * 3 + k * 4) % 9, y + 5, "c")
    if c["hurt"]:
        f.rect(13, y + 5, 9, 1, "E")
    f.rect(19, y - 3, 1, 3, "n")   # antenna
    f.set(19, y - 4, "c")


def glitch_post(f, c):
    i = c["idx"]
    for r0 in (8 + (i * 7) % 22, 8 + (i * 13 + 5) % 22):  # displaced scanlines
        for yy in (r0, r0 + 1):
            f.px[yy] = [None, None] + f.px[yy][:-2]
    for yy in range(FH):           # chromatic split on the silhouette edges
        if (yy + i) % 3:
            continue
        row = f.px[yy]
        for x in range(1, FW - 1):
            if row[x] == "K":
                if row[x - 1] is None:
                    row[x - 1] = "c"
                elif row[x + 1] is None:
                    row[x + 1] = "C"


ACCESSORIES = {
    "Ember": {"head": ember_head, "post": ember_post},
    "Frost": {"head": frost_head, "scarf": frost_scarf, "torso": frost_torso},
    "Toxic": {"back": toxic_back, "torso": toxic_torso, "head": toxic_head},
    "Royal": {"back": royal_back, "head": royal_head, "scarf": royal_scarf},
    "Bone": {"head": bone_head, "torso": bone_torso},
    "Glitch": {"head": glitch_head, "post": glitch_post},
}


def poses():
    """(bob, legs, arms, scarf tail, hurt, frame index) for every frame."""
    hipB, hipF = (13, 26), (19, 26)
    out = []
    tail_idle = [(10, 16), (10, 18), (9, 20)]
    tail_run_a = [(10, 15), (8, 15), (6, 16), (4, 16)]
    tail_run_b = [(10, 15), (8, 16), (6, 15), (4, 14)]
    # idle
    for bob in (0, 1):
        out.append((bob,
                    ((hipB, (13, 36)), (hipF, (19, 36))),
                    (((12, 18 + bob), (11, 26 + bob)), ((20, 18 + bob), (21, 26 + bob))),
                    tail_idle, False))
    # run cycle
    run = [
        (0, ((10, 36), (23, 36)), ((21, 22), (10, 24)), tail_run_a),
        (1, ((12, 32), (17, 36)), ((16, 25), (15, 25)), tail_run_b),
        (0, ((22, 36), (10, 36)), ((10, 24), (23, 22)), tail_run_a),
        (1, ((16, 36), (14, 32)), ((15, 25), (16, 25)), tail_run_b),
    ]
    for bob, (bfoot, ffoot), (bhand, fhand), tail in run:
        out.append((bob,
                    (((hipB[0], hipB[1] + bob), bfoot), ((hipF[0], hipF[1] + bob), ffoot)),
                    (((12, 18 + bob), bhand), ((20, 18 + bob), fhand)),
                    tail, False))
    # jump: legs tucked, arms reaching up, scarf trailing down
    out.append((-1,
                ((hipB, (12, 33)), (hipF, (22, 32))),
                (((12, 17), (9, 9)), ((20, 17), (24, 8))),
                [(10, 16), (9, 18), (8, 20), (8, 22)], False))
    # fall: legs dangling, arms out, scarf blown upward
    out.append((0,
                ((hipB, (12, 37)), (hipF, (20, 35))),
                (((12, 18), (6, 15)), ((20, 18), (26, 14))),
                [(10, 14), (9, 12), (8, 10), (8, 8)], False))
    # wall slide: front hand on the wall (right edge), knees bent
    out.append((0,
                ((hipB, (11, 36)), (hipF, (22, 34))),
                (((12, 18), (9, 24)), ((20, 18), (28, 12))),
                [(10, 14), (9, 12), (9, 10)], False))
    # hurt: flung like a ragdoll
    out.append((0,
                ((hipB, (8, 34)), (hipF, (25, 33))),
                (((12, 18), (5, 8)), ((20, 18), (27, 9))),
                [(10, 13), (8, 11), (7, 9), (5, 8)], True))
    return [p + (i,) for i, p in enumerate(out)]


# name, price in coins, palette overrides (order == Skin ids in include/skins.h)
SKINS = [
    ("The Shape", 0, {}),
    ("Ember", 60, {"O": (220, 52, 40, 255), "o": (138, 24, 20, 255), "S": (48, 44, 46, 255),
                   "s": (30, 28, 30, 255), "t": (74, 68, 70, 255), "a": (88, 80, 82, 255), "G": (28, 24, 24, 255),
                   "H": (60, 34, 30, 255), "h": (96, 52, 40, 255)}),
    ("Frost", 120, {"S": (122, 154, 200, 255), "s": (88, 118, 166, 255), "t": (168, 200, 232, 255),
                    "a": (150, 180, 220, 255), "H": (150, 184, 226, 255), "h": (206, 228, 250, 255),
                    "O": (90, 220, 255, 255), "o": (40, 140, 200, 255), "G": (230, 240, 255, 255)}),
    ("Toxic", 180, {"S": (74, 90, 42, 255), "s": (50, 62, 28, 255), "t": (104, 124, 60, 255),
                    "a": (118, 138, 70, 255), "O": (160, 255, 60, 255), "o": (90, 170, 20, 255),
                    "H": (44, 52, 30, 255), "h": (70, 82, 44, 255), "G": (40, 44, 36, 255)}),
    ("Royal", 300, {"S": (74, 40, 110, 255), "s": (48, 24, 76, 255), "t": (110, 70, 150, 255),
                    "a": (124, 84, 168, 255), "H": (60, 30, 90, 255), "h": (96, 56, 136, 255),
                    "M": (255, 214, 70, 255), "m": (200, 150, 30, 255), "O": (240, 240, 250, 255),
                    "o": (170, 170, 190, 255)}),
    ("Bone", 500, {"S": (30, 30, 34, 255), "s": (18, 18, 22, 255), "t": (52, 52, 60, 255),
                   "a": (44, 44, 50, 255), "H": (22, 22, 26, 255), "h": (46, 46, 54, 255),
                   "O": (150, 20, 30, 255), "o": (96, 10, 20, 255), "G": (230, 226, 210, 255)}),
    ("Glitch", 800, {"S": (26, 22, 44, 255), "s": (16, 12, 30, 255), "t": (60, 40, 110, 255),
                     "a": (50, 36, 90, 255), "H": (14, 12, 24, 255), "h": (255, 60, 200, 255),
                     "M": (40, 40, 60, 255), "m": (30, 30, 46, 255), "O": (255, 60, 200, 255),
                     "o": (150, 20, 120, 255)}),
]


def write_png(path, w, h, rows):
    raw = b"".join(b"\x00" + bytes(r) for r in rows)

    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9))
    png += chunk(b"IEND", b"")
    with open(path, "wb") as fp:
        fp.write(png)


def main(scale=1, name="player_sheet.png"):
    ps = poses()
    sheet = [[make(p, si) for p in ps] for si in range(len(SKINS))]
    W, H = FW * len(ps) * scale, FH * len(SKINS) * scale
    rows = []
    for y in range(H):
        row = []
        skin, fy = divmod(y // scale, FH)
        pal = dict(PAL, **SKINS[skin][2])
        for x in range(W):
            fi, fx = divmod(x // scale, FW)
            c = sheet[skin][fi].px[fy][fx]
            row.extend(pal[c] if c else (0, 0, 0, 0))
        rows.append(row)
    root = os.path.join(os.path.dirname(__file__), "..", "assets", "textures")
    write_png(os.path.join(root, name), W, H, rows)
    print(f"wrote {name} ({W}x{H}, {len(ps)} frames x {len(SKINS)} skins)")


def showcase(path, scale=4, frame=0, gap=6):
    """One idle frame of every skin side by side (used for the README)."""
    ps = poses()
    figs = [make(ps[frame], si) for si in range(len(SKINS))]
    W = (FW * len(SKINS) + gap * (len(SKINS) - 1)) * scale
    H = FH * scale
    rows = []
    for y in range(H):
        row = []
        fy = y // scale
        for x in range(W):
            cx = x // scale
            si, fx = divmod(cx, FW + gap)
            c = figs[si].px[fy][fx] if fx < FW else None
            row.extend(dict(PAL, **SKINS[si][2])[c] if c else (0, 0, 0, 0))
        rows.append(row)
    write_png(path, W, H, rows)
    print(f"wrote {path} ({W}x{H})")


if __name__ == "__main__":
    import sys
    main()
    if "--preview" in sys.argv:
        main(scale=4, name="player_sheet_preview.png")
    if "--showcase" in sys.argv:
        showcase(sys.argv[sys.argv.index("--showcase") + 1])
