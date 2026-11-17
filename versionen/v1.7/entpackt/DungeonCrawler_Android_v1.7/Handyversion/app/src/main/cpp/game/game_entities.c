// game_entities.c — enemy chase AI and enemy rendering.
#include "game_entities.h"
#include "globals.h"
#include "constants.h"
#include "types.h"
#include "game_particles.h"  // Burst
#include "game_save.h"       // AddEnemyKill, UpdateAndSaveStats
#include "game_sfx.h"        // PlaySfx
#include "game_map.h"        // WX, WY
#include "game_challenges.h" // ChallengeRunActive
#include "game_player.h"     // GoldTouchHitEnemy
#include "game_utils.h"      // AddScreenShake
#include <math.h>
#include <stdlib.h>

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
        #define ENE_BLOCKED(c) ((c)=='#' || (c)=='G' || (c)=='k' || (c)=='L' || (c)==' ')
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
                AddScreenShake(12.0f, 0.28f);
                StatsInc(&global_deaths_enemy);
                e->on = false;   // only the killer disappears
                pdead = true; gs = GS_DEAD; sttime = 0;
                UpdateAndSaveStats();
            }
        }
    }
}

// ─── DRAW ENEMIES ─────────────────────────────────────────────────────────────
static void EnemyTriangle(int x,int y,float x1,float y1,float x2,float y2,
                          float x3,float y3,Color color) {
    Vector2 a={(float)x+x1,(float)y+y1};
    Vector2 b={(float)x+x2,(float)y+y2};
    Vector2 c={(float)x+x3,(float)y+y3};
    float cross=(x2-x1)*(y3-y1)-(y2-y1)*(x3-x1);
    if(cross>0.0f)DrawTriangle(a,c,b,color);
    else DrawTriangle(a,b,c,color);
}

static void EnemyLine(int x,int y,int x1,int y1,int x2,int y2,Color color) {
    DrawLineEx((Vector2){(float)(x+x1),(float)(y+y1)},
               (Vector2){(float)(x+x2),(float)(y+y2)},1.0f,color);
}

static void DrawAzureWispEnemy(int x,int y,float t) {
    float pulse=photosensitivity?0.5f:0.5f+0.5f*sinf(t*3.1f);
    Vector2 auraCenter={(float)(x+16),(float)(y+16)};
    DrawCircle(x+16,y+16,22,(Color){102,220,255,(uchar)(25+15*pulse)});
    DrawRing(auraCenter,16,19,0,360,36,
             (Color){155,235,255,(uchar)(95+35*pulse)});
    DrawRing(auraCenter,20,21,0,360,36,
             (Color){113,218,255,(uchar)(38+20*pulse)});
    EnemyTriangle(x,y,11,9,8,2,15,8,(Color){36,107,149,255});
    EnemyTriangle(x,y,20,9,25,3,23,12,(Color){54,150,173,255});
    EnemyTriangle(x,y,9,19,2,16,8,23,(Color){39,119,148,255});
    DrawCircle(x+16,y+17,11,(Color){30,67,103,255});
    DrawCircle(x+12,y+13,6,(Color){108,205,216,88});
    DrawCircle(x+12,y+15,3,(Color){255,230,139,255});
    DrawCircle(x+20,y+15,3,(Color){255,230,139,255});
    DrawCircle(x+12,y+15,1,(Color){18,47,72,255});
    DrawCircle(x+20,y+15,1,(Color){18,47,72,255});
    EnemyTriangle(x,y,13,24,17,28,18,22,(Color){12,43,76,255});
    EnemyLine(x,y,0,12,5,13,(Color){101,223,232,255});
    EnemyLine(x,y,0,19,5,18,(Color){101,223,232,255});
    EnemyLine(x,y,26,9,30,7,(Color){101,223,232,255});
    EnemyLine(x,y,26,22,28,22,(Color){141,242,232,255});
    EnemyLine(x,y,27,21,27,23,(Color){141,242,232,255});
    DrawCircle(x+27,y+22,1,(Color){141,242,232,255});
}

void DrawAzureWispPreview(int cx,int cy,float t) {
    DrawAzureWispEnemy(cx-16,cy-16,t);
}

