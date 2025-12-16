#include "localization.h"

static Language currentLanguage = Language::EN;

Language Loc::GetLanguage() { return currentLanguage; }
void Loc::SetLanguage(Language lang) { currentLanguage = lang; }

// Helper macro for bilingual strings
#define LOC(en, pl) (currentLanguage == Language::EN ? (en) : (pl))

// UI - Tabs
const char* Loc::Tab_Game() { return LOC("GAME", "GRA"); }
const char* Loc::Tab_Video() { return "VIDEO"; }
const char* Loc::Tab_Audio() { return "AUDIO"; }
const char* Loc::Tab_Keys() { return LOC("KEYS", "KLAWISZE"); }
const char* Loc::Tab_Effects() { return LOC("EFFECTS", "EFEKTY"); }

// Main Menu - Game tab
const char* Loc::Menu_Title() { return "SHAPEY TOWER"; }
const char* Loc::Menu_StartGame() { return LOC("Start Game", "Rozpocznij gre"); }
const char* Loc::Menu_Difficulty() { return LOC("Difficulty:", "Poziom trudnosci:"); }
const char* Loc::Menu_Easy() { return LOC("EASY", "LATWY"); }
const char* Loc::Menu_Normal() { return LOC("NORMAL", "NORMALNY"); }
const char* Loc::Menu_Hard() { return LOC("HARD", "TRUDNY"); }
const char* Loc::Menu_EasyDesc() { return LOC("Wider platforms, more coins", "Szersze platformy, wiecej monet"); }
const char* Loc::Menu_NormalDesc() { return LOC("Standard settings", "Standardowe ustawienia"); }
const char* Loc::Menu_HardDesc() { return LOC("Narrow platforms, faster pace", "Wezsze platformy, szybsze tempo"); }
const char* Loc::Menu_Play() { return LOC("PLAY", "GRAJ"); }
const char* Loc::Menu_DailyChallenge() { return "Daily Challenge"; }
const char* Loc::Menu_Today() { return LOC("Today:", "Dzis:"); }
const char* Loc::Menu_Exit() { return LOC("EXIT", "WYJSCIE"); }
const char* Loc::Menu_HighScore() { return LOC("High Score:", "Najlepszy wynik:"); }

// Main Menu - Video tab
const char* Loc::Video_Title() { return "Video"; }
const char* Loc::Video_Resolution() { return LOC("Resolution:", "Rozdzielczosc:"); }
const char* Loc::Video_Fullscreen() { return LOC("Fullscreen", "Pelny ekran"); }
const char* Loc::Video_VSync() { return "VSync"; }
const char* Loc::Video_FPSLimit() { return LOC("FPS Limit:", "Limit FPS:"); }
const char* Loc::Video_ShowFPS() { return LOC("Show FPS", "Pokazuj FPS"); }

// Main Menu - Audio tab
const char* Loc::Audio_Title() { return "Audio"; }
const char* Loc::Audio_Master() { return LOC("Master Volume", "Glosnosc glowna"); }
const char* Loc::Audio_Music() { return LOC("Music", "Muzyka"); }
const char* Loc::Audio_Jump() { return LOC("Jump", "Skok"); }
const char* Loc::Audio_Bounce() { return LOC("Bounce", "Odbicie"); }
const char* Loc::Audio_Death() { return LOC("Death", "Smierc"); }
const char* Loc::Audio_ThemeChange() { return LOC("Theme Change", "Zmiana tematu"); }
const char* Loc::Audio_Default() { return LOC("Default", "Domyslne"); }

// Main Menu - Keys tab
const char* Loc::Keys_Title() { return LOC("Controls", "Sterowanie"); }
const char* Loc::Keys_MoveLeft() { return LOC("Move Left", "Ruch w lewo"); }
const char* Loc::Keys_MoveRight() { return LOC("Move Right", "Ruch w prawo"); }
const char* Loc::Keys_Jump() { return LOC("Jump", "Skok"); }
const char* Loc::Keys_PressKey() { return LOC("Press a key...", "Nacisnij klawisz..."); }
const char* Loc::Keys_Default() { return LOC("Default", "Domyslne"); }

// Main Menu - Effects tab
const char* Loc::Effects_Title() { return LOC("Effects", "Efekty"); }
const char* Loc::Effects_ScreenShake() { return LOC("Screen Shake", "Trzesienie ekranu"); }
const char* Loc::Effects_Particles() { return LOC("Particles", "Czasteczki"); }
const char* Loc::Effects_ComboFire() { return LOC("Combo Fire Effect", "Efekt combo (ogien)"); }
const char* Loc::Effects_PowerUp() { return LOC("Power-up Effects", "Efekty power-upow"); }
const char* Loc::Effects_ResetAll() { return LOC("Reset All", "Reset wszystkiego"); }

// Pause Menu
const char* Loc::Pause_Title() { return LOC("PAUSED", "PAUZA"); }
const char* Loc::Pause_Resume() { return LOC("RESUME", "WZNOW"); }
const char* Loc::Pause_Restart() { return LOC("RESTART", "RESTART"); }
const char* Loc::Pause_MainMenu() { return LOC("MAIN MENU", "MENU GLOWNE"); }
const char* Loc::Pause_Exit() { return LOC("EXIT", "WYJSCIE"); }
const char* Loc::Pause_Score() { return LOC("Score:", "Wynik:"); }
const char* Loc::Pause_EscResume() { return LOC("ESC = resume", "ESC = wznow gre"); }

