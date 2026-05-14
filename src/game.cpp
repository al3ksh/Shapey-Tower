#include "game.h"
#include "debug.h"
#include "tutorial.h"
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cmath>
#if defined(_WIN32)
void SetWindowsTaskbarIconFromResource();
#endif
#ifdef __APPLE__
extern "C" void SetDockIcon(const char* path);
#endif

Game::Game(const GameConfig &cfg):cfg(cfg){
    DEBUG_LOG("Game ctor start (%dx%d)", cfg.screenWidth, cfg.screenHeight);
    InitWindow(cfg.screenWidth,cfg.screenHeight,"Shapey Tower");
    Vector2 initialPos = GetWindowPosition();
    windowedPosX = (int)initialPos.x;
    windowedPosY = (int)initialPos.y;
    const char* iconPaths[] = {"assets/icons/shapeyicon.png","icons/shapeyicon.png","shapeyicon.png"};
    for(const char* ip: iconPaths){
        if(FileExists(ip)){
            Image icon = LoadImage(ip);
            if(icon.data){
                ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                SetWindowIcon(icon);
                LOG_INFO("Window icon set: %s", ip);
                UnloadImage(icon);
            }
#ifdef __APPLE__
            extern void SetDockIcon(const char* path); SetDockIcon(ip);
#endif
            break;
        }
    }

#if defined(_WIN32)
    SetWindowsTaskbarIconFromResource();
#endif
    windowedW = cfg.screenWidth; windowedH = cfg.screenHeight;
    LoadSettings("settings.cfg", settings);
    resolutionIndex = settings.resolutionIndex;
    fullscreen = settings.fullscreen;
    state.keys.left = settings.keyLeft;
    state.keys.right = settings.keyRight;
    state.keys.jump = settings.keyJump;
    SetExitKey(KEY_NULL);
    if(settings.vsync) {
        SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
    } else {
        SetTargetFPS(settings.targetFPS);
    }
    InitAudioDevice();
    state.audio = GameAudio{};
    auto clamp01=[](float v){ return v<0?0.f:(v>1?1.f:v); };
    state.audio.masterSlider = clamp01(settings.master);
    state.audio.volMusic = clamp01(settings.music);
    state.audio.volJump = clamp01(settings.jump);
    state.audio.volBounce = clamp01(settings.bounce);
    state.audio.volDeath = clamp01(settings.death);
    state.audio.volThemeChange = clamp01(settings.theme);
    state.audio.volCoin = clamp01(settings.coin);
    state.audio.volPowerUp = clamp01(settings.powerup);
    state.audio.masterVolume = 0.0001f + state.audio.masterSlider;
    SetMasterVolume(state.audio.masterVolume);
    LoadGameAudio(state.audio);
    state.themes = GetThemes();
    state.currentThemeIndex=0; state.currentTheme=state.themes[0];
    state.player = Player{ {cfg.gameWidth/2.f-16.f, cfg.gameHeight-120.f},{0,0},32.f,40.f };
    state.platforms.push_back({Rectangle{0,(float)cfg.gameHeight-60.f,(float)cfg.gameWidth,20.f}});
    state.highScore = LoadHighScore("highscore.txt");
    state.globalCoins = LoadGlobalCoins("coins.txt");

    float currentY=state.platforms[0].rect.y-100.f; for(int i=0;i<15;i++){ float gap=state.currentTheme.gapMin+state.rng.nextInt((int)(state.currentTheme.gapMax-state.currentTheme.gapMin+1)); currentY-=gap; SpawnOnePlatform(currentY);} state.highestPlatformY=currentY;
    state.camera.target={ (float)cfg.gameWidth/2, state.player.pos.y+state.player.height/2};
    state.camera.offset={ (float)cfg.gameWidth/2,(float)cfg.gameHeight/2};
    state.camera.zoom=1.f;
    state.cameraTopY=state.camera.target.y;
    state.currentScreen = GameState::Screen::MENU;
    const char* playerTexPaths[] = {"assets/textures/model.png","model.png"};
    for(const char* p: playerTexPaths){
        if(FileExists(p)){
            state.playerTexture = LoadTexture(p);
            Image img = LoadImage(p);
            if(img.data){
                int bottomPad=0; bool found=false;
                for(int y=img.height-1; y>=0 && !found; --y){
                    for(int x=0;x<img.width;++x){
                        Color *pix = ((Color*)img.data) + y*img.width + x;
                        if(pix->a>10){
                            bottomPad = img.height-1 - y; 
                            found=true; break;
                        }
                    }
                }
                state.playerSpriteBottomPad = (float)bottomPad;
                UnloadImage(img);
                LOG_INFO("Sprite bottomPad=%d", bottomPad);
            }
            break; 
        }
    }
    const char* firePaths[] = {"assets/shaders/fire.fs","shaders/fire.fs","fire.fs"};
    for(const char* p: firePaths){
        if(FileExists(p)){
            state.shaderFire = LoadShader(nullptr, p);
            if(state.shaderFire.id>0){
                state.fireLocTime = GetShaderLocation(state.shaderFire, "uTime");
                state.fireLocIntensity = GetShaderLocation(state.shaderFire, "uIntensity");
                state.fireLocMode = GetShaderLocation(state.shaderFire, "uMode");
                state.fireLocSpriteSize = GetShaderLocation(state.shaderFire, "uSpriteSize");
                LOG_INFO("Loaded fire shader '%s' (loc time=%d intens=%d mode=%d size=%d)", p, state.fireLocTime, state.fireLocIntensity, state.fireLocMode, state.fireLocSpriteSize);
            } else {
                LOG_WARN("Failed to load fire shader '%s'", p);
            }
            break;
        }
    }
    state.parallaxLayers = CreateDefaultParallaxLayers();
    state.achievements = CreateAchievements();
    auto unlockedIds = LoadUnlockedAchievements("achievements.txt");
    for (auto &id : unlockedIds) {
        for (auto &ach : state.achievements) {
            if (ach.id == id) { ach.unlocked = true; break; }
        }
    }
    state.dailyChallenge = GetTodaysChallenge();
    state.dailyChallenge.bestScore = LoadDailyHighScore("daily_highscore.txt", 
        state.dailyChallenge.year, state.dailyChallenge.month, state.dailyChallenge.day);
    LoadStats("stats.txt", state.stats);
    LoadLeaderboard("leaderboard.txt", state.leaderboard);
    ApplyResolution();
    ApplyAudioVolumes();
    
    if (!LoadTutorialDone("tutorial_done.txt")) {
        state.tutorial.active = false;
    }
}

