// Audio config
#pragma once
#include "raylib.h"

struct GameAudio {
    Sound sndJump{};
    Sound sndBounce{};
    Sound sndDeath{};
    Sound sndThemeChange{};
    Music musicBg{};
    float volJump = 0.5f;          
    float volBounce = 0.5f;
    float volDeath = 0.5f;
    float volThemeChange = 0.5f;
    float volMusic = 0.5f;         
    float masterVolume = 0.05f; 
    float masterSlider = 0.5f; 
};

constexpr float VOL_JUMP_MULT = 3.0f;
constexpr float VOL_THEME_MULT = 8.0f; 
constexpr float VOL_DEATH_MULT = 2.5f;
constexpr float VOL_BOUNCE_MULT = 1.0f;
constexpr float VOL_MUSIC_MULT = 0.08f; 

constexpr float VOLUME_SCALE = 0.01f; 

void LoadGameAudio(GameAudio &audio);
void UnloadGameAudio(GameAudio &audio);
