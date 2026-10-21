#ifndef GAME_MAP_H
#define GAME_MAP_H

#include "constants.h"   // CELL (for the inline tile<->world helpers)

// Tile-centre to world-pixel conversions. Kept inline in the header so every
// module (player, entities, ...) can use them without a call.
static inline float WX(int tx) { return tx * CELL + CELL * 0.5f; }
static inline float WY(int ty) { return ty * CELL + CELL * 0.5f; }

// Procedural round generation: fills map[][], places treasure/portals/coins/
// items/enemies and resets the player & camera for the current roundn.
void GenRound(void);

// Rendering.
void DrawTile(int x, int y, float t);   // one tile at grid (x,y)
void DrawMap(float t);                   // whole visible map (loops DrawTile)

// Challenge-room mechanics (V1.6).
bool ArrowLaneLethal(void);     // RK_TRAP: are the arrow lanes ('r') firing this instant?
void UpdLava(float dt);         // RK_LAVA: advance the rising lava front
void DrawDarkness(void);        // RK_DARK: fog overlay + treasure/portal glow

#endif // GAME_MAP_H
