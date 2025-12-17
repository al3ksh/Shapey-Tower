#include "game.h"
#include "debug.h"
#include "difficulty.h"
#include "daily_challenge.h"
#include "persistence.h"
#include "input.h"
#include "localization.h"
#include <cmath>

static int menuTab = 0;
static float uiScale = 1.0f;

static int S(int base) { return (int)(base * uiScale); }
static float Sf(float base) { return base * uiScale; }

static void DrawTabButton(float x, int y, int w, int h, const char* label, int tabIndex, Vector2 mPos, bool click) {
    Rectangle rect{x, (float)y, (float)w, (float)h};
    bool selected = (menuTab == tabIndex);
    bool hovered = CheckCollisionPointRec(mPos, rect);
    Color col = selected ? Color{60,120,180,255} : (hovered ? Color{50,70,100,255} : Color{35,45,60,255});
    DrawRectangleRec(rect, col);
    if(selected) DrawRectangle((int)x, y+h-S(3), w, S(3), Color{100,180,255,255});
    int fontSize = S(14);
    int tw = MeasureText(label, fontSize);
    DrawText(label, (int)(x + w/2 - tw/2), y + h/2 - fontSize/2, fontSize, selected ? WHITE : Color{180,180,180,255});
    if(click && hovered) menuTab = tabIndex;
}

static void DrawSectionHeader(int &y, float uiCenterX, const char* text) {
    int fontSize = S(20);
    int tw = MeasureText(text, fontSize);
    DrawText(text, (int)(uiCenterX - tw/2), y, fontSize, Color{100,180,255,255});
    y += S(28);
    DrawLine((int)(uiCenterX - S(100)), y-S(5), (int)(uiCenterX + S(100)), y-S(5), Color{60,80,120,180});
}

