#include "game.h"
#include "debug.h"
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cmath>
#if defined(_WIN32)
// forward declare helper implemented in win_icon.cpp
void SetWindowsTaskbarIconFromResource();
#endif
#ifdef __APPLE__
extern "C" void SetDockIcon(const char* path);
#endif

Game::Game(const GameConfig &cfg):cfg(cfg){
    DEBUG_LOG("Game ctor start (%dx%d)", cfg.screenWidth, cfg.screenHeight);
    InitWindow(cfg.screenWidth,cfg.screenHeight,"Shapey Tower");
    // App icon: try common paths (window/dock where supported)
    const char* iconPaths[] = {"assets/icons/shapeyicon.png","icons/shapeyicon.png","shapeyicon.png"};
    for(const char* ip: iconPaths){
        if(FileExists(ip)){
            Image icon = LoadImage(ip);
            if(icon.data){
                // Ensure RGBA (R8G8B8A8) for icon compatibility
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
    // Set process/window icon from RC so Windows taskbar picks correct sizes
    SetWindowsTaskbarIconFromResource();
#endif
    windowedW = cfg.screenWidth; windowedH = cfg.screenHeight;
    // Load user settings (ignore if missing)
    LoadSettings("settings.cfg", settings);
    resolutionIndex = settings.resolutionIndex;
    fullscreen = settings.fullscreen;
    // Key bindings
    state.keys.left = settings.keyLeft;
    state.keys.right = settings.keyRight;
    state.keys.jump = settings.keyJump;
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    InitAudioDevice();
    state.audio = GameAudio{};
    // Nadpisz suwakami z settings (clamp 0..1)
    auto clamp01=[](float v){ return v<0?0.f:(v>1?1.f:v); };
    state.audio.masterSlider = clamp01(settings.master);
    state.audio.volMusic = clamp01(settings.music);
    state.audio.volJump = clamp01(settings.jump);
    state.audio.volBounce = clamp01(settings.bounce);
    state.audio.volDeath = clamp01(settings.death);
    state.audio.volThemeChange = clamp01(settings.theme);
    state.audio.masterVolume = 0.0001f + state.audio.masterSlider;
    SetMasterVolume(state.audio.masterVolume);
    LoadGameAudio(state.audio);
    if(state.audio.musicBg.ctxData){
        SetMusicVolume(state.audio.musicBg, state.audio.volMusic);
        PlayMusicStream(state.audio.musicBg);
    }
    std::srand((unsigned)std::time(nullptr));
    state.themes = GetThemes();
    state.currentThemeIndex=0; state.currentTheme=state.themes[0];
    state.player = Player{ {cfg.screenWidth/2.f-16.f, cfg.screenHeight-120.f},{0,0},32.f,40.f };
    state.platforms.push_back({Rectangle{0,(float)cfg.screenHeight-60.f,(float)cfg.screenWidth,20.f}});
    state.highScore = LoadHighScore("highscore.txt");
    // initial generation
    auto spawnPlatform=[&](float y){ float width=80.f+(std::rand()%140); float x=(float)(std::rand()%(int)(cfg.screenWidth-(int)width)); Platform p; p.rect={x,y,width,18.f}; if((std::rand()%100)<state.currentTheme.moveChance){ p.moving=true; p.baseX=x; p.moveAmplitude=40.f+(std::rand()%41); p.moveSpeed=1.f+(std::rand()%200)/100.f; } state.platforms.push_back(p); state.generatedPlatformsCount++; };
    float currentY=state.platforms[0].rect.y-100.f; for(int i=0;i<15;i++){ float gap=state.currentTheme.gapMin+(std::rand()%(int)(state.currentTheme.gapMax-state.currentTheme.gapMin+1)); currentY-=gap; spawnPlatform(currentY);} state.highestPlatformY=currentY;
    state.camera.target={ (float)cfg.screenWidth/2, state.player.pos.y+state.player.height/2};
    state.camera.offset={ (float)cfg.screenWidth/2,(float)cfg.screenHeight/2};
    state.camera.zoom=1.f;
    state.cameraTopY=state.camera.target.y;
    state.currentScreen = GameState::Screen::MENU;
    // Load player sprite (prefer assets/textures, then project root)
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
                            bottomPad = img.height-1 - y; // trailing transparent rows
                            found=true; break;
                        }
                    }
                }
                state.playerSpriteBottomPad = (float)bottomPad; // used to avoid "floating" look when drawing
                UnloadImage(img);
                LOG_INFO("Sprite bottomPad=%d", bottomPad);
            }
            break; // first available
        }
    }
    // Load fire shader (aura + sprite modulation on combo)
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
    ApplyResolution();
    ApplyAudioVolumes();
}

