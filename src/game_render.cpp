#include "game.h"
#include "raylib.h"
#include "collectibles.h"
#include "localization.h"
#include "daily_challenge.h"
#include "tutorial.h"
#include "rng.h"
#include <algorithm>
#include <cmath>

#include "debug.h"
#include "constants.h"
#include "world_art.h"
#include "ui_kit.h"
#include "rlgl.h"

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
        WorldArt::PixelDot(s.x, drawY, s.size > 2.f ? 1.5f : 1.f, c);
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

    if (worldArt.loaded) {
        int prevIdx = state.prevThemeIndex, curIdx = state.currentThemeIndex;
        WorldArt::DrawSky(worldArt, prevIdx, cfg.gameWidth, cfg.gameHeight, WHITE);
        if (tb < 1.f) {
            WorldArt::DrawSky(worldArt, curIdx, cfg.gameWidth, cfg.gameHeight, {255, 255, 255, (unsigned char)(tb * 255)});
        } else {
            WorldArt::DrawSky(worldArt, curIdx, cfg.gameWidth, cfg.gameHeight, WHITE);
        }
    } else {
        DrawVerticalGradient(cfg.gameWidth, cfg.gameHeight, blended.bgTop, blended.bgBottom);
    }
    DrawBgStars(cfg.gameWidth, cfg.gameHeight, state.animTime, blended, state.camera.target.y);
    if (worldArt.loaded) {
        float camY = state.camera.target.y;
        if (state.themeBlend < 1.f) {
            WorldArt::DrawBiomeLayers(worldArt, state.prevTheme.biomeType, camY, cfg.gameWidth, cfg.gameHeight,
                                      {255, 255, 255, (unsigned char)((1.f - tb) * 255)});
            WorldArt::DrawBiomeLayers(worldArt, state.currentTheme.biomeType, camY, cfg.gameWidth, cfg.gameHeight,
                                      {255, 255, 255, (unsigned char)(tb * 255)});
        } else {
            WorldArt::DrawBiomeLayers(worldArt, state.currentTheme.biomeType, camY, cfg.gameWidth, cfg.gameHeight, WHITE);
        }
    }

    DrawBiomeEffects(cfg.gameWidth, cfg.gameHeight, state.camera.target.y, state.animTime);

    Particles::Update(state.particles, dt, cfg.GRAVITY, 0.2f);
    BeginMode2D(state.camera);

    for (auto &pf : state.platforms) {
        if (!pf.visible) continue;
        int row = pf.biome;
        switch (pf.type) {
            case PlatformType::CRUMBLING: row = WorldArt::ROW_CRUMBLING; break;
            case PlatformType::SPRING: row = WorldArt::ROW_SPRING; break;
            case PlatformType::ICE: row = WorldArt::ROW_ICE; break;
            case PlatformType::DISAPPEARING: row = WorldArt::ROW_GHOST; break;
            default: break;
        }
        Rectangle r = pf.rect;
        unsigned char alpha = 255;

        if (pf.type == PlatformType::CRUMBLING && pf.triggered) {
            pf.crumbleProgress = (pf.stateTimer > 0.3f) ? std::fmin(1.f, (pf.stateTimer - 0.3f) * 2.f) : pf.stateTimer / 0.3f;
            float shakeAmt = 1.f + pf.crumbleProgress * 3.f;
            r.x += std::round(std::sin(state.animTime * 40.f) * shakeAmt / 2.f) * 2.f;
            r.y += pf.crumbleProgress * pf.crumbleProgress * 24.f;
            alpha = (unsigned char)(255 * (1.f - pf.crumbleProgress));
            // Falling debris chunks
            int chunks = (int)(pf.crumbleProgress * 8);
            for (int ci = 0; ci < chunks; ci++) {
                float seed = (float)ci * 3.7f;
                float cx = pf.rect.x + std::fmod(seed * 47.1f, pf.rect.width);
                float cy = pf.rect.y + 18.f + pf.crumbleProgress * 60.f * (1.f + std::fmod(seed, 1.f));
                WorldArt::PixelDot(cx, cy, 1.f + (ci % 2), Color{176, 112, 62, alpha});
            }
        }
        if (pf.type == PlatformType::DISAPPEARING) {
            if (pf.alpha <= 0.02f && !(pf.triggered && pf.stateTimer >= 2.f)) continue;
            float a = pf.alpha;
            if (pf.triggered && pf.stateTimer < 0.5f) a = (std::sin(pf.stateTimer * 40.f) > 0.f) ? 1.f : 0.35f;
            else if (!pf.triggered) a = 0.75f + 0.25f * std::sin(state.animTime * 3.f);
            alpha = (unsigned char)(255 * std::fmax(0.f, std::fmin(1.f, a)));
        }

        if (worldArt.loaded) {
            WorldArt::DrawPlatform(worldArt, r, row, {255, 255, 255, alpha});
        } else {
            Color c = GetPlatformColor(pf, blended.platMoving, blended.platStatic);
            DrawRectangleRec(r, {c.r, c.g, c.b, (unsigned char)(c.a * alpha / 255)});
        }

        if (row == BIOME_NEON && pf.type == PlatformType::NORMAL) {
            Color g = blended.glowColor;
            float pulse = 0.7f + 0.3f * std::sin(state.animTime * 4.f + pf.rect.x * 0.05f);
            DrawRectangle((int)r.x + 2, (int)r.y, (int)r.width - 4, 2, {g.r, g.g, g.b, 255});
            DrawRectangle((int)r.x + 2, (int)r.y - 4, (int)r.width - 4, 4, {g.r, g.g, g.b, (unsigned char)(60 * pulse)});
        }
        if (pf.moving && alpha > 0) {
            // Rail markers under moving platforms: little chevrons pointing along the path
            unsigned char ma = (unsigned char)(alpha * 0.8f);
            float cy = r.y + 26.f;
            float cx = r.x + r.width / 2.f;
            for (int k = -1; k <= 1; k += 2) {
                float bx = cx + k * 10.f;
                WorldArt::PixelDot(bx, cy, 1.f, {255, 255, 255, ma});
                WorldArt::PixelDot(bx - k * 2.f, cy - 2.f, 1.f, {255, 255, 255, ma});
                WorldArt::PixelDot(bx - k * 2.f, cy + 2.f, 1.f, {255, 255, 255, ma});
            }
        }
        if (pf.type == PlatformType::SPRING) {
            // Pixel spring: bouncing pad on a zig-zag coil
            float squash = std::sin(state.animTime * 8.f) * 2.f;
            float cx = std::floor((r.x + r.width / 2.f) / 2.f) * 2.f;
            float padY = std::floor((r.y - 14.f + squash) / 2.f) * 2.f;
            for (int i = 0; i < 3; i++) {
                float yy = padY + 4.f + i * 3.f;
                DrawRectangle((int)cx - 8 + (i % 2) * 4, (int)yy, 12, 2, Color{150, 160, 170, 255});
            }
            DrawRectangle((int)cx - 12, (int)padY, 24, 4, Color{255, 90, 70, 255});
            DrawRectangle((int)cx - 12, (int)padY, 24, 2, Color{255, 160, 140, 255});
            DrawRectangle((int)cx - 12, (int)padY + 4, 24, 2, Color{120, 30, 30, 255});
        }
    }

    if (worldArt.items.id > 0) {
        for (const auto &c : state.coins)
            if (!c.collected) WorldArt::DrawCoin(worldArt, c.pos, c.animTime);
        for (const auto &p : state.powerups)
            if (!p.collected) WorldArt::DrawPowerUp(worldArt, p.pos, (int)p.type, p.animTime);
    } else {
        Collectibles::DrawCoins(state.coins, state.animTime);
        Collectibles::DrawPowerUps(state.powerups, state.animTime);
    }
    Particles::Draw(state.particles);

    if (state.playerTexture.id > 0) {
        Texture2D playerTex = state.playerTexture;
        float pvx = state.player.vel.x;
        if (pvx < -10.f) state.playerFacingLeft = true;
        else if (pvx > 10.f) state.playerFacingLeft = false;
        if (state.playerFramesPerSkin >= 10) {
            enum { IDLE = 0, RUN = 2, JUMP = 6, FALL = 7, WALL = 8, HURT = 9 };
            int frame;
            if (state.dying) {
                frame = HURT;
            } else if (state.wallSlidingLeft || state.wallSlidingRight) {
                frame = WALL;
                state.playerFacingLeft = state.wallSlidingLeft;
            } else if (!state.onGround) {
                frame = state.player.vel.y < 0.f ? JUMP : FALL;
            } else if (std::fabs(pvx) > 40.f) {
                state.runAnimTime += dt * (6.f + 8.f * std::fabs(pvx) / cfg.MAX_HSPEED);
                frame = RUN + (int)state.runAnimTime % 4;
            } else {
                state.runAnimTime = 0.f;
                frame = IDLE + (int)(state.animTime * 2.f) % 2;
            }
            playerTex = PlayerFrame(state.skins.selected, frame);
        }
        Rectangle src{0, 0, (float)playerTex.width, (float)playerTex.height};
        float baseScale = state.playerSpriteScale;
        float dstW = state.player.width * baseScale;
        float dstH = state.player.height * baseScale;
        float padRatio = (playerTex.height > 0)
                             ? (state.playerSpriteBottomPad / (float)playerTex.height)
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
        if (state.playerFacingLeft) src.width = -src.width;
        constexpr int MIN_COMBO = Const::COMBO_MIN_MULT;
        bool comboActive = (settings.comboEffects && state.comboCount >= MIN_COMBO &&
                            state.comboTimer > 0 && state.shaderFire.id > 0);
        if (state.dying) {
            Rectangle d{dst.x + dst.width / 2.f, dst.y + dst.height / 2.f, dst.width, dst.height};
            DrawTexturePro(playerTex, src, d, {dst.width / 2.f, dst.height / 2.f}, state.dyingSpin, WHITE);
        } else if (comboActive) {
            float intensity = std::fmin(1.f, (float)(state.comboCount - 1) / 6.f);
            float pulse = (std::sin(state.animTime * 5.f) + 1.f) * 0.5f;
            float finalIntensity = (0.5f + 0.5f * pulse) * intensity;
            Vector2 sprSize{(float)playerTex.width, (float)playerTex.height};
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
            DrawTexturePro(playerTex, src, aura, {0, 0}, leanDeg, WHITE);
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
            DrawTexturePro(playerTex, src, dst, {0, 0}, leanDeg, WHITE);
            EndShaderMode();
        } else
            DrawTexturePro(playerTex, src, dst, {0, 0}, leanDeg, WHITE);
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
        // Friction sparks/dust scraping off the wall under the glove
        float wx = state.wallSlidingLeft ? state.player.pos.x : state.player.pos.x + state.player.width;
        float side = state.wallSlidingLeft ? 1.f : -1.f;
        for (int i = 0; i < 5; i++) {
            float t = std::fmod(state.animTime * 3.f + i * 0.2f, 1.f);
            float px = wx + side * t * 10.f;
            float py = state.player.pos.y + 6.f - t * 18.f + i * 2.f;
            unsigned char a = (unsigned char)(220 * (1.f - t));
            WorldArt::PixelDot(px, py, 1.f, Color{230, 230, 240, a});
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
            WorldArt::PixelDot(fx + drift, fy, 1.f, Color{180, 255, 120, a});
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
            WorldArt::PixelDot(fx, fy, sz > 3.5f ? 2.f : 1.f, Color{255, (unsigned char)(120 + (int)(life * 80)), 20, a});
        }
    } else if (biome == BIOME_SNOW) {
        for (int i = 0; i < 30; i++) {
            float seed = (float)i * 3.9f;
            float fx = std::fmod(seed * 53.7f + std::sin(time * 0.3f + seed) * 50.f, (float)w);
            float fy = std::fmod(cameraY * 0.05f + seed * 41.3f + time * 25.f + seed * 10.f, (float)(h + 50)) - 25;
            float sz = 1.5f + std::fmod(seed, 3.f);
            unsigned char a = (unsigned char)(100 + 80 * std::sin(time + seed));
            WorldArt::PixelDot(fx, fy, sz > 2.5f ? 2.f : 1.f, Color{230, 240, 255, a});
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
            WorldArt::PixelDot(fx, fy, 2.f, Color{220, 190, 120, a});
        }
    }
}