Game::~Game(){
    settings.resolutionIndex = resolutionIndex;
    settings.fullscreen = fullscreen;
    settings.master = state.audio.masterSlider;
    settings.music = state.audio.volMusic;
    settings.jump = state.audio.volJump;
    settings.bounce = state.audio.volBounce;
    settings.death = state.audio.volDeath;
    settings.theme = state.audio.volThemeChange;
    settings.coin = state.audio.volCoin;
    settings.powerup = state.audio.volPowerUp;
    settings.keyLeft = state.keys.left;
    settings.keyRight = state.keys.right;
    settings.keyJump = state.keys.jump;

    SaveSettings("settings.cfg", settings);
    SaveHighScore("highscore.txt", state.highScore);
    SaveStats("stats.txt", state.stats);
    SaveLeaderboard("leaderboard.txt", state.leaderboard);
    
    {
        std::vector<std::string> unlocked;
        for (auto &ach : state.achievements) {
            if (ach.unlocked) unlocked.push_back(ach.id);
        }
        SaveUnlockedAchievements("achievements.txt", unlocked);
    }
    
    if(state.dailyChallenge.bestScore > 0) {
        SaveDailyHighScore("daily_highscore.txt", 
            state.dailyChallenge.year, state.dailyChallenge.month, state.dailyChallenge.day,
            state.dailyChallenge.bestScore);
    }
    
    UnloadGameAudio(state.audio);
    if(state.playerTexture.id>0) UnloadTexture(state.playerTexture);
    if(state.shaderFire.id>0) UnloadShader(state.shaderFire);
    if(gameRT.id>0) UnloadRenderTexture(gameRT);
    CloseAudioDevice();
    CloseWindow();
}

void Game::ApplyResolution(bool recenterCamera){
    DEBUG_LOG("ApplyResolution idx=%d fs=%d recenter=%d", resolutionIndex, (int)fullscreen, (int)recenterCamera);
    int resCount = (int)(sizeof(kResolutions)/sizeof(kResolutions[0]));
    if(resolutionIndex < 0) resolutionIndex = 0; else if(resolutionIndex >= resCount) resolutionIndex = resCount-1;
    int newW = kResolutions[resolutionIndex].w;
    int newH = kResolutions[resolutionIndex].h;
    cfg.screenWidth = newW;
    cfg.screenHeight = newH;
    if(fullscreen){
        if(!fakeFullscreenActive){
            Vector2 pos = GetWindowPosition();
            windowedPosX = (int)pos.x;
            windowedPosY = (int)pos.y;
        }
        int monitor = GetCurrentMonitor();
        Vector2 monitorPos = GetMonitorPosition(monitor);
        int monitorW = GetMonitorWidth(monitor);
        int monitorH = GetMonitorHeight(monitor);
        SetWindowState(FLAG_WINDOW_UNDECORATED);
#ifdef FLAG_WINDOW_TOPMOST
        SetWindowState(FLAG_WINDOW_TOPMOST);
#endif
        SetWindowPosition((int)monitorPos.x, (int)monitorPos.y);
        SetWindowSize(monitorW, monitorH);
        fakeFullscreenActive = true;
    } else {
        if(fakeFullscreenActive){
            ClearWindowState(FLAG_WINDOW_UNDECORATED);
#ifdef FLAG_WINDOW_TOPMOST
            ClearWindowState(FLAG_WINDOW_TOPMOST);
#endif
            SetWindowPosition(windowedPosX, windowedPosY);
            SetWindowSize(newW,newH);
            fakeFullscreenActive = false;
        } else {
            SetWindowSize(newW,newH);
        }
    }
    windowedW = newW;
    windowedH = newH;
    state.camera.offset = {(float)cfg.gameWidth/2.f,(float)cfg.gameHeight/2.f};
    if(recenterCamera){ state.camera.target.x = (float)cfg.gameWidth/2.f; }
    for(auto &pf: state.platforms){ if(pf.rect.x + pf.rect.width > cfg.gameWidth){ pf.rect.x = cfg.gameWidth - pf.rect.width; if(pf.rect.x < 0) pf.rect.x = 0; } }
    if(state.player.pos.x + state.player.width > cfg.gameWidth){ state.player.pos.x = cfg.gameWidth - state.player.width; }
    if(state.player.pos.x < 0) state.player.pos.x = 0;
}

bool Game::ShouldClose() const { return !running || WindowShouldClose(); }

void Game::ResetGame(){
    if (state.isDailyRun) {
        state.rng.seed(state.dailyChallenge.seed);
    } else {
        state.rng.seed((unsigned int)std::time(nullptr));
    }
    
    DifficultySettings diffSettings = GetDifficultySettings(state.difficulty);
    
    DailyChallengeModifiers dailyMods;
    if (state.isDailyRun) {
        dailyMods = GetChallengeModifiers(state.dailyChallenge.type);
    }
    
    state.score=0; state.comboTimer=0.f; state.comboCount=0; state.lastLandedPlatformIndex=0; state.gameOver=false; state.platforms.clear();
    state.coins.clear(); state.powerups.clear(); state.activePowerUps.clear();
    state.sessionCoins = 0;
    state.hasRevivedThisRun = false;
    state.hasDoubleJump = false; state.doubleJumpUsed = false; state.hasShield = false;
    state.slowMotionFactor = 1.f; state.coinMagnetRange = 0.f;
    state.activeDoubleJump = false; state.activeShield = false;
    state.activeSlowMotion = false; state.activeMagnet = false;
    for(int i=0; i<4; i++) state.powerUpTimers[i] = 0.f;
    state.totalCoins = 0;
    state.screenShake = ScreenShake{};
    state.currentRunPlatforms = 0;
    state.currentRunJumps = 0;
    state.currentRunPowerUps = 0;
    state.currentRunBestCombo = 0;
    
    state.platforms.push_back({Rectangle{0,(float)cfg.gameHeight-60.f,(float)cfg.gameWidth,20.f}});
    state.player.pos = {cfg.gameWidth/2.f - state.player.width/2, state.platforms[0].rect.y - state.player.height};
    state.player.vel={0,0};
    state.currentThemeIndex=0; state.currentTheme=state.themes[0]; state.themeChangeTimer=2.f; state.generatedPlatformsCount=1; state.lastScoredPlatformIndex=-1; state.lastLandY=state.platforms[0].rect.y;
    
    float gapMult = diffSettings.gapMultiplier;
    float currentY2=state.platforms[0].rect.y-100.f; 
    for(int i=0;i<15;i++){ 
        float gap=(state.currentTheme.gapMin+state.rng.nextInt((int)(state.currentTheme.gapMax-state.currentTheme.gapMin+1))) * gapMult; 
        currentY2-=gap; SpawnOnePlatform(currentY2);
    }
    state.highestPlatformY=currentY2; state.cameraTopY=state.player.pos.y+state.player.height/2; state.scrollActive=false; state.speedStage=0; state.stageTimer=0.f; 
    
    float baseScrollSpeed = 60.f * diffSettings.scrollSpeedMultiplier;
    if(state.isDailyRun) baseScrollSpeed *= dailyMods.scrollSpeedMult;
    state.scrollSpeed = baseScrollSpeed;

    if(state.musicPausedOnDeath && state.audio.musicBg.ctxData){
        StopMusicStream(state.audio.musicBg); 
        PlayMusicStream(state.audio.musicBg);
        SetMusicVolume(state.audio.musicBg, state.audio.volMusic * VOL_MUSIC_MULT);
        state.musicPausedOnDeath=false;
    }
}

