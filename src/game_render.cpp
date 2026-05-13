#include "game.h"
#include "raylib.h"
#include "collectibles.h"
#include "localization.h"
#include "daily_challenge.h"
#include <cmath>

#include "debug.h"
#include "constants.h"
static void DrawVerticalGradient(int w,int h, Color top, Color bottom){
    for(int y=0;y<h;y+=Const::GRADIENT_STEP){ float k=(float)y/h; unsigned char r=(unsigned char)(top.r+(bottom.r-top.r)*k); unsigned char g=(unsigned char)(top.g+(bottom.g-top.g)*k); unsigned char b=(unsigned char)(top.b+(bottom.b-top.b)*k); DrawRectangle(0,y,w,Const::GRADIENT_STEP,{r,g,b,255}); }
}

static Color GetPlatformColor(const Platform& pf, Color baseMoving, Color baseStatic) {
    Color base = pf.moving ? baseMoving : baseStatic;
    switch(pf.type) {
        case PlatformType::CRUMBLING:
            return Color{180, 120, 80, (unsigned char)(255 * (1.0f - pf.crumbleProgress * 0.5f))};
        case PlatformType::SPRING:
            return Color{80, 200, 120, 255};
        case PlatformType::ICE:
            return Color{150, 220, 255, 230};
        case PlatformType::DISAPPEARING:
            return Color{base.r, base.g, base.b, (unsigned char)(pf.alpha * 255)};
        default:
            return base;
    }
}

