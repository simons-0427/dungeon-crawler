#ifndef SKINS_H
#define SKINS_H

#include "raylib.h"   // Color

// Skin colour lookup by id (0..SKIN_COUNT-1). Animated for rare/epic ids.
Color GetSkinColor(int id);

// Layered skin rendering. "Back" passes draw behind the body circle, "Face"
// passes draw on top. All offsets scale with R so the same code serves the
// in-game player (R~12) and the larger shop preview (R~18).
void DrawEpicBack(int id, float cx, float cy, float R, float t);   // ids 27-29, 32-33
void DrawEpicFace(int id, float cx, float cy, float R, float t);   // ids 27,29,32,33
void DrawRareBack(int id, float cx, float cy, float R, float t);   // ids 22-26, 30-31
void DrawRareFace(int id, float cx, float cy, float R, float t);   // ids 22-26, 30-31
void DrawSkull(float cx, float cy, float R, float t);              // id 28 (replaces body)

// Legendary skin art (achievement-unlocked only, never purchasable).
void DrawLegendaryBack(int id, float cx, float cy, float R, float t); // ids 34-35
void DrawLegendaryFace(int id, float cx, float cy, float R, float t); // ids 34-35

#endif // SKINS_H