Game::~Game(){
    // Save settings on exit
    settings.resolutionIndex = resolutionIndex;
    settings.fullscreen = fullscreen;
    settings.master = state.audio.masterSlider;
    settings.music = state.audio.volMusic;
    settings.jump = state.audio.volJump;
    settings.bounce = state.audio.volBounce;
    settings.death = state.audio.volDeath;
    settings.theme = state.audio.volThemeChange;
    // Persist current key bindings
    settings.keyLeft = state.keys.left;
    settings.keyRight = state.keys.right;
    settings.keyJump = state.keys.jump;
    // showFPS already toggled in settings during runtime (KEY_F3)
    SaveSettings("settings.cfg", settings);
    SaveHighScore("highscore.txt", state.highScore);
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
    bool isFs = IsWindowFullscreen();
    if(fullscreen != isFs){
        if(fullscreen){
            windowedW = GetScreenWidth();
            windowedH = GetScreenHeight();
            ToggleFullscreen();
        } else {
            ToggleFullscreen();
            SetWindowSize(windowedW, windowedH);
        }
        isFs = !isFs;
    }
    cfg.screenWidth = newW;
    cfg.screenHeight = newH;
    if(!isFs){
        SetWindowSize(newW,newH);
        windowedW = newW; windowedH = newH;
    }
    state.camera.offset = {(float)newW/2.f,(float)newH/2.f};
    if(recenterCamera){ state.camera.target.x = (float)newW/2.f; }
    for(auto &pf: state.platforms){ if(pf.rect.x + pf.rect.width > cfg.screenWidth){ pf.rect.x = cfg.screenWidth - pf.rect.width; if(pf.rect.x < 0) pf.rect.x = 0; } }
    if(state.player.pos.x + state.player.width > cfg.screenWidth){ state.player.pos.x = cfg.screenWidth - state.player.width; }
    if(state.player.pos.x < 0) state.player.pos.x = 0;
}

bool Game::ShouldClose() const { return !running || WindowShouldClose(); }

void Game::ResetGame(){
    state.score=0; state.comboTimer=0.f; state.comboCount=0; state.lastLandedPlatformIndex=0; state.gameOver=false; state.platforms.clear();
    state.platforms.push_back({Rectangle{0,(float)cfg.screenHeight-60.f,(float)cfg.screenWidth,20.f}});
    state.player.pos = {cfg.screenWidth/2.f - state.player.width/2, state.platforms[0].rect.y - state.player.height};
    state.player.vel={0,0};
    state.currentThemeIndex=0; state.currentTheme=state.themes[0]; state.themeChangeTimer=2.f; state.generatedPlatformsCount=1; state.lastScoredPlatformIndex=-1; state.lastLandY=state.platforms[0].rect.y;
    auto spawnPlatformLocal=[&](float y){ float w=80.f+(std::rand()%140); float x=(float)(std::rand()%(int)(cfg.screenWidth-(int)w)); Platform p; p.rect={x,y,w,18.f}; if((std::rand()%100)<25){ p.moving=true; p.baseX=x; p.moveAmplitude=40.f+(std::rand()%41); p.moveSpeed=1.f+(std::rand()%200)/100.f; } state.platforms.push_back(p); };
    float currentY2=state.platforms[0].rect.y-100.f; for(int i=0;i<15;i++){ float gap=state.currentTheme.gapMin+(std::rand()%(int)(state.currentTheme.gapMax-state.currentTheme.gapMin+1)); currentY2-=gap; spawnPlatformLocal(currentY2); state.generatedPlatformsCount++; }
    state.highestPlatformY=currentY2; state.cameraTopY=state.player.pos.y+state.player.height/2; state.scrollActive=false; state.speedStage=0; state.stageTimer=0.f; state.scrollSpeed=60.f;
    // Resume background music only if it was paused on death; if started from MENU, keep current position.
    if(state.musicPausedOnDeath && state.audio.musicBg.ctxData){
        StopMusicStream(state.audio.musicBg); // reset pozycji do 0
        PlayMusicStream(state.audio.musicBg);
        SetMusicVolume(state.audio.musicBg, state.audio.volMusic * VOL_MUSIC_MULT);
        state.musicPausedOnDeath=false;
    }
}

