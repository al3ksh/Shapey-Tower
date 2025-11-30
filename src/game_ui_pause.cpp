#include "game.h"
#include "raylib.h"
#include "input.h"
#include <cmath>

static int pauseTab = 0;

static void DrawPauseTabButton(float x, int y, int w, int h, const char* label, int tabIndex, Vector2 mPos, bool click) {
    Rectangle rect{x, (float)y, (float)w, (float)h};
    bool selected = (pauseTab == tabIndex);
    bool hovered = CheckCollisionPointRec(mPos, rect);
    Color col = selected ? Color{60,120,180,255} : (hovered ? Color{50,70,100,255} : Color{35,45,60,255});
    DrawRectangleRec(rect, col);
    if(selected) DrawRectangle((int)x, y+h-3, w, 3, Color{100,180,255,255});
    int tw = MeasureText(label, 13);
    DrawText(label, (int)(x + w/2 - tw/2), y + h/2 - 6, 13, selected ? WHITE : Color{180,180,180,255});
    if(click && hovered) pauseTab = tabIndex;
}

static void DrawPauseSectionHeader(int &y, float uiCenterX, const char* text) {
    int tw = MeasureText(text, 18);
    DrawText(text, (int)(uiCenterX - tw/2), y, 18, Color{100,180,255,255});
    y += 24;
    DrawLine((int)(uiCenterX - 80), y-4, (int)(uiCenterX + 80), y-4, Color{60,80,120,180});
}

static void DrawPauseToggle(int &y, float uiCenterX, const char* label, bool &value, Vector2 mPos, bool click, bool &changed, int sw) {
    int boxW = 260, boxH = 30;
    int boxX = (int)(uiCenterX - boxW/2);
    if(boxX < 10) boxX = 10;
    if(boxX + boxW > sw - 10) boxX = sw - 10 - boxW;
    
    Rectangle rect{(float)boxX, (float)y, (float)boxW, (float)boxH};
    bool hovered = CheckCollisionPointRec(mPos, rect);
    DrawRectangleRec(rect, hovered ? Color{50,60,80,255} : Color{40,50,65,255});
    DrawText(label, boxX + 10, y + 7, 14, RAYWHITE);
    
    int toggleX = boxX + boxW - 50;
    int toggleY = y + 6;
    int toggleW = 40, toggleH = 18;
    DrawRectangle(toggleX, toggleY, toggleW, toggleH, value ? Color{60,160,80,255} : Color{80,80,80,255});
    int knobX = value ? (toggleX + toggleW - 14) : (toggleX + 2);
    DrawRectangle(knobX, toggleY + 2, 12, 14, WHITE);
    
    if(click && hovered) { value = !value; changed = true; }
    y += boxH + 5;
}

static void DrawPauseSlider(int &y, float uiCenterX, const char* label, float &value, Vector2 mPos, bool drag, bool &changed, int sw) {
    int boxW = 260, boxH = 32;
    int boxX = (int)(uiCenterX - boxW/2);
    if(boxX < 10) boxX = 10;
    if(boxX + boxW > sw - 10) boxX = sw - 10 - boxW;
    
    DrawText(label, boxX, y, 12, Color{180,180,180,255});
    y += 14;
    
    int sliderX = boxX;
    int sliderW = boxW - 40;
    int sliderH = 5;
    int sliderY = y + 4;
    
    DrawRectangle(sliderX, sliderY, sliderW, sliderH, Color{50,50,60,255});
    int fillW = (int)(sliderW * value);
    DrawRectangle(sliderX, sliderY, fillW, sliderH, Color{80,140,200,255});
    DrawCircle(sliderX + fillW, sliderY + sliderH/2, 7, Color{120,180,240,255});
    
    Rectangle sliderRect{(float)(sliderX - 5), (float)(sliderY - 8), (float)(sliderW + 10), 22.f};
    if(drag && CheckCollisionPointRec(mPos, sliderRect)) {
        float newVal = (mPos.x - sliderX) / sliderW;
        if(newVal < 0) newVal = 0; if(newVal > 1) newVal = 1;
        if(std::fabs(newVal - value) > 0.001f) { value = newVal; changed = true; }
    }
    
    char pctText[16];
    snprintf(pctText, sizeof(pctText), "%d%%", (int)(value * 100));
    DrawText(pctText, boxX + boxW - 35, y, 13, RAYWHITE);
    
    y += boxH;
}

enum PauseRebindTarget { PRB_NONE, PRB_LEFT, PRB_RIGHT, PRB_JUMP };
static PauseRebindTarget pauseRebindActive = PRB_NONE;
static float pauseBlinkTime = 0.f;

