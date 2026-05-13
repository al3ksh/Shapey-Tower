#pragma once
#include <random>

class GameRNG {
    std::mt19937 engine;
public:
    GameRNG(unsigned int seed = 0) : engine(seed ? seed : std::random_device{}()) {}
    void seed(unsigned int s) { engine.seed(s); }
    int nextInt(int min, int max) { return std::uniform_int_distribution<int>(min, max)(engine); }
    int nextInt(int max) { return nextInt(0, max - 1); }
    float nextFloat(float min, float max) { return std::uniform_real_distribution<float>(min, max)(engine); }
    float nextFloat() { return std::uniform_real_distribution<float>(0.f, 1.f)(engine); }
    int nextPercent() { return nextInt(0, 99); }
};
