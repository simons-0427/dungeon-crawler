#ifndef GAME_UI_H
#define GAME_UI_H

#include "raylib.h"   // Rectangle

// ─── HUD / PAUSE LAYOUT (queried by the main loop for input hit-testing) ──────
void GetPauseMenuLayout(Rectangle *panel, Rectangle *resumeBtn, Rectangle *settingsBtn, Rectangle *menuBtn);
void GetPauseLeaveConfirmLayout(Rectangle *panel, Rectangle *confirmBtn, Rectangle *cancelBtn);
bool PauseSettingsOpen(void);
void PauseSettingsSet(bool open);
bool PauseLeaveConfirmOpen(void);
void PauseLeaveConfirmSet(bool open);
Rectangle GetPauseButtonRect(void);
void GetMenuIconRects(Rectangle *settingsR, Rectangle *musicR, Rectangle *futureR);

enum {
    MN_SETTINGS = 0,
    MN_MUSIC,
    MN_FUTURE,
    MN_TUTORIAL,
    MN_STATS,
    MN_PLAY,
    MN_SHOP,
    MN_ACHIEVE,
    MN_CHAL,
    MN_COUNT
};
void GetMainMenuRects(Rectangle out[MN_COUNT]);
bool MenuNavIsOn(void);
int  MenuNavFocus(void);
void MenuNavExit(void);
void UpdateMenuNav(void);            // Tab / arrows; Escape is handled by the caller
int  MenuNavActivatePressed(void);   // MN_* on home, -2 = BACK, -1 = none/consumed
void TutorialBookReset(void);        // start the tutorial at the front cover / pages 1-2
void MenuPlayBegin(void);            // start PLAY door enter anim
bool MenuPlayBusy(void);             // true while the door anim is running
bool MenuPlayShouldStart(void);      // true once when the anim should launch the game
void UpdateMenuPlayAnim(void);

int GetInventorySlotCount(void);
Rectangle GetInventorySlotRect(int slot);
bool SettingsCapturingKey(void);
void SettingsCancelCapture(void);
bool SettingsDeleteMode(void);
void SettingsCancelDelete(void);

// ─── SCREENS ──────────────────────────────────────────────────────────────────
#ifdef DEBUG_ROOMS
bool UpdateDebugRoomPanel(void);
void DrawDebugRoomPanel(void);
#endif
void DrawHUD(float t);        // in-game heads-up display
void DrawPauseMenu(void);     // pause overlay panel
void DrawMenu(float t);       // main menu + all sub-screens (tutorial/stats/shop/...)
void DrawOverlay(float t);    // death / next-round overlay

#endif // GAME_UI_H
