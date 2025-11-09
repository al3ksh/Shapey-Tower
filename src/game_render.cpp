#include "game.h"
#include "raylib.h"

// --- Breakdown into smaller steps of rendering ---
#include "debug.h"
#include "constants.h"
static void DrawVerticalGradient(int w,int h, Color top, Color bottom){
    for(int y=0;y<h;y+=Const::GRADIENT_STEP){ float k=(float)y/h; unsigned char r=(unsigned char)(top.r+(bottom.r-top.r)*k); unsigned char g=(unsigned char)(top.g+(bottom.g-top.g)*k); unsigned char b=(unsigned char)(top.b+(bottom.b-top.b)*k); DrawRectangle(0,y,w,Const::GRADIENT_STEP,{r,g,b,255}); }
}

void Game::DrawGameWorld(float dt){
    DrawVerticalGradient(cfg.screenWidth,cfg.screenHeight, state.currentTheme.bgTop, state.currentTheme.bgBottom);
    // update particles
    Particles::Update(state.particles, dt, cfg.GRAVITY, 0.2f);
    BeginMode2D(state.camera);
    for(auto &pf:state.platforms){ Color c=pf.moving?state.currentTheme.platMoving:state.currentTheme.platStatic; DrawRectangleRec(pf.rect,c); DrawRectangle((int)pf.rect.x,(int)pf.rect.y,(int)pf.rect.width,3,{255,255,255,60}); DrawRectangleLines((int)pf.rect.x,(int)pf.rect.y,(int)pf.rect.width,(int)pf.rect.height,{255,255,255,30}); }
    Particles::Draw(state.particles);
    // player
    if(state.playerTexture.id>0){
        Rectangle src{0,0,(float)state.playerTexture.width,(float)state.playerTexture.height};
        float baseScale=state.playerSpriteScale; float dstW=state.player.width*baseScale; float dstH=state.player.height*baseScale; float padRatio=(state.playerTexture.height>0)?(state.playerSpriteBottomPad/(float)state.playerTexture.height):0.f;
        float vx=state.player.vel.x; float lean=vx/cfg.MAX_HSPEED; if(lean>1) lean=1; if(lean<-1) lean=-1; float leanDeg=lean*8.f; float finalW=dstW, finalH=dstH;
    if(state.landingSquashActive){ state.landingSquashTime+=dt; float t=state.landingSquashTime/state.landingSquashDuration; if(t>1){ t=1; state.landingSquashActive=false; } float e=1.f-std::pow(1.f-t,3.f); float squashAmt=0.55f; finalW*=1.f+squashAmt*(1.f-e); finalH*=1.f - squashAmt*0.65f*(1.f-e); }
        float dstX=state.player.pos.x + (state.player.width-finalW)/2.f; float baseY=state.player.pos.y - (dstH - state.player.height) + state.playerSpriteYOffset + padRatio*dstH; float dstY=baseY - (finalH - dstH); Rectangle dst{dstX,dstY,finalW,finalH}; if(vx<-10.f) src.width=-src.width;
    constexpr int MIN_COMBO=Const::COMBO_MIN_MULT; bool comboActive=(state.comboCount>=MIN_COMBO && state.comboTimer>0 && state.shaderFire.id>0);
    if(comboActive){ float intensity=std::fmin(1.f,(float)(state.comboCount-1)/6.f); float pulse=(std::sin(state.animTime*5.f)+1.f)*0.5f; float finalIntensity=(0.5f+0.5f*pulse)*intensity; Vector2 sprSize{(float)state.playerTexture.width,(float)state.playerTexture.height}; BeginBlendMode(BLEND_ADDITIVE); BeginShaderMode(state.shaderFire); if(state.fireLocTime>=0) SetShaderValue(state.shaderFire,state.fireLocTime,&state.animTime,SHADER_UNIFORM_FLOAT); if(state.fireLocIntensity>=0) SetShaderValue(state.shaderFire,state.fireLocIntensity,&finalIntensity,SHADER_UNIFORM_FLOAT); if(state.fireLocSpriteSize>=0) SetShaderValue(state.shaderFire,state.fireLocSpriteSize,&sprSize,SHADER_UNIFORM_VEC2); if(state.fireLocMode>=0){ int mode=1; SetShaderValue(state.shaderFire,state.fireLocMode,&mode,SHADER_UNIFORM_INT);} Rectangle aura=dst; aura.x-=aura.width*Const::AURA_OFFSET_X; aura.y-=aura.height*Const::AURA_OFFSET_Y; aura.width*=Const::AURA_GROW_W; aura.height*=Const::AURA_GROW_H; DrawTexturePro(state.playerTexture,src,aura,{0,0},leanDeg,WHITE); EndShaderMode(); EndBlendMode(); BeginShaderMode(state.shaderFire); if(state.fireLocTime>=0) SetShaderValue(state.shaderFire,state.fireLocTime,&state.animTime,SHADER_UNIFORM_FLOAT); if(state.fireLocIntensity>=0) SetShaderValue(state.shaderFire,state.fireLocIntensity,&finalIntensity,SHADER_UNIFORM_FLOAT); if(state.fireLocSpriteSize>=0) SetShaderValue(state.shaderFire,state.fireLocSpriteSize,&sprSize,SHADER_UNIFORM_VEC2); if(state.fireLocMode>=0){ int mode=0; SetShaderValue(state.shaderFire,state.fireLocMode,&mode,SHADER_UNIFORM_INT);} DrawTexturePro(state.playerTexture,src,dst,{0,0},leanDeg,WHITE); EndShaderMode(); }
        else DrawTexturePro(state.playerTexture,src,dst,{0,0},leanDeg,WHITE);
    } else {
        DrawRectangle((int)state.player.pos.x,(int)state.player.pos.y,(int)state.player.width,(int)state.player.height,state.currentTheme.playerBody);
        DrawRectangleLines((int)state.player.pos.x,(int)state.player.pos.y,(int)state.player.width,(int)state.player.height,{40,40,40,255});
    }
    EndMode2D();
}

