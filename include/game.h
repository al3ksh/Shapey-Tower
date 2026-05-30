#pragma once
#include <vector>
#include <string>
#include <cmath>
#include "raylib.h"
#include "player.h"
#include "platform.h"
#include "theme.h"
#include "audio.h"
#include "persistence.h"
#include "input.h"
#include "settings.h"
#include "particles.h"
#include "video_constants.h"
#include "collectibles.h"
#include "difficulty.h"
#include "daily_challenge.h"
#include "screen_shake.h"
#include "parallax.h"
#include "achievements.h"
#include "rng.h"
#include "tutorial.h"
#include "stats.h"
#include "leaderboard.h"
#include "world_art.h"
#include "skins.h"
#include "ui_kit.h"

namespace UiLayout {
    inline constexpr int ButtonGap = 18;
    inline constexpr int CompactPullUp = 6;
    inline constexpr int SectionLabelGap = 22;
    inline constexpr int SliderGap = 12;
    inline constexpr int ResetGap = 30;
    inline constexpr int BottomMargin = 60;
}

struct GameState {
    Player player;
    std::vector<Platform> platforms;
    std::vector<Theme> themes;
    int currentThemeIndex = 0;
    int prevThemeIndex = 0;
    Theme currentTheme{};
    int generatedPlatformsCount = 0;
    float themeChangeTimer = 0.f;
    int score = 0;
    int highScore = 0;
    int runStartHighScore = 0;   // best score before this run, for the NEW BEST badge
    float comboTimer=0.f; int comboCount=0; int lastLandedPlatformIndex=0; int lastScoredPlatformIndex=-1; float lastLandY=0.f;
    bool gameOver=false;
    std::vector<Particle> particles;
    bool scrollActive=false; int speedStage=0; float stageTimer=0.f; float scrollSpeed=60.f;
    float highestPlatformY=0.f; float cameraTopY=0.f;
    Camera2D camera{};
    bool paused=true; bool started=false;
    bool onGround=false; bool onIce=false; float coyoteTimer=0.f; float jumpBufferTimer=0.f;
    GameAudio audio;
    Texture2D playerTexture{};
    std::vector<Texture2D> playerFrames;   // skin-major: [skin * playerFramesPerSkin + frame]
    int playerFramesPerSkin = 0;
    int playerSkinRows = 0;
    SkinState skins;
    bool dying = false;          // death animation playing before the game-over/revive screen
    float dyingTimer = 0.f;
    float dyingSpin = 0.f;
    bool playerFacingLeft = false;
    float runAnimTime = 0.f;
    float playerSpriteScale = 1.8f;
    float playerSpriteYOffset = 0.f;
    float playerSpriteBottomPad = 0.f;
    float animTime = 0.f;
    bool landingSquashActive = false;
    float landingSquashTime = 0.f;
    float landingSquashDuration = 0.18f;
    float lastVerticalVelocity = 0.f;
    float hardLandingThreshold = 520.f;
    Shader shaderFire{}; int fireLocTime=-1; int fireLocIntensity=-1; int fireLocSpriteSize=-1; int fireLocMode=-1;
    KeyBindings keys;
    enum class Screen { MENU, GAME, PAUSE, GAMEOVER, DAILY, REVIVE_PROMPT };
    Screen currentScreen = Screen::MENU;
    float fadeAlpha = 0.f;
    float fadeTarget = 0.f;
    float fadeSpeed = 2.5f;
    bool musicPausedOnDeath = false;
    float reviveTimer = 5.0f;
    const float REVIVE_TIME_LIMIT = 5.0f;
    
    std::vector<Coin> coins;
    std::vector<PowerUp> powerups;
    std::vector<ActivePowerUp> activePowerUps;
    int totalCoinsCollected = 0;
    int totalCoins = 0;
    int sessionCoins = 0;
    int globalCoins = 0;
    bool hasRevivedThisRun = false;
    int reviveCost = 10;  
    bool hasDoubleJump = false;
    bool doubleJumpUsed = false;
    bool hasShield = false;
    float slowMotionFactor = 1.f;
    float coinMagnetRange = 0.f;
    
    bool activeDoubleJump = false;
    bool activeShield = false;
    bool activeSlowMotion = false;
    bool activeMagnet = false;
    float powerUpTimers[4] = {0.f, 0.f, 0.f, 0.f};
    float shieldFlashAlpha = 0.f;      
    float doubleJumpEffectTimer = 0.f;
    
    Difficulty difficulty = Difficulty::NORMAL;
    DailyChallenge dailyChallenge;
    bool isDailyRun = false;
    
    ScreenShake screenShake;
    std::vector<ParallaxLayer> parallaxLayers;
    
