#pragma once
#include <string>

int LoadHighScore(const std::string &path);
void SaveHighScore(const std::string &path, int value);

int LoadDailyHighScore(const std::string &path, int year, int month, int day);
void SaveDailyHighScore(const std::string &path, int year, int month, int day, int score);

int LoadGlobalCoins(const std::string &path);
void SaveGlobalCoins(const std::string &path, int value);