// HUD
const char* Loc::HUD_Score() { return LOC("Score:", "Wynik:"); }
const char* Loc::HUD_Best() { return LOC("Best:", "Najlepszy:"); }
const char* Loc::HUD_Combo() { return "Combo"; }
const char* Loc::HUD_Platforms() { return LOC("Platforms:", "Platformy:"); }
const char* Loc::HUD_Theme() { return LOC("Theme:", "Temat:"); }
const char* Loc::HUD_Speed() { return LOC("Speed", "Predkosc"); }
const char* Loc::HUD_Stage() { return LOC("stage", "etap"); }
const char* Loc::HUD_DoubleJump() { return LOC("2x Jump", "2x Skok"); }
const char* Loc::HUD_Shield() { return LOC("Shield", "Tarcza"); }
const char* Loc::HUD_Slow() { return LOC("Slow", "Spowolnienie"); }
const char* Loc::HUD_Magnet() { return LOC("Magnet", "Magnes"); }

// Game Over
const char* Loc::GameOver_Title() { return "GAME OVER"; }
const char* Loc::GameOver_Score() { return LOC("Score:", "Wynik:"); }
const char* Loc::GameOver_Best() { return LOC("Best:", "Najlepszy:"); }
const char* Loc::GameOver_Restart() { return "Restart"; }
const char* Loc::GameOver_Revive() { return LOC("Revive", "Wskrzes"); }
const char* Loc::GameOver_Coins() { return LOC("Coins:", "Monety:"); }
const char* Loc::GameOver_Menu() { return "Menu"; }
const char* Loc::GameOver_Exit() { return LOC("Exit", "Wyjscie"); }
const char* Loc::GameOver_Cancel() { return LOC("Cancel", "Anuluj"); }

// Settings
const char* Loc::Settings_Language() { return LOC("Language", "Jezyk"); }
const char* Loc::Settings_TabHint() { return LOC("TAB = change tab", "TAB = zmien zakladke"); }

// Daily Challenge
const char* Loc::Daily_Title() { return LOC("Daily Challenge", "Wyzwanie Dnia"); }
const char* Loc::Daily_Best() { return LOC("Daily Best:", "Rekord dnia:"); }

// Challenge names
const char* Loc::Daily_Challenge_IceWorld() { return LOC("Ice World", "Lodowy Swiat"); }
const char* Loc::Daily_Challenge_CrumbleChaos() { return LOC("Crumble Chaos", "Kruchy Chaos"); }
const char* Loc::Daily_Challenge_SpringMadness() { return LOC("Spring Madness", "Sprezyne Szalenstwo"); }
const char* Loc::Daily_Challenge_NarrowEscape() { return LOC("Narrow Escape", "Waska Ucieczka"); }
const char* Loc::Daily_Challenge_DisappearingAct() { return LOC("Disappearing Act", "Znikajacy Akt"); }
const char* Loc::Daily_Challenge_MovingMayhem() { return LOC("Moving Mayhem", "Ruchomy Chaos"); }
const char* Loc::Daily_Challenge_NoPowerups() { return LOC("No Power-ups", "Bez Bonusow"); }
const char* Loc::Daily_Challenge_CoinRush() { return LOC("Coin Rush", "Zlota Goraczka"); }
const char* Loc::Daily_Challenge_MixedChaos() { return LOC("Mixed Chaos", "Mieszany Chaos"); }
const char* Loc::Daily_Challenge_SpeedDemon() { return LOC("Speed Demon", "Demon Predkosci"); }

// Challenge descriptions
const char* Loc::Daily_Desc_IceWorld() { return LOC("All platforms are slippery ice!", "Wszystkie platformy sa sliskim lodem!"); }
const char* Loc::Daily_Desc_CrumbleChaos() { return LOC("Platforms crumble after you land!", "Platformy krusza sie po ladowaniu!"); }
const char* Loc::Daily_Desc_SpringMadness() { return LOC("Bounce on springs everywhere!", "Sprezone platformy wszedzie!"); }
const char* Loc::Daily_Desc_NarrowEscape() { return LOC("Extra narrow platforms!", "Bardzo waskie platformy!"); }
const char* Loc::Daily_Desc_DisappearingAct() { return LOC("Platforms vanish after landing!", "Platformy znikaja po ladowaniu!"); }
const char* Loc::Daily_Desc_MovingMayhem() { return LOC("All platforms are moving!", "Wszystkie platformy sie ruszaja!"); }
const char* Loc::Daily_Desc_NoPowerups() { return LOC("No power-ups, pure skill!", "Bez bonusow, czysta umiejetnosc!"); }
const char* Loc::Daily_Desc_CoinRush() { return LOC("Extra coins everywhere!", "Dodatkowe monety wszedzie!"); }
const char* Loc::Daily_Desc_MixedChaos() { return LOC("Random special platforms!", "Losowe specjalne platformy!"); }
const char* Loc::Daily_Desc_SpeedDemon() { return LOC("Faster scrolling speed!", "Szybsze tempo gry!"); }