void Game::DrawGameWorld(float dt){
    DrawVerticalGradient(cfg.gameWidth,cfg.gameHeight, state.currentTheme.bgTop, state.currentTheme.bgBottom);
    Particles::Update(state.particles, dt, cfg.GRAVITY, 0.2f);
    BeginMode2D(state.camera);
    for(auto &pf:state.platforms){ 
        if(!pf.visible) continue;
        Color c = GetPlatformColor(pf, state.currentTheme.platMoving, state.currentTheme.platStatic);
        DrawRectangleRec(pf.rect,c); 
        if(pf.type == PlatformType::SPRING) {
            float springY = pf.rect.y - 5 + std::sin(state.animTime * 8.f) * 2.f;
            DrawRectangle((int)(pf.rect.x + pf.rect.width/4), (int)springY, (int)(pf.rect.width/2), 5, Color{50,180,90,255});
        }
        if(pf.type == PlatformType::ICE) {
            for(int i=0; i<3; i++) {
                float sparkleX = pf.rect.x + (i+1) * pf.rect.width/4.f;
                float sparkleY = pf.rect.y + 3;
                unsigned char a = (unsigned char)(180 + 75 * std::sin(state.animTime * 4.f + i));
                DrawCircle((int)sparkleX, (int)sparkleY, 2, Color{255,255,255,a});
            }
        }
        if(pf.type == PlatformType::CRUMBLING && pf.crumbleProgress > 0) {
            int cracks = (int)(pf.crumbleProgress * 5);
            for(int i=0; i<cracks; i++) {
                float cx = pf.rect.x + (i+1) * pf.rect.width / (cracks+1);
                DrawLine((int)cx, (int)pf.rect.y, (int)(cx + 3), (int)(pf.rect.y + pf.rect.height), Color{60,40,20,200});
            }
        }
        DrawRectangle((int)pf.rect.x,(int)pf.rect.y,(int)pf.rect.width,3,{255,255,255,60}); 
        DrawRectangleLines((int)pf.rect.x,(int)pf.rect.y,(int)pf.rect.width,(int)pf.rect.height,{255,255,255,30}); 
    }
    Collectibles::DrawCoins(state.coins, state.animTime);
    Collectibles::DrawPowerUps(state.powerups, state.animTime);
    Particles::Draw(state.particles);
    if(state.playerTexture.id>0){
        Rectangle src{0,0,(float)state.playerTexture.width,(float)state.playerTexture.height};
        float baseScale=state.playerSpriteScale; float dstW=state.player.width*baseScale; float dstH=state.player.height*baseScale; float padRatio=(state.playerTexture.height>0)?(state.playerSpriteBottomPad/(float)state.playerTexture.height):0.f;
        float vx=state.player.vel.x; float lean=vx/cfg.MAX_HSPEED; if(lean>1) lean=1; if(lean<-1) lean=-1; float leanDeg=lean*8.f; float finalW=dstW, finalH=dstH;
    if(state.landingSquashActive){ state.landingSquashTime+=dt; float t=state.landingSquashTime/state.landingSquashDuration; if(t>1){ t=1; state.landingSquashActive=false; } float e=1.f-std::pow(1.f-t,3.f); float squashAmt=0.55f; finalW*=1.f+squashAmt*(1.f-e); finalH*=1.f - squashAmt*0.65f*(1.f-e); }
        float dstX=state.player.pos.x + (state.player.width-finalW)/2.f; float baseY=state.player.pos.y - (dstH - state.player.height) + state.playerSpriteYOffset + padRatio*dstH; float dstY=baseY - (finalH - dstH); Rectangle dst{dstX,dstY,finalW,finalH}; if(vx<-10.f) src.width=-src.width;
    constexpr int MIN_COMBO=Const::COMBO_MIN_MULT; bool comboActive=(settings.comboEffects && state.comboCount>=MIN_COMBO && state.comboTimer>0 && state.shaderFire.id>0);
    if(comboActive){ float intensity=std::fmin(1.f,(float)(state.comboCount-1)/6.f); float pulse=(std::sin(state.animTime*5.f)+1.f)*0.5f; float finalIntensity=(0.5f+0.5f*pulse)*intensity; Vector2 sprSize{(float)state.playerTexture.width,(float)state.playerTexture.height}; BeginBlendMode(BLEND_ADDITIVE); BeginShaderMode(state.shaderFire); if(state.fireLocTime>=0) SetShaderValue(state.shaderFire,state.fireLocTime,&state.animTime,SHADER_UNIFORM_FLOAT); if(state.fireLocIntensity>=0) SetShaderValue(state.shaderFire,state.fireLocIntensity,&finalIntensity,SHADER_UNIFORM_FLOAT); if(state.fireLocSpriteSize>=0) SetShaderValue(state.shaderFire,state.fireLocSpriteSize,&sprSize,SHADER_UNIFORM_VEC2); if(state.fireLocMode>=0){ int mode=1; SetShaderValue(state.shaderFire,state.fireLocMode,&mode,SHADER_UNIFORM_INT);} Rectangle aura=dst; aura.x-=aura.width*Const::AURA_OFFSET_X; aura.y-=aura.height*Const::AURA_OFFSET_Y; aura.width*=Const::AURA_GROW_W; aura.height*=Const::AURA_GROW_H; DrawTexturePro(state.playerTexture,src,aura,{0,0},leanDeg,WHITE); EndShaderMode(); EndBlendMode(); BeginShaderMode(state.shaderFire); if(state.fireLocTime>=0) SetShaderValue(state.shaderFire,state.fireLocTime,&state.animTime,SHADER_UNIFORM_FLOAT); if(state.fireLocIntensity>=0) SetShaderValue(state.shaderFire,state.fireLocIntensity,&finalIntensity,SHADER_UNIFORM_FLOAT); if(state.fireLocSpriteSize>=0) SetShaderValue(state.shaderFire,state.fireLocSpriteSize,&sprSize,SHADER_UNIFORM_VEC2); if(state.fireLocMode>=0){ int mode=0; SetShaderValue(state.shaderFire,state.fireLocMode,&mode,SHADER_UNIFORM_INT);} DrawTexturePro(state.playerTexture,src,dst,{0,0},leanDeg,WHITE); EndShaderMode(); }
        else DrawTexturePro(state.playerTexture,src,dst,{0,0},leanDeg,WHITE);
    } else {
        DrawRectangle((int)state.player.pos.x,(int)state.player.pos.y,(int)state.player.width,(int)state.player.height,state.currentTheme.playerBody);
        DrawRectangleLines((int)state.player.pos.x,(int)state.player.pos.y,(int)state.player.width,(int)state.player.height,{40,40,40,255});
    }
    
    if(settings.powerUpEffects && state.doubleJumpEffectTimer > 0) {
        float t = 1.f - (state.doubleJumpEffectTimer / 0.3f); 
        float radius = 15.f + t * 40.f; 
        unsigned char alpha = (unsigned char)(200 * (1.f - t)); 
        float cx = state.player.pos.x + state.player.width/2;
        float cy = state.player.pos.y + state.player.height/2;
        DrawCircleLines((int)cx, (int)cy, radius, Color{150, 220, 255, alpha});
        DrawCircleLines((int)cx, (int)cy, radius * 0.7f, Color{200, 240, 255, (unsigned char)(alpha * 0.5f)});
    }
    
    EndMode2D();
}

