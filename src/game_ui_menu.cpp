#include "game.h"
#include "debug.h"
#include "difficulty.h"
#include "daily_challenge.h"
#include "persistence.h"
#include "input.h"
#include "localization.h"
#include <cmath>

static int menuTab = 0;

static void DrawTabButton(float x, int y, int w, int h, const char* label, int tabIndex, Vector2 mPos, bool click) {
    Rectangle rect{x, (float)y, (float)w, (float)h};
    bool selected = (menuTab == tabIndex);
    bool hovered = CheckCollisionPointRec(mPos, rect);
    Color col = selected ? Color{60,120,180,255} : (hovered ? Color{50,70,100,255} : Color{35,45,60,255});
    DrawRectangleRec(rect, col);
    if(selected) DrawRectangle((int)x, y+h-3, w, 3, Color{100,180,255,255});
    int tw = MeasureText(label, 14);
    DrawText(label, (int)(x + w/2 - tw/2), y + h/2 - 7, 14, selected ? WHITE : Color{180,180,180,255});
    if(click && hovered) menuTab = tabIndex;
}

static void DrawSectionHeader(int &y, float uiCenterX, const char* text) {
    int tw = MeasureText(text, 20);
    DrawText(text, (int)(uiCenterX - tw/2), y, 20, Color{100,180,255,255});
    y += 28;
    DrawLine((int)(uiCenterX - 100), y-5, (int)(uiCenterX + 100), y-5, Color{60,80,120,180});
}

static void DrawToggle(int &y, float uiCenterX, const char* label, bool &value, Vector2 mPos, bool click, bool &changed, int sw) {
    int boxW = 280, boxH = 32;
    int boxX = (int)(uiCenterX - boxW/2);
    if(boxX < 10) boxX = 10;
    if(boxX + boxW > sw - 10) boxX = sw - 10 - boxW;
    
    Rectangle rect{(float)boxX, (float)y, (float)boxW, (float)boxH};
    bool hovered = CheckCollisionPointRec(mPos, rect);
    DrawRectangleRec(rect, hovered ? Color{50,60,80,255} : Color{40,50,65,255});
    DrawText(label, boxX + 12, y + 8, 15, RAYWHITE);
    
    int toggleX = boxX + boxW - 55;
    int toggleY = y + 7;
    int toggleW = 44, toggleH = 18;
    DrawRectangle(toggleX, toggleY, toggleW, toggleH, value ? Color{60,160,80,255} : Color{80,80,80,255});
    DrawRectangleLines(toggleX, toggleY, toggleW, toggleH, Color{100,100,100,255});
    int knobX = value ? (toggleX + toggleW - 16) : (toggleX + 2);
    DrawRectangle(knobX, toggleY + 2, 14, 14, WHITE);
    
    if(click && hovered) { value = !value; changed = true; }
    y += boxH + 6;
}

static void DrawSliderRow(int &y, float uiCenterX, const char* label, float &value, Vector2 mPos, bool drag, bool &changed, int sw) {
    int boxW = 280, boxH = 34;
    int boxX = (int)(uiCenterX - boxW/2);
    if(boxX < 10) boxX = 10;
    if(boxX + boxW > sw - 10) boxX = sw - 10 - boxW;
    
    DrawText(label, boxX, y, 13, Color{180,180,180,255});
    y += 16;
    
    int sliderX = boxX;
    int sliderW = boxW - 45;
    int sliderH = 6;
    int sliderY = y + 5;
    
    DrawRectangle(sliderX, sliderY, sliderW, sliderH, Color{50,50,60,255});
    int fillW = (int)(sliderW * value);
    DrawRectangle(sliderX, sliderY, fillW, sliderH, Color{80,140,200,255});
    
    int knobR = 8;
    int knobX = sliderX + fillW;
    DrawCircle(knobX, sliderY + sliderH/2, (float)knobR, Color{120,180,240,255});
    
    Rectangle sliderRect{(float)(sliderX - 5), (float)(sliderY - 10), (float)(sliderW + 10), 26.f};
    if(drag && CheckCollisionPointRec(mPos, sliderRect)) {
        float newVal = (mPos.x - sliderX) / sliderW;
        if(newVal < 0) newVal = 0; if(newVal > 1) newVal = 1;
        if(std::fabs(newVal - value) > 0.001f) { value = newVal; changed = true; }
    }
    
    char pctText[16];
    snprintf(pctText, sizeof(pctText), "%d%%", (int)(value * 100));
    DrawText(pctText, boxX + boxW - 38, y + 2, 14, RAYWHITE);
    
    y += boxH;
}