// ---------------------------------------------------------------------------------------------
// HUD (drawn in 480x800 game space with a 2px pixel unit to match the world art)
// ---------------------------------------------------------------------------------------------
static constexpr int HP = 2; // HUD pixel unit

static void DrawItemSprite(const Texture2D &items, int cell, float x, float y, float scale) {
    if (items.id == 0) return;
    DrawTexturePro(items, {(float)(cell * 16), 0, 16, 16}, {std::floor(x), std::floor(y), 16 * scale, 16 * scale}, {0, 0}, 0.f, WHITE);
}

void Game::DrawHud(float dt) {
    if (state.score > state.highScore) state.highScore = state.score;
    if (state.currentScreen == GameState::Screen::GAMEOVER) return;
    Ui::SetUnit(HP);
    const Color muted{150, 160, 196, 255};
    const Color gold{255, 214, 110, 255};

    // --- Score panel (top-left) ---
    {
        const char *scoreTxt = TextFormat("%d", state.score);
        int kScore = 4;
        int sw = Ui::Measure(scoreTxt, kScore);
        int w = std::max(120, sw + 12 * HP);
        Rectangle pnl{8, 8, (float)w, 64};
        Ui::Panel(pnl, HP, false, 215);
        Ui::Text(Loc::HUD_Score(), pnl.x + 5 * HP, pnl.y + 4 * HP, 2, muted);
        Ui::TextOutlined(scoreTxt, pnl.x + 5 * HP, pnl.y + 13 * HP, kScore, WHITE);

        const char *bestTxt = TextFormat("%d", state.highScore);
        Ui::DrawIcon(Ui::ICON_CROWN, 12, pnl.y + pnl.height + 6, HP);
        Ui::TextOutlined(bestTxt, 12 + Ui::IconW(Ui::ICON_CROWN) * HP + 6, pnl.y + pnl.height + 7, 2, gold);
    }

    // --- Combo (under the score) ---
    {
        static int lastCombo = 0;
        static float pop = 0.f;
        if (state.comboCount > lastCombo) pop = 0.25f;
        lastCombo = state.comboCount;
        pop = std::max(0.f, pop - GetFrameTime());
        if (state.comboCount >= 2 && state.comboTimer > 0) {
            bool hot = state.comboCount >= Const::COMBO_MIN_MULT;
            float y = 110;
            int k = pop > 0.f ? 4 : 3;
            Color c = hot ? Color{255, 150, 60, 255} : Color{255, 226, 150, 255};
            float wob = hot ? std::round(std::sin(state.animTime * 20.f)) * HP : 0.f;
            Ui::DrawIcon(Ui::ICON_FLAME, 10, y + wob, HP, hot ? WHITE : Color{255, 220, 200, 230});
            const char *txt = TextFormat("x%d", state.comboCount);
            Ui::TextOutlined(txt, 10 + Ui::IconW(Ui::ICON_FLAME) * HP + 6, y + (k == 4 ? -2 : 2), k, c);
            Ui::Bar({10, y + 28, 96, 8}, HP, state.comboTimer / cfg.COMBO_WINDOW, hot ? Color{255, 120, 40, 255} : Color{240, 200, 90, 255});
        }
    }

    // --- Active power-ups (left column) ---
    {
        float y = 158;
        const char *names[4] = {Loc::HUD_DoubleJump(), Loc::HUD_Shield(), Loc::HUD_Slow(), Loc::HUD_Magnet()};
        const bool active[4] = {state.activeDoubleJump, state.activeShield, state.activeSlowMotion, state.activeMagnet};
        const float maxT[4] = {10.f, 8.f, 8.f, 10.f};
        const Color cols[4] = {{100, 200, 255, 255}, {110, 240, 150, 255}, {190, 140, 255, 255}, {255, 190, 90, 255}};
        for (int i = 0; i < 4; i++) {
            if (!active[i] || state.powerUpTimers[i] <= 0) continue;
            float t = state.powerUpTimers[i];
            // blink when about to expire
            if (t < 2.f && std::fmod(t, 0.3f) < 0.12f) { y += 40; continue; }
            DrawItemSprite(worldArt.items, 4 + i, 6, y, 2.f);
            Ui::TextOutlined(names[i], 44, y + 2, 2, WHITE);
            Ui::Bar({44, y + 20, 90, 10}, HP, t / maxT[i], cols[i]);
            y += 40;
        }
    }

    // --- Coins pill (top-right) ---
    float rightY = 8;
    {
        const char *coinTxt = TextFormat("%d", state.globalCoins);
        int tw = Ui::Measure(coinTxt, 3);
        float w = 32 + 3 * HP + tw + 10 * HP;
        Rectangle pill{(float)cfg.gameWidth - 8 - w, rightY, w, 44};
        Ui::Panel(pill, HP, false, 215);
        DrawItemSprite(worldArt.items, 0, pill.x + 3 * HP, pill.y + 6, 2.f);
        Ui::TextOutlined(coinTxt, pill.x + 3 * HP + 32 + 3 * HP, pill.y + 12, 3, gold);
        rightY += 44 + 6;
    }

    // --- Speed stage gauge: bolt + 5 pips, the current one fills up ---
    {
        const int pips = 5;
        const float pipW = 10, pipH = 16, gap = 2;
        float w = Ui::IconW(Ui::ICON_BOLT) * HP + 6 + pips * pipW + (pips - 1) * gap;
        float x = cfg.gameWidth - 8 - w - 6;
        bool maxed = state.speedStage >= pips;
        float phase = maxed ? 1.f : state.stageTimer / cfg.STAGE_DURATION;
        bool flash = maxed && std::fmod(state.animTime, 0.5f) < 0.25f;
        Ui::DrawIcon(Ui::ICON_BOLT, x, rightY, HP, maxed ? Color{255, 120, 100, 255} : WHITE);
        float px = x + Ui::IconW(Ui::ICON_BOLT) * HP + 6;
        for (int i = 0; i < pips; i++) {
            Rectangle r{px + i * (pipW + gap), rightY + 1, pipW, pipH};
            Ui::Frame(r, HP, {18, 20, 34, 230}, {18, 20, 34, 230}, {18, 20, 34, 230}, {6, 6, 14, 255});
            float fill = (i < state.speedStage) ? 1.f : (i == state.speedStage ? phase : 0.f);
            if (fill > 0.f) {
                float ih = std::floor((pipH - 2 * HP) * fill / HP) * HP;
                Color c = maxed ? (flash ? Color{255, 80, 70, 255} : Color{200, 40, 50, 255})
                                : (i < state.speedStage ? Color{255, 206, 72, 255} : Color{140, 190, 255, 255});
                DrawRectangle((int)r.x + HP, (int)(r.y + pipH - HP - ih), (int)pipW - 2 * HP, (int)ih, c);
            }
        }
        rightY += pipH + 10;
    }

    if (settings.showFPS) {
        const char *fps = TextFormat("%d FPS", GetFPS());
        Ui::TextOutlined(fps, cfg.gameWidth - 12 - Ui::Measure(fps, 2), rightY, 2, {220, 230, 255, 200});
        rightY += 20;
    }

    if (state.isDailyRun) {
        const char *challengeName = GetChallengeName(state.dailyChallenge.type);
        int cw = Ui::Measure(challengeName, 2);
        float w = cw + Ui::IconW(Ui::ICON_CALENDAR) * HP + 14 * HP / 2 + 8;
        Rectangle tag{(float)cfg.gameWidth - 8 - w, rightY, w, 28};
        Ui::Frame(tag, HP, {60, 40, 100, 220}, {100, 70, 150, 220}, {34, 22, 60, 220}, {6, 6, 14, 255});
        Ui::DrawIcon(Ui::ICON_CALENDAR, tag.x + 3 * HP, tag.y + 5, HP);
        Ui::Text(challengeName, tag.x + 3 * HP + Ui::IconW(Ui::ICON_CALENDAR) * HP + 6, tag.y + 7, 2, {255, 180, 80, 255});
    }

    // --- Biome banner ---
    if (state.themeChangeTimer > 0) {
        state.themeChangeTimer -= dt;
        float t = state.themeChangeTimer;
        float appear = std::fmin(1.f, (3.f - t) / 0.3f);
        float fade = std::fmin(1.f, t / 0.6f);
        float a = std::fmax(0.f, std::fmin(appear, fade));
        unsigned char al = (unsigned char)(a * 255);
        const char *name = state.currentTheme.name;
        int k = Ui::FitK(name, 4, cfg.gameWidth - 80);
        int len = (int)TextLength(name);
        int tw = Ui::Measure(name, k) + (len - 1) * k;
        float slide = std::round((1.f - appear) * -20.f / HP) * HP;
        Rectangle ribbon{std::floor(cfg.gameWidth / 2.f - tw / 2.f - 20), 170 + slide, (float)tw + 40, (float)(7 * k + 24)};
        Ui::Panel(ribbon, HP, true, (unsigned char)(al * 0.9f));
        Ui::FancyText(name, ribbon.x + 20, ribbon.y + 10, k, {255, 255, 255, al}, {160, 200, 255, al}, {10, 10, 26, al}, 0.f, 0.f);
    }

    // --- Achievement toast ---
    if (state.achievementPopupTimer > 0 && !state.lastUnlockedAchievement.empty()) {
        float t = state.achievementPopupTimer / 3.f;
        float slide = 1.f;
        if (t > 0.85f) slide = (1.f - t) / 0.15f;
        else if (t < 0.2f) slide = t / 0.2f;
        slide = std::fmax(0.f, std::fmin(1.f, slide));

        const char *nm = state.lastUnlockedAchievement.c_str();
        int k = Ui::FitK(nm, 2, cfg.gameWidth - 90);
        float popupW = std::fmax(220.f, (float)Ui::Measure(nm, k) + 64);
        float popupH = 48;
        float popupX = 8;
        float popupY = std::round((cfg.gameHeight - popupH - 10 + (1.f - slide) * 70.f) / HP) * HP;
        Ui::Panel({popupX, popupY, popupW, popupH}, HP, false, 235);
        Ui::DrawIconCentered(Ui::ICON_TROPHY, popupX + 24, popupY + popupH / 2, HP);
        const char *hdr = Loc::GetLanguage() == Language::EN ? "UNLOCKED!" : "ODBLOKOWANO!";
        Ui::Text(hdr, popupX + 46, popupY + 8, 1, gold);
        Ui::Text(nm, popupX + 46, popupY + 22, k, WHITE);
    }
}

