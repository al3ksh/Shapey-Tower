#include "game.h"
#include "raylib.h"
#include "input.h"
#include "localization.h"
#include "ui_helpers.h"
#include <algorithm>
#include <cmath>

static int pauseTab = 0;
static float uiScale = 1.0f;

static int S(int base) { return (int)(base * uiScale); }

enum PauseRebindTarget { PRB_NONE, PRB_LEFT, PRB_RIGHT, PRB_JUMP };
static PauseRebindTarget pauseRebindActive = PRB_NONE;
static float pauseBlinkTime = 0.f;

void Game::DrawPause(){
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    uiScale = sh / 720.0f;
    if(uiScale < 1.0f) uiScale = 1.0f;
    int P = Ui::UnitFor(uiScale);
    int kB = Ui::TextKFor(uiScale);
    int kS = std::max(2, kB - 1);
    Ui::SetUnit(P);

    // Frozen game world behind the pause panel
    EnsureRenderTarget();
    BeginTextureMode(gameRT);
    ClearBackground(BLACK);
    DrawGameWorld(0.f);
    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);
    PresentGameRT();
    DrawRectangle(0, 0, sw, sh, Color{8, 8, 20, 170});

    viewportRect = {0,0,(float)sw,(float)sh};
    float uiCenterX = sw / 2.f;
    Vector2 mPos = GetMousePosition();
    bool click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool drag = IsMouseButtonDown(MOUSE_LEFT_BUTTON);

    const char* title = Loc::Pause_Title();
    int kT = std::max(3, (int)(3.6f * uiScale + 0.5f));
    int tlen = (int)TextLength(title);
    int tw = Ui::Measure(title, kT) + (tlen - 1) * kT;
    Ui::FancyText(title, uiCenterX - tw / 2.f, (float)S(18), kT, Color{220, 236, 255, 255}, Color{110, 160, 240, 255},
                  Color{10, 12, 30, 255}, 1.f, (float)GetTime());

    const Ui::Icon tabIcons[5] = {Ui::ICON_PAD, Ui::ICON_MONITOR, Ui::ICON_SPEAKER, Ui::ICON_KEY, Ui::ICON_SPARK};
    int tabY = S(78);
    int tabGap = 2 * P;
    int tabW = std::min(S(56), (sw - S(16) - 4 * tabGap) / 5), tabH = S(38);
    float tabStartX = std::floor(uiCenterX - (5 * tabW + 4 * tabGap) / 2.f);
    for(int i = 0; i < 5; i++) {
        Rectangle r{tabStartX + i * (tabW + tabGap), (float)tabY, (float)tabW, (float)tabH};
        if(Ui::IconButton(r, tabIcons[i], mPos, click, pauseTab == i)) pauseTab = i;
    }

    int panelTop = tabY + tabH + 3 * P;
    int panelW = std::min(sw - S(16), S(380));
    static int contentBottom[8] = {0};
    int maxPanelH = sh - panelTop - S(48);
    int panelH = (true && contentBottom[pauseTab] > 0) ? std::min(maxPanelH, contentBottom[pauseTab] - panelTop + S(10)) : maxPanelH;
    Rectangle panel{std::floor(uiCenterX - panelW / 2.f), (float)panelTop, (float)panelW, (float)panelH};
    Ui::Panel(panel, P, true, 235);

    int y = panelTop + S(14);
    bool settingsChanged = false;

    if(pauseTab == 0) {
        Ui::DrawSectionHeader(y, uiCenterX, Loc::Tab_Game(), uiScale);

        // Current run summary
        {
            int boxW = std::min(S(300), panelW - S(24));
            Rectangle box{std::floor(uiCenterX - boxW / 2.f), (float)y, (float)boxW, (float)(Ui::GlyphHeight(kB + 1) + Ui::GlyphHeight(kS) + 9 * P)};
            Ui::Inset(box, P, {14, 16, 30, 255});
            float colL = box.x + box.width * 0.3f, colR = box.x + box.width * 0.72f;
            float ty = box.y + 3 * P;
            Ui::TextCentered(TextFormat("%d", state.score), colL, ty, kB + 1, Color{255, 220, 140, 255});
            const char* coins = TextFormat("%d", state.globalCoins);
            int cw = Ui::Measure(coins, kB + 1);
            int cs = 16 * std::max(1, P / 2);
            float cx0 = std::floor(colR - (cs + 2 * P + cw) / 2.f);
            if(worldArt.items.id > 0)
                DrawTexturePro(worldArt.items, {0, 0, 16, 16}, {cx0, ty + Ui::GlyphHeight(kB + 1) / 2.f - cs / 2.f, (float)cs, (float)cs}, {0, 0}, 0.f, WHITE);
            Ui::Text(coins, cx0 + cs + 2 * P, ty, kB + 1, Color{255, 214, 80, 255});
            ty += Ui::GlyphHeight(kB + 1) + 3 * P;
            Ui::TextCentered(Loc::Pause_Score(), colL, ty, kS, Color{130, 136, 166, 255});
            Ui::TextCentered(Loc::GameOver_Coins(), colR, ty, kS, Color{130, 136, 166, 255});
            y += (int)box.height + 6 * P;
        }

        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, S(250), S(48), Loc::Pause_Resume(), mPos, pressed, Ui::STYLE_GREEN);
        if(pressed) { state.paused = false; ChangeScreen(GameState::Screen::GAME); }

        GuiButtonCentered(uiCenterX, y, S(250), S(40), Loc::Pause_Restart(), mPos, pressed);
        if(pressed) { ResetGame(); ChangeScreen(GameState::Screen::GAME, false); }

        GuiButtonCentered(uiCenterX, y, S(250), S(40), Loc::Pause_MainMenu(), mPos, pressed);
        if(pressed) { ChangeScreen(GameState::Screen::MENU); }

        GuiButtonCentered(uiCenterX, y, S(200), S(36), Loc::Pause_Exit(), mPos, pressed, Ui::STYLE_RED);
        if(pressed) { running = false; }
    }
    else if(pauseTab == 1) {
        Ui::DrawSectionHeader(y, uiCenterX, Loc::Video_Title(), uiScale);

        Ui::Text(Loc::Video_Resolution(), (float)Ui::RowX(uiCenterX, S(300), sw, uiScale), (float)y, kS, Color{150, 156, 184, 255});
        y += Ui::GlyphHeight(kS) + 2 * P;
        DrawResolutionSelector(y, uiCenterX, mPos, click, sw, uiScale);
        y += 2 * P;

        Ui::DrawToggle(y, uiCenterX, Loc::Video_Fullscreen(), fullscreen, mPos, click, settingsChanged, sw, uiScale);
        if(settingsChanged) { ApplyResolution(false); settingsChanged = false; settingsDirty = true; }

        bool vsyncChanged = false;
        Ui::DrawToggle(y, uiCenterX, Loc::Video_VSync(), settings.vsync, mPos, click, vsyncChanged, sw, uiScale);
        if(vsyncChanged) {
            if(settings.vsync) SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
            else SetTargetFPS(settings.targetFPS);
            settingsDirty = true;
        }

        y += 2 * P;
        Ui::Text(Loc::Video_FPSLimit(), (float)Ui::RowX(uiCenterX, S(300), sw, uiScale), (float)y, kS, Color{150, 156, 184, 255});
        y += Ui::GlyphHeight(kS) + 2 * P;
        const char* fpsOptions[] = {"30", "60", "120", "144", "Max"};
        int fpsValues[] = {30, 60, 120, 144, 0};
        int fpsIndex = 1;
        for(int i = 0; i < 5; i++) {
            if(fpsValues[i] == settings.targetFPS) { fpsIndex = i; break; }
        }
        int boxW = S(300), boxH = S(36);
        int boxX = Ui::RowX(uiCenterX, boxW, sw, uiScale);
        int gap = 2 * P;
        int btnW = (boxW - 4 * gap) / 5;
        for(int i = 0; i < 5; i++) {
            Rectangle rect{(float)(boxX + i*(btnW+gap)), (float)y, (float)btnW, (float)boxH};
            bool sel = (fpsIndex == i);
            if(Ui::Button(rect, fpsOptions[i], mPos, click, sel ? Ui::STYLE_BLUE : Ui::STYLE_DARK, kB) && !sel) {
                settings.targetFPS = fpsValues[i];
                if(!settings.vsync) SetTargetFPS(settings.targetFPS);
                settingsDirty = true;
            }
        }
        y += boxH + 4 * P;

        Ui::DrawToggle(y, uiCenterX, Loc::Video_ShowFPS(), settings.showFPS, mPos, click, settingsChanged, sw, uiScale);
        if(settingsChanged) { settingsDirty = true; settingsChanged = false; }
    }
    else if(pauseTab == 2) {
        Ui::DrawSectionHeader(y, uiCenterX, Loc::Audio_Title(), uiScale);

        Ui::DrawSlider(y, uiCenterX, Loc::Audio_Master(), state.audio.masterSlider, mPos, drag, settingsChanged, sw, uiScale);
        Ui::DrawSlider(y, uiCenterX, Loc::Audio_Music(), state.audio.volMusic, mPos, drag, settingsChanged, sw, uiScale);
        Ui::DrawSlider(y, uiCenterX, Loc::Audio_Jump(), state.audio.volJump, mPos, drag, settingsChanged, sw, uiScale);
        Ui::DrawSlider(y, uiCenterX, Loc::Audio_Bounce(), state.audio.volBounce, mPos, drag, settingsChanged, sw, uiScale);
        Ui::DrawSlider(y, uiCenterX, Loc::Audio_Death(), state.audio.volDeath, mPos, drag, settingsChanged, sw, uiScale);
        Ui::DrawSlider(y, uiCenterX, Loc::Audio_Coin(), state.audio.volCoin, mPos, drag, settingsChanged, sw, uiScale);
        Ui::DrawSlider(y, uiCenterX, Loc::Audio_PowerUp(), state.audio.volPowerUp, mPos, drag, settingsChanged, sw, uiScale);

        if(settingsChanged) {
            settingsDirty = true;
            settingsSaveTimer = 0.f;
            ApplyAudioVolumes();
        }

        y += 2 * P;
        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, S(180), S(34), Loc::Audio_Default(), mPos, pressed, Ui::STYLE_DARK);
        if(pressed) {
            state.audio.masterSlider = 0.5f;
            state.audio.volMusic = 0.5f;
            state.audio.volJump = 0.5f;
            state.audio.volBounce = 0.5f;
            state.audio.volDeath = 0.5f;
            state.audio.volThemeChange = 0.5f;
            state.audio.volCoin = 0.5f;
            state.audio.volPowerUp = 0.5f;
            ApplyAudioVolumes();
            settingsDirty = true;
        }
    }
    else if(pauseTab == 3) {
        Ui::DrawSectionHeader(y, uiCenterX, Loc::Keys_Title(), uiScale);

        pauseBlinkTime += GetFrameTime();
        float blinkAlpha = (std::sin(pauseBlinkTime * 6.f) * 0.5f + 0.5f);

        auto keyRow = [&](const char* label, int key, PauseRebindTarget target) {
            if(Ui::DrawKeyRow(y, uiCenterX, label, KeyName(key), pauseRebindActive == target, blinkAlpha, mPos, click, sw, uiScale))
                pauseRebindActive = (pauseRebindActive == target) ? PRB_NONE : target;
        };
        keyRow(Loc::Keys_MoveLeft(), state.keys.left, PRB_LEFT);
        keyRow(Loc::Keys_MoveRight(), state.keys.right, PRB_RIGHT);
        keyRow(Loc::Keys_Jump(), state.keys.jump, PRB_JUMP);

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
            Ui::TextCentered(Loc::Keys_PressKey(), uiCenterX, (float)(y + 2 * P), kB, Color{255, 200, 100, (unsigned char)(150 + 105 * blinkAlpha)});
        }

        y += S(34);
        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, S(180), S(34), Loc::Keys_Default(), mPos, pressed, Ui::STYLE_DARK);
        if(pressed) {
            state.keys.left = KEY_A;
            state.keys.right = KEY_D;
            state.keys.jump = KEY_SPACE;
            settingsDirty = true;
        }
    }
    else if(pauseTab == 4) {
        Ui::DrawSectionHeader(y, uiCenterX, Loc::Effects_Title(), uiScale);

        bool shakeChanged = false;
        Ui::DrawToggle(y, uiCenterX, Loc::Effects_ScreenShake(), settings.screenShake, mPos, click, shakeChanged, sw, uiScale);
        if(shakeChanged) settingsDirty = true;

        bool partChanged = false;
        Ui::DrawToggle(y, uiCenterX, Loc::Effects_Particles(), settings.particles, mPos, click, partChanged, sw, uiScale);
        if(partChanged) settingsDirty = true;

        bool comboChanged = false;
        Ui::DrawToggle(y, uiCenterX, Loc::Effects_ComboFire(), settings.comboEffects, mPos, click, comboChanged, sw, uiScale);
        if(comboChanged) settingsDirty = true;

        bool powerUpChanged = false;
        Ui::DrawToggle(y, uiCenterX, Loc::Effects_PowerUp(), settings.powerUpEffects, mPos, click, powerUpChanged, sw, uiScale);
        if(powerUpChanged) settingsDirty = true;
    }

    contentBottom[pauseTab] = y;
    ApplyMenuAudioVolumes();

    Ui::TextCentered(Loc::Pause_EscResume(), uiCenterX, (float)(sh - S(30)), kS, Color{120, 126, 156, 220});

    if(IsKeyPressed(KEY_TAB)) {
        pauseTab = (pauseTab + 1) % 5;
    }

    EndDrawing();
}
