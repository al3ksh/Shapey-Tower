#include "ui_kit.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

namespace Ui {

// ---------------------------------------------------------------------------------------------
// Bitmap data. Glyphs are 5x7 (narrower ones are trimmed); lowercase renders as uppercase.
// ---------------------------------------------------------------------------------------------
struct GlyphDef { char c; const char *rows[7]; };

static const GlyphDef kGlyphs[] = {
    {'A', {".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}},
    {'B', {"####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."}},
    {'C', {".###.", "#...#", "#....", "#....", "#....", "#...#", ".###."}},
    {'D', {"####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."}},
    {'E', {"#####", "#....", "#....", "####.", "#....", "#....", "#####"}},
    {'F', {"#####", "#....", "#....", "####.", "#....", "#....", "#...."}},
    {'G', {".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".####"}},
    {'H', {"#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}},
    {'I', {"###", ".#.", ".#.", ".#.", ".#.", ".#.", "###"}},
    {'J', {"..###", "...#.", "...#.", "...#.", "...#.", "#..#.", ".##.."}},
    {'K', {"#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"}},
    {'L', {"#....", "#....", "#....", "#....", "#....", "#....", "#####"}},
    {'M', {"#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#"}},
    {'N', {"#...#", "#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#"}},
    {'O', {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}},
    {'P', {"####.", "#...#", "#...#", "####.", "#....", "#....", "#...."}},
    {'Q', {".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#"}},
    {'R', {"####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"}},
    {'S', {".####", "#....", "#....", ".###.", "....#", "....#", "####."}},
    {'T', {"#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."}},
    {'U', {"#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}},
    {'V', {"#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.."}},
    {'W', {"#...#", "#...#", "#...#", "#.#.#", "#.#.#", "#.#.#", ".#.#."}},
    {'X', {"#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"}},
    {'Y', {"#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."}},
    {'Z', {"#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"}},
    {'0', {".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###."}},
    {'1', {".#.", "##.", ".#.", ".#.", ".#.", ".#.", "###"}},
    {'2', {".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"}},
    {'3', {"####.", "....#", "....#", ".###.", "....#", "....#", "####."}},
    {'4', {"...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."}},
    {'5', {"#####", "#....", "####.", "....#", "....#", "#...#", ".###."}},
    {'6', {"..##.", ".#...", "#....", "####.", "#...#", "#...#", ".###."}},
    {'7', {"#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."}},
    {'8', {".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."}},
    {'9', {".###.", "#...#", "#...#", ".####", "....#", "...#.", ".##.."}},
    {'!', {"#", "#", "#", "#", "#", ".", "#"}},
    {'"', {"#.#", "#.#", "...", "...", "...", "...", "..."}},
    {'#', {".#.#.", ".#.#.", "#####", ".#.#.", "#####", ".#.#.", ".#.#."}},
    {'$', {"..#..", ".####", "#.#..", ".###.", "..#.#", "####.", "..#.."}},
    {'%', {"##..#", "##..#", "...#.", "..#..", ".#...", "#..##", "#..##"}},
    {'&', {".##..", "#..#.", "#.#..", ".#...", "#.#.#", "#..#.", ".##.#"}},
    {'\'', {"#", "#", ".", ".", ".", ".", "."}},
    {'(', {".#", "#.", "#.", "#.", "#.", "#.", ".#"}},
    {')', {"#.", ".#", ".#", ".#", ".#", ".#", "#."}},
    {'*', {".....", "#.#.#", ".###.", "#####", ".###.", "#.#.#", "....."}},
    {'+', {".....", "..#..", "..#..", "#####", "..#..", "..#..", "....."}},
    {',', {"..", "..", "..", "..", "..", ".#", "#."}},
    {'-', {"....", "....", "....", "####", "....", "....", "...."}},
    {'.', {".", ".", ".", ".", ".", ".", "#"}},
    {'/', {"....#", "....#", "...#.", "..#..", ".#...", "#....", "#...."}},
    {':', {".", "#", ".", ".", ".", "#", "."}},
    {';', {"..", ".#", "..", "..", "..", ".#", "#."}},
    {'<', {"...#", "..#.", ".#..", "#...", ".#..", "..#.", "...#"}},
    {'=', {"....", "....", "####", "....", "####", "....", "...."}},
    {'>', {"#...", ".#..", "..#.", "...#", "..#.", ".#..", "#..."}},
    {'?', {".###.", "#...#", "....#", "...#.", "..#..", ".....", "..#.."}},
    {'@', {".###.", "#...#", "#.###", "#.#.#", "#.###", "#....", ".###."}},
    {'[', {"###", "#..", "#..", "#..", "#..", "#..", "###"}},
    {'\\', {"#....", "#....", ".#...", "..#..", "...#.", "....#", "....#"}},
    {']', {"###", "..#", "..#", "..#", "..#", "..#", "###"}},
    {'^', {"..#..", ".#.#.", "#...#", ".....", ".....", ".....", "....."}},
    {'_', {".....", ".....", ".....", ".....", ".....", ".....", "#####"}},
    {'`', {"#.", ".#", "..", "..", "..", "..", ".."}},
    {'{', {".##", ".#.", ".#.", "#..", ".#.", ".#.", ".##"}},
    {'|', {"#", "#", "#", "#", "#", "#", "#"}},
    {'}', {"##.", ".#.", ".#.", "..#", ".#.", ".#.", "##."}},
    {'~', {".....", ".....", ".#...", "#.#.#", "...#.", ".....", "....."}},
};

struct IconDef { const char *rows[12]; bool autoOutline; };

// K outline, W white, L light gray, D gray, Y gold, y dark gold, R red, O orange,
// B blue, b dark blue, G green, P purple, S skin/mask
static const IconDef kIcons[ICON_COUNT] = {
    /* PAD */ {{".KKKKKKKKK.", "KLLLLLLLLLK", "KLDLLLLLRLK", "KDDDLLLRLGK", "KLDLLLLLGLK", "KLLLKKKLLLK", ".KKK...KKK."}, false},
    /* MONITOR */ {{"KKKKKKKKKKK", "KBBBBBBBBBK", "KBWBBBBBBBK", "KBBBBBBBBBK", "KbBBBBBBBbK", "KKKKKKKKKKK", "....KLK....", "..KKKKKKK.."}, false},
    /* SPEAKER */ {{"....KK.....", "...KLK..W..", "KKKLLK...W.", "KLLLLK.W.W.", "KLLLLK.W.W.", "KKKLLK...W.", "...KLK..W..", "....KK....."}, false},
    /* KEY */ {{".KKKKKKK.", "KLLLLLLLK", "KLWWWWWLK", "KLWKWKWLK", "KLWKKKWLK", "KLWKWKWLK", "KDLLLLLDK", "KDDDDDDDK", ".KKKKKKK."}, false},
    /* SPARK */ {{"....Y....", "....Y....", "...YWY...", "YYYWWWYYY", "...YWY...", "....Y....", "....Y...."}, true},
    /* TROPHY */ {{".KKKKKKKKK.", "KKYYYYYYYKK", "KYKYWYYYKYK", "KYKYWYYYKYK", ".KKYYYYyKK.", "..KyYYYyK..", "...KyYyK...", "....KyK....", "...KKyKK...", "..KyyyyyK..", "..KKKKKKK.."}, false},
    /* HOOD */ {{"..KKKKK..", ".KDDDDDK.", "KDDSSSDDK", "KDSKSKSDK", "KDSSSSSDK", "KDDSSSDDK", ".KDDDDDK.", "KDDDDDDDK", "KKKKKKKKK"}, false},
    /* STAR */ {{"....Y....", "...YYY...", "YYYYWYYYY", ".YYWWWYY.", "..YYYYY..", ".YYY.YYY.", ".YY...YY."}, true},
    /* FLAME */ {{"...R...", "..RR...", "..ROR..", ".ROORR.", ".ROYOR.", "RROYYOR", "ROYYYOR", "ROYWYOR", ".ROOOR."}, true},
    /* CROWN */ {{"K....K....K", "KY..KYK..YK", "KYYKYYYKYYK", "KYYYYRYYYYK", "KYYYYYYYYYK", "KyyyyyyyyyK", "KKKKKKKKKKK"}, false},
    /* LOCK */ {{"..KKK..", ".K...K.", ".K...K.", "KKKKKKK", "KYYYYYK", "KYYKYYK", "KYYKYYK", "KyyyyyK", "KKKKKKK"}, false},
    /* CALENDAR */ {{"KKKKKKKKK", "KRRRRRRRK", "KKKKKKKKK", "KWWWWWWWK", "KWKWKWKWK", "KWWWWWWWK", "KWKWKWKWK", "KWWWWWWWK", "KKKKKKKKK"}, false},
    /* SKULL */ {{".KKKKKKK.", "KWWWWWWWK", "KWWWWWWWK", "KWKKWKKWK", "KWKKWKKWK", "KWWWKWWWK", ".KWWWWWK.", "..KWKWK..", "..KKKKK.."}, false},
    /* ARROW_L */ {{"...W", "..WW", ".WWW", "WWWW", ".WWW", "..WW", "...W"}, true},
    /* ARROW_R */ {{"W...", "WW..", "WWW.", "WWWW", "WWW.", "WW..", "W..."}, true},
    /* CHECK */ {{"......W", ".....WW", "W...WW.", "WW.WW..", ".WWW...", "..W...."}, true},
    /* BOLT */ {{"...YYY.", "..YYY..", ".YYY...", "YYYYYY.", "...YY..", "..YY...", ".YY....", ".Y....."}, true},
    /* GLOBE */ {{"..KKKKK..", ".KBBGBBK.", "KBGGGBBBK", "KBBGGBGBK", "KBBBBGGBK", "KGBBBGBBK", "KBBBBBBBK", ".KBBBBBK.", "..KKKKK.."}, false},
};

static Color PaletteColor(char ch) {
    switch (ch) {
        case 'K': return {14, 12, 24, 255};
        case 'W': return {250, 250, 255, 255};
        case 'L': return {190, 196, 214, 255};
        case 'D': return {104, 110, 138, 255};
        case 'Y': return {255, 206, 72, 255};
        case 'y': return {196, 128, 40, 255};
        case 'R': return {226, 62, 58, 255};
        case 'O': return {255, 146, 48, 255};
        case 'B': return {70, 132, 230, 255};
        case 'b': return {40, 70, 150, 255};
        case 'G': return {96, 204, 110, 255};
        case 'P': return {166, 104, 232, 255};
        case 'S': return {236, 226, 206, 255};
        default: return {0, 0, 0, 0};
    }
}

struct AtlasRect { int x, y, w, h; };
static Texture2D gAtlas{};
static AtlasRect gGlyph[128]{};
static AtlasRect gIcon[ICON_COUNT]{};
static int gUnit = 2;
static constexpr int GLYPH_H = 7;
static constexpr int SPACE_W = 3;

void SetUnit(int p) { gUnit = p < 1 ? 1 : p; }
int Unit() { return gUnit; }

void Init() {
    if (gAtlas.id > 0) return;
    const int atlasW = 256, atlasH = 64;
    Image img = GenImageColor(atlasW, atlasH, BLANK);
    int cx = 0, cy = 0;
    for (const auto &g : kGlyphs) {
        int w = (int)std::strlen(g.rows[0]);
        if (cx + w + 1 > atlasW) { cx = 0; cy += GLYPH_H + 1; }
        for (int y = 0; y < GLYPH_H; y++)
            for (int x = 0; x < w; x++)
                if (g.rows[y][x] == '#') ImageDrawPixel(&img, cx + x, cy + y, WHITE);
        gGlyph[(unsigned char)g.c] = {cx, cy, w, GLYPH_H};
        cx += w + 1;
    }
    cx = 0;
    cy += GLYPH_H + 2;
    for (int i = 0; i < ICON_COUNT; i++) {
        const IconDef &d = kIcons[i];
        int h = 0;
        while (h < 12 && d.rows[h]) h++;
        int w = (int)std::strlen(d.rows[0]);
        int pad = d.autoOutline ? 1 : 0;
        int cw = w + pad * 2, ch = h + pad * 2;
        if (cx + cw + 1 > atlasW) { cx = 0; cy += 16; }
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                Color c = PaletteColor(d.rows[y][x]);
                if (c.a) ImageDrawPixel(&img, cx + pad + x, cy + pad + y, c);
            }
        if (d.autoOutline) {
            // 1px dark outline around opaque pixels (drawn into empty neighbours only)
            Color k = PaletteColor('K');
            for (int y = 0; y < ch; y++)
                for (int x = 0; x < cw; x++) {
                    if (GetImageColor(img, cx + x, cy + y).a) continue;
                    bool near = false;
                    for (int dy = -1; dy <= 1 && !near; dy++)
                        for (int dx = -1; dx <= 1 && !near; dx++) {
                            if (dx && dy) continue;
                            int sx = x + dx - pad, sy = y + dy - pad;
                            if (sx >= 0 && sy >= 0 && sx < w && sy < h && d.rows[sy][sx] != '.') near = true;
                        }
                    if (near) ImageDrawPixel(&img, cx + x, cy + y, k);
                }
        }
        gIcon[i] = {cx, cy, cw, ch};
        cx += cw + 1;
    }
    gAtlas = LoadTextureFromImage(img);
    SetTextureFilter(gAtlas, TEXTURE_FILTER_POINT);
    UnloadImage(img);
}

void Shutdown() {
    if (gAtlas.id > 0) UnloadTexture(gAtlas);
    gAtlas = Texture2D{};
}

static const AtlasRect *GlyphFor(char ch) {
    unsigned char c = (unsigned char)std::toupper((unsigned char)ch);
    if (c >= 128 || gGlyph[c].w == 0) return nullptr;
    return &gGlyph[c];
}

int GlyphHeight(int k) { return GLYPH_H * k; }

int Measure(const char *text, int k) {
    if (!text) return 0;
    int w = 0, best = 0;
    for (const char *p = text; *p; ++p) {
        if (*p == '\n') { if (w > best) best = w; w = 0; continue; }
        const AtlasRect *g = GlyphFor(*p);
        w += ((g ? g->w : SPACE_W) + 1) * k;
    }
    if (w > best) best = w;
    return best > 0 ? best - k : 0;
}

int FitK(const char *text, int k, int maxWidth) {
    while (k > 1 && Measure(text, k) > maxWidth) k--;
    return k;
}

static void RawText(const char *text, float x, float y, int k, Color c) {
    if (!text || gAtlas.id == 0) return;
    float px = std::floor(x), py = std::floor(y);
    float startX = px;
    for (const char *p = text; *p; ++p) {
        if (*p == '\n') { px = startX; py += (GLYPH_H + 3) * k; continue; }
        const AtlasRect *g = GlyphFor(*p);
        if (!g) { px += (SPACE_W + 1) * k; continue; }
        DrawTexturePro(gAtlas, {(float)g->x, (float)g->y, (float)g->w, (float)g->h},
                       {px, py, (float)(g->w * k), (float)(g->h * k)}, {0, 0}, 0.f, c);
        px += (g->w + 1) * k;
    }
}

void Text(const char *text, float x, float y, int k, Color c, bool shadow) {
    if (shadow) RawText(text, x, y + k, k, {10, 8, 18, (unsigned char)(c.a * 0.7f)});
    RawText(text, x, y, k, c);
}

void TextOutlined(const char *text, float x, float y, int k, Color c, Color outline) {
    outline.a = (unsigned char)(outline.a * (c.a / 255.f));
    // Big text keeps a thin outline instead of one that grows with the glyph scale
    int o = k < 3 ? k : 3;
    RawText(text, x, y + 2 * o, k, outline);
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (dx || dy) RawText(text, x + dx * o, y + dy * o, k, outline);
    RawText(text, x, y, k, c);
}

void TextCentered(const char *text, float cx, float y, int k, Color c, bool shadow) {
    Text(text, std::floor(cx - Measure(text, k) / 2.f), y, k, c, shadow);
}

void FancyText(const char *text, float x, float y, int k, Color top, Color bottom, Color outline, float wave, float time) {
    if (!text) return;
    float px = std::floor(x);
    int i = 0;
    Color deep{(unsigned char)(outline.r / 2 + 20), (unsigned char)(outline.g / 2), (unsigned char)(outline.b / 2 + 30), outline.a};
    for (const char *p = text; *p; ++p, ++i) {
        char s[2] = {*p, 0};
        const AtlasRect *g = GlyphFor(*p);
        if (!g) { px += (SPACE_W + 1) * k; continue; }
        float off = wave > 0.f ? std::round(std::sin(time * 3.f + i * 0.55f) * wave) * k : 0.f;
        float py = std::floor(y) + off;
        // Deep drop shadow, outline ring, then two-tone fill split at the glyph's middle row
        for (int d = 1; d <= 2; d++) RawText(s, px + d * k, py + (d + 1) * k, k, deep);
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
                if (dx || dy) RawText(s, px + dx * k, py + dy * k, k, outline);
        RawText(s, px, py, k, bottom);
        DrawTexturePro(gAtlas, {(float)g->x, (float)g->y, (float)g->w, 4.f},
                       {px, py, (float)(g->w * k), (float)(4 * k)}, {0, 0}, 0.f, top);
        // specular pixel on the top-left of each letter
        DrawTexturePro(gAtlas, {(float)g->x, (float)g->y, 1.f, 1.f}, {px, py, (float)k, (float)k}, {0, 0}, 0.f,
                       {255, 255, 255, (unsigned char)(top.a * 0.8f)});
        px += (g->w + 2) * k;
    }
}

int IconW(Icon icon) { return gIcon[icon].w; }
int IconH(Icon icon) { return gIcon[icon].h; }

void DrawIcon(Icon icon, float x, float y, int p, Color tint) {
    const AtlasRect &r = gIcon[icon];
    DrawTexturePro(gAtlas, {(float)r.x, (float)r.y, (float)r.w, (float)r.h},
                   {std::floor(x), std::floor(y), (float)(r.w * p), (float)(r.h * p)}, {0, 0}, 0.f, tint);
}

void DrawIconCentered(Icon icon, float cx, float cy, int p, Color tint) {
    const AtlasRect &r = gIcon[icon];
    DrawIcon(icon, cx - r.w * p / 2.f, cy - r.h * p / 2.f, p, tint);
}

// ---------------------------------------------------------------------------------------------
// Frames and widgets
// ---------------------------------------------------------------------------------------------
static Rectangle Snap(Rectangle r, int p) {
    float x = std::floor(r.x), y = std::floor(r.y);
    float w = std::floor(r.width / p) * p, h = std::floor(r.height / p) * p;
    return {x, y, w, h};
}

void Frame(Rectangle r, int p, Color fill, Color light, Color dark, Color outline) {
    r = Snap(r, p);
    float x = r.x, y = r.y, w = r.width, h = r.height;
    if (w < 4 * p || h < 4 * p) { DrawRectangleRec(r, fill); return; }
    // notched outline
    DrawRectangle((int)(x + p), (int)y, (int)(w - 2 * p), p, outline);
    DrawRectangle((int)(x + p), (int)(y + h - p), (int)(w - 2 * p), p, outline);
    DrawRectangle((int)x, (int)(y + p), p, (int)(h - 2 * p), outline);
    DrawRectangle((int)(x + w - p), (int)(y + p), p, (int)(h - 2 * p), outline);
    // body and bevel
    DrawRectangle((int)(x + p), (int)(y + p), (int)(w - 2 * p), (int)(h - 2 * p), fill);
    DrawRectangle((int)(x + p), (int)(y + p), (int)(w - 2 * p), p, light);
    DrawRectangle((int)(x + p), (int)(y + 2 * p), p, (int)(h - 4 * p), light);
    DrawRectangle((int)(x + p), (int)(y + h - 2 * p), (int)(w - 2 * p), p, dark);
    DrawRectangle((int)(x + w - 2 * p), (int)(y + 2 * p), p, (int)(h - 4 * p), dark);
}

void Panel(Rectangle r, int p, bool studs, unsigned char alpha) {
    Frame(r, p, {24, 28, 48, alpha}, {62, 72, 112, alpha}, {13, 15, 30, alpha}, {6, 6, 14, alpha});
    r = Snap(r, p);
    // thin inner rule for depth
    Color rule{40, 46, 76, alpha};
    DrawRectangleLinesEx({r.x + 3 * p, r.y + 3 * p, r.width - 6 * p, r.height - 6 * p}, (float)1, rule);
    if (studs && r.width > 12 * p && r.height > 12 * p) {
        Color gold{232, 184, 72, alpha}, goldDark{150, 96, 34, alpha};
        float pts[4][2] = {{r.x + 2 * p, r.y + 2 * p}, {r.x + r.width - 4 * p, r.y + 2 * p},
                           {r.x + 2 * p, r.y + r.height - 4 * p}, {r.x + r.width - 4 * p, r.y + r.height - 4 * p}};
        for (auto &pt : pts) {
            DrawRectangle((int)pt[0], (int)pt[1], 2 * p, 2 * p, goldDark);
            DrawRectangle((int)pt[0], (int)pt[1], p, p, gold);
        }
    }
}

void Inset(Rectangle r, int p, Color fill) {
    r = Snap(r, p);
    DrawRectangleRec(r, fill);
    DrawRectangle((int)r.x, (int)r.y, (int)r.width, p, {4, 4, 10, fill.a});
    DrawRectangle((int)r.x, (int)r.y, p, (int)r.height, {4, 4, 10, fill.a});
    DrawRectangle((int)r.x, (int)(r.y + r.height - p), (int)r.width, p, {60, 68, 104, (unsigned char)(fill.a * 0.6f)});
}

struct Ramp { Color base, light, dark, text; };
static Ramp RampFor(Style s, bool hover) {
    Ramp r;
    switch (s) {
        case STYLE_GREEN: r = {{62, 156, 78, 255}, {130, 226, 120, 255}, {30, 90, 52, 255}, WHITE}; break;
        case STYLE_RED: r = {{178, 56, 60, 255}, {240, 116, 100, 255}, {100, 26, 40, 255}, WHITE}; break;
        case STYLE_GOLD: r = {{212, 150, 44, 255}, {255, 222, 110, 255}, {124, 74, 24, 255}, {40, 22, 8, 255}}; break;
        case STYLE_DARK: r = {{36, 42, 66, 255}, {70, 80, 120, 255}, {18, 20, 38, 255}, {200, 206, 228, 255}}; break;
        default: r = {{58, 92, 172, 255}, {122, 170, 250, 255}, {30, 44, 102, 255}, WHITE}; break;
    }
    if (hover) {
        auto up = [](Color c, int d) {
            return Color{(unsigned char)std::min(255, c.r + d), (unsigned char)std::min(255, c.g + d),
                         (unsigned char)std::min(255, c.b + d), c.a};
        };
        r.base = up(r.base, 26);
        r.light = up(r.light, 20);
    }
    return r;
}

bool Button(Rectangle r, const char *label, Vector2 mouse, bool click, Style style, int k, bool enabled) {
    int p = gUnit;
    bool hover = enabled && CheckCollisionPointRec(mouse, r);
    bool held = hover && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    Ramp c = RampFor(enabled ? style : STYLE_DARK, hover);
    if (!enabled) { c.text = {110, 114, 134, 255}; }
    Rectangle body = r;
    // 3D lip below the button; pressing sinks the face into it
    Rectangle lip{r.x, r.y + p, r.width, r.height};
    if (!held) Frame(lip, p, c.dark, c.dark, c.dark, {6, 6, 14, 255});
    else body.y += p;
    Frame(body, p, c.base, c.light, c.dark, {6, 6, 14, 255});
    if (k <= 0) k = TextKFor(p / 2.f) + (r.height >= 25 * p ? 1 : 0);
    k = FitK(label, k, (int)r.width - 6 * p);
    int tw = Measure(label, k);
    float ty = std::floor(body.y + (body.height - GLYPH_H * k) / 2.f);
    float tx = std::floor(body.x + (body.width - tw) / 2.f);
    Text(label, tx, ty, k, c.text, style != STYLE_GOLD || !enabled);
    if (hover && r.width > tw + 16 * p) {
        // bobbing selection arrows
        float bob = std::round(std::sin(GetTime() * 8.0) * 0.5f + 0.5f) * p;
        float cy = body.y + body.height / 2.f;
        DrawIconCentered(ICON_ARROW_R, tx - 5 * p - bob, cy, p / 2 > 0 ? p / 2 : 1, WHITE);
        DrawIconCentered(ICON_ARROW_L, tx + tw + 5 * p + bob, cy, p / 2 > 0 ? p / 2 : 1, WHITE);
    }
    return hover && click;
}

bool IconButton(Rectangle r, Icon icon, Vector2 mouse, bool click, bool selected, Style style) {
    int p = gUnit;
    bool hover = CheckCollisionPointRec(mouse, r);
    Ramp c = RampFor(selected ? STYLE_BLUE : style, hover);
    Rectangle body = r;
    if (!selected) body.y += p;
    Frame(body, p, c.base, c.light, c.dark, {6, 6, 14, 255});
    if (selected) {
        // gold tab marker under the active tab
        DrawRectangle((int)(r.x + 2 * p), (int)(r.y + r.height), (int)(r.width - 4 * p), p, {255, 206, 72, 255});
    }
    int ip = p;
    while (ip > 1 && (IconW(icon) * ip > r.width - 4 * p || IconH(icon) * ip > r.height - 4 * p)) ip--;
    DrawIconCentered(icon, body.x + body.width / 2.f, body.y + body.height / 2.f, ip,
                     selected || hover ? WHITE : Color{170, 176, 200, 255});
    return hover && click;
}

void Bar(Rectangle r, int p, float pct, Color fill, Color back) {
    if (pct < 0) pct = 0;
    if (pct > 1) pct = 1;
    r = Snap(r, p);
    Frame(r, p, back, back, back, {6, 6, 14, 255});
    float inner = r.width - 2 * p;
    float fw = std::floor(inner * pct / p) * p;
    if (fw > 0) {
        DrawRectangle((int)(r.x + p), (int)(r.y + p), (int)fw, (int)(r.height - 2 * p), fill);
        Color hi{(unsigned char)std::min(255, fill.r + 60), (unsigned char)std::min(255, fill.g + 60),
                 (unsigned char)std::min(255, fill.b + 60), fill.a};
        DrawRectangle((int)(r.x + p), (int)(r.y + p), (int)fw, p, hi);
    }
}

void Switch(Rectangle r, int p, bool on) {
    r = Snap(r, p);
    Color track = on ? Color{52, 150, 76, 255} : Color{46, 50, 72, 255};
    Frame(r, p, track, track, Color{(unsigned char)(track.r / 2), (unsigned char)(track.g / 2), (unsigned char)(track.b / 2), 255}, {6, 6, 14, 255});
    float kw = std::floor(r.height / p) * p - 2 * p;
    float kx = on ? r.x + r.width - p - kw : r.x + p;
    Frame({kx, r.y + p, kw, r.height - 2 * p}, p, {226, 230, 244, 255}, WHITE, {140, 146, 170, 255}, {6, 6, 14, 255});
}

void Divider(float cx, float y, float halfW, int p) {
    float yy = std::floor(y);
    DrawRectangle((int)(cx - halfW), (int)yy, (int)(halfW * 2), p, {50, 58, 92, 200});
    DrawRectangle((int)(cx - p * 1.5f), (int)(yy - p), 3 * p, 3 * p, {232, 184, 72, 255});
}

} // namespace Ui