void Game::RevivePlayer(){
    if(state.globalCoins < state.reviveCost) return;
    if(state.hasRevivedThisRun) return;
    
    state.globalCoins -= state.reviveCost;
    SaveGlobalCoins("coins.txt", state.globalCoins);
    state.hasRevivedThisRun = true;
    state.gameOver = false;
    state.stats.revives++;
    
    float safeY = state.cameraTopY + cfg.gameHeight * 0.3f;
    const Platform* bestPlatform = nullptr;
    for(const auto& p : state.platforms) {
        if(p.rect.y < safeY && p.rect.y > state.cameraTopY - 100) {
            if(!bestPlatform || p.rect.y > bestPlatform->rect.y) {
                bestPlatform = &p;
            }
        }
    }
    
    if(bestPlatform) {
        state.player.pos.x = bestPlatform->rect.x + bestPlatform->rect.width/2 - state.player.width/2;
        state.player.pos.y = bestPlatform->rect.y - state.player.height - 5;
    } else {
        state.player.pos.x = cfg.gameWidth/2 - state.player.width/2;
        state.player.pos.y = state.cameraTopY + cfg.gameHeight * 0.3f;
    }
    
    state.player.vel = {0, 0};
    
    state.hasShield = true;
    state.activeShield = true;
    state.powerUpTimers[1] = 3.f; 
    state.activePowerUps.push_back({PowerUpType::SHIELD, 3.f});
    
    if(state.musicPausedOnDeath && state.audio.musicBg.ctxData){
        ResumeMusicStream(state.audio.musicBg);
        state.musicPausedOnDeath = false;
    }
    
    ChangeScreen(GameState::Screen::GAME, false);
}

void Game::EmitLandingParticles(Vector2 contact,int count){ 
    if(!settings.particles) return;
    for(int i=0;i<count;i++){ float spd=60.f+state.rng.nextInt(120); float ang=(-90.f+state.rng.nextInt(120)-60.f)*(3.14159f/180.f); Vector2 v{std::cos(ang)*spd,std::sin(ang)*spd}; state.particles.push_back({contact,v,0.6f,0.6f,state.currentTheme.platStatic}); }
}
void Game::EmitWallBounceParticles(Vector2 contact,int count){
    if(!settings.particles) return;
    bool left = (contact.x < state.player.width*0.5f + 2.f);
    bool right = !left;
    for(int i=0;i<count;i++){
        float spd = 120.f + state.rng.nextInt(160);
        float deg;
        if(left){ deg = -60.f + state.rng.nextInt(120); }
        else { deg = 120.f + state.rng.nextInt(120); }
        float ang = deg * (3.1415926f/180.f);
    Vector2 v{std::cos(ang)*spd, std::sin(ang)*spd};
        Color c = (i%2==0)? state.currentTheme.platStatic : state.currentTheme.playerBody;
        state.particles.push_back({contact,v,0.55f,0.55f,c});
    }
}

void Game::ApplyThemeIfNeeded(){
    int stage = state.generatedPlatformsCount / PLATFORMS_PER_THEME;
    int nextTheme = stage % (int)state.themes.size();
    if(nextTheme != state.currentThemeIndex){
        state.prevTheme = state.currentTheme;
        state.themeBlend = 0.f;
        state.currentThemeIndex = nextTheme;
        state.currentTheme = state.themes[state.currentThemeIndex];
        state.themeChangeTimer = 3.f;
        if(state.audio.sndThemeChange.frameCount>0){
            SetSoundVolume(state.audio.sndThemeChange, state.audio.volThemeChange * VOL_THEME_MULT * VOLUME_SCALE);
            PlaySound(state.audio.sndThemeChange);
        }
    }
}

