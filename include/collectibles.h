#pragma once
#include "raylib.h"
#include <vector>

struct Coin {
    Vector2 pos;
    float radius = 12.f;
    bool collected = false;
    float animTime = 0.f;
    int value = 10;
};

enum class PowerUpType {
    DOUBLE_JUMP,
    SHIELD,
    SLOW_MOTION,
    COIN_MAGNET
};

struct PowerUp {
    Vector2 pos;
    PowerUpType type;
    float radius = 16.f;
    bool collected = false;
    float animTime = 0.f;
};

struct ActivePowerUp {
    PowerUpType type;
    float timeRemaining;
};

namespace Collectibles {
    void UpdateCoins(std::vector<Coin>& coins, float dt);
    void DrawCoins(const std::vector<Coin>& coins, float animTime);
    void UpdatePowerUps(std::vector<PowerUp>& powerups, float dt);
    void DrawPowerUps(const std::vector<PowerUp>& powerups, float animTime);
    Color GetPowerUpColor(PowerUpType type);
    const char* GetPowerUpName(PowerUpType type);
}