    std::vector<Achievement> achievements;
    std::string lastUnlockedAchievement;
    float achievementPopupTimer = 0.f;
    GameRNG rng;
    TutorialState tutorial;
    GamepadState gamepad;
    GameStats stats;
    Leaderboard leaderboard;
    int currentRunPlatforms = 0;
    int currentRunJumps = 0;
    int currentRunPowerUps = 0;
    int currentRunBestCombo = 0;
    bool wallSlidingLeft = false;
    bool wallSlidingRight = false;
    float wallSlideGravity = 200.f;
    float themeBlend = 1.f;
    Theme prevTheme{};
};

struct GameConfig {
    int screenWidth=480;
    int screenHeight=800;
    int gameWidth = Video::GAME_WIDTH;
    int gameHeight = Video::GAME_HEIGHT;
    float GRAVITY=1400.f;
    float MOVE_ACCEL=3600.f;
    float MAX_HSPEED=580.f;
    float FRICTION=1800.f;
    float ICE_FRICTION=200.f;
    float BASE_JUMP_SPEED=-900.f;
    float EXTRA_JUMP_BOOST=300.f;
    float COYOTE_TIME=0.10f;
    float JUMP_BUFFER=0.12f;
    float COMBO_WINDOW=2.f;
    float deadzone=200.f;
    float STAGE_DURATION=30.f;
};

class Game {
public:
    Game(const GameConfig &cfg);
    ~Game();
    void Update();
    void Render();
    bool ShouldClose() const;
private:
    static constexpr auto &kResolutions = Video::RESOLUTIONS;
    static constexpr int RESOLUTION_COUNT = Video::RESOLUTION_COUNT;
    GameConfig cfg;
    GameState state;
    WorldArt::Assets worldArt;
    GameSettings settings;
    bool running=true;
    RenderTexture2D gameRT{};
    void EnsureRenderTarget();
    Rectangle viewportRect{0,0,0,0};
    int windowedW = 480;
    int windowedH = 800;
    int windowedPosX = 0;
    int windowedPosY = 0;
    int resolutionIndex = 0;
    bool fullscreen = false;
    bool fakeFullscreenActive = false;
    void ApplyResolution(bool recenterCamera=true);
    bool settingsDirty = false;
    float settingsSaveTimer = 0.f;
    void CaptureSettings(GameSettings &out);
    void AutoSaveSettings(float dt);
    void ResetSettingsToDefaults();
    void ResetGame();
    void SaveProgress();
    void RevivePlayer();
    void StartDying();
    void FinishDying();
    void UpdateGameplay(float dt);
    void DrawMenu();
    void DrawPause();
    void DrawGame(float dt);
    void DrawRevivePrompt();
    void UpdateFade(float dt);
    void ChangeScreen(GameState::Screen next, bool withFade=true);
    void EmitLandingParticles(Vector2 contact,int count);
    void EmitWallBounceParticles(Vector2 contact,int count);
    void SpawnOnePlatform(float y);
    void ApplyThemeIfNeeded();
    void DrawResolutionSelector(int &y, float uiCenterX, Vector2 mPos, bool click, int sw, float scale = 1.0f);
    void ApplyAudioVolumes();
    void ApplyMenuAudioVolumes();
    Vector2 MapWindowToLogical(Vector2 win) const {
        if(viewportRect.width<=0 || viewportRect.height<=0) return win;
        float scale = viewportRect.width / (float)cfg.gameWidth; // uniform
        Vector2 out { (win.x - viewportRect.x)/scale, (win.y - viewportRect.y)/scale };
        return out;
    }
    Rectangle GuiButtonCentered(float centerX, int &y, int w, int h, const char* label, Vector2 mouse, bool &pressedOut, Ui::Style style = Ui::STYLE_BLUE, int k = 0) const {
        Rectangle rc{std::floor(centerX - w/2.f), (float)y, (float)w, (float)h};
        pressedOut = Ui::Button(rc, label, mouse, IsMouseButtonPressed(MOUSE_LEFT_BUTTON), style, k);
        y += h + Ui::Unit() * 4; return rc;
    }
    // Draws gameRT letterboxed into the window (blurred backdrop + crisp pixel blit); no Begin/EndDrawing.
    void PresentGameRT();
    // Animated tower scene rendered into gameRT for the menu background.
    void RenderMenuScene(float time);
    void DrawGameWorld(float dt);
    Texture2D PlayerFrame(int skin, int frame) const {
        if(state.playerFramesPerSkin <= 0) return state.playerTexture;
        if(skin < 0 || skin >= state.playerSkinRows) skin = 0;
        return state.playerFrames[skin * state.playerFramesPerSkin + frame];
    }
    void DrawPlayerPreview(Vector2 feet, float scale, int skin, float time, bool running);
    void DrawHeroTab(int &y, float uiCenterX, Vector2 mPos, bool click);
    void DrawLanguageBox(Vector2 mPos, bool click, int sw, int sh, float scale);
    void DrawHud(float dt);
    void DrawGameOverOverlay();
    void DrawBiomeEffects(int w, int h, float cameraY, float time);
};
