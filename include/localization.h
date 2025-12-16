#pragma once

enum class Language { EN = 0, PL = 1 };

namespace Loc {

// Get/Set current language
Language GetLanguage();
void SetLanguage(Language lang);

// UI - Tabs
const char* Tab_Game();
const char* Tab_Video();
const char* Tab_Audio();
const char* Tab_Keys();
const char* Tab_Effects();

// Main Menu - Game tab
const char* Menu_Title();
const char* Menu_StartGame();
const char* Menu_Difficulty();
const char* Menu_Easy();
const char* Menu_Normal();
const char* Menu_Hard();
const char* Menu_EasyDesc();
const char* Menu_NormalDesc();
const char* Menu_HardDesc();
const char* Menu_Play();
const char* Menu_DailyChallenge();
const char* Menu_Today();
const char* Menu_Exit();
const char* Menu_HighScore();

// Main Menu - Video tab
const char* Video_Title();
const char* Video_Resolution();
const char* Video_Fullscreen();
const char* Video_VSync();
const char* Video_FPSLimit();
const char* Video_ShowFPS();

// Main Menu - Audio tab
const char* Audio_Title();
const char* Audio_Master();
const char* Audio_Music();
const char* Audio_Jump();
const char* Audio_Bounce();
const char* Audio_Death();
const char* Audio_ThemeChange();
const char* Audio_Default();

// Main Menu - Keys tab
const char* Keys_Title();
const char* Keys_MoveLeft();
const char* Keys_MoveRight();
const char* Keys_Jump();
const char* Keys_PressKey();
const char* Keys_Default();

// Main Menu - Effects tab
const char* Effects_Title();
const char* Effects_ScreenShake();
const char* Effects_Particles();
const char* Effects_ComboFire();
const char* Effects_PowerUp();
const char* Effects_ResetAll();

// Pause Menu
const char* Pause_Title();
const char* Pause_Resume();
const char* Pause_Restart();
const char* Pause_MainMenu();
const char* Pause_Exit();
const char* Pause_Score();
const char* Pause_EscResume();

// HUD
const char* HUD_Score();
const char* HUD_Best();
const char* HUD_Combo();
const char* HUD_Platforms();
const char* HUD_Theme();
const char* HUD_Speed();
const char* HUD_Stage();
const char* HUD_DoubleJump();
const char* HUD_Shield();
const char* HUD_Slow();
const char* HUD_Magnet();

// Game Over
const char* GameOver_Title();
const char* GameOver_Score();
const char* GameOver_Best();
const char* GameOver_Restart();
const char* GameOver_Revive();
const char* GameOver_Coins();
const char* GameOver_Menu();
const char* GameOver_Exit();
const char* GameOver_Cancel();

// Settings
const char* Settings_Language();
const char* Settings_TabHint();

// Daily Challenge
const char* Daily_Title();
const char* Daily_Best();
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

} // namespace Loc
