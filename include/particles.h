// particles.h - simple particle system extracted from Game
#pragma once
#include <vector>
#include "raylib.h"

struct Particle { Vector2 pos, vel; float life, total; Color color; };

namespace Particles {
    // Update: gravity passed as gravity * gravityScaleFactor (e.g. 0.2f)
    inline void Update(std::vector<Particle>& list, float dt, float gravity, float gravityScale){
        for(size_t i=0;i<list.size();){
            Particle &p=list[i];
            p.life -= dt;
            if(p.life <= 0.f){ list[i]=list.back(); list.pop_back(); continue; }
            p.vel.y += gravity * gravityScale * dt;
            p.pos.x += p.vel.x * dt;
            p.pos.y += p.vel.y * dt;
            ++i;
        }
    }

    inline void Draw(const std::vector<Particle>& list){
        for(auto &p : list){
            float a = p.life / p.total;
            unsigned char alpha = (unsigned char)(200 * a);
            DrawRectangle((int)p.pos.x-2,(int)p.pos.y-2,4,4,{p.color.r,p.color.g,p.color.b,alpha});
        }
    }
}