void Game::DrawHud(float dt){
    if(state.score>state.highScore) state.highScore=state.score;
    if(state.currentScreen==GameState::Screen::GAMEOVER) return; // HUD tylko podczas gry
    DrawText(TextFormat("Score: %d  Best: %d",state.score,state.highScore),10,Const::HUD_TOP_MARGIN,Const::HUD_SCORE_FONT,RAYWHITE);
    constexpr int MIN_COMBO=Const::COMBO_MIN_MULT; unsigned char a=(state.comboTimer>0)?255:70; Color col=(state.comboCount>=MIN_COMBO && state.comboTimer>0)?Color{255,200,100,a}:Color{160,160,160,a}; DrawText(TextFormat("Combo x%d",state.comboCount),10,40,Const::HUD_COMBO_FONT,col);
    if(state.themeChangeTimer>0){ state.themeChangeTimer-=dt; float alpha=state.themeChangeTimer/3.f; if(alpha<0) alpha=0; if(alpha>1) alpha=1; int a2=(int)(alpha*255); const char* name=state.currentTheme.name; int w=MeasureText(name,Const::HUD_THEME_FONT); DrawText(name,cfg.screenWidth/2-w/2,80,Const::HUD_THEME_FONT,{255,255,255,(unsigned char)a2}); }
    if(settings.showFPS){ DrawText(TextFormat("FPS: %d", GetFPS()), cfg.screenWidth-Const::HUD_FPS_OFFSET_X, Const::HUD_TOP_MARGIN, 18, {255,255,255,220}); }
    DrawText(TextFormat("Platforms: %d Theme:%s(%d)",state.generatedPlatformsCount,state.currentTheme.name,state.currentThemeIndex),10,70,14,{180,200,230,200});
    float radius=Const::HUD_CLOCK_RADIUS; float clockX=cfg.screenWidth - radius - 20.f; float clockY=radius + 20.f; DrawCircleLines((int)clockX,(int)clockY,radius,RAYWHITE); int segmentsDone=state.speedStage; for(int s=0;s<segmentsDone && s<5;++s){ float a0=-PI/2+(2*PI/5)*s; float a1=-PI/2+(2*PI/5)*(s+1); Vector2 p0{clockX+std::cos(a0)*radius*0.9f,clockY+std::sin(a0)*radius*0.9f}; Vector2 p1{clockX+std::cos(a1)*radius*0.9f,clockY+std::sin(a1)*radius*0.9f}; DrawLineEx({clockX,clockY},p0,2.f,{180,180,255,200}); DrawLineEx({clockX,clockY},p1,2.f,{180,180,255,200}); }
    float phase=(state.speedStage<5)?(state.stageTimer/cfg.STAGE_DURATION):0.f; float angle=(state.speedStage<5)?(-PI/2+phase*2*PI):(-PI/2-GetTime()*5.f); Vector2 hand{clockX+std::cos(angle)*radius*0.85f,clockY+std::sin(angle)*radius*0.85f}; DrawLineEx({clockX,clockY},hand,3.f,(state.speedStage<5)?Color{255,220,120,255}:Color{255,80,80,255}); DrawCircle((int)clockX,(int)clockY,3,(state.speedStage<5)?RAYWHITE:Color{255,80,80,255}); if(state.scrollActive) DrawText(TextFormat("Speed x%.2f stage %d",state.scrollSpeed/60.f,state.speedStage),(int)(clockX-radius-140),(int)(clockY+radius+4),14,{200,180,255,200});
}

