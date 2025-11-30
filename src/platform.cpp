#include "platform.h"
#include <cmath>
#include <algorithm>

void UpdateMovingPlatforms(std::vector<Platform> &platforms){
    for(auto &pf:platforms){
        if(pf.moving){
            float t=(float)GetTime();
            pf.rect.x = pf.baseX + std::sin(t*pf.moveSpeed)*pf.moveAmplitude;
        }
    }
}

void UpdatePlatformStates(std::vector<Platform> &platforms, float dt) {
    for (auto &pf : platforms) {
        if (!pf.triggered) continue;
        
        pf.stateTimer += dt;
        
        switch (pf.type) {
            case PlatformType::CRUMBLING:
                if (pf.stateTimer > 0.3f) {
                    pf.alpha = std::max(0.f, 1.f - (pf.stateTimer - 0.3f) * 2.f);
                }
                break;
            case PlatformType::DISAPPEARING:
                if (pf.stateTimer < 0.2f) {
                    pf.alpha = 1.f - pf.stateTimer * 5.f;
                } else if (pf.stateTimer < 2.f) {
                    pf.alpha = 0.f;
                } else if (pf.stateTimer < 2.2f) {
                    pf.alpha = (pf.stateTimer - 2.f) * 5.f;
                } else {
                    pf.alpha = 1.f;
                    pf.triggered = false;
                    pf.stateTimer = 0.f;
                }
                break;
            default:
                break;
        }
    }
    
    platforms.erase(
        std::remove_if(platforms.begin(), platforms.end(),
            [](const Platform& p) { 
                return p.type == PlatformType::CRUMBLING && p.alpha <= 0.f; 
            }),
        platforms.end()
    );
}
