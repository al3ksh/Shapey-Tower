#pragma once
#include <string>
#include <vector>

int LoadHighScore(const std::string &path);
void SaveHighScore(const std::string &path, int value);

int LoadDailyHighScore(const std::string &path, int year, int month, int day);
void SaveDailyHighScore(const std::string &path, int year, int month, int day, int score);

int LoadGlobalCoins(const std::string &path);
void SaveGlobalCoins(const std::string &path, int value);

std::vector<std::string> LoadUnlockedAchievements(const std::string &path);
void SaveUnlockedAchievements(const std::string &path, const std::vector<std::string> &ids);
