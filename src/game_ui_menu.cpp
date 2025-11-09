#include "game.h"
#include "debug.h"
#include <cmath>

// Draw main menu UI
void Game::DrawMenu(){
    BeginDrawing();
    ClearBackground(Color{10,14,20,255});
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    viewportRect = {0,0,(float)sw,(float)sh};
    float uiCenterX = (viewportRect.width>0)? (viewportRect.x + viewportRect.width/2.f) : sw/2.f;
    int topSafe = (viewportRect.height>0)? (int)viewportRect.y + 40 : 40;
    const char* title="SHAPEY TOWER"; int tw=MeasureText(title,36);
    DrawText(title, (int)(uiCenterX - tw/2), topSafe, 36, RAYWHITE);
    int y = topSafe + 90;
    Vector2 mPos=GetMousePosition();
    bool click=IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    auto clampX=[&](int desired,int w){ int x=desired; if(x<10) x=10; if(x+w>sw-10) x=sw-10-w; return x; };
    bool pressed=false;
    GuiButtonCentered(uiCenterX, y, 300,40, "Start", mPos, pressed); if(pressed){ ResetGame(); state.started=true; state.paused=false; ChangeScreen(GameState::Screen::GAME); }
    y -= UiLayout::CompactPullUp;
    GuiButtonCentered(uiCenterX, y, 300,40, "Wyjscie", mPos, pressed); if(pressed){ running=false; }
    y -= UiLayout::CompactPullUp;
    DrawText("Video:", clampX((int)(uiCenterX - 150),300), y, 18, RAYWHITE); y+=28;
    DrawResolutionSelector(y, uiCenterX, mPos, click, sw);
    int fsW=300; int fsH=30; int fsX=clampX((int)(uiCenterX - fsW/2),fsW); Rectangle fsRect{(float)fsX,(float)y,(float)fsW,(float)fsH}; Color fsCol = CheckCollisionPointRec(mPos,fsRect)?Color{70,110,160,255}:Color{55,80,120,255}; DrawRectangleRec(fsRect, fsCol); DrawRectangleLines(fsX,y,fsW,fsH,RAYWHITE);
    const char* fsTxt = fullscreen?"Fullscreen: ON":"Fullscreen: OFF"; int fsTW=MeasureText(fsTxt,18); DrawText(fsTxt, fsX + fsW/2 - fsTW/2, y+6,18,RAYWHITE);
    if(click && CheckCollisionPointRec(mPos,fsRect)){ fullscreen=!fullscreen; ApplyResolution(false); }
    y += fsH + 18;
    DrawText("Sterowanie:", clampX((int)(uiCenterX - 150),300), y, 18, RAYWHITE); y+=UiLayout::SectionLabelGap;
    enum RebindTargetMenu{MN_NONE,MN_LEFT,MN_RIGHT,MN_JUMP}; static RebindTargetMenu rebindMenu=MN_NONE; static float rebindBlinkTime=0.f; rebindBlinkTime += GetFrameTime(); float blinkAlpha = (std::sin(rebindBlinkTime*6.f)*0.5f+0.5f);
    Rectangle lastRebindHovered{0,0,0,0}; const char* lastRebindDefault=nullptr; bool clicked=false; bool activeChanged=false;
    DrawRebindKey<RebindTargetMenu>(y, uiCenterX, mPos, sw, "Lewo", state.keys.left, rebindMenu, MN_LEFT, clicked, blinkAlpha, lastRebindHovered, lastRebindDefault, "Domyslnie: A", activeChanged); if(clicked){ rebindMenu = (rebindMenu==MN_LEFT?MN_NONE:MN_LEFT); }
    DrawRebindKey<RebindTargetMenu>(y, uiCenterX, mPos, sw, "Prawo", state.keys.right, rebindMenu, MN_RIGHT, clicked, blinkAlpha, lastRebindHovered, lastRebindDefault, "Domyslnie: D", activeChanged); if(clicked){ rebindMenu = (rebindMenu==MN_RIGHT?MN_NONE:MN_RIGHT); }
    DrawRebindKey<RebindTargetMenu>(y, uiCenterX, mPos, sw, "Skok", state.keys.jump, rebindMenu, MN_JUMP, clicked, blinkAlpha, lastRebindHovered, lastRebindDefault, "Domyslnie: SPACE", activeChanged); if(clicked){ rebindMenu = (rebindMenu==MN_JUMP?MN_NONE:MN_JUMP); }
    if(rebindMenu!=MN_NONE){ for(int k=32;k<350;k++){ if(IsKeyPressed(k)){ if(rebindMenu==MN_LEFT) state.keys.left=k; else if(rebindMenu==MN_RIGHT) state.keys.right=k; else if(rebindMenu==MN_JUMP) state.keys.jump=k; rebindMenu=MN_NONE; settingsDirty=true; settingsSaveTimer=0.f; break; } } }
    int volStartY = y+4; y = volStartY;
    DrawText("Glosnosci:", clampX((int)(uiCenterX - 150),300), y, 18, RAYWHITE); y+=UiLayout::SectionLabelGap;
    bool changed=false; DrawAudioSliders(y, uiCenterX, mPos, sw, changed); if(changed){ settingsDirty=true; settingsSaveTimer=0.f; ApplyAudioVolumes(); }
    int desiredBottomMargin = UiLayout::BottomMargin;
    int resetY = y + UiLayout::ResetGap;
    int currentBottom = sh - (resetY + 34);
    if(currentBottom < desiredBottomMargin){ int deficit = desiredBottomMargin - currentBottom; resetY -= deficit; if(resetY < y + 12) resetY = y + 12; if(resetY < 0) resetY = 0; }
    bool pressedReset=false; int tempY = resetY; GuiButtonCentered(uiCenterX, tempY, 260,34, "Reset ustawien", mPos, pressedReset); if(pressedReset){ ResetSettingsToDefaults(); }
    if(lastRebindDefault){ int ttPad=6; int txtW=MeasureText(lastRebindDefault,14); int boxW=txtW+ttPad*2; int boxH=22; int bx=(int)(lastRebindHovered.x + lastRebindHovered.width + 8); int by=(int)lastRebindHovered.y; if(bx+boxW>sw-10) bx = (int)lastRebindHovered.x + (int)lastRebindHovered.width - boxW; if(by+boxH>sh-10) by=sh-10-boxH; DrawRectangle(bx,by,boxW,boxH,{30,30,45,230}); DrawRectangleLines(bx,by,boxW,boxH,{180,180,220,200}); DrawText(lastRebindDefault, bx+ttPad, by+4,14,RAYWHITE); }
    state.audio.masterVolume = 0.0001f + state.audio.masterSlider * 1.0f;
    SetMasterVolume(state.audio.masterVolume);
    if(state.audio.musicBg.ctxData){ SetMusicVolume(state.audio.musicBg, state.audio.volMusic * VOL_MUSIC_MULT); }
    EndDrawing();
}
