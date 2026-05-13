#include "persistence.h"
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>

int LoadHighScore(const std::string &path){
    std::ifstream ifs(path);
    int v=0; if(ifs) ifs>>v; return v;
}

void SaveHighScore(const std::string &path, int value){
    std::ofstream ofs(path, std::ios::trunc);
    if(ofs) ofs<<value;
}

// Daily highscores stored as: YYYYMMDD score
// Each line is a different day's score
int LoadDailyHighScore(const std::string &path, int year, int month, int day) {
    std::ifstream ifs(path);
    if (!ifs) return 0;
    
    int targetDate = year * 10000 + month * 100 + day;
    std::string line;
    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        int date, score;
        if (iss >> date >> score) {
            if (date == targetDate) {
                return score;
            }
        }
    }
    return 0;
}

void SaveDailyHighScore(const std::string &path, int year, int month, int day, int score) {
    std::map<int, int> scores;
    {
        std::ifstream ifs(path);
        if (ifs) {
            std::string line;
            while (std::getline(ifs, line)) {
                std::istringstream iss(line);
                int date, oldScore;
                if (iss >> date >> oldScore) {
                    scores[date] = oldScore;
                }
            }
        }
    }
    
    int targetDate = year * 10000 + month * 100 + day;
    if (score > scores[targetDate]) {
        scores[targetDate] = score;
    }
    
    std::vector<int> dates;
    for (auto& p : scores) dates.push_back(p.first);
    std::sort(dates.begin(), dates.end(), std::greater<int>());
    
    std::ofstream ofs(path, std::ios::trunc);
    if (ofs) {
        int count = 0;
        for (int d : dates) {
            if (count++ >= 30) break;
            ofs << d << " " << scores[d] << "\n";
        }
    }
}

int LoadGlobalCoins(const std::string &path) {
    std::ifstream ifs(path);
    int v = 0;
    if (ifs) ifs >> v;
    return v;
}

void SaveGlobalCoins(const std::string &path, int value) {
    std::ofstream ofs(path, std::ios::trunc);
    if (ofs) ofs << value;
}

std::vector<std::string> LoadUnlockedAchievements(const std::string &path) {
    std::vector<std::string> result;
    std::ifstream ifs(path);
    if (!ifs) return result;
    std::string line;
    while (std::getline(ifs, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
            line.pop_back();
        if (!line.empty()) result.push_back(line);
    }
    return result;
}

void SaveUnlockedAchievements(const std::string &path, const std::vector<std::string> &ids) {
    std::ofstream ofs(path, std::ios::trunc);
    if (!ofs) return;
    for (auto &id : ids) ofs << id << "\n";
}