void Game::DrawGameOverOverlay(){
    if(state.currentScreen!=GameState::Screen::GAMEOVER) return;
    for(int y=0;y<cfg.screenHeight;y+=Const::GRADIENT_STEP){ float k=(float)y/cfg.screenHeight; unsigned char a=(unsigned char)(160+60*k); DrawRectangle(0,y,cfg.screenWidth,Const::GRADIENT_STEP,{10,12,20,a}); }
    int w=Const::GAMEOVER_PANEL_WIDTH; int yTop=Const::GAMEOVER_TOP; int buttons=3,bh=Const::GAMEOVER_BUTTON_H,spacing=Const::GAMEOVER_BUTTON_GAP; int yButtonsTop=yTop+125; int h=(yButtonsTop - yTop)+buttons*bh+(buttons-1)*spacing+20; int x=cfg.screenWidth/2 - w/2; DrawRectangle(x,yTop,w,h,{25,28,42,240}); DrawRectangleLines(x,yTop,w,h,{180,200,255,180}); const char* title="GAME OVER"; int tw=MeasureText(title,Const::GAMEOVER_TITLE_FONT); DrawText(title,cfg.screenWidth/2 - tw/2,yTop+15,Const::GAMEOVER_TITLE_FONT,RAYWHITE); const char* scoreTxt=TextFormat("Score: %d",state.score); int sw=MeasureText(scoreTxt,Const::GAMEOVER_SCORE_FONT); DrawText(scoreTxt,cfg.screenWidth/2 - sw/2,yTop+60,Const::GAMEOVER_SCORE_FONT,{255,220,140,255}); const char* bestTxt=TextFormat("Best: %d",state.highScore); int bw=MeasureText(bestTxt,Const::GAMEOVER_BEST_FONT); DrawText(bestTxt,cfg.screenWidth/2 - bw/2,yTop+92,Const::GAMEOVER_BEST_FONT,{200,230,255,255}); int yb=yButtonsTop; Vector2 m=GetMousePosition(); bool click=IsMouseButtonPressed(MOUSE_LEFT_BUTTON); auto btn=[&](const char* label){ int bw2=260,bh2=Const::GAMEOVER_BUTTON_H; int bx=cfg.screenWidth/2 - bw2/2; Rectangle rc{(float)bx,(float)yb,(float)bw2,(float)bh2}; Color c=CheckCollisionPointRec(m,rc)?Color{90,140,220,255}:Color{60,90,140,255}; DrawRectangleRec(rc,c); DrawRectangleLines(bx,yb,bw2,bh2,RAYWHITE); int ltw=MeasureText(label,20); DrawText(label,bx + bw2/2 - ltw/2,yb+12,20,RAYWHITE); yb+=bh2+Const::GAMEOVER_BUTTON_GAP; return rc; }; Rectangle rRestart=btn("Restart"); if(click && CheckCollisionPointRec(m,rRestart)){ ResetGame(); ChangeScreen(GameState::Screen::GAME,false); } Rectangle rMenu=btn("Menu"); if(click && CheckCollisionPointRec(m,rMenu)){ ChangeScreen(GameState::Screen::MENU,false); } Rectangle rExit=btn("Wyjscie"); if(click && CheckCollisionPointRec(m,rExit)){ running=false; }
}