void Game::DrawGameOverOverlay() {
    if (state.currentScreen != GameState::Screen::GAMEOVER) return;
    Ui::SetUnit(HP);
    DrawRectangleGradientV(0, 0, cfg.gameWidth, cfg.gameHeight, {10, 8, 20, 150}, {10, 8, 20, 220});

    float t = (float)GetTime();
    float cx = cfg.gameWidth / 2.f;
    int w = 360;
    float x = std::floor(cx - w / 2.f);
    float yTop = 120;
    bool newBest = state.score > 0 && state.score > state.runStartHighScore && !state.isDailyRun;
    float panelH = 36 + (state.isDailyRun ? 20 : 0) + (newBest ? 30 : 0) + 98 + 50 + 58 + 52 + 36 + 20;
    Rectangle panel{x, yTop, (float)w, panelH};
    Ui::Panel(panel, HP, true, 240);

    const char *title = Loc::GameOver_Title();
    int kT = 5;
    int len = (int)TextLength(title);
    int tw = Ui::Measure(title, kT) + (len - 1) * kT;
    Ui::FancyText(title, cx - tw / 2.f, yTop - 22, kT, {255, 150, 120, 255}, {200, 40, 56, 255}, {20, 6, 14, 255}, 1.f, t);

    float y = yTop + 36;
    if (state.isDailyRun) {
        Ui::TextCentered(GetChallengeName(state.dailyChallenge.type), cx, y, 2, {255, 180, 80, 255});
        y += 20;
    }

    if (newBest) {
        const char *nb = Loc::GetLanguage() == Language::EN ? "NEW BEST!" : "NOWY REKORD!";
        bool on = std::fmod(t, 0.6f) < 0.4f;
        int nw = Ui::Measure(nb, 3);
        float bx = std::floor(cx - (nw + 30) / 2.f);
        Ui::DrawIcon(Ui::ICON_CROWN, bx, y + 1, HP);
        Ui::TextOutlined(nb, bx + 30, y, 3, on ? Color{255, 226, 90, 255} : Color{255, 170, 60, 255});
        y += 30;
    }

    // Score showcase
    Rectangle box{x + 24, y, (float)w - 48, 84};
    Ui::Inset(box, HP, {12, 12, 26, 255});
    Ui::TextCentered(Loc::HUD_Score(), cx, box.y + 8, 2, {150, 160, 196, 255});
    Ui::TextCentered(TextFormat("%d", state.score), cx, box.y + 28, 6, {255, 226, 150, 255});
    y += box.height + 14;

    int bestScore = state.isDailyRun ? state.dailyChallenge.bestScore : state.highScore;
    float colL = cx - 80, colR = cx + 80;
    {
        const char *bs = TextFormat("%d", bestScore);
        int bw = Ui::Measure(bs, 3);
        float bx = std::floor(colL - (bw + 28) / 2.f);
        Ui::DrawIcon(Ui::ICON_CROWN, bx, y + 3, HP);
        Ui::Text(bs, bx + 28, y, 3, {200, 230, 255, 255});
        const char *cs = TextFormat("%d", state.globalCoins);
        int cw = Ui::Measure(cs, 3);
        float cxx = std::floor(colR - (cw + 36) / 2.f);
        DrawItemSprite(worldArt.items, 0, cxx, y - 6, 2.f);
        Ui::Text(cs, cxx + 36, y, 3, {255, 214, 80, 255});
        y += 26;
        Ui::TextCentered(state.isDailyRun ? Loc::Daily_Best() : Loc::GameOver_Best(), colL, y, 1, {130, 136, 166, 255});
        Ui::TextCentered(Loc::GameOver_Coins(), colR, y, 1, {130, 136, 166, 255});
        y += 24;
    }

    Vector2 m = MapWindowToLogical(GetMousePosition());
    bool click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    float bw2 = 260, bh2 = 46;
    Rectangle rRestart{std::floor(cx - bw2 / 2), y, bw2, bh2};
    if (Ui::Button(rRestart, Loc::GameOver_Restart(), m, click, Ui::STYLE_GREEN, 3)) {
        ResetGame();
        ChangeScreen(GameState::Screen::GAME, false);
    }
    y += bh2 + 12;
    Rectangle rMenu{std::floor(cx - bw2 / 2), y, bw2, 40};
    if (Ui::Button(rMenu, Loc::GameOver_Menu(), m, click, Ui::STYLE_BLUE, 2)) {
        ChangeScreen(GameState::Screen::MENU, false);
    }
    y += 40 + 12;
    Rectangle rExit{std::floor(cx - 90), y, 180, 36};
    if (Ui::Button(rExit, Loc::GameOver_Exit(), m, click, Ui::STYLE_RED, 2)) {
        running = false;
    }
}

