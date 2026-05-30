#pragma once
#include <string>

// Player skins = rows of assets/textures/player_sheet.png (tools/make_player_sprite.py SKINS).
struct SkinInfo {
    const char *name;
    int price;
    const char *descEN;
    const char *descPL;
};

inline constexpr SkinInfo kSkins[] = {
    {"The Shape", 0, "The original masked climber", "Oryginalny zamaskowany wspinacz"},
    {"Ember", 60, "Flaming crest, burning eyes", "Plonacy grzebien, zarzace oczy"},
    {"Frost", 120, "Icicle crown and frozen gear", "Korona z sopli, zamarzniety stroj"},
    {"Toxic", 180, "Gas mask and a glowing tank", "Maska gazowa i swiecacy zbiornik"},
    {"Royal", 300, "Gold crown and a royal cape", "Zlota korona i krolewska peleryna"},
    {"Bone", 500, "Skull face, ribs on show", "Czaszka zamiast twarzy i zebra"},
    {"Glitch", 800, "Corrupted visor, broken signal", "Uszkodzony wizjer, zerwany sygnal"},
};
inline constexpr int SKIN_COUNT = (int)(sizeof(kSkins) / sizeof(kSkins[0]));

struct SkinState {
    int selected = 0;
    unsigned int owned = 1u; // bit i = skin i unlocked; the default skin is always owned
    bool Owns(int i) const { return (owned >> i) & 1u; }
};

SkinState LoadSkins(const std::string &path);
void SaveSkins(const std::string &path, const SkinState &s);
