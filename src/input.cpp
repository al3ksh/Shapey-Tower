#include "input.h"
#include <string>
#include <cmath>

void UpdateGamepadState(GamepadState &gp) {
    gp.active = IsGamepadAvailable(gp.gamepadId);
    if (!gp.active) {
        gp.leftX = 0.f;
        gp.leftY = 0.f;
        gp.jumpPressed = false;
        gp.jumpDown = false;
        gp.startPressed = false;
        gp.backPressed = false;
        return;
    }

    float ax = GetGamepadAxisMovement(gp.gamepadId, GAMEPAD_AXIS_LEFT_X);
    float ay = GetGamepadAxisMovement(gp.gamepadId, GAMEPAD_AXIS_LEFT_Y);
    auto deadzoneFilter = [d = gp.deadzone](float v) {
        if (std::fabs(v) < d) return 0.f;
        float sign = v < 0 ? -1.f : 1.f;
        float norm = (std::fabs(v) - d) / (1.f - d);
        return sign * norm;
    };
    gp.leftX = deadzoneFilter(ax);
    gp.leftY = deadzoneFilter(ay);

    gp.jumpPressed = IsGamepadButtonPressed(gp.gamepadId, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) ||
                     IsGamepadButtonPressed(gp.gamepadId, GAMEPAD_BUTTON_RIGHT_FACE_UP);
    gp.jumpDown = IsGamepadButtonDown(gp.gamepadId, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) ||
                  IsGamepadButtonDown(gp.gamepadId, GAMEPAD_BUTTON_RIGHT_FACE_UP);

    gp.startPressed = IsGamepadButtonPressed(gp.gamepadId, GAMEPAD_BUTTON_MIDDLE_RIGHT);
    gp.backPressed = IsGamepadButtonPressed(gp.gamepadId, GAMEPAD_BUTTON_MIDDLE_LEFT);
}

float GetGamepadAxisClamped(const GamepadState &gp, int axis) {
    if (!gp.active) return 0.f;
    float v = GetGamepadAxisMovement(gp.gamepadId, axis);
    if (std::fabs(v) < gp.deadzone) return 0.f;
    return v;
}

const char* KeyName(int key){
    switch(key){
        case KEY_A: return "A"; case KEY_D: return "D"; case KEY_LEFT: return "LEFT"; case KEY_RIGHT: return "RIGHT";
        case KEY_SPACE: return "SPACE"; case KEY_W: return "W"; case KEY_S: return "S"; case KEY_UP: return "UP"; case KEY_DOWN: return "DOWN";
        case KEY_LEFT_SHIFT: return "LSHIFT"; case KEY_RIGHT_SHIFT: return "RSHIFT"; case KEY_ENTER: return "ENTER"; case KEY_TAB: return "TAB"; case KEY_LEFT_CONTROL: return "LCTRL"; case KEY_RIGHT_CONTROL: return "RCTRL"; case KEY_ESCAPE: return "ESC";
        default: return TextFormat("%d", key);
    }
}
