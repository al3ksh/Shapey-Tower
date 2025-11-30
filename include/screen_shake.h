#pragma once
#include "raylib.h"

struct ScreenShake {
    float intensity = 0.f;
    float duration = 0.f;
    float timer = 0.f;
    Vector2 offset = {0, 0};
};

inline void TriggerShake(ScreenShake& shake, float intensity, float duration) {
    shake.intensity = intensity;
    shake.duration = duration;
    shake.timer = duration;
}

inline void UpdateShake(ScreenShake& shake, float dt) {
    if (shake.timer > 0) {
        shake.timer -= dt;
        float t = shake.timer / shake.duration;
        float currentIntensity = shake.intensity * t;
        shake.offset.x = ((float)(rand() % 100) / 50.f - 1.f) * currentIntensity;
        shake.offset.y = ((float)(rand() % 100) / 50.f - 1.f) * currentIntensity;
    } else {
        shake.offset = {0, 0};
    }
}
