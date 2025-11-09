// input.h - mapowanie klawiszy i funkcja nazewnictwa
#pragma once
#include "raylib.h"

struct KeyBindings { int left=KEY_A; int right=KEY_D; int jump=KEY_SPACE; };

const char* KeyName(int key);
