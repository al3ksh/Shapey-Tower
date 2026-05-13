#pragma once
#include "raylib.h"
#include <cmath>

namespace Ui {

inline bool DrawTabButton(float x, int y, int w, int h, const char* label, int tabIndex, int activeTab, Vector2 mPos, bool click, float scale) {
    auto S = [scale](int v) { return (int)(v * scale); };
    Rectangle rect{x, (float)y, (float)w, (float)h};
    bool selected = (activeTab == tabIndex);
    bool hovered = CheckCollisionPointRec(mPos, rect);
    Color col = selected ? Color{60,120,180,255} : (hovered ? Color{50,70,100,255} : Color{35,45,60,255});
    DrawRectangleRec(rect, col);
    if(selected) DrawRectangle((int)x, y+h-S(3), w, S(3), Color{100,180,255,255});
    int fontSize = S(13);
    int tw = MeasureText(label, fontSize);
    DrawText(label, (int)(x + w/2 - tw/2), y + h/2 - fontSize/2, fontSize, selected ? WHITE : Color{180,180,180,255});
    return click && hovered;
}

inline void DrawSectionHeader(int &y, float uiCenterX, const char* text, float scale) {
    auto S = [scale](int v) { return (int)(v * scale); };
    int fontSize = S(20);
    int tw = MeasureText(text, fontSize);
    DrawText(text, (int)(uiCenterX - tw/2), y, fontSize, Color{100,180,255,255});
    y += S(28);
    DrawLine((int)(uiCenterX - S(100)), y-S(5), (int)(uiCenterX + S(100)), y-S(5), Color{60,80,120,180});
}

inline void DrawToggle(int &y, float uiCenterX, const char* label, bool &value, Vector2 mPos, bool click, bool &changed, int sw, float scale) {
    auto S = [scale](int v) { return (int)(v * scale); };
    int boxW = S(280), boxH = S(32);
    int boxX = (int)(uiCenterX - boxW/2);
    if(boxX < S(10)) boxX = S(10);
    if(boxX + boxW > sw - S(10)) boxX = sw - S(10) - boxW;

    Rectangle rect{(float)boxX, (float)y, (float)boxW, (float)boxH};
    bool hovered = CheckCollisionPointRec(mPos, rect);
    DrawRectangleRec(rect, hovered ? Color{50,60,80,255} : Color{40,50,65,255});
    DrawText(label, boxX + S(12), y + S(8), S(15), RAYWHITE);

    int toggleX = boxX + boxW - S(55);
    int toggleY = y + S(7);
    int toggleW = S(44), toggleH = S(18);
    DrawRectangle(toggleX, toggleY, toggleW, toggleH, value ? Color{60,160,80,255} : Color{80,80,80,255});
    DrawRectangleLines(toggleX, toggleY, toggleW, toggleH, Color{100,100,100,255});
    int knobX = value ? (toggleX + toggleW - S(16)) : (toggleX + S(2));
    DrawRectangle(knobX, toggleY + S(2), S(14), S(14), WHITE);

    if(click && hovered) { value = !value; changed = true; }
    y += boxH + S(6);
}

inline void DrawSlider(int &y, float uiCenterX, const char* label, float &value, Vector2 mPos, bool drag, bool &changed, int sw, float scale) {
    auto S = [scale](int v) { return (int)(v * scale); };
    int boxW = S(280), boxH = S(34);
    int boxX = (int)(uiCenterX - boxW/2);
    if(boxX < S(10)) boxX = S(10);
    if(boxX + boxW > sw - S(10)) boxX = sw - S(10) - boxW;

    DrawText(label, boxX, y, S(13), Color{180,180,180,255});
    y += S(16);

    int sliderX = boxX;
    int sliderW = boxW - S(45);
    int sliderH = S(6);
    int sliderY = y + S(5);

    DrawRectangle(sliderX, sliderY, sliderW, sliderH, Color{50,50,60,255});
    int fillW = (int)(sliderW * value);
    DrawRectangle(sliderX, sliderY, fillW, sliderH, Color{80,140,200,255});

    int knobR = S(8);
    int knobX = sliderX + fillW;
    DrawCircle(knobX, sliderY + sliderH/2, (float)knobR, Color{120,180,240,255});

    Rectangle sliderRect{(float)(sliderX - S(5)), (float)(sliderY - S(10)), (float)(sliderW + S(10)), scale*26.f};
    if(drag && CheckCollisionPointRec(mPos, sliderRect)) {
        float newVal = (mPos.x - sliderX) / sliderW;
        if(newVal < 0) newVal = 0; if(newVal > 1) newVal = 1;
        if(std::fabs(newVal - value) > 0.001f) { value = newVal; changed = true; }
    }

    char pctText[16];
    snprintf(pctText, sizeof(pctText), "%d%%", (int)(value * 100));
    DrawText(pctText, boxX + boxW - S(38), y + S(2), S(14), RAYWHITE);

    y += boxH;
}

inline void DrawKeyBind(int &y, float uiCenterX, const char* label, int &key, int activeTarget, int selfId, Vector2 mPos, bool click, float blinkAlpha, bool &activeChanged, float scale) {
    auto S = [scale](int v) { return (int)(v * scale); };

    auto keyNameStr = [](int k)->const char* {
        switch(k){
            case KEY_A: return "A"; case KEY_D: return "D"; case KEY_LEFT: return "LEFT"; case KEY_RIGHT: return "RIGHT";
            case KEY_SPACE: return "SPACE"; case KEY_W: return "W"; case KEY_S: return "S"; case KEY_UP: return "UP"; case KEY_DOWN: return "DOWN";
            case KEY_LEFT_SHIFT: return "LSHIFT"; case KEY_RIGHT_SHIFT: return "RSHIFT"; case KEY_ENTER: return "ENTER";
            case KEY_TAB: return "TAB"; case KEY_ESCAPE: return "ESC";
            default: return TextFormat("%d", k);
        }
    };

    int boxW = S(280), boxH = S(36);
    int boxX = (int)(uiCenterX - boxW/2);
    Rectangle rect{(float)boxX, (float)y, (float)boxW, (float)boxH};
    bool hovered = CheckCollisionPointRec(mPos, rect);
    bool isActive = (activeTarget == selfId);

    Color bgCol = isActive ? Color{80,60,100,255} : (hovered ? Color{50,60,80,255} : Color{40,50,65,255});
    if(isActive) bgCol.a = (unsigned char)(180 + 75 * blinkAlpha);
    DrawRectangleRec(rect, bgCol);
    DrawRectangleLines(boxX, y, boxW, boxH, isActive ? Color{180,140,255,255} : Color{80,80,80,255});

    int labelFont = S(15);
    DrawText(label, boxX + S(12), y + S(10), labelFont, RAYWHITE);

    int keyFont = S(16);
    const char* kn = isActive ? "..." : keyNameStr(key);
    int knw = MeasureText(kn, keyFont);
    DrawRectangle(boxX + boxW - knw - S(25), y + S(7), knw + S(16), S(22), Color{30,35,45,255});
    DrawText(kn, boxX + boxW - knw - S(17), y + S(10), keyFont, isActive ? Color{255,200,100,255} : Color{150,200,255,255});

    if(click && hovered) {
        activeChanged = true;
    }
    y += boxH + S(6);
}

}
