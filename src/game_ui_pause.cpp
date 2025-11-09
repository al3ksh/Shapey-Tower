// Pause screen drawing
#include "game.h"
#include "raylib.h"
#include <cmath>

void Game::DrawPause(){
    BeginDrawing();
    ClearBackground(Color{10,14,20,255});
    int sw=GetScreenWidth();
    int sh=GetScreenHeight();
    viewportRect = {0,0,(float)sw,(float)sh};
    float uiCenterX = (viewportRect.width>0)? (viewportRect.x + viewportRect.width/2.f) : sw/2.f;
    int topSafe = (viewportRect.height>0)? (int)viewportRect.y + 40 : 40;
    static float scroll=0.f; 
    float wheel = GetMouseWheelMove();
    // Dynamic content area start (below title)
    int yContentStart = topSafe + 80;
    Rectangle contentViewport{0,(float)yContentStart,(float)sw,(float)(sh - yContentStart - 10)};
    // Virtual Y with scroll applied
    int y = (int)(yContentStart - scroll);
    Vector2 rawMouse=GetMousePosition();
    bool mouseInContent = rawMouse.y >= contentViewport.y && rawMouse.y <= contentViewport.y+contentViewport.height;
    if(!mouseInContent) wheel=0;
    Vector2 mPos=rawMouse; bool click=IsMouseButtonPressed(MOUSE_LEFT_BUTTON); bool pressed=false;
    const char* title = "PAUZA"; int tw=MeasureText(title,36); DrawText(title,(int)(uiCenterX - tw/2), topSafe,36,RAYWHITE);
    GuiButtonCentered(uiCenterX, y, 300,40, "Wznow", mPos, pressed); if(pressed){ state.paused=false; ChangeScreen(GameState::Screen::GAME); }
    y -= UiLayout::CompactPullUp;
    GuiButtonCentered(uiCenterX, y, 300,40, "Restart", mPos, pressed); if(pressed){ ResetGame(); ChangeScreen(GameState::Screen::GAME,false); }
    y -= UiLayout::CompactPullUp;
    GuiButtonCentered(uiCenterX, y, 300,40, "Menu", mPos, pressed); if(pressed){ ChangeScreen(GameState::Screen::MENU); }
    y -= UiLayout::CompactPullUp;
    GuiButtonCentered(uiCenterX, y, 300,40, "Wyjscie", mPos, pressed); if(pressed){ running=false; }
    y -= UiLayout::CompactPullUp;
    // Video section (quick resolution/fullscreen toggles)
    DrawText("Video:", (int)(uiCenterX-150), y, 18,RAYWHITE); y+=28;
    DrawResolutionSelector(y, uiCenterX, mPos, click, sw);
    int fsW=300; int fsH=30; int fsX=(int)(uiCenterX - fsW/2); if(fsX<10) fsX=10; if(fsX+fsW>sw-10) fsX=sw-10-fsW; Rectangle fsRect{(float)fsX,(float)y,(float)fsW,(float)fsH}; Color fsCol = CheckCollisionPointRec(mPos,fsRect)?Color{70,110,160,255}:Color{55,80,120,255}; DrawRectangleRec(fsRect, fsCol); DrawRectangleLines(fsX,y,fsW,fsH,RAYWHITE); const char* fsTxt = fullscreen?"Fullscreen: ON":"Fullscreen: OFF"; int fsTW=MeasureText(fsTxt,18); DrawText(fsTxt, fsX + fsW/2 - fsTW/2, y+6,18,RAYWHITE); if(click && CheckCollisionPointRec(mPos,fsRect)){ fullscreen=!fullscreen; ApplyResolution(false); }
    y += fsH + 22;
    DrawText("Sterowanie:", (int)(uiCenterX-150), y, 18,RAYWHITE); y+=UiLayout::SectionLabelGap;
    enum RebindTargetPause{P_NONE,P_LEFT,P_RIGHT,P_JUMP}; static RebindTargetPause rebindPause=P_NONE; static float rebindBlinkTime=0.f; rebindBlinkTime += GetFrameTime(); float blinkAlpha = (std::sin(rebindBlinkTime*6.f)*0.5f+0.5f);
    Rectangle lastHover{0,0,0,0}; const char* lastDefault=nullptr; bool clicked=false; bool activeChanged=false;
    DrawRebindKey<RebindTargetPause>(y, uiCenterX, mPos, sw, "Lewo", state.keys.left, rebindPause, P_LEFT, clicked, blinkAlpha, lastHover, lastDefault, "Domyslnie: A", activeChanged); if(clicked){ rebindPause=(rebindPause==P_LEFT?P_NONE:P_LEFT); }
    DrawRebindKey<RebindTargetPause>(y, uiCenterX, mPos, sw, "Prawo", state.keys.right, rebindPause, P_RIGHT, clicked, blinkAlpha, lastHover, lastDefault, "Domyslnie: D", activeChanged); if(clicked){ rebindPause=(rebindPause==P_RIGHT?P_NONE:P_RIGHT); }
    DrawRebindKey<RebindTargetPause>(y, uiCenterX, mPos, sw, "Skok", state.keys.jump, rebindPause, P_JUMP, clicked, blinkAlpha, lastHover, lastDefault, "Domyslnie: SPACE", activeChanged); if(clicked){ rebindPause=(rebindPause==P_JUMP?P_NONE:P_JUMP); }
    if(rebindPause!=P_NONE){ for(int k=32;k<350;k++){ if(IsKeyPressed(k)){ if(rebindPause==P_LEFT) state.keys.left=k; else if(rebindPause==P_RIGHT) state.keys.right=k; else if(rebindPause==P_JUMP) state.keys.jump=k; rebindPause=P_NONE; settingsDirty=true; settingsSaveTimer=0.f; break; } } }
    if(lastDefault){ int ttPad=6; int txtW=MeasureText(lastDefault,14); int boxW=txtW+ttPad*2; int boxH=22; int bx=(int)(lastHover.x + lastHover.width + 8); int by=(int)lastHover.y; if(bx+boxW>sw-10) bx = (int)lastHover.x + (int)lastHover.width - boxW; if(by+boxH>sh-10) by=sh-10-boxH; DrawRectangle(bx,by,boxW,boxH,{30,30,45,230}); DrawRectangleLines(bx,by,boxW,boxH,{180,180,220,200}); DrawText(lastDefault, bx+ttPad, by+4,14,RAYWHITE); }
    DrawText("Glosnosci:", (int)(uiCenterX-150), y, 18,RAYWHITE); y+=UiLayout::SectionLabelGap;
    bool changed=false; DrawAudioSliders(y, uiCenterX, mPos, sw, changed); if(changed){ settingsDirty=true; settingsSaveTimer=0.f; ApplyAudioVolumes(); }
    // Reset settings button
    int resetGap = 28; y += resetGap;
    bool resetPressed=false; GuiButtonCentered(uiCenterX, y, 260,34, "Reset ustawien", mPos, resetPressed); if(resetPressed){ ResetSettingsToDefaults(); }
    // Compute total content height then clamp scroll
    int contentHeight = y - (yContentStart - scroll);
    int maxScroll = contentHeight - (int)contentViewport.height;
    if(maxScroll < 0) maxScroll = 0;
    // Scroll only if content overflow
    if(maxScroll>0 && std::fabs(wheel)>0.01f && mouseInContent){ scroll -= wheel*60.f; if(scroll<0) scroll=0; if(scroll>maxScroll) scroll=(float)maxScroll; }
    else if(maxScroll==0) scroll=0; 
    // Scroll bar
    if(maxScroll > 0){
        float barW = 6.f; float trackX = sw - 10.f - barW; float trackY = contentViewport.y; float trackH = contentViewport.height; DrawRectangle(trackX, trackY, barW, trackH, Color{30,40,55,160});
        float ratio = contentViewport.height / (float)contentHeight; if(ratio<0.08f) ratio=0.08f; float thumbH = trackH * ratio; float scrollRatio = scroll / (float)maxScroll; float thumbY = trackY + (trackH - thumbH)*scrollRatio; DrawRectangle(trackX, thumbY, barW, thumbH, Color{120,170,230,220});
    }
    EndDrawing();
}
