// Game state & interface
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
    Theme currentTheme{};
    int generatedPlatformsCount = 0;
    float themeChangeTimer = 0.f;
    int score = 0;
    int highScore = 0;
    float comboTimer=0.f; int comboCount=0; int lastLandedPlatformIndex=0; int lastScoredPlatformIndex=-1; float lastLandY=0.f;
    bool gameOver=false;
    std::vector<Particle> particles;
    bool scrollActive=false; int speedStage=0; float stageTimer=0.f; float scrollSpeed=60.f;
    float highestPlatformY=0.f; float cameraTopY=0.f;
    Camera2D camera{};
    bool paused=true; bool started=false;
    bool onGround=false; float coyoteTimer=0.f; float jumpBufferTimer=0.f;
    GameAudio audio;
    Texture2D playerTexture{};
    float playerSpriteScale = 1.8f;
    float playerSpriteYOffset = 0.f;
    float playerSpriteBottomPad = 0.f;
    float animTime = 0.f;
    float landingTimer = 0.f;
    bool landingSquashActive = false;
    float landingSquashTime = 0.f;
    float landingSquashDuration = 0.18f;
    float lastVerticalVelocity = 0.f;
    float hardLandingThreshold = 520.f;
    // Fire shader
    Shader shaderFire{}; int fireLocTime=-1; int fireLocIntensity=-1; int fireLocSpriteSize=-1; int fireLocMode=-1;
    KeyBindings keys;
    enum class Screen { MENU, GAME, PAUSE, GAMEOVER };
    Screen currentScreen = Screen::MENU;
    float fadeAlpha = 0.f;
    float fadeTarget = 0.f;
    float fadeSpeed = 2.5f;
    bool musicPausedOnDeath = false;
};

struct GameConfig {
    int screenWidth=480;
    int screenHeight=800;
    float GRAVITY=1400.f;
    float MOVE_ACCEL=3600.f;
    float MAX_HSPEED=580.f;
    float FRICTION=1800.f;
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
    GameSettings settings;
    bool running=true;
    RenderTexture2D gameRT{};
    void EnsureRenderTarget();
    Rectangle viewportRect{0,0,0,0};
    int windowedW = 480;
    int windowedH = 800;
    int resolutionIndex = 0;
    bool fullscreen = false;
    void ApplyResolution(bool recenterCamera=true);
    bool settingsDirty = false;
    float settingsSaveTimer = 0.f;
    void CaptureSettings(GameSettings &out);
    void AutoSaveSettings(float dt);
    void ResetSettingsToDefaults();
    void ResetGame();
    void UpdateGameplay(float dt);
    void DrawMenu();
    void DrawPause();
    void DrawGame();
    void UpdateFade(float dt);
    void ChangeScreen(GameState::Screen next, bool withFade=true);
    void EmitLandingParticles(Vector2 contact,int count);
    void EmitWallBounceParticles(Vector2 contact,int count);
    // Helpers extracted
    void DrawResolutionSelector(int &y, float uiCenterX, Vector2 mPos, bool click, int sw);
    void DrawAudioSliders(int &y, float uiCenterX, Vector2 mPos, int sw, bool &changedOut);
    void ApplyAudioVolumes();
    template<typename RebindEnum>
    Rectangle DrawRebindKey(int &y, float uiCenterX, Vector2 mPos, int sw, const char* label, int key, RebindEnum active, RebindEnum selfId, bool &clicked, float blinkAlpha, Rectangle &lastHover, const char* &lastDefault, const char* defaultTxt, bool &activeChanged){
        auto clampX=[&](int desired,int w){ int x=desired; if(x<10) x=10; if(x+w>sw-10) x=sw-10-w; return x; };
        int w=320,h=30; int x=clampX((int)(uiCenterX - w/2),w); Rectangle rc{(float)x,(float)y,(float)w,(float)h}; bool isActive = (active==selfId); bool hover=CheckCollisionPointRec(mPos,rc); if(hover){ lastHover=rc; lastDefault=defaultTxt; }
        Color base=isActive?Color{120,70,30,255}:Color{50,60,80,255}; if(isActive){ base={(unsigned char)(120 + 40*blinkAlpha),(unsigned char)(70 + 30*blinkAlpha),30,255}; }
        DrawRectangleRec(rc,base); DrawRectangleLines((int)rc.x,(int)rc.y,(int)rc.width,(int)rc.height,RAYWHITE);
        std::string txt=std::string(label)+": "+KeyName(key); if(isActive) txt += "  (nacisnij klawisz)"; DrawText(txt.c_str(), x+10, y+7,16,RAYWHITE);
        clicked=false; if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hover){ clicked=true; activeChanged=true; }
        y += h + 8; return rc;
    }
    // Map window-space mouse pos do logicznej przestrzeni gry (letterbox)
    Vector2 MapWindowToLogical(Vector2 win) const {
        if(viewportRect.width<=0 || viewportRect.height<=0) return win;
        float scale = viewportRect.width / (float)cfg.screenWidth; // uniform
        Vector2 out { (win.x - viewportRect.x)/scale, (win.y - viewportRect.y)/scale };
        return out;
    }
    // Proste GUI helpers (inline by uniknąć dodatkowych plików)
    Rectangle GuiButtonCentered(float centerX, int &y, int w, int h, const char* label, Vector2 mouse, bool &pressedOut) const {
        int bx = (int)(centerX - w/2);
        Rectangle rc{(float)bx,(float)y,(float)w,(float)h};
        bool hover = CheckCollisionPointRec(mouse, rc);
        Color c = hover?Color{90,140,220,255}:Color{60,90,140,255};
        DrawRectangleRec(rc,c); DrawRectangleLines(bx,y,w,h,RAYWHITE);
        int tw=MeasureText(label,20); DrawText(label,bx + w/2 - tw/2,y+ (h-20)/2,20,RAYWHITE);
        if(hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) pressedOut=true; else pressedOut=false;
        y += h + UiLayout::ButtonGap; return rc;
    }
    void GuiVolumeSlider(float centerX, int &y, const char* name, float &val, Vector2 mouse, int clampLeft, int clampRight, bool &changed){
        int w=320, h=24; int x=(int)(centerX - w/2); if(x<clampLeft) x=clampLeft; if(x+w>clampRight) x=clampRight-w; Rectangle bar{(float)x,(float)y,(float)w,(float)h};
        DrawRectangleRec(bar,Color{40,50,70,255}); DrawRectangleLines(x,y,w,h,RAYWHITE);
        float knobX = x + val * (w-10); Rectangle knob{knobX,(float)y,10.f,(float)h}; DrawRectangleRec(knob,Color{150,180,240,255});
        DrawText(name,x+5,y+4,16,RAYWHITE);
        if(IsMouseButtonDown(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse,bar)){
            float nv = (mouse.x - x)/(float)w; if(nv<0) nv=0; if(nv>1) nv=1; if(std::fabs(nv-val)>0.0001f){ val=nv; changed=true; }
        }
        y += h + UiLayout::SliderGap;
    }
    void DrawGameWorld(float dt);
    void DrawHud(float dt);
    void DrawGameOverOverlay();
};
