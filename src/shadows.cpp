#include "shadows.h"
#include <cmath>

static void ProjectRect(const Rectangle &r, const Vector2 &lightPos){
    Color shadowColor{0,0,0,40};
    Vector2 c[4]={{r.x,r.y},{r.x+r.width,r.y},{r.x+r.width,r.y+r.height},{r.x,r.y+r.height}};
    Vector2 out[4];
    for(int i=0;i<4;i++){
        Vector2 dir={c[i].x-lightPos.x,c[i].y-lightPos.y};
        float len = sqrtf(dir.x*dir.x+dir.y*dir.y)+0.001f;
        dir.x/=len; dir.y/=len;
        float extend=600.f;
        out[i]={c[i].x+dir.x*extend, c[i].y+dir.y*extend};
    }
    for(int layer=0; layer<3; ++layer){
        float fade = 1.f - layer*0.5f; if(fade<0) fade=0;
        unsigned char aAlpha=(unsigned char)(shadowColor.a*fade);
        Color layerCol{shadowColor.r,shadowColor.g,shadowColor.b,aAlpha};
        float shrink = layer*0.15f;
        for(int i=0;i<4;i++){ int j=(i+1)&3; Vector2 a=c[i], b=c[j], A=out[i], B=out[j];
            Vector2 mid{(A.x+B.x)/2.f,(A.y+B.y)/2.f};
            Vector2 A2{A.x+(mid.x-A.x)*shrink, A.y+(mid.y-A.y)*shrink};
            Vector2 B2{B.x+(mid.x-B.x)*shrink, B.y+(mid.y-B.y)*shrink};
            DrawTriangle(a,b,B2,layerCol); DrawTriangle(a,B2,A2,layerCol);
        }
    }
}

void DrawShadows(const ShadowSystem &sys, const std::vector<Platform> &platforms, const Player &player, float cameraTopY){
    if(!sys.enabled) return;
    Vector2 lightPos = sys.lightPos;
    lightPos.y = cameraTopY - 400.f;
    for(auto &pf:platforms) ProjectRect(pf.rect, lightPos);
    Rectangle pr{player.pos.x,player.pos.y,player.width,player.height};
    ProjectRect(pr, lightPos);
}
