#pragma once
#include <string>
#include "raylib.h"
struct GameSettings {
    int configVersion = 5; 
    int resolutionIndex = 0; 
    bool fullscreen = false;
    bool vsync = true;
    int targetFPS = 60;
    float master = 0.5f;
    float music = 0.5f;
    float jump = 0.5f;
    float bounce = 0.5f;
    float death = 0.5f;
    float theme = 0.5f;
    float coin = 0.5f;
    float powerup = 0.5f;
    int keyLeft = KEY_A;
    int keyRight = KEY_D;
    int keyJump = KEY_SPACE;
    bool showFPS = false;
    bool screenShake = true;
    bool particles = true;
    bool comboEffects = true;
    bool powerUpEffects = true;
    int language = 0; // 0 = EN, 1 = PL
};

bool LoadSettings(const std::string &path, GameSettings &out);
bool SaveSettings(const std::string &path, const GameSettings &in);
