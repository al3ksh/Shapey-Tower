#include "game.h"
#include "raylib.h"
#include "collectibles.h"
#include "localization.h"
#include "daily_challenge.h"
#include "tutorial.h"
#include "rng.h"
#include <cmath>

#include "debug.h"
#include "constants.h"

static Color LerpColor(Color a, Color b, float t) {
    return {
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}

struct BgStar {
    float x, y, size, speed, phase;
    unsigned char brightness;
};

static std::vector<BgStar> bgStars;
static bool bgStarsInit = false;
static int bgStarsW = 0, bgStarsH = 0;

static void EnsureBgStars(int w, int h) {
    if (bgStarsInit && bgStarsW == w && bgStarsH == h) return;
    bgStars.clear();
    GameRNG starRng;
    starRng.seed(42);
    int count = (w * h) / 2000;
    if (count < 30) count = 30;
    if (count > 120) count = 120;
    for (int i = 0; i < count; i++) {
        BgStar s;
        s.x = (float)starRng.nextInt(w);
        s.y = (float)starRng.nextInt(h);
        s.size = 1.f + starRng.nextFloat(0.f, 2.5f);
        s.speed = 0.3f + starRng.nextFloat(0.f, 1.5f);
        s.phase = starRng.nextFloat(0.f, 6.28f);
        s.brightness = (unsigned char)(100 + starRng.nextInt(155));
        bgStars.push_back(s);
    }
    bgStarsInit = true;
    bgStarsW = w;
    bgStarsH = h;
}

static void DrawVerticalGradient(int w, int h, Color top, Color bottom) {
    for (int y = 0; y < h; y += Const::GRADIENT_STEP) {
        float k = (float)y / h;
        unsigned char r = (unsigned char)(top.r + (bottom.r - top.r) * k);
        unsigned char g = (unsigned char)(top.g + (bottom.g - top.g) * k);
        unsigned char b = (unsigned char)(top.b + (bottom.b - top.b) * k);
        DrawRectangle(0, y, w, Const::GRADIENT_STEP, {r, g, b, 255});
    }
}

static void DrawBgStars(int w, int h, float time, const Theme &theme, float cameraY) {
    if (!theme.hasStars) return;
    EnsureBgStars(w, h);
    for (auto &s : bgStars) {
        float twinkle = std::sin(time * s.speed + s.phase) * 0.5f + 0.5f;
        unsigned char a = (unsigned char)(s.brightness * (0.3f + 0.7f * twinkle));
        float drawY = s.y + cameraY * 0.05f;
        while (drawY > h) drawY -= h;
        while (drawY < 0) drawY += h;
        Color c = {theme.starColor.r, theme.starColor.g, theme.starColor.b, a};
        DrawCircle((int)s.x, (int)drawY, s.size, c);
    }
}

static void DrawVignette(int w, int h) {
    int cx = w / 2, cy = h / 2;
    int maxR = (int)std::sqrt((float)(cx * cx + cy * cy));
    for (int r = maxR; r > maxR - 80; r -= 4) {
        float t = (float)(maxR - r) / 80.f;
        unsigned char a = (unsigned char)(t * t * 60);
        DrawCircleLines(cx, cy, r, {0, 0, 0, a});
    }
}

static void DrawRoundedRect(Rectangle rec, float radius, Color color) {
    DrawRectangleRec(rec, color);
    DrawRectangle((int)rec.x, (int)(rec.y + radius), (int)rec.width, (int)(rec.height - radius * 2), color);
    DrawCircle((int)(rec.x + radius), (int)(rec.y + radius), (int)radius, color);
    DrawCircle((int)(rec.x + rec.width - radius), (int)(rec.y + radius), (int)radius, color);
    DrawCircle((int)(rec.x + radius), (int)(rec.y + rec.height - radius), (int)radius, color);
    DrawCircle((int)(rec.x + rec.width - radius), (int)(rec.y + rec.height - radius), (int)radius, color);
}

static Color GetPlatformColor(const Platform &pf, Color baseMoving, Color baseStatic) {
    Color base = pf.moving ? baseMoving : baseStatic;
    switch (pf.type) {
        case PlatformType::CRUMBLING:
            return Color{180, 120, 80, (unsigned char)(255 * (1.0f - pf.crumbleProgress * 0.5f))};
        case PlatformType::SPRING:
            return Color{80, 200, 120, 255};
        case PlatformType::ICE:
            return Color{150, 220, 255, 230};
        case PlatformType::DISAPPEARING:
            return Color{base.r, base.g, base.b, (unsigned char)(pf.alpha * 255)};
        default:
            return base;
    }
}

static void DrawPlatformGlow(Rectangle rect, Color glowCol, float time, float intensity) {
    float pulse = 0.7f + 0.3f * std::sin(time * 2.f);
    unsigned char ga = (unsigned char)(intensity * pulse * 40);
    int expand = 6;
    DrawRectangle((int)rect.x - expand, (int)rect.y - expand,
                  (int)rect.width + expand * 2, (int)rect.height + expand * 2,
                  {glowCol.r, glowCol.g, glowCol.b, ga});
}

void Game::DrawGameWorld(float dt) {
    float tb = state.themeBlend;
    Theme blended;
    blended.bgTop = LerpColor(state.prevTheme.bgTop, state.currentTheme.bgTop, tb);
    blended.bgBottom = LerpColor(state.prevTheme.bgBottom, state.currentTheme.bgBottom, tb);
    blended.platStatic = LerpColor(state.prevTheme.platStatic, state.currentTheme.platStatic, tb);
    blended.platMoving = LerpColor(state.prevTheme.platMoving, state.currentTheme.platMoving, tb);
    blended.playerBody = LerpColor(state.prevTheme.playerBody, state.currentTheme.playerBody, tb);
    blended.glowColor = LerpColor(state.prevTheme.glowColor, state.currentTheme.glowColor, tb);
    blended.starColor = LerpColor(state.prevTheme.starColor, state.currentTheme.starColor, tb);
    blended.hasStars = state.currentTheme.hasStars || state.prevTheme.hasStars;

    DrawVerticalGradient(cfg.gameWidth, cfg.gameHeight, blended.bgTop, blended.bgBottom);
    DrawBgStars(cfg.gameWidth, cfg.gameHeight, state.animTime, blended, state.camera.target.y);
    if (state.themeBlend < 1.f) {
        unsigned char oldAlpha = (unsigned char)((1.f - state.themeBlend) * 255);
        unsigned char newAlpha = (unsigned char)(state.themeBlend * 255);
        DrawBiomeBackground(cfg.gameWidth, cfg.gameHeight, state.camera.target.y, state.animTime, state.prevTheme.biomeType, {255, 255, 255, oldAlpha});
        DrawBiomeBackground(cfg.gameWidth, cfg.gameHeight, state.camera.target.y, state.animTime, state.currentTheme.biomeType, {255, 255, 255, newAlpha});
    } else {
        DrawBiomeBackground(cfg.gameWidth, cfg.gameHeight, state.camera.target.y, state.animTime, state.currentTheme.biomeType, {255, 255, 255, 255});
    }

    DrawBiomeEffects(cfg.gameWidth, cfg.gameHeight, state.camera.target.y, state.animTime);

    Particles::Update(state.particles, dt, cfg.GRAVITY, 0.2f);
    BeginMode2D(state.camera);

    for (auto &pf : state.platforms) {
        if (!pf.visible) continue;
        Color c = GetPlatformColor(pf, blended.platMoving, blended.platStatic);

        if (pf.type != PlatformType::DISAPPEARING || pf.alpha > 0.1f) {
            DrawPlatformGlow(pf.rect, blended.glowColor, state.animTime,
                             pf.type == PlatformType::SPRING ? 1.5f : 0.8f);
        }

        bool skipDefaultDraw = false;

        if (pf.type == PlatformType::CRUMBLING) {
            skipDefaultDraw = true;
            if (pf.triggered) {
                pf.crumbleProgress = (pf.stateTimer > 0.3f) ? std::fmin(1.f, (pf.stateTimer - 0.3f) * 2.f) : pf.stateTimer / 0.3f;
            }
            if (pf.crumbleProgress > 0.f) {
                float shakeAmt = pf.crumbleProgress * 3.f;
                float shakeX = std::sin(state.animTime * 30.f) * shakeAmt;
                float shakeY = std::cos(state.animTime * 25.f) * shakeAmt;
                DrawRoundedRect({pf.rect.x + shakeX, pf.rect.y + shakeY, pf.rect.width, pf.rect.height}, 5.f, c);
                int cracks = (int)(pf.crumbleProgress * 6) + 1;
                for (int ci = 0; ci < cracks; ci++) {
                    float crackX = pf.rect.x + (ci + 1) * pf.rect.width / (cracks + 1);
                    float wobble = std::sin(ci * 2.7f + state.animTime * 5.f) * 4.f;
                    DrawLine((int)(crackX + shakeX), (int)(pf.rect.y + shakeY),
                             (int)(crackX + wobble + shakeX), (int)(pf.rect.y + pf.rect.height + shakeY),
                             Color{40, 25, 10, 220});
                }
                if (pf.crumbleProgress > 0.4f) {
                    int chunks = (int)(pf.crumbleProgress * 4);
                    for (int ci = 0; ci < chunks; ci++) {
                        float seed = (float)ci * 3.7f;
                        float chunkX = pf.rect.x + std::fmod(seed * 47.1f, pf.rect.width);
                        float chunkY = pf.rect.y + pf.rect.height * 0.3f + pf.crumbleProgress * 20.f * (1.f + ci * 0.5f);
                        unsigned char chunkA = (unsigned char)((1.f - pf.crumbleProgress) * 255);
                        DrawRectangle((int)(chunkX + shakeX), (int)(chunkY + shakeY),
                                      (int)(6.f + std::sin(seed) * 4.f), (int)(4.f + std::cos(seed) * 2.f),
                                      Color{160, 100, 55, chunkA});
                    }
                }
                if (pf.crumbleProgress > 0.6f) {
                    for (int pi = 0; pi < 4; pi++) {
                        float px = pf.rect.x + pf.rect.width * (0.15f + pi * 0.25f);
                        float py = pf.rect.y - pf.crumbleProgress * 15.f * (1.f + pi * 0.3f);
                        unsigned char dustA = (unsigned char)((1.f - pf.crumbleProgress) * 120);
                        DrawCircle((int)(px + shakeX), (int)(py + shakeY), 3.f, Color{140, 100, 60, dustA});
                    }
                }
            } else {
                DrawRoundedRect(pf.rect, 5.f, c);
                DrawRectangle((int)pf.rect.x + 2, (int)pf.rect.y, (int)pf.rect.width - 4, 2, {255, 200, 150, 50});
            }
        }

        if (pf.type == PlatformType::DISAPPEARING) {
            skipDefaultDraw = true;
            if (pf.triggered) {
                float t = pf.stateTimer;
                if (t < 0.5f) {
                    float warn = t / 0.5f;
                    float blink = std::sin(t * 20.f) * 0.5f + 0.5f;
                    unsigned char flashA = (unsigned char)(warn * blink * 180);
                    DrawRoundedRect(pf.rect, 4.f, c);
                    DrawRectangle((int)pf.rect.x, (int)pf.rect.y, (int)pf.rect.width, (int)pf.rect.height, {255, 255, 255, flashA});
                    DrawRectangleLines((int)pf.rect.x, (int)pf.rect.y, (int)pf.rect.width, (int)pf.rect.height,
                                       {255, 200, 200, (unsigned char)(100 + 155 * warn)});
                } else if (t >= 2.f && t < 2.3f) {
                    float reappear = (t - 2.f) / 0.3f;
                    int scanlines = 4;
                    float sliceH = pf.rect.height / scanlines;
                    for (int si = 0; si < scanlines; si++) {
                        float lineProgress = std::fmin(1.f, reappear * scanlines - si);
                        if (lineProgress <= 0.f) continue;
                        unsigned char lineA = (unsigned char)(lineProgress * 180);
                        float sliceY = pf.rect.y + si * sliceH;
                        DrawRectangle((int)pf.rect.x, (int)sliceY, (int)pf.rect.width, (int)(sliceH * lineProgress),
                                      {c.r, c.g, c.b, lineA});
                    }
                    DrawRectangle((int)pf.rect.x - 2, (int)pf.rect.y - 2,
                                  (int)pf.rect.width + 4, (int)pf.rect.height + 4,
                                  {180, 200, 255, (unsigned char)(reappear * 60)});
                }
            } else {
                DrawRoundedRect(pf.rect, 4.f, c);
                float hover = std::sin(state.animTime * 2.f) * 0.3f + 0.7f;
                DrawRectangle((int)pf.rect.x, (int)pf.rect.y, (int)pf.rect.width, (int)pf.rect.height,
                              {200, 220, 255, (unsigned char)(20 * hover)});
            }
        }

        if (!skipDefaultDraw) {
            float cornerR = 5.f;
            if (pf.type == PlatformType::SPRING) cornerR = 7.f;
            if (pf.type == PlatformType::ICE) cornerR = 4.f;
            DrawRoundedRect(pf.rect, cornerR, c);
        }

        if (pf.type == PlatformType::SPRING) {
            float springH = 8.f;
            float springY = pf.rect.y - springH + std::sin(state.animTime * 8.f) * 2.f;
            float coilX = pf.rect.x + pf.rect.width * 0.3f;
            float coilW = pf.rect.width * 0.4f;
            DrawRectangle((int)coilX, (int)springY, (int)coilW, (int)springH, Color{50, 180, 90, 255});
            for (int i = 0; i < 3; i++) {
                float sx = coilX + (i + 1) * coilW / 4.f;
                DrawLine((int)sx, (int)springY, (int)(sx + 2), (int)(springY + springH),
                         Color{30, 120, 60, 200});
            }
        }

        if (pf.type == PlatformType::ICE) {
            for (int i = 0; i < 5; i++) {
                float sparkleX = pf.rect.x + (i + 1) * pf.rect.width / 6.f;
                float sparkleY = pf.rect.y + pf.rect.height * 0.3f;
                float shimmer = std::sin(state.animTime * 5.f + i * 1.5f);
                unsigned char a = (unsigned char)(140 + 115 * shimmer);
                float sz = 1.5f + shimmer * 1.f;
                DrawCircle((int)sparkleX, (int)sparkleY, sz, Color{255, 255, 255, a});
            }
        }

        Color topHighlight = {255, 255, 255, 50};
        if (pf.type == PlatformType::SPRING) topHighlight = {180, 255, 200, 80};
        if (pf.type == PlatformType::ICE) topHighlight = {255, 255, 255, 90};
        DrawRectangle((int)pf.rect.x + 2, (int)pf.rect.y, (int)pf.rect.width - 4, 2, topHighlight);
    }

    Collectibles::DrawCoins(state.coins, state.animTime);
    Collectibles::DrawPowerUps(state.powerups, state.animTime);
    Particles::Draw(state.particles);

    if (state.playerTexture.id > 0) {
        Rectangle src{0, 0, (float)state.playerTexture.width, (float)state.playerTexture.height};
        float baseScale = state.playerSpriteScale;
        float dstW = state.player.width * baseScale;
        float dstH = state.player.height * baseScale;
        float padRatio = (state.playerTexture.height > 0)
                             ? (state.playerSpriteBottomPad / (float)state.playerTexture.height)
                             : 0.f;
        float vx = state.player.vel.x;
        float lean = vx / cfg.MAX_HSPEED;
        if (lean > 1) lean = 1;
        if (lean < -1) lean = -1;
        float leanDeg = lean * 8.f;
        float finalW = dstW, finalH = dstH;
        if (state.landingSquashActive) {
            state.landingSquashTime += dt;
            float t = state.landingSquashTime / state.landingSquashDuration;
            if (t > 1) {
                t = 1;
                state.landingSquashActive = false;
            }
            float e = 1.f - std::pow(1.f - t, 3.f);
            float squashAmt = 0.55f;
            finalW *= 1.f + squashAmt * (1.f - e);
            finalH *= 1.f - squashAmt * 0.65f * (1.f - e);
        }
        float dstX = state.player.pos.x + (state.player.width - finalW) / 2.f;
        float baseY =
            state.player.pos.y - (dstH - state.player.height) + state.playerSpriteYOffset + padRatio * dstH;
        float dstY = baseY - (finalH - dstH);
        Rectangle dst{dstX, dstY, finalW, finalH};
        if (vx < -10.f) src.width = -src.width;
        constexpr int MIN_COMBO = Const::COMBO_MIN_MULT;
        bool comboActive = (settings.comboEffects && state.comboCount >= MIN_COMBO &&
                            state.comboTimer > 0 && state.shaderFire.id > 0);
        if (comboActive) {
            float intensity = std::fmin(1.f, (float)(state.comboCount - 1) / 6.f);
            float pulse = (std::sin(state.animTime * 5.f) + 1.f) * 0.5f;
            float finalIntensity = (0.5f + 0.5f * pulse) * intensity;
            Vector2 sprSize{(float)state.playerTexture.width, (float)state.playerTexture.height};
            BeginBlendMode(BLEND_ADDITIVE);
            BeginShaderMode(state.shaderFire);
            if (state.fireLocTime >= 0)
                SetShaderValue(state.shaderFire, state.fireLocTime, &state.animTime, SHADER_UNIFORM_FLOAT);
            if (state.fireLocIntensity >= 0)
                SetShaderValue(state.shaderFire, state.fireLocIntensity, &finalIntensity, SHADER_UNIFORM_FLOAT);
            if (state.fireLocSpriteSize >= 0)
                SetShaderValue(state.shaderFire, state.fireLocSpriteSize, &sprSize, SHADER_UNIFORM_VEC2);
            if (state.fireLocMode >= 0) {
                int mode = 1;
                SetShaderValue(state.shaderFire, state.fireLocMode, &mode, SHADER_UNIFORM_INT);
            }
            Rectangle aura = dst;
            aura.x -= aura.width * Const::AURA_OFFSET_X;
            aura.y -= aura.height * Const::AURA_OFFSET_Y;
            aura.width *= Const::AURA_GROW_W;
            aura.height *= Const::AURA_GROW_H;
            DrawTexturePro(state.playerTexture, src, aura, {0, 0}, leanDeg, WHITE);
            EndShaderMode();
            EndBlendMode();
            BeginShaderMode(state.shaderFire);
            if (state.fireLocTime >= 0)
                SetShaderValue(state.shaderFire, state.fireLocTime, &state.animTime, SHADER_UNIFORM_FLOAT);
            if (state.fireLocIntensity >= 0)
                SetShaderValue(state.shaderFire, state.fireLocIntensity, &finalIntensity, SHADER_UNIFORM_FLOAT);
            if (state.fireLocSpriteSize >= 0)
                SetShaderValue(state.shaderFire, state.fireLocSpriteSize, &sprSize, SHADER_UNIFORM_VEC2);
            if (state.fireLocMode >= 0) {
                int mode = 0;
                SetShaderValue(state.shaderFire, state.fireLocMode, &mode, SHADER_UNIFORM_INT);
            }
            DrawTexturePro(state.playerTexture, src, dst, {0, 0}, leanDeg, WHITE);
            EndShaderMode();
        } else
            DrawTexturePro(state.playerTexture, src, dst, {0, 0}, leanDeg, WHITE);
    } else {
        DrawRectangle((int)state.player.pos.x, (int)state.player.pos.y, (int)state.player.width,
                       (int)state.player.height, blended.playerBody);
        DrawRectangleLines((int)state.player.pos.x, (int)state.player.pos.y, (int)state.player.width,
                           (int)state.player.height, {40, 40, 40, 255});
    }

    if (settings.powerUpEffects && state.doubleJumpEffectTimer > 0) {
        float t = 1.f - (state.doubleJumpEffectTimer / 0.3f);
        float radius = 15.f + t * 40.f;
        unsigned char alpha = (unsigned char)(200 * (1.f - t));
        float cx = state.player.pos.x + state.player.width / 2;
        float cy = state.player.pos.y + state.player.height / 2;
        DrawCircleLines((int)cx, (int)cy, radius, Color{150, 220, 255, alpha});
        DrawCircleLines((int)cx, (int)cy, radius * 0.7f, Color{200, 240, 255, (unsigned char)(alpha * 0.5f)});
    }

    if (state.wallSlidingLeft || state.wallSlidingRight) {
        float cx = state.player.pos.x + state.player.width / 2;
        float cy = state.player.pos.y + state.player.height;
        unsigned char sa = (unsigned char)(120 + 60 * std::sin(state.animTime * 12.f));
        for (int i = 0; i < 3; i++) {
            float yOff = (float)i * 12.f;
            float xOff = state.wallSlidingLeft ? 5.f : -5.f;
            float sz = 3.f + (float)i * 0.5f;
            DrawCircle((int)(cx + xOff), (int)(cy - yOff - 5.f), (int)sz,
                       Color{200, 220, 255, (unsigned char)(sa - i * 30)});
        }
    }

    EndMode2D();
    DrawVignette(cfg.gameWidth, cfg.gameHeight);
}

void Game::DrawBiomeEffects(int w, int h, float cameraY, float time) {
    int biome = state.currentTheme.biomeType;
    if (biome == BIOME_FOREST) {
        for (int i = 0; i < 15; i++) {
            float seed = (float)i * 7.3f;
            float fx = std::fmod(seed * 47.1f, (float)w);
            float baseY = std::fmod(cameraY * 0.03f + seed * 31.3f, (float)(h + 100)) - 50;
            float drift = std::sin(time * 0.8f + seed) * 30.f;
            float fy = baseY + std::sin(time * 0.5f + seed * 2.3f) * 20.f;
            unsigned char a = (unsigned char)(60 + 40 * std::sin(time * 2.f + seed));
            float sz = 2.f + std::sin(seed) * 1.f;
            DrawCircle((int)(fx + drift), (int)fy, sz, Color{180, 255, 120, a});
        }
    } else if (biome == BIOME_LAVA) {
        for (int i = 0; i < 20; i++) {
            float seed = (float)i * 5.7f;
            float fx = std::fmod(seed * 61.3f, (float)w);
            float rise = std::fmod(time * 40.f + seed * 100.f, (float)h);
            float fy = h - 30 - rise;
            float life = 1.f - rise / (float)h;
            if (life < 0.f) life = 0.f;
            unsigned char a = (unsigned char)(life * 120);
            float sz = 2.f + life * 3.f;
            DrawCircle((int)fx, (int)fy, sz, Color{255, (unsigned char)(120 + (int)(life * 80)), 20, a});
        }
    } else if (biome == BIOME_SNOW) {
        for (int i = 0; i < 30; i++) {
            float seed = (float)i * 3.9f;
            float fx = std::fmod(seed * 53.7f + std::sin(time * 0.3f + seed) * 50.f, (float)w);
            float fy = std::fmod(cameraY * 0.05f + seed * 41.3f + time * 25.f + seed * 10.f, (float)(h + 50)) - 25;
            float sz = 1.5f + std::fmod(seed, 3.f);
            unsigned char a = (unsigned char)(100 + 80 * std::sin(time + seed));
            DrawCircle((int)fx, (int)fy, sz, Color{230, 240, 255, a});
        }
    } else if (biome == BIOME_COSMIC) {
        for (int i = 0; i < 12; i++) {
            float seed = (float)i * 8.1f;
            float cx = std::fmod(seed * 43.7f, (float)w);
            float cy = std::fmod(cameraY * 0.04f + seed * 29.3f, (float)h);
            float rot = time * 0.5f + seed;
            float sz = 15.f + std::sin(seed) * 10.f;
            unsigned char a = (unsigned char)(30 + 20 * std::sin(time * 1.5f + seed));
            for (int j = 0; j < 4; j++) {
                float angle = rot + j * PI / 2.f;
                float len = sz * (0.7f + 0.3f * std::sin(time * 2.f + j + seed));
                DrawLine((int)cx, (int)cy, (int)(cx + std::cos(angle) * len), (int)(cy + std::sin(angle) * len), Color{200, 150, 255, a});
            }
        }
    } else if (biome == BIOME_NEON) {
        for (int i = 0; i < 8; i++) {
            float seed = (float)i * 6.3f;
            float x1 = std::fmod(seed * 37.1f, (float)w);
            float y1 = std::fmod(cameraY * 0.03f + seed * 19.7f, (float)h);
            float x2 = x1 + std::sin(time * 0.7f + seed) * 60.f;
            float y2 = y1 + std::cos(time * 0.5f + seed * 1.3f) * 40.f;
            unsigned char a = (unsigned char)(40 + 30 * std::sin(time * 3.f + seed));
            Color neonCol = (i % 3 == 0) ? Color{0, 255, 180, a} :
                            (i % 3 == 1) ? Color{255, 0, 150, a} : Color{0, 180, 255, a};
            DrawLine((int)x1, (int)y1, (int)x2, (int)y2, neonCol);
        }
    } else if (biome == BIOME_DESERT) {
        for (int i = 0; i < 10; i++) {
            float seed = (float)i * 9.1f;
            float fx = std::fmod(seed * 41.3f + time * 15.f, (float)(w + 40)) - 20;
            float fy = std::fmod(cameraY * 0.06f + seed * 23.1f, (float)h);
            unsigned char a = (unsigned char)(50 + 30 * std::sin(time + seed));
            DrawCircle((int)fx, (int)fy, 4, Color{220, 190, 120, a});
        }
    }
}

void Game::DrawBiomeBackground(int w, int h, float cameraY, float time, int biome, Color tint) {
    unsigned char ta = tint.a;
    if (ta < 5) return;

    auto tintC = [=](Color c) -> Color {
        return {
            (unsigned char)(c.r * tint.r / 255),
            (unsigned char)(c.g * tint.g / 255),
            (unsigned char)(c.b * tint.b / 255),
            (unsigned char)(c.a * ta / 255)
        };
    };

    if (biome == BIOME_FOREST || biome == 0) {
        // Upgrade: More parallax layers, denser forest, fireflies
        for (int layer = 0; layer < 3; layer++) {
            float parallax = 0.02f + layer * 0.035f;
            float scroll = cameraY * parallax;
            float treeSpacing = 120.f - layer * 30.f;
            int treeCount = (int)(w / treeSpacing) + 4;
            float darkFactor = 0.3f + layer * 0.25f;
            
            Color trunkCol = tintC({(unsigned char)(40 * darkFactor), (unsigned char)(25 * darkFactor), (unsigned char)(15 * darkFactor), (unsigned char)(100 + layer * 40)});
            Color leafCol1 = tintC({(unsigned char)(20 * darkFactor), (unsigned char)(80 * darkFactor), (unsigned char)(30 * darkFactor), (unsigned char)(90 + layer * 40)});
            Color leafCol2 = tintC({(unsigned char)(10 * darkFactor), (unsigned char)(50 * darkFactor), (unsigned char)(20 * darkFactor), (unsigned char)(90 + layer * 40)});
            
            for (int i = 0; i < treeCount; i++) {
                float seed = (float)(i + layer * 100);
                float tx = i * treeSpacing + std::sin(seed * 3.7f) * 30.f;
                float treeH = 150.f + std::sin(seed * 2.1f) * 80.f + layer * 50.f;
                float ty = h - treeH + scroll + std::fmod(seed * 47.3f, 250.f) - 50.f;
                
                while (ty > h + treeH) ty -= h + treeH + 100.f;
                while (ty < -treeH - 100.f) ty += h + treeH + 100.f;
                
                float trunkW = 8.f + layer * 4.f + std::sin(seed)*2.f;
                float trunkDrawH = (h + 300.f) - ty;
                DrawRectangleGradientV((int)(tx - trunkW / 2), (int)ty, (int)trunkW, (int)trunkDrawH, trunkCol, tintC({(unsigned char)(trunkCol.r/2), (unsigned char)(trunkCol.g/2), (unsigned char)(trunkCol.b/2), trunkCol.a}));
                
                float leafW = 50.f + layer * 20.f + std::sin(seed) * 15.f;
                float leafH = treeH * 0.8f;
                int sections = 4 + (int)(std::fabs(std::sin(seed*1.1f)) * 3);
                for(int s=0; s<sections; s++) {
                    float sPct = (float)s / (sections - 1.0f);
                    float sY = ty + leafH * 0.1f + sPct * (leafH * 0.7f); // from top to bottom
                    float sW = leafW * (0.3f + sPct * 0.7f); // narrow at top, wide at bottom
                    DrawTriangle({tx, sY - leafH*0.3f}, 
                                 {tx - sW / 2, sY + leafH * 0.2f}, 
                                 {tx + sW / 2, sY + leafH * 0.2f}, 
                                 (s%2==0) ? leafCol1 : leafCol2);
                }
            }
            
            if (layer == 2) {
                // Fireflies in foreground
                int fireflyCount = 15;
                for(int f=0; f<fireflyCount; f++) {
                    float fSeed = f * 11.3f;
                    float fx = std::fmod(fSeed * 37.1f + time * 10.f * std::sin(fSeed), (float)w);
                    float fy = std::fmod(h - 50 + scroll*1.5f + std::sin(time*2.f + fSeed)*20.f + fSeed * 19.3f, (float)h);
                    unsigned char fa = (unsigned char)(100 + 150 * std::sin(time * 3.f + fSeed));
                    DrawCircle((int)fx, (int)fy, 2 + std::sin(fSeed), tintC({200, 255, 100, fa}));
                }
            }
        }
    }

    if (biome == BIOME_DESERT) {
        // Upgrade: Gradients for dunes, a large sun, heat haze
        float scroll = cameraY * 0.03f;
        
        // Draw the Sun
        float sunY = h*0.4f + scroll*0.2f;
        DrawCircleGradient(w*0.7f, sunY, 80.f, tintC({255, 220, 100, 200}), tintC({255, 150, 50, 0}));
        
        int duneCount = 6;
        for (int d = 0; d < duneCount; d++) {
            float seed = (float)d * 5.3f;
            float dx = seed * 73.1f;
            float baseY = h - 30 - d * 35.f + scroll * (0.3f + d * 0.15f);
            while (baseY > h + 150) baseY -= h + 250;
            while (baseY < -150) baseY += h + 250;
            
            float dw = 300.f + std::sin(seed) * 120.f;
            float dh = 60.f + d * 20.f;
            
            Color duneTop = tintC({(unsigned char)(220 + d*5), (unsigned char)(180 + d*5), (unsigned char)(100 + d*5), (unsigned char)(100 + d*25)});
            Color duneBot = tintC({(unsigned char)(160 + d*10), (unsigned char)(100 + d*10), (unsigned char)(40 + d*5), (unsigned char)(80 + d*20)});
            
            // Heat wave simulation (vertical offset)
            float heat = std::sin(time * 3.f + dx * 0.05f) * 3.f;
            float plateauTop = baseY - dh*0.2f + heat;
            DrawEllipse((int)(dx + dw / 2), (int)plateauTop, (int)(dw / 2), (int)(dh*0.8f), duneTop);
            DrawRectangle((int)dx, (int)plateauTop, (int)dw, (int)(h + 300.f - plateauTop), duneBot);
        }
        
        float cactScroll = cameraY * 0.05f;
        for (int i = 0; i < 6; i++) {
            float seed = (float)i * 11.7f;
            float cx = std::fmod(seed * 31.1f, (float)w);
            float cy = h - 80 + cactScroll + std::fmod(seed * 41.3f, 150.f);
            while (cy > h + 100) cy -= h + 200;
            while (cy < -100) cy += h + 200;
            
            Color cactCol1 = tintC({60, 130, 50, 150});
            Color cactCol2 = tintC({30, 80, 20, 150});
            
            float cactusDrawH = (h + 300.f) - cy;
            // Main stem
            DrawRectangleGradientH((int)cx, (int)cy, 12, (int)cactusDrawH, cactCol1, cactCol2);
            DrawCircle((int)cx + 6, (int)cy, 6, cactCol1);
            // Left arm
            DrawRectangleGradientH((int)(cx - 14), (int)(cy + 15), 14, 8, cactCol1, cactCol2);
            DrawRectangleGradientH((int)(cx - 14), (int)(cy + 5), 8, 18, cactCol1, cactCol2);
            DrawCircle((int)(cx - 10), (int)(cy + 5), 4, cactCol1);
            // Right arm
            DrawRectangleGradientH((int)(cx + 12), (int)(cy + 25), 16, 8, cactCol1, cactCol2);
            DrawRectangleGradientH((int)(cx + 20), (int)(cy + 15), 8, 18, cactCol1, cactCol2);
            DrawCircle((int)(cx + 24), (int)(cy + 15), 4, cactCol1);
        }
    }

    if (biome == BIOME_LAVA) {
        // Upgrade: More menacing mountains, animated lava falls, glowing ash particles
        float scroll = cameraY * 0.03f;
        for (int layer = 0; layer < 3; layer++) {
            Color mtnCol1 = tintC({(unsigned char)(40 + layer * 20), (unsigned char)(15 + layer * 5), (unsigned char)(10 + layer * 5), (unsigned char)(100 + layer * 30)});
            Color mtnCol2 = tintC({(unsigned char)(20 + layer * 10), (unsigned char)(5 + layer * 2), (unsigned char)(5 + layer * 2), (unsigned char)(100 + layer * 30)});
            int peaks = 5 + layer * 2;
            for (int i = 0; i < peaks; i++) {
                float seed = (float)(i + layer * 50);
                float px = seed * (w / (float)peaks) + std::sin(seed * 2.3f) * 40.f;
                float py = h - 40 + scroll * (0.2f + layer * 0.15f) + std::fmod(seed * 37.1f, 250.f);
                while (py > h + 250) py -= h + 350;
                while (py < -200) py += h + 350;
                
                float mw = 100.f + layer * 35.f + std::sin(seed) * 40.f;
                float mh = 140.f + layer * 50.f + std::cos(seed * 1.7f) * 60.f;
                
                // Draw mountain 
                DrawTriangle({px, py - mh}, {px - mw, py}, {px + mw, py}, mtnCol1);
                DrawTriangle({px, py - mh}, {px, py}, {px + mw, py}, mtnCol2); // fake shading
                
                // Extend mountain to bottom
                DrawRectangle((int)(px - mw), (int)py, (int)mw, (int)(h + 300.f - py), mtnCol1);
                DrawRectangle((int)px, (int)py, (int)mw, (int)(h + 300.f - py), mtnCol2);
                
                if (layer == 0) {
                    // Lava streaming down from the tip
                    float lavaW = mw * 0.15f + std::sin(time*2.0f + seed)*3.f;
                    Color lavaCol = tintC({255, (unsigned char)(100 + 40*std::sin(time*4.f + seed)), 20, (unsigned char)(180 + 50 * std::sin(time * 3.f + seed))});
                    DrawTriangle({px, py - mh + 10}, {px - lavaW*0.5f, py - mh*0.4f}, {px + lavaW*0.5f, py - mh*0.4f}, lavaCol);
                    
                    Color lavaGlow = tintC({255, 60, 0, (unsigned char)(80 + 40 * std::sin(time * 2.f + seed))});
                    DrawCircleGradient((int)px, (int)(py - mh + 10), 30.f, lavaGlow, tintC({0,0,0,0}));
                }
            }
        }
        
        // Embers / Ash
        for(int p = 0; p < 20; p++) {
            float pSeed = p * 13.5f;
            float px = std::fmod(pSeed * 29.3f + time * 15.f * std::sin(pSeed), (float)w);
            float py = std::fmod(h + scroll * 1.2f - time * (30.f + std::fmod(pSeed, 40.f)), (float)h);
            unsigned char pa = (unsigned char)(100 + 100 * std::sin(time * 5.f + pSeed));
            DrawCircle((int)px, (int)py, 2 + std::fmod(pSeed, 2.f), tintC({255, 120, 30, pa}));
        }
    }

    if (biome == BIOME_SNOW) {
        // Upgrade: Snowflakes, deeper parallax mountains, nicer snow caps
        float scroll = cameraY * 0.03f;
        for (int layer = 0; layer < 3; layer++) {
            Color mtnCol1 = tintC({(unsigned char)(50 + layer * 30), (unsigned char)(70 + layer * 35), (unsigned char)(100 + layer * 45), (unsigned char)(80 + layer * 40)});
            Color mtnCol2 = tintC({(unsigned char)(40 + layer * 25), (unsigned char)(60 + layer * 30), (unsigned char)(90 + layer * 40), (unsigned char)(80 + layer * 40)});
            Color snowCol = tintC({220, 240, 255, (unsigned char)(100 + layer * 40)});
            int peaks = 4 + layer;
            for (int i = 0; i < peaks; i++) {
                float seed = (float)(i + layer * 30);
                float px = i * (w / (float)peaks) + std::sin(seed * 3.1f) * 50.f;
                float py = h - 20 + scroll * (0.2f + layer * 0.15f) + std::fmod(seed * 29.7f, 250.f);
                while (py > h + 250) py -= h + 350;
                while (py < -250) py += h + 350;
                
                float mw = 120.f + layer * 40.f;
                float mh = 180.f + layer * 40.f + std::sin(seed * 1.3f) * 40.f;
                
                DrawTriangle({px, py - mh}, {px - mw, py}, {px + mw, py}, mtnCol1);
                DrawTriangle({px, py - mh}, {px, py}, {px + mw, py}, mtnCol2);
                
                // Extend mountain to bottom
                DrawRectangle((int)(px - mw), (int)py, (int)mw, (int)(h + 300.f - py), mtnCol1);
                DrawRectangle((int)px, (int)py, (int)mw, (int)(h + 300.f - py), mtnCol2);
                
                // Jagged snow cap
                float snowW = mw * 0.5f;
                float snowH = mh * 0.4f;
                DrawTriangle({px, py - mh}, {px - snowW, py - mh + snowH}, {px + snowW, py - mh + snowH}, snowCol);
                DrawTriangle({px, py - mh + snowH*1.2f}, {px - snowW*0.6f, py - mh + snowH}, {px + snowW*0.6f, py - mh + snowH}, snowCol);
            }
        }
        
        // Snowfall
        for(int s = 0; s < 30; s++) {
            float sSeed = s * 7.5f;
            float sx = std::fmod(sSeed * 47.1f + time * 10.f * std::sin(sSeed), (float)w);
            float sy = std::fmod(h + scroll * 1.5f + time * (40.f + std::fmod(sSeed, 30.f)), (float)h);
            DrawCircle((int)sx, (int)sy, 2 + std::fmod(sSeed, 2.f), tintC({255, 255, 255, (unsigned char)(100 + 50*std::sin(time+sSeed))}));
        }
    }

    if (biome == BIOME_COSMIC) {
        // Upgrade: Gradients for planets, shooting stars, better nebula blending
        float scroll = cameraY * 0.02f;
        
        // Stars
        for (int i = 0; i < 40; i++) {
            float seed = i * 2.1f;
            float sx = std::fmod(seed * 83.1f, (float)w);
            float sy = std::fmod(scroll * 0.5f + seed * 53.7f, (float)h);
            unsigned char sa = (unsigned char)(100 + 100 * std::sin(time * 2.f + seed));
            DrawRectangle((int)sx, (int)sy, 2, 2, tintC({255, 255, 255, sa}));
        }
        
        // Planets
        for (int i = 0; i < 4; i++) {
            float seed = (float)i * 9.3f;
            float px = std::fmod(seed * 57.1f, (float)w);
            float py = std::fmod(scroll + seed * 43.7f, (float)(h + 300)) - 150;
            float r = 30.f + std::sin(seed * 2.1f) * 20.f;
            
            Color pCol1 = tintC({(unsigned char)(100 + (int)(std::sin(seed) * 80)), (unsigned char)(50 + (int)(std::cos(seed * 1.3f) * 40)), (unsigned char)(150 + (int)(std::sin(seed * 0.7f) * 60)), 180});
            Color pCol2 = tintC({(unsigned char)(40 + (int)(std::sin(seed) * 30)), (unsigned char)(20 + (int)(std::cos(seed * 1.3f) * 20)), (unsigned char)(60 + (int)(std::sin(seed * 0.7f) * 30)), 180});
            
            DrawCircleGradient((int)px, (int)py, r, pCol1, pCol2);
            DrawCircle((int)(px - r*0.3f), (int)(py - r*0.3f), r*0.2f, tintC({255,255,255,30})); // reflection
            
            // Rings
            if (i % 2 == 0) {
                Color ringCol = tintC({200, 180, 255, 80});
                DrawEllipseLines((int)px, (int)py, (int)(r * 2.2f), (int)(r * 0.6f), ringCol);
                DrawEllipseLines((int)px, (int)py, (int)(r * 2.0f), (int)(r * 0.5f), tintC({200, 180, 255, 40}));
            }
        }
        
        // Nebula
        for (int i = 0; i < 3; i++) {
            float seed = (float)i * 13.7f;
            float nx = std::fmod(seed * 31.3f, (float)w);
            float ny = std::fmod(scroll * 1.5f + seed * 67.1f, (float)(h + 200)) - 100;
            float nw = 100.f + std::sin(seed) * 50.f;
            
            Color nebCol = tintC({(unsigned char)(150 + i * 50), (unsigned char)(60 + i * 40), (unsigned char)(200 - i * 30), 10});
            for (int j = 0; j < 6; j++) {
                float offX = std::sin(j * 1.5f + time * 0.2f) * nw * 0.5f;
                float offY = std::cos(j * 2.1f + time * 0.15f) * nw * 0.4f;
                DrawCircleGradient((int)(nx + offX), (int)(ny + offY), nw * 0.6f + j * 10.f, nebCol, tintC({0,0,0,0}));
            }
        }
    }

    if (biome == BIOME_NEON) {
        // Upgrade: Better grid, glowing sun in the back, more detailed buildings
        float scroll = cameraY * 0.04f;
        
        // Background Sun
        float sunY = h*0.5f + scroll*0.1f;
        Color sunCol1 = tintC({255, 50, 150, 200});
        Color sunCol2 = tintC({100, 0, 200, 0});
        DrawCircleGradient(w*0.5f, sunY, 150.f, sunCol1, sunCol2);
        for(int l=0; l<5; l++) {
            DrawRectangle(w*0.5f - 150, sunY + 20 + l*20, 300, 5 + l*2, tintC({0,0,0, 100})); // Retrowave lines
        }
        
        int gridSpacing = 50;
        Color gridCol = tintC({0, (unsigned char)(100 + 50 * std::sin(time * 1.5f)), (unsigned char)(100 + 40 * std::sin(time * 1.3f)), 40});
        float gridOff = std::fmod(scroll, (float)gridSpacing);
        for (int gy = (int)(-gridSpacing + gridOff); gy < h + gridSpacing; gy += gridSpacing) {
            DrawLine(0, gy, w, gy, gridCol);
            DrawLine(0, gy+1, w, gy+1, tintC({0,0,0,50})); // Fake shadow
        }
        for (int gx = 0; gx < w; gx += gridSpacing) {
            DrawLine(gx, 0, gx, h, gridCol);
        }
        
        for (int i = 0; i < 8; i++) {
            float seed = (float)i * 7.1f;
            float bx = std::fmod(seed * 41.3f, (float)w);
            float by = h - 20 + scroll * 0.8f + std::fmod(seed * 53.7f, (float)h);
            while (by > h + 150) by -= h + 250;
            while (by < -150) by += h + 250;
            float bw = 25.f + std::sin(seed * 2.3f) * 20.f;
            int floors = 4 + (int)std::fabs(std::sin(seed)) * 5;
            
            Color bldCol1 = tintC({(unsigned char)(20 + i * 5), (unsigned char)(10 + i * 5), (unsigned char)(40 + i * 8), 120});
            Color bldCol2 = tintC({(unsigned char)(5 + i * 2), (unsigned char)(5 + i * 2), (unsigned char)(15 + i * 4), 160});
            Color winCol = tintC({0, 255, (unsigned char)(150 + i * 30), (unsigned char)(60 + 30 * std::sin(time * 2.f + seed))});
            
            float totalH = floors * 30.f;
            float bTop = by - totalH;
            float drawH = (h + 300.f) - bTop;
            DrawRectangleGradientV((int)bx, (int)bTop, (int)bw, (int)drawH, bldCol1, bldCol2);
            DrawLine((int)bx, (int)bTop, (int)(bx+bw), (int)bTop, winCol); // Roof glow
            
            int drawFloors = (int)(drawH / 30.f);
            for (int f = 0; f < drawFloors; f++) {
                if ((int)(seed + f) % 3 != 0) {
                    float wy = bTop + f * 30.f + 10.f;
                    DrawRectangle((int)(bx + 5), (int)wy, (int)(bw - 10), 8, winCol);
                    DrawRectangle((int)(bx + 5), (int)wy+8, (int)(bw - 10), 2, tintC({0,255,255,100}));
                }
            }
        }
    }

    if (biome == BIOME_DEFAULT) {
        // Upgrade: Puffy procedural clouds with gradients
        float scroll = cameraY * 0.03f;
        for (int i = 0; i < 8; i++) {
            float seed = (float)i * 8.3f;
            float cx = std::fmod(seed * 47.1f + time*(5.f+std::sin(seed)), (float)(w+200)) - 100;
            float cy = std::fmod(scroll * 0.5f + seed * 31.3f, (float)(h + 200)) - 100;
            float cw = 80.f + std::sin(seed * 1.7f) * 40.f;
            float ch = 30.f + std::cos(seed) * 15.f;
            
            Color cloudCol1 = tintC({255, 255, 255, 60});
            Color cloudCol2 = tintC({200, 210, 230, 40});
            
            DrawEllipse((int)cx, (int)cy, cw*0.6f, ch*0.6f, cloudCol1);
            DrawEllipse((int)(cx + cw * 0.3f), (int)(cy - ch * 0.2f), cw * 0.4f, ch * 0.5f, cloudCol1);
            DrawEllipse((int)(cx - cw * 0.2f), (int)(cy - ch * 0.1f), cw * 0.35f, ch * 0.4f, cloudCol2);
        }
    }

    if (biome == BIOME_NIGHT) {
        // Upgrade: Detailed moon, stars, more dense and gradient buildings
        // Stars
        for (int i = 0; i < 30; i++) {
            float seed = i * 4.1f;
            float sx = std::fmod(seed * 63.1f, (float)w);
            float sy = std::fmod(cameraY * 0.01f + seed * 33.7f, (float)h*0.7f); // mostly top half
            unsigned char sa = (unsigned char)(80 + 80 * std::sin(time * 1.5f + seed));
            DrawRectangle((int)sx, (int)sy, 2, 2, tintC({255, 255, 255, sa}));
        }
        
        float moonX = w * 0.8f;
        float moonY = 80.f + cameraY * 0.015f;
        DrawCircleGradient((int)moonX, (int)moonY, 60.f, tintC({240, 235, 200, 40}), tintC({200, 200, 180, 0}));
        DrawCircle((int)moonX, (int)moonY, 25, tintC({240, 235, 200, 100}));
        DrawCircle((int)(moonX - 8), (int)(moonY - 5), 18, tintC({220, 215, 180, 80})); // craters
        DrawCircle((int)(moonX + 6), (int)(moonY + 8), 10, tintC({220, 215, 180, 80}));
        
        float scroll = cameraY * 0.04f;
        int bldCount = 12;
        for (int i = 0; i < bldCount; i++) {
            float seed = (float)i * 6.1f;
            float bx = i * (w / (float)bldCount) + std::sin(seed * 3.7f) * 20.f;
            float by = h + scroll + std::fmod(seed * 41.3f, (float)h);
            while (by > h + 150) by -= h + 250;
            while (by < -200) by += h + 250;
            
            float bw = 35.f + std::sin(seed * 2.1f) * 20.f;
            int floors = 5 + (int)(std::fabs(std::sin(seed * 1.3f)) * 10.f);
            float totalH = floors * 20.f;
            float bTop = by - totalH;
            float drawH = (h + 300.f) - bTop;
            
            Color bldCol1 = tintC({20, 25, 45, 120});
            Color bldCol2 = tintC({10, 12, 25, 180});
            DrawRectangleGradientV((int)bx, (int)bTop, (int)bw, (int)drawH, bldCol1, bldCol2);
            
            Color winOn = tintC({255, 230, 150, (unsigned char)(80 + 30 * std::sin(time * 1.0f + seed))});
            Color winOff = tintC({30, 35, 55, 80});
            
            int windowsPerFloor = (int)(bw / 12.f);
            int drawFloors = (int)(drawH / 20.f);
            for (int f = 0; f < drawFloors; f++) {
                for (int wx = 0; wx < windowsPerFloor; wx++) {
                    float wy = bTop + f * 20.f + 5.f;
                    float wxx = bx + 4.f + wx * 10.f;
                    bool lit = ((int)(seed * 10.f + f * 3.f + wx * 7.f) % 4) != 0;
                    DrawRectangle((int)wxx, (int)wy, 6, 10, lit ? winOn : winOff);
                }
            }
        }
    }

    if (biome == BIOME_MONO) {
        float scroll = cameraY * 0.025f;
        int shapeCount = 10;
        for (int i = 0; i < shapeCount; i++) {
            float seed = (float)i * 4.7f;
            float sx = std::fmod(seed * 37.1f, (float)w);
            float sy = std::fmod(scroll + seed * 53.3f, (float)(h + 200)) - 100;
            float sz = 20.f + std::sin(seed * 1.9f) * 15.f;
            unsigned char a = (unsigned char)(25 + 15 * std::sin(time * 0.8f + seed));
            Color shapeCol = tintC({160, 160, 160, a});
            int type = (int)(seed * 2.3f) % 3;
            float rot = time * 0.2f + seed;
            if (type == 0) {
                DrawCircle((int)sx, (int)sy, (int)(sz * 0.6f), shapeCol);
                DrawCircleLines((int)sx, (int)sy, (int)(sz * 0.6f), tintC({180, 180, 180, (unsigned char)(a + 10)}));
            } else if (type == 1) {
                float hw = sz * 0.5f;
                float rot2 = rot + PI / 4.f;
                float cx = sx + std::cos(rot2) * 0.f;
                float cy = sy + std::sin(rot2) * 0.f;
                Vector2 corners[4];
                for (int c = 0; c < 4; c++) {
                    float ang = rot2 + c * PI / 2.f;
                    corners[c] = {cx + std::cos(ang) * hw, cy + std::sin(ang) * hw};
                }
                for (int c = 0; c < 4; c++) {
                    DrawLineV(corners[c], corners[(c + 1) % 4], shapeCol);
                }
            } else {
                DrawTriangle({sx, sy - sz * 0.5f},
                             {sx - sz * 0.4f, sy + sz * 0.3f},
                             {sx + sz * 0.4f, sy + sz * 0.3f}, shapeCol);
            }
        }
        for (int i = 0; i < 3; i++) {
            float seed = (float)i * 11.1f;
            float lx1 = std::fmod(seed * 29.3f, (float)w);
            float ly1 = std::fmod(scroll * 1.5f + seed * 41.7f, (float)h);
            float lx2 = lx1 + std::sin(time * 0.3f + seed) * 80.f;
            float ly2 = ly1 + std::cos(time * 0.2f + seed * 1.3f) * 60.f;
            Color lineCol = tintC({120, 120, 120, 18});
            DrawLine((int)lx1, (int)ly1, (int)lx2, (int)ly2, lineCol);
            DrawCircle((int)lx1, (int)ly1, 3, lineCol);
            DrawCircle((int)lx2, (int)ly2, 3, lineCol);
        }
    }
}

void Game::DrawHud(float dt) {
    if (state.score > state.highScore) state.highScore = state.score;
    if (state.currentScreen == GameState::Screen::GAMEOVER) return;
    DrawText(TextFormat("Score: %d  Best: %d", state.score, state.highScore), 10,
             Const::HUD_TOP_MARGIN, Const::HUD_SCORE_FONT, RAYWHITE);
    constexpr int MIN_COMBO = Const::COMBO_MIN_MULT;
    unsigned char a = (state.comboTimer > 0) ? 255 : 70;
    Color col = (state.comboCount >= MIN_COMBO && state.comboTimer > 0) ? Color{255, 200, 100, a}
                                                                        : Color{160, 160, 160, a};
    DrawText(TextFormat("Combo x%d", state.comboCount), 10, 40, Const::HUD_COMBO_FONT, col);

    float radius = Const::HUD_CLOCK_RADIUS;
    float clockX = cfg.gameWidth - radius - 20.f;
    float clockY = radius + 20.f;
    DrawCircleLines((int)clockX, (int)clockY, radius, RAYWHITE);
    int segmentsDone = state.speedStage;
    for (int s = 0; s < segmentsDone && s < 5; ++s) {
        float a0 = -PI / 2 + (2 * PI / 5) * s;
        float a1 = -PI / 2 + (2 * PI / 5) * (s + 1);
        Vector2 p0{clockX + std::cos(a0) * radius * 0.9f, clockY + std::sin(a0) * radius * 0.9f};
        Vector2 p1{clockX + std::cos(a1) * radius * 0.9f, clockY + std::sin(a1) * radius * 0.9f};
        DrawLineEx({clockX, clockY}, p0, 2.f, {180, 180, 255, 200});
        DrawLineEx({clockX, clockY}, p1, 2.f, {180, 180, 255, 200});
    }
    float phase = (state.speedStage < 5) ? (state.stageTimer / cfg.STAGE_DURATION) : 0.f;
    float angle = (state.speedStage < 5) ? (-PI / 2 + phase * 2 * PI) : (-PI / 2 - GetTime() * 5.f);
    Vector2 hand{clockX + std::cos(angle) * radius * 0.85f, clockY + std::sin(angle) * radius * 0.85f};
    DrawLineEx({clockX, clockY}, hand, 3.f,
               (state.speedStage < 5) ? Color{255, 220, 120, 255} : Color{255, 80, 80, 255});
    DrawCircle((int)clockX, (int)clockY, 3,
               (state.speedStage < 5) ? RAYWHITE : Color{255, 80, 80, 255});

    int coinY = (int)(clockY + radius + 15);
    DrawCircle(cfg.gameWidth - 45, coinY, 8, GOLD);
    DrawText(TextFormat("%d", state.globalCoins), cfg.gameWidth - 30, coinY - 10, 20, GOLD);

    if (settings.showFPS) {
        DrawText(TextFormat("FPS: %d", GetFPS()), cfg.gameWidth - 70, coinY + 15, 16, {255, 255, 255, 180});
    }

    if (state.isDailyRun) {
        const char *challengeName = GetChallengeName(state.dailyChallenge.type);
        int cw = MeasureText(challengeName, 14);
        int y = coinY + (settings.showFPS ? 35 : 15);
        int x = cfg.gameWidth - cw - 12;
        DrawRectangle(x - 8, y, cw + 16, 22, Color{60, 40, 100, 200});
        DrawText(challengeName, x, y + 4, 14, Color{255, 180, 80, 255});
    }

    int powerUpY = 70;
    if (state.activeDoubleJump && state.powerUpTimers[0] > 0) {
        float pct = state.powerUpTimers[0] / 10.0f;
        DrawRectangle(10, powerUpY, (int)(100 * pct), 12, Color{100, 200, 255, 200});
        DrawText("2x Jump", 15, powerUpY - 2, 14, WHITE);
        powerUpY += 18;
    }
    if (state.activeShield && state.powerUpTimers[1] > 0) {
        float pct = state.powerUpTimers[1] / 10.0f;
        DrawRectangle(10, powerUpY, (int)(100 * pct), 12, Color{100, 255, 150, 200});
        DrawText("Shield", 15, powerUpY - 2, 14, WHITE);
        powerUpY += 18;
    }
    if (state.activeSlowMotion && state.powerUpTimers[2] > 0) {
        float pct = state.powerUpTimers[2] / 10.0f;
        DrawRectangle(10, powerUpY, (int)(100 * pct), 12, Color{200, 150, 255, 200});
        DrawText("Slow", 15, powerUpY - 2, 14, WHITE);
        powerUpY += 18;
    }
    if (state.activeMagnet && state.powerUpTimers[3] > 0) {
        float pct = state.powerUpTimers[3] / 10.0f;
        DrawRectangle(10, powerUpY, (int)(100 * pct), 12, Color{255, 200, 100, 200});
        DrawText("Magnet", 15, powerUpY - 2, 14, WHITE);
    }
    if (state.themeChangeTimer > 0) {
        state.themeChangeTimer -= dt;
        float alpha = state.themeChangeTimer / 3.f;
        if (alpha < 0) alpha = 0;
        if (alpha > 1) alpha = 1;
        int a2 = (int)(alpha * 255);
        const char *name = state.currentTheme.name;
        int w = MeasureText(name, Const::HUD_THEME_FONT);
        DrawText(name, cfg.gameWidth / 2 - w / 2, 80, Const::HUD_THEME_FONT,
                 {255, 255, 255, (unsigned char)a2});
    }

    if (state.achievementPopupTimer > 0 && !state.lastUnlockedAchievement.empty()) {
        float t = state.achievementPopupTimer / 3.f;
        float slide = 1.f;
        if (t > 0.85f) slide = (1.f - t) / 0.15f;
        else if (t < 0.2f) slide = t / 0.2f;
        if (slide > 1.f) slide = 1.f;
        if (slide < 0.f) slide = 0.f;

        int popupW = 200, popupH = 36;
        int popupX = 8;
        int popupTargetY = cfg.gameHeight - popupH - 10;
        int popupY = (int)(popupTargetY + (1.f - slide) * 50.f);
        unsigned char popupAlpha = (unsigned char)(slide * 220);

        DrawRectangle(popupX, popupY, popupW, popupH, {20, 25, 50, popupAlpha});
        DrawRectangleLines(popupX, popupY, popupW, popupH, {255, 200, 80, popupAlpha});

        DrawText("*", popupX + 6, popupY + 4, 11, {255, 200, 80, popupAlpha});
        DrawText(state.lastUnlockedAchievement.c_str(), popupX + 18, popupY + 10, 14,
                 {255, 255, 255, popupAlpha});
    }
}

void Game::DrawGameOverOverlay() {
    if (state.currentScreen != GameState::Screen::GAMEOVER) return;
    for (int y = 0; y < cfg.gameHeight; y += Const::GRADIENT_STEP) {
        float k = (float)y / cfg.gameHeight;
        unsigned char a = (unsigned char)(160 + 60 * k);
        DrawRectangle(0, y, cfg.gameWidth, Const::GRADIENT_STEP, {10, 12, 20, a});
    }

    int buttons = 3;

    int w = Const::GAMEOVER_PANEL_WIDTH;
    int yTop = Const::GAMEOVER_TOP;
    int bh = Const::GAMEOVER_BUTTON_H, spacing = Const::GAMEOVER_BUTTON_GAP;
    int yButtonsTop = yTop + 125;
    int h = (yButtonsTop - yTop) + buttons * bh + (buttons - 1) * spacing + 20;
    int x = cfg.gameWidth / 2 - w / 2;
    DrawRectangle(x, yTop, w, h, {25, 28, 42, 240});
    DrawRectangleLines(x, yTop, w, h, {180, 200, 255, 180});
    const char *title = Loc::GameOver_Title();
    int tw = MeasureText(title, Const::GAMEOVER_TITLE_FONT);
    DrawText(title, cfg.gameWidth / 2 - tw / 2, yTop + 15, Const::GAMEOVER_TITLE_FONT, RAYWHITE);

    if (state.isDailyRun) {
        const char *challengeName = GetChallengeName(state.dailyChallenge.type);
        int cnw = MeasureText(challengeName, 12);
        DrawText(challengeName, cfg.gameWidth / 2 - cnw / 2, yTop + 42, 12, Color{255, 180, 80, 255});
    }

    const char *scoreTxt = TextFormat("%s %d", Loc::GameOver_Score(), state.score);
    int sw = MeasureText(scoreTxt, Const::GAMEOVER_SCORE_FONT);
    DrawText(scoreTxt, cfg.gameWidth / 2 - sw / 2, yTop + 60, Const::GAMEOVER_SCORE_FONT,
             {255, 220, 140, 255});

    int bestScore = state.isDailyRun ? state.dailyChallenge.bestScore : state.highScore;
    const char *bestLabel = state.isDailyRun ? Loc::Daily_Best() : Loc::GameOver_Best();
    const char *bestTxt = TextFormat("%s %d", bestLabel, bestScore);
    int bw = MeasureText(bestTxt, Const::GAMEOVER_BEST_FONT);
    DrawText(bestTxt, cfg.gameWidth / 2 - bw / 2, yTop + 85, Const::GAMEOVER_BEST_FONT,
             {200, 230, 255, 255});

    const char *coinsTxt = TextFormat("%s %d", Loc::GameOver_Coins(), state.globalCoins);
    int cw = MeasureText(coinsTxt, 16);
    DrawText(coinsTxt, cfg.gameWidth / 2 - cw / 2, yTop + 108, 16, Color{255, 215, 0, 255});

    int yb = yButtonsTop;
    Vector2 m = MapWindowToLogical(GetMousePosition());
    bool click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    auto btn = [&](const char *label, Color baseColor = Color{60, 90, 140, 255},
                   Color hoverColor = Color{90, 140, 220, 255}) {
        int bw2 = 260, bh2 = Const::GAMEOVER_BUTTON_H;
        int bx = cfg.gameWidth / 2 - bw2 / 2;
        Rectangle rc{(float)bx, (float)yb, (float)bw2, (float)bh2};
        Color c = CheckCollisionPointRec(m, rc) ? hoverColor : baseColor;
        DrawRectangleRec(rc, c);
        DrawRectangleLines(bx, yb, bw2, bh2, RAYWHITE);
        int ltw = MeasureText(label, 20);
        DrawText(label, bx + bw2 / 2 - ltw / 2, yb + 12, 20, RAYWHITE);
        yb += bh2 + Const::GAMEOVER_BUTTON_GAP;
        return rc;
    };

    Rectangle rRestart = btn(Loc::GameOver_Restart());
    if (click && CheckCollisionPointRec(m, rRestart)) {
        ResetGame();
        ChangeScreen(GameState::Screen::GAME, false);
    }
    Rectangle rMenu = btn(Loc::GameOver_Menu());
    if (click && CheckCollisionPointRec(m, rMenu)) {
        ChangeScreen(GameState::Screen::MENU, false);
    }
    Rectangle rExit = btn(Loc::GameOver_Exit());
    if (click && CheckCollisionPointRec(m, rExit)) {
        running = false;
    }
}

void Game::DrawRevivePrompt() {
    EnsureRenderTarget();
    BeginTextureMode(gameRT);
    ClearBackground(BLACK);

    DrawVerticalGradient(cfg.gameWidth, cfg.gameHeight, state.currentTheme.bgTop, state.currentTheme.bgBottom);
    BeginMode2D(state.camera);
    for (const auto &pf : state.platforms) {
        Color c = GetPlatformColor(pf, state.currentTheme.platMoving, state.currentTheme.platStatic);
        DrawRectangleRec(pf.rect, c);
    }
    EndMode2D();

    DrawRectangle(0, 0, cfg.gameWidth, cfg.gameHeight, Color{0, 0, 0, 180});

    float timerValue = state.reviveTimer > 0 ? state.reviveTimer : 0;
    int timerInt = (int)std::ceil(timerValue);

    int centerX = cfg.gameWidth / 2;
    int centerY = cfg.gameHeight / 2 - 60;
    float radius = 80.f;
    float progress = timerValue / state.REVIVE_TIME_LIMIT;

    DrawCircle(centerX, centerY, radius + 8, Color{40, 40, 50, 255});
    DrawCircle(centerX, centerY, radius, Color{20, 25, 35, 255});

    float startAngle = -90.f;
    float endAngle = startAngle + (360.f * progress);
    Color arcColor;
    if (timerInt <= 2) {
        arcColor = Color{220, 80, 80, 255};
    } else if (timerInt <= 3) {
        arcColor = Color{220, 180, 80, 255};
    } else {
        arcColor = Color{80, 180, 80, 255};
    }
    DrawCircleSector({(float)centerX, (float)centerY}, radius - 5, startAngle, endAngle, 36, arcColor);

    DrawCircle(centerX, centerY, radius - 15, Color{30, 35, 45, 255});

    const char *timerText = TextFormat("%d", timerInt);
    int timerFontSize = 60;
    int tw = MeasureText(timerText, timerFontSize);
    Color timerColor = timerInt <= 2 ? Color{255, 100, 100, 255} : Color{255, 255, 255, 255};
    DrawText(timerText, centerX - tw / 2, centerY - timerFontSize / 2 + 5, timerFontSize, timerColor);

    const char *reviveText = Loc::GameOver_Revive();
    int rw = MeasureText(reviveText, 28);
    DrawText(reviveText, centerX - rw / 2, centerY + (int)radius + 30, 28, WHITE);

    const char *costText = TextFormat("%d", state.reviveCost);
    int cw = MeasureText(costText, 20);
    DrawText(costText, centerX - cw / 2, centerY + (int)radius + 65, 20, Color{255, 215, 0, 255});
    DrawCircle(centerX + cw / 2 + 15, centerY + (int)radius + 75, 8, Color{255, 215, 0, 255});

    Vector2 m = MapWindowToLogical(GetMousePosition());
    bool click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    int btnW = 200, btnH = 50;
    int btnX = centerX - btnW / 2;
    int btnY = centerY + (int)radius + 100;
    Rectangle btnRect = {(float)btnX, (float)btnY, (float)btnW, (float)btnH};
    bool hover = CheckCollisionPointRec(m, btnRect);

    Color btnColor = hover ? Color{80, 200, 80, 255} : Color{60, 160, 60, 255};
    DrawRectangleRec(btnRect, btnColor);
    DrawRectangleLines(btnX, btnY, btnW, btnH, WHITE);

    const char *btnText = Loc::GameOver_Revive();
    int btw = MeasureText(btnText, 24);
    DrawText(btnText, btnX + btnW / 2 - btw / 2, btnY + 13, 24, WHITE);

    if (click && hover) {
        RevivePlayer();
    }

    int skipBtnW = 120, skipBtnH = 35;
    int skipBtnX = centerX - skipBtnW / 2;
    int skipBtnY = btnY + btnH + 20;
    Rectangle skipRect = {(float)skipBtnX, (float)skipBtnY, (float)skipBtnW, (float)skipBtnH};
    bool skipHover = CheckCollisionPointRec(m, skipRect);

    Color skipColor = skipHover ? Color{100, 70, 70, 255} : Color{70, 50, 50, 255};
    DrawRectangleRec(skipRect, skipColor);
    DrawRectangleLines(skipBtnX, skipBtnY, skipBtnW, skipBtnH, Color{200, 200, 200, 200});

    const char *skipText = Loc::GameOver_Cancel();
    int stw = MeasureText(skipText, 18);
    DrawText(skipText, skipBtnX + skipBtnW / 2 - stw / 2, skipBtnY + 9, 18, Color{200, 200, 200, 255});

    if (click && skipHover) {
        ChangeScreen(GameState::Screen::GAMEOVER, false);
    }

    EndTextureMode();

    int winW = GetScreenWidth(), winH = GetScreenHeight();
    float scale = std::fmin((float)winW / cfg.gameWidth, (float)winH / cfg.gameHeight);
    int drawW = (int)(cfg.gameWidth * scale);
    int drawH = (int)(cfg.gameHeight * scale);
    int offX = (winW - drawW) / 2;
    int offY = (winH - drawH) / 2;
    viewportRect = {(float)offX, (float)offY, (float)drawW, (float)drawH};

    BeginDrawing();
    ClearBackground(BLACK);
    Rectangle src = {0, 0, (float)gameRT.texture.width, (float)-gameRT.texture.height};
    Rectangle dst = {(float)offX, (float)offY, (float)drawW, (float)drawH};
    DrawTexturePro(gameRT.texture, src, dst, {0, 0}, 0.f, WHITE);
    EndDrawing();
}

void Game::DrawGame(float dt) {
    PROF_SCOPE("DrawGame");
    if (gameRT.id == 0 || gameRT.texture.width != cfg.gameWidth ||
        gameRT.texture.height != cfg.gameHeight) {
        if (gameRT.id > 0) UnloadRenderTexture(gameRT);
        gameRT = LoadRenderTexture(cfg.gameWidth, cfg.gameHeight);
    }
    BeginTextureMode(gameRT);
    ClearBackground(BLACK);
    DrawGameWorld(dt);
    DrawHud(dt);
    DrawGameOverOverlay();
    if (settings.powerUpEffects && state.shieldFlashAlpha > 0) {
        unsigned char a = (unsigned char)(state.shieldFlashAlpha * 200);
        DrawRectangle(0, 0, cfg.gameWidth, cfg.gameHeight, Color{255, 255, 255, a});
    }
    if (state.fadeAlpha > 0.01f)
        DrawRectangle(0, 0, cfg.gameWidth, cfg.gameHeight,
                      {0, 0, 0, (unsigned char)(state.fadeAlpha * 255)});

    DrawTutorialOverlay(state.tutorial, cfg.gameWidth, cfg.gameHeight);

    EndTextureMode();
    int winW = GetScreenWidth(), winH = GetScreenHeight();
    float scale = std::fmin((float)winW / cfg.gameWidth, (float)winH / cfg.gameHeight);
    int drawW = (int)(cfg.gameWidth * scale);
    int drawH = (int)(cfg.gameHeight * scale);
    int offX = (winW - drawW) / 2;
    int offY = (winH - drawH) / 2;
    viewportRect = {(float)offX, (float)offY, (float)drawW, (float)drawH};
    BeginDrawing();
    DrawVerticalGradient(winW, winH, state.currentTheme.bgTop, state.currentTheme.bgBottom);
    if (gameRT.id > 0) {
        SetTextureFilter(gameRT.texture, TEXTURE_FILTER_BILINEAR);
        float bgScale = std::fmax((float)winW / cfg.gameWidth, (float)winH / cfg.gameHeight) * 1.15f;
        float bgW = cfg.gameWidth * bgScale;
        float bgH = cfg.gameHeight * bgScale;
        float bgX = (winW - bgW) / 2.f;
        float bgY = (winH - bgH) / 2.f;
        Rectangle bgSrc{0, 0, (float)gameRT.texture.width, (float)-gameRT.texture.height};
        Rectangle bgDst{bgX, bgY, bgW, bgH};
        DrawTexturePro(gameRT.texture, bgSrc, bgDst, {0, 0}, 0.f, Color{255, 255, 255, 60});
        DrawTexturePro(gameRT.texture, bgSrc, bgDst, {0, 0}, 0.f, Color{200, 200, 255, 40});
        DrawRectangle(0, 0, winW, winH, Color{0, 0, 20, 90});
    }
    Rectangle src{0, 0, (float)gameRT.texture.width, (float)-gameRT.texture.height};
    Rectangle dst{(float)offX, (float)offY, (float)drawW, (float)drawH};
    DrawTexturePro(gameRT.texture, src, dst, {0, 0}, 0.f, WHITE);
    if (state.fadeAlpha > 0.01f)
        DrawRectangle(0, 0, winW, winH, {0, 0, 0, (unsigned char)(state.fadeAlpha * 255)});
    EndDrawing();
}