static void DrawToggle(int &y, float uiCenterX, const char* label, bool &value, Vector2 mPos, bool click, bool &changed, int sw) {
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

static void DrawSliderRow(int &y, float uiCenterX, const char* label, float &value, Vector2 mPos, bool drag, bool &changed, int sw) {
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
    
    Rectangle sliderRect{(float)(sliderX - S(5)), (float)(sliderY - S(10)), (float)(sliderW + S(10)), Sf(26.f)};
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

static void DrawSelector(int &y, float uiCenterX, const char** options, int optCount, int &selected, Vector2 mPos, bool click, bool &changed, int sw) {
    int boxW = S(280), boxH = S(30);
    int boxX = (int)(uiCenterX - boxW/2);
    if(boxX < S(10)) boxX = S(10);
    if(boxX + boxW > sw - S(10)) boxX = sw - S(10) - boxW;
    
    int gap = S(3);
    int btnW = (boxW - (optCount-1)*gap) / optCount;
    for(int i = 0; i < optCount; i++) {
        Rectangle rect{(float)(boxX + i*(btnW+gap)), (float)y, (float)btnW, (float)boxH};
        bool sel = (selected == i);
        bool hov = CheckCollisionPointRec(mPos, rect);
        Color col = sel ? Color{60,140,100,255} : (hov ? Color{60,80,110,255} : Color{45,55,70,255});
        DrawRectangleRec(rect, col);
        DrawRectangleLines((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height, sel ? Color{100,220,140,255} : Color{80,80,80,255});
        int fontSize = S(13);
        int tw = MeasureText(options[i], fontSize);
        DrawText(options[i], (int)(rect.x + rect.width/2 - tw/2), (int)(rect.y + boxH/2 - fontSize/2), fontSize, RAYWHITE);
        if(click && hov && selected != i) { selected = i; changed = true; }
    }
    y += boxH + S(10);
}

static void DrawDifficultySelector(int &y, float uiCenterX, const char** options, int &selected, Vector2 mPos, bool click, bool &changed, int sw) {
    int boxW = S(280), boxH = S(30);
    int boxX = (int)(uiCenterX - boxW/2);
    if(boxX < S(10)) boxX = S(10);
    if(boxX + boxW > sw - S(10)) boxX = sw - S(10) - boxW;
    
    Color selColors[3] = {
        Color{60,160,80,255},   
        Color{200,160,50,255},  
        Color{180,60,60,255}    
    };
    Color borderColors[3] = {
        Color{100,220,100,255}, 
        Color{255,200,80,255},  
        Color{255,100,100,255}  
    };
    
    int gap = S(3);
    int btnW = (boxW - 2*gap) / 3;
    for(int i = 0; i < 3; i++) {
        Rectangle rect{(float)(boxX + i*(btnW+gap)), (float)y, (float)btnW, (float)boxH};
        bool sel = (selected == i);
        bool hov = CheckCollisionPointRec(mPos, rect);
        
        Color bgCol, borderCol;
        if(sel) {
            bgCol = selColors[i];
            borderCol = borderColors[i];
        } else if(hov) {
            bgCol = Color{(unsigned char)(selColors[i].r/2), (unsigned char)(selColors[i].g/2), (unsigned char)(selColors[i].b/2), 255};
            borderCol = Color{80,80,80,255};
        } else {
            bgCol = Color{45,55,70,255};
            borderCol = Color{80,80,80,255};
        }
        
        DrawRectangleRec(rect, bgCol);
        DrawRectangleLines((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height, borderCol);
        int fontSize = S(13);
        int tw = MeasureText(options[i], fontSize);
        DrawText(options[i], (int)(rect.x + rect.width/2 - tw/2), (int)(rect.y + boxH/2 - fontSize/2), fontSize, RAYWHITE);
        if(click && hov && selected != i) { selected = i; changed = true; }
    }
    y += boxH + S(10);
}

void Game::DrawMenu(){
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
    
    const char* title = "SHAPEY TOWER";
    int titleFont = S(38);
    int tw = MeasureText(title, titleFont);
    DrawText(title, (int)(uiCenterX - tw/2), S(20), titleFont, RAYWHITE);
    DrawText("v1.5", sw - S(45), sh - S(22), S(12), Color{70,70,70,255});
    
    int tabY = S(72);
    int tabW = S(80), tabH = S(28);
    int tabGap = S(3);
    float tabStartX = uiCenterX - (5 * tabW + 4*tabGap) / 2.f;
    DrawTabButton(tabStartX, tabY, tabW, tabH, Loc::Tab_Game(), 0, mPos, click);
    DrawTabButton(tabStartX + (tabW+tabGap), tabY, tabW, tabH, Loc::Tab_Video(), 1, mPos, click);
    DrawTabButton(tabStartX + 2*(tabW+tabGap), tabY, tabW, tabH, Loc::Tab_Audio(), 2, mPos, click);
    DrawTabButton(tabStartX + 3*(tabW+tabGap), tabY, tabW, tabH, Loc::Tab_Keys(), 3, mPos, click);
    DrawTabButton(tabStartX + 4*(tabW+tabGap), tabY, tabW, tabH, Loc::Tab_Effects(), 4, mPos, click);
    
    int contentY = tabY + tabH + S(20);
    int y = contentY;
    bool settingsChanged = false;
    
    if(menuTab == 0) {
        DrawSectionHeader(y, uiCenterX, Loc::Menu_StartGame());
        y += S(8);
        
        DrawText(Loc::Menu_Difficulty(), (int)(uiCenterX - S(140)), y, S(13), Color{180,180,180,255});
        y += S(18);
        const char* diffNames[] = {"EASY", "NORMAL", "HARD"};
        int diffInt = (int)state.difficulty;
        bool diffChanged = false;
        DrawDifficultySelector(y, uiCenterX, diffNames, diffInt, mPos, click, diffChanged, sw);
        if(diffChanged) {
            state.difficulty = (Difficulty)diffInt;
            settingsDirty = true;
        }
        
        const char* diffDescEN[] = {
            "Wider platforms, more coins",
            "Standard settings",
            "Narrow platforms, faster pace"
        };
        const char* diffDescPL[] = {
            "Szersze platformy, wiecej monet",
            "Standardowe ustawienia",
            "Wezsze platformy, szybsze tempo"
        };
        const char* diffDesc = (Loc::GetLanguage() == Language::EN) ? diffDescEN[diffInt] : diffDescPL[diffInt];
        int descFont = S(11);
        int ddw = MeasureText(diffDesc, descFont);
        DrawText(diffDesc, (int)(uiCenterX - ddw/2), y, descFont, Color{130,130,150,255});
        y += S(20);
        
        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, S(260), S(46), Loc::Menu_Play(), mPos, pressed);
        if(pressed) {
            state.isDailyRun = false;
            ResetGame();
            state.started = true;
            state.paused = false;
            ChangeScreen(GameState::Screen::GAME);
        }
        y += S(16);
        
        DrawLine((int)(uiCenterX - S(130)), y, (int)(uiCenterX + S(130)), y, Color{60,80,120,150});
        y += S(12);
        
        DailyChallenge today = state.dailyChallenge;
        
        const char* challengeName = GetChallengeName(today.type);
        char dailyTitle[128];
        snprintf(dailyTitle, sizeof(dailyTitle), ">> %s <<", challengeName);
        int dailyFont = S(16);
        int ctw = MeasureText(dailyTitle, dailyFont);
        DrawText(dailyTitle, (int)(uiCenterX - ctw/2), y, dailyFont, Color{255,180,80,255});
        y += S(22);
        
        const char* challengeDesc = GetChallengeDescription(today.type);
        int cdw = MeasureText(challengeDesc, descFont);
        DrawText(challengeDesc, (int)(uiCenterX - cdw/2), y, descFont, Color{150,150,170,255});
        y += S(18);
        
        GuiButtonCentered(uiCenterX, y, S(260), S(40), Loc::Daily_Title(), mPos, pressed);
        if(pressed) {
            state.isDailyRun = true;
            state.dailyChallenge = GetTodaysChallenge();
            state.dailyChallenge.bestScore = LoadDailyHighScore("daily_highscore.txt", 
                state.dailyChallenge.year, state.dailyChallenge.month, state.dailyChallenge.day);
            state.difficulty = Difficulty::NORMAL;
            ResetGame();
            state.started = true;
            state.paused = false;
            ChangeScreen(GameState::Screen::GAME);
        }
        

        char dateStr[64];
        snprintf(dateStr, sizeof(dateStr), "%s %04d-%02d-%02d", Loc::Menu_Today(), today.year, today.month, today.day);
        int dtw = MeasureText(dateStr, descFont);
        DrawText(dateStr, (int)(uiCenterX - dtw/2), y + S(6), descFont, Color{100,100,120,255});
        y += S(22);
        
        if(today.bestScore > 0) {
            char dailyBest[64];
            snprintf(dailyBest, sizeof(dailyBest), "%s %d", Loc::Daily_Best(), today.bestScore);
            int smallFont = S(13);
            int dbw = MeasureText(dailyBest, smallFont);
            DrawText(dailyBest, (int)(uiCenterX - dbw/2), y, smallFont, Color{100,200,255,255});
            y += S(18);
        }
        y += S(10);
        
        DrawLine((int)(uiCenterX - S(130)), y, (int)(uiCenterX + S(130)), y, Color{60,80,120,150});
        y += S(18);
        
        GuiButtonCentered(uiCenterX, y, S(180), S(36), Loc::Menu_Exit(), mPos, pressed);
        if(pressed) running = false;
        
        y += S(40);
        
        int coinFont = S(16);
        const char* coinsTxt = TextFormat("%s %d", Loc::GameOver_Coins(), state.globalCoins);
        int coinsTxtW = MeasureText(coinsTxt, coinFont);
        DrawCircle((int)(uiCenterX - coinsTxtW/2 - S(12)), y + S(8), Sf(8), GOLD);
        DrawText(coinsTxt, (int)(uiCenterX - coinsTxtW/2), y, coinFont, GOLD);
        y += S(24);
        
        char hsText[64];
        snprintf(hsText, sizeof(hsText), "%s %d", Loc::Menu_HighScore(), state.highScore);
        int hsFont = S(16);
        int hsw = MeasureText(hsText, hsFont);
        DrawText(hsText, (int)(uiCenterX - hsw/2), y, hsFont, Color{255,220,100,255});
    }
    else if(menuTab == 1) {
        DrawSectionHeader(y, uiCenterX, Loc::Video_Title());
        y += S(8);
        
        DrawText(Loc::Video_Resolution(), (int)(uiCenterX - S(140)), y, S(13), Color{180,180,180,255});
        y += S(18);
        DrawResolutionSelector(y, uiCenterX, mPos, click, sw, uiScale);
        y += S(8);
        
        DrawToggle(y, uiCenterX, Loc::Video_Fullscreen(), fullscreen, mPos, click, settingsChanged, sw);
        if(settingsChanged) { ApplyResolution(false); settingsChanged = false; settingsDirty = true; }
        
        bool vsyncChanged = false;
        DrawToggle(y, uiCenterX, Loc::Video_VSync(), settings.vsync, mPos, click, vsyncChanged, sw);
        if(vsyncChanged) { 
            if(settings.vsync) {
                SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
            } else {
                SetTargetFPS(settings.targetFPS);
            }
            settingsDirty = true;
        }
        
        y += S(5);
        DrawText(Loc::Video_FPSLimit(), (int)(uiCenterX - S(140)), y, S(13), Color{180,180,180,255});
        y += S(18);
        const char* fpsOptions[] = {"30", "60", "120", "144", "Max"};
        int fpsValues[] = {30, 60, 120, 144, 0};
        int fpsIndex = 1;
        for(int i = 0; i < 5; i++) {
            if(fpsValues[i] == settings.targetFPS) { fpsIndex = i; break; }
        }
        bool fpsChanged = false;
        DrawSelector(y, uiCenterX, fpsOptions, 5, fpsIndex, mPos, click, fpsChanged, sw);
        if(fpsChanged) {
            settings.targetFPS = fpsValues[fpsIndex];
            if(!settings.vsync) {
                SetTargetFPS(settings.targetFPS);
            }
            settingsDirty = true;
        }
        
        DrawToggle(y, uiCenterX, Loc::Video_ShowFPS(), settings.showFPS, mPos, click, settingsChanged, sw);
        if(settingsChanged) { settingsDirty = true; settingsChanged = false; }
    }
    else if(menuTab == 2) {
        DrawSectionHeader(y, uiCenterX, Loc::Audio_Title());
        y += S(8);
        
        DrawSliderRow(y, uiCenterX, Loc::Audio_Master(), state.audio.masterSlider, mPos, drag, settingsChanged, sw);
        DrawSliderRow(y, uiCenterX, Loc::Audio_Music(), state.audio.volMusic, mPos, drag, settingsChanged, sw);
        DrawSliderRow(y, uiCenterX, Loc::Audio_Jump(), state.audio.volJump, mPos, drag, settingsChanged, sw);
        DrawSliderRow(y, uiCenterX, Loc::Audio_Bounce(), state.audio.volBounce, mPos, drag, settingsChanged, sw);
        DrawSliderRow(y, uiCenterX, Loc::Audio_Death(), state.audio.volDeath, mPos, drag, settingsChanged, sw);
        DrawSliderRow(y, uiCenterX, Loc::Audio_ThemeChange(), state.audio.volThemeChange, mPos, drag, settingsChanged, sw);
        
        if(settingsChanged) {
            settingsDirty = true;
            settingsSaveTimer = 0.f;
            ApplyAudioVolumes();
        }
        
        y += S(12);
        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, S(180), S(32), Loc::Audio_Default(), mPos, pressed);
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
    else if(menuTab == 3) {
        DrawSectionHeader(y, uiCenterX, Loc::Keys_Title());
        y += S(8);
        
        enum RebindTarget { RB_NONE, RB_LEFT, RB_RIGHT, RB_JUMP };
        static RebindTarget rebindActive = RB_NONE;
        static float blinkTime = 0.f;
        blinkTime += GetFrameTime();
        float blinkAlpha = (std::sin(blinkTime * 6.f) * 0.5f + 0.5f);
        
        auto DrawKeyBind = [&](const char* label, int &key, RebindTarget target) {
            int boxW = S(280), boxH = S(36);
            int boxX = (int)(uiCenterX - boxW/2);
            
            Rectangle rect{(float)boxX, (float)y, (float)boxW, (float)boxH};
            bool hovered = CheckCollisionPointRec(mPos, rect);
            bool active = (rebindActive == target);
            
            Color bgCol = active ? Color{80,60,100,255} : (hovered ? Color{50,60,80,255} : Color{40,50,65,255});
            if(active) bgCol.a = (unsigned char)(180 + 75 * blinkAlpha);
            DrawRectangleRec(rect, bgCol);
            DrawRectangleLines(boxX, y, boxW, boxH, active ? Color{180,140,255,255} : Color{80,80,80,255});
            
            int labelFont = S(15);
            DrawText(label, boxX + S(12), y + S(10), labelFont, RAYWHITE);
            
            int keyFont = S(16);
            const char* keyName = active ? "..." : KeyName(key);
            int knw = MeasureText(keyName, keyFont);
            DrawRectangle(boxX + boxW - knw - S(25), y + S(7), knw + S(16), S(22), Color{30,35,45,255});
            DrawText(keyName, boxX + boxW - knw - S(17), y + S(10), keyFont, active ? Color{255,200,100,255} : Color{150,200,255,255});
            
            if(click && hovered) {
                rebindActive = (rebindActive == target) ? RB_NONE : target;
            }
            
            y += boxH + S(6);
        };
        
        DrawKeyBind(Loc::Keys_MoveLeft(), state.keys.left, RB_LEFT);
        DrawKeyBind(Loc::Keys_MoveRight(), state.keys.right, RB_RIGHT);
        DrawKeyBind(Loc::Keys_Jump(), state.keys.jump, RB_JUMP);
        
        if(rebindActive != RB_NONE) {
            for(int k = 32; k < 350; k++) {
                if(IsKeyPressed(k)) {
                    if(rebindActive == RB_LEFT) state.keys.left = k;
                    else if(rebindActive == RB_RIGHT) state.keys.right = k;
                    else if(rebindActive == RB_JUMP) state.keys.jump = k;
                    rebindActive = RB_NONE;
                    settingsDirty = true;
                    break;
                }
            }
            DrawText(Loc::Keys_PressKey(), (int)(uiCenterX - S(60)), y + S(10), S(14), Color{255,200,100,255});
        }
        
        y += S(25);
        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, S(180), S(32), Loc::Keys_Default(), mPos, pressed);
        if(pressed) {
            state.keys.left = KEY_A;
            state.keys.right = KEY_D;
            state.keys.jump = KEY_SPACE;
            settingsDirty = true;
        }
    }
    else if(menuTab == 4) {
        DrawSectionHeader(y, uiCenterX, Loc::Effects_Title());
        y += S(8);
        
        bool shakeChanged = false;
        DrawToggle(y, uiCenterX, Loc::Effects_ScreenShake(), settings.screenShake, mPos, click, shakeChanged, sw);
        if(shakeChanged) settingsDirty = true;
        
        bool partChanged = false;
        DrawToggle(y, uiCenterX, Loc::Effects_Particles(), settings.particles, mPos, click, partChanged, sw);
        if(partChanged) settingsDirty = true;
        
        bool comboChanged = false;
        DrawToggle(y, uiCenterX, Loc::Effects_ComboFire(), settings.comboEffects, mPos, click, comboChanged, sw);
        if(comboChanged) settingsDirty = true;
        
        bool powerUpChanged = false;
        DrawToggle(y, uiCenterX, Loc::Effects_PowerUp(), settings.powerUpEffects, mPos, click, powerUpChanged, sw);
        if(powerUpChanged) settingsDirty = true;
        
        y += S(25);
        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, S(180), S(32), Loc::Effects_ResetAll(), mPos, pressed);
        if(pressed) {
            ResetSettingsToDefaults();
        }
    }
    
    state.audio.masterVolume = 0.0001f + state.audio.masterSlider * 1.0f;
    SetMasterVolume(state.audio.masterVolume);
    if(state.audio.musicBg.ctxData) {
        SetMusicVolume(state.audio.musicBg, state.audio.volMusic * VOL_MUSIC_MULT);
    }
    
    {
        static bool langBoxOpen = false;
        int boxW = S(90), boxH = S(26);
        int boxX = sw - boxW - S(10);
        int boxY = sh - boxH - S(45);
        
        const char* langLabel = (settings.language == 0) ? "EN" : "PL";
        Rectangle boxRect{(float)boxX, (float)boxY, (float)boxW, (float)boxH};
        bool hovered = CheckCollisionPointRec(mPos, boxRect);
        
        DrawRectangleRec(boxRect, hovered ? Color{60,70,90,255} : Color{40,50,65,255});
        DrawRectangleLines(boxX, boxY, boxW, boxH, Color{80,100,130,255});
        
        int iconX = boxX + S(12);
        int iconY = boxY + boxH/2;
        DrawCircleLines(iconX, iconY, Sf(8), Color{150,180,220,255});
        DrawLine(iconX - S(8), iconY, iconX + S(8), iconY, Color{150,180,220,255});
        DrawLine(iconX, iconY - S(8), iconX, iconY + S(8), Color{150,180,220,255});
        
        int langFont = S(14);
        int ltw = MeasureText(langLabel, langFont);
        DrawText(langLabel, boxX + S(30), boxY + S(6), langFont, RAYWHITE);
        
        int arrowX = boxX + boxW - S(18);
        int arrowY = boxY + boxH/2;
        int arrowSize = S(5);
        if(langBoxOpen) {
            DrawTriangle({(float)(arrowX-arrowSize), (float)(arrowY+S(3))}, {(float)(arrowX+arrowSize), (float)(arrowY+S(3))}, {(float)arrowX, (float)(arrowY-S(4))}, Color{180,180,180,255});
        } else {
            DrawTriangle({(float)(arrowX-arrowSize), (float)(arrowY-S(3))}, {(float)(arrowX+arrowSize), (float)(arrowY-S(3))}, {(float)arrowX, (float)(arrowY+S(4))}, Color{180,180,180,255});
        }
        
        if(click && hovered) langBoxOpen = !langBoxOpen;
        
        if(langBoxOpen) {
            int optH = S(28);
            int dropY = boxY - optH * 2 - S(4);
            
            Rectangle optEN{(float)boxX, (float)dropY, (float)boxW, (float)optH};
            bool hovEN = CheckCollisionPointRec(mPos, optEN);
            DrawRectangleRec(optEN, hovEN ? Color{70,90,120,255} : Color{50,60,80,255});
            DrawRectangleLines(boxX, dropY, boxW, optH, Color{80,100,130,255});
            DrawText("English", boxX + S(10), dropY + S(6), langFont, settings.language == 0 ? Color{100,200,150,255} : RAYWHITE);
            if(click && hovEN) {
                settings.language = 0;
                Loc::SetLanguage(Language::EN);
                settingsDirty = true;
                langBoxOpen = false;
            }
            
            Rectangle optPL{(float)boxX, (float)(dropY + optH + S(2)), (float)boxW, (float)optH};
            bool hovPL = CheckCollisionPointRec(mPos, optPL);
            DrawRectangleRec(optPL, hovPL ? Color{70,90,120,255} : Color{50,60,80,255});
            DrawRectangleLines(boxX, dropY + optH + S(2), boxW, optH, Color{80,100,130,255});
            DrawText("Polski", boxX + S(10), dropY + optH + S(8), langFont, settings.language == 1 ? Color{100,200,150,255} : RAYWHITE);
            if(click && hovPL) {
                settings.language = 1;
                Loc::SetLanguage(Language::PL);
                settingsDirty = true;
                langBoxOpen = false;
            }
            
            if(click && !hovered && !hovEN && !hovPL) langBoxOpen = false;
        }
    }
    
    DrawText(Loc::Settings_TabHint(), (int)(uiCenterX - S(70)), sh - S(35), S(11), Color{70,70,90,255});
    
    if(IsKeyPressed(KEY_TAB)) {
        menuTab = (menuTab + 1) % 5;
    }
    
    EndDrawing();
}
