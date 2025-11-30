#pragma once
#include <ctime>
#include <string>

struct DailyChallenge {
    unsigned int seed;
    int year, month, day;
    int bestScore = 0;
    bool played = false;
};

inline DailyChallenge GetTodaysChallenge() {
    time_t now = time(nullptr);
    tm* t = localtime(&now);
    DailyChallenge dc;
    dc.year = t->tm_year + 1900;
    dc.month = t->tm_mon + 1;
    dc.day = t->tm_mday;
    dc.seed = (unsigned int)(dc.year * 10000 + dc.month * 100 + dc.day);
    return dc;
}

inline std::string GetDailyDateString(const DailyChallenge& dc) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", dc.year, dc.month, dc.day);
    return std::string(buf);
}
