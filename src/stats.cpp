#include "stats.h"
#include <fstream>
#include <sstream>

bool LoadStats(const std::string &path, GameStats &out) {
    std::ifstream ifs(path);
    if (!ifs) return false;
    std::string line;
    while (std::getline(ifs, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
            line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        auto sep = line.find('=');
        if (sep == std::string::npos) continue;
        std::string key = line.substr(0, sep);
        std::string val = line.substr(sep + 1);
        try {
            long long v = std::stoll(val);
            if (key == "gamesPlayed") out.gamesPlayed = (int)v;
            else if (key == "totalScore") out.totalScore = (int)v;
            else if (key == "bestScore") out.bestScore = (int)v;
            else if (key == "totalCoinsCollected") out.totalCoinsCollected = (int)v;
            else if (key == "bestCombo") out.bestCombo = (int)v;
            else if (key == "totalPlatformsLanded") out.totalPlatformsLanded = (int)v;
            else if (key == "bestPlatformStreak") out.bestPlatformStreak = (int)v;
            else if (key == "totalPowerUpsCollected") out.totalPowerUpsCollected = (int)v;
            else if (key == "totalJumps") out.totalJumps = (int)v;
            else if (key == "deaths") out.deaths = (int)v;
            else if (key == "revives") out.revives = (int)v;
        } catch (...) {}
        try {
            double fv = std::stod(val);
            if (key == "totalPlayTime") out.totalPlayTime = (float)fv;
        } catch (...) {}
    }
    return true;
}

void SaveStats(const std::string &path, const GameStats &in) {
    std::ofstream ofs(path, std::ios::trunc);
    if (!ofs) return;
    ofs << "gamesPlayed=" << in.gamesPlayed << "\n";
    ofs << "totalScore=" << in.totalScore << "\n";
    ofs << "bestScore=" << in.bestScore << "\n";
    ofs << "totalCoinsCollected=" << in.totalCoinsCollected << "\n";
    ofs << "bestCombo=" << in.bestCombo << "\n";
    ofs << "totalPlatformsLanded=" << in.totalPlatformsLanded << "\n";
    ofs << "bestPlatformStreak=" << in.bestPlatformStreak << "\n";
    ofs << "totalPowerUpsCollected=" << in.totalPowerUpsCollected << "\n";
    ofs << "totalJumps=" << in.totalJumps << "\n";
    ofs << "deaths=" << in.deaths << "\n";
    ofs << "revives=" << in.revives << "\n";
    ofs << "totalPlayTime=" << in.totalPlayTime << "\n";
}
