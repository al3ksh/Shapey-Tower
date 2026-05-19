#include "world_art.h"
#include "debug.h"
#include <cmath>
#include <string>

namespace WorldArt {

static constexpr int TILE_H = 12;
static constexpr int CAP = 8;
static constexpr int MID = 16;
static constexpr int LAYER_W = 240;
static constexpr int LAYER_H = 400;

static Texture2D LoadPixelTexture(const std::string &name) {
    const std::string paths[] = {"assets/textures/" + name, name};
    for (auto &p : paths) {
        if (FileExists(p.c_str())) {
            Texture2D t = LoadTexture(p.c_str());
            SetTextureFilter(t, TEXTURE_FILTER_POINT);
            return t;
        }
    }
    LOG_WARN("World art '%s' not found", name.c_str());
    return Texture2D{};
}

// Ordered-dither vertical gradient so the sky matches the pixel art instead of a smooth blend.
static Texture2D MakeSky(Color top, Color bottom) {
    static const int bayer[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
    const int levels = 8;
    Image img = GenImageColor(LAYER_W, LAYER_H, BLACK);
    for (int y = 0; y < LAYER_H; y++) {
        float t = (float)y / (LAYER_H - 1) * (levels - 1);
        int base = (int)t;
        float frac = t - base;
        for (int x = 0; x < LAYER_W; x++) {
            int k = base + ((frac * 16.f > bayer[y % 4][x % 4]) ? 1 : 0);
            if (k > levels - 1) k = levels - 1;
            float m = (float)k / (levels - 1);
            Color c{(unsigned char)(top.r + (bottom.r - top.r) * m), (unsigned char)(top.g + (bottom.g - top.g) * m),
                    (unsigned char)(top.b + (bottom.b - top.b) * m), 255};
            ImageDrawPixel(&img, x, y, c);
        }
    }
    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    return tex;
}

void Load(Assets &a, const std::vector<Theme> &themes) {
    a.tiles = LoadPixelTexture("tiles.png");
    a.items = LoadPixelTexture("items.png");
    for (int i = 0; i < BIOME_COUNT; i++) a.biome[i] = LoadPixelTexture("bg_" + std::to_string(i) + ".png");
    for (auto &t : themes) a.skies.push_back(MakeSky(t.bgTop, t.bgBottom));
    a.loaded = a.tiles.id > 0;
}

void Unload(Assets &a) {
    if (a.tiles.id > 0) UnloadTexture(a.tiles);
    if (a.items.id > 0) UnloadTexture(a.items);
    for (auto &t : a.biome)
        if (t.id > 0) UnloadTexture(t);
    for (auto &t : a.skies) UnloadTexture(t);
    a.skies.clear();
    a.loaded = false;
}

void DrawPlatform(const Assets &a, Rectangle rect, int row, Color tint) {
    if (a.tiles.id == 0) return;
    const float s = PIXEL_SCALE;
    const float sy = (float)(row * TILE_H);
    const float dh = TILE_H * s;
    // Snap to the art grid so tiles never shimmer between pixels.
    float x = std::floor(rect.x / s) * s;
    float y = std::floor(rect.y / s) * s;
    float w = std::floor(rect.width / s) * s;
    float capW = CAP * s;
    DrawTexturePro(a.tiles, {0, sy, (float)CAP, (float)TILE_H}, {x, y, capW, dh}, {0, 0}, 0.f, tint);
    float midEnd = x + w - capW;
    int piece = 0;
    for (float mx = x + capW; mx < midEnd; mx += MID * s, piece++) {
        float pw = std::fmin(MID * s, midEnd - mx);
        float srcX = (float)(CAP + ((piece % 3 == 2) ? MID : 0));
        DrawTexturePro(a.tiles, {srcX, sy, pw / s, (float)TILE_H}, {mx, y, pw, dh}, {0, 0}, 0.f, tint);
    }
    DrawTexturePro(a.tiles, {(float)(CAP + 2 * MID), sy, (float)CAP, (float)TILE_H}, {midEnd, y, capW, dh}, {0, 0},
                   0.f, tint);
}

static void DrawItemCell(const Assets &a, int cell, Vector2 center, bool flip) {
    const float size = 16.f * PIXEL_SCALE;
    float x = std::floor((center.x - size / 2.f) / PIXEL_SCALE) * PIXEL_SCALE;
    float y = std::floor((center.y - size / 2.f) / PIXEL_SCALE) * PIXEL_SCALE;
    Rectangle src{(float)(cell * 16), 0, flip ? -16.f : 16.f, 16.f};
    DrawTexturePro(a.items, src, {x, y, size, size}, {0, 0}, 0.f, WHITE);
}

void DrawCoin(const Assets &a, Vector2 center, float animTime) {
    // Ping-pong through the spin frames; mirror on the way back so it reads as a full turn.
    static const int seq[6] = {0, 1, 2, 3, 2, 1};
    int step = (int)(animTime * 10.f) % 6;
    center.y += std::round(std::sin(animTime * 4.f) * 1.5f) * 2.f;
    DrawItemCell(a, seq[step], center, step > 3);
}

void DrawPowerUp(const Assets &a, Vector2 center, int type, float animTime) {
    center.y += std::round(std::sin(animTime * 3.f) * 1.5f) * 2.f;
    // Blinking pixel sparkles orbiting the orb
    for (int i = 0; i < 3; i++) {
        float ang = animTime * 2.f + i * 2.094f;
        float px = center.x + std::cos(ang) * 22.f;
        float py = center.y + std::sin(ang) * 22.f;
        unsigned char al = (unsigned char)(140 + 110 * std::sin(animTime * 6.f + i));
        PixelDot(px, py, 1.f, {255, 255, 255, al});
    }
    DrawItemCell(a, 4 + type, center, false);
}

void DrawSky(const Assets &a, int themeIndex, int w, int h, Color tint) {
    if (themeIndex < 0 || themeIndex >= (int)a.skies.size()) return;
    const Texture2D &t = a.skies[themeIndex];
    DrawTexturePro(t, {0, 0, (float)t.width, (float)t.height}, {0, 0, (float)w, (float)h}, {0, 0}, 0.f, tint);
}

static void DrawScrollingLayer(const Texture2D &t, int layer, float scroll, int w, int h, Color tint) {
    const float dh = (float)h;
    float off = std::fmod(scroll, dh);
    if (off < 0) off += dh;
    off = std::floor(off / PIXEL_SCALE) * PIXEL_SCALE;
    Rectangle src{(float)(layer * LAYER_W), 0, (float)LAYER_W, (float)LAYER_H};
    DrawTexturePro(t, src, {0, off, (float)w, dh}, {0, 0}, 0.f, tint);
    DrawTexturePro(t, src, {0, off - dh, (float)w, dh}, {0, 0}, 0.f, tint);
}

void DrawBiomeLayers(const Assets &a, int biome, float cameraY, int w, int h, Color tint) {
    if (biome < 0 || biome >= BIOME_COUNT) biome = 0;
    const Texture2D &t = a.biome[biome];
    if (t.id == 0) return;
    // Sky decoration drifts very slightly so moons/planets feel far away.
    DrawScrollingLayer(t, 0, -cameraY * 0.01f, w, h, tint);
    DrawScrollingLayer(t, 1, -cameraY * 0.08f, w, h, tint);
    DrawScrollingLayer(t, 2, -cameraY * 0.3f, w, h, tint);
}

} // namespace WorldArt
