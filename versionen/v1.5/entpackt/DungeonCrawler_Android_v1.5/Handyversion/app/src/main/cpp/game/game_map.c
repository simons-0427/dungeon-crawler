// game_map.c — round generation and tile/map rendering.
#include "game_map.h"
#include "globals.h"
#include "constants.h"
#include "types.h"
#include "game_items.h"   // IK_COL
#include "game_utils.h"   // UpdateCameraOffset
#include <math.h>
#include <string.h>       // memset
#include <stdlib.h>       // abs

// ─── GENERATE ROUND ───────────────────────────────────────────────────────────
void GenRound(void) {
    global_rounds++;
    mh = 13 + (roundn - 1); if (mh > 42) mh = 42;
    mw = 23 + 2*(roundn - 1); if (mw > 76) mw = 76;
    int tp = 8 + roundn;  if (tp > 34) tp = 34;
    int xp = tp + 5;

    for (int y = 0; y < mh; y++)
    for (int x = 0; x < mw; x++) {
        if (x==0||y==0||x==mw-1||y==mh-1) { map[y][x]='#'; continue; }
        int r = GetRandomValue(0,99);
        map[y][x] = (r < tp) ? '#' : (r < xp) ? '+' : '.';
    }

   px = mw/2; py = mh/2;
    map[py][px] = '.';

    for (;;) {
        int tx = GetRandomValue(1,mw-2), ty = GetRandomValue(1,mh-2);
        if (map[ty][tx]!='#' && !(tx==px&&ty==py)) { map[ty][tx]='T'; break; }
    }

    for (int i = 0; i < MAX_PRT; i++) prts[i].on = false;
    for (int i = 0; i < MAX_PRT; i++) {
        if (GetRandomValue(0,1)==0) {
            for (int t = 0; t < 300; t++) {
                int ox = GetRandomValue(1,mw-2), oy = GetRandomValue(1,mh-2);
                if (map[oy][ox]=='.') {
                    prts[i] = (PrtObj){ox, oy, true};
                    map[oy][ox] = 'O'; break;
                }
            }
        }
    }

    for (int i = 0; i < MAX_COI; i++) coins[i].on = false;
    int nc = 8 + GetRandomValue(0,9); if (nc > MAX_COI) nc = MAX_COI;
    for (int placed=0, t=0; placed<nc && t<400; t++) {
        int cx = GetRandomValue(1,mw-2), cy = GetRandomValue(1,mh-2);
        if (map[cy][cx]=='.') {
            coins[placed++] = (CoinObj){cx, cy, true};
            map[cy][cx] = '$';
        }
    }

    for (int i = 0; i < MAX_ITM; i++) itms[i].on = false;
    int ni = 2 + GetRandomValue(0,1); if (ni > MAX_ITM) ni = MAX_ITM;
    for (int placed=0, t=0; placed<ni && t<400; t++) {
        int ix = GetRandomValue(1,mw-2), iy = GetRandomValue(1,mh-2);
        if (map[iy][ix]=='.') {
            itms[placed++] = (ItmObj){ix, iy, true, (IKind)GetRandomValue(0, IK_NONE-1)};
            map[iy][ix] = 'I';
        }
    }

    for (int i = 0; i < MAX_ENE; i++) enes[i].on = false;
    if (eneon) {
        int ne = 1 + roundn/2; if (ne > MAX_ENE) ne = MAX_ENE;
        for (int i=0, t=0; i<ne && t<400; t++) {
            int ex = GetRandomValue(1,mw-2), ey = GetRandomValue(1,mh-2);
            if (map[ey][ex]!='#' && abs(ex-px)+abs(ey-py) > 7) {
                enes[i++] = (Ene){(float)ex,(float)ey, 0,
                                  0.65f + GetRandomValue(0,35)*0.01f, true};
            }
        }
    }

    UpdateCameraOffset();
   cam.target   = (Vector2){px*CELL + CELL/2, py*CELL + CELL/2};
#if defined(PLATFORM_ANDROID)
    cam.zoom     = 1.18f;
#else
    cam.zoom     = 1.0f;
#endif
    cam.rotation = 0;

    pdead  = false; pmt = 0; pmd = 0.13f;
    pspd   = 0;
    pwlk   = 0;
    pshield = false; pshield_dur = 0;
    if (roundn == 1) pshield_perk = (perk_shield2 > 0);
    sttime = 0;
    memset(pars, 0, sizeof(pars));
}

