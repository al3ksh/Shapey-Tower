#pragma once
#include <string>
#include "raylib.h"
struct GameSettings {
    int configVersion = 2; 
    int resolutionIndex = 0; 
    bool fullscreen = false;
    float master = 0.5f;
    float music = 0.5f;
    float jump = 0.5f;
    float bounce = 0.5f;
    float death = 0.5f;
    float theme = 0.5f;
    int keyLeft = KEY_A;
    int keyRight = KEY_D;
    int keyJump = KEY_SPACE;
    bool showFPS = false;
};

bool LoadSettings(const std::string &path, GameSettings &out);
bool SaveSettings(const std::string &path, const GameSettings &in);
