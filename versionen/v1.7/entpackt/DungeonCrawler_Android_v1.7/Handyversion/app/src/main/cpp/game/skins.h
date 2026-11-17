#ifndef SKINS_H
#define SKINS_H

#include "raylib.h"   // Color

// Skin colour lookup by id (0..SKIN_COUNT-1). Animated for rare/epic ids.
Color GetSkinColor(int id);

// Layered skin rendering. "Back" passes draw behind the body circle, "Face"
// passes draw on top. All offsets scale with R so the same code serves the
// in-game player (R~12) and the larger shop preview (R~18).
void DrawEpicBack(int id, float cx, float cy, float R, float t);   // ids 27,29,32,38
void DrawEpicFace(int id, float cx, float cy, float R, float t);   // ids 27,29,32
void DrawRareBack(int id, float cx, float cy, float R, float t);   // ids 22-26,28,30-35
void DrawRareFace(int id, float cx, float cy, float R, float t);   // ids 22-26,28,30-35
void DrawSkull(float cx, float cy, float R, float t);              // id 28 (replaces body)
void DrawSkullvoidCrowned(float cx, float cy, float R, float t);   // id 38 (replaces body)
void DrawPoptartCat(float cx, float cy, float R, float t);         // id 36 (replaces body)
void DrawPoptartCatInGame(float cx, float cy, float R, float t, bool flipX); // id 36, larger + directional
void DrawApexRex(float cx, float cy, float R, float t);            // id 37 (replaces body)
void DrawApexRexInGame(float cx, float cy, float R, float t, bool flipX); // id 37, larger + directional
void DrawApexBladelord(float cx, float cy, float R, float t);      // id 33, exact Skin-Ideas #27
void DrawBloodscaleVanguard(float cx, float cy, float R, float t); // id 39, exact Skin-Ideas #23
void DrawStormforgedAegis(float cx, float cy, float R, float t);   // id 40, exact Skin-Ideas #24
void DrawSunforgedCrusader(float cx, float cy, float R, float t);  // id 41, exact Skin-Ideas #25
void DrawBlackHole(float cx, float cy, float R, float t);          // id 42, exact Skin-Ideas #26

// Shared constellation/ascendant art, now called from the Rare category.
void DrawLegendaryBack(int id, float cx, float cy, float R, float t); // ids 34-35
void DrawLegendaryFace(int id, float cx, float cy, float R, float t); // ids 34-35
void DrawConstellation(float cx, float cy, float R, float t);         // id 34, unchanged composition
void DrawAcceptedSkin(int id, float cx, float cy, float R, float t);  // ids 43-48, design ideas
void DrawSelectedFixSkin(int id, float cx, float cy, float R);         // ids 49-51, accepted fixes

#endif // SKINS_H
