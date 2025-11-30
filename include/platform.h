#pragma once
#include "raylib.h"
#include <vector>

enum class PlatformType {
    NORMAL,
    CRUMBLING,
    SPRING,
    ICE,
    DISAPPEARING
};

struct Platform {
    Rectangle rect;
    PlatformType type = PlatformType::NORMAL;
    bool moving = false;
    float baseX = 0.f;
    float moveAmplitude = 0.f;
    float moveSpeed = 0.f;
    float stateTimer = 0.f;
    bool triggered = false;
    float alpha = 1.f;
    bool visible = true;
    float crumbleProgress = 0.f;
};

void UpdateMovingPlatforms(std::vector<Platform> &platforms);
void UpdatePlatformStates(std::vector<Platform> &platforms, float dt);