void Game::DrawGame(){
    PROF_SCOPE("DrawGame");
    float dt=GetFrameTime();
    if(gameRT.id==0 || gameRT.texture.width!=cfg.screenWidth || gameRT.texture.height!=cfg.screenHeight){ if(gameRT.id>0) UnloadRenderTexture(gameRT); gameRT = LoadRenderTexture(cfg.screenWidth,cfg.screenHeight); }
    BeginTextureMode(gameRT);
    ClearBackground(BLACK);
    DrawGameWorld(dt);
    DrawHud(dt);
    DrawGameOverOverlay();
    if(state.fadeAlpha>0.01f) DrawRectangle(0,0,cfg.screenWidth,cfg.screenHeight,{0,0,0,(unsigned char)(state.fadeAlpha*255)});
    EndTextureMode();
    int winW=GetScreenWidth(), winH=GetScreenHeight(); float scale=std::fmin((float)winW/cfg.screenWidth,(float)winH/cfg.screenHeight); int drawW=(int)(cfg.screenWidth*scale); int drawH=(int)(cfg.screenHeight*scale); int offX=(winW-drawW)/2; int offY=(winH-drawH)/2; viewportRect={(float)offX,(float)offY,(float)drawW,(float)drawH};
    BeginDrawing();
    DrawVerticalGradient(winW,winH,state.currentTheme.bgTop,state.currentTheme.bgBottom);
    if(gameRT.id>0){ SetTextureFilter(gameRT.texture,TEXTURE_FILTER_BILINEAR); float bgScale=std::fmax((float)winW/cfg.screenWidth,(float)winH/cfg.screenHeight)*1.15f; float bgW=cfg.screenWidth*bgScale; float bgH=cfg.screenHeight*bgScale; float bgX=(winW-bgW)/2.f; float bgY=(winH-bgH)/2.f; Rectangle bgSrc{0,0,(float)gameRT.texture.width,(float)-gameRT.texture.height}; Rectangle bgDst{bgX,bgY,bgW,bgH}; DrawTexturePro(gameRT.texture,bgSrc,bgDst,{0,0},0.f,Color{255,255,255,60}); DrawTexturePro(gameRT.texture,bgSrc,bgDst,{0,0},0.f,Color{200,200,255,40}); DrawRectangle(0,0,winW,winH,Color{0,0,20,90}); }
    Rectangle src{0,0,(float)gameRT.texture.width,(float)-gameRT.texture.height}; Rectangle dst{(float)offX,(float)offY,(float)drawW,(float)drawH}; DrawTexturePro(gameRT.texture,src,dst,{0,0},0.f,WHITE);
    if(state.fadeAlpha>0.01f) DrawRectangle(0,0,winW,winH,{0,0,0,(unsigned char)(state.fadeAlpha*255)});
    EndDrawing();
}