void Game::DrawHud(float dt){
    if(state.score>state.highScore) state.highScore=state.score;
    if(state.currentScreen==GameState::Screen::GAMEOVER) return; 
    DrawText(TextFormat("Score: %d  Best: %d",state.score,state.highScore),10,Const::HUD_TOP_MARGIN,Const::HUD_SCORE_FONT,RAYWHITE);
    constexpr int MIN_COMBO=Const::COMBO_MIN_MULT; unsigned char a=(state.comboTimer>0)?255:70; Color col=(state.comboCount>=MIN_COMBO && state.comboTimer>0)?Color{255,200,100,a}:Color{160,160,160,a}; DrawText(TextFormat("Combo x%d",state.comboCount),10,40,Const::HUD_COMBO_FONT,col);
    
    float radius=Const::HUD_CLOCK_RADIUS; float clockX=cfg.gameWidth - radius - 20.f; float clockY=radius + 20.f; DrawCircleLines((int)clockX,(int)clockY,radius,RAYWHITE); int segmentsDone=state.speedStage; for(int s=0;s<segmentsDone && s<5;++s){ float a0=-PI/2+(2*PI/5)*s; float a1=-PI/2+(2*PI/5)*(s+1); Vector2 p0{clockX+std::cos(a0)*radius*0.9f,clockY+std::sin(a0)*radius*0.9f}; Vector2 p1{clockX+std::cos(a1)*radius*0.9f,clockY+std::sin(a1)*radius*0.9f}; DrawLineEx({clockX,clockY},p0,2.f,{180,180,255,200}); DrawLineEx({clockX,clockY},p1,2.f,{180,180,255,200}); }
    float phase=(state.speedStage<5)?(state.stageTimer/cfg.STAGE_DURATION):0.f; float angle=(state.speedStage<5)?(-PI/2+phase*2*PI):(-PI/2-GetTime()*5.f); Vector2 hand{clockX+std::cos(angle)*radius*0.85f,clockY+std::sin(angle)*radius*0.85f}; DrawLineEx({clockX,clockY},hand,3.f,(state.speedStage<5)?Color{255,220,120,255}:Color{255,80,80,255}); DrawCircle((int)clockX,(int)clockY,3,(state.speedStage<5)?RAYWHITE:Color{255,80,80,255});
    
    int coinY = (int)(clockY + radius + 15);
    DrawCircle(cfg.gameWidth - 45, coinY, 8, GOLD);
    DrawText(TextFormat("%d", state.globalCoins), cfg.gameWidth - 30, coinY - 10, 20, GOLD);
    
    if(settings.showFPS){ DrawText(TextFormat("FPS: %d", GetFPS()), cfg.gameWidth - 70, coinY + 15, 16, {255,255,255,180}); }
    
    if(state.isDailyRun) {
        const char* challengeName = GetChallengeName(state.dailyChallenge.type);
        int cw = MeasureText(challengeName, 14);
        
        int y = coinY + (settings.showFPS ? 35 : 15);
        int x = cfg.gameWidth - cw - 12;
        
        DrawRectangle(x - 8, y, cw + 16, 22, Color{60, 40, 100, 200});
        DrawText(challengeName, x, y + 4, 14, Color{255, 180, 80, 255});
    }
    
    int powerUpY = 70;
    if(state.activeDoubleJump && state.powerUpTimers[0] > 0) {
        float pct = state.powerUpTimers[0] / 10.0f;
        DrawRectangle(10, powerUpY, (int)(100 * pct), 12, Color{100, 200, 255, 200});
        DrawText("2x Jump", 15, powerUpY - 2, 14, WHITE);
        powerUpY += 18;
    }
    if(state.activeShield && state.powerUpTimers[1] > 0) {
        float pct = state.powerUpTimers[1] / 10.0f;
        DrawRectangle(10, powerUpY, (int)(100 * pct), 12, Color{100, 255, 150, 200});
        DrawText("Shield", 15, powerUpY - 2, 14, WHITE);
        powerUpY += 18;
    }
    if(state.activeSlowMotion && state.powerUpTimers[2] > 0) {
        float pct = state.powerUpTimers[2] / 10.0f;
        DrawRectangle(10, powerUpY, (int)(100 * pct), 12, Color{200, 150, 255, 200});
        DrawText("Slow", 15, powerUpY - 2, 14, WHITE);
        powerUpY += 18;
    }
    if(state.activeMagnet && state.powerUpTimers[3] > 0) {
        float pct = state.powerUpTimers[3] / 10.0f;
        DrawRectangle(10, powerUpY, (int)(100 * pct), 12, Color{255, 200, 100, 200});
        DrawText("Magnet", 15, powerUpY - 2, 14, WHITE);
    }
    if(state.themeChangeTimer>0){ state.themeChangeTimer-=dt; float alpha=state.themeChangeTimer/3.f; if(alpha<0) alpha=0; if(alpha>1) alpha=1; int a2=(int)(alpha*255); const char* name=state.currentTheme.name; int w=MeasureText(name,Const::HUD_THEME_FONT); DrawText(name,cfg.gameWidth/2-w/2,80,Const::HUD_THEME_FONT,{255,255,255,(unsigned char)a2}); }

    if(state.achievementPopupTimer > 0 && !state.lastUnlockedAchievement.empty()) {
        float t = state.achievementPopupTimer / 3.f;
        float slide = 1.f;
        if(t > 0.85f) slide = (1.f - t) / 0.15f;
        else if(t < 0.2f) slide = t / 0.2f;
        if(slide > 1.f) slide = 1.f;
        if(slide < 0.f) slide = 0.f;

        int popupW = 200, popupH = 36;
        int popupX = 8;
        int popupTargetY = cfg.gameHeight - popupH - 10;
        int popupY = (int)(popupTargetY + (1.f - slide) * 50.f);
        unsigned char popupAlpha = (unsigned char)(slide * 220);

        DrawRectangle(popupX, popupY, popupW, popupH, {20, 25, 50, popupAlpha});
        DrawRectangleLines(popupX, popupY, popupW, popupH, {255, 200, 80, popupAlpha});

        int labelFont = 11;
        DrawText("*", popupX + 6, popupY + 4, labelFont, {255, 200, 80, popupAlpha});

        int nameFont = 14;
        DrawText(state.lastUnlockedAchievement.c_str(), popupX + 18, popupY + 10, nameFont, {255, 255, 255, popupAlpha});
    }
}

