#include "game.h"
#include "raylib.h"
#include "input.h"
#include "localization.h"
#include "ui_helpers.h"
#include <cmath>

static int pauseTab = 0;
static float uiScale = 1.0f;

static int S(int base) { return (int)(base * uiScale); }
static float Sf(float base) { return base * uiScale; }

static void DrawPauseSectionHeader(int &y, float uiCenterX, const char* text) {
    int fontSize = S(18);
    int tw = MeasureText(text, fontSize);
    DrawText(text, (int)(uiCenterX - tw/2), y, fontSize, Color{100,180,255,255});
    y += S(24);
    DrawLine((int)(uiCenterX - S(80)), y-S(4), (int)(uiCenterX + S(80)), y-S(4), Color{60,80,120,180});
}

static void DrawPauseToggle(int &y, float uiCenterX, const char* label, bool &value, Vector2 mPos, bool click, bool &changed, int sw) {
    int boxW = S(260), boxH = S(30);
    int boxX = (int)(uiCenterX - boxW/2);
    if(boxX < S(10)) boxX = S(10);
    if(boxX + boxW > sw - S(10)) boxX = sw - S(10) - boxW;
    
    Rectangle rect{(float)boxX, (float)y, (float)boxW, (float)boxH};
    bool hovered = CheckCollisionPointRec(mPos, rect);
    DrawRectangleRec(rect, hovered ? Color{50,60,80,255} : Color{40,50,65,255});
    DrawText(label, boxX + S(10), y + S(7), S(14), RAYWHITE);
    
    int toggleX = boxX + boxW - S(50);
    int toggleY = y + S(6);
    int toggleW = S(40), toggleH = S(18);
    DrawRectangle(toggleX, toggleY, toggleW, toggleH, value ? Color{60,160,80,255} : Color{80,80,80,255});
    int knobX = value ? (toggleX + toggleW - S(14)) : (toggleX + S(2));
    DrawRectangle(knobX, toggleY + S(2), S(12), S(14), WHITE);
    
    if(click && hovered) { value = !value; changed = true; }
    y += boxH + S(5);
}

