#pragma once
#include "raylib.h"

struct KeyBindings {
    int left = KEY_A;
    int right = KEY_D;
    int jump = KEY_SPACE;
};

struct GamepadState {
    int gamepadId = 0;
    bool active = false;
    float deadzone = 0.25f;
    float leftX = 0.f;
    float leftY = 0.f;
    bool jumpPressed = false;
    bool jumpDown = false;
    bool startPressed = false;
    bool backPressed = false;
};

void UpdateGamepadState(GamepadState &gp);
float GetGamepadAxisClamped(const GamepadState &gp, int axis);

const char* KeyName(int key);
