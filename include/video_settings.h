#pragma once
#include "raylib.h"
class Game; 

namespace VideoSettings {
    void ApplyResolution(Game& game, bool recenterCamera=true);
    void DrawResolutionSelector(Game& game, int &y, float uiCenterX, Vector2 mPos, bool click, int sw);
}