void Game::SpawnOnePlatform(float y){
    DifficultySettings diffSettings = GetDifficultySettings(state.difficulty);
    DailyChallengeModifiers dailyMods;
    if(state.isDailyRun) dailyMods = GetChallengeModifiers(state.dailyChallenge.type);

    float w = 80.f + state.rng.nextInt(140) + diffSettings.platformWidthBonus;
    if(state.isDailyRun) w *= dailyMods.platformWidthMult;
    if(w < 50) w = 50;
    float x = (float)(state.rng.nextInt((int)(cfg.gameWidth - (int)w)));
    Platform p; p.rect = {x, y, w, 18.f};

    if(state.isDailyRun){
        if(dailyMods.allIce) p.type = PlatformType::ICE;
        else if(dailyMods.allCrumbling) p.type = PlatformType::CRUMBLING;
        else if(dailyMods.allSpring) p.type = PlatformType::SPRING;
        else if(dailyMods.allDisappearing) p.type = PlatformType::DISAPPEARING;
        else {
            int typeRoll = state.rng.nextPercent();
            int extraChance = dailyMods.specialPlatformChance;
            if(typeRoll < 5 + extraChance/4) p.type = PlatformType::SPRING;
            else if(typeRoll < 12 + extraChance/3) p.type = PlatformType::CRUMBLING;
            else if(typeRoll < 18 + extraChance/3) p.type = PlatformType::ICE;
            else if(typeRoll < 22 + extraChance/4) p.type = PlatformType::DISAPPEARING;
        }
    } else {
        int typeRoll = state.rng.nextPercent();
        if(typeRoll < 5) p.type = PlatformType::SPRING;
        else if(typeRoll < 12) p.type = PlatformType::CRUMBLING;
        else if(typeRoll < 18) p.type = PlatformType::ICE;
        else if(typeRoll < 22) p.type = PlatformType::DISAPPEARING;
    }

    if(state.isDailyRun && dailyMods.allMoving){
        p.moving = true;
        p.baseX = x;
        p.moveAmplitude = 40.f + state.rng.nextInt(60);
        p.moveSpeed = 1.2f + state.rng.nextFloat(0.f, 2.f);
    } else if(state.rng.nextPercent() < state.currentTheme.moveChance){
        p.moving = true; p.baseX = x;
        p.moveAmplitude = 40.f + state.rng.nextInt(41);
        p.moveSpeed = 1.f + state.rng.nextFloat(0.f, 2.f);
    }
    state.platforms.push_back(p);
    state.generatedPlatformsCount++;
    ApplyThemeIfNeeded();

    int coinChance = diffSettings.coinSpawnChance;
    if(state.isDailyRun && dailyMods.extraCoins) coinChance = 60;
    if(state.rng.nextPercent() < coinChance){
        Coin coin; coin.pos = {x + w/2, y - 30}; state.coins.push_back(coin);
        if(state.isDailyRun && dailyMods.extraCoins && state.rng.nextPercent() < 40){
            Coin coin2; coin2.pos = {x + w/2 + 25, y - 30}; state.coins.push_back(coin2);
        }
    }

    if(!(state.isDailyRun && dailyMods.noPowerUps)){
        if(state.rng.nextPercent() < diffSettings.powerUpSpawnChance){
            PowerUp pu; pu.pos = {x + w/2, y - 50}; pu.type = (PowerUpType)(state.rng.nextInt(4)); state.powerups.push_back(pu);
        }
    }
}

void Game::EnsureRenderTarget(){
    if(gameRT.id==0 || gameRT.texture.width!=cfg.gameWidth || gameRT.texture.height!=cfg.gameHeight){
        if(gameRT.id>0) UnloadRenderTexture(gameRT);
        gameRT = LoadRenderTexture(cfg.gameWidth,cfg.gameHeight);
    }
}

void Game::Update(){
    PROF_SCOPE("Game::Update");
    if(ShouldClose()){ running=false; return; }
    if(IsKeyPressed(KEY_GRAVE)) { running=false; return; }
    if(IsKeyPressed(KEY_F3)) { settings.showFPS = !settings.showFPS; settingsDirty = true; settingsSaveTimer = 0.f; }
    if(IsKeyPressed(KEY_ESCAPE) || state.gamepad.startPressed || state.gamepad.backPressed){
        switch(state.currentScreen){
            case GameState::Screen::MENU: running=false; break;
            case GameState::Screen::GAME: ChangeScreen(GameState::Screen::PAUSE); break;
            case GameState::Screen::PAUSE: ChangeScreen(GameState::Screen::GAME); break;
            case GameState::Screen::GAMEOVER: ChangeScreen(GameState::Screen::MENU); break;
            case GameState::Screen::REVIVE_PROMPT: ChangeScreen(GameState::Screen::GAMEOVER, false); break;
            default: break;
        }
    }
    float dt=GetFrameTime();
    AutoSaveSettings(dt);
    
    UpdateGamepadState(state.gamepad);
    
    if(state.audio.musicBg.ctxData){ UpdateMusicStream(state.audio.musicBg); }
    UpdateFade(dt);
    switch(state.currentScreen){
        case GameState::Screen::MENU: DrawMenu(); break;
        case GameState::Screen::PAUSE: DrawPause(); break;
        case GameState::Screen::GAME:
            UpdateGameplay(dt);
            state.stats.totalPlayTime += dt;
            DrawGame(dt);
            break;
        case GameState::Screen::GAMEOVER:
            DrawGame(dt);
            break;
        case GameState::Screen::REVIVE_PROMPT:
            state.reviveTimer -= dt;
            if(state.reviveTimer <= 0) {
                ChangeScreen(GameState::Screen::GAMEOVER, false);
            }
            DrawRevivePrompt();
            break;
        default: break;
    }
}

