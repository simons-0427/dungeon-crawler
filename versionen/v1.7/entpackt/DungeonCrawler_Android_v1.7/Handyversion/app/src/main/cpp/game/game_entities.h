#ifndef GAME_ENTITIES_H
#define GAME_ENTITIES_H

// Enemy AI and rendering, operating on the shared enes[] array (globals.h).
void UpdEnes(float dt);     // step chase AI; handles player collision/death
void DrawEnemies(float t);  // draw all live enemies (glow + body + eyes)
void DrawEnemyIcon(int cx, int cy, float t);  // one enemy at screen pixels, CELL-sized
void DrawAzureWispPreview(int cx, int cy, float t); // Artifact Room guide image

#endif // GAME_ENTITIES_H
