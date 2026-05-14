#include "leaderboard.h"
#include <fstream>
#include <sstream>
#include <algorithm>

void LoadLeaderboard(const std::string &path, Leaderboard &lb) {
    lb.entries.clear();
    std::ifstream ifs(path);
    if (!ifs) return;
    std::string line;
    while (std::getline(ifs, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
            line.pop_back();
        if (line.empty()) continue;
        LeaderboardEntry e{};
        e.isDaily = false;
        char dailyFlag = 0;
        if (sscanf(line.c_str(), "%d %d %d %c", &e.score, &e.coins, &e.combo, &dailyFlag) >= 3) {
            if (dailyFlag == 'D') e.isDaily = true;
            lb.entries.push_back(e);
        }
    }
}

void SaveLeaderboard(const std::string &path, const Leaderboard &lb) {
    std::ofstream ofs(path, std::ios::trunc);
    if (!ofs) return;
    for (auto &e : lb.entries) {
        ofs << e.score << " " << e.coins << " " << e.combo;
        if (e.isDaily) ofs << " D";
        ofs << "\n";
    }
}

void AddLeaderboardEntry(Leaderboard &lb, const LeaderboardEntry &entry) {
    lb.entries.push_back(entry);
    std::sort(lb.entries.begin(), lb.entries.end(),
              [](const LeaderboardEntry &a, const LeaderboardEntry &b) {
                  return a.score > b.score;
              });
    if ((int)lb.entries.size() > Leaderboard::MAX_ENTRIES) {
        lb.entries.resize(Leaderboard::MAX_ENTRIES);
    }
}
