// theme.h - theme definitions and function returning the list of themes
#pragma once
#include "raylib.h"
#include <vector>

struct Theme {
    Color bgTop,bgBottom,platStatic,platMoving,playerBody;
    float gapMin,gapMax;
    int moveChance;
    const char* name;
};

extern const int PLATFORMS_PER_THEME;

std::vector<Theme> GetThemes();
