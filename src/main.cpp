#include "game.h"

int main(){
    GameConfig cfg;
    Game game(cfg);
    while(!game.ShouldClose()){
        game.Update();
    }
    return 0;
}
