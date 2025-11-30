#pragma once
#include "raylib.h"
#include <vector>
#include "platform.h"
#include "player.h"

struct ShadowSystem {
    bool enabled = true;
    Vector2 lightPos{0,0};
};

void DrawShadows(const ShadowSystem &sys, const std::vector<Platform> &platforms, const Player &player, float cameraTopY);
