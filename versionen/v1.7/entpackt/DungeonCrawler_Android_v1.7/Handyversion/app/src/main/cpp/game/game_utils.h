#ifndef GAME_UTILS_H
#define GAME_UTILS_H

#include "raylib.h"   // Vector2, Rectangle, Color

// Star rendering (used by the achievements screen).
void DrawCustomStar(float cx, float cy, float outerRadius, float innerRadius, Color color);
void DrawCustomStarLines(float cx, float cy, float outerRadius, float innerRadius, Color color);

// Camera / window helpers.
void UpdateCameraOffset(void);     // recentres cam.offset on the current screen
void AddScreenShake(float mag, float dur); // no-op if screen_shake is off
void ToggleFullscreenNow(void);    // borderless fullscreen <-> windowed
bool GameWindowIsFullscreen(void);

// Window geometry persistence (SETTINGS_FILE). Capture on exit / resize;
// Apply after InitWindow. Fullscreen keeps the last windowed x/y/w/h.
void SetPersistedWindowState(int fullscreen, int x, int y, int w, int h);
void GetPersistedWindowState(int *fullscreen, int *x, int *y, int *w, int *h);
void ApplyPersistedWindowState(void);
void CaptureWindowState(void);

// Thin wrapper around CheckCollisionPointRec for readability at call sites.
bool PointInRect(Vector2 p, Rectangle r);

#endif // GAME_UTILS_H