void Game::DrawGameOverOverlay(){
    if(state.currentScreen!=GameState::Screen::GAMEOVER) return;
    for(int y=0;y<cfg.gameHeight;y+=Const::GRADIENT_STEP){ float k=(float)y/cfg.gameHeight; unsigned char a=(unsigned char)(160+60*k); DrawRectangle(0,y,cfg.gameWidth,Const::GRADIENT_STEP,{10,12,20,a}); }
    
    int buttons = 3;
    
    int w=Const::GAMEOVER_PANEL_WIDTH; int yTop=Const::GAMEOVER_TOP; int bh=Const::GAMEOVER_BUTTON_H,spacing=Const::GAMEOVER_BUTTON_GAP; int yButtonsTop=yTop+125; int h=(yButtonsTop - yTop)+buttons*bh+(buttons-1)*spacing+20; int x=cfg.gameWidth/2 - w/2; DrawRectangle(x,yTop,w,h,{25,28,42,240}); DrawRectangleLines(x,yTop,w,h,{180,200,255,180}); 
    const char* title=Loc::GameOver_Title(); int tw=MeasureText(title,Const::GAMEOVER_TITLE_FONT); DrawText(title,cfg.gameWidth/2 - tw/2,yTop+15,Const::GAMEOVER_TITLE_FONT,RAYWHITE); 
    
    if(state.isDailyRun) {
        const char* challengeName = GetChallengeName(state.dailyChallenge.type);
        int cnw = MeasureText(challengeName, 12);
        DrawText(challengeName, cfg.gameWidth/2 - cnw/2, yTop+42, 12, Color{255, 180, 80, 255});
    }
    
    const char* scoreTxt=TextFormat("%s %d",Loc::GameOver_Score(),state.score); int sw=MeasureText(scoreTxt,Const::GAMEOVER_SCORE_FONT); DrawText(scoreTxt,cfg.gameWidth/2 - sw/2,yTop+60,Const::GAMEOVER_SCORE_FONT,{255,220,140,255}); 
    
    int bestScore = state.isDailyRun ? state.dailyChallenge.bestScore : state.highScore;
    const char* bestLabel = state.isDailyRun ? Loc::Daily_Best() : Loc::GameOver_Best();
    const char* bestTxt=TextFormat("%s %d", bestLabel, bestScore); int bw=MeasureText(bestTxt,Const::GAMEOVER_BEST_FONT); DrawText(bestTxt,cfg.gameWidth/2 - bw/2,yTop+85,Const::GAMEOVER_BEST_FONT,{200,230,255,255}); 
    
    // Show global coins
    const char* coinsTxt = TextFormat("%s %d", Loc::GameOver_Coins(), state.globalCoins);
    int cw = MeasureText(coinsTxt, 16);
    DrawText(coinsTxt, cfg.gameWidth/2 - cw/2, yTop+108, 16, Color{255, 215, 0, 255});
    
    int yb=yButtonsTop; Vector2 m=MapWindowToLogical(GetMousePosition()); bool click=IsMouseButtonPressed(MOUSE_LEFT_BUTTON); 
    
    auto btn=[&](const char* label, Color baseColor = Color{60,90,140,255}, Color hoverColor = Color{90,140,220,255}){ 
        int bw2=260,bh2=Const::GAMEOVER_BUTTON_H; int bx=cfg.gameWidth/2 - bw2/2; 
        Rectangle rc{(float)bx,(float)yb,(float)bw2,(float)bh2}; 
        Color c=CheckCollisionPointRec(m,rc)?hoverColor:baseColor; 
        DrawRectangleRec(rc,c); DrawRectangleLines(bx,yb,bw2,bh2,RAYWHITE); 
        int ltw=MeasureText(label,20); DrawText(label,bx + bw2/2 - ltw/2,yb+12,20,RAYWHITE); 
        yb+=bh2+Const::GAMEOVER_BUTTON_GAP; return rc; 
    };
    
    Rectangle rRestart=btn(Loc::GameOver_Restart()); if(click && CheckCollisionPointRec(m,rRestart)){ ResetGame(); ChangeScreen(GameState::Screen::GAME,false); } 
    Rectangle rMenu=btn(Loc::GameOver_Menu()); if(click && CheckCollisionPointRec(m,rMenu)){ ChangeScreen(GameState::Screen::MENU,false); } 
    Rectangle rExit=btn(Loc::GameOver_Exit()); if(click && CheckCollisionPointRec(m,rExit)){ running=false; }
}

