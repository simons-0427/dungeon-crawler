#ifndef GAME_UI_H
#define GAME_UI_H

#include "raylib.h"   // Rectangle

// ─── HUD / PAUSE LAYOUT (queried by the main loop for input hit-testing) ──────
void GetPauseMenuLayout(Rectangle *panel, Rectangle *resumeBtn, Rectangle *menuBtn);
Rectangle GetPauseButtonRect(void);
Rectangle GetInventorySlotRect(int slot);
void GetOverlayButtonLayout(Rectangle *primaryBtn, Rectangle *secondaryBtn);
void GetMenuIconRects(Rectangle *settingsR, Rectangle *futureR);
Rectangle GetMenuSubPanelRect(void);
Rectangle GetMenuBackButtonRect(void);
Rectangle GetSettingsResetButtonRect(void);

// ─── SCREENS ──────────────────────────────────────────────────────────────────
void DrawHUD(float t);        // in-game heads-up display
void DrawPauseMenu(void);     // pause overlay panel
void DrawMenu(float t);       // main menu + all sub-screens (tutorial/stats/shop/...)
void DrawOverlay(float t);    // death / next-round overlay

#endif // GAME_UI_H