void Game::DrawRevivePrompt() {
    EnsureRenderTarget();
    BeginTextureMode(gameRT);
    ClearBackground(BLACK);

    DrawGameWorld(0.f); // frozen world behind the prompt
    DrawRectangleGradientV(0, 0, cfg.gameWidth, cfg.gameHeight, {10, 8, 20, 150}, {10, 8, 20, 220});
    Ui::SetUnit(HP);

    float timerValue = state.reviveTimer > 0 ? state.reviveTimer : 0;
    int timerInt = (int)std::ceil(timerValue);
    float progress = timerValue / state.REVIVE_TIME_LIMIT;
    float cx = cfg.gameWidth / 2.f;
    float t = (float)GetTime();

    Rectangle panel{std::floor(cx - 170), 170, 340, 396};
    Ui::Panel(panel, HP, true, 240);
    const char *title = Loc::GameOver_Revive();
    int len = (int)TextLength(title);
    int tw = Ui::Measure(title, 5) + (len - 1) * 5;
    Ui::FancyText(title, cx - tw / 2.f, panel.y - 22, 5, {170, 255, 170, 255}, {50, 170, 80, 255}, {6, 20, 10, 255}, 1.f, t);

    // Countdown ring made of pixel blocks
    float ringCy = panel.y + 130;
    float radius = 72;
    const int blocks = 40;
    Color arc = timerInt <= 2 ? Color{230, 70, 70, 255} : (timerInt <= 3 ? Color{240, 190, 70, 255} : Color{90, 210, 100, 255});
    for (int i = 0; i < blocks; i++) {
        float a = -PI / 2 + (2 * PI) * i / blocks;
        float bx = std::floor((cx + std::cos(a) * radius) / HP) * HP;
        float by = std::floor((ringCy + std::sin(a) * radius) / HP) * HP;
        bool lit = (float)i / blocks < progress;
        DrawRectangle((int)bx - 5, (int)by - 5, 10, 10, {6, 6, 14, 255});
        DrawRectangle((int)bx - 3, (int)by - 3, 6, 6, lit ? arc : Color{40, 44, 66, 255});
    }
    int kNum = 9;
    bool pulse = timerInt <= 2 && std::fmod(timerValue, 1.f) > 0.5f;
    Ui::TextOutlined(TextFormat("%d", timerInt), cx - Ui::Measure(TextFormat("%d", timerInt), kNum) / 2.f,
                     ringCy - Ui::GlyphHeight(kNum) / 2.f, kNum, pulse ? Color{255, 120, 110, 255} : WHITE);

    // Cost
    float y = ringCy + radius + 30;
    const char *costText = TextFormat("%d", state.reviveCost);
    int cw = Ui::Measure(costText, 3);
    float cx0 = std::floor(cx - (32 + 8 + cw) / 2.f);
    DrawItemSprite(worldArt.items, 0, cx0, y - 6, 2.f);
    Ui::TextOutlined(costText, cx0 + 40, y, 3, {255, 214, 80, 255});
    y += 40;

    Vector2 m = MapWindowToLogical(GetMousePosition());
    bool click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    Rectangle btn{std::floor(cx - 110), y, 220, 50};
    if (Ui::Button(btn, Loc::GameOver_Revive(), m, click, Ui::STYLE_GREEN, 3, state.globalCoins >= state.reviveCost)) {
        RevivePlayer();
    }
    y += 50 + 14;
    Rectangle skip{std::floor(cx - 70), y, 140, 36};
    if (Ui::Button(skip, Loc::GameOver_Cancel(), m, click, Ui::STYLE_RED, 2)) {
        ChangeScreen(GameState::Screen::GAMEOVER, false);
    }

    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);
    PresentGameRT();
    EndDrawing();
}

