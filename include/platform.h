// platform.h - struktura platformy oraz aktualizacja ruchu platform ruchomych
#pragma once
#include "raylib.h"
#include <vector>

struct Platform {
    Rectangle rect;
    bool moving = false;
    float baseX = 0.f;
    float moveAmplitude = 0.f;
    float moveSpeed = 0.f;
};

void UpdateMovingPlatforms(std::vector<Platform> &platforms);
