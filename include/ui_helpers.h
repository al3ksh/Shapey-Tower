#pragma once
#include "raylib.h"
#include "ui_kit.h"
#include <cmath>
#include <cstdio>

// Shared settings widgets for the main menu and the pause screen (pixel-art styled via ui_kit).
namespace Ui {

inline int RowX(float uiCenterX, int boxW, int sw, float scale) {
    int margin = (int)(10 * scale);
    int boxX = (int)(uiCenterX - boxW / 2);
    if (boxX < margin) boxX = margin;
    if (boxX + boxW > sw - margin) boxX = sw - margin - boxW;
    return boxX;
}

inline bool DrawTabButton(float x, int y, int w, int h, const char* label, int tabIndex, int activeTab, Vector2 mPos, bool click, float scale) {
    int p = UnitFor(scale);
    Rectangle rect{x, (float)y, (float)w, (float)h};
    bool selected = (activeTab == tabIndex);
    bool hovered = CheckCollisionPointRec(mPos, rect);
    Color fill = selected ? Color{58, 92, 172, 255} : (hovered ? Color{50, 58, 90, 255} : Color{32, 36, 58, 255});
    Color light = selected ? Color{122, 170, 250, 255} : Color{70, 80, 120, 255};
    Frame(rect, p, fill, light, {18, 20, 38, 255}, {6, 6, 14, 255});
    int k = FitK(label, TextKFor(scale), w - 4 * p);
    int tw = Measure(label, k);
    Text(label, x + w / 2 - tw / 2, y + h / 2 - GlyphHeight(k) / 2, k, selected ? WHITE : Color{170, 176, 200, 255});
    return click && hovered;
}

inline void DrawSectionHeader(int &y, float uiCenterX, const char* text, float scale) {
    int p = UnitFor(scale);
    int k = TextKFor(scale) + 1;
    int tw = Measure(text, k);
    TextOutlined(text, uiCenterX - tw / 2, (float)y, k, {255, 214, 110, 255});
    y += GlyphHeight(k) + 4 * p;
    Divider(uiCenterX, (float)(y - 2 * p), (float)(tw / 2 + 10 * p), p);
    y += 2 * p;
}

inline void DrawToggle(int &y, float uiCenterX, const char* label, bool &value, Vector2 mPos, bool click, bool &changed, int sw, float scale) {
    int p = UnitFor(scale);
    int boxW = (int)(300 * scale), boxH = (int)(34 * scale);
    int boxX = RowX(uiCenterX, boxW, sw, scale);
    Rectangle rect{(float)boxX, (float)y, (float)boxW, (float)boxH};
    bool hovered = CheckCollisionPointRec(mPos, rect);
    Frame(rect, p, hovered ? Color{40, 46, 76, 255} : Color{30, 34, 56, 255}, {56, 64, 100, 255}, {16, 18, 34, 255}, {6, 6, 14, 255});

    int swW = 16 * p, swH = 8 * p;
    int k = FitK(label, TextKFor(scale), boxW - swW - 10 * p);
    Text(label, (float)(boxX + 4 * p), (float)(y + boxH / 2 - GlyphHeight(k) / 2), k, RAYWHITE);
    Switch({(float)(boxX + boxW - swW - 3 * p), (float)(y + boxH / 2 - swH / 2), (float)swW, (float)swH}, p, value);

    if (click && hovered) { value = !value; changed = true; }
    y += boxH + 3 * p;
}

inline void DrawSlider(int &y, float uiCenterX, const char* label, float &value, Vector2 mPos, bool drag, bool &changed, int sw, float scale) {
    int p = UnitFor(scale);
    int boxW = (int)(300 * scale);
    int boxX = RowX(uiCenterX, boxW, sw, scale);
    int k = TextKFor(scale);
    int ks = k > 1 ? k - 1 : 1;

    Text(label, (float)boxX, (float)y, ks, {176, 184, 212, 255});
    char pctText[16];
    snprintf(pctText, sizeof(pctText), "%d%%", (int)(value * 100 + 0.5f));
    int pw = Measure(pctText, ks);
    Text(pctText, (float)(boxX + boxW - pw), (float)y, ks, {255, 214, 110, 255});
    y += GlyphHeight(ks) + 2 * p;

    int trackH = 6 * p;
    Rectangle track{(float)boxX, (float)y, (float)boxW, (float)trackH};
    Bar(track, p, value, {86, 150, 240, 255});
    // Knob: a small raised block riding on the track
    int knobW = 4 * p, knobH = 10 * p;
    float kx = std::floor((boxX + p + (boxW - 2 * p - knobW) * value) / p) * p;
    Frame({kx, (float)(y - 2 * p), (float)knobW, (float)knobH}, p, {226, 230, 244, 255}, WHITE, {140, 146, 170, 255}, {6, 6, 14, 255});

    Rectangle hit{(float)(boxX - 2 * p), (float)(y - 4 * p), (float)(boxW + 4 * p), (float)(trackH + 8 * p)};
    if (drag && CheckCollisionPointRec(mPos, hit)) {
        float newVal = (mPos.x - boxX) / (float)boxW;
        if (newVal < 0) newVal = 0;
        if (newVal > 1) newVal = 1;
        if (std::fabs(newVal - value) > 0.001f) { value = newVal; changed = true; }
    }
    y += trackH + 6 * p;
}

// One rebindable key row. Returns true when clicked (caller toggles the listening state).
inline bool DrawKeyRow(int &y, float uiCenterX, const char* label, const char* keyName, bool listening, float blink, Vector2 mPos, bool click, int sw, float scale) {
    int p = UnitFor(scale);
    int boxW = (int)(300 * scale), boxH = (int)(38 * scale);
    int boxX = RowX(uiCenterX, boxW, sw, scale);
    Rectangle rect{(float)boxX, (float)y, (float)boxW, (float)boxH};
    bool hovered = CheckCollisionPointRec(mPos, rect);
    Color fill = listening ? Color{(unsigned char)(60 + 30 * blink), 44, 96, 255} : (hovered ? Color{40, 46, 76, 255} : Color{30, 34, 56, 255});
    Frame(rect, p, fill, {56, 64, 100, 255}, {16, 18, 34, 255}, listening ? Color{180, 140, 255, 255} : Color{6, 6, 14, 255});
    int k = TextKFor(scale);
    Text(label, (float)(boxX + 4 * p), (float)(y + boxH / 2 - GlyphHeight(k) / 2), k, RAYWHITE);

    const char* kn = listening ? "..." : keyName;
    int knw = Measure(kn, k);
    int capW = knw + 8 * p, capH = GlyphHeight(k) + 6 * p;
    Rectangle cap{(float)(boxX + boxW - capW - 3 * p), (float)(y + boxH / 2 - capH / 2), (float)capW, (float)capH};
    Frame(cap, p, {214, 218, 232, 255}, WHITE, {130, 136, 160, 255}, {6, 6, 14, 255});
    Text(kn, cap.x + 4 * p, cap.y + 2 * p, k, listening ? Color{150, 90, 20, 255} : Color{24, 28, 48, 255}, false);
    y += boxH + 3 * p;
    return click && hovered;
}

}