void Game::UpdateGameplay(float dt){
    PROF_SCOPE("UpdateGameplay");
    if(state.gameOver && state.currentScreen != GameState::Screen::REVIVE_PROMPT){ 
        ChangeScreen(GameState::Screen::GAMEOVER,false); 
    }
    
    if(IsTutorialActive(state.tutorial) && IsKeyPressed(KEY_ESCAPE)) {
        state.tutorial.currentStep = TutorialStep::DONE;
        state.tutorial.active = false;
        SaveTutorialDone("tutorial_done.txt");
    }
    
    float effectiveDt = dt * state.slowMotionFactor;
    
    state.animTime += dt;
    UpdateShake(state.screenShake, dt);
    
    if(state.shieldFlashAlpha > 0) state.shieldFlashAlpha -= dt * 4.f;
    if(state.doubleJumpEffectTimer > 0) state.doubleJumpEffectTimer -= dt;
    if(state.themeBlend < 1.f) { state.themeBlend += dt * 0.8f; if(state.themeBlend > 1.f) state.themeBlend = 1.f; }
    
    UpdateGamepadState(state.gamepad);
    
    for (auto it = state.activePowerUps.begin(); it != state.activePowerUps.end();) {
        it->timeRemaining -= dt;
        if (it->timeRemaining <= 0) {
            if (it->type == PowerUpType::SLOW_MOTION) state.slowMotionFactor = 1.f;
            else if (it->type == PowerUpType::COIN_MAGNET) state.coinMagnetRange = 0.f;
            else if (it->type == PowerUpType::DOUBLE_JUMP) { state.hasDoubleJump = false; state.activeDoubleJump = false; }
            it = state.activePowerUps.erase(it);
        } else ++it;
    }
    
    state.lastVerticalVelocity = state.player.vel.y;
    float dir=0.f; if(IsKeyDown(state.keys.left)) dir-=1.f; if(IsKeyDown(state.keys.right)) dir+=1.f;
    if(state.gamepad.active) dir += state.gamepad.leftX;
    if(dir > 1.f) dir = 1.f; if(dir < -1.f) dir = -1.f;
    bool jumpPressed=IsKeyPressed(state.keys.jump) || state.gamepad.jumpPressed;
    bool jumpDown=IsKeyDown(state.keys.jump) || state.gamepad.jumpDown;
    if(jumpPressed || jumpDown) state.jumpBufferTimer=cfg.JUMP_BUFFER; else if(state.jumpBufferTimer>0) state.jumpBufferTimer-=dt;
    if(state.onGround) { state.coyoteTimer=cfg.COYOTE_TIME; state.doubleJumpUsed = false; }
    else if(state.coyoteTimer>0) state.coyoteTimer-=dt;
    bool wantsJump = (state.jumpBufferTimer>0 && state.coyoteTimer>0);
    bool wantsDoubleJump = jumpPressed && !state.onGround && state.hasDoubleJump && !state.doubleJumpUsed && state.coyoteTimer <= 0;

    Vector2 prevPos=state.player.pos;
    float currentFriction = state.onIce ? cfg.ICE_FRICTION : cfg.FRICTION;
    UpdatePlayerPhysics(state.player, effectiveDt, dir, state.onGround, cfg.MOVE_ACCEL, cfg.MAX_HSPEED, currentFriction, cfg.GRAVITY);

    state.onGround=false;
    state.onIce=false;
    if(state.player.vel.y>0){
        float prevBottom=prevPos.y+state.player.height;
        float currBottom=state.player.pos.y+state.player.height;
        for(size_t i=0;i<state.platforms.size();++i){
            auto &pf=state.platforms[i]; 
            if(pf.alpha < 0.5f) continue;
            float topY=pf.rect.y;
            if(prevBottom<=topY && currBottom>=topY){
                float pL=state.player.pos.x, pR=state.player.pos.x+state.player.width;
                float platL=pf.rect.x, platR=pf.rect.x+pf.rect.width;
                if(pR>platL && pL<platR){
                    state.player.pos.y=topY-state.player.height;
                    
                    if(pf.type == PlatformType::SPRING) {
                        state.player.vel.y = cfg.BASE_JUMP_SPEED * 1.5f;
                        if(settings.screenShake) TriggerShake(state.screenShake, 4.f, 0.15f);
                    } else {
                        state.player.vel.y=0; state.onGround=true;
                    }
                    
                    if(pf.type == PlatformType::ICE) {
                        state.onIce = true;
                    }
                    
                    if(pf.type == PlatformType::CRUMBLING || pf.type == PlatformType::DISAPPEARING) {
                        if(!pf.triggered) { pf.triggered = true; pf.stateTimer = 0.f; }
                    }
                    
                    float impactSpeed = std::fabs(state.lastVerticalVelocity);
                    if(impactSpeed >= state.hardLandingThreshold){
                        state.landingSquashActive = true;
                        state.landingSquashTime = 0.f;
                        EmitLandingParticles({state.player.pos.x+state.player.width/2, topY},14+state.rng.nextInt(8));
                        if(settings.screenShake) TriggerShake(state.screenShake, 3.f, 0.1f);
                    } else {
                        EmitLandingParticles({state.player.pos.x+state.player.width/2, topY},6+state.rng.nextInt(6));
                    }
                    if(!state.scrollActive && (int)i>0){ state.scrollActive=true; state.stageTimer=0.f; }
                    if((int)i!=state.lastScoredPlatformIndex){
                        float deltaY=state.lastLandY-pf.rect.y;
                        int floorsJumped=(int)(deltaY/95.f); if(floorsJumped<1) floorsJumped=1;
                        if(state.comboTimer>0) state.comboCount++; else state.comboCount=1; state.comboTimer=cfg.COMBO_WINDOW;
                        
                        if(state.tutorial.active && state.tutorial.currentStep == TutorialStep::LAND_ON_PLATFORMS) {
                            state.tutorial.platformsLanded++;
                        }
                        state.currentRunPlatforms++;
                        if(state.comboCount > state.currentRunBestCombo) state.currentRunBestCombo = state.comboCount;
                        
                        if(state.comboCount >= 10 && settings.screenShake) TriggerShake(state.screenShake, 2.f + state.comboCount * 0.1f, 0.08f);
                        
                        constexpr int MIN_COMBO = 10;
                        int base=50; int heightBonus=floorsJumped>1?(floorsJumped-1)*30:0;
                        int effectiveMultiplier = (state.comboCount >= MIN_COMBO) ? state.comboCount : 1; 
                        int gained=(base+heightBonus)*effectiveMultiplier; state.score+=gained;
                        state.lastLandedPlatformIndex=(int)i; state.lastScoredPlatformIndex=(int)i; state.lastLandY=pf.rect.y;
                    }
                    break;
                }
            }
        }
    }
    
    if(wantsJump){
        float speedRatio=std::fmin(std::fabs(state.player.vel.x)/cfg.MAX_HSPEED,1.f);
        float jumpSpeed=cfg.BASE_JUMP_SPEED - cfg.EXTRA_JUMP_BOOST*speedRatio;
        state.player.vel.y=jumpSpeed; state.onGround=false; state.coyoteTimer=0.f; state.jumpBufferTimer=0.f;
        state.landingSquashActive = false;
        state.currentRunJumps++;
        if(state.audio.sndJump.frameCount>0){ SetSoundVolume(state.audio.sndJump, state.audio.volJump * VOL_JUMP_MULT * VOLUME_SCALE); PlaySound(state.audio.sndJump);} 
    } else if(wantsDoubleJump) {
        state.player.vel.y = cfg.BASE_JUMP_SPEED * 0.85f;
        state.doubleJumpUsed = true;
        state.jumpBufferTimer = 0.f;
        state.doubleJumpEffectTimer = 0.3f;
        state.currentRunJumps++;
        if(state.audio.sndJump.frameCount>0){ SetSoundVolume(state.audio.sndJump, state.audio.volJump * VOL_JUMP_MULT * VOLUME_SCALE * 0.8f); PlaySound(state.audio.sndJump);}
    }
    
    if(state.comboTimer>0){ state.comboTimer-=dt; if(state.comboTimer<=0){ state.comboTimer=0; state.comboCount=0; }}

    state.wallSlidingLeft = false;
    state.wallSlidingRight = false;
    if(!state.onGround && state.player.vel.y > 0) {
        float slideDir = 0.f;
        if(state.player.pos.x <= 2.f && (IsKeyDown(state.keys.left) || (state.gamepad.active && state.gamepad.leftX < -0.4f))) {
            state.wallSlidingLeft = true;
            slideDir = 1.f;
        }
        if(state.player.pos.x + state.player.width >= (float)cfg.gameWidth - 2.f && (IsKeyDown(state.keys.right) || (state.gamepad.active && state.gamepad.leftX > 0.4f))) {
            state.wallSlidingRight = true;
            slideDir = 1.f;
        }
        if(slideDir > 0.f && state.player.vel.y > state.wallSlideGravity) {
            state.player.vel.y = state.wallSlideGravity;
        }
    }

    const float restitution=0.95f; const float wallImpulse=80.f; if(state.player.pos.x<0.f){ state.player.pos.x=0.f; if(state.player.vel.x<0){ state.player.vel.x=-state.player.vel.x*restitution + wallImpulse; EmitWallBounceParticles({0.f,state.player.pos.y+state.player.height/2},6); if(state.audio.sndBounce.frameCount>0){ SetSoundVolume(state.audio.sndBounce, state.audio.volBounce*VOLUME_SCALE); PlaySound(state.audio.sndBounce);} } } if(state.player.pos.x+state.player.width>(float)cfg.gameWidth){ state.player.pos.x=(float)cfg.gameWidth-state.player.width; if(state.player.vel.x>0){ state.player.vel.x=-state.player.vel.x*restitution - wallImpulse; EmitWallBounceParticles({(float)cfg.gameWidth,state.player.pos.y+state.player.height/2},6); if(state.audio.sndBounce.frameCount>0){ SetSoundVolume(state.audio.sndBounce, state.audio.volBounce*VOLUME_SCALE); PlaySound(state.audio.sndBounce);} } }

    UpdateMovingPlatforms(state.platforms);
    UpdatePlatformStates(state.platforms, effectiveDt);
    
    Vector2 playerCenter = {state.player.pos.x + state.player.width/2, state.player.pos.y + state.player.height/2};
    for(auto& coin : state.coins) {
        if(coin.collected) continue;
        float dist = std::sqrt(std::pow(coin.pos.x - playerCenter.x, 2) + std::pow(coin.pos.y - playerCenter.y, 2));
        if(state.coinMagnetRange > 0 && dist < state.coinMagnetRange) {
            float dx = playerCenter.x - coin.pos.x;
            float dy = playerCenter.y - coin.pos.y;
            float len = std::sqrt(dx*dx + dy*dy);
            if(len > 1) { coin.pos.x += (dx/len) * 300.f * dt; coin.pos.y += (dy/len) * 300.f * dt; }
        }
        if(dist < coin.radius + 20.f) {
            coin.collected = true;
            state.score += coin.value;
            state.sessionCoins++;
            state.totalCoinsCollected++;
            state.totalCoins++;
            state.globalCoins++;
            SaveGlobalCoins("coins.txt", state.globalCoins);
            if(state.tutorial.active && state.tutorial.currentStep == TutorialStep::COLLECT_COINS) {
                state.tutorial.coinDone = true;
            }
            if(state.audio.sndCoin.frameCount>0){ SetSoundVolume(state.audio.sndCoin, state.audio.volCoin * VOL_COIN_MULT * VOLUME_SCALE); PlaySound(state.audio.sndCoin); }
            if(settings.particles) {
                for(int j=0;j<6;j++) {
                    float ang = (float)state.rng.nextInt(360) * 3.14159f/180.f;
                    float spd = 80.f + state.rng.nextInt(60);
                    state.particles.push_back({{coin.pos.x, coin.pos.y}, {std::cos(ang)*spd, std::sin(ang)*spd}, 0.4f, 0.4f, {255,215,0,255}});
                }
            }
        }
    }
    Collectibles::UpdateCoins(state.coins, dt);
    
    for(auto& pu : state.powerups) {
        if(pu.collected) continue;
        float dist = std::sqrt(std::pow(pu.pos.x - playerCenter.x, 2) + std::pow(pu.pos.y - playerCenter.y, 2));
        if(dist < pu.radius + 20.f) {
            pu.collected = true;
            float duration = 10.f;
            switch(pu.type) {
                case PowerUpType::DOUBLE_JUMP: 
                    state.hasDoubleJump = true; 
                    state.activeDoubleJump = true;
                    state.powerUpTimers[0] = 10.f;
                    duration = 10.f; 
                    break;
                case PowerUpType::SHIELD: 
                    state.hasShield = true; 
                    state.activeShield = true;
                    state.powerUpTimers[1] = 8.f;
                    duration = 8.f; 
                    break;
                case PowerUpType::SLOW_MOTION: 
                    state.slowMotionFactor = 0.6f; 
                    state.activeSlowMotion = true;
                    state.powerUpTimers[2] = 8.f;
                    duration = 8.f; 
                    break;
                case PowerUpType::COIN_MAGNET: 
                    state.coinMagnetRange = 150.f; 
                    state.activeMagnet = true;
                    state.powerUpTimers[3] = 10.f;
                    duration = 10.f; 
                    break;
            }
            state.activePowerUps.erase(std::remove_if(state.activePowerUps.begin(), state.activePowerUps.end(),
                [&pu](const ActivePowerUp& p){ return p.type == pu.type; }), state.activePowerUps.end());
            state.activePowerUps.push_back({pu.type, duration});
            state.currentRunPowerUps++;
            if(state.audio.sndPowerUp.frameCount>0){ SetSoundVolume(state.audio.sndPowerUp, state.audio.volPowerUp * VOL_POWERUP_MULT * VOLUME_SCALE); PlaySound(state.audio.sndPowerUp); }
            if(settings.screenShake) TriggerShake(state.screenShake, 3.f, 0.1f);
        }
    }
    Collectibles::UpdatePowerUps(state.powerups, dt);
    
    for(int i=0; i<4; i++) {
        if(state.powerUpTimers[i] > 0) state.powerUpTimers[i] -= dt;
    }
    if(state.powerUpTimers[0] <= 0) { state.activeDoubleJump = false; state.hasDoubleJump = false; }
    if(state.powerUpTimers[1] <= 0) { state.activeShield = false; state.hasShield = false; }
    if(state.powerUpTimers[2] <= 0) { state.activeSlowMotion = false; state.slowMotionFactor = 1.f; }
    if(state.powerUpTimers[3] <= 0) { state.activeMagnet = false; state.coinMagnetRange = 0.f; }
    
    for(auto& ach : state.achievements) {
        if(!ach.unlocked && ach.condition(state.score, state.comboCount, state.globalCoins, state.generatedPlatformsCount)) {
            ach.unlocked = true;
            state.lastUnlockedAchievement = ach.name;
            state.achievementPopupTimer = 3.f;
        }
    }
    if(state.achievementPopupTimer > 0) state.achievementPopupTimer -= dt;

    if(state.tutorial.active) {
        UpdateTutorial(state.tutorial, dt, state.keys);
        if(state.tutorial.currentStep == TutorialStep::DONE) {
            SaveTutorialDone("tutorial_done.txt");
        }
    }

    DifficultySettings diffSettings = GetDifficultySettings(state.difficulty);
    float gapMult = diffSettings.gapMultiplier;
    float genThreshold=state.cameraTopY - 800.f;
    while(state.highestPlatformY>genThreshold){ float dynamicMax=state.currentTheme.gapMax - state.speedStage*5.f - (state.scrollActive?10.f:0.f); if(dynamicMax<state.currentTheme.gapMin+5.f) dynamicMax=state.currentTheme.gapMin+5.f; float gap=(state.currentTheme.gapMin + state.rng.nextInt((int)(dynamicMax-state.currentTheme.gapMin+1))) * gapMult; state.highestPlatformY -= gap; SpawnOnePlatform(state.highestPlatformY);} 

    float desiredY=state.player.pos.y+state.player.height/2; if(desiredY < state.cameraTopY - cfg.deadzone) state.cameraTopY = desiredY + cfg.deadzone; if(state.scrollActive){ state.stageTimer += dt; if(state.speedStage<5 && state.stageTimer>=cfg.STAGE_DURATION){ state.stageTimer-=cfg.STAGE_DURATION; state.speedStage++; if(state.speedStage<5) state.scrollSpeed*=1.25f; } state.cameraTopY -= state.scrollSpeed*dt; float topLimit=state.player.pos.y+state.player.height/2+cfg.deadzone; if(state.cameraTopY > topLimit) state.cameraTopY = topLimit; }
    state.camera.target={(float)cfg.gameWidth/2 + state.screenShake.offset.x, state.cameraTopY + state.screenShake.offset.y};
    
    float cleanupY = state.cameraTopY + cfg.gameHeight + 300.f; 
    if(state.platforms.size()>50){ state.platforms.erase(std::remove_if(state.platforms.begin(),state.platforms.end(),[&](const Platform &p){ return p.rect.y > cleanupY; }), state.platforms.end()); }
    state.coins.erase(std::remove_if(state.coins.begin(), state.coins.end(), [&](const Coin& c){ return c.pos.y > cleanupY; }), state.coins.end());
    state.powerups.erase(std::remove_if(state.powerups.begin(), state.powerups.end(), [&](const PowerUp& p){ return p.pos.y > cleanupY; }), state.powerups.end());
    
    float loseThreshold=state.cameraTopY + cfg.gameHeight/2.f + 40.f; 
    if(state.player.pos.y > loseThreshold){ 
        if(state.hasShield) {
            state.hasShield = false;
            state.activeShield = false;
            state.powerUpTimers[1] = 0.f;
            state.shieldFlashAlpha = 1.0f; // Trigger white flash
            state.player.vel.y = cfg.BASE_JUMP_SPEED * 1.4f; // Stronger bounce when shield saves you
            state.activePowerUps.erase(std::remove_if(state.activePowerUps.begin(), state.activePowerUps.end(), 
                [](const ActivePowerUp& p){ return p.type == PowerUpType::SHIELD; }), state.activePowerUps.end());
            if(settings.screenShake) TriggerShake(state.screenShake, 8.f, 0.3f);
        } else if(!state.gameOver){ 
            state.gameOver=true; 
            if(settings.screenShake) TriggerShake(state.screenShake, 10.f, 0.5f);
            if(state.isDailyRun && state.score > state.dailyChallenge.bestScore) {
                state.dailyChallenge.bestScore = state.score;
            }
            state.stats.gamesPlayed++;
            state.stats.totalScore += state.score;
            if(state.score > state.stats.bestScore) state.stats.bestScore = state.score;
            state.stats.totalCoinsCollected += state.sessionCoins;
            if(state.currentRunBestCombo > state.stats.bestCombo) state.stats.bestCombo = state.currentRunBestCombo;
            state.stats.totalPlatformsLanded += state.currentRunPlatforms;
            if(state.currentRunPlatforms > state.stats.bestPlatformStreak) state.stats.bestPlatformStreak = state.currentRunPlatforms;
            state.stats.totalPowerUpsCollected += state.currentRunPowerUps;
            state.stats.totalJumps += state.currentRunJumps;
            state.stats.deaths++;
            AddLeaderboardEntry(state.leaderboard, {state.score, state.sessionCoins, state.currentRunBestCombo, state.isDailyRun});
            if(state.audio.musicBg.ctxData){ PauseMusicStream(state.audio.musicBg); state.musicPausedOnDeath=true; } 
            if(state.audio.sndDeath.frameCount>0){ SetSoundVolume(state.audio.sndDeath, state.audio.volDeath * VOL_DEATH_MULT * VOLUME_SCALE); PlaySound(state.audio.sndDeath);}
            
            // Check if player can revive - show revive prompt instead of game over
            bool canRevive = !state.hasRevivedThisRun && state.globalCoins >= state.reviveCost && !state.isDailyRun;
            if(canRevive) {
                state.reviveTimer = state.REVIVE_TIME_LIMIT;
                ChangeScreen(GameState::Screen::REVIVE_PROMPT, false);
            }
        } 
    }
}

