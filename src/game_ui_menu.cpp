#include "game.h"
#include "debug.h"
#include "difficulty.h"
#include "daily_challenge.h"
#include "persistence.h"
#include "input.h"
#include "localization.h"
#include "ui_helpers.h"
#include "tutorial.h"
#include "leaderboard.h"
#include "stats.h"
#include <cmath>
#include <algorithm>

static int menuTab = 0;
static float uiScale = 1.0f;
static int P = 2;       // pixel unit for frames/icons
static int kB = 2;      // body text scale
static int kS = 1;      // small text scale

#ifndef SHAPEY_VERSION
#define SHAPEY_VERSION "dev"
#endif

static int S(int base) { return (int)(base * uiScale); }

static const Color kGold{255, 214, 110, 255};
static const Color kMuted{150, 156, 184, 255};
static const Color kDim{104, 110, 138, 255};

static void DrawSelector(int &y, float uiCenterX, const char** options, int optCount, int &selected, Vector2 mPos, bool click, bool &changed, int sw, const Ui::Style *selStyles = nullptr) {
    int boxW = S(300), boxH = S(36);
    int boxX = Ui::RowX(uiCenterX, boxW, sw, uiScale);
    int gap = 2 * P;
    int btnW = (boxW - (optCount-1)*gap) / optCount;
    for(int i = 0; i < optCount; i++) {
        Rectangle rect{(float)(boxX + i*(btnW+gap)), (float)y, (float)btnW, (float)boxH};
        bool sel = (selected == i);
        Ui::Style st = sel ? (selStyles ? selStyles[i] : Ui::STYLE_BLUE) : Ui::STYLE_DARK;
        if(Ui::Button(rect, options[i], mPos, click, st, kB) && !sel) { selected = i; changed = true; }
    }
    y += boxH + 4 * P;
}

// Coin sprite + amount, centered on cx.
static void DrawCoinAmount(const Texture2D &items, float cx, float y, int amount, int k, int p) {
    const char *txt = TextFormat("%d", amount);
    int tw = Ui::Measure(txt, k);
    int cs = 16 * std::max(1, p / 2);
    float x = std::floor(cx - (cs + 3 * p + tw) / 2.f);
    if(items.id > 0)
        DrawTexturePro(items, {0, 0, 16, 16}, {x, std::floor(y + Ui::GlyphHeight(k) / 2.f - cs / 2.f), (float)cs, (float)cs}, {0, 0}, 0.f, WHITE);
    Ui::Text(txt, x + cs + 3 * p, y, k, kGold);
}