void Game::DrawRevivePrompt(){
    EnsureRenderTarget();
    BeginTextureMode(gameRT);
    ClearBackground(BLACK);
    
    DrawVerticalGradient(cfg.gameWidth, cfg.gameHeight, state.currentTheme.bgTop, state.currentTheme.bgBottom);
    BeginMode2D(state.camera);
    for(const auto& pf : state.platforms) {
        Color c = GetPlatformColor(pf, state.currentTheme.platMoving, state.currentTheme.platStatic);
        DrawRectangleRec(pf.rect, c);
    }
    EndMode2D();
    
    DrawRectangle(0, 0, cfg.gameWidth, cfg.gameHeight, Color{0, 0, 0, 180});
    
    float timerValue = state.reviveTimer > 0 ? state.reviveTimer : 0;
    int timerInt = (int)std::ceil(timerValue);
    
    int centerX = cfg.gameWidth / 2;
    int centerY = cfg.gameHeight / 2 - 60;
    float radius = 80.f;
    float progress = timerValue / state.REVIVE_TIME_LIMIT;
    
    DrawCircle(centerX, centerY, radius + 8, Color{40, 40, 50, 255});
    DrawCircle(centerX, centerY, radius, Color{20, 25, 35, 255});
    
    float startAngle = -90.f;
    float endAngle = startAngle + (360.f * progress);
    Color arcColor;
    if(timerInt <= 2) {
        arcColor = Color{220, 80, 80, 255}; 
    } else if(timerInt <= 3) {
        arcColor = Color{220, 180, 80, 255}; 
    } else {
        arcColor = Color{80, 180, 80, 255}; 
    }
    DrawCircleSector({(float)centerX, (float)centerY}, radius - 5, startAngle, endAngle, 36, arcColor);
    
    DrawCircle(centerX, centerY, radius - 15, Color{30, 35, 45, 255});
    
    const char* timerText = TextFormat("%d", timerInt);
    int timerFontSize = 60;
    int tw = MeasureText(timerText, timerFontSize);
    Color timerColor = timerInt <= 2 ? Color{255, 100, 100, 255} : Color{255, 255, 255, 255};
    DrawText(timerText, centerX - tw/2, centerY - timerFontSize/2 + 5, timerFontSize, timerColor);
    
    const char* reviveText = Loc::GameOver_Revive();
    int reviveFontSize = 28;
    int rw = MeasureText(reviveText, reviveFontSize);
    DrawText(reviveText, centerX - rw/2, centerY + radius + 30, reviveFontSize, WHITE);
    
    const char* costText = TextFormat("%d", state.reviveCost);
    int costFontSize = 20;
    int cw = MeasureText(costText, costFontSize);
    DrawText(costText, centerX - cw/2, centerY + radius + 65, costFontSize, Color{255, 215, 0, 255});
    
    DrawCircle(centerX + cw/2 + 15, centerY + radius + 75, 8, Color{255, 215, 0, 255});
    
    Vector2 m = MapWindowToLogical(GetMousePosition());
    bool click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    
    int btnW = 200, btnH = 50;
    int btnX = centerX - btnW/2;
    int btnY = centerY + radius + 100;
    Rectangle btnRect = {(float)btnX, (float)btnY, (float)btnW, (float)btnH};
    bool hover = CheckCollisionPointRec(m, btnRect);
    
    Color btnColor = hover ? Color{80, 200, 80, 255} : Color{60, 160, 60, 255};
    DrawRectangleRec(btnRect, btnColor);
    DrawRectangleLines(btnX, btnY, btnW, btnH, WHITE);
    
    const char* btnText = Loc::GameOver_Revive();
    int btw = MeasureText(btnText, 24);
    DrawText(btnText, btnX + btnW/2 - btw/2, btnY + 13, 24, WHITE);
    
    if(click && hover) {
        RevivePlayer();
    }
    
    int skipBtnW = 120, skipBtnH = 35;
    int skipBtnX = centerX - skipBtnW/2;
    int skipBtnY = btnY + btnH + 20;
    Rectangle skipRect = {(float)skipBtnX, (float)skipBtnY, (float)skipBtnW, (float)skipBtnH};
    bool skipHover = CheckCollisionPointRec(m, skipRect);
    
    Color skipColor = skipHover ? Color{100, 70, 70, 255} : Color{70, 50, 50, 255};
    DrawRectangleRec(skipRect, skipColor);
    DrawRectangleLines(skipBtnX, skipBtnY, skipBtnW, skipBtnH, Color{200, 200, 200, 200});
    
    const char* skipText = Loc::GameOver_Cancel();
    int stw = MeasureText(skipText, 18);
    DrawText(skipText, skipBtnX + skipBtnW/2 - stw/2, skipBtnY + 9, 18, Color{200, 200, 200, 255});
    
    if(click && skipHover) {
        ChangeScreen(GameState::Screen::GAMEOVER, false);
    }
    
    EndTextureMode();
    
    int winW = GetScreenWidth(), winH = GetScreenHeight();
    float scale = std::fmin((float)winW/cfg.gameWidth, (float)winH/cfg.gameHeight);
    int drawW = (int)(cfg.gameWidth * scale);
    int drawH = (int)(cfg.gameHeight * scale);
    int offX = (winW - drawW) / 2;
    int offY = (winH - drawH) / 2;
    viewportRect = {(float)offX, (float)offY, (float)drawW, (float)drawH};
    
    BeginDrawing();
    ClearBackground(BLACK);
    Rectangle src = {0, 0, (float)gameRT.texture.width, (float)-gameRT.texture.height};
    Rectangle dst = {(float)offX, (float)offY, (float)drawW, (float)drawH};
    DrawTexturePro(gameRT.texture, src, dst, {0, 0}, 0.f, WHITE);
    EndDrawing();
}

