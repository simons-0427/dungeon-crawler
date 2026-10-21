// game_particles.c — particle pool spawning, burst effects, update & draw.
#include "game_particles.h"
#include "globals.h"     // pars[]
#include "constants.h"   // MAX_PAR
#include "types.h"       // uchar, Par
#include <math.h>

void SpawnPar(float x, float y, Color c,
              float vx, float vy, float life, float sz) {
    for (int i = 0; i < MAX_PAR; i++) {
        if (pars[i].life <= 0) {
            pars[i] = (Par){x,y,vx,vy,life,life,c,sz};
            return;
        }
    }
}

void Burst(float x, float y, Color c, int n, float spd) {
    static const float DS[3] = {0.5f, 1.0f, 2.0f};  // Low / Med / High scale
    int pd = par_density;
    if (pd < 0) pd = 0; else if (pd > 2) pd = 2;
    int count = (int)(n * DS[pd]);
    if (count < 1) count = 1;
    for (int i = 0; i < count; i++) {
        float a = (float)i / count * (2*PI) + GetRandomValue(0,100)*0.063f;
        float s = spd * (0.5f + GetRandomValue(0,100)*0.005f);
        float lf = 0.4f + GetRandomValue(0,60)*0.01f;
        float sz = (float)(2 + GetRandomValue(0,3));
        SpawnPar(x, y, c, cosf(a)*s, sinf(a)*s, lf, sz);
    }
}

void UpdPars(float dt) {
    for (int i = 0; i < MAX_PAR; i++) {
        Par *p = &pars[i]; if (p->life <= 0) continue;
        p->x += p->vx * dt * 60;
        p->y += p->vy * dt * 60;
        p->vy += 0.05f * dt * 60;
        p->life -= dt;
    }
}

void DrawPars(void) {
    for (int i = 0; i < MAX_PAR; i++) {
        Par *p = &pars[i]; if (p->life <= 0) continue;
        float a = p->life / p->ml;
        Color c = p->c; c.a = (uchar)(a * 255);
        DrawCircleV((Vector2){p->x, p->y}, p->sz * a + 1.0f, c);
    }
}