static void DrawPauseSlider(int &y, float uiCenterX, const char* label, float &value, Vector2 mPos, bool drag, bool &changed, int sw) {
    int boxW = S(260), boxH = S(32);
    int boxX = (int)(uiCenterX - boxW/2);
    if(boxX < S(10)) boxX = S(10);
    if(boxX + boxW > sw - S(10)) boxX = sw - S(10) - boxW;
    
    DrawText(label, boxX, y, S(12), Color{180,180,180,255});
    y += S(14);
    
    int sliderX = boxX;
    int sliderW = boxW - S(40);
    int sliderH = S(5);
    int sliderY = y + S(4);
    
    DrawRectangle(sliderX, sliderY, sliderW, sliderH, Color{50,50,60,255});
    int fillW = (int)(sliderW * value);
    DrawRectangle(sliderX, sliderY, fillW, sliderH, Color{80,140,200,255});
    DrawCircle(sliderX + fillW, sliderY + sliderH/2, Sf(7), Color{120,180,240,255});
    
    Rectangle sliderRect{(float)(sliderX - S(5)), (float)(sliderY - S(8)), (float)(sliderW + S(10)), Sf(22.f)};
    if(drag && CheckCollisionPointRec(mPos, sliderRect)) {
        float newVal = (mPos.x - sliderX) / sliderW;
        if(newVal < 0) newVal = 0; if(newVal > 1) newVal = 1;
        if(std::fabs(newVal - value) > 0.001f) { value = newVal; changed = true; }
    }
    
    char pctText[16];
    snprintf(pctText, sizeof(pctText), "%d%%", (int)(value * 100));
    DrawText(pctText, boxX + boxW - S(35), y, S(13), RAYWHITE);
    
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
    
    uiScale = sh / 720.0f;
    if(uiScale < 1.0f) uiScale = 1.0f;
    
    viewportRect = {0,0,(float)sw,(float)sh};
    float uiCenterX = sw / 2.f;
    Vector2 mPos = GetMousePosition();
    bool click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool drag = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    
    const char* title = Loc::Pause_Title();
    int titleFont = S(32);
    int tw = MeasureText(title, titleFont);
    DrawText(title, (int)(uiCenterX - tw/2), S(18), titleFont, RAYWHITE);
    
    int tabY = S(60);
    int tabW = S(70), tabH = S(26);
    int tabGap = S(3);
    float tabStartX = uiCenterX - (5 * tabW + 4*tabGap) / 2.f;
    if(Ui::DrawTabButton(tabStartX, tabY, tabW, tabH, Loc::Tab_Game(), 0, pauseTab, mPos, click, uiScale)) pauseTab=0;
    if(Ui::DrawTabButton(tabStartX + (tabW+tabGap), tabY, tabW, tabH, Loc::Tab_Video(), 1, pauseTab, mPos, click, uiScale)) pauseTab=1;
    if(Ui::DrawTabButton(tabStartX + 2*(tabW+tabGap), tabY, tabW, tabH, Loc::Tab_Audio(), 2, pauseTab, mPos, click, uiScale)) pauseTab=2;
    if(Ui::DrawTabButton(tabStartX + 3*(tabW+tabGap), tabY, tabW, tabH, Loc::Tab_Keys(), 3, pauseTab, mPos, click, uiScale)) pauseTab=3;
    if(Ui::DrawTabButton(tabStartX + 4*(tabW+tabGap), tabY, tabW, tabH, Loc::Tab_Effects(), 4, pauseTab, mPos, click, uiScale)) pauseTab=4;
    
    int contentY = tabY + tabH + S(18);
    int y = contentY;
    bool settingsChanged = false;
    
    if(pauseTab == 0) {
        DrawPauseSectionHeader(y, uiCenterX, Loc::Tab_Game());
        y += S(6);
        
        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, S(240), S(42), Loc::Pause_Resume(), mPos, pressed);
        if(pressed) { state.paused = false; ChangeScreen(GameState::Screen::GAME); }
        y += S(10);
        
        GuiButtonCentered(uiCenterX, y, S(240), S(38), Loc::Pause_Restart(), mPos, pressed);
        if(pressed) { ResetGame(); ChangeScreen(GameState::Screen::GAME, false); }
        y += S(10);
        
        GuiButtonCentered(uiCenterX, y, S(240), S(38), Loc::Pause_MainMenu(), mPos, pressed);
        if(pressed) { ChangeScreen(GameState::Screen::MENU); }
        y += S(10);
        
        GuiButtonCentered(uiCenterX, y, S(200), S(34), Loc::Pause_Exit(), mPos, pressed);
        if(pressed) { running = false; }
        
        y += S(30);
        
        int coinFontSize = S(14);
        const char* coinsTxt = TextFormat("%s %d", Loc::GameOver_Coins(), state.globalCoins);
        int coinsTxtW = MeasureText(coinsTxt, coinFontSize);
        DrawCircle((int)(uiCenterX - coinsTxtW/2 - S(10)), y + S(7), Sf(6), GOLD);
        DrawText(coinsTxt, (int)(uiCenterX - coinsTxtW/2), y, coinFontSize, GOLD);
        y += S(22);
        
        int scoreFontSize = S(16);
        char scoreText[64];
        snprintf(scoreText, sizeof(scoreText), "%s %d", Loc::Pause_Score(), state.score);
        int stw = MeasureText(scoreText, scoreFontSize);
        DrawText(scoreText, (int)(uiCenterX - stw/2), y, scoreFontSize, Color{255,220,100,255});
    }
    else if(pauseTab == 1) {
        DrawPauseSectionHeader(y, uiCenterX, Loc::Video_Title());
        y += S(6);
        
        int labelFontSize = S(12);
        DrawText(Loc::Video_Resolution(), (int)(uiCenterX - S(130)), y, labelFontSize, Color{180,180,180,255});
        y += S(16);
        DrawResolutionSelector(y, uiCenterX, mPos, click, sw, uiScale);
        y += S(6);
        
        DrawPauseToggle(y, uiCenterX, Loc::Video_Fullscreen(), fullscreen, mPos, click, settingsChanged, sw);
        if(settingsChanged) { ApplyResolution(false); settingsChanged = false; settingsDirty = true; }
        
        bool vsyncChanged = false;
        DrawPauseToggle(y, uiCenterX, Loc::Video_VSync(), settings.vsync, mPos, click, vsyncChanged, sw);
        if(vsyncChanged) {
            if(settings.vsync) SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
            else SetTargetFPS(settings.targetFPS);
            settingsDirty = true;
        }
        
        y += S(4);
        DrawText(Loc::Video_FPSLimit(), (int)(uiCenterX - S(130)), y, labelFontSize, Color{180,180,180,255});
        y += S(14);
        const char* fpsOptions[] = {"30", "60", "120", "144", "Max"};
        int fpsValues[] = {30, 60, 120, 144, 0};
        int fpsIndex = 1;
        for(int i = 0; i < 5; i++) {
            if(fpsValues[i] == settings.targetFPS) { fpsIndex = i; break; }
        }
        int boxW = S(260), boxH = S(26);
        int boxX = (int)(uiCenterX - boxW/2);
        if(boxX < S(10)) boxX = S(10);
        if(boxX + boxW > sw - S(10)) boxX = sw - S(10) - boxW;
        int btnW = (boxW - S(4)*3) / 5;
        for(int i = 0; i < 5; i++) {
            Rectangle rect{(float)(boxX + i*(btnW+S(3))), (float)y, (float)btnW, (float)boxH};
            bool sel = (fpsIndex == i);
            bool hov = CheckCollisionPointRec(mPos, rect);
            Color col = sel ? Color{60,140,100,255} : (hov ? Color{60,80,110,255} : Color{45,55,70,255});
            DrawRectangleRec(rect, col);
            DrawRectangleLines((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height, sel ? Color{100,220,140,255} : Color{80,80,80,255});
            int ftw = MeasureText(fpsOptions[i], S(12));
            DrawText(fpsOptions[i], (int)(rect.x + rect.width/2 - ftw/2), (int)(rect.y + S(7)), S(12), RAYWHITE);
            if(click && hov && fpsIndex != i) {
                settings.targetFPS = fpsValues[i];
                if(!settings.vsync) {
                    SetTargetFPS(settings.targetFPS);
                }
                settingsDirty = true;
            }
        }
        y += boxH + S(6);
        
        DrawPauseToggle(y, uiCenterX, Loc::Video_ShowFPS(), settings.showFPS, mPos, click, settingsChanged, sw);
        if(settingsChanged) { settingsDirty = true; settingsChanged = false; }
    }
    else if(pauseTab == 2) {
        DrawPauseSectionHeader(y, uiCenterX, Loc::Audio_Title());
        y += S(6);
        
        DrawPauseSlider(y, uiCenterX, Loc::Audio_Master(), state.audio.masterSlider, mPos, drag, settingsChanged, sw);
        DrawPauseSlider(y, uiCenterX, Loc::Audio_Music(), state.audio.volMusic, mPos, drag, settingsChanged, sw);
        DrawPauseSlider(y, uiCenterX, Loc::Audio_Jump(), state.audio.volJump, mPos, drag, settingsChanged, sw);
        DrawPauseSlider(y, uiCenterX, Loc::Audio_Bounce(), state.audio.volBounce, mPos, drag, settingsChanged, sw);
        DrawPauseSlider(y, uiCenterX, Loc::Audio_Death(), state.audio.volDeath, mPos, drag, settingsChanged, sw);
        
        if(settingsChanged) {
            settingsDirty = true;
            settingsSaveTimer = 0.f;
            ApplyAudioVolumes();
        }
        
        y += S(10);
        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, S(160), S(30), Loc::Audio_Default(), mPos, pressed);
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
        DrawPauseSectionHeader(y, uiCenterX, Loc::Keys_Title());
        y += S(6);
        
        pauseBlinkTime += GetFrameTime();
        float blinkAlpha = (std::sin(pauseBlinkTime * 6.f) * 0.5f + 0.5f);
        
        auto DrawKeyBind = [&](const char* label, int &key, PauseRebindTarget target) {
            int boxW = S(260), boxH = S(34);
            int boxX = (int)(uiCenterX - boxW/2);
            
            Rectangle rect{(float)boxX, (float)y, (float)boxW, (float)boxH};
            bool hovered = CheckCollisionPointRec(mPos, rect);
            bool active = (pauseRebindActive == target);
            
            Color bgCol = active ? Color{80,60,100,255} : (hovered ? Color{50,60,80,255} : Color{40,50,65,255});
            if(active) bgCol.a = (unsigned char)(180 + 75 * blinkAlpha);
            DrawRectangleRec(rect, bgCol);
            DrawRectangleLines(boxX, y, boxW, boxH, active ? Color{180,140,255,255} : Color{80,80,80,255});
            
            int labelFontSize = S(14);
            DrawText(label, boxX + S(10), y + S(9), labelFontSize, RAYWHITE);
            
            int keyFontSize = S(15);
            const char* keyName = active ? "..." : KeyName(key);
            int knw = MeasureText(keyName, keyFontSize);
            DrawRectangle(boxX + boxW - knw - S(22), y + S(6), knw + S(14), S(22), Color{30,35,45,255});
            DrawText(keyName, boxX + boxW - knw - S(15), y + S(9), keyFontSize, active ? Color{255,200,100,255} : Color{150,200,255,255});
            
            if(click && hovered) {
                pauseRebindActive = (pauseRebindActive == target) ? PRB_NONE : target;
            }
            
            y += boxH + S(5);
        };
        
        DrawKeyBind(Loc::Keys_MoveLeft(), state.keys.left, PRB_LEFT);
        DrawKeyBind(Loc::Keys_MoveRight(), state.keys.right, PRB_RIGHT);
        DrawKeyBind(Loc::Keys_Jump(), state.keys.jump, PRB_JUMP);
        
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
            int hintFontSize = S(13);
            int hintW = MeasureText(Loc::Keys_PressKey(), hintFontSize);
            DrawText(Loc::Keys_PressKey(), (int)(uiCenterX - hintW/2), y + S(5), hintFontSize, Color{255,200,100,255});
        }
        
        y += S(20);
        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, S(160), S(30), Loc::Keys_Default(), mPos, pressed);
        if(pressed) {
            state.keys.left = KEY_A;
            state.keys.right = KEY_D;
            state.keys.jump = KEY_SPACE;
            settingsDirty = true;
        }
    }
    else if(pauseTab == 4) {
        DrawPauseSectionHeader(y, uiCenterX, Loc::Effects_Title());
        y += S(6);
        
        bool shakeChanged = false;
        DrawPauseToggle(y, uiCenterX, Loc::Effects_ScreenShake(), settings.screenShake, mPos, click, shakeChanged, sw);
        if(shakeChanged) settingsDirty = true;
        
        bool partChanged = false;
        DrawPauseToggle(y, uiCenterX, Loc::Effects_Particles(), settings.particles, mPos, click, partChanged, sw);
        if(partChanged) settingsDirty = true;
        
        bool comboChanged = false;
        DrawPauseToggle(y, uiCenterX, Loc::Effects_ComboFire(), settings.comboEffects, mPos, click, comboChanged, sw);
        if(comboChanged) settingsDirty = true;
        
        bool powerUpChanged = false;
        DrawPauseToggle(y, uiCenterX, Loc::Effects_PowerUp(), settings.powerUpEffects, mPos, click, powerUpChanged, sw);
        if(powerUpChanged) settingsDirty = true;
    }
    
    ApplyMenuAudioVolumes();
    
    DrawText(Loc::Pause_EscResume(), (int)(uiCenterX - S(55)), sh - S(30), S(11), Color{70,70,90,255});
    
    if(IsKeyPressed(KEY_TAB)) {
        pauseTab = (pauseTab + 1) % 5;
    }
    
    EndDrawing();
}

