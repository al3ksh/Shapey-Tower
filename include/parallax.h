#pragma once
#include "raylib.h"
#include <vector>
#include <cmath>

struct ParallaxLayer {
    float speedFactor;
    Color color;
    float yOffset;
};

inline void DrawParallaxBackground(float, int, int, const std::vector<ParallaxLayer>&) {}

inline std::vector<ParallaxLayer> CreateDefaultParallaxLayers() {
    return {};
}
