#include "input.h"
#include <string>

const char* KeyName(int key){
    switch(key){
        case KEY_A: return "A"; case KEY_D: return "D"; case KEY_LEFT: return "LEFT"; case KEY_RIGHT: return "RIGHT";
        case KEY_SPACE: return "SPACE"; case KEY_W: return "W"; case KEY_S: return "S"; case KEY_UP: return "UP"; case KEY_DOWN: return "DOWN";
        case KEY_LEFT_SHIFT: return "LSHIFT"; case KEY_RIGHT_SHIFT: return "RSHIFT"; case KEY_ENTER: return "ENTER"; case KEY_TAB: return "TAB"; case KEY_LEFT_CONTROL: return "LCTRL"; case KEY_RIGHT_CONTROL: return "RCTRL"; case KEY_ESCAPE: return "ESC";
        default: return TextFormat("%d", key);
    }
}
