// game_entities.c — enemy chase AI and enemy rendering.
#include "game_entities.h"
#include "globals.h"
#include "constants.h"
#include "types.h"
#include "game_particles.h"  // Burst
#include "game_save.h"       // AddEnemyKill, UpdateAndSaveStats
#include "game_sfx.h"        // PlaySfx
#include "game_map.h"        // WX, WY
#include "game_player.h"     // GoldTouchHitEnemy
#include <math.h>

// ─── ENEMY AI ─────────────────────────────────────────────────────────────────
void UpdEnes(float dt) {
    if (!eneon) return;
    for (int i = 0; i < MAX_ENE; i++) {
        Ene *e = &enes[i]; if (!e->on) continue;
        e->mt += dt;
        if (e->mt < e->cd) continue;
        e->mt = 0;

        int ex = (int)(e->x + 0.5f), ey = (int)(e->y + 0.5f);
        int ddx = (px > ex) ? 1 : (px < ex) ? -1 : 0;
        int ddy = (py > ey) ? 1 : (py < ey) ? -1 : 0;

        bool yFirst = (GetRandomValue(0,3) == 0);
        bool moved = false;

        // Walls and lava both block enemies.
        #define ENE_BLOCKED(c) ((c)=='#' || (c)=='L')
        if (!yFirst && ddx) {
            int nx = ex+ddx;
            if (nx>=0&&nx<mw && !ENE_BLOCKED(map[ey][nx])) { e->x=(float)nx; moved=true; }
        }
        if (!moved && ddy) {
            int ny = ey+ddy;
            if (ny>=0&&ny<mh && !ENE_BLOCKED(map[ny][ex])) { e->y=(float)ny; moved=true; }
        }
        if (!moved && yFirst && ddx) {
            int nx = ex+ddx;
            if (nx>=0&&nx<mw && !ENE_BLOCKED(map[ey][nx])) { e->x=(float)nx; }
        }
        #undef ENE_BLOCKED

        if ((int)(e->x+0.5f)==px && (int)(e->y+0.5f)==py) {
            if (pgold > 0) {
                GoldTouchHitEnemy(e);
            } else if (pshield || pshield_perk) {
                if (pshield) pshield = false;
                else pshield_perk = false;
                e->on   = false;
                AddEnemyKill();
                StatsInc(&global_kills_shield);
                Burst(WX(px), WY(py), CSHD, 24, 5);
                Burst(WX(px), WY(py), CENE, 16, 3);
                PlaySfx(SFX_SHIELD_TRIGGER);
            } else {
                Burst(WX(px), WY(py), CPD, 28, 5);
                PlaySfx(SFX_DEATH_ENEMY);
                StatsInc(&global_deaths_enemy);
                e->on = false;   // only the killer disappears
                pdead = true; gs = GS_DEAD; sttime = 0;
                UpdateAndSaveStats();
            }
        }
    }
}

// ─── DRAW ENEMIES ─────────────────────────────────────────────────────────────
void DrawEnemies(float t) {
    if (!eneon) return;
    for (int i=0; i<MAX_ENE; i++) {
        Ene *e = &enes[i]; if (!e->on) continue;
        int ex = (int)(e->x+0.5f), ey = (int)(e->y+0.5f);
        float ewx = ex*CELL, ewy = ey*CELL;
        float pp = 0.5f + 0.5f*sinf(t*4.0f + i);
        DrawCircle((int)ewx+CELL/2, (int)ewy+CELL/2,
                   (int)(CELL/2+4+2*pp), (Color){200,40,40,(int)(28+18*pp)});
        DrawCircle((int)ewx+CELL/2, (int)ewy+CELL/2, CELL/2-4, CENE);
        DrawCircle((int)ewx+CELL/2-2,(int)ewy+CELL/2-3, CELL/4,
                   (Color){240,100,100,110});
        DrawCircle((int)ewx+CELL/2-4,(int)ewy+CELL/2-2, 3, (Color){255,200,0,255});
        DrawCircle((int)ewx+CELL/2+4,(int)ewy+CELL/2-2, 3, (Color){255,200,0,255});
        DrawCircle((int)ewx+CELL/2-4,(int)ewy+CELL/2-2, 1, (Color){20,0,0,255});
        DrawCircle((int)ewx+CELL/2+4,(int)ewy+CELL/2-2, 1, (Color){20,0,0,255});
    }
}
