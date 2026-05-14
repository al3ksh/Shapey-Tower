#pragma once
#include "raylib.h"
#include <vector>

struct Theme {
    Color bgTop,bgBottom,platStatic,platMoving,playerBody;
    float gapMin,gapMax;
    int moveChance;
    const char* name;
    bool hasStars;
    Color starColor;
    Color glowColor;
    int biomeType;
};

enum BiomeType {
    BIOME_DEFAULT = 0,
    BIOME_FOREST = 1,
    BIOME_LAVA = 2,
    BIOME_SNOW = 3,
    BIOME_COSMIC = 4,
    BIOME_NEON = 5,
    BIOME_DESERT = 6,
    BIOME_NIGHT = 7,
    BIOME_MONO = 8
};

extern const int PLATFORMS_PER_THEME;

std::vector<Theme> GetThemes();
