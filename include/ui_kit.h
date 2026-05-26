#pragma once
#include "raylib.h"

// Pixel-art UI toolkit: a baked 5x7 bitmap font, small icons and beveled frames drawn on
// an integer pixel grid so menus and the HUD match the 2x world art.
namespace Ui {

enum Icon {
    ICON_PAD, ICON_MONITOR, ICON_SPEAKER, ICON_KEY, ICON_SPARK, ICON_TROPHY, ICON_HOOD,
    ICON_STAR, ICON_FLAME, ICON_CROWN, ICON_LOCK, ICON_CALENDAR, ICON_SKULL,
    ICON_ARROW_L, ICON_ARROW_R, ICON_CHECK, ICON_BOLT, ICON_GLOBE, ICON_COUNT
};

enum Style { STYLE_BLUE, STYLE_GREEN, STYLE_RED, STYLE_GOLD, STYLE_DARK, STYLE_COUNT };

void Init();
void Shutdown();

// Pixel unit / text scale derived from a UI scale factor (1.0 = 480x800 game space).
inline int UnitFor(float scale) { int p = (int)(scale * 2.f + 0.5f); return p < 2 ? 2 : p; }
inline int TextKFor(float scale) { int k = (int)(scale * 1.5f + 0.5f); return k < 2 ? 2 : k; }
// Maps a raylib-style font size to an integer glyph scale (7px glyphs, ~same footprint).
inline int KFromSize(int size) { int k = (size + 5) / 10; return k < 1 ? 1 : k; }

// Current pixel unit used by helpers that don't take one explicitly (set per screen).
void SetUnit(int p);
int Unit();

int Measure(const char *text, int k);
int GlyphHeight(int k);
// Plain text with a 1-unit drop shadow.
void Text(const char *text, float x, float y, int k, Color c, bool shadow = true);
// Text with a full dark outline, readable over busy backgrounds.
void TextOutlined(const char *text, float x, float y, int k, Color c, Color outline = {12, 10, 20, 255});
void TextCentered(const char *text, float cx, float y, int k, Color c, bool shadow = true);
// Title treatment: outline, deep shadow, two-tone fill; wave > 0 bobs each letter.
void FancyText(const char *text, float x, float y, int k, Color top, Color bottom, Color outline, float wave, float time);
int FitK(const char *text, int k, int maxWidth);

// raylib-compatible drop-ins (size is a raylib font size).
inline int MeasureTextPx(const char *text, int size) { return Measure(text, KFromSize(size)); }
inline void DrawTextPx(const char *text, int x, int y, int size, Color c) { Text(text, (float)x, (float)y, KFromSize(size), c); }

void DrawIcon(Icon icon, float x, float y, int p, Color tint = WHITE);
void DrawIconCentered(Icon icon, float cx, float cy, int p, Color tint = WHITE);
int IconW(Icon icon);
int IconH(Icon icon);

// Beveled pixel frame with notched corners.
void Frame(Rectangle r, int p, Color fill, Color light, Color dark, Color outline);
void Panel(Rectangle r, int p, bool studs = true, unsigned char alpha = 235);
void Inset(Rectangle r, int p, Color fill = {12, 14, 26, 255});
// Button returning true on click; style picks the color ramp.
bool Button(Rectangle r, const char *label, Vector2 mouse, bool click, Style style = STYLE_BLUE, int k = 0, bool enabled = true);
bool IconButton(Rectangle r, Icon icon, Vector2 mouse, bool click, bool selected, Style style = STYLE_DARK);
void Bar(Rectangle r, int p, float pct, Color fill, Color back = {18, 20, 34, 255});
void Switch(Rectangle r, int p, bool on);
void Divider(float cx, float y, float halfW, int p);

} // namespace Ui
