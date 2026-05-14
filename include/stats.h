#pragma once
#include <string>

struct GameStats {
    int gamesPlayed = 0;
    int totalScore = 0;
    int bestScore = 0;
    int totalCoinsCollected = 0;
    int bestCombo = 0;
    int totalPlatformsLanded = 0;
    int bestPlatformStreak = 0;
    int totalPowerUpsCollected = 0;
    int totalJumps = 0;
    int deaths = 0;
    int revives = 0;
    float totalPlayTime = 0.f;
};

bool LoadStats(const std::string &path, GameStats &out);
void SaveStats(const std::string &path, const GameStats &in);
