#include "tutorial.h"
#include "localization.h"
#include "raylib.h"
#include <cmath>
#include <cstdio>

void UpdateTutorial(TutorialState &ts, float dt, const KeyBindings &keys) {
    if (!ts.active || ts.currentStep == TutorialStep::NONE || ts.currentStep == TutorialStep::DONE) return;

    ts.stepTimer += dt;
    ts.arrowPulse += dt * 3.f;

    if (ts.messageAlpha < 1.f) {
        ts.messageAlpha += dt * 2.5f;
        if (ts.messageAlpha > 1.f) ts.messageAlpha = 1.f;
    }

    switch (ts.currentStep) {
        case TutorialStep::WELCOME:
            if (ts.stepTimer > 2.5f) {
                ts.currentStep = TutorialStep::MOVE_LEFT_RIGHT;
                ts.stepTimer = 0.f;
                ts.messageAlpha = 0.f;
            }
            break;
        case TutorialStep::MOVE_LEFT_RIGHT:
            if (IsKeyDown(keys.left) && !ts.moveLeftDone) ts.moveLeftDone = true;
            if (IsKeyDown(keys.right) && !ts.moveRightDone) ts.moveRightDone = true;
            if (ts.moveLeftDone && ts.moveRightDone && ts.stepTimer > 1.f) {
                ts.actionCompleted = true;
                ts.currentStep = TutorialStep::JUMP;
                ts.stepTimer = 0.f;
                ts.messageAlpha = 0.f;
            }
            break;
        case TutorialStep::JUMP:
            if (IsKeyPressed(keys.jump)) ts.jumpDone = true;
            if (ts.jumpDone && ts.stepTimer > 0.8f) {
                ts.actionCompleted = true;
                ts.currentStep = TutorialStep::LAND_ON_PLATFORMS;
                ts.stepTimer = 0.f;
                ts.messageAlpha = 0.f;
            }
            break;
        case TutorialStep::LAND_ON_PLATFORMS:
            if (ts.platformsLanded >= 3) {
                ts.actionCompleted = true;
                ts.currentStep = TutorialStep::COLLECT_COINS;
                ts.stepTimer = 0.f;
                ts.messageAlpha = 0.f;
            }
            break;
        case TutorialStep::COLLECT_COINS:
            if (ts.coinDone || ts.stepTimer > 8.f) {
                ts.currentStep = TutorialStep::SPECIAL_PLATFORMS;
                ts.stepTimer = 0.f;
                ts.messageAlpha = 0.f;
            }
            break;
        case TutorialStep::SPECIAL_PLATFORMS:
            if (ts.stepTimer > 4.f) {
                ts.currentStep = TutorialStep::COMBO_SYSTEM;
                ts.stepTimer = 0.f;
                ts.messageAlpha = 0.f;
            }
            break;
        case TutorialStep::COMBO_SYSTEM:
            if (ts.stepTimer > 3.5f) {
                ts.currentStep = TutorialStep::POWER_UPS;
                ts.stepTimer = 0.f;
                ts.messageAlpha = 0.f;
            }
            break;
        case TutorialStep::POWER_UPS:
            if (ts.stepTimer > 3.5f) {
                ts.currentStep = TutorialStep::SCROLL_WARNING;
                ts.stepTimer = 0.f;
                ts.messageAlpha = 0.f;
            }
            break;
        case TutorialStep::SCROLL_WARNING:
            if (ts.stepTimer > 3.f) {
                ts.currentStep = TutorialStep::DONE;
                ts.stepTimer = 0.f;
                ts.active = false;
                ts.actionCompleted = true;
            }
            break;
        default:
            break;
    }
}

static const char *GetStepText(TutorialStep step) {
    switch (step) {
        case TutorialStep::WELCOME:
            return Loc::Tutorial_Welcome();
        case TutorialStep::MOVE_LEFT_RIGHT:
            return Loc::Tutorial_Move();
        case TutorialStep::JUMP:
            return Loc::Tutorial_Jump();
        case TutorialStep::LAND_ON_PLATFORMS:
            return Loc::Tutorial_Land();
        case TutorialStep::COLLECT_COINS:
            return Loc::Tutorial_Coins();
        case TutorialStep::SPECIAL_PLATFORMS:
            return Loc::Tutorial_Special();
        case TutorialStep::COMBO_SYSTEM:
            return Loc::Tutorial_Combo();
        case TutorialStep::POWER_UPS:
            return Loc::Tutorial_PowerUps();
        case TutorialStep::SCROLL_WARNING:
            return Loc::Tutorial_Scroll();
        default:
            return "";
    }
}

