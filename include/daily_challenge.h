#pragma once
#include <ctime>
#include <string>

enum class DailyChallengeType {
    ICE_WORLD = 0,      
    CRUMBLE_CHAOS,      
    SPRING_MADNESS,     
    NARROW_ESCAPE,      
    DISAPPEARING_ACT,   
    MOVING_MAYHEM,      
    NO_POWERUPS,        
    COIN_RUSH,          
    MIXED_CHAOS,        
    SPEED_DEMON,        
    COUNT               
};

struct DailyChallengeModifiers {
    bool allIce = false;
    bool allCrumbling = false;
    bool allSpring = false;
    bool allDisappearing = false;
    bool allMoving = false;
    bool noPowerUps = false;
    bool extraCoins = false;
    float platformWidthMult = 1.0f;
    float scrollSpeedMult = 1.0f;
    int specialPlatformChance = 0; 
};

struct DailyChallenge {
    unsigned int seed;
    int year, month, day;
    DailyChallengeType type;
    int bestScore = 0;
};

inline DailyChallengeType GetChallengeTypeForSeed(unsigned int seed) {
    return (DailyChallengeType)(seed % (int)DailyChallengeType::COUNT);
}

inline DailyChallenge GetTodaysChallenge() {
    time_t now = time(nullptr);
    tm* t = localtime(&now);
    DailyChallenge dc;
    dc.year = t->tm_year + 1900;
    dc.month = t->tm_mon + 1;
    dc.day = t->tm_mday;
    dc.seed = (unsigned int)(dc.year * 10000 + dc.month * 100 + dc.day);
    dc.type = GetChallengeTypeForSeed(dc.seed);
    return dc;
}

inline std::string GetDailyDateString(const DailyChallenge& dc) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", dc.year, dc.month, dc.day);
    return std::string(buf);
}

inline DailyChallengeModifiers GetChallengeModifiers(DailyChallengeType type) {
    DailyChallengeModifiers m;
    switch(type) {
        case DailyChallengeType::ICE_WORLD:
            m.allIce = true;
            break;
        case DailyChallengeType::CRUMBLE_CHAOS:
            m.allCrumbling = true;
            break;
        case DailyChallengeType::SPRING_MADNESS:
            m.allSpring = true;
            break;
        case DailyChallengeType::NARROW_ESCAPE:
            m.platformWidthMult = 0.5f;
            break;
        case DailyChallengeType::DISAPPEARING_ACT:
            m.allDisappearing = true;
            break;
        case DailyChallengeType::MOVING_MAYHEM:
            m.allMoving = true;
            break;
        case DailyChallengeType::NO_POWERUPS:
            m.noPowerUps = true;
            break;
        case DailyChallengeType::COIN_RUSH:
            m.extraCoins = true;
            break;
        case DailyChallengeType::MIXED_CHAOS:
            m.specialPlatformChance = 40; 
            break;
        case DailyChallengeType::SPEED_DEMON:
            m.scrollSpeedMult = 1.5f;
            break;
        default:
            break;
    }
    return m;
}

namespace Loc {
    const char* Daily_Challenge_IceWorld();
    const char* Daily_Challenge_CrumbleChaos();
    const char* Daily_Challenge_SpringMadness();
    const char* Daily_Challenge_NarrowEscape();
    const char* Daily_Challenge_DisappearingAct();
    const char* Daily_Challenge_MovingMayhem();
    const char* Daily_Challenge_NoPowerups();
    const char* Daily_Challenge_CoinRush();
    const char* Daily_Challenge_MixedChaos();
    const char* Daily_Challenge_SpeedDemon();
    const char* Daily_Desc_IceWorld();
    const char* Daily_Desc_CrumbleChaos();
    const char* Daily_Desc_SpringMadness();
    const char* Daily_Desc_NarrowEscape();
    const char* Daily_Desc_DisappearingAct();
    const char* Daily_Desc_MovingMayhem();
    const char* Daily_Desc_NoPowerups();
    const char* Daily_Desc_CoinRush();
    const char* Daily_Desc_MixedChaos();
    const char* Daily_Desc_SpeedDemon();
}

inline const char* GetChallengeName(DailyChallengeType type) {
    switch(type) {
        case DailyChallengeType::ICE_WORLD: return Loc::Daily_Challenge_IceWorld();
        case DailyChallengeType::CRUMBLE_CHAOS: return Loc::Daily_Challenge_CrumbleChaos();
        case DailyChallengeType::SPRING_MADNESS: return Loc::Daily_Challenge_SpringMadness();
        case DailyChallengeType::NARROW_ESCAPE: return Loc::Daily_Challenge_NarrowEscape();
        case DailyChallengeType::DISAPPEARING_ACT: return Loc::Daily_Challenge_DisappearingAct();
        case DailyChallengeType::MOVING_MAYHEM: return Loc::Daily_Challenge_MovingMayhem();
        case DailyChallengeType::NO_POWERUPS: return Loc::Daily_Challenge_NoPowerups();
        case DailyChallengeType::COIN_RUSH: return Loc::Daily_Challenge_CoinRush();
        case DailyChallengeType::MIXED_CHAOS: return Loc::Daily_Challenge_MixedChaos();
        case DailyChallengeType::SPEED_DEMON: return Loc::Daily_Challenge_SpeedDemon();
        default: return "Unknown";
    }
}

inline const char* GetChallengeDescription(DailyChallengeType type) {
    switch(type) {
        case DailyChallengeType::ICE_WORLD: return Loc::Daily_Desc_IceWorld();
        case DailyChallengeType::CRUMBLE_CHAOS: return Loc::Daily_Desc_CrumbleChaos();
        case DailyChallengeType::SPRING_MADNESS: return Loc::Daily_Desc_SpringMadness();
        case DailyChallengeType::NARROW_ESCAPE: return Loc::Daily_Desc_NarrowEscape();
        case DailyChallengeType::DISAPPEARING_ACT: return Loc::Daily_Desc_DisappearingAct();
        case DailyChallengeType::MOVING_MAYHEM: return Loc::Daily_Desc_MovingMayhem();
        case DailyChallengeType::NO_POWERUPS: return Loc::Daily_Desc_NoPowerups();
        case DailyChallengeType::COIN_RUSH: return Loc::Daily_Desc_CoinRush();
        case DailyChallengeType::MIXED_CHAOS: return Loc::Daily_Desc_MixedChaos();
        case DailyChallengeType::SPEED_DEMON: return Loc::Daily_Desc_SpeedDemon();
        default: return "";
    }
}