void Game::DrawPause(){
    BeginDrawing();
    ClearBackground(Color{12,16,24,255});
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    viewportRect = {0,0,(float)sw,(float)sh};
    float uiCenterX = sw / 2.f;
    Vector2 mPos = GetMousePosition();
    bool click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool drag = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    
    const char* title = "PAUZA";
    int tw = MeasureText(title, 32);
    DrawText(title, (int)(uiCenterX - tw/2), 18, 32, RAYWHITE);
    
    int tabY = 60;
    int tabW = 70, tabH = 26;
    float tabStartX = uiCenterX - (5 * tabW + 4*3) / 2.f;
    DrawPauseTabButton(tabStartX, tabY, tabW, tabH, "GRA", 0, mPos, click);
    DrawPauseTabButton(tabStartX + (tabW+3), tabY, tabW, tabH, "VIDEO", 1, mPos, click);
    DrawPauseTabButton(tabStartX + 2*(tabW+3), tabY, tabW, tabH, "AUDIO", 2, mPos, click);
    DrawPauseTabButton(tabStartX + 3*(tabW+3), tabY, tabW, tabH, "KLAWISZE", 3, mPos, click);
    DrawPauseTabButton(tabStartX + 4*(tabW+3), tabY, tabW, tabH, "EFEKTY", 4, mPos, click);
    
    int contentY = tabY + tabH + 18;
    int y = contentY;
    bool settingsChanged = false;
    
    if(pauseTab == 0) {
        DrawPauseSectionHeader(y, uiCenterX, "Gra");
        y += 6;
        
        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, 240, 42, "WZNOW", mPos, pressed);
        if(pressed) { state.paused = false; ChangeScreen(GameState::Screen::GAME); }
        y += 10;
        
        GuiButtonCentered(uiCenterX, y, 240, 38, "RESTART", mPos, pressed);
        if(pressed) { ResetGame(); ChangeScreen(GameState::Screen::GAME, false); }
        y += 10;
        
        GuiButtonCentered(uiCenterX, y, 240, 38, "MENU GLOWNE", mPos, pressed);
        if(pressed) { ChangeScreen(GameState::Screen::MENU); }
        y += 10;
        
        GuiButtonCentered(uiCenterX, y, 200, 34, "WYJSCIE", mPos, pressed);
        if(pressed) { running = false; }
        
        y += 30;
        char scoreText[64];
        snprintf(scoreText, sizeof(scoreText), "Wynik: %d", state.score);
        int stw = MeasureText(scoreText, 16);
        DrawText(scoreText, (int)(uiCenterX - stw/2), y, 16, Color{255,220,100,255});
    }
    else if(pauseTab == 1) {
        DrawPauseSectionHeader(y, uiCenterX, "Video");
        y += 6;
        
        DrawText("Rozdzielczosc:", (int)(uiCenterX - 130), y, 12, Color{180,180,180,255});
        y += 16;
        DrawResolutionSelector(y, uiCenterX, mPos, click, sw);
        y += 6;
        
        DrawPauseToggle(y, uiCenterX, "Pelny ekran", fullscreen, mPos, click, settingsChanged, sw);
        if(settingsChanged) { ApplyResolution(false); settingsChanged = false; settingsDirty = true; }
        
        bool vsyncChanged = false;
        DrawPauseToggle(y, uiCenterX, "VSync", settings.vsync, mPos, click, vsyncChanged, sw);
        if(vsyncChanged) {
            if(settings.vsync) SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
            else SetTargetFPS(settings.targetFPS);
            settingsDirty = true;
        }
        
        DrawPauseToggle(y, uiCenterX, "Pokazuj FPS", settings.showFPS, mPos, click, settingsChanged, sw);
        if(settingsChanged) { settingsDirty = true; settingsChanged = false; }
    }
    else if(pauseTab == 2) {
        DrawPauseSectionHeader(y, uiCenterX, "Audio");
        y += 6;
        
        DrawPauseSlider(y, uiCenterX, "Glosnosc glowna", state.audio.masterSlider, mPos, drag, settingsChanged, sw);
        DrawPauseSlider(y, uiCenterX, "Muzyka", state.audio.volMusic, mPos, drag, settingsChanged, sw);
        DrawPauseSlider(y, uiCenterX, "Skok", state.audio.volJump, mPos, drag, settingsChanged, sw);
        DrawPauseSlider(y, uiCenterX, "Odbicie", state.audio.volBounce, mPos, drag, settingsChanged, sw);
        DrawPauseSlider(y, uiCenterX, "Smierc", state.audio.volDeath, mPos, drag, settingsChanged, sw);
        
        if(settingsChanged) {
            settingsDirty = true;
            settingsSaveTimer = 0.f;
            ApplyAudioVolumes();
        }
        
        y += 10;
        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, 160, 30, "Domyslne", mPos, pressed);
        if(pressed) {
            state.audio.masterSlider = 0.5f;
            state.audio.volMusic = 0.5f;
            state.audio.volJump = 0.5f;
            state.audio.volBounce = 0.5f;
            state.audio.volDeath = 0.5f;
            state.audio.volThemeChange = 0.5f;
            ApplyAudioVolumes();
            settingsDirty = true;
        }
    }
    else if(pauseTab == 3) {
        DrawPauseSectionHeader(y, uiCenterX, "Sterowanie");
        y += 6;
        
        pauseBlinkTime += GetFrameTime();
        float blinkAlpha = (std::sin(pauseBlinkTime * 6.f) * 0.5f + 0.5f);
        
        auto DrawKeyBind = [&](const char* label, int &key, PauseRebindTarget target) {
            int boxW = 260, boxH = 34;
            int boxX = (int)(uiCenterX - boxW/2);
            
            Rectangle rect{(float)boxX, (float)y, (float)boxW, (float)boxH};
            bool hovered = CheckCollisionPointRec(mPos, rect);
            bool active = (pauseRebindActive == target);
            
            Color bgCol = active ? Color{80,60,100,255} : (hovered ? Color{50,60,80,255} : Color{40,50,65,255});
            if(active) bgCol.a = (unsigned char)(180 + 75 * blinkAlpha);
            DrawRectangleRec(rect, bgCol);
            DrawRectangleLines(boxX, y, boxW, boxH, active ? Color{180,140,255,255} : Color{80,80,80,255});
            
            DrawText(label, boxX + 10, y + 9, 14, RAYWHITE);
            
            const char* keyName = active ? "..." : KeyName(key);
            int knw = MeasureText(keyName, 15);
            DrawRectangle(boxX + boxW - knw - 22, y + 6, knw + 14, 22, Color{30,35,45,255});
            DrawText(keyName, boxX + boxW - knw - 15, y + 9, 15, active ? Color{255,200,100,255} : Color{150,200,255,255});
            
            if(click && hovered) {
                pauseRebindActive = (pauseRebindActive == target) ? PRB_NONE : target;
            }
            
            y += boxH + 5;
        };
        
        DrawKeyBind("Ruch w lewo", state.keys.left, PRB_LEFT);
        DrawKeyBind("Ruch w prawo", state.keys.right, PRB_RIGHT);
        DrawKeyBind("Skok", state.keys.jump, PRB_JUMP);
        
        if(pauseRebindActive != PRB_NONE) {
            for(int k = 32; k < 350; k++) {
                if(IsKeyPressed(k)) {
                    if(pauseRebindActive == PRB_LEFT) state.keys.left = k;
                    else if(pauseRebindActive == PRB_RIGHT) state.keys.right = k;
                    else if(pauseRebindActive == PRB_JUMP) state.keys.jump = k;
                    pauseRebindActive = PRB_NONE;
                    settingsDirty = true;
                    break;
                }
            }
            DrawText("Nacisnij klawisz...", (int)(uiCenterX - 55), y + 5, 13, Color{255,200,100,255});
        }
        
        y += 20;
        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, 160, 30, "Domyslne", mPos, pressed);
        if(pressed) {
            state.keys.left = KEY_A;
            state.keys.right = KEY_D;
            state.keys.jump = KEY_SPACE;
            settingsDirty = true;
        }
    }
    else if(pauseTab == 4) {
        DrawPauseSectionHeader(y, uiCenterX, "Efekty");
        y += 6;
        
        bool shakeChanged = false;
        DrawPauseToggle(y, uiCenterX, "Trzesienie ekranu", settings.screenShake, mPos, click, shakeChanged, sw);
        if(shakeChanged) settingsDirty = true;
        
        bool partChanged = false;
        DrawPauseToggle(y, uiCenterX, "Czasteczki", settings.particles, mPos, click, partChanged, sw);
        if(partChanged) settingsDirty = true;
        
        bool comboChanged = false;
        DrawPauseToggle(y, uiCenterX, "Efekt combo (ogien)", settings.comboEffects, mPos, click, comboChanged, sw);
        if(comboChanged) settingsDirty = true;
    }
    
    state.audio.masterVolume = 0.0001f + state.audio.masterSlider * 1.0f;
    SetMasterVolume(state.audio.masterVolume);
    if(state.audio.musicBg.ctxData) {
        SetMusicVolume(state.audio.musicBg, state.audio.volMusic * VOL_MUSIC_MULT);
    }
    
    DrawText("ESC = wznow gre", (int)(uiCenterX - 55), sh - 30, 11, Color{70,70,90,255});
    
    if(IsKeyPressed(KEY_TAB)) {
        pauseTab = (pauseTab + 1) % 5;
    }
    
    EndDrawing();
}

