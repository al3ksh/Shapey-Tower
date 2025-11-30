#pragma once
#include <string>

int LoadHighScore(const std::string &path);
void SaveHighScore(const std::string &path, int value);
