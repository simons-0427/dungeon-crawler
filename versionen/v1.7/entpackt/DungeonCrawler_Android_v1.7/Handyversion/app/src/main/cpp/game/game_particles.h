#ifndef GAME_PARTICLES_H
#define GAME_PARTICLES_H

#include "raylib.h"   // Color

// Particle system operating on the shared pars[] pool (see globals.h).
void SpawnPar(float x, float y, Color c, float vx, float vy, float life, float sz);
void Burst(float x, float y, Color c, int n, float spd);   // radial burst of n particles
void UpdPars(float dt);                                    // integrate + age particles
void DrawPars(void);                                       // draw all live particles

#endif // GAME_PARTICLES_H
