#pragma once

enum class Difficulty {
    EASY,
    NORMAL,
    HARD
};

struct DifficultySettings {
    float gapMultiplier;
    float scrollSpeedMultiplier;
    float platformWidthBonus;
    int coinSpawnChance;
    int powerUpSpawnChance;
    const char* name;
};

inline DifficultySettings GetDifficultySettings(Difficulty d) {
    switch(d) {
        case Difficulty::EASY:
            return {1.3f, 0.7f, 30.f, 20, 8, "Easy"};
        case Difficulty::NORMAL:
            return {1.0f, 1.0f, 0.f, 15, 5, "Normal"};
        case Difficulty::HARD:
            return {0.8f, 1.3f, -20.f, 10, 3, "Hard"};
        default:
            return {1.0f, 1.0f, 0.f, 15, 5, "Normal"};
    }
}
