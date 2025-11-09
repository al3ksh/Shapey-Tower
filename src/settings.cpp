#include "settings.h"
#include <fstream>
#include <sstream>
#include <cctype>

static std::string Trim(const std::string &s){ size_t a=0; while(a<s.size() && isspace((unsigned char)s[a])) a++; size_t b=s.size(); while(b>a && isspace((unsigned char)s[b-1])) b--; return s.substr(a,b-a); }

bool LoadSettings(const std::string &path, GameSettings &out){
    std::ifstream ifs(path);
    if(!ifs) return false;
    std::string line; int parsedMask=0; // bitfield for optional diagnostics
    while(std::getline(ifs,line)){
        line=Trim(line);
        if(line.empty()||line[0]=='#') continue;
        auto eq=line.find('='); if(eq==std::string::npos) continue;
        std::string k=Trim(line.substr(0,eq));
        std::string v=Trim(line.substr(eq+1));
        try {
            if(k=="configVersion") { out.configVersion = std::stoi(v); parsedMask|=1; }
            else if(k=="resolutionIndex") { out.resolutionIndex=std::stoi(v); parsedMask|=2; }
            else if(k=="fullscreen") { out.fullscreen=(v=="1"||v=="true"); parsedMask|=4; }
            else if(k=="master") { out.master=std::stof(v); parsedMask|=8; }
            else if(k=="music") { out.music=std::stof(v); parsedMask|=16; }
            else if(k=="jump") { out.jump=std::stof(v); parsedMask|=32; }
            else if(k=="bounce") { out.bounce=std::stof(v); parsedMask|=64; }
            else if(k=="death") { out.death=std::stof(v); parsedMask|=128; }
            else if(k=="theme") { out.theme=std::stof(v); parsedMask|=256; }
            else if(k=="keyLeft") { out.keyLeft=std::stoi(v); parsedMask|=512; }
            else if(k=="keyRight") { out.keyRight=std::stoi(v); parsedMask|=1024; }
            else if(k=="keyJump") { out.keyJump=std::stoi(v); parsedMask|=2048; }
            else if(k=="showFPS") { out.showFPS=(v=="1"||v=="true"); parsedMask|=4096; }
    } catch(...){ /* ignore individual conversion errors */ }
    }
    // Version left as-is; future migrations can map here. Clamp floats to 0..1.
    auto clamp01=[](float x){ return x<0.f?0.f:(x>1.f?1.f:x); };
    out.master=clamp01(out.master); out.music=clamp01(out.music); out.jump=clamp01(out.jump); out.bounce=clamp01(out.bounce); out.death=clamp01(out.death); out.theme=clamp01(out.theme);
    if(out.resolutionIndex<0) out.resolutionIndex=0;
    // Harden key codes: fallback to defaults if outside Raylib range (32..348).
    auto validKey=[](int k){ return k>=32 && k<=348; };
    if(!validKey(out.keyLeft)) out.keyLeft=KEY_A;
    if(!validKey(out.keyRight)) out.keyRight=KEY_D;
    if(!validKey(out.keyJump)) out.keyJump=KEY_SPACE;
    return true;
}

bool SaveSettings(const std::string &path, const GameSettings &in){
    std::ofstream ofs(path, std::ios::trunc);
    if(!ofs) return false;
    ofs << "configVersion="<< in.configVersion <<"\n";
    ofs << "resolutionIndex="<< in.resolutionIndex <<"\n";
    ofs << "fullscreen="<< (in.fullscreen?1:0) <<"\n";
    ofs << "master="<< in.master <<"\n";
    ofs << "music="<< in.music <<"\n";
    ofs << "jump="<< in.jump <<"\n";
    ofs << "bounce="<< in.bounce <<"\n";
    ofs << "death="<< in.death <<"\n";
    ofs << "theme="<< in.theme <<"\n";
    ofs << "keyLeft="<< in.keyLeft <<"\n";
    ofs << "keyRight="<< in.keyRight <<"\n";
    ofs << "keyJump="<< in.keyJump <<"\n";
    ofs << "showFPS="<< (in.showFPS?1:0) <<"\n";
    return true;
}
