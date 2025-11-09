// constants.h - various game constants
#pragma once

namespace Const {
    // Render
    inline constexpr int GRADIENT_STEP = 4;          
    inline constexpr float AURA_SCALE = 1.8f;        
    inline constexpr float AURA_OFFSET_X = 0.4f;     
    inline constexpr float AURA_OFFSET_Y = 0.45f;   
    inline constexpr float AURA_GROW_W = 1.8f;       
    inline constexpr float AURA_GROW_H = 1.9f;       

    // HUD
    inline constexpr int HUD_SCORE_FONT = 20;
    inline constexpr int HUD_COMBO_FONT = 18;
    inline constexpr int HUD_THEME_FONT = 36;
    inline constexpr int HUD_CLOCK_RADIUS = 26;      
    inline constexpr int HUD_FPS_OFFSET_X = 120;     
    inline constexpr int HUD_TOP_MARGIN = 10;

    // Combo
    inline constexpr int COMBO_MIN_MULT = 10;        

    // Game Over UI
    inline constexpr int GAMEOVER_PANEL_WIDTH = 340;
    inline constexpr int GAMEOVER_TOP = 140;
    inline constexpr int GAMEOVER_BUTTON_H = 42;
    inline constexpr int GAMEOVER_BUTTON_GAP = 14;
    inline constexpr int GAMEOVER_TITLE_FONT = 34;
    inline constexpr int GAMEOVER_SCORE_FONT = 24;
    inline constexpr int GAMEOVER_BEST_FONT = 20;
}
