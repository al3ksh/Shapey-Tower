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

Each row of the sheet is one skin (palette swap), see SKINS - keep the order in
sync with include/skins.h.

No external dependencies - writes the PNG with zlib + struct.
Run:  python tools/make_player_sprite.py
"""
import os
import struct
import zlib

FW, FH = 32, 40
FRAMES = 9

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


def make(bob, legs, arms, tail, hurt=False):
    f = Frame()
    (bh, bf), (fh, ff) = legs
    (bs, bhd), (fs, fhd) = arms
    draw_arm(f, bs, bhd, True)
    draw_leg(f, bh, bf, True)
    draw_torso(f, bob)
    draw_leg(f, fh, ff, False)
    draw_scarf(f, bob, tail)
    draw_head(f, bob, hurt)
    draw_arm(f, fs, fhd, False)
    f.outline()
    return f


def frames():
    hipB, hipF = (13, 26), (19, 26)
    out = []
    tail_idle = [(10, 16), (10, 18), (9, 20)]
    tail_run_a = [(10, 15), (8, 15), (6, 16), (4, 16)]
    tail_run_b = [(10, 15), (8, 16), (6, 15), (4, 14)]
    # idle
    for bob in (0, 1):
        out.append(make(bob,
                        ((hipB, (13, 36)), (hipF, (19, 36))),
                        (((12, 18 + bob), (11, 26 + bob)), ((20, 18 + bob), (21, 26 + bob))),
                        tail_idle))
    # run cycle
    run = [
        (0, ((10, 36), (23, 36)), ((21, 22), (10, 24)), tail_run_a),
        (1, ((12, 32), (17, 36)), ((16, 25), (15, 25)), tail_run_b),
        (0, ((22, 36), (10, 36)), ((10, 24), (23, 22)), tail_run_a),
        (1, ((16, 36), (14, 32)), ((15, 25), (16, 25)), tail_run_b),
    ]
    for bob, (bfoot, ffoot), (bhand, fhand), tail in run:
        out.append(make(bob,
                        (((hipB[0], hipB[1] + bob), bfoot), ((hipF[0], hipF[1] + bob), ffoot)),
                        (((12, 18 + bob), bhand), ((20, 18 + bob), fhand)),
                        tail))
    # jump: legs tucked, arms reaching up, scarf trailing down
    out.append(make(-1,
                    ((hipB, (12, 33)), (hipF, (22, 32))),
                    (((12, 17), (9, 9)), ((20, 17), (24, 8))),
                    [(10, 16), (9, 18), (8, 20), (8, 22)]))
    # fall: legs dangling, arms out, scarf blown upward
    out.append(make(0,
                    ((hipB, (12, 37)), (hipF, (20, 35))),
                    (((12, 18), (6, 15)), ((20, 18), (26, 14))),
                    [(10, 14), (9, 12), (8, 10), (8, 8)]))
    # wall slide: front hand on the wall (right edge), knees bent
    out.append(make(0,
                    ((hipB, (11, 36)), (hipF, (22, 34))),
                    (((12, 18), (9, 24)), ((20, 18), (28, 12))),
                    [(10, 14), (9, 12), (9, 10)]))
    # hurt: flung like a ragdoll
    out.append(make(0,
                    ((hipB, (8, 34)), (hipF, (25, 33))),
                    (((12, 18), (5, 8)), ((20, 18), (27, 9))),
                    [(10, 13), (8, 11), (7, 9), (5, 8)], hurt=True))
    return out


# name, price in coins, palette overrides (order == Skin ids in include/skins.h)
SKINS = [
    ("The Shape", 0, {}),
    ("Ember", 60, {"O": (220, 52, 40, 255), "o": (138, 24, 20, 255), "S": (48, 44, 46, 255),
                   "s": (30, 28, 30, 255), "t": (74, 68, 70, 255), "a": (88, 80, 82, 255), "G": (28, 24, 24, 255)}),
    ("Frost", 120, {"S": (122, 154, 200, 255), "s": (88, 118, 166, 255), "t": (168, 200, 232, 255),
                    "a": (150, 180, 220, 255), "H": (210, 222, 240, 255), "h": (255, 255, 255, 255),
                    "O": (90, 220, 255, 255), "o": (40, 140, 200, 255), "G": (230, 240, 255, 255)}),
    ("Toxic", 180, {"S": (74, 90, 42, 255), "s": (50, 62, 28, 255), "t": (104, 124, 60, 255),
                    "a": (118, 138, 70, 255), "O": (160, 255, 60, 255), "o": (90, 170, 20, 255),
                    "M": (220, 236, 190, 255), "m": (170, 190, 140, 255)}),
    ("Royal", 300, {"S": (74, 40, 110, 255), "s": (48, 24, 76, 255), "t": (110, 70, 150, 255),
                    "a": (124, 84, 168, 255), "H": (60, 30, 90, 255), "h": (96, 56, 136, 255),
                    "M": (255, 214, 70, 255), "m": (200, 150, 30, 255), "O": (240, 240, 250, 255),
                    "o": (170, 170, 190, 255)}),
    ("Bone", 500, {"S": (30, 30, 34, 255), "s": (18, 18, 22, 255), "t": (52, 52, 60, 255),
                   "a": (44, 44, 50, 255), "H": (22, 22, 26, 255), "h": (46, 46, 54, 255),
                   "O": (230, 230, 230, 255), "o": (150, 150, 150, 255), "G": (230, 226, 210, 255)}),
    ("Glitch", 800, {"S": (26, 22, 44, 255), "s": (16, 12, 30, 255), "t": (60, 40, 110, 255),
                     "a": (50, 36, 90, 255), "H": (14, 12, 24, 255), "h": (255, 60, 200, 255),
                     "M": (90, 255, 230, 255), "m": (30, 170, 170, 255), "O": (255, 60, 200, 255),
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
    fs = frames()
    W, H = FW * len(fs) * scale, FH * len(SKINS) * scale
    rows = []
    for y in range(H):
        row = []
        skin, fy = divmod(y // scale, FH)
        pal = dict(PAL, **SKINS[skin][2])
        for x in range(W):
            fi, fx = divmod(x // scale, FW)
            c = fs[fi].px[fy][fx]
            row.extend(pal[c] if c else (0, 0, 0, 0))
        rows.append(row)
    root = os.path.join(os.path.dirname(__file__), "..", "assets", "textures")
    write_png(os.path.join(root, name), W, H, rows)
    print(f"wrote {name} ({W}x{H}, {len(fs)} frames x {len(SKINS)} skins)")


if __name__ == "__main__":
    import sys
    main()
    if "--preview" in sys.argv:
        main(scale=8, name="player_sheet_preview.png")
