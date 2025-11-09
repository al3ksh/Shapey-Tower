#include "raylib.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <string>

struct Platform { Rectangle rect; bool moving=false; float baseX=0.f; float moveAmplitude=0.f; float moveSpeed=0.f; };
struct Player { Vector2 pos; Vector2 vel; float width=32.f; float height=40.f; };
struct Theme { Color bgTop,bgBottom,platStatic,platMoving,playerBody; float gapMin,gapMax; int moveChance; const char* name; };
struct Particle { Vector2 pos,vel; float life,total; Color color; };
struct KeyBindings { int left=KEY_A; int right=KEY_D; int jump=KEY_SPACE; };

static float GRAVITY=1400.f; static float MOVE_ACCEL=3600.f; static float MAX_HSPEED=580.f; static float FRICTION=1800.f; static float BASE_JUMP_SPEED=-900.f; static float EXTRA_JUMP_BOOST=300.f; static float COYOTE_TIME=0.10f; static float JUMP_BUFFER=0.12f;

int main(){
    const int screenWidth=480, screenHeight=800; InitWindow(screenWidth,screenHeight,"Shapey Tower"); SetExitKey(KEY_NULL); SetTargetFPS(60); InitAudioDevice(); SetMasterVolume(1.0f);
    KeyBindings keys; bool paused=true; bool started=false; enum RebindTarget{NONE,REB_LEFT,REB_RIGHT,REB_JUMP}; RebindTarget rebindTarget=NONE;
    // Volume scale for UI sliders (0..1)
    const float VOLUME_SCALE=0.01f;
    float volJump=0.5f, volLand=0.5f, volBounce=0.5f; // start: 0.5 * 0.1 = 0.05

    Sound sndJump{}; Sound sndLand{}; Sound sndBounce{}; if(FileExists("jump.mp3")) sndJump=LoadSound("jump.mp3"); if(FileExists("land.mp3")) sndLand=LoadSound("land.mp3"); if(FileExists("bounce.mp3")) sndBounce=LoadSound("bounce.mp3");

    std::srand((unsigned)std::time(nullptr));
    Player player{{screenWidth/2.f-16.f,screenHeight-120.f},{0,0},32.f,40.f};
    std::vector<Platform> platforms; platforms.push_back({Rectangle{0,(float)screenHeight-60.f,(float)screenWidth,20.f}});
    float highestPlatformY=platforms.back().rect.y; Camera2D camera{}; camera.target={(float)screenWidth/2,player.pos.y+player.height/2}; camera.offset={screenWidth/2.f,screenHeight/2.f}; camera.zoom=1.f; float cameraTopY=camera.target.y; const float deadzone=200.f;
    int score=0, highScore=0; { std::ifstream ifs("highscore.txt"); if(ifs) ifs>>highScore; }
    std::vector<Theme> themes={ {{18,24,40,255},{45,58,92,255},{120,210,255,255},{90,180,255,255},{255,220,110,255},75.f,110.f,25,"Start"}, {{12,40,22,255},{30,80,46,255},{90,200,110,255},{130,240,150,255},{255,235,130,255},80.f,115.f,30,"Forest"}, {{40,12,48,255},{80,24,96,255},{210,120,255,255},{240,170,255,255},{255,210,170,255},85.f,120.f,35,"Mystic"}, {{48,28,8,255},{120,70,25,255},{220,170,90,255},{250,200,120,255},{255,210,150,255},90.f,125.f,40,"Desert"}, {{8,12,60,255},{20,30,110,255},{140,170,255,255},{170,200,255,255},{255,245,180,255},95.f,130.f,45,"Night"} };
    const int PLATFORMS_PER_THEME=30; int currentThemeIndex=0; Theme currentTheme=themes[currentThemeIndex]; int generatedPlatformsCount=1; float themeChangeTimer=0.f; float comboTimer=0.f; const float COMBO_WINDOW=2.f; int comboCount=0; int lastLandedPlatformIndex=0; int lastScoredPlatformIndex=-1; float lastLandY=platforms[0].rect.y; bool gameOver=false; std::vector<Particle> particles; bool scrollActive=false; int speedStage=0; float stageTimer=0.f; const float STAGE_DURATION=30.f; float scrollSpeed=60.f;

    auto applyThemeIfNeeded=[&](){ int stage=generatedPlatformsCount/PLATFORMS_PER_THEME; int nextTheme=stage%(int)themes.size(); if(nextTheme!=currentThemeIndex){ currentThemeIndex=nextTheme; currentTheme=themes[currentThemeIndex]; themeChangeTimer=3.f; } };
    auto spawnPlatform=[&](float y){ float width=80.f+(std::rand()%140); float x=(float)(std::rand()%(int)(screenWidth-(int)width)); Platform p; p.rect={x,y,width,18.f}; if((std::rand()%100)<currentTheme.moveChance){ p.moving=true; p.baseX=x; p.moveAmplitude=40.f+(std::rand()%41); p.moveSpeed=1.f+(std::rand()%200)/100.f; } platforms.push_back(p); generatedPlatformsCount++; applyThemeIfNeeded(); };
    float currentY=platforms[0].rect.y-100.f; for(int i=0;i<15;i++){ float gap=currentTheme.gapMin+(std::rand()%(int)(currentTheme.gapMax-currentTheme.gapMin+1)); currentY-=gap; spawnPlatform(currentY);} highestPlatformY=currentY;
    auto emitLandingParticles=[&](Vector2 contact,int count){ for(int i=0;i<count;i++){ float spd=60.f+(std::rand()%120); float ang=(-90.f+(std::rand()%120)-60.f)*(3.14159f/180.f); Vector2 v{cosf(ang)*spd,sinf(ang)*spd}; particles.push_back({contact,v,0.6f,0.6f,currentTheme.platStatic}); }};
    auto emitWallBounceParticles=[&](Vector2 contact,int count){ for(int i=0;i<count;i++){ float spd=80.f+(std::rand()%150); float ang=(180.f+(std::rand()%100)-50.f)*(3.14159f/180.f); Vector2 v{cosf(ang)*spd,sinf(ang)*spd}; particles.push_back({contact,v,0.4f,0.4f,currentTheme.playerBody}); }};

    auto resetGame=[&](){ score=0; comboTimer=0.f; comboCount=0; lastLandedPlatformIndex=0; gameOver=false; player.pos={(float)screenWidth/2 - player.width/2, platforms[0].rect.y - player.height}; player.vel={0,0}; platforms.clear(); platforms.push_back({Rectangle{0,(float)screenHeight-60.f,(float)screenWidth,20.f}}); float currentY2=platforms[0].rect.y-100.f; auto spawnPlatformLocal=[&](float y){ float w=80.f+(std::rand()%140); float x=(float)(std::rand()%(int)(screenWidth-(int)w)); Platform p; p.rect={x,y,w,18.f}; if((std::rand()%100)<25){ p.moving=true; p.baseX=x; p.moveAmplitude=40.f+(std::rand()%41); p.moveSpeed=1.f+(std::rand()%200)/100.f; } platforms.push_back(p); }; currentThemeIndex=0; currentTheme=themes[0]; themeChangeTimer=2.f; generatedPlatformsCount=1; lastScoredPlatformIndex=-1; lastLandY=platforms[0].rect.y; for(int i=0;i<15;i++){ float gap=currentTheme.gapMin+(std::rand()%(int)(currentTheme.gapMax-currentTheme.gapMin+1)); currentY2-=gap; spawnPlatformLocal(currentY2); generatedPlatformsCount++; } highestPlatformY=currentY2; cameraTopY=player.pos.y+player.height/2; scrollActive=false; speedStage=0; stageTimer=0.f; scrollSpeed=60.f; };

    bool enableShadows=true; 
    Vector2 lightPos{screenWidth*0.2f, cameraTopY - 400.f};
    

    static float coyoteTimer=0.f, jumpBufferTimer=0.f; static bool onGround=false;

    auto keyName=[](int key){
        switch(key){
            case KEY_A: return "A"; case KEY_D: return "D"; case KEY_LEFT: return "LEFT"; case KEY_RIGHT: return "RIGHT";
            case KEY_SPACE: return "SPACE"; case KEY_W: return "W"; case KEY_S: return "S"; case KEY_UP: return "UP"; case KEY_DOWN: return "DOWN";
            case KEY_LEFT_SHIFT: return "LSHIFT"; case KEY_RIGHT_SHIFT: return "RSHIFT"; case KEY_ENTER: return "ENTER"; case KEY_TAB: return "TAB"; case KEY_LEFT_CONTROL: return "LCTRL"; case KEY_RIGHT_CONTROL: return "RCTRL"; case KEY_ESCAPE: return "ESC";
            default: return TextFormat("%d", key);
        }
    };

    TraceLog(LOG_INFO, "ENTER MAIN LOOP setup");
    int debugFrame=0;
    bool running=true;
    const int MAX_DEBUG_FRAMES=5;
    while(running && !WindowShouldClose()) {
        if(debugFrame < MAX_DEBUG_FRAMES) TraceLog(LOG_INFO, TextFormat("FRAME %d loop", debugFrame));
        if(IsKeyPressed(KEY_GRAVE)) { TraceLog(LOG_INFO, "GRAVE pressed -> running=false"); running=false; }
    if(IsKeyPressed(KEY_ESCAPE) && started && !gameOver){ paused=!paused; rebindTarget=NONE; }
    
        if(paused){
            BeginDrawing(); ClearBackground(Color{10,14,20,255});
            const char* title = started?"PAUZA":"SHAPEY TOWER";
            int tw = MeasureText(title, started?30:36);
            DrawText(title, screenWidth/2 - tw/2, 40, started?30:36, RAYWHITE);
            int y=130;
            Vector2 mPos=GetMousePosition(); bool click=IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
            auto drawButton=[&](const char* label){ int w=300; int h=40; int x=screenWidth/2 - w/2; Rectangle rc{(float)x,(float)y,(float)w,(float)h}; Color c=CheckCollisionPointRec(mPos,rc)?Color{90,140,220,255}:Color{60,90,140,255}; DrawRectangleRec(rc,c); DrawRectangleLines(x,y,w,h,RAYWHITE); DrawText(label,x+16,y+12,20,RAYWHITE); y+=h+18; return rc; };
            if(!started){
                Rectangle rStart=drawButton("Start");
                if(click && CheckCollisionPointRec(mPos,rStart)) { resetGame(); started=true; paused=false; EndDrawing(); continue; }
                Rectangle rExit =drawButton("Exit");
                if(click && CheckCollisionPointRec(mPos,rExit)) { TraceLog(LOG_INFO, "Exit from main menu"); running=false; EndDrawing(); break; }
                DrawText("ESC - open menu", screenWidth/2-MeasureText("ESC - open menu",16)/2, y+10,16,{200,200,200,200});
                EndDrawing(); if(!running) break; else continue;
            } else {
                Rectangle rCont=drawButton("Continue"); if(click && CheckCollisionPointRec(mPos,rCont)){ paused=false; }
                Rectangle rReset=drawButton("Reset"); if(click && CheckCollisionPointRec(mPos,rReset)){ resetGame(); paused=false; }
                Rectangle rExit=drawButton("Exit"); if(click && CheckCollisionPointRec(mPos,rExit)){ TraceLog(LOG_INFO, "Exit from pause menu"); running=false; EndDrawing(); break; }
                DrawText("Controls & Audio", 40, y, 18, RAYWHITE); y+=30;
                auto drawRebind=[&](const char* label,int key,RebindTarget target){ int w=320,h=30; int x=screenWidth/2 - w/2; Rectangle rc{(float)x,(float)y,(float)w,(float)h}; Color c = (rebindTarget==target)?Color{120,70,30,255}:Color{50,60,80,255}; DrawRectangleRec(rc,c); DrawRectangleLines((int)rc.x,(int)rc.y,(int)rc.width,(int)rc.height,RAYWHITE); std::string txt=std::string(label)+": "+ keyName(key); DrawText(txt.c_str(), x+10, y+7,16,RAYWHITE); if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&CheckCollisionPointRec(GetMousePosition(),rc)){ if(rebindTarget==target) rebindTarget=NONE; else rebindTarget=target; } y+=h+8; };
                drawRebind("Left", keys.left, REB_LEFT); drawRebind("Right", keys.right, REB_RIGHT); drawRebind("Jump", keys.jump, REB_JUMP);
                if(rebindTarget!=NONE){ for(int k=32;k<350;k++){ if(IsKeyPressed(k)){ if(rebindTarget==REB_LEFT) keys.left=k; else if(rebindTarget==REB_RIGHT) keys.right=k; else if(rebindTarget==REB_JUMP) keys.jump=k; rebindTarget=NONE; break; } } }
                DrawText("Volumes:",40,y,18,RAYWHITE); y+=28;
                auto volSlider=[&](const char* name,float &val){ int w=320,h=24; int x=screenWidth/2 - w/2; Rectangle bar{(float)x,(float)y,(float)w,(float)h}; DrawRectangleRec(bar,Color{40,50,70,255}); DrawRectangleLines(x,y,w,h,RAYWHITE); float knobX = x + val * (w-10); Rectangle knob{knobX,(float)y,10.f,(float)h}; DrawRectangleRec(knob,Color{150,180,240,255}); DrawText(name,x+5,y+4,16,RAYWHITE); if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)&&CheckCollisionPointRec(mPos,bar)){ val=(mPos.x-x)/(float)w; if(val<0) val=0; if(val>1) val=1; } y+=h+12; };
                volSlider("Jump", volJump); volSlider("Land", volLand); volSlider("Bounce", volBounce);
                DrawText("~ - exit", 10, screenHeight-30,16,{180,180,180,180});
        EndDrawing(); if(!running) break; else continue;
            }
        }

    if(gameOver){ if(IsKeyPressed(KEY_R)) resetGame(); BeginDrawing(); ClearBackground({20,20,30,255}); BeginMode2D(camera); for(auto &pf:platforms) DrawRectangleRec(pf.rect,pf.moving?Color{120,200,255,255}:Color{100,180,220,255}); DrawRectangle((int)player.pos.x,(int)player.pos.y,(int)player.width,(int)player.height,{255,80,80,255}); EndMode2D(); DrawText(TextFormat("GAME OVER - Score %d Best %d",score,highScore),40,200,24,RAYWHITE); DrawText("Press R to restart",40,240,20,{200,200,200,255}); EndDrawing(); continue; }

        float dtGame=GetFrameTime(); Vector2 prevPos=player.pos; float dir=0.f; if(IsKeyDown(keys.left)) dir-=1.f; if(IsKeyDown(keys.right)) dir+=1.f; if(dir!=0.f){ player.vel.x += dir*MOVE_ACCEL*dtGame; } else if(onGround){ if(player.vel.x>0) player.vel.x=fmaxf(0.f,player.vel.x-FRICTION*dtGame); else if(player.vel.x<0) player.vel.x=fminf(0.f,player.vel.x+FRICTION*dtGame);} if(player.vel.x>MAX_HSPEED) player.vel.x=MAX_HSPEED; if(player.vel.x<-MAX_HSPEED) player.vel.x=-MAX_HSPEED;
        bool jumpKeyPressed = IsKeyPressed(keys.jump); bool jumpKeyDown=IsKeyDown(keys.jump);
        if(jumpKeyPressed || jumpKeyDown) jumpBufferTimer=JUMP_BUFFER; else if(jumpBufferTimer>0) jumpBufferTimer-=dtGame;
        if(onGround) coyoteTimer=COYOTE_TIME; else if(coyoteTimer>0) coyoteTimer-=dtGame;
        bool wantsJump = (jumpBufferTimer>0 && coyoteTimer>0);
        player.vel.y += GRAVITY*dtGame;
        player.pos.x += player.vel.x*dtGame;
        player.pos.y += player.vel.y*dtGame;
        onGround=false;
        if(player.vel.y>0){
            float prevBottom = prevPos.y+player.height;
            float currBottom = player.pos.y+player.height;
            for(size_t i=0;i<platforms.size();++i){
                auto &pf=platforms[i];
                float topY=pf.rect.y;
                if(prevBottom<=topY && currBottom>=topY){
                    float pL=player.pos.x, pR=player.pos.x+player.width;
                    float platL=pf.rect.x, platR=pf.rect.x+pf.rect.width;
                    if(pR>platL && pL<platR){
                        player.pos.y=topY-player.height;
                        player.vel.y=0;
                        onGround=true;
                        emitLandingParticles({player.pos.x+player.width/2,topY},10+std::rand()%10);
                        if(sndLand.frameCount>0){ SetSoundVolume(sndLand, volLand*VOLUME_SCALE); PlaySound(sndLand);}                        
                        if(!scrollActive && (int)i>0){ scrollActive=true; stageTimer=0.f; }
                        if((int)i!=lastScoredPlatformIndex){
                            float deltaY=lastLandY-pf.rect.y;
                            int floorsJumped=(int)(deltaY/95.f); if(floorsJumped<1) floorsJumped=1;
                            if(comboTimer>0) comboCount++; else comboCount=1; comboTimer=COMBO_WINDOW;
                            int base=50; int heightBonus=floorsJumped>1?(floorsJumped-1)*30:0;
                            int gained=(base+heightBonus)*comboCount; score+=gained;
                            lastLandedPlatformIndex=(int)i; lastScoredPlatformIndex=(int)i; lastLandY=pf.rect.y;
                        }
                        break;
                    }
                }
            }
        }
        // Apply buffered/coyote jump
        if(wantsJump){
            float speedRatio=fminf(fabsf(player.vel.x)/MAX_HSPEED,1.f);
            float jumpSpeed=BASE_JUMP_SPEED - EXTRA_JUMP_BOOST*speedRatio;
            player.vel.y=jumpSpeed;
            onGround=false;
            coyoteTimer=0.f; jumpBufferTimer=0.f;
            if(sndJump.frameCount>0){ SetSoundVolume(sndJump, volJump*VOLUME_SCALE); PlaySound(sndJump);}        
        }
        if(comboTimer>0){ comboTimer-=dtGame; if(comboTimer<=0){ comboTimer=0; comboCount=0; }}
    const float restitution=0.95f; const float wallImpulse=80.f; if(player.pos.x<0.f){ player.pos.x=0.f; if(player.vel.x<0){ player.vel.x=-player.vel.x*restitution + wallImpulse; emitWallBounceParticles({0.f,player.pos.y+player.height/2},6); if(sndBounce.frameCount>0){ SetSoundVolume(sndBounce, volBounce*VOLUME_SCALE); PlaySound(sndBounce);} } } if(player.pos.x+player.width>(float)screenWidth){ player.pos.x=(float)screenWidth-player.width; if(player.vel.x>0){ player.vel.x=-player.vel.x*restitution - wallImpulse; emitWallBounceParticles({(float)screenWidth,player.pos.y+player.height/2},6); if(sndBounce.frameCount>0){ SetSoundVolume(sndBounce, volBounce*VOLUME_SCALE); PlaySound(sndBounce);} } }
        for(auto &pf:platforms) if(pf.moving){ float t=(float)GetTime(); pf.rect.x=pf.baseX + sinf(t*pf.moveSpeed)*pf.moveAmplitude; }
        float genThreshold=cameraTopY - 800.f; while(highestPlatformY>genThreshold){ float dynamicMax=currentTheme.gapMax - speedStage*5.f - (scrollActive?10.f:0.f); if(dynamicMax<currentTheme.gapMin+5.f) dynamicMax=currentTheme.gapMin+5.f; float gap=currentTheme.gapMin + (std::rand()%(int)(dynamicMax-currentTheme.gapMin+1)); highestPlatformY -= gap; spawnPlatform(highestPlatformY);} float desiredY=player.pos.y+player.height/2; if(desiredY < cameraTopY - deadzone) cameraTopY = desiredY + deadzone; if(scrollActive){ stageTimer += dtGame; if(speedStage<5 && stageTimer>=STAGE_DURATION){ stageTimer-=STAGE_DURATION; speedStage++; if(speedStage<5) scrollSpeed*=1.25f; } cameraTopY -= scrollSpeed*dtGame; float topLimit=player.pos.y+player.height/2+deadzone; if(cameraTopY > topLimit) cameraTopY = topLimit; } camera.target={(float)screenWidth/2,cameraTopY}; float cleanupY = cameraTopY + screenHeight + 300.f; if(platforms.size()>50){ platforms.erase(std::remove_if(platforms.begin(),platforms.end(),[&](const Platform &p){ return p.rect.y > cleanupY; }), platforms.end()); } float loseThreshold=cameraTopY + screenHeight/2.f + 40.f; if(player.pos.y > loseThreshold) gameOver=true;

        BeginDrawing(); Color topCol=currentTheme.bgTop,bottomCol=currentTheme.bgBottom; for(int y=0;y<screenHeight;y+=4){ float k=(float)y/screenHeight; unsigned char r=(unsigned char)(topCol.r+(bottomCol.r-topCol.r)*k); unsigned char g=(unsigned char)(topCol.g+(bottomCol.g-topCol.g)*k); unsigned char b=(unsigned char)(topCol.b+(bottomCol.b-topCol.b)*k); DrawRectangle(0,y,screenWidth,4,{r,g,b,255}); } for(size_t i=0;i<particles.size();){ Particle &p=particles[i]; p.life-=dtGame; if(p.life<=0){ particles[i]=particles.back(); particles.pop_back(); continue; } p.vel.y += GRAVITY*0.2f*dtGame; p.pos.x += p.vel.x*dtGame; p.pos.y += p.vel.y*dtGame; i++; } BeginMode2D(camera);
        
        for(auto &pf:platforms){ Color c=pf.moving?currentTheme.platMoving:currentTheme.platStatic; DrawRectangleRec(pf.rect,c); DrawRectangle((int)pf.rect.x,(int)pf.rect.y,(int)pf.rect.width,3,{255,255,255,60}); DrawPixel((int)pf.rect.x,(int)pf.rect.y,c); DrawPixel((int)(pf.rect.x+pf.rect.width-1),(int)pf.rect.y,c); DrawRectangleLines((int)pf.rect.x,(int)pf.rect.y,(int)pf.rect.width,(int)pf.rect.height,{255,255,255,30}); }
        for(auto &p:particles){ float a=p.life/p.total; unsigned char alpha=(unsigned char)(200*a); DrawRectangle((int)p.pos.x-2,(int)p.pos.y-2,4,4,{p.color.r,p.color.g,p.color.b,alpha}); }
        DrawRectangle((int)player.pos.x,(int)player.pos.y,(int)player.width,(int)player.height,currentTheme.playerBody); DrawRectangleLines((int)player.pos.x,(int)player.pos.y,(int)player.width,(int)player.height,{40,40,40,255}); EndMode2D(); if(score>highScore) highScore=score; DrawText(TextFormat("Score: %d  Best: %d",score,highScore),10,10,20,RAYWHITE); if(comboCount>1 && comboTimer>0) DrawText(TextFormat("Combo x%d",comboCount),10,40,18,{255,200,100,255}); if(themeChangeTimer>0){ themeChangeTimer-=dtGame; float alpha=themeChangeTimer/3.f; if(alpha<0) alpha=0; if(alpha>1) alpha=1; int a=(int)(alpha*255); const char* name=currentTheme.name; int w=MeasureText(name,36); DrawText(name,screenWidth/2-w/2,80,36,{255,255,255,(unsigned char)a}); } DrawText(TextFormat("Platforms: %d Theme:%s(%d)",generatedPlatformsCount,currentTheme.name,currentThemeIndex),10,70,14,{180,200,230,200});
        // HUD clock
        float radius=26.f; float clockX=screenWidth - radius - 20.f; float clockY=radius + 20.f; 
        DrawCircleLines((int)clockX,(int)clockY,radius,RAYWHITE);
        int segmentsDone=speedStage;
        for(int s=0;s<segmentsDone && s<5;++s){
            float a0=-PI/2+(2*PI/5)*s; float a1=-PI/2+(2*PI/5)*(s+1);
            Vector2 p0{clockX+cosf(a0)*radius*0.9f,clockY+sinf(a0)*radius*0.9f};
            Vector2 p1{clockX+cosf(a1)*radius*0.9f,clockY+sinf(a1)*radius*0.9f};
            DrawLineEx({clockX,clockY},p0,2.f,{180,180,255,200});
            DrawLineEx({clockX,clockY},p1,2.f,{180,180,255,200});
        }
        float phase=(speedStage<5)?(stageTimer/STAGE_DURATION):0.f;
        float angle=(speedStage<5)?(-PI/2+phase*2*PI):(-PI/2-GetTime()*5.f);
        Vector2 hand{clockX+cosf(angle)*radius*0.85f,clockY+sinf(angle)*radius*0.85f};
        DrawLineEx({clockX,clockY},hand,3.f,(speedStage<5)?Color{255,220,120,255}:Color{255,80,80,255});
        DrawCircle((int)clockX,(int)clockY,3,(speedStage<5)?RAYWHITE:Color{255,80,80,255});
        if(scrollActive) DrawText(TextFormat("Speed x%.2f stage %d",scrollSpeed/60.f,speedStage),(int)(clockX-radius-140), (int)(clockY+radius+4),14,{200,180,255,200});
    EndDrawing();
        debugFrame++;
    } // end while
    { std::ofstream ofs("highscore.txt",std::ios::trunc); if(ofs) ofs<<highScore; }
    if(sndJump.frameCount>0) UnloadSound(sndJump);
    if(sndLand.frameCount>0) UnloadSound(sndLand);
    if(sndBounce.frameCount>0) UnloadSound(sndBounce);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
