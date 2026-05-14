#pragma once
#include <string>
#include <vector>

struct LeaderboardEntry {
    int score;
    int coins;
    int combo;
    bool isDaily;
};

struct Leaderboard {
    static constexpr int MAX_ENTRIES = 10;
    std::vector<LeaderboardEntry> entries;
};

void LoadLeaderboard(const std::string &path, Leaderboard &lb);
void SaveLeaderboard(const std::string &path, const Leaderboard &lb);
void AddLeaderboardEntry(Leaderboard &lb, const LeaderboardEntry &entry);