// ─── DRAW TILE ────────────────────────────────────────────────────────────────
void DrawTile(int x, int y, float t) {
    int wx = x*CELL, wy = y*CELL;
    char c = map[y][x];
    char dc = c;

    DrawRectangle(wx, wy, CELL, CELL, CB);

    if (dc=='#') {
        DrawRectangle(wx+1, wy+1, CELL-2, CELL-2, CW);
        DrawRectangle(wx+1, wy+1, CELL-2, 3, CWH);
        DrawRectangle(wx+1, wy+1, 3, CELL-2, CWH);
        DrawRectangle(wx+1, wy+CELL-4, CELL-2, 3, CWD);
        DrawRectangle(wx+CELL-4, wy+1, 3, CELL-2, CWD);
    } else {
        DrawRectangle(wx+1, wy+1, CELL-2, CELL-2, CF);

        if (dc=='+') {
            DrawRectangle(wx + 3, wy + 3, CELL - 6, CELL - 6, (Color){20, 15, 25, 255});
            DrawRectangleLines(wx + 3, wy + 3, CELL - 6, CELL - 6, (Color){60, 50, 70, 255});
            for(int i = 0; i < 3; i++) {
                for(int j = 0; j < 3; j++) {
                    int spx = wx + 8 + i * 8;
                    int spy = wy + 8 + j * 8;
                    DrawCircle(spx, spy, 2, (Color){190, 190, 210, 255});
                    DrawRectangle(spx - 1, spy - 2, 2, 2, CTRAP);
                }
            }

        } else if (dc=='T') {
            float p = 0.5f + 0.5f*sinf(t * 3.0f);
            Color tc = CTRES; tc.a = (uchar)(200 + 55*p);
            DrawRectangle(wx+6,  wy+9,  CELL-12, CELL-14, tc);
            DrawRectangle(wx+6,  wy+9,  CELL-12, 4, (Color){255,240,180,230});
            DrawCircle(wx+CELL/2, wy+CELL/2+2, 3, (Color){200,150,30,255});
            DrawCircle(wx+CELL/2, wy+CELL/2,
                       (int)(12 + 5*p), (Color){255,200,50,(int)(18+20*p)});

        } else if (dc=='$') {
            float p = sinf(t * 4.0f + x*0.5f + y*0.7f);
            int r = (int)(5 + p*1.5f);
            DrawCircle(wx+CELL/2+1, wy+CELL/2+2, r, (Color){160,120,0,80});
            DrawCircle(wx+CELL/2,   wy+CELL/2,   r, CCOIN);
            DrawCircle(wx+CELL/2-1, wy+CELL/2-1,
                       (int)(r*0.4f)+1, (Color){255,245,160,200});

        } else if (dc=='I') {
            IKind k = IK_NONE;
            for (int i=0;i<MAX_ITM;i++)
                if (itms[i].on&&itms[i].x==x&&itms[i].y==y) { k=itms[i].k; break; }
            if (k != IK_NONE) {
                Color ic = IK_COL[k];
                float p = 0.5f + 0.5f*sinf(t*3.0f + (float)(x+y)*0.3f);
                DrawTriangle(
                    (Vector2){wx+CELL/2, wy+4},
                    (Vector2){wx+CELL-4, wy+CELL/2},
                    (Vector2){wx+CELL/2, wy+CELL-4}, ic);
                DrawTriangle(
                    (Vector2){wx+CELL/2, wy+4},
                    (Vector2){wx+CELL/2, wy+CELL-4},
                    (Vector2){wx+4,      wy+CELL/2}, ic);
                Color gc = ic; gc.a = (uchar)(55 * p);
                DrawCircle(wx+CELL/2, wy+CELL/2, (int)(14+3*p), gc);
            }

        } else if (dc=='O') {
            float p = 0.5f + 0.5f*sinf(t*2.0f + (float)(x+y)*0.2f);
            DrawCircle(wx+CELL/2, wy+CELL/2,
                       (int)(12+5*p), (Color){80,160,255,(int)(65+50*p)});
            DrawCircle(wx+CELL/2, wy+CELL/2,
                       (int)(7+2*p),  (Color){160,210,255,(int)(140+60*p)});
            DrawCircleLines(wx+CELL/2, wy+CELL/2,
                            (int)(12+5*p), (Color){120,190,255,200});
        }
    }
}

// ─── DRAW MAP ─────────────────────────────────────────────────────────────────
void DrawMap(float t) {
    for (int y = 0; y < mh; y++)
    for (int x = 0; x < mw; x++)
        DrawTile(x, y, t);
}