void Game::AutoSaveSettings(float dt){
    if(!settingsDirty) return;
    settingsSaveTimer += dt;
    if(settingsSaveTimer >= 5.f){
        settings.resolutionIndex = resolutionIndex;
        settings.fullscreen = fullscreen;
        settings.master = state.audio.masterSlider;
        settings.music = state.audio.volMusic;
        settings.jump = state.audio.volJump;
        settings.bounce = state.audio.volBounce;
        settings.death = state.audio.volDeath;
        settings.theme = state.audio.volThemeChange;
        settings.coin = state.audio.volCoin;
        settings.powerup = state.audio.volPowerUp;
        settings.keyLeft = state.keys.left;
        settings.keyRight = state.keys.right;
        settings.keyJump = state.keys.jump;
        SaveSettings("settings.cfg", settings);
        settingsDirty = false;
        settingsSaveTimer = 0.f;
        DEBUG_LOG("Settings autosaved");
    }
}

void Game::ResetSettingsToDefaults(){
    DEBUG_LOG("ResetSettingsToDefaults");
    state.audio.masterSlider = 0.5f;
    state.audio.volMusic = 0.5f;
    state.audio.volJump = 0.5f;
    state.audio.volBounce = 0.5f;
    state.audio.volDeath = 0.5f;
    state.audio.volThemeChange = 0.5f;
    state.audio.volCoin = 0.5f;
    state.audio.volPowerUp = 0.5f;
    state.keys.left = KEY_A;
    state.keys.right = KEY_D;
    state.keys.jump = KEY_SPACE;
    settings.showFPS = false;
    settings.configVersion = 2; 
    ApplyAudioVolumes();
    settingsDirty = true;
    settingsSaveTimer = 0.f;
}

