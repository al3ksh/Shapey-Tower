#include "audio.h"
#include <string>

static Sound TryLoadSoundMulti(const char* base){
    // Order: mp3, ogg, wav; search assets/audio/<base><ext> then fallback to working dir
    const char* exts[] = {".mp3",".ogg",".wav"};
    Sound s{};
    for(auto &ext: exts){
        std::string path = std::string("assets/audio/")+base+ext;
        if(FileExists(path.c_str())){ s = LoadSound(path.c_str()); if(s.frameCount>0) return s; }
        path = std::string(base)+ext;
        if(FileExists(path.c_str())){ s = LoadSound(path.c_str()); if(s.frameCount>0) return s; }
    }
    return s;
}

static Music TryLoadMusicMulti(const char* base){
    const char* exts[] = {".mp3",".ogg",".wav"};
    Music m{};
    for(auto &ext: exts){
        std::string path = std::string("assets/audio/")+base+ext;
        if(FileExists(path.c_str())){ m = LoadMusicStream(path.c_str()); if(m.ctxData) return m; }
        path = std::string(base)+ext;
        if(FileExists(path.c_str())){ m = LoadMusicStream(path.c_str()); if(m.ctxData) return m; }
    }
    return m;
}

void LoadGameAudio(GameAudio &audio){
    audio.sndJump = TryLoadSoundMulti("jump");
    audio.sndBounce = TryLoadSoundMulti("bounce");
    audio.sndDeath = TryLoadSoundMulti("death");
    audio.sndThemeChange = TryLoadSoundMulti("theme-change");
    audio.musicBg = TryLoadMusicMulti("background-music");
}

void UnloadGameAudio(GameAudio &audio){
    if(audio.sndJump.frameCount>0) UnloadSound(audio.sndJump);
    if(audio.sndBounce.frameCount>0) UnloadSound(audio.sndBounce);
    if(audio.sndDeath.frameCount>0) UnloadSound(audio.sndDeath);
    if(audio.sndThemeChange.frameCount>0) UnloadSound(audio.sndThemeChange);
    if(audio.musicBg.ctxData) UnloadMusicStream(audio.musicBg);
}