void Game::EmitLandingParticles(Vector2 contact,int count){ for(int i=0;i<count;i++){ float spd=60.f+(std::rand()%120); float ang=(-90.f+(std::rand()%120)-60.f)*(3.14159f/180.f); Vector2 v{std::cos(ang)*spd,std::sin(ang)*spd}; state.particles.push_back({contact,v,0.6f,0.6f,state.currentTheme.platStatic}); }}
void Game::EmitWallBounceParticles(Vector2 contact,int count){
    // Determine which wall side (left/right) based on contact.x
    bool left = (contact.x < state.player.width*0.5f + 2.f);
    bool right = !left;
    for(int i=0;i<count;i++){
        float spd = 120.f + (std::rand()%160);
        float deg;
        if(left){ deg = -60.f + (std::rand()%120); }
        else { deg = 120.f + (std::rand()%120); }
        float ang = deg * (3.1415926f/180.f);
    Vector2 v{std::cos(ang)*spd, std::sin(ang)*spd};
        Color c = (i%2==0)? state.currentTheme.platStatic : state.currentTheme.playerBody;
        state.particles.push_back({contact,v,0.55f,0.55f,c});
    }
}

void Game::EnsureRenderTarget(){
    if(gameRT.id==0 || gameRT.texture.width!=cfg.screenWidth || gameRT.texture.height!=cfg.screenHeight){
        if(gameRT.id>0) UnloadRenderTexture(gameRT);
        gameRT = LoadRenderTexture(cfg.screenWidth,cfg.screenHeight);
    }
}

void Game::Update(){
    PROF_SCOPE("Game::Update");
    if(ShouldClose()){ running=false; return; }
    if(IsKeyPressed(KEY_GRAVE)) { running=false; return; }
    if(IsKeyPressed(KEY_F3)) { settings.showFPS = !settings.showFPS; settingsDirty = true; settingsSaveTimer = 0.f; }
    if(IsKeyPressed(KEY_ESCAPE)){
        switch(state.currentScreen){
            case GameState::Screen::MENU: running=false; break;
            case GameState::Screen::GAME: ChangeScreen(GameState::Screen::PAUSE); break;
            case GameState::Screen::PAUSE: ChangeScreen(GameState::Screen::GAME); break;
            case GameState::Screen::GAMEOVER: ChangeScreen(GameState::Screen::MENU); break;
        }
    }
    float dt=GetFrameTime();
    AutoSaveSettings(dt);
    if(state.audio.musicBg.ctxData){ UpdateMusicStream(state.audio.musicBg); }
    UpdateFade(dt);
    switch(state.currentScreen){
        case GameState::Screen::MENU: DrawMenu(); break;
        case GameState::Screen::PAUSE: DrawPause(); break;
        case GameState::Screen::GAME:
            UpdateGameplay(dt);
            DrawGame();
            break;
        case GameState::Screen::GAMEOVER:
            DrawGame();
            break;
    }
}

