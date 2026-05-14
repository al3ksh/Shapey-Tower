#pragma once
#include "input.h"

enum class TutorialStep {
    NONE = -1,
    WELCOME = 0,
    MOVE_LEFT_RIGHT = 1,
    JUMP = 2,
    LAND_ON_PLATFORMS = 3,
    COLLECT_COINS = 4,
    SPECIAL_PLATFORMS = 5,
    COMBO_SYSTEM = 6,
    POWER_UPS = 7,
    SCROLL_WARNING = 8,
    DONE = 9,
    COUNT = 10
};

struct TutorialState {
    TutorialStep currentStep = TutorialStep::NONE;
    bool active = false;
    float stepTimer = 0.f;
    float messageAlpha = 0.f;
    float arrowPulse = 0.f;
    bool actionCompleted = false;
    bool moveLeftDone = false;
    bool moveRightDone = false;
    bool jumpDone = false;
    bool landDone = false;
    bool coinDone = false;
    int platformsLanded = 0;
    bool skipped = false;
};

inline bool IsTutorialActive(const TutorialState &ts) {
    return ts.active && ts.currentStep != TutorialStep::NONE && ts.currentStep != TutorialStep::DONE;
}

inline void StartTutorial(TutorialState &ts) {
    ts.active = true;
    ts.currentStep = TutorialStep::WELCOME;
    ts.stepTimer = 0.f;
    ts.messageAlpha = 0.f;
    ts.arrowPulse = 0.f;
    ts.actionCompleted = false;
    ts.moveLeftDone = false;
    ts.moveRightDone = false;
    ts.jumpDone = false;
    ts.landDone = false;
    ts.coinDone = false;
    ts.platformsLanded = 0;
    ts.skipped = false;
}

void UpdateTutorial(TutorialState &ts, float dt, const KeyBindings &keys);
void DrawTutorialOverlay(const TutorialState &ts, int gameWidth, int gameHeight);
bool LoadTutorialDone(const char *path);
void SaveTutorialDone(const char *path);
