#pragma once
#include <string>
#include <vector>
#include <functional>

struct Achievement {
    std::string id;
    std::string name;
    std::string description;
    bool unlocked = false;
    std::function<bool(int score, int combo, int coins, int platformsClimbed)> condition;
};

inline std::vector<Achievement> CreateAchievements() {
    std::vector<Achievement> achievements;
    
    achievements.push_back({"first_jump", "First Steps", "Complete your first jump", false,
        [](int, int, int, int platforms) { return platforms >= 1; }});
    
    achievements.push_back({"reach_100", "Getting Started", "Reach 100 points", false,
        [](int score, int, int, int) { return score >= 100; }});
    
    achievements.push_back({"reach_500", "Climber", "Reach 500 points", false,
        [](int score, int, int, int) { return score >= 500; }});
    
    achievements.push_back({"reach_1000", "Sky High", "Reach 1000 points", false,
        [](int score, int, int, int) { return score >= 1000; }});
    
    achievements.push_back({"reach_5000", "To the Moon", "Reach 5000 points", false,
        [](int score, int, int, int) { return score >= 5000; }});
    
    achievements.push_back({"combo_5", "Combo Starter", "Get a 5x combo", false,
        [](int, int combo, int, int) { return combo >= 5; }});
    
    achievements.push_back({"combo_10", "Combo Master", "Get a 10x combo", false,
        [](int, int combo, int, int) { return combo >= 10; }});
    
    achievements.push_back({"combo_20", "Combo Legend", "Get a 20x combo", false,
        [](int, int combo, int, int) { return combo >= 20; }});
    
    achievements.push_back({"coins_50", "Coin Collector", "Collect 50 coins total", false,
        [](int, int, int coins, int) { return coins >= 50; }});
    
    achievements.push_back({"coins_200", "Treasure Hunter", "Collect 200 coins total", false,
        [](int, int, int coins, int) { return coins >= 200; }});
    
    achievements.push_back({"platforms_100", "Marathon", "Climb 100 platforms in one run", false,
        [](int, int, int, int platforms) { return platforms >= 100; }});
    
    return achievements;
}
