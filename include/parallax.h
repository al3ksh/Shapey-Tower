#pragma once
#include "raylib.h"
#include <vector>

struct ParallaxLayer {
    float speedFactor;
    Color color;
    float yOffset;
};

inline void DrawParallaxBackground(float cameraY, int width, int height, const std::vector<ParallaxLayer>& layers) {
    for (const auto& layer : layers) {
        float offset = cameraY * layer.speedFactor;
        int stripeHeight = 200;
        int startY = (int)(offset) % stripeHeight - stripeHeight;
        for (int y = startY; y < height + stripeHeight; y += stripeHeight) {
            unsigned char alpha = (unsigned char)(layer.color.a * 0.3f);
            Color c1 = {layer.color.r, layer.color.g, layer.color.b, alpha};
            Color c2 = {(unsigned char)(layer.color.r * 0.8f), (unsigned char)(layer.color.g * 0.8f), (unsigned char)(layer.color.b * 0.8f), (unsigned char)(alpha * 0.7f)};
            DrawRectangle(0, y, width, stripeHeight / 2, c1);
            DrawRectangle(0, y + stripeHeight / 2, width, stripeHeight / 2, c2);
        }
    }
}

inline std::vector<ParallaxLayer> CreateDefaultParallaxLayers() {
    return {
        {0.1f, {40, 50, 80, 60}, 0},
        {0.2f, {60, 70, 100, 40}, 50},
        {0.35f, {80, 90, 120, 30}, 100}
    };
}