void Game::UpdateFade(float dt){
    if(std::fabs(state.fadeAlpha - state.fadeTarget) < 0.01f){ state.fadeAlpha = state.fadeTarget; return; }
    if(state.fadeAlpha < state.fadeTarget){ state.fadeAlpha += state.fadeSpeed*dt; if(state.fadeAlpha>state.fadeTarget) state.fadeAlpha=state.fadeTarget; }
    else { state.fadeAlpha -= state.fadeSpeed*dt; if(state.fadeAlpha<state.fadeTarget) state.fadeAlpha=state.fadeTarget; }
}

void Game::DrawResolutionSelector(int &y, float uiCenterX, Vector2 mPos, bool click, int sw, float scale){
    auto S = [scale](int v) { return (int)(v * scale); };
    auto clampX=[&](int desired,int w){ int x=desired; if(x<S(10)) x=S(10); if(x+w>sw-S(10)) x=sw-S(10)-w; return x; };
    int rw=S(300); int rh=S(34); int rx=clampX((int)(uiCenterX - rw/2),rw);
    Rectangle resBox{(float)rx,(float)y,(float)rw,(float)rh};
    DrawRectangleRec(resBox,{40,50,70,255}); DrawRectangleLines(rx,y,rw,rh,RAYWHITE);
    int arrowW=S(32); Rectangle leftA{(float)(rx+S(4)),(float)(y+S(4)),(float)arrowW,(float)(rh-S(8))}; Rectangle rightA{(float)(rx+rw-arrowW-S(4)),(float)(y+S(4)),(float)arrowW,(float)(rh-S(8))};
    auto hover=[&](Rectangle r){ return CheckCollisionPointRec(mPos,r); };
    if(hover(leftA)) DrawRectangleRec(leftA,{70,90,130,255}); else DrawRectangleRec(leftA,{60,80,120,255});
    if(hover(rightA)) DrawRectangleRec(rightA,{70,90,130,255}); else DrawRectangleRec(rightA,{60,80,120,255});
    int arrowFont = S(20);
    DrawText("<", (int)(leftA.x+arrowW/2 - MeasureText("<",arrowFont)/2), (int)(leftA.y+S(6)),arrowFont,RAYWHITE);
    DrawText(">", (int)(rightA.x+arrowW/2 - MeasureText(">",arrowFont)/2), (int)(rightA.y+S(6)),arrowFont,RAYWHITE);
    if(click && hover(leftA)){ if(resolutionIndex>0){ resolutionIndex--; ApplyResolution(); settingsDirty=true; settingsSaveTimer=0.f; } }
    if(click && hover(rightA)){ if(resolutionIndex<RESOLUTION_COUNT-1){ resolutionIndex++; ApplyResolution(); settingsDirty=true; settingsSaveTimer=0.f; } }
    int labelFont = S(18);
    int cw = kResolutions[resolutionIndex].w; int ch = kResolutions[resolutionIndex].h; std::string resLabel = std::to_string(cw) + "x" + std::to_string(ch); int rtw=MeasureText(resLabel.c_str(),labelFont); DrawText(resLabel.c_str(), rx + rw/2 - rtw/2, y+S(8), labelFont, RAYWHITE);
    y += rh + S(10);
}