void Game::DrawMenu(){
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    uiScale = sh / 720.0f;
    if(uiScale < 1.0f) uiScale = 1.0f;
    P = Ui::UnitFor(uiScale);
    kB = Ui::TextKFor(uiScale);
    kS = std::max(2, kB - 1);
    Ui::SetUnit(P);
    float time = (float)GetTime();

    RenderMenuScene(time);

    BeginDrawing();
    ClearBackground(BLACK);
    PresentGameRT();
    DrawRectangleGradientV(0, 0, sw, sh, Color{6, 6, 18, 30}, Color{6, 6, 18, 120});

    viewportRect = {0,0,(float)sw,(float)sh};
    float uiCenterX = sw / 2.f;
    Vector2 mPos = GetMousePosition();
    bool click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool drag = IsMouseButtonDown(MOUSE_LEFT_BUTTON);

    // Logo
    {
        const char* title = "SHAPEY TOWER";
        int kT = std::max(3, (int)(3.6f * uiScale + 0.5f));
        int glyphs = 0;
        for(const char* c = title; *c; ++c) glyphs++;
        // FancyText uses 2-unit letter spacing; approximate width for centering
        int tw = Ui::Measure(title, kT) + (glyphs - 1) * kT;
        while(tw > sw - S(24) && kT > 2) { kT--; tw = Ui::Measure(title, kT) + (glyphs - 1) * kT; }
        Ui::FancyText(title, uiCenterX - tw / 2.f, (float)S(18), kT, Color{255, 236, 150, 255}, Color{240, 140, 50, 255},
                      Color{24, 10, 30, 255}, 1.f, time);
    }

    // Icon tabs
    const Ui::Icon tabIcons[7] = {Ui::ICON_PAD, Ui::ICON_MONITOR, Ui::ICON_SPEAKER, Ui::ICON_KEY, Ui::ICON_SPARK, Ui::ICON_TROPHY, Ui::ICON_HOOD};
    int tabY = S(78);
    int tabGap = 2 * P;
    int tabW = std::min(S(50), (sw - S(16) - 6 * tabGap) / 7), tabH = S(38);
    float tabStartX = std::floor(uiCenterX - (7 * tabW + 6 * tabGap) / 2.f);
    for(int i = 0; i < 7; i++) {
        Rectangle r{tabStartX + i * (tabW + tabGap), (float)tabY, (float)tabW, (float)tabH};
        if(Ui::IconButton(r, tabIcons[i], mPos, click, menuTab == i)) menuTab = i;
    }

    // Content panel
    int panelTop = tabY + tabH + 3 * P;
    int panelW = std::min(sw - S(16), S(380));
    static int contentBottom[8] = {0};
    int maxPanelH = sh - panelTop - S(48);
    int panelH = (menuTab != 5 && contentBottom[menuTab] > 0) ? std::min(maxPanelH, contentBottom[menuTab] - panelTop + S(10)) : maxPanelH;
    Rectangle panel{std::floor(uiCenterX - panelW / 2.f), (float)panelTop, (float)panelW, (float)panelH};
    Ui::Panel(panel, P, true, 230);

    int y = panelTop + S(14);
    bool settingsChanged = false;

    if(menuTab == 0) {
        Ui::DrawSectionHeader(y, uiCenterX, Loc::Menu_StartGame(), uiScale);

        Ui::TextCentered(Loc::Menu_Difficulty(), uiCenterX, (float)y, kS, kMuted);
        y += Ui::GlyphHeight(kS) + 3 * P;
        const char* diffNames[] = {Loc::Menu_Easy(), Loc::Menu_Normal(), Loc::Menu_Hard()};
        const Ui::Style diffStyles[] = {Ui::STYLE_GREEN, Ui::STYLE_GOLD, Ui::STYLE_RED};
        int diffInt = (int)state.difficulty;
        bool diffChanged = false;
        DrawSelector(y, uiCenterX, diffNames, 3, diffInt, mPos, click, diffChanged, sw, diffStyles);
        if(diffChanged) {
            state.difficulty = (Difficulty)diffInt;
            settingsDirty = true;
        }
        const char* diffDesc[] = {Loc::Menu_EasyDesc(), Loc::Menu_NormalDesc(), Loc::Menu_HardDesc()};
        Ui::TextCentered(diffDesc[diffInt], uiCenterX, (float)y, kS, kDim);
        y += Ui::GlyphHeight(kS) + 5 * P;

        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, S(260), S(50), Loc::Menu_Play(), mPos, pressed, Ui::STYLE_GREEN, kB + 1);
        if(pressed) {
            state.isDailyRun = false;
            ResetGame();
            state.started = true;
            state.paused = false;
            if(!LoadTutorialDone("tutorial_done.txt")) {
                StartTutorial(state.tutorial);
            }
            ChangeScreen(GameState::Screen::GAME);
        }
        y += P;
        Ui::Divider(uiCenterX, (float)y, (float)S(130), P);
        y += 5 * P;

        DailyChallenge today = state.dailyChallenge;
        const char* challengeName = GetChallengeName(today.type);
        {
            int k = Ui::FitK(challengeName, kB, panelW - S(60));
            int ctw = Ui::Measure(challengeName, k);
            int iw = Ui::IconW(Ui::ICON_CALENDAR) * P;
            float x0 = std::floor(uiCenterX - (iw + 3 * P + ctw) / 2.f);
            Ui::DrawIcon(Ui::ICON_CALENDAR, x0, y + Ui::GlyphHeight(k) / 2.f - Ui::IconH(Ui::ICON_CALENDAR) * P / 2.f, P);
            Ui::Text(challengeName, x0 + iw + 3 * P, (float)y, k, Color{255, 170, 80, 255});
            y += std::max(Ui::GlyphHeight(k), Ui::IconH(Ui::ICON_CALENDAR) * P) + 3 * P;
        }
        const char* challengeDesc = GetChallengeDescription(today.type);
        Ui::TextCentered(challengeDesc, uiCenterX, (float)y, Ui::FitK(challengeDesc, kS, panelW - S(20)), kMuted);
        y += Ui::GlyphHeight(kS) + 4 * P;

        GuiButtonCentered(uiCenterX, y, S(260), S(42), Loc::Daily_Title(), mPos, pressed);
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
        y -= 2 * P;

        char dateStr[64];
        snprintf(dateStr, sizeof(dateStr), "%s %04d-%02d-%02d", Loc::Menu_Today(), today.year, today.month, today.day);
        if(today.bestScore > 0) {
            char withBest[128];
            snprintf(withBest, sizeof(withBest), "%s   %s %d", dateStr, Loc::Daily_Best(), today.bestScore);
            Ui::TextCentered(withBest, uiCenterX, (float)y, kS, Color{110, 190, 255, 255});
        } else {
            Ui::TextCentered(dateStr, uiCenterX, (float)y, kS, kDim);
        }
        y += Ui::GlyphHeight(kS) + 5 * P;
        Ui::Divider(uiCenterX, (float)y, (float)S(130), P);
        y += 5 * P;

        // Coins and best score side by side
        {
            char hsText[64];
            snprintf(hsText, sizeof(hsText), "%d", state.highScore);
            int iw = Ui::IconW(Ui::ICON_CROWN) * P;
            int hw = Ui::Measure(hsText, kB);
            float colL = uiCenterX - S(70), colR = uiCenterX + S(70);
            DrawCoinAmount(worldArt.items, colL, (float)y, state.globalCoins, kB, P);
            float hx = std::floor(colR - (iw + 3 * P + hw) / 2.f);
            Ui::DrawIcon(Ui::ICON_CROWN, hx, y + Ui::GlyphHeight(kB) / 2.f - Ui::IconH(Ui::ICON_CROWN) * P / 2.f, P);
            Ui::Text(hsText, hx + iw + 3 * P, (float)y, kB, kGold);
            y += Ui::GlyphHeight(kB) + 2 * P;
            Ui::TextCentered(Loc::GameOver_Coins(), colL, (float)y, kS, kDim);
            Ui::TextCentered(Loc::Menu_HighScore(), colR, (float)y, kS, kDim);
            y += Ui::GlyphHeight(kS) + 5 * P;
        }

        GuiButtonCentered(uiCenterX, y, S(170), S(36), Loc::Menu_Exit(), mPos, pressed, Ui::STYLE_RED);
        if(pressed) running = false;
    }
    else if(menuTab == 1) {
        Ui::DrawSectionHeader(y, uiCenterX, Loc::Video_Title(), uiScale);

        Ui::Text(Loc::Video_Resolution(), (float)Ui::RowX(uiCenterX, S(300), sw, uiScale), (float)y, kS, kMuted);
        y += Ui::GlyphHeight(kS) + 2 * P;
        DrawResolutionSelector(y, uiCenterX, mPos, click, sw, uiScale);
        y += 2 * P;

        Ui::DrawToggle(y, uiCenterX, Loc::Video_Fullscreen(), fullscreen, mPos, click, settingsChanged, sw, uiScale);
        if(settingsChanged) { ApplyResolution(false); settingsChanged = false; settingsDirty = true; }

        bool vsyncChanged = false;
        Ui::DrawToggle(y, uiCenterX, Loc::Video_VSync(), settings.vsync, mPos, click, vsyncChanged, sw, uiScale);
        if(vsyncChanged) {
            if(settings.vsync) {
                SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
            } else {
                SetTargetFPS(settings.targetFPS);
            }
            settingsDirty = true;
        }

        y += 2 * P;
        Ui::Text(Loc::Video_FPSLimit(), (float)Ui::RowX(uiCenterX, S(300), sw, uiScale), (float)y, kS, kMuted);
        y += Ui::GlyphHeight(kS) + 2 * P;
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

        Ui::DrawToggle(y, uiCenterX, Loc::Video_ShowFPS(), settings.showFPS, mPos, click, settingsChanged, sw, uiScale);
        if(settingsChanged) { settingsDirty = true; settingsChanged = false; }
    }
    else if(menuTab == 2) {
        Ui::DrawSectionHeader(y, uiCenterX, Loc::Audio_Title(), uiScale);

        Ui::DrawSlider(y, uiCenterX, Loc::Audio_Master(), state.audio.masterSlider, mPos, drag, settingsChanged, sw, uiScale);
        Ui::DrawSlider(y, uiCenterX, Loc::Audio_Music(), state.audio.volMusic, mPos, drag, settingsChanged, sw, uiScale);
        Ui::DrawSlider(y, uiCenterX, Loc::Audio_Jump(), state.audio.volJump, mPos, drag, settingsChanged, sw, uiScale);
        Ui::DrawSlider(y, uiCenterX, Loc::Audio_Bounce(), state.audio.volBounce, mPos, drag, settingsChanged, sw, uiScale);
        Ui::DrawSlider(y, uiCenterX, Loc::Audio_Death(), state.audio.volDeath, mPos, drag, settingsChanged, sw, uiScale);
        Ui::DrawSlider(y, uiCenterX, Loc::Audio_ThemeChange(), state.audio.volThemeChange, mPos, drag, settingsChanged, sw, uiScale);
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
    else if(menuTab == 3) {
        Ui::DrawSectionHeader(y, uiCenterX, Loc::Keys_Title(), uiScale);

        enum RebindTarget { RB_NONE, RB_LEFT, RB_RIGHT, RB_JUMP };
        static RebindTarget rebindActive = RB_NONE;
        static float blinkTime = 0.f;
        blinkTime += GetFrameTime();
        float blinkAlpha = (std::sin(blinkTime * 6.f) * 0.5f + 0.5f);

        auto keyRow = [&](const char* label, int key, RebindTarget target) {
            if(Ui::DrawKeyRow(y, uiCenterX, label, KeyName(key), rebindActive == target, blinkAlpha, mPos, click, sw, uiScale))
                rebindActive = (rebindActive == target) ? RB_NONE : target;
        };
        keyRow(Loc::Keys_MoveLeft(), state.keys.left, RB_LEFT);
        keyRow(Loc::Keys_MoveRight(), state.keys.right, RB_RIGHT);
        keyRow(Loc::Keys_Jump(), state.keys.jump, RB_JUMP);

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
    else if(menuTab == 4) {
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

        y += S(20);
        bool pressed = false;
        GuiButtonCentered(uiCenterX, y, S(200), S(34), Loc::Effects_ResetAll(), mPos, pressed, Ui::STYLE_RED);
        if(pressed) {
            ResetSettingsToDefaults();
        }
    }
    else if(menuTab == 6) {
        DrawHeroTab(y, uiCenterX, mPos, click);
    }
    else if(menuTab == 5) {
        static int statsSubTab = 0;
        int subW = std::min(S(100), (panelW - S(24)) / 3), subH = S(30);
        int subGap = 2 * P;
        float subStartX = std::floor(uiCenterX - (3 * subW + 2 * subGap) / 2.f);
        if(Ui::DrawTabButton(subStartX, y, subW, subH, Loc::Stats_SubAchievements(), 0, statsSubTab, mPos, click, uiScale)) statsSubTab = 0;
        if(Ui::DrawTabButton(subStartX + (subW+subGap), y, subW, subH, Loc::Stats_SubLeaderboard(), 1, statsSubTab, mPos, click, uiScale)) statsSubTab = 1;
        if(Ui::DrawTabButton(subStartX + 2*(subW+subGap), y, subW, subH, Loc::Stats_SubStats(), 2, statsSubTab, mPos, click, uiScale)) statsSubTab = 2;
        y += subH + 5 * P;

        int rowW = std::min(S(320), panelW - S(24));
        int rowX = (int)std::floor(uiCenterX - rowW / 2.f);
        int listBottom = (int)(panel.y + panel.height) - S(14);

        if(statsSubTab == 0) {
            int achUnlocked = 0;
            for (auto &ach : state.achievements) if (ach.unlocked) achUnlocked++;
            char achCount[64];
            snprintf(achCount, sizeof(achCount), "%d/%d %s", achUnlocked, (int)state.achievements.size(), Loc::Stats_AchUnlocked());
            Ui::TextCentered(achCount, uiCenterX, (float)y, kB, kGold);
            y += Ui::GlyphHeight(kB) + 3 * P;
            Ui::Bar({(float)rowX, (float)y, (float)rowW, (float)(4 * P)}, P,
                    state.achievements.empty() ? 0.f : (float)achUnlocked / state.achievements.size(), Color{232, 184, 72, 255});
            y += 7 * P;

            int rowH = Ui::GlyphHeight(kB) + Ui::GlyphHeight(kS) + 7 * P;
            for (auto &ach : state.achievements) {
                if(y + rowH > listBottom) break;
                Rectangle row{(float)rowX, (float)y, (float)rowW, (float)rowH};
                if(ach.unlocked) Ui::Frame(row, P, {30, 52, 44, 255}, {62, 104, 80, 255}, {16, 30, 26, 255}, {6, 6, 14, 255});
                else Ui::Frame(row, P, {28, 30, 46, 255}, {44, 48, 70, 255}, {16, 18, 30, 255}, {6, 6, 14, 255});
                Ui::Icon ic = ach.unlocked ? Ui::ICON_STAR : Ui::ICON_LOCK;
                Ui::DrawIconCentered(ic, row.x + 9 * P, row.y + rowH / 2.f, P, ach.unlocked ? WHITE : Color{120, 120, 140, 255});
                float tx = row.x + 18 * P;
                int maxW = rowW - 21 * P;
                Ui::Text(ach.name.c_str(), tx, row.y + 3 * P, Ui::FitK(ach.name.c_str(), kB, maxW), ach.unlocked ? RAYWHITE : kDim);
                Ui::Text(ach.description.c_str(), tx, row.y + 4 * P + Ui::GlyphHeight(kB), Ui::FitK(ach.description.c_str(), kS, maxW),
                         ach.unlocked ? Color{170, 200, 180, 255} : Color{80, 84, 104, 255});
                y += rowH + 2 * P;
            }
        }
        else if(statsSubTab == 1) {
            Ui::TextCentered(Loc::Stats_LBHeader(), uiCenterX, (float)y, kS, Color{110, 170, 230, 255});
            y += Ui::GlyphHeight(kS) + 4 * P;

            int shown = 0;
            int rowH = Ui::GlyphHeight(kB) + 6 * P;
            for (auto &e : state.leaderboard.entries) {
                if(shown >= 10 || y + rowH > listBottom) break;
                Rectangle row{(float)rowX, (float)y, (float)rowW, (float)rowH};
                Color fill = (shown % 2 == 0) ? Color{30, 34, 56, 255} : Color{26, 30, 48, 255};
                Ui::Frame(row, P, fill, {48, 54, 84, 255}, {16, 18, 32, 255}, {6, 6, 14, 255});
                Color rankCol = (shown == 0) ? Color{255, 215, 80, 255} : (shown == 1) ? Color{210, 214, 226, 255}
                              : (shown == 2) ? Color{214, 140, 80, 255} : kMuted;
                float ty = row.y + 3 * P;
                if(shown == 0) Ui::DrawIconCentered(Ui::ICON_CROWN, row.x + 9 * P, row.y + rowH / 2.f, P);
                else Ui::TextCentered(TextFormat("%d", shown + 1), row.x + 9 * P, ty, kB, rankCol);
                Ui::Text(TextFormat("%d", e.score), row.x + 20 * P, ty, kB, rankCol);
                const char *right = TextFormat("x%d  %d%s", e.combo, e.coins, e.isDaily ? "  D" : "");
                int rw = Ui::Measure(right, kS);
                Ui::Text(right, row.x + rowW - rw - 5 * P, ty + (Ui::GlyphHeight(kB) - Ui::GlyphHeight(kS)) / 2.f, kS, kMuted);
                y += rowH + P;
                shown++;
            }
            if(shown == 0) {
                const char *empty = Loc::GetLanguage() == Language::EN ? "No entries yet" : "Brak wynikow";
                Ui::TextCentered(empty, uiCenterX, (float)(y + S(20)), kB, kDim);
            }
        }
        else if(statsSubTab == 2) {
            int rowH = Ui::GlyphHeight(kB) + 6 * P;
            auto drawStatRow = [&](const char *label, const char *valStr, Color valCol = Color{255, 220, 140, 255}) {
                if(y + rowH > listBottom) return;
                Rectangle row{(float)rowX, (float)y, (float)rowW, (float)rowH};
                Ui::Frame(row, P, {28, 32, 52, 255}, {48, 54, 84, 255}, {16, 18, 32, 255}, {6, 6, 14, 255});
                int vw = Ui::Measure(valStr, kB);
                Ui::Text(label, row.x + 4 * P, row.y + 3 * P, Ui::FitK(label, kB, rowW - vw - 12 * P), Color{186, 190, 210, 255});
                Ui::Text(valStr, row.x + rowW - vw - 4 * P, row.y + 3 * P, kB, valCol);
                y += rowH + P;
            };

            int mins = (int)(state.stats.totalPlayTime) / 60;
            int hrs = mins / 60;
            char timeStr[32];
            if(hrs > 0) snprintf(timeStr, sizeof(timeStr), "%dh %dm", hrs, mins % 60);
            else snprintf(timeStr, sizeof(timeStr), "%dm %ds", mins, (int)state.stats.totalPlayTime % 60);

            char buf[32];
            snprintf(buf, sizeof(buf), "%d", state.stats.gamesPlayed);
            drawStatRow(Loc::Stats_GamesPlayed(), buf);
            snprintf(buf, sizeof(buf), "%d", state.stats.bestScore);
            drawStatRow(Loc::Stats_BestScore(), buf, Color{255, 200, 80, 255});
            snprintf(buf, sizeof(buf), "%d", state.stats.totalScore);
            drawStatRow(Loc::Stats_TotalScore(), buf);
            snprintf(buf, sizeof(buf), "%d", state.stats.totalCoinsCollected);
            drawStatRow(Loc::Stats_CoinsCollected(), buf, Color{255, 215, 0, 255});
            snprintf(buf, sizeof(buf), "x%d", state.stats.bestCombo);
            drawStatRow(Loc::Stats_BestCombo(), buf, Color{200, 150, 255, 255});
            snprintf(buf, sizeof(buf), "%d", state.stats.totalPlatformsLanded);
            drawStatRow(Loc::Stats_PlatformsLanded(), buf);
            snprintf(buf, sizeof(buf), "%d", state.stats.bestPlatformStreak);
            drawStatRow(Loc::Stats_BestStreak(), buf);
            snprintf(buf, sizeof(buf), "%d", state.stats.totalPowerUpsCollected);
            drawStatRow(Loc::Stats_PowerUps(), buf);
            snprintf(buf, sizeof(buf), "%d", state.stats.totalJumps);
            drawStatRow(Loc::Stats_TotalJumps(), buf);
            snprintf(buf, sizeof(buf), "%d", state.stats.deaths);
            drawStatRow(Loc::Stats_Deaths(), buf, Color{255, 120, 120, 255});
            snprintf(buf, sizeof(buf), "%d", state.stats.revives);
            drawStatRow(Loc::Stats_Revives(), buf, Color{120, 255, 120, 255});
            drawStatRow(Loc::Stats_PlayTime(), timeStr, Color{150, 200, 255, 255});
        }
    }

    contentBottom[menuTab] = y;
    ApplyMenuAudioVolumes();

    // Footer: tab hint, version, language picker
    Ui::Text(Loc::Settings_TabHint(), (float)S(10), (float)(sh - S(30)), kS, Color{120, 126, 156, 200});
    Ui::Text("v" SHAPEY_VERSION, (float)S(10), (float)(sh - S(30) + Ui::GlyphHeight(kS) + 2 * P), kS, Color{80, 86, 110, 200});
    DrawLanguageBox(mPos, click, sw, sh, uiScale);

    if(IsKeyPressed(KEY_TAB)) {
        menuTab = (menuTab + 1) % 7;
    }

    EndDrawing();
}

void Game::DrawLanguageBox(Vector2 mPos, bool click, int sw, int sh, float scale) {
    static bool langBoxOpen = false;
    int p = Ui::UnitFor(scale);
    int k = Ui::TextKFor(scale);
    int boxW = (int)(96 * scale), boxH = (int)(32 * scale);
    int boxX = sw - boxW - (int)(10 * scale);
    int boxY = sh - boxH - (int)(8 * scale);
    Rectangle boxRect{(float)boxX, (float)boxY, (float)boxW, (float)boxH};
    bool hovered = CheckCollisionPointRec(mPos, boxRect);
    Ui::Frame(boxRect, p, hovered ? Color{50, 58, 90, 255} : Color{32, 36, 58, 255}, {70, 80, 120, 255}, {18, 20, 38, 255}, {6, 6, 14, 255});
    Ui::DrawIconCentered(Ui::ICON_GLOBE, boxX + 8.f * p, boxY + boxH / 2.f, p);
    Ui::Text(settings.language == 0 ? "EN" : "PL", (float)(boxX + 16 * p), (float)(boxY + boxH / 2 - Ui::GlyphHeight(k) / 2), k, RAYWHITE);
    Ui::DrawIconCentered(langBoxOpen ? Ui::ICON_ARROW_L : Ui::ICON_ARROW_R, (float)(boxX + boxW - 5 * p), boxY + boxH / 2.f, std::max(1, p / 2));
    if(click && hovered) { langBoxOpen = !langBoxOpen; return; }
    if(!langBoxOpen) return;

    const char *names[2] = {"English", "Polski"};
    int optH = boxH;
    int optW = boxW + (int)(20 * scale);
    int dropX = sw - optW - (int)(10 * scale);
    bool anyHover = false;
    for(int i = 0; i < 2; i++) {
        Rectangle opt{(float)dropX, (float)(boxY - (2 - i) * (optH + p) - p), (float)optW, (float)optH};
        bool sel = settings.language == i;
        bool hov = CheckCollisionPointRec(mPos, opt);
        anyHover |= hov;
        if(Ui::Button(opt, names[i], mPos, click, sel ? Ui::STYLE_BLUE : Ui::STYLE_DARK, k)) {
            settings.language = i;
            Loc::SetLanguage(i == 0 ? Language::EN : Language::PL);
            settingsDirty = true;
            langBoxOpen = false;
        }
    }
    if(click && !anyHover) langBoxOpen = false;
}

void Game::DrawHeroTab(int &y, float uiCenterX, Vector2 mPos, bool click) {
    static int preview = -1;
    if(preview < 0) preview = state.skins.selected;

    Ui::DrawSectionHeader(y, uiCenterX, Loc::Hero_Title(), uiScale);

    // Showcase: the hero idles, then runs, on a pixel platform in front of the tower backdrop
    int panelW = S(300), panelH = S(190);
    Rectangle stage{std::floor(uiCenterX - panelW / 2.f), (float)y, (float)panelW, (float)panelH};
    Ui::Inset(stage, P, {14, 18, 32, 255});
    if(worldArt.biome[BIOME_DEFAULT].id > 0) {
        BeginScissorMode((int)stage.x + P, (int)stage.y + P, (int)stage.width - 2 * P, (int)stage.height - 2 * P);
        WorldArt::DrawSky(worldArt, 0, GetScreenWidth(), GetScreenHeight(), Color{255, 255, 255, 255});
        const Texture2D &bg = worldArt.biome[BIOME_DEFAULT];
        float s = std::max(1.f, std::floor(stage.width / 240.f));
        float bgH = 400.f * s;
        for(int layer = 0; layer < 3; layer++)
            DrawTexturePro(bg, {layer * 240.f, 0, 240.f, 400.f},
                           {uiCenterX - 120.f * s, stage.y + stage.height - bgH * 0.55f, 240.f * s, bgH}, {0, 0}, 0.f,
                           Color{150, 150, 180, 255});
        EndScissorMode();
    }
    float t = (float)GetTime();
    bool running = std::fmod(t, 6.f) > 3.f;
    float px = std::floor(uiCenterX / 2.f) * 2.f;
    float groundY = std::floor((stage.y + panelH - S(40)) / 2.f) * 2.f;
    if(worldArt.loaded){
        float tileW = (float)S(200);
        WorldArt::DrawPlatform(worldArt, {px - tileW/2.f, groundY, tileW, 18.f}, BIOME_DEFAULT, WHITE);
    }
    DrawPlayerPreview({px, groundY}, std::floor(4.f * uiScale), preview, t, running);

    int arrowW = S(36), arrowH = S(56);
    Rectangle left{stage.x + 3 * P, stage.y + panelH / 2.f - arrowH / 2.f, (float)arrowW, (float)arrowH};
    Rectangle right{stage.x + panelW - 3 * P - arrowW, left.y, (float)arrowW, (float)arrowH};
    if(Ui::Button(left, "<", mPos, click, Ui::STYLE_DARK, kB + 1) || IsKeyPressed(KEY_LEFT)) preview = (preview + SKIN_COUNT - 1) % SKIN_COUNT;
    if(Ui::Button(right, ">", mPos, click, Ui::STYLE_DARK, kB + 1) || IsKeyPressed(KEY_RIGHT)) preview = (preview + 1) % SKIN_COUNT;
    y += panelH + 4 * P;

    const SkinInfo &info = kSkins[preview];
    Ui::TextCentered(info.name, uiCenterX, (float)y, kB + 1, RAYWHITE);
    y += Ui::GlyphHeight(kB + 1) + 3 * P;
    const char *desc = Loc::GetLanguage() == Language::EN ? info.descEN : info.descPL;
    Ui::TextCentered(desc, uiCenterX, (float)y, Ui::FitK(desc, kS, S(340)), kMuted);
    y += Ui::GlyphHeight(kS) + 4 * P;

    bool owned = state.skins.Owns(preview);
    bool equipped = owned && state.skins.selected == preview;
    bool affordable = state.globalCoins >= info.price;
    char label[64];
    if(equipped) snprintf(label, sizeof(label), "%s", Loc::Hero_Equipped());
    else if(owned) snprintf(label, sizeof(label), "%s", Loc::Hero_Equip());
    else snprintf(label, sizeof(label), "%s  %d", Loc::Hero_Buy(), info.price);
    Rectangle btn{std::floor(uiCenterX - S(110)), (float)y, (float)S(220), (float)S(42)};
    Ui::Style st = equipped ? Ui::STYLE_DARK : (owned ? Ui::STYLE_GREEN : Ui::STYLE_GOLD);
    if(Ui::Button(btn, label, mPos, click, st, 0, !equipped && (owned || affordable))){
        if(owned){
            state.skins.selected = preview;
            SaveSkins("skins.txt", state.skins);
        } else {
            state.globalCoins -= info.price;
            state.skins.owned |= (1u << preview);
            state.skins.selected = preview;
            SaveGlobalCoins("coins.txt", state.globalCoins);
            SaveSkins("skins.txt", state.skins);
            if(state.audio.sndPowerUp.frameCount>0) PlaySound(state.audio.sndPowerUp);
        }
    }
    if(equipped) Ui::DrawIconCentered(Ui::ICON_CHECK, btn.x + btn.width - 8 * P, btn.y + btn.height / 2.f + P, P);
    y += S(42) + 3 * P;
    if(!owned && !affordable){
        Ui::TextCentered(Loc::Hero_NotEnough(), uiCenterX, (float)y, kS, Color{230, 110, 90, 255});
    }
    y += Ui::GlyphHeight(kS) + 3 * P;

    // Thumbnail strip of every skin; locked ones are dimmed with a padlock
    // Largest integer thumbnail scale whose strip still fits inside the content panel
    int stripMax = std::min(GetScreenWidth() - S(16), S(380)) - S(20);
    int thumbP = std::max(1, P / 2 + (P % 2));
    while(thumbP > 1 && (32 * thumbP + 5 * P) * SKIN_COUNT > stripMax) thumbP--;
    float cellW = 32.f * thumbP + 4 * P;
    float cellH = 40.f * thumbP + 4 * P;
    float gap = (float)P;
    float stripW = cellW * SKIN_COUNT + gap * (SKIN_COUNT - 1);
    float stripX = std::floor(uiCenterX - stripW / 2.f);
    for(int i = 0; i < SKIN_COUNT; i++){
        Rectangle cell{stripX + i * (cellW + gap), (float)y, cellW, cellH};
        bool hov = CheckCollisionPointRec(mPos, cell);
        Color fill = (i == preview) ? Color{58, 92, 172, 255} : (hov ? Color{44, 52, 84, 255} : Color{26, 30, 48, 255});
        Color outline = (i == state.skins.selected) ? Color{255, 206, 72, 255} : Color{6, 6, 14, 255};
        Ui::Frame(cell, P, fill, {70, 80, 120, 255}, {16, 18, 32, 255}, outline);
        Texture2D tex = PlayerFrame(i, 0);
        bool own = state.skins.Owns(i);
        DrawTexturePro(tex, {0,0,(float)tex.width,(float)tex.height},
                       {cell.x + 2 * P, cell.y + 2 * P, 32.f * thumbP, 40.f * thumbP}, {0,0}, 0.f, own ? WHITE : Color{60, 60, 76, 255});
        if(!own) Ui::DrawIconCentered(Ui::ICON_LOCK, cell.x + cellW / 2.f, cell.y + cellH / 2.f, std::max(1, P / 2));
        if(hov && click) preview = i;
    }
    y += (int)cellH + 5 * P;

    DrawCoinAmount(worldArt.items, uiCenterX, (float)y, state.globalCoins, kB, P);
    y += Ui::GlyphHeight(kB);
}
