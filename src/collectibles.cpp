#include "collectibles.h"
#include <cmath>
#include <algorithm>

namespace Collectibles {

void UpdateCoins(std::vector<Coin>& coins, float dt) {
    for (auto& c : coins) {
        c.animTime += dt;
    }
    coins.erase(std::remove_if(coins.begin(), coins.end(), [](const Coin& c){ return c.collected; }), coins.end());
}

void DrawCoins(const std::vector<Coin>& coins, float animTime) {
    for (const auto& c : coins) {
        if (c.collected) continue;
        float bounce = std::sin(c.animTime * 4.f) * 2.f;
        float scale = 1.f + std::sin(c.animTime * 3.f) * 0.1f;
        Color gold = {255, 215, 0, 255};
        Color outline = {200, 160, 0, 255};
        DrawCircle((int)c.pos.x, (int)(c.pos.y + bounce), c.radius * scale, gold);
        DrawCircleLines((int)c.pos.x, (int)(c.pos.y + bounce), c.radius * scale, outline);
        DrawCircle((int)(c.pos.x - 3), (int)(c.pos.y + bounce - 3), 3, {255,255,200,180});
    }
}

void UpdatePowerUps(std::vector<PowerUp>& powerups, float dt) {
    for (auto& p : powerups) {
        p.animTime += dt;
    }
    powerups.erase(std::remove_if(powerups.begin(), powerups.end(), [](const PowerUp& p){ return p.collected; }), powerups.end());
}

Color GetPowerUpColor(PowerUpType type) {
    switch(type) {
        case PowerUpType::DOUBLE_JUMP: return {100, 200, 255, 255};
        case PowerUpType::SHIELD: return {100, 255, 100, 255};
        case PowerUpType::SLOW_MOTION: return {200, 100, 255, 255};
        case PowerUpType::COIN_MAGNET: return {255, 200, 100, 255};
        default: return WHITE;
    }
}

const char* GetPowerUpName(PowerUpType type) {
    switch(type) {
        case PowerUpType::DOUBLE_JUMP: return "Double Jump";
        case PowerUpType::SHIELD: return "Shield";
        case PowerUpType::SLOW_MOTION: return "Slow Motion";
        case PowerUpType::COIN_MAGNET: return "Coin Magnet";
        default: return "???";
    }
}

void DrawPowerUps(const std::vector<PowerUp>& powerups, float animTime) {
    for (const auto& p : powerups) {
        if (p.collected) continue;
        float bounce = std::sin(p.animTime * 3.f) * 3.f;
        float pulse = 1.f + std::sin(p.animTime * 5.f) * 0.15f;
        Color col = GetPowerUpColor(p.type);
        DrawCircle((int)p.pos.x, (int)(p.pos.y + bounce), p.radius * pulse, col);
        DrawCircleLines((int)p.pos.x, (int)(p.pos.y + bounce), p.radius * pulse * 1.2f, {255,255,255,100});
        const char* icon = "?";
        switch(p.type) {
            case PowerUpType::DOUBLE_JUMP: icon = "2x"; break;
            case PowerUpType::SHIELD: icon = "S"; break;
            case PowerUpType::SLOW_MOTION: icon = "~"; break;
            case PowerUpType::COIN_MAGNET: icon = "M"; break;
        }
        int tw = MeasureText(icon, 14);
        DrawText(icon, (int)(p.pos.x - tw/2), (int)(p.pos.y + bounce - 7), 14, BLACK);
    }
}

}
