#include "player.h"
#include <cmath>

void UpdatePlayerPhysics(Player &player, float dt, float dir, bool onGround,
                          float MOVE_ACCEL, float MAX_HSPEED, float FRICTION, float GRAVITY){
    if(dir!=0.f){
        player.vel.x += dir*MOVE_ACCEL*dt;
    } else if(onGround){
        if(player.vel.x>0) player.vel.x=std::fmax(0.f,player.vel.x-FRICTION*dt);
        else if(player.vel.x<0) player.vel.x=std::fmin(0.f,player.vel.x+FRICTION*dt);
    }
    if(player.vel.x>MAX_HSPEED) player.vel.x=MAX_HSPEED;
    if(player.vel.x<-MAX_HSPEED) player.vel.x=-MAX_HSPEED;
    player.vel.y += GRAVITY*dt;
    player.pos.x += player.vel.x*dt;
    player.pos.y += player.vel.y*dt;
}
