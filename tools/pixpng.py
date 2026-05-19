"""Tiny RGBA canvas + PNG writer shared by the art generators (no dependencies)."""
import struct
import zlib


def rgb(h, a=255):
    h = h.lstrip("#")
    return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16), a)


def mix(c1, c2, t):
    return tuple(round(c1[i] + (c2[i] - c1[i]) * t) for i in range(4))


BAYER4 = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]


class Canvas:
    def __init__(self, w, h, wrap_y=False):
        self.w, self.h, self.wrap_y = w, h, wrap_y
        self.px = [[None] * w for _ in range(h)]

    def get(self, x, y):
        if self.wrap_y:
            y %= self.h
        if 0 <= x < self.w and 0 <= y < self.h:
            return self.px[y][x]
        return None

    def set(self, x, y, c):
        if c is None:
            return
        if self.wrap_y:
            y %= self.h
        if not (0 <= x < self.w and 0 <= y < self.h):
            return
        old = self.px[y][x]
        if c[3] >= 255 or old is None:
            self.px[y][x] = c
            return
        a = c[3] / 255
        oa = old[3] / 255
        na = a + oa * (1 - a)
        if na <= 0:
            return
        col = tuple(round((c[i] * a + old[i] * oa * (1 - a)) / na) for i in range(3))
        self.px[y][x] = col + (round(na * 255),)

    def rect(self, x, y, w, h, c):
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                self.set(xx, yy, c)

    def disc(self, cx, cy, r, c):
        for yy in range(int(cy - r) - 1, int(cy + r) + 2):
            for xx in range(int(cx - r) - 1, int(cx + r) + 2):
                if (xx - cx) ** 2 + (yy - cy) ** 2 <= r * r:
                    self.set(xx, yy, c)

    def line(self, x0, y0, x1, y1, c):
        steps = max(abs(x1 - x0), abs(y1 - y0), 1)
        for i in range(steps + 1):
            t = i / steps
            self.set(round(x0 + (x1 - x0) * t), round(y0 + (y1 - y0) * t), c)

    def dither_v(self, x, y, w, h, c_top, c_bot, levels=6):
        """Vertical gradient quantised to `levels` bands, blended with a 4x4 Bayer pattern."""
        for yy in range(y, y + h):
            t = (yy - y) / max(h - 1, 1) * (levels - 1)
            base = int(t)
            frac = t - base
            for xx in range(x, x + w):
                k = base + (1 if frac * 16 > BAYER4[yy % 4][xx % 4] else 0)
                self.set(xx, yy, mix(c_top, c_bot, min(k, levels - 1) / (levels - 1)))

    def outline(self, c, only_empty=True):
        add = []
        for y in range(self.h):
            for x in range(self.w):
                if self.px[y][x] is not None:
                    continue
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    n = self.get(x + dx, y + dy)
                    if n is not None and n[3] > 200 and n != c:
                        add.append((x, y))
                        break
        for x, y in add:
            self.px[y][x] = c

    def blit(self, other, ox, oy):
        for y in range(other.h):
            for x in range(other.w):
                p = other.px[y][x]
                if p is not None:
                    self.set(ox + x, oy + y, p)


def write_png(path, canvas, scale=1):
    w, h = canvas.w * scale, canvas.h * scale
    rows = []
    for y in range(h):
        row = bytearray(b"\x00")
        src = canvas.px[y // scale]
        for x in range(w):
            p = src[x // scale]
            row.extend(p if p else (0, 0, 0, 0))
        rows.append(bytes(row))

    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(b"".join(rows), 9))
    png += chunk(b"IEND", b"")
    with open(path, "wb") as fp:
        fp.write(png)
