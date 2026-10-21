#ifndef GAME_SETTINGS_H
#define GAME_SETTINGS_H

// User-preference persistence (SETTINGS_FILE).
// These settings are independent of game progress (shop/stats live elsewhere).
void LoadSettings(void);
void SaveSettings(void);

#endif // GAME_SETTINGS_H