static void DrawSelector(int &y, float uiCenterX, const char** options, int optCount, int &selected, Vector2 mPos, bool click, bool &changed, int sw) {
    int boxW = 280, boxH = 30;
    int boxX = (int)(uiCenterX - boxW/2);
    if(boxX < 10) boxX = 10;
    if(boxX + boxW > sw - 10) boxX = sw - 10 - boxW;
    
    int btnW = (boxW - (optCount-1)*3) / optCount;
    for(int i = 0; i < optCount; i++) {
        Rectangle rect{(float)(boxX + i*(btnW+3)), (float)y, (float)btnW, (float)boxH};
        bool sel = (selected == i);
        bool hov = CheckCollisionPointRec(mPos, rect);
        Color col = sel ? Color{60,140,100,255} : (hov ? Color{60,80,110,255} : Color{45,55,70,255});
        DrawRectangleRec(rect, col);
        DrawRectangleLines((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height, sel ? Color{100,220,140,255} : Color{80,80,80,255});
        int tw = MeasureText(options[i], 13);
        DrawText(options[i], (int)(rect.x + rect.width/2 - tw/2), (int)(rect.y + 8), 13, RAYWHITE);
        if(click && hov && selected != i) { selected = i; changed = true; }
    }
    y += boxH + 10;
}

static void DrawDifficultySelector(int &y, float uiCenterX, const char** options, int &selected, Vector2 mPos, bool click, bool &changed, int sw) {
    int boxW = 280, boxH = 30;
    int boxX = (int)(uiCenterX - boxW/2);
    if(boxX < 10) boxX = 10;
    if(boxX + boxW > sw - 10) boxX = sw - 10 - boxW;
    
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
    
    int btnW = (boxW - 2*3) / 3;
    for(int i = 0; i < 3; i++) {
        Rectangle rect{(float)(boxX + i*(btnW+3)), (float)y, (float)btnW, (float)boxH};
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
        int tw = MeasureText(options[i], 13);
        DrawText(options[i], (int)(rect.x + rect.width/2 - tw/2), (int)(rect.y + 8), 13, RAYWHITE);
        if(click && hov && selected != i) { selected = i; changed = true; }
    }
    y += boxH + 10;
}

void Game::DrawMenu(){
    BeginDrawing();
    ClearBackground(Color{12,16,24,255});
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    viewportRect = {0,0,(float)sw,(float)sh};
    float uiCenterX = sw / 2.f;
    Vector2 mPos = GetMousePosition();
    bool click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool drag = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    
    const char* title = "SHAPEY TOWER";
    int tw = MeasureText(title, 38);
    DrawText(title, (int)(uiCenterX - tw/2), 20, 38, RAYWHITE);
    DrawText("v1.5", sw - 45, sh - 22, 12, Color{70,70,70,255});
    
    int tabY = 72;
    int tabW = 80, tabH = 28;
    float tabStartX = uiCenterX - (5 * tabW + 4*3) / 2.f;
    DrawTabButton(tabStartX, tabY, tabW, tabH, Loc::Tab_Game(), 0, mPos, click);
    DrawTabButton(tabStartX + (tabW+3), tabY, tabW, tabH, Loc::Tab_Video(), 1, mPos, click);
    DrawTabButton(tabStartX + 2*(tabW+3), tabY, tabW, tabH, Loc::Tab_Audio(), 2, mPos, click);
    DrawTabButton(tabStartX + 3*(tabW+3), tabY, tabW, tabH, Loc::Tab_Keys(), 3, mPos, click);
    DrawTabButton(tabStartX + 4*(tabW+3), tabY, tabW, tabH, Loc::Tab_Effects(), 4, mPos, click);
    
    int contentY = tabY + tabH + 20;
    int y = contentY;
    bool settingsChanged = false;
    
    if(menuTab == 0) {
        DrawSectionHeader(y, uiCenterX, Loc::Menu_StartGame());
        y += 8;
        
        DrawText(Loc::Menu_Difficulty(), (int)(uiCenterX - 140), y, 13, Color{180,180,180,255});
        y += 18;
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
        int ddw = MeasureText(diffDesc, 11);
        DrawText(diffDesc, (int)(uiCenterX - ddw/2), y, 11, Color{130,130,150,255});
        y += 20;
        
        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, 260, 46, Loc::Menu_Play(), mPos, pressed);
        if(pressed) {
            state.isDailyRun = false;
            ResetGame();
            state.started = true;
            state.paused = false;
            ChangeScreen(GameState::Screen::GAME);
        }
        y += 16;
        
        DrawLine((int)(uiCenterX - 130), y, (int)(uiCenterX + 130), y, Color{60,80,120,150});
        y += 12;
        
        DailyChallenge today = state.dailyChallenge;
        
        const char* challengeName = GetChallengeName(today.type);
        char dailyTitle[128];
        snprintf(dailyTitle, sizeof(dailyTitle), ">> %s <<", challengeName);
        int ctw = MeasureText(dailyTitle, 16);
        DrawText(dailyTitle, (int)(uiCenterX - ctw/2), y, 16, Color{255,180,80,255});
        y += 22;
        
        const char* challengeDesc = GetChallengeDescription(today.type);
        int cdw = MeasureText(challengeDesc, 11);
        DrawText(challengeDesc, (int)(uiCenterX - cdw/2), y, 11, Color{150,150,170,255});
        y += 18;
        
        GuiButtonCentered(uiCenterX, y, 260, 40, Loc::Daily_Title(), mPos, pressed);
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
        int dtw = MeasureText(dateStr, 11);
        DrawText(dateStr, (int)(uiCenterX - dtw/2), y + 6, 11, Color{100,100,120,255});
        y += 22;
        
        if(today.bestScore > 0) {
            char dailyBest[64];
            snprintf(dailyBest, sizeof(dailyBest), "%s %d", Loc::Daily_Best(), today.bestScore);
            int dbw = MeasureText(dailyBest, 13);
            DrawText(dailyBest, (int)(uiCenterX - dbw/2), y, 13, Color{100,200,255,255});
            y += 18;
        }
        y += 10;
        
        DrawLine((int)(uiCenterX - 130), y, (int)(uiCenterX + 130), y, Color{60,80,120,150});
        y += 18;
        
        GuiButtonCentered(uiCenterX, y, 180, 36, Loc::Menu_Exit(), mPos, pressed);
        if(pressed) running = false;
        
        y += 40;
        char hsText[64];
        snprintf(hsText, sizeof(hsText), "%s %d", Loc::Menu_HighScore(), state.highScore);
        int hsw = MeasureText(hsText, 16);
        DrawText(hsText, (int)(uiCenterX - hsw/2), y, 16, Color{255,220,100,255});
    }
    else if(menuTab == 1) {
        DrawSectionHeader(y, uiCenterX, Loc::Video_Title());
        y += 8;
        
        DrawText(Loc::Video_Resolution(), (int)(uiCenterX - 140), y, 13, Color{180,180,180,255});
        y += 18;
        DrawResolutionSelector(y, uiCenterX, mPos, click, sw);
        y += 8;
        
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
        
        y += 5;
        DrawText(Loc::Video_FPSLimit(), (int)(uiCenterX - 140), y, 13, Color{180,180,180,255});
        y += 18;
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
        y += 8;
        
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
        
        y += 12;
        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, 180, 32, Loc::Audio_Default(), mPos, pressed);
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
        y += 8;
        
        enum RebindTarget { RB_NONE, RB_LEFT, RB_RIGHT, RB_JUMP };
        static RebindTarget rebindActive = RB_NONE;
        static float blinkTime = 0.f;
        blinkTime += GetFrameTime();
        float blinkAlpha = (std::sin(blinkTime * 6.f) * 0.5f + 0.5f);
        
        auto DrawKeyBind = [&](const char* label, int &key, RebindTarget target) {
            int boxW = 280, boxH = 36;
            int boxX = (int)(uiCenterX - boxW/2);
            
            Rectangle rect{(float)boxX, (float)y, (float)boxW, (float)boxH};
            bool hovered = CheckCollisionPointRec(mPos, rect);
            bool active = (rebindActive == target);
            
            Color bgCol = active ? Color{80,60,100,255} : (hovered ? Color{50,60,80,255} : Color{40,50,65,255});
            if(active) bgCol.a = (unsigned char)(180 + 75 * blinkAlpha);
            DrawRectangleRec(rect, bgCol);
            DrawRectangleLines(boxX, y, boxW, boxH, active ? Color{180,140,255,255} : Color{80,80,80,255});
            
            DrawText(label, boxX + 12, y + 10, 15, RAYWHITE);
            
            const char* keyName = active ? "..." : KeyName(key);
            int knw = MeasureText(keyName, 16);
            DrawRectangle(boxX + boxW - knw - 25, y + 7, knw + 16, 22, Color{30,35,45,255});
            DrawText(keyName, boxX + boxW - knw - 17, y + 10, 16, active ? Color{255,200,100,255} : Color{150,200,255,255});
            
            if(click && hovered) {
                rebindActive = (rebindActive == target) ? RB_NONE : target;
            }
            
            y += boxH + 6;
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
            DrawText(Loc::Keys_PressKey(), (int)(uiCenterX - 60), y + 10, 14, Color{255,200,100,255});
        }
        
        y += 25;
        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, 180, 32, Loc::Keys_Default(), mPos, pressed);
        if(pressed) {
            state.keys.left = KEY_A;
            state.keys.right = KEY_D;
            state.keys.jump = KEY_SPACE;
            settingsDirty = true;
        }
    }
    else if(menuTab == 4) {
        DrawSectionHeader(y, uiCenterX, Loc::Effects_Title());
        y += 8;
        
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
        
        y += 25;
        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, 180, 32, Loc::Effects_ResetAll(), mPos, pressed);
        if(pressed) {
            ResetSettingsToDefaults();
        }
    }
    
    state.audio.masterVolume = 0.0001f + state.audio.masterSlider * 1.0f;
    SetMasterVolume(state.audio.masterVolume);
    if(state.audio.musicBg.ctxData) {
        SetMusicVolume(state.audio.musicBg, state.audio.volMusic * VOL_MUSIC_MULT);
    }
    
    // Language selector box in bottom-right corner
    {
        static bool langBoxOpen = false;
        int boxW = 90, boxH = 26;
        int boxX = sw - boxW - 10;
        int boxY = sh - boxH - 45;
        
        const char* langLabel = (settings.language == 0) ? "EN" : "PL";
        Rectangle boxRect{(float)boxX, (float)boxY, (float)boxW, (float)boxH};
        bool hovered = CheckCollisionPointRec(mPos, boxRect);
        
        DrawRectangleRec(boxRect, hovered ? Color{60,70,90,255} : Color{40,50,65,255});
        DrawRectangleLines(boxX, boxY, boxW, boxH, Color{80,100,130,255});
        
        // Globe icon (simple circle with lines)
        int iconX = boxX + 12;
        int iconY = boxY + boxH/2;
        DrawCircleLines(iconX, iconY, 8, Color{150,180,220,255});
        DrawLine(iconX - 8, iconY, iconX + 8, iconY, Color{150,180,220,255});
        DrawLine(iconX, iconY - 8, iconX, iconY + 8, Color{150,180,220,255});
        
        int ltw = MeasureText(langLabel, 14);
        DrawText(langLabel, boxX + 30, boxY + 6, 14, RAYWHITE);
        
        // Dropdown arrow
        int arrowX = boxX + boxW - 18;
        int arrowY = boxY + boxH/2;
        if(langBoxOpen) {
            DrawTriangle({(float)(arrowX-5), (float)(arrowY+3)}, {(float)(arrowX+5), (float)(arrowY+3)}, {(float)arrowX, (float)(arrowY-4)}, Color{180,180,180,255});
        } else {
            DrawTriangle({(float)(arrowX-5), (float)(arrowY-3)}, {(float)(arrowX+5), (float)(arrowY-3)}, {(float)arrowX, (float)(arrowY+4)}, Color{180,180,180,255});
        }
        
        if(click && hovered) langBoxOpen = !langBoxOpen;
        
        // Dropdown options
        if(langBoxOpen) {
            int optH = 28;
            int dropY = boxY - optH * 2 - 4;
            
            // English option
            Rectangle optEN{(float)boxX, (float)dropY, (float)boxW, (float)optH};
            bool hovEN = CheckCollisionPointRec(mPos, optEN);
            DrawRectangleRec(optEN, hovEN ? Color{70,90,120,255} : Color{50,60,80,255});
            DrawRectangleLines(boxX, dropY, boxW, optH, Color{80,100,130,255});
            DrawText("English", boxX + 10, dropY + 6, 14, settings.language == 0 ? Color{100,200,150,255} : RAYWHITE);
            if(click && hovEN) {
                settings.language = 0;
                Loc::SetLanguage(Language::EN);
                settingsDirty = true;
                langBoxOpen = false;
            }
            
            // Polski option
            Rectangle optPL{(float)boxX, (float)(dropY + optH + 2), (float)boxW, (float)optH};
            bool hovPL = CheckCollisionPointRec(mPos, optPL);
            DrawRectangleRec(optPL, hovPL ? Color{70,90,120,255} : Color{50,60,80,255});
            DrawRectangleLines(boxX, dropY + optH + 2, boxW, optH, Color{80,100,130,255});
            DrawText("Polski", boxX + 10, dropY + optH + 8, 14, settings.language == 1 ? Color{100,200,150,255} : RAYWHITE);
            if(click && hovPL) {
                settings.language = 1;
                Loc::SetLanguage(Language::PL);
                settingsDirty = true;
                langBoxOpen = false;
            }
            
            // Close dropdown if clicked elsewhere
            if(click && !hovered && !hovEN && !hovPL) langBoxOpen = false;
        }
    }
    
    DrawText(Loc::Settings_TabHint(), (int)(uiCenterX - 70), sh - 35, 11, Color{70,70,90,255});
    
    if(IsKeyPressed(KEY_TAB)) {
        menuTab = (menuTab + 1) % 5;
    }
    
    EndDrawing();
}
