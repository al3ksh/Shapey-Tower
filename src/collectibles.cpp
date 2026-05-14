#include "collectibles.h"
#include <cmath>
#include <algorithm>

namespace Collectibles {

static void DrawArc(int cx, int cy, float radius, float startAngle, float endAngle, int segments, Color color) {
    float step = (endAngle - startAngle) / segments;
    for (int i = 0; i < segments; i++) {
        float a1 = startAngle + step * i;
        float a2 = startAngle + step * (i + 1);
        DrawLine((int)(cx + std::cos(a1) * radius), (int)(cy + std::sin(a1) * radius),
                 (int)(cx + std::cos(a2) * radius), (int)(cy + std::sin(a2) * radius), color);
    }
}

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
        float squeeze = std::fabs(std::cos(c.animTime * 3.f));
        float scaleX = 0.6f + 0.4f * squeeze;
        float drawY = c.pos.y + bounce;
        float drawX = c.pos.x;

        Color gold = {255, 215, 0, 255};
        Color darkGold = {200, 160, 0, 255};
        Color highlight = {255, 250, 200, 200};

        int w = (int)(c.radius * 2 * scaleX);
        int h = (int)(c.radius * 2);
        DrawEllipse((int)drawX, (int)drawY, w / 2, h / 2, darkGold);
        DrawEllipse((int)drawX, (int)drawY, w / 2 - 2, h / 2 - 2, gold);

        if (scaleX > 0.5f) {
            int hw = (int)((w / 2 - 4) * (scaleX - 0.3f));
            if (hw > 1) {
                DrawEllipse((int)drawX - 2, (int)drawY - 3, hw, h / 4, highlight);
            }
        }

        unsigned char glowA = (unsigned char)(40 + 20 * std::sin(c.animTime * 6.f));
        DrawCircle((int)drawX, (int)drawY, (int)(c.radius * 1.6f), Color{255, 200, 0, glowA});
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
        float pulse = 1.f + std::sin(p.animTime * 5.f) * 0.1f;
        float drawY = p.pos.y + bounce;
        Color col = GetPowerUpColor(p.type);

        float ringR = p.radius * pulse * 1.6f;
        unsigned char ringA = (unsigned char)(60 + 40 * std::sin(p.animTime * 4.f));
        DrawCircle((int)p.pos.x, (int)drawY, (int)ringR, Color{col.r, col.g, col.b, ringA});
        DrawCircle((int)p.pos.x, (int)drawY, (int)(ringR * 0.7f), Color{col.r, col.g, col.b, (unsigned char)(ringA * 2)});

        DrawCircle((int)p.pos.x, (int)drawY, (int)(p.radius * pulse), col);

        Color innerCol = {255, 255, 255, 100};
        DrawCircle((int)p.pos.x, (int)(drawY - 3), (int)(p.radius * pulse * 0.5f), innerCol);

        DrawCircleLines((int)p.pos.x, (int)drawY, (int)(p.radius * pulse * 1.2f), {255, 255, 255, 120});

        int ix = (int)p.pos.x;
        int iy = (int)drawY;
        int iconSz = (int)(p.radius * pulse * 0.5f);
        switch(p.type) {
            case PowerUpType::DOUBLE_JUMP: {
                int wingW = iconSz;
                int wingH = iconSz / 2;
                DrawTriangle({(float)(ix - 2), (float)(iy + 2)},
                             {(float)(ix - 2 - wingW), (float)(iy - wingH)},
                             {(float)(ix - 2 - wingW/2), (float)(iy + 2)}, {20, 50, 80, 255});
                DrawTriangle({(float)(ix + 2), (float)(iy + 2)},
                             {(float)(ix + 2 + wingW), (float)(iy - wingH)},
                             {(float)(ix + 2 + wingW/2), (float)(iy + 2)}, {20, 50, 80, 255});
                DrawLine(ix, iy - iconSz/2, ix, iy + iconSz/2, {20, 50, 80, 255});
                break;
            }
            case PowerUpType::SHIELD: {
                DrawTriangle({(float)ix, (float)(iy - iconSz)},
                             {(float)(ix - iconSz), (float)(iy - iconSz/3)},
                             {(float)(ix + iconSz), (float)(iy - iconSz/3)}, {20, 60, 20, 255});
                DrawTriangle({(float)ix, (float)(iy + iconSz * 0.8f)},
                             {(float)(ix - iconSz), (float)(iy - iconSz/3)},
                             {(float)(ix + iconSz), (float)(iy - iconSz/3)}, {20, 60, 20, 255});
                break;
            }
            case PowerUpType::SLOW_MOTION: {
                int r = iconSz * 3 / 4;
                DrawCircleLines(ix, iy, r, {30, 10, 50, 255});
                DrawLine(ix, iy, ix, iy - r, {30, 10, 50, 255});
                DrawLine(ix, iy, ix + r/2, iy + r/3, {30, 10, 50, 255});
                break;
            }
            case PowerUpType::COIN_MAGNET: {
                int mw = iconSz;
                int mh = iconSz / 2;
                DrawLine(ix - mw/2, iy - mh, ix - mw/2, iy + mh, {60, 40, 0, 255});
                DrawLine(ix + mw/2, iy - mh, ix + mw/2, iy + mh, {60, 40, 0, 255});
                DrawArc(ix, iy, mw/2, 0.5f, PI - 0.5f, 8, {60, 40, 0, 255});
                break;
            }
        }
    }
}

}