void Game::ChangeScreen(GameState::Screen next, bool withFade){
    DEBUG_LOG("ChangeScreen -> %d fade=%d", (int)next, (int)withFade);
    if(withFade){
    state.fadeAlpha=0.f; state.fadeTarget=0.55f;
    }
    state.currentScreen = next;
    if(withFade){ state.fadeTarget=0.f; }
    if(next==GameState::Screen::MENU && state.musicPausedOnDeath && state.audio.musicBg.ctxData){
        StopMusicStream(state.audio.musicBg);
        PlayMusicStream(state.audio.musicBg);
        SetMusicVolume(state.audio.musicBg, state.audio.volMusic * VOL_MUSIC_MULT);
        state.musicPausedOnDeath=false;
    }
}

void Game::ApplyAudioVolumes(){
    auto clamp01=[](float v){ return v<0?0.f:(v>1?1.f:v); };
    state.audio.masterSlider = clamp01(state.audio.masterSlider);
    state.audio.volMusic = clamp01(state.audio.volMusic);
    state.audio.volJump = clamp01(state.audio.volJump);
    state.audio.volBounce = clamp01(state.audio.volBounce);
    state.audio.volDeath = clamp01(state.audio.volDeath);
    state.audio.volThemeChange = clamp01(state.audio.volThemeChange);
    state.audio.masterVolume = 0.0001f + state.audio.masterSlider;
    SetMasterVolume(state.audio.masterVolume);
    if(state.audio.musicBg.ctxData){ SetMusicVolume(state.audio.musicBg, state.audio.volMusic * VOL_MUSIC_MULT); }
}

void Game::ApplyMenuAudioVolumes(){
    state.audio.masterVolume = 0.0001f + state.audio.masterSlider * 1.0f;
    SetMasterVolume(state.audio.masterVolume);
    if(state.audio.musicBg.ctxData) {
        SetMusicVolume(state.audio.musicBg, state.audio.volMusic * VOL_MUSIC_MULT);
    }
}

void Game::DrawAudioSliders(int &y, float uiCenterX, Vector2 mPos, int sw, bool &changedOut){
    struct AudioSliderSpec { const char* name; float GameAudio::* member; };
    static constexpr AudioSliderSpec specs[] = {
        {"Master", &GameAudio::masterSlider},
    {"Music", &GameAudio::volMusic},
    {"Jump", &GameAudio::volJump},
    {"Bounce", &GameAudio::volBounce},
    {"Death", &GameAudio::volDeath},
    {"Theme", &GameAudio::volThemeChange}
    };
    bool changedLocal=false;
    for(auto &s: specs){ float &ref = state.audio.*(s.member); GuiVolumeSlider(uiCenterX, y, s.name, ref, mPos, 10, sw-10, changedLocal); }
    if(changedLocal) changedOut=true;
}