void Game::PresentGameRT() {
    int winW = GetScreenWidth(), winH = GetScreenHeight();
    float scale = std::fmin((float)winW / cfg.gameWidth, (float)winH / cfg.gameHeight);
    int drawW = (int)(cfg.gameWidth * scale);
    int drawH = (int)(cfg.gameHeight * scale);
    int offX = (winW - drawW) / 2;
    int offY = (winH - drawH) / 2;
    viewportRect = {(float)offX, (float)offY, (float)drawW, (float)drawH};
    DrawVerticalGradient(winW, winH, state.currentTheme.bgTop, state.currentTheme.bgBottom);
    if (gameRT.id == 0) return;
    if (drawW < winW - 1 || drawH < winH - 1) {
        SetTextureFilter(gameRT.texture, TEXTURE_FILTER_BILINEAR); // soft blurred backdrop for letterbox
        float bgScale = std::fmax((float)winW / cfg.gameWidth, (float)winH / cfg.gameHeight) * 1.15f;
        float bgW = cfg.gameWidth * bgScale;
        float bgH = cfg.gameHeight * bgScale;
        Rectangle bgSrc{0, 0, (float)gameRT.texture.width, (float)-gameRT.texture.height};
        Rectangle bgDst{(winW - bgW) / 2.f, (winH - bgH) / 2.f, bgW, bgH};
        DrawTexturePro(gameRT.texture, bgSrc, bgDst, {0, 0}, 0.f, Color{255, 255, 255, 60});
        DrawTexturePro(gameRT.texture, bgSrc, bgDst, {0, 0}, 0.f, Color{200, 200, 255, 40});
        DrawRectangle(0, 0, winW, winH, Color{0, 0, 20, 90});
    }
    SetTextureFilter(gameRT.texture, TEXTURE_FILTER_POINT); // keep pixel art crisp
    Rectangle src{0, 0, (float)gameRT.texture.width, (float)-gameRT.texture.height};
    Rectangle dst{(float)offX, (float)offY, (float)drawW, (float)drawH};
    // Overlays leave the render target's alpha below 1; copy it opaque so the backdrop can't bleed through.
    rlDrawRenderBatchActive();
    rlSetBlendFactors(RL_ONE, RL_ZERO, RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM);
    DrawTexturePro(gameRT.texture, src, dst, {0, 0}, 0.f, WHITE);
    EndBlendMode();
}

