#include "platform.h"
#include <cmath>

void UpdateMovingPlatforms(std::vector<Platform> &platforms){
    for(auto &pf:platforms){
        if(pf.moving){
            float t=(float)GetTime();
            pf.rect.x = pf.baseX + std::sin(t*pf.moveSpeed)*pf.moveAmplitude;
        }
    }
}
