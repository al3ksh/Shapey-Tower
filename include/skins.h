#pragma once
#include <string>

// Player skins = rows of assets/textures/player_sheet.png (tools/make_player_sprite.py SKINS).
struct SkinInfo {
    const char *name;
    int price;
};

inline constexpr SkinInfo kSkins[] = {
    {"The Shape", 0}, {"Ember", 60}, {"Frost", 120}, {"Toxic", 180}, {"Royal", 300}, {"Bone", 500}, {"Glitch", 800},
};
inline constexpr int SKIN_COUNT = (int)(sizeof(kSkins) / sizeof(kSkins[0]));

struct SkinState {
    int selected = 0;
    unsigned int owned = 1u; // bit i = skin i unlocked; the default skin is always owned
    bool Owns(int i) const { return (owned >> i) & 1u; }
};

SkinState LoadSkins(const std::string &path);
void SaveSkins(const std::string &path, const SkinState &s);