static void DrawArtifactEnemySpeedTrail(int cx,int cy,int ex,int ey,
                                        int index,float t) {
    int dx=px-ex,dy=py-ey,dirX=0,dirY=0;
    if(abs(dx)>=abs(dy))dirX=(dx>0)-(dx<0);
    else dirY=(dy>0)-(dy<0);
    if(!dirX&&!dirY)return;
    float pulse=0.5f+0.5f*sinf(t*9.0f+index*1.7f);
    Color streak={110,220,255,(uchar)(135+105*pulse)};
    for(int lane=-1;lane<=1;lane++) {
        float side=lane*5.0f;
        Vector2 tail={(float)(cx-dirX*20-dirY*side),
                      (float)(cy-dirY*20+dirX*side)};
        Vector2 tip={(float)(cx-dirX*9-dirY*side),
                     (float)(cy-dirY*9+dirX*side)};
        DrawLineEx(tail,tip,2.0f,streak);
    }
}

static void DrawBoneCrownEnemy(int x,int y,float t) {
    float pulse=photosensitivity?0.5f:0.5f+0.5f*sinf(t*3.1f);
    DrawCircle(x+16,y+17,16,(Color){100,213,255,(uchar)(28+24*pulse)});
    DrawRing((Vector2){(float)(x+16),(float)(y+17)},13,15,0,360,28,
             (Color){157,231,255,(uchar)(72+40*pulse)});
    DrawCircle(x+16,y+17,13,(Color){161,76,255,(uchar)(48+30*pulse)});
    EnemyTriangle(x,y,4,16,5,2,13,9,(Color){221,203,163,255});
    EnemyTriangle(x,y,28,16,27,2,19,9,(Color){221,203,163,255});
    EnemyTriangle(x,y,8,7,16,3,24,7,(Color){241,226,192,255});
    DrawCircle(x+16,y+17,11,(Color){48,37,65,255});
    EnemyTriangle(x,y,8,9,16,6,16,25,(Color){225,211,183,255});
    EnemyTriangle(x,y,24,9,16,6,16,25,(Color){190,175,157,255});
    DrawRectangle(x+8,y+15,6,3,(Color){13,8,21,255});
    DrawRectangle(x+18,y+15,6,3,(Color){13,8,21,255});
    DrawCircle(x+11,y+16,2,(Color){255,60,179,255});
    DrawCircle(x+21,y+16,2,(Color){255,60,179,255});
    EnemyTriangle(x,y,14,21,18,21,16,28,(Color){16,9,24,255});
}

void DrawEnemyIcon(int cx, int cy, float t) {
    float pp = 0.5f + 0.5f * sinf(t * 4.0f);
    DrawCircle(cx, cy, (int)(CELL / 2 + 4 + 2 * pp),
               (Color){ 200, 40, 40, (uchar)(28 + 18 * pp) });
    DrawCircle(cx, cy, CELL / 2 - 4, CENE);
    DrawCircle(cx - 2, cy - 3, CELL / 4, (Color){ 240, 100, 100, 110 });
    DrawCircle(cx - 4, cy - 2, 3, (Color){ 255, 200, 0, 255 });
    DrawCircle(cx + 4, cy - 2, 3, (Color){ 255, 200, 0, 255 });
    DrawCircle(cx - 4, cy - 2, 1, (Color){ 20, 0, 0, 255 });
    DrawCircle(cx + 4, cy - 2, 1, (Color){ 20, 0, 0, 255 });
}

void DrawEnemies(float t) {
    if (!eneon) return;
    for (int i=0; i<MAX_ENE; i++) {
        Ene *e = &enes[i]; if (!e->on) continue;
        int ex = (int)(e->x+0.5f), ey = (int)(e->y+0.5f);
        float ewx = ex*CELL, ewy = ey*CELL;
        if(ChallengeRunActive()) {
            DrawBoneCrownEnemy((int)ewx,(int)ewy,t+i*0.23f);
            continue;
        }
        if (roomKind == RK_ARTIFACT) {
            DrawArtifactEnemySpeedTrail((int)ewx+CELL/2,(int)ewy+CELL/2,
                                        ex,ey,i,t);
            DrawAzureWispEnemy((int)ewx,(int)ewy,t+i*0.23f);
            continue;
        }
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
