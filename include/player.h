// player.h - definicja struktury gracza i bazowa aktualizacja fizyki poziomej/pionowej
#pragma once
#include "raylib.h"

struct Player {
    Vector2 pos;
    Vector2 vel;
    float width = 32.f;
    float height = 40.f;
};

void UpdatePlayerPhysics(Player &player, float dt, float dir, bool onGround,
                          float MOVE_ACCEL, float MAX_HSPEED, float FRICTION, float GRAVITY);