static const char *GetStepSubText(TutorialStep step) {
    switch (step) {
        case TutorialStep::WELCOME:
            return Loc::Tutorial_WelcomeSub();
        case TutorialStep::MOVE_LEFT_RIGHT:
            return Loc::Tutorial_MoveSub();
        case TutorialStep::JUMP:
            return Loc::Tutorial_JumpSub();
        case TutorialStep::LAND_ON_PLATFORMS:
            return Loc::Tutorial_LandSub();
        case TutorialStep::COLLECT_COINS:
            return Loc::Tutorial_CoinsSub();
        case TutorialStep::SPECIAL_PLATFORMS:
            return Loc::Tutorial_SpecialSub();
        case TutorialStep::COMBO_SYSTEM:
            return Loc::Tutorial_ComboSub();
        case TutorialStep::POWER_UPS:
            return Loc::Tutorial_PowerUpsSub();
        case TutorialStep::SCROLL_WARNING:
            return Loc::Tutorial_ScrollSub();
        default:
            return "";
    }
}

void DrawTutorialOverlay(const TutorialState &ts, int gameWidth, int gameHeight) {
    if (!ts.active || ts.currentStep == TutorialStep::NONE || ts.currentStep == TutorialStep::DONE) return;

    unsigned char masterAlpha = (unsigned char)(ts.messageAlpha * 255);

    int boxW = gameWidth - 40;
    int boxH = 72;
    int boxX = 20;
    int boxY = gameHeight - boxH - 50;

    DrawRectangle(0, boxY - 10, gameWidth, boxH + 60, Color{0, 0, 0, (unsigned char)(masterAlpha * 0.5f)});
    DrawRectangle(boxX, boxY, boxW, boxH, Color{10, 18, 40, (unsigned char)(masterAlpha * 0.92f)});
    DrawRectangleLines(boxX, boxY, boxW, boxH, Color{80, 160, 255, (unsigned char)(masterAlpha * 0.8f)});

    float pulse = (std::sin(ts.arrowPulse) * 0.5f + 0.5f);
    unsigned char iconAlpha = (unsigned char)(masterAlpha * (0.5f + 0.5f * pulse));

    int iconX = boxX + 14;
    int iconY = boxY + 8;
    DrawText("!", iconX + 2, iconY, 22, Color{100, 200, 255, iconAlpha});

    const char *mainText = GetStepText(ts.currentStep);
    int mainFont = 17;
    int tw = MeasureText(mainText, mainFont);
    int textX = boxX + boxW / 2 - tw / 2;
    if (textX < boxX + 30) textX = boxX + 30;
    DrawText(mainText, textX, boxY + 12, mainFont, Color{255, 255, 255, masterAlpha});

    const char *subText = GetStepSubText(ts.currentStep);
    int subFont = 13;
    int stw = MeasureText(subText, subFont);
    int subX = boxX + boxW / 2 - stw / 2;
    if (subX < boxX + 30) subX = boxX + 30;
    DrawText(subText, subX, boxY + 36, subFont, Color{180, 200, 230, (unsigned char)(masterAlpha * 0.85f)});

    bool isActionStep = (ts.currentStep == TutorialStep::MOVE_LEFT_RIGHT ||
                         ts.currentStep == TutorialStep::JUMP ||
                         ts.currentStep == TutorialStep::LAND_ON_PLATFORMS ||
                         ts.currentStep == TutorialStep::COLLECT_COINS);
    if (isActionStep) {
        const char *hint = "...";
        switch (ts.currentStep) {
            case TutorialStep::MOVE_LEFT_RIGHT: {
                int done = (int)ts.moveLeftDone + (int)ts.moveRightDone;
                char buf[64];
                snprintf(buf, sizeof(buf), "%s %d/2", Loc::Tutorial_Progress(), done);
                hint = buf;
                break;
            }
            case TutorialStep::JUMP:
                hint = ts.jumpDone ? Loc::Tutorial_Ok() : Loc::Tutorial_Waiting();
                break;
            case TutorialStep::LAND_ON_PLATFORMS: {
                char buf[64];
                int landed = ts.platformsLanded;
                if (landed > 3) landed = 3;
                snprintf(buf, sizeof(buf), "%s %d/3", Loc::Tutorial_Progress(), landed);
                hint = buf;
                break;
            }
            case TutorialStep::COLLECT_COINS:
                hint = ts.coinDone ? Loc::Tutorial_Ok() : Loc::Tutorial_Optional();
                break;
            default:
                break;
        }
        int hintFont = 12;
        int hw = MeasureText(hint, hintFont);
        DrawText(hint, boxX + boxW / 2 - hw / 2, boxY + boxH - 18, hintFont,
                 Color{140, 220, 160, (unsigned char)(masterAlpha * 0.9f)});
    }

    if (ts.stepTimer > 1.f) {
        const char *skipText = Loc::Tutorial_Skip();
        int skipFont = 11;
        int skipW = MeasureText(skipText, skipFont);
        DrawText(skipText, gameWidth / 2 - skipW / 2, boxY + boxH + 8, skipFont,
                 Color{120, 120, 140, (unsigned char)(masterAlpha * 0.6f)});
    }
}

bool LoadTutorialDone(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return false;
    int v = 0;
    fscanf(f, "%d", &v);
    fclose(f);
    return v == 1;
}

void SaveTutorialDone(const char *path) {
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "1");
        fclose(f);
    }
}