void Game::DrawGame(){
    PROF_SCOPE("DrawGame");
    float dt=GetFrameTime();
    if(gameRT.id==0 || gameRT.texture.width!=cfg.gameWidth || gameRT.texture.height!=cfg.gameHeight){ if(gameRT.id>0) UnloadRenderTexture(gameRT); gameRT = LoadRenderTexture(cfg.gameWidth,cfg.gameHeight); }
    BeginTextureMode(gameRT);
    ClearBackground(BLACK);
    DrawGameWorld(dt);
    DrawHud(dt);
    DrawGameOverOverlay();
    if(settings.powerUpEffects && state.shieldFlashAlpha > 0) {
        unsigned char a = (unsigned char)(state.shieldFlashAlpha * 200);
        DrawRectangle(0, 0, cfg.gameWidth, cfg.gameHeight, Color{255, 255, 255, a});
    }
    if(state.fadeAlpha>0.01f) DrawRectangle(0,0,cfg.gameWidth,cfg.gameHeight,{0,0,0,(unsigned char)(state.fadeAlpha*255)});
    EndTextureMode();
    int winW=GetScreenWidth(), winH=GetScreenHeight(); float scale=std::fmin((float)winW/cfg.gameWidth,(float)winH/cfg.gameHeight); int drawW=(int)(cfg.gameWidth*scale); int drawH=(int)(cfg.gameHeight*scale); int offX=(winW-drawW)/2; int offY=(winH-drawH)/2; viewportRect={(float)offX,(float)offY,(float)drawW,(float)drawH};
    BeginDrawing();
    DrawVerticalGradient(winW,winH,state.currentTheme.bgTop,state.currentTheme.bgBottom);
    if(gameRT.id>0){ SetTextureFilter(gameRT.texture,TEXTURE_FILTER_BILINEAR); float bgScale=std::fmax((float)winW/cfg.gameWidth,(float)winH/cfg.gameHeight)*1.15f; float bgW=cfg.gameWidth*bgScale; float bgH=cfg.gameHeight*bgScale; float bgX=(winW-bgW)/2.f; float bgY=(winH-bgH)/2.f; Rectangle bgSrc{0,0,(float)gameRT.texture.width,(float)-gameRT.texture.height}; Rectangle bgDst{bgX,bgY,bgW,bgH}; DrawTexturePro(gameRT.texture,bgSrc,bgDst,{0,0},0.f,Color{255,255,255,60}); DrawTexturePro(gameRT.texture,bgSrc,bgDst,{0,0},0.f,Color{200,200,255,40}); DrawRectangle(0,0,winW,winH,Color{0,0,20,90}); }
    Rectangle src{0,0,(float)gameRT.texture.width,(float)-gameRT.texture.height}; Rectangle dst{(float)offX,(float)offY,(float)drawW,(float)drawH}; DrawTexturePro(gameRT.texture,src,dst,{0,0},0.f,WHITE);
    if(state.fadeAlpha>0.01f) DrawRectangle(0,0,winW,winH,{0,0,0,(unsigned char)(state.fadeAlpha*255)});
    EndDrawing();
}
