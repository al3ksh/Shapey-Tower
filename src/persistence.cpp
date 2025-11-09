#include "persistence.h"
#include <fstream>

int LoadHighScore(const std::string &path){
    std::ifstream ifs(path);
    int v=0; if(ifs) ifs>>v; return v;
}

void SaveHighScore(const std::string &path, int value){
    std::ofstream ofs(path, std::ios::trunc);
    if(ofs) ofs<<value;
}
