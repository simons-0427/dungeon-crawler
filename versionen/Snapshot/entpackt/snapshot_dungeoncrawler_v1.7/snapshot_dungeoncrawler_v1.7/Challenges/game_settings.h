#ifndef GAME_SETTINGS_H
#define GAME_SETTINGS_H
#include <stdbool.h>

// User-preference persistence (SETTINGS_FILE).
// These settings are independent of game progress (shop/stats live elsewhere).
#ifdef DEBUG_ROOMS
extern int debugSecretChance, debugPortalChance;
void LoadDebugRoomSettings(void);
void SaveDebugRoomSettings(void);
void SetDebugRoomChance(bool portal, int value);
#endif
void LoadSettings(void);
void SaveSettings(void);
void ApplyColorblindMode(void);

// Keybind helpers. The debug build still stores binds but ActionDown/Pressed
// ignore custom keys so WASD + arrows-for-round-jump stay predictable.
void ResetKeybindsToDefault(void);
bool ActionDown(int act);
bool ActionPressed(int act);
bool ActionPressedKey(int act);         // keyboard only — skips mouse binds (menu UI)
int  CountKeybinds(int act);
int  FindActionWithKey(int key);                 // first ACT_* that owns this bind, or -1
int  FindConflictingAction(int key, int act);    // -1 if this key can share with act
bool AddKeybind(int act, int key);               // false if simultaneous-conflict / full / invalid
bool ReplaceKeybind(int act, int index, int key); // rewrite one slot; false on conflict
bool RemoveKeybind(int act, int index); // false if it would leave the action empty
void FormatKeyName(int key, char *buf, int n);
void FormatActionBinds(int act, char *buf, int n); // "W / UP" / "ENTER / LMB"
void FormatMoveHint(char *buf, int n);             // first bind of each dir: "W/A/S/D"
void FormatUseSlotHint(char *buf, int n);          // "1-5" or "1/2/3/4/5"

#endif // GAME_SETTINGS_H
