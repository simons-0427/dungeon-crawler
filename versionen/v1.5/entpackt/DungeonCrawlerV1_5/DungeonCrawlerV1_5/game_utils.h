#ifndef GAME_UTILS_H
#define GAME_UTILS_H

#include "raylib.h"   // Vector2, Rectangle, Color

// Star rendering (used by the achievements screen).
void DrawCustomStar(float cx, float cy, float outerRadius, float innerRadius, Color color);
void DrawCustomStarLines(float cx, float cy, float outerRadius, float innerRadius, Color color);

// Camera / window helpers.
void UpdateCameraOffset(void);     // recentres cam.offset on the current screen
void ToggleFullscreenNow(void);    // fullscreen <-> windowed, keeping cam centred

// Thin wrapper around CheckCollisionPointRec for readability at call sites.
bool PointInRect(Vector2 p, Rectangle r);

#endif // GAME_UTILS_H