void Game::UpdateGameplay(float dt){
    PROF_SCOPE("UpdateGameplay");
    if(state.gameOver){ ChangeScreen(GameState::Screen::GAMEOVER,false); }
    state.animTime += dt;
    if(state.landingTimer>0.f){ state.landingTimer -= dt; if(state.landingTimer<0) state.landingTimer=0; }
    // track last vertical velocity before physics integration for hard landing detection
    state.lastVerticalVelocity = state.player.vel.y;
    float dir=0.f; if(IsKeyDown(state.keys.left)) dir-=1.f; if(IsKeyDown(state.keys.right)) dir+=1.f;
    bool jumpPressed=IsKeyPressed(state.keys.jump); bool jumpDown=IsKeyDown(state.keys.jump);
    if(jumpPressed || jumpDown) state.jumpBufferTimer=cfg.JUMP_BUFFER; else if(state.jumpBufferTimer>0) state.jumpBufferTimer-=dt;
    if(state.onGround) state.coyoteTimer=cfg.COYOTE_TIME; else if(state.coyoteTimer>0) state.coyoteTimer-=dt;
    bool wantsJump = (state.jumpBufferTimer>0 && state.coyoteTimer>0);

    Vector2 prevPos=state.player.pos;
    UpdatePlayerPhysics(state.player, dt, dir, state.onGround, cfg.MOVE_ACCEL, cfg.MAX_HSPEED, cfg.FRICTION, cfg.GRAVITY);

    state.onGround=false;
    if(state.player.vel.y>0){
        float prevBottom=prevPos.y+state.player.height;
        float currBottom=state.player.pos.y+state.player.height;
        for(size_t i=0;i<state.platforms.size();++i){
            auto &pf=state.platforms[i]; float topY=pf.rect.y;
            if(prevBottom<=topY && currBottom>=topY){
                float pL=state.player.pos.x, pR=state.player.pos.x+state.player.width;
                float platL=pf.rect.x, platR=pf.rect.x+pf.rect.width;
                if(pR>platL && pL<platR){
                    state.player.pos.y=topY-state.player.height;
                    state.player.vel.y=0; state.onGround=true;
                    // Hard landing detection
                    float impactSpeed = std::fabs(state.lastVerticalVelocity);
                    if(impactSpeed >= state.hardLandingThreshold){
                        state.landingSquashActive = true;
                        state.landingSquashTime = 0.f;
                        EmitLandingParticles({state.player.pos.x+state.player.width/2, topY},14+std::rand()%8);
                    } else {
                        EmitLandingParticles({state.player.pos.x+state.player.width/2, topY},6+std::rand()%6);
                    }
                    state.landingTimer = 0.f; // disable legacy continuous stretch/squash
                    // (removed) land sound disabled
                    if(!state.scrollActive && (int)i>0){ state.scrollActive=true; state.stageTimer=0.f; }
                    if((int)i!=state.lastScoredPlatformIndex){
                        float deltaY=state.lastLandY-pf.rect.y;
                        int floorsJumped=(int)(deltaY/95.f); if(floorsJumped<1) floorsJumped=1;
                        if(state.comboTimer>0) state.comboCount++; else state.comboCount=1; state.comboTimer=cfg.COMBO_WINDOW;
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
    // reset squash on jump
    state.landingSquashActive = false;
    if(state.audio.sndJump.frameCount>0){ SetSoundVolume(state.audio.sndJump, state.audio.volJump * VOL_JUMP_MULT * VOLUME_SCALE); PlaySound(state.audio.sndJump);} 
    }
    if(state.comboTimer>0){ state.comboTimer-=dt; if(state.comboTimer<=0){ state.comboTimer=0; state.comboCount=0; }}

    const float restitution=0.95f; const float wallImpulse=80.f; if(state.player.pos.x<0.f){ state.player.pos.x=0.f; if(state.player.vel.x<0){ state.player.vel.x=-state.player.vel.x*restitution + wallImpulse; EmitWallBounceParticles({0.f,state.player.pos.y+state.player.height/2},6); if(state.audio.sndBounce.frameCount>0){ SetSoundVolume(state.audio.sndBounce, state.audio.volBounce*VOLUME_SCALE); PlaySound(state.audio.sndBounce);} } } if(state.player.pos.x+state.player.width>(float)cfg.screenWidth){ state.player.pos.x=(float)cfg.screenWidth-state.player.width; if(state.player.vel.x>0){ state.player.vel.x=-state.player.vel.x*restitution - wallImpulse; EmitWallBounceParticles({(float)cfg.screenWidth,state.player.pos.y+state.player.height/2},6); if(state.audio.sndBounce.frameCount>0){ SetSoundVolume(state.audio.sndBounce, state.audio.volBounce*VOLUME_SCALE); PlaySound(state.audio.sndBounce);} } }

    UpdateMovingPlatforms(state.platforms);

    auto applyThemeIfNeeded=[&](){ int stage=state.generatedPlatformsCount/PLATFORMS_PER_THEME; int nextTheme=stage%(int)state.themes.size(); if(nextTheme!=state.currentThemeIndex){ state.currentThemeIndex=nextTheme; state.currentTheme=state.themes[state.currentThemeIndex]; state.themeChangeTimer=3.f; if(state.audio.sndThemeChange.frameCount>0){ SetSoundVolume(state.audio.sndThemeChange, state.audio.volThemeChange * VOL_THEME_MULT * VOLUME_SCALE); PlaySound(state.audio.sndThemeChange);} } };
    auto spawnPlatform=[&](float y){ float width=80.f+(std::rand()%140); float x=(float)(std::rand()%(int)(cfg.screenWidth-(int)width)); Platform p; p.rect={x,y,width,18.f}; if((std::rand()%100)<state.currentTheme.moveChance){ p.moving=true; p.baseX=x; p.moveAmplitude=40.f+(std::rand()%41); p.moveSpeed=1.f+(std::rand()%200)/100.f; } state.platforms.push_back(p); state.generatedPlatformsCount++; applyThemeIfNeeded(); };

    float genThreshold=state.cameraTopY - 800.f;
    while(state.highestPlatformY>genThreshold){ float dynamicMax=state.currentTheme.gapMax - state.speedStage*5.f - (state.scrollActive?10.f:0.f); if(dynamicMax<state.currentTheme.gapMin+5.f) dynamicMax=state.currentTheme.gapMin+5.f; float gap=state.currentTheme.gapMin + (std::rand()%(int)(dynamicMax-state.currentTheme.gapMin+1)); state.highestPlatformY -= gap; spawnPlatform(state.highestPlatformY);} 

    float desiredY=state.player.pos.y+state.player.height/2; if(desiredY < state.cameraTopY - cfg.deadzone) state.cameraTopY = desiredY + cfg.deadzone; if(state.scrollActive){ state.stageTimer += dt; if(state.speedStage<5 && state.stageTimer>=cfg.STAGE_DURATION){ state.stageTimer-=cfg.STAGE_DURATION; state.speedStage++; if(state.speedStage<5) state.scrollSpeed*=1.25f; } state.cameraTopY -= state.scrollSpeed*dt; float topLimit=state.player.pos.y+state.player.height/2+cfg.deadzone; if(state.cameraTopY > topLimit) state.cameraTopY = topLimit; }
    state.camera.target={(float)cfg.screenWidth/2,state.cameraTopY};
    float cleanupY = state.cameraTopY + cfg.screenHeight + 300.f; if(state.platforms.size()>50){ state.platforms.erase(std::remove_if(state.platforms.begin(),state.platforms.end(),[&](const Platform &p){ return p.rect.y > cleanupY; }), state.platforms.end()); }
    float loseThreshold=state.cameraTopY + cfg.screenHeight/2.f + 40.f; if(state.player.pos.y > loseThreshold){ if(!state.gameOver){ state.gameOver=true; if(state.audio.musicBg.ctxData){ PauseMusicStream(state.audio.musicBg); state.musicPausedOnDeath=true; } if(state.audio.sndDeath.frameCount>0){ SetSoundVolume(state.audio.sndDeath, state.audio.volDeath * VOL_DEATH_MULT * VOLUME_SCALE); PlaySound(state.audio.sndDeath);} } }
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

void Game::DrawResolutionSelector(int &y, float uiCenterX, Vector2 mPos, bool click, int sw){
    auto clampX=[&](int desired,int w){ int x=desired; if(x<10) x=10; if(x+w>sw-10) x=sw-10-w; return x; };
    constexpr int rw=300; constexpr int rh=34; int rx=clampX((int)(uiCenterX - rw/2),rw);
    Rectangle resBox{(float)rx,(float)y,(float)rw,(float)rh};
    DrawRectangleRec(resBox,{40,50,70,255}); DrawRectangleLines(rx,y,rw,rh,RAYWHITE);
    int arrowW=32; Rectangle leftA{(float)(rx+4),(float)(y+4),(float)arrowW,(float)(rh-8)}; Rectangle rightA{(float)(rx+rw-arrowW-4),(float)(y+4),(float)arrowW,(float)(rh-8)};
    auto hover=[&](Rectangle r){ return CheckCollisionPointRec(mPos,r); };
    if(hover(leftA)) DrawRectangleRec(leftA,{70,90,130,255}); else DrawRectangleRec(leftA,{60,80,120,255});
    if(hover(rightA)) DrawRectangleRec(rightA,{70,90,130,255}); else DrawRectangleRec(rightA,{60,80,120,255});
    DrawText("<", (int)(leftA.x+arrowW/2 - MeasureText("<",20)/2), (int)(leftA.y+6),20,RAYWHITE);
    DrawText(">", (int)(rightA.x+arrowW/2 - MeasureText(">",20)/2), (int)(rightA.y+6),20,RAYWHITE);
    if(click && hover(leftA)){ if(resolutionIndex>0){ resolutionIndex--; ApplyResolution(); settingsDirty=true; settingsSaveTimer=0.f; } }
    if(click && hover(rightA)){ if(resolutionIndex<RESOLUTION_COUNT-1){ resolutionIndex++; ApplyResolution(); settingsDirty=true; settingsSaveTimer=0.f; } }
    int cw = kResolutions[resolutionIndex].w; int ch = kResolutions[resolutionIndex].h; std::string resLabel = std::to_string(cw) + "x" + std::to_string(ch); int rtw=MeasureText(resLabel.c_str(),18); DrawText(resLabel.c_str(), rx + rw/2 - rtw/2, y+8, 18, RAYWHITE);
    y += rh + 10;
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