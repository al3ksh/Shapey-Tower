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
#include "world_art.h"

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
    if (worldArt.items.id > 0)
        DrawTexturePro(worldArt.items, {0, 0, 16, 16}, {(float)cfg.gameWidth - 57, (float)coinY - 12, 24, 24}, {0, 0}, 0.f, WHITE);
    else
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

    DrawGameWorld(0.f); // frozen world behind the prompt

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
        SetTextureFilter(gameRT.texture, TEXTURE_FILTER_BILINEAR); // soft blurred backdrop for letterbox
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
    SetTextureFilter(gameRT.texture, TEXTURE_FILTER_POINT); // keep pixel art crisp
    Rectangle src{0, 0, (float)gameRT.texture.width, (float)-gameRT.texture.height};
    Rectangle dst{(float)offX, (float)offY, (float)drawW, (float)drawH};
    DrawTexturePro(gameRT.texture, src, dst, {0, 0}, 0.f, WHITE);
    if (state.fadeAlpha > 0.01f)
        DrawRectangle(0, 0, winW, winH, {0, 0, 0, (unsigned char)(state.fadeAlpha * 255)});
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