void Game::RenderMenuScene(float time) {
    EnsureRenderTarget();
    BeginTextureMode(gameRT);
    ClearBackground(BLACK);
    int W = cfg.gameWidth, H = cfg.gameHeight;
    if (!worldArt.loaded || state.themes.empty()) {
        DrawVerticalGradient(W, H, {20, 24, 44, 255}, {8, 8, 16, 255});
        EndTextureMode();
        return;
    }
    // Slow climb up an endless tower, cycling through the biomes with a crossfade
    const float period = 9.f, fadeTime = 1.5f;
    int n = (int)state.themes.size();
    int cur = (int)(time / period) % n;
    int next = (cur + 1) % n;
    float local = std::fmod(time, period);
    float blend = local > period - fadeTime ? (local - (period - fadeTime)) / fadeTime : 0.f;
    float camY = -time * 45.f;

    auto layer = [&](int idx, unsigned char a) {
        Color tint{255, 255, 255, a};
        WorldArt::DrawSky(worldArt, idx, W, H, tint);
        WorldArt::DrawBiomeLayers(worldArt, state.themes[idx].biomeType, camY, W, H, tint);
    };
    layer(cur, 255);
    if (blend > 0.f) layer(next, (unsigned char)(blend * 255));

    // Floating platforms drifting past, in the current biome's tiles
    int biome = state.themes[blend > 0.5f ? next : cur].biomeType;
    const float spacing = 120.f;
    float scroll = std::fmod(-camY, spacing);
    for (int i = -1; i < H / (int)spacing + 2; i++) {
        float wy = i * spacing + scroll;
        int idx = i - (int)std::floor(-camY / spacing);
        unsigned int h = (unsigned int)(idx * 2654435761u);
        float pw = 80.f + (float)(h % 5) * 16.f;
        float px = 20.f + (float)((h >> 8) % (unsigned int)(W - 40 - pw));
        WorldArt::DrawPlatform(worldArt, {px, std::floor(wy), pw, 18.f}, biome, {255, 255, 255, 200});
    }
    EndTextureMode();
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
    BeginDrawing();
    PresentGameRT();
    if (state.fadeAlpha > 0.01f)
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, (unsigned char)(state.fadeAlpha * 255)});
    EndDrawing();
}

void Game::DrawPlayerPreview(Vector2 feet, float scale, int skin, float time, bool running) {
    if (state.playerFramesPerSkin < 10) return;
    int frame = running ? 2 + (int)(time * 10.f) % 4 : (int)(time * 2.f) % 2;
    Texture2D tex = PlayerFrame(skin, frame);
    float w = tex.width * scale, h = tex.height * scale;
    float pad = state.playerSpriteBottomPad * scale;
    DrawTexturePro(tex, {0, 0, (float)tex.width, (float)tex.height}, {feet.x - w / 2.f, feet.y - h + pad, w, h}, {0, 0},
                   0.f, WHITE);
}
