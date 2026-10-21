#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>

// ─── CONSTANTS ────────────────────────────────────────────────────────────────
#define SW        1280
#define SH        720
#define CELL      32
#define MAP_ROWS  55
#define MAP_COLS  80
#define MAX_ENE   8
#define MAX_COI   32
#define MAX_ITM   4
#define MAX_PRT   2
#define MAX_PAR   256
#define INV_SZ    1

#define CLAMP255(v) ((unsigned char)((v)<0?0:((v)>255?255:(v))))

// ─── TYPES ────────────────────────────────────────────────────────────────────
typedef unsigned char uchar;
typedef enum { IK_SPD=0, IK_SHD, IK_BMB, IK_WLK, IK_SWD, IK_NONE } IKind;
typedef enum { GS_MENU, GS_PLAY, GS_PAUSE, GS_DEAD, GS_NEXT } GState;

static const char* IK_NAME[] = { "SPD", "SHD", "BMB", "WLK", "SWD" };
static const Color IK_COL[] = {
    {  80,220, 80,255 },   
    {  80,150,255,255 },  
    { 255,140, 40,255 },  
    { 180, 80,255,255 },   
    { 255, 60,120,255 }   
};

typedef struct { float x,y,vx,vy,life,ml; Color c; float sz; } Par;
typedef struct { float x,y,mt,cd; bool on; } Ene;
typedef struct { int x,y; bool on; } CoinObj;
typedef struct { int x,y; bool on; IKind k; } ItmObj;
typedef struct { int x,y; bool on; } PrtObj;

// ─── GLOBALS ──────────────────────────────────────────────────────────────────
static GState    gs        = GS_MENU;
static char      map[MAP_ROWS][MAP_COLS];
static int       mw, mh;

// Player
static int    px, py;
static float  pmt, pmd;
static bool   pdead, pshield, pshield_perk;
static float  pspd;   
static float  pwlk;   
static IKind  pinv[INV_SZ + 2];   // +2 for max perk_slots
static int    pinvn;

// World objects
static Ene    enes[MAX_ENE];
static CoinObj coins[MAX_COI];
static ItmObj  itms[MAX_ITM];
static PrtObj  prts[MAX_PRT];
static Par     pars[MAX_PAR];

// Stats / flags
static int   roundn, totcoi, tottrs;
static bool  eneon, wasportal;
static float sttime;   
static Camera2D cam;

// ─── HIGHSCORES & PERSISTENCE ─────────────────────────────────────────────────
#define STATS_FILE "dungeons_stats.txt"
static int hs_round = 0;
static int hs_trs = 0;
static int hs_coi = 0;
static int hs_kills = 0;
static int global_totcoi = 0;
static int global_totkills = 0;
static int global_tottrs = 0;
static int global_rounds = 0;
static int global_games_played = 0;
static int coin_balance = 0;
static int kills_game = 0;

// ─── SHOP STATE ───────────────────────────────────────────────────────────────
#define SHOP_FILE "dungeons_shop.txt"
static int  skin_owned[10];
static int  skin_equipped = 0;
static int  spd_level = 0;
static int  perk_slots = 0;
static int  perk_magnet = 0;
static int  perk_shield2 = 0;

static void LoadShop(void) {
    FILE *f = fopen(SHOP_FILE, "r");
    if (f) {
        fscanf(f, "%d %d %d %d %d %d",
               &skin_equipped, &spd_level, &perk_slots, &perk_magnet, &perk_shield2,
               skin_owned+0);
        for (int i=1;i<10;i++) fscanf(f, "%d", &skin_owned[i]);
        fclose(f);
    }
    skin_owned[0] = 1;
}

static void SaveShop(void) {
    FILE *f = fopen(SHOP_FILE, "w");
    if (f) {
        fprintf(f, "%d %d %d %d %d",
                skin_equipped, spd_level, perk_slots, perk_magnet, perk_shield2);
        for (int i=0;i<10;i++) fprintf(f, " %d", skin_owned[i]);
        fprintf(f, "\n");
        fclose(f);
    }
}

static void LoadStats(void) {
    FILE *f = fopen(STATS_FILE, "r");
    if (f) {
        fscanf(f, "%d %d %d %d %d %d %d %d %d %d",
               &hs_round, &hs_trs, &hs_coi, &global_totcoi,
               &hs_kills, &global_totkills, &global_tottrs, &global_rounds,
               &coin_balance, &global_games_played);
        fclose(f);
    }
}

static void SaveStats(void) {
    FILE *f = fopen(STATS_FILE, "w");
    if (f) {
        fprintf(f, "%d %d %d %d %d %d %d %d %d %d\n",
                hs_round, hs_trs, hs_coi, global_totcoi,
                hs_kills, global_totkills, global_tottrs, global_rounds,
                coin_balance, global_games_played);
        fclose(f);
    }
}

static void UpdateAndSaveStats(void) {
    if (roundn > hs_round) hs_round = roundn;
    if (tottrs > hs_trs) hs_trs = tottrs;
    if (totcoi > hs_coi) hs_coi = totcoi;
    if (kills_game > hs_kills) hs_kills = kills_game;
    SaveStats();
}

static void AddEnemyKill(void) {
    kills_game++;
    global_totkills++;
    if (kills_game > hs_kills) hs_kills = kills_game;
}

// ─── COLOUR PALETTE ───────────────────────────────────────────────────────────
#define CB     ((Color){12,10,18,255})
#define CF     ((Color){28,24,40,255})
#define CW     ((Color){50,45,66,255})
#define CWH    ((Color){72,65,92,255})
#define CWD    ((Color){18,15,28,255})
#define CTRAP  ((Color){195,40,40,255})
#define CTRES  ((Color){255,200,50,255})
#define CPL    ((Color){90,220,120,255})
#define CPD    ((Color){230,55,55,255})
#define CCOIN  ((Color){255,215,0,255})
#define CPORT  ((Color){80,160,255,255})
#define CENE   ((Color){220,65,65,255})
#define CSHD   ((Color){80,150,255,255})
#define CWLK   ((Color){180,80,255,255})  
#define CSWD   ((Color){255,60,120,255}) 

// ─── PARTICLE SYSTEM ──────────────────────────────────────────────────────────
static void SpawnPar(float x, float y, Color c,
                     float vx, float vy, float life, float sz) {
    for (int i = 0; i < MAX_PAR; i++) {
        if (pars[i].life <= 0) {
            pars[i] = (Par){x,y,vx,vy,life,life,c,sz};
            return;
        }
    }
}
static void Burst(float x, float y, Color c, int n, float spd) {
    for (int i = 0; i < n; i++) {
        float a = (float)i / n * (2*PI) + GetRandomValue(0,100)*0.063f;
        float s = spd * (0.5f + GetRandomValue(0,100)*0.005f);
        float lf = 0.4f + GetRandomValue(0,60)*0.01f;
        float sz = (float)(2 + GetRandomValue(0,3));
        SpawnPar(x, y, c, cosf(a)*s, sinf(a)*s, lf, sz);
    }
}
static void UpdPars(float dt) {
    for (int i = 0; i < MAX_PAR; i++) {
        Par *p = &pars[i]; if (p->life <= 0) continue;
        p->x += p->vx * dt * 60;
        p->y += p->vy * dt * 60;
        p->vy += 0.05f * dt * 60;
        p->life -= dt;
    }
}
static void DrawPars(void) {
    for (int i = 0; i < MAX_PAR; i++) {
        Par *p = &pars[i]; if (p->life <= 0) continue;
        float a = p->life / p->ml;
        Color c = p->c; c.a = (uchar)(a * 255);
        DrawCircleV((Vector2){p->x, p->y}, p->sz * a + 1.0f, c);
    }
}

// ─── WORLD HELPERS ────────────────────────────────────────────────────────────
static inline float WX(int tx) { return tx * CELL + CELL * 0.5f; }
static inline float WY(int ty) { return ty * CELL + CELL * 0.5f; }

static void UpdateCameraOffset(void) {
    cam.offset = (Vector2){GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
}

static void ToggleFullscreenNow(void) {
    if (!IsWindowFullscreen()) {
        int monitor = GetCurrentMonitor();
        SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
        ToggleFullscreen();
    } else {
        ToggleFullscreen();
        SetWindowSize(SW, SH);
    }
    UpdateCameraOffset();
}

static bool PointInRect(Vector2 p, Rectangle r) {
    return CheckCollisionPointRec(p, r);
}

static void GetPauseMenuLayout(Rectangle *panel, Rectangle *resumeBtn, Rectangle *menuBtn) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int pw = 460, ph = 220;
    int px2 = sw / 2 - pw / 2;
    int py2 = sh / 2 - ph / 2;

    if (panel)     *panel     = (Rectangle){ (float)px2, (float)py2, (float)pw, (float)ph };
    if (resumeBtn) *resumeBtn = (Rectangle){ (float)(px2 + pw/2 - 120), (float)(py2 + 110), 240.0f, 36.0f };
    if (menuBtn)   *menuBtn   = (Rectangle){ (float)(px2 + pw/2 - 120), (float)(py2 + 156), 240.0f, 36.0f };
}

static Rectangle GetPauseButtonRect(void) {
    int sw = GetScreenWidth();
    return (Rectangle){ (float)(sw - 14 - 44), 15.0f, 44.0f, 44.0f };
}

// ─── GENERATE ROUND ───────────────────────────────────────────────────────────
static void GenRound(void) {
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

    px = mw/2; py = mh/2; map[py][px] = '.';

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
    cam.target   = (Vector2){WX(px), WY(py)};
    cam.zoom     = 1.0f;
    cam.rotation = 0;

    pdead  = false; pmt = 0; pmd = 0.13f;
    pspd   = 0;
    pwlk   = 0;
    pshield = false;                                        // item shield resets every round
    if (roundn == 1) pshield_perk = (perk_shield2 > 0);   // perk shield: only granted at game start
    sttime = 0;
    memset(pars, 0, sizeof(pars));
}

// ─── USE ITEM ─────────────────────────────────────────────────────────────────
static void UseItm(int slot) {
    if (slot >= pinvn || slot >= INV_SZ + 2) return;
    IKind k = pinv[slot];
    switch (k) {
        case IK_SPD:
            pspd = 4.0f + spd_level * 2.0f;
            Burst(WX(px), WY(py), IK_COL[IK_SPD], 16, 3);
            break;
        case IK_SHD:
            pshield = true;
            Burst(WX(px), WY(py), CSHD, 18, 3);
            break;
        case IK_BMB:
            for (int dy=-2; dy<=2; dy++)
            for (int dx=-2; dx<=2; dx++) {
                int nx=px+dx, ny=py+dy;
                if (nx>=0&&ny>=0&&nx<mw&&ny<mh && map[ny][nx]=='+') {
                    map[ny][nx]='.';
                    Burst(WX(nx), WY(ny), IK_COL[IK_BMB], 8, 3);
                }
            }
            break;

        case IK_WLK:
            pwlk = 2.0f;
            Burst(WX(px), WY(py), CWLK, 20, 3);
            break;

        case IK_SWD:
            if (eneon) {
                for (int i = 0; i < MAX_ENE; i++) {
                    Ene *e = &enes[i]; if (!e->on) continue;
                    int ex = (int)(e->x+0.5f), ey = (int)(e->y+0.5f);
                    if (abs(ex-px) <= 3 && abs(ey-py) <= 3) {
                        e->on = false;
                        AddEnemyKill();
                        Burst(WX(ex), WY(ey), CENE, 20, 4);
                        Burst(WX(ex), WY(ey), CSWD, 12, 5);
                    }
                }
            }
            Burst(WX(px), WY(py), CSWD, 28, 5);
            break;

        default: break;
    }
    for (int i = slot; i < pinvn-1; i++) pinv[i] = pinv[i+1];
    pinvn--;
}

// ─── PLAYER MOVEMENT ──────────────────────────────────────────────────────────
static void TryMove(int dx, int dy) {
    int nx = px+dx, ny = py+dy;
    if (nx<0||ny<0||nx>=mw||ny>=mh) return;
    char tg = map[ny][nx];

    if (tg=='#') {
        if (pwlk <= 0) return;
        px = nx; py = ny;
        Burst(WX(px), WY(py), CWLK, 4, 2);
        return;
    }

    if (eneon) {
        for (int i = 0; i < MAX_ENE; i++) {
            Ene *e = &enes[i]; if (!e->on) continue;
            if ((int)(e->x+0.5f)==nx && (int)(e->y+0.5f)==ny) {
                if (pshield || pshield_perk) {
                    if (pshield) pshield = false;
                    else pshield_perk = false;
                    e->on   = false;
                    AddEnemyKill();
                    px = nx; py = ny;
                    Burst(WX(px), WY(py), CSHD, 24, 5);
                    Burst(WX(px), WY(py), CENE, 16, 3);
                    return;
                } else {
                    px = nx; py = ny;
                    Burst(WX(px), WY(py), CPD,  32, 5);
                    Burst(WX(px), WY(py), CENE, 16, 3);
                    pdead = true; gs = GS_DEAD; sttime = 0;
                    UpdateAndSaveStats();
                    return;
                }
            }
        }
    }

    px = nx; py = ny;

    // Trap
    if (tg=='+') {
        if (pshield || pshield_perk) {
            if (pshield) pshield = false;
            else pshield_perk = false;
            map[py][px] = '.';
            Burst(WX(px), WY(py), CSHD, 22, 4);
        } else {
            Burst(WX(px), WY(py), CPD, 32, 5);
            pdead = true; gs = GS_DEAD; sttime = 0;
            UpdateAndSaveStats();
        }
        return;
    }
    // Treasure
    if (tg=='T') {
        Burst(WX(px), WY(py), CTRES, 32, 5);
        tottrs++; global_tottrs++; wasportal = false; gs = GS_NEXT; sttime = 0;
        UpdateAndSaveStats();
        return;
    }
    // Coin
    if (tg=='$') {
        for (int i=0; i<MAX_COI; i++)
            if (coins[i].on && coins[i].x==px && coins[i].y==py)
                { coins[i].on=false; break; }
        map[py][px] = '.'; totcoi++;
        global_totcoi++;
        coin_balance++;
        Burst(WX(px), WY(py), CCOIN, 10, 2);
        return;
    }
    // Item pickup
    if (tg=='I') {
        for (int i=0; i<MAX_ITM; i++) {
            if (itms[i].on && itms[i].x==px && itms[i].y==py) {
                if (pinvn < INV_SZ + perk_slots) {
                    IKind k = itms[i].k;
                    itms[i].on = false; map[py][px] = '.';
                    pinv[pinvn++] = k;
                    Burst(WX(px), WY(py), IK_COL[k], 16, 3);
                }
                break;
            }
        }
        return;
    }
    // Portal
    if (tg=='O') {
        Burst(WX(px), WY(py), CPORT, 32, 5);
        wasportal = true; gs = GS_NEXT; sttime = 0;
        UpdateAndSaveStats();
        return;
    }
}

// ─── ENEMY AI ─────────────────────────────────────────────────────────────────
static void UpdEnes(float dt) {
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

        if (!yFirst && ddx) {
            int nx = ex+ddx;
            if (nx>=0&&nx<mw && map[ey][nx]!='#') { e->x=(float)nx; moved=true; }
        }
        if (!moved && ddy) {
            int ny = ey+ddy;
            if (ny>=0&&ny<mh && map[ny][ex]!='#') { e->y=(float)ny; moved=true; }
        }
        if (!moved && yFirst && ddx) {
            int nx = ex+ddx;
            if (nx>=0&&nx<mw && map[ey][nx]!='#') { e->x=(float)nx; }
        }

        if ((int)(e->x+0.5f)==px && (int)(e->y+0.5f)==py) {
            if (pshield || pshield_perk) {
                if (pshield) pshield = false;
                else pshield_perk = false;
                e->on   = false;
                AddEnemyKill();
                Burst(WX(px), WY(py), CSHD, 24, 5);
                Burst(WX(px), WY(py), CENE, 16, 3);
            } else {
                Burst(WX(px), WY(py), CPD, 28, 5);
                pdead = true; gs = GS_DEAD; sttime = 0;
                UpdateAndSaveStats();
            }
        }
    }
}

// ─── DRAW TILE ────────────────────────────────────────────────────────────────
static void DrawTile(int x, int y, float t) {
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

// ─── DRAW HUD ─────────────────────────────────────────────────────────────────
static void DrawPauseButton(void);
static void DrawHUD(float t) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, 76, (Color){8,6,14,235});
    DrawRectangle(0, 75, sw, 2, (Color){55,50,72,200});

    DrawText(TextFormat("Round  %d", roundn), 16, 8, 26, (Color){210,205,230,255});

    DrawCircle(213, 21, 9, CCOIN);
    DrawCircle(210, 18, 4, (Color){255,245,160,200});
    DrawText(TextFormat("%d", totcoi), 228, 8, 26, CCOIN);

    DrawRectangle(295, 10, 20, 15, CTRES);
    DrawRectangle(295, 10, 20, 4, (Color){255,240,180,200});
    DrawText(TextFormat("%d", tottrs), 323, 10, 20, (Color){200,180,80,255});

    DrawCircle(392, 21, 9, CENE);
    DrawText(TextFormat("%d", kills_game), 407, 8, 26, CENE);

    if (eneon) {
        DrawText("ENEMIES ON", sw/2 - MeasureText("ENEMIES ON",16)/2,
                 10, 16, (Color){220,80,80,160});
    } else {
        DrawText("ENEMIES OFF", sw/2 - MeasureText("ENEMIES OFF",16)/2,
                 10, 16, (Color){80,80,100,130});
    }

    DrawLine(14, 38, 520, 38, (Color){45,40,62,180});
    DrawText("HIGHSCORES:", 16, 42, 14, (Color){100,95,130,180});

    bool newRound = (roundn > hs_round);
    Color rcol = newRound ? (Color){255,210,60,255} : (Color){160,155,185,200};
    DrawText(TextFormat("Round %d", newRound ? roundn : hs_round),
             16, 56, 14, rcol);

    bool newCoi = (totcoi > hs_coi);
    Color ccol = newCoi ? (Color){255,215,0,255} : (Color){160,155,185,200};
    DrawCircle(108, 61, 5, newCoi ? CCOIN : (Color){130,110,0,180});
    DrawText(TextFormat("%d", newCoi ? totcoi : hs_coi), 118, 56, 14, ccol);

    bool newTrs = (tottrs > hs_trs);
    Color tcol = newTrs ? (Color){255,200,50,255} : (Color){160,155,185,200};
    DrawRectangle(176, 57, 10, 8, newTrs ? CTRES : (Color){130,100,0,160});
    DrawText(TextFormat("%d", newTrs ? tottrs : hs_trs), 190, 56, 14, tcol);

    bool newKills = (kills_game > hs_kills);
    Color kcol = newKills ? CENE : (Color){160,155,185,200};
    DrawCircle(250, 61, 5, newKills ? CENE : (Color){130,55,55,180});
    DrawText(TextFormat("%d", newKills ? kills_game : hs_kills), 260, 56, 14, kcol);

    DrawCircle(328, 62, 5, CCOIN);
    DrawCircle(326, 60, 2, (Color){255,245,160,200});
    DrawText(TextFormat("Coin Balance: %d", coin_balance), 338, 56, 14, (Color){255,215,0,200});

    // ── Item inventory slots ──────────────────────────────────────────────────
    int eff_slots = INV_SZ + perk_slots;
    int sx = sw - 14 - 64 - eff_slots * 68;
    DrawText("ITEMS", sx-70, 20, 15, (Color){130,125,160,190});
    for (int i = 0; i < eff_slots; i++) {
        int bx = sx + i*68, by = 9;
        bool has = (i < pinvn);
        DrawRectangleRounded((Rectangle){bx,by,62,56}, 0.3f, 8,
                             has ? (Color){35,30,52,210} : (Color){20,18,32,120});
        Color bc = has ? IK_COL[pinv[i]] : (Color){50,46,68,140};
        DrawRectangleRoundedLines((Rectangle){bx,by,62,56}, 0.3f, 8, bc);
        if (has) {
            DrawText(IK_NAME[pinv[i]], bx+6, by+5, 14, IK_COL[pinv[i]]);
            DrawText(TextFormat("[%d]",i+1), bx+6, by+36, 12, (Color){130,125,155,200});
        } else {
            DrawText(TextFormat("[%d]",i+1), bx+20, by+22, 13, (Color){65,60,85,140});
        }
    }

    // ── Active effects (bottom-left) ─────────────────────────────────────────
    int efx = 14, efy = sh - 32;
    if (pspd > 0) {
        DrawText(TextFormat("SPEED  %.1fs", pspd), efx, efy, 18,
                 (Color){80,220,80,255}); efx += 155;
    }
    if (pshield) {
        DrawText("SHIELD", efx, efy, 18, (Color){80,150,255,255}); efx += 95;
    }
    if (pshield_perk) {
        DrawText("PERK SHIELD", efx, efy, 18, (Color){100,180,255,200}); efx += 155;
    }
    if (pwlk > 0) {
        DrawText(TextFormat("GHOSTWALK  %.1fs", pwlk), efx, efy, 18, CWLK);
        efx += 205;
    }

    DrawPauseButton();

    const char *help = "WASD/Arrows:Move   1/2/3:Use Item   F11:Fullscreen";
    DrawText(help, sw - MeasureText(help,14) - 14,
             sh-22, 14, (Color){80,75,100,160});
}
static void DrawPauseMenu(void) {
    Rectangle panel, resumeBtn, menuBtn;
    GetPauseMenuLayout(&panel, &resumeBtn, &menuBtn);

    DrawRectangleRounded(panel, 0.08f, 8, (Color){10, 8, 18, 240});
    DrawRectangleRoundedLines(panel, 0.08f, 8, CW);

    const char *TT = "PAUSED";
    int ttw = MeasureText(TT, 36);
    DrawText(TT, (int)(panel.x + panel.width/2 - ttw/2), (int)(panel.y + 20),
             36, (Color){215, 210, 235, 255});
    
    const char *ST = "ESC or the pause icon resumes the game";
    int stw = MeasureText(ST, 16);
    DrawText(ST, (int)(panel.x + panel.width/2 - stw/2), (int)(panel.y + 68),
             16, (Color){160, 155, 185, 255});

    DrawLine((int)panel.x + 16, (int)panel.y + 50, (int)panel.x + (int)panel.width - 16,
             (int)panel.y + 50, (Color){65, 58, 88, 180});

    bool hoverResume = PointInRect(GetMousePosition(), resumeBtn);
    bool hoverMenu   = PointInRect(GetMousePosition(), menuBtn);

    Color rb = hoverResume ? (Color){35, 65, 42, 255} : (Color){20, 40, 25, 240};
    Color mb = hoverMenu   ? (Color){75, 35, 32, 255} : (Color){45, 20, 18, 240};
    Color rlc = hoverResume ? (Color){80, 220, 80, 255} : (Color){40, 150, 50, 255};
    Color mlc = hoverMenu   ? (Color){220, 65, 65, 255} : (Color){150, 40, 40, 255};

    DrawRectangleRounded(resumeBtn, 0.25f, 8, rb);
    DrawRectangleRoundedLines(resumeBtn, 0.25f, 8, rlc);
    DrawRectangleRounded(menuBtn,   0.25f, 8, mb);
    DrawRectangleRoundedLines(menuBtn,   0.25f, 8, mlc);

    const char *resumeTxt = "RESUME";
    const char *menuTxt = "MENU";
    DrawText(resumeTxt,
             (int)(resumeBtn.x + resumeBtn.width/2 - MeasureText(resumeTxt, 20)/2),
             (int)(resumeBtn.y + 8), 20, (Color){230, 235, 240, 255});
    DrawText(menuTxt,
             (int)(menuBtn.x + menuBtn.width/2 - MeasureText(menuTxt, 20)/2),
             (int)(menuBtn.y + 8), 20, (Color){230, 235, 240, 255});
}

static void DrawPauseButton(void) {
    Rectangle r = GetPauseButtonRect();
    bool hover = PointInRect(GetMousePosition(), r);

   DrawRectangleRounded(r, 0.25f, 8,
                         hover ? (Color){40, 36, 58, 240} : (Color){24, 20, 34, 220});
    DrawRectangleRoundedLines(r, 0.25f, 8,
                              hover ? (Color){180, 170, 220, 255} : (Color){110, 100, 140, 210});

    int barW = 6;
    int barH = 20;
    int barY = (int)(r.y + 12);
    int barX1 = (int)(r.x + 12);
    int barX2 = (int)(r.x + 26);
    DrawRectangle(barX1, barY, barW, barH, (Color){220, 215, 235, 255});
    DrawRectangle(barX2, barY, barW, barH, (Color){220, 215, 235, 255});
}

// ─── MENU SUB-SCREEN STATE ────────────────────────────────────────────────────
static int menuSub = 0;
static int statScroll = 0;
static int shopTab = 0;

// ─── DRAW MENU ────────────────────────────────────────────────────────────────
static void DrawMenuBG(float t, int sw, int sh) {
    for (int y = 0; y <= sh/CELL; y++)
    for (int x = 0; x <= sw/CELL; x++) {
        float f = 0.22f + 0.14f*sinf(x*0.41f + y*0.53f + t*0.35f)
                        + 0.08f*sinf(x*0.17f - y*0.29f + t*0.22f);
        DrawRectangle(x*CELL, y*CELL, CELL-1, CELL-1,
                      (Color){(uchar)(12*f),(uchar)(9*f),(uchar)(20*f),255});
    }
    for (int i = 7; i >= 1; i--)
        DrawCircle(sw/2, sh/2, (float)(i*160),
                   (Color){6,4,14,(uchar)(4+i*6)});
}

static void DrawCornerAccents(Rectangle r, Color c, int len) {
    int x1=(int)r.x, y1=(int)r.y;
    int x2=(int)(r.x+r.width-1), y2=(int)(r.y+r.height-1);
    DrawLine(x1,y1,   x1+len,y1,   c); DrawLine(x1,y1, x1,y1+len,   c);
    DrawLine(x2,y1,   x2-len,y1,   c); DrawLine(x2,y1, x2,y1+len,   c);
    DrawLine(x1,y2,   x1+len,y2,   c); DrawLine(x1,y2, x1,y2-len,   c);
    DrawLine(x2,y2,   x2-len,y2,   c); DrawLine(x2,y2, x2,y2-len,   c);
}

static bool DrawNavCard(Rectangle r, const char *label, Color accent, float t, float phase) {
    Vector2 mp  = GetMousePosition();
    bool hover  = CheckCollisionPointRec(mp, r);
    float gp    = 0.5f + 0.5f*sinf(t*1.6f + phase);

    int fs  = 32;
    int lw  = MeasureText(label, fs);
    int lx  = (int)(r.x + r.width/2 - lw/2);
    int ly  = (int)(r.y + r.height/2 - fs/2);

    {
        float bf = 0.5f + 0.4f*gp;
        Color fill = hover
            ? (Color){(uchar)(20 + accent.r/10),
                      (uchar)(16 + accent.g/10),
                      (uchar)(32 + accent.b/10), 210}
            : (Color){14, 11, 22, 180};
        DrawRectangleRounded(r, 0.10f, 8, fill);
        Color border = hover
            ? (Color){accent.r, accent.g, accent.b, 200}
            : (Color){(uchar)(accent.r*bf), (uchar)(accent.g*bf), (uchar)(accent.b*bf), 130};
        DrawRectangleRoundedLines(r, 0.10f, 8, border);
        Rectangle inner = {r.x+3, r.y+3, r.width-6, r.height-6};
        uchar inal = hover ? 40u : (uchar)(15 + (int)(12*gp));
        DrawRectangleRoundedLines(inner, 0.10f, 8,
                                  (Color){accent.r, accent.g, accent.b, inal});
    }

    for (int g = 5; g >= 1; g--) {
        uchar al = hover ? (uchar)(6*g + (int)(6*g*gp))
                         : (uchar)(4*g + (int)(3*g*gp));
        Color gc = {accent.r, accent.g, accent.b, al};
        DrawText(label, lx - g, ly - g, fs, gc);
        DrawText(label, lx + g, ly + g, fs, gc);
    }

    DrawText(label, lx+2, ly+3, fs, (Color){0,0,0,160});

    Color mc = hover
        ? (Color){255,255,255,255}
        : (Color){(uchar)CLAMP255(accent.r+60),
                  (uchar)CLAMP255(accent.g+60),
                  (uchar)CLAMP255(accent.b+60), 235};
    DrawText(label, lx, ly, fs, mc);

    float ulf = 0.5f + 0.5f*sinf(t*2.0f + phase);
    int ulx1  = lx - 6;
    int ulx2  = lx + lw + 6;
    int uly   = ly + fs + 4;
    uchar ulAlpha = hover ? (uchar)180 : (uchar)(60 + (int)(40*ulf));
    DrawLine(ulx1,    uly,   ulx2,    uly,
             (Color){accent.r, accent.g, accent.b, ulAlpha});
    DrawLine(ulx1+10, uly+5, ulx2-10, uly+5,
             (Color){accent.r, accent.g, accent.b, 35});

    if (hover) {
        int ax = lx + lw + 16;
        int ay = ly + fs/2 - 10;
        DrawText(">", ax+1, ay+1, 20, (Color){0,0,0,100});
        DrawText(">", ax,   ay,   20, (Color){(uchar)CLAMP255(accent.r+60),
                                              (uchar)CLAMP255(accent.g+60),
                                              (uchar)CLAMP255(accent.b+60), 220});
    }

    return hover;
}

static Rectangle DrawSubPanel(const char *title, Color accent, float t, int sw, int sh) {
    int margin2 = 32;
    int pw = sw - margin2*2, ph = sh - margin2*2;
    int px2 = margin2, py2 = margin2;
    Rectangle r = { (float)px2, (float)py2, (float)pw, (float)ph };

    float gp = 0.5f + 0.5f*sinf(t*1.5f);
    DrawRectangleRounded(r, 0.04f, 8, (Color){10,8,18,252});
    Color bdr = {(uchar)(accent.r*(0.5f+0.5f*gp)),
                 (uchar)(accent.g*(0.5f+0.5f*gp)),
                 (uchar)(accent.b*(0.5f+0.5f*gp)), 200};
    DrawRectangleRoundedLines(r, 0.04f, 8, bdr);

    DrawRectangleRounded((Rectangle){r.x+1,r.y+1,r.width-2,54},
                         0.04f,8,(Color){accent.r/7,accent.g/7,accent.b/7,240});
    DrawLine(px2+16,(int)(r.y+56),px2+pw-16,(int)(r.y+56),
             (Color){accent.r,accent.g,accent.b,70});
    int ttw = MeasureText(title, 26);
    DrawText(title, px2+pw/2-ttw/2+1, py2+15+1, 26, (Color){0,0,0,140});
    DrawText(title, px2+pw/2-ttw/2,   py2+15,   26,
             (Color){accent.r,accent.g,accent.b,255});
    DrawCornerAccents(r, (Color){accent.r,accent.g,accent.b,140}, 20);

    // ── Red BACK button (bottom-left of panel) ────────────────────────────────
    int bkW = 130, bkH = 40;
    int bkX = px2 + 20, bkY = py2 + ph - bkH - 16;
    Rectangle backR = { (float)bkX, (float)bkY, (float)bkW, (float)bkH };

    Vector2 mp = GetMousePosition();
    bool bhov = CheckCollisionPointRec(mp, backR);
    Color bkAccent = (Color){220, 55, 55, 255};

    if (bhov) {
        for (int g=4;g>=1;g--)
            DrawRectangleRounded(
                (Rectangle){backR.x-g*3,backR.y-g*2,backR.width+g*6,backR.height+g*4},
                0.4f,8,(Color){bkAccent.r,bkAccent.g,bkAccent.b,(uchar)(12*g)});
    }
    DrawRectangleRounded((Rectangle){backR.x+3,backR.y+4,backR.width,backR.height},
                         0.4f,8,(Color){0,0,0,90});
    DrawRectangleRounded(backR, 0.4f, 8,
                         bhov?(Color){70,12,12,240}:(Color){30,8,8,230});
    DrawRectangleRoundedLines(backR, 0.4f, 8,
                              bhov?bkAccent:(Color){150,35,35,200});
    DrawCornerAccents(backR,(Color){bkAccent.r,bkAccent.g,bkAccent.b,bhov?200u:100u},8);

    const char *bkLabel = "<  BACK";
    int bklw = MeasureText(bkLabel, 17);
    DrawText(bkLabel, bkX+bkW/2-bklw/2+1, bkY+bkH/2-9+1, 17, (Color){0,0,0,100});
    DrawText(bkLabel, bkX+bkW/2-bklw/2,   bkY+bkH/2-9,   17,
             bhov?(Color){255,180,180,255}:(Color){230,100,100,230});

    return backR;
}

static void DrawMenu(float t) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawMenuBG(t, sw, sh);

    // ── TUTORIAL ─────────────────────────────────────────────────────────────
    if (menuSub == 1) {
        Color ac = (Color){100,190,255,255};
        DrawSubPanel("TUTORIAL", ac, t, sw, sh);
        int margin2=32, pw=sw-margin2*2;
        int px2=margin2, py2=margin2;

        int col1x = px2+30, col2x = px2+pw/2+20;
        int topy  = py2+72;

        // ── Left column: SYMBOLS ─────────────────────────────────────────────
        DrawText("SYMBOLS", col1x, topy, 17, (Color){ac.r,ac.g,ac.b,210});
        DrawLine(col1x, topy+22, col1x+260, topy+22, (Color){ac.r,ac.g,ac.b,55});

        struct { const char* desc; Color col; } leg[] = {
            { "Treasure  Reach to advance", CTRES },
            { "Portal   Skip to next round", CPORT },
            { "Trap     Instant death!",     (Color){195,80,80,255} },
            { "Coin     Collect for score",  CCOIN },
            { "Item     Use with 1 / 2 / 3", IK_COL[IK_SPD] },
            { "Enemy    They will chase you", CENE },
        };
        for (int i=0; i<6; i++) {
            int ry2 = topy+32+i*44;
            Color rowbg=(i%2==0)?(Color){20,16,34,130}:(Color){16,13,28,70};
            DrawRectangleRounded((Rectangle){(float)col1x-4,(float)ry2-2,290,36},0.2f,6,rowbg);

            int wx = col1x, wy = ry2 + 4;
            int cs = 28;
            DrawRectangle(wx, wy, cs, cs, CB);
            DrawRectangle(wx+1, wy+1, cs-2, cs-2, CF);

            if (i == 0) {
                float p = 0.5f + 0.5f*sinf(t*3.0f);
                Color tc = CTRES; tc.a = (uchar)(200+55*p);
                DrawRectangle(wx+4,  wy+6,  cs-8, cs-10, tc);
                DrawRectangle(wx+4,  wy+6,  cs-8, 3, (Color){255,240,180,230});
                DrawCircle(wx+cs/2, wy+cs/2+1, 2, (Color){200,150,30,255});
                DrawCircle(wx+cs/2, wy+cs/2, (int)(8+3*p), (Color){255,200,50,(int)(18+20*p)});
            } else if (i == 1) {
                float p = 0.5f + 0.5f*sinf(t*2.0f);
                DrawCircle(wx+cs/2, wy+cs/2, (int)(9+3*p), (Color){80,160,255,(int)(65+50*p)});
                DrawCircle(wx+cs/2, wy+cs/2, (int)(5+2*p), (Color){160,210,255,(int)(140+60*p)});
                DrawCircleLines(wx+cs/2, wy+cs/2, (int)(9+3*p), (Color){120,190,255,200});
            } else if (i == 2) {
                DrawRectangle(wx+2, wy+2, cs-4, cs-4, (Color){20,15,25,255});
                DrawRectangleLines(wx+2, wy+2, cs-4, cs-4, (Color){60,50,70,255});
                for (int gi=0; gi<3; gi++) {
                    for (int gj=0; gj<3; gj++) {
                        int spx = wx + 6 + gi*7;
                        int spy = wy + 6 + gj*7;
                        DrawCircle(spx, spy, 2, (Color){190,190,210,255});
                        DrawRectangle(spx-1, spy-2, 2, 2, CTRAP);
                    }
                }
            } else if (i == 3) {
                // Coin '$'
                float p = sinf(t*4.0f);
                int r = (int)(5 + p*1.2f);
                DrawCircle(wx+cs/2+1, wy+cs/2+2, r, (Color){160,120,0,80});
                DrawCircle(wx+cs/2,   wy+cs/2,   r, CCOIN);
                DrawCircle(wx+cs/2-1, wy+cs/2-1, (int)(r*0.4f)+1, (Color){255,245,160,200});
            } else if (i == 4) {
                Color ic = IK_COL[IK_SPD];
                float p = 0.5f + 0.5f*sinf(t*3.0f);
                DrawTriangle(
                    (Vector2){(float)(wx+cs/2), (float)(wy+3)},
                    (Vector2){(float)(wx+cs-3),  (float)(wy+cs/2)},
                    (Vector2){(float)(wx+cs/2), (float)(wy+cs-3)}, ic);
                DrawTriangle(
                    (Vector2){(float)(wx+cs/2), (float)(wy+3)},
                    (Vector2){(float)(wx+cs/2), (float)(wy+cs-3)},
                    (Vector2){(float)(wx+3),     (float)(wy+cs/2)}, ic);
                Color gc = ic; gc.a = (uchar)(55*p);
                DrawCircle(wx+cs/2, wy+cs/2, (int)(10+2*p), gc);
            } else if (i == 5) {
                DrawCircle(wx+cs/2, wy+cs/2, cs/2-3, CENE);
                DrawCircle(wx+cs/2-2, wy+cs/2-2, cs/4-1, (Color){240,100,100,110});
                DrawCircle(wx+cs/2-3, wy+cs/2-2, 2, (Color){255,200,0,255});
                DrawCircle(wx+cs/2+3, wy+cs/2-2, 2, (Color){255,200,0,255});
                DrawCircle(wx+cs/2-3, wy+cs/2-2, 1, (Color){20,0,0,255});
                DrawCircle(wx+cs/2+3, wy+cs/2-2, 1, (Color){20,0,0,255});
            }

            DrawText(leg[i].desc, col1x+34, ry2+6, 14, (Color){170,165,195,220});
        }

        // ── Right column: ITEMS ───────────────────────────────────────────────
        DrawText("ITEMS", col2x, topy, 17, (Color){ac.r,ac.g,ac.b,210});
        DrawLine(col2x, topy+22, col2x+320, topy+22, (Color){ac.r,ac.g,ac.b,55});

        const char *ileg[] = { "SPD","SHD","BMB","WLK","SWD" };
        const char *idesc[] = {
            "2x movement speed",
            "Block one trap or enemy hit",
            "Destroy all nearby traps",
            "Walk through walls",
            "Instantly kill nearby enemies"
        };
        Color iicol[] = {IK_COL[IK_SPD],IK_COL[IK_SHD],IK_COL[IK_BMB],IK_COL[IK_WLK],IK_COL[IK_SWD]};
        for (int i=0; i<5; i++) {
            int ry2 = topy+32+i*50;
            Color rowbg2=(i%2==0)?(Color){20,16,34,130}:(Color){16,13,28,70};
            DrawRectangleRounded((Rectangle){(float)col2x-4,(float)ry2-2,340,40},0.2f,6,rowbg2);

            int cs2 = 32;
            int iwx = col2x, iwy = ry2 + 4;
            DrawRectangle(iwx, iwy, cs2, cs2, CB);
            DrawRectangle(iwx+1, iwy+1, cs2-2, cs2-2, CF);
            Color ic = iicol[i];
            float p2 = 0.5f + 0.5f*sinf(t*3.0f + (float)i*0.8f);
            DrawTriangle(
                (Vector2){(float)(iwx+cs2/2), (float)(iwy+3)},
                (Vector2){(float)(iwx+cs2-3), (float)(iwy+cs2/2)},
                (Vector2){(float)(iwx+cs2/2), (float)(iwy+cs2-3)}, ic);
            DrawTriangle(
                (Vector2){(float)(iwx+cs2/2), (float)(iwy+3)},
                (Vector2){(float)(iwx+cs2/2), (float)(iwy+cs2-3)},
                (Vector2){(float)(iwx+3),     (float)(iwy+cs2/2)}, ic);
            Color gc2 = ic; gc2.a = (uchar)(55*p2);
            DrawCircle(iwx+cs2/2, iwy+cs2/2, (int)(12+3*p2), gc2);

            DrawText(idesc[i], col2x+40, ry2+5,  12, (Color){160,155,185,215});
            DrawText(ileg[i],  col2x+40, ry2+19, 13, ic);
        }

        DrawText("WASD / Arrow Keys: Move    1 / 2 / 3: Use Item    ESC: Pause    F11: Fullscreen",
                 px2+30, py2+(sh-margin2*2)-90, 14, (Color){100,95,130,180});

        // ── Explanation box (bottom right) ───────────────────────────────────
        {
            int bw = (pw/2) - 30;
            int bh = 120;
            int bx = col2x - 4;
            int by = py2 + (sh-margin2*2) - bh - 8;

            DrawRectangleRounded((Rectangle){(float)bx,(float)by,(float)bw,(float)bh},
                                 0.12f, 8, (Color){14,11,22,190});
            DrawRectangleRoundedLines((Rectangle){(float)bx,(float)by,(float)bw,(float)bh},
                                     0.12f, 8, (Color){ac.r,ac.g,ac.b,80});
            DrawRectangleRoundedLines(
                (Rectangle){(float)(bx+3),(float)(by+3),(float)(bw-6),(float)(bh-6)},
                0.12f, 8, (Color){ac.r,ac.g,ac.b,25});
            DrawRectangleRounded((Rectangle){(float)bx,(float)(by+8),4,(float)(bh-16)},
                                 1.0f, 4, (Color){ac.r,ac.g,ac.b,140});

            // title
            DrawText("HOW TO PLAY", bx+12, by+10, 15, (Color){ac.r,ac.g,ac.b,220});
            DrawLine(bx+12, by+28, bx+bw-12, by+28, (Color){ac.r,ac.g,ac.b,40});

            const char *lines[] = {
                "Navigate the dungeon and find the Treasure to advance.",
                "Avoid Traps! One touch and you're dead.",
                "Collect Coins and Items as you explore each floor.",
                "Enemies chase you. Use items wisely to survive.",
                "Each round the map grows larger and deadlier.",
            };
            for (int li=0; li<5; li++)
                DrawText(lines[li], bx+14, by+34+li*17, 13, (Color){155,150,185,210});
        }
        return;
    }

    // ── STATS ─────────────────────────────────────────────────────────────────
    if (menuSub == 2) {
        Color ac = (Color){255,205,55,255};
        DrawSubPanel("STATS", ac, t, sw, sh);
        int margin2=32, pw=sw-margin2*2, ph=sh-margin2*2;
        int px2=margin2, py2=margin2;

        int zoneTop    = py2 + 64;
        int zoneBottom = py2 + ph - 70;
        int availH     = zoneBottom - zoneTop;
        
        float avg_round = (global_games_played > 0) ? ((float)global_rounds / global_games_played) : 0.0f;
        float avg_coi   = (global_games_played > 0) ? ((float)global_totcoi  / global_games_played) : 0.0f;
        float avg_trs   = (global_games_played > 0) ? ((float)global_tottrs  / global_games_played) : 0.0f;
        float avg_kills = (global_games_played > 0) ? ((float)global_totkills / global_games_played) : 0.0f;

        int rowH = availH / 7;
        if (rowH < 52) rowH = 52;
        int totalRowsH = 14 * rowH;

        int maxScroll = totalRowsH - availH;
        if (maxScroll < 0) maxScroll = 0;
        float wheel = GetMouseWheelMoveV().y;
        if (wheel != 0.0f) {statScroll -= (int)(wheel * 40.0f);}
        if (IsKeyDown(KEY_UP))   statScroll -= 7;
        if (IsKeyDown(KEY_DOWN)) statScroll += 7;
        if (statScroll < 0) statScroll = 0;
        if (statScroll > maxScroll) statScroll = maxScroll;

        struct { const char *label; float val; bool is_avg; Color col; } stats[] = {
            { "Total Rounds Played",        (float)global_rounds,       false, (Color){200,195,235,255} },
            { "Best Round Reached",         (float)hs_round,            false, (Color){200,195,235,255} },
            { "Coin Balance",               (float)coin_balance,        false, (Color){255,215,  0,255} },
            { "Total Coins Collected",      (float)global_totcoi,       false, (Color){255,200, 40,255} },
            { "Most Coins in One Game",     (float)hs_coi,              false, (Color){255,215,  0,255} },
            { "Total Treasures Collected",  (float)global_tottrs,       false, CTRES                    },
            { "Most Treasures in One Game", (float)hs_trs,              false, CTRES                    },
            { "Total Enemy Kills",          (float)global_totkills,     false, (Color){220, 90, 90,255} },
            { "Most Kills in One Round",    (float)hs_kills,            false, (Color){220, 75, 75,255} },
            { "Total Games Played",         (float)global_games_played, false, (Color){200,185,255,255} },
            { "Average Rounds per Game",    avg_round,                  true,  (Color){200,185,255,255} },
            { "Average Treasures per Game", avg_trs,                    true,  (Color){200,185,255,255} },
            { "Average Coins per Game",     avg_coi,                    true,  (Color){200,185,255,255} },
            { "Average Kills per Game",     avg_kills,                  true,  (Color){200,185,255,255} },
        };
        Color icolS[] = {            
            (Color){200,195,235,255},
            (Color){200,195,235,255},
            (Color){255,215,  0,255},
            (Color){255,200, 40,255},
            (Color){255,215,  0,255},
            CTRES,
            CTRES,
            (Color){220, 90, 90,255},
            (Color){220, 75, 75,255},
            (Color){200,185,255,255},
            (Color){200,185,255,255}, 
            (Color){200,185,255,255},
            (Color){200,185,255,255},
            (Color){200,185,255,255},
        };

        int cx = px2 + 40;

        BeginScissorMode(px2, zoneTop, pw, availH);
        for (int i = 0; i < 14; i++) {
            int ry2 = zoneTop + i * rowH - statScroll;
            if (ry2 + rowH < zoneTop || ry2 > zoneBottom) continue;

            Color rowbg = (i % 2 == 0) ? (Color){20,16,34,160} : (Color){15,12,26,90};
            DrawRectangleRounded(
                (Rectangle){(float)(cx-8),(float)(ry2-4),(float)(pw-64),(float)(rowH-6)},
                0.12f, 6, rowbg);
            DrawRectangleRounded(
                (Rectangle){(float)(cx-8),(float)(ry2-4),4.0f,(float)(rowH-6)},
                1.0f, 4, icolS[i]);
            DrawText(stats[i].label, cx+6, ry2+2, 16, (Color){140,135,170,210});
            DrawText(stats[i].is_avg ? TextFormat("%.2f", stats[i].val)
                                     : TextFormat("%d", (int)stats[i].val),
                     cx+6, ry2+22, 24, stats[i].col);
        }
        EndScissorMode();

        if (maxScroll > 0) {
            int sbX  = px2 + pw - 10;
            int sbY  = zoneTop;
            int sbH  = availH;
            int sbTH = (int)((float)availH / totalRowsH * sbH);
            if (sbTH < 20) sbTH = 20;
            int sbTY = sbY + (int)((float)statScroll / maxScroll * (sbH - sbTH));
            DrawRectangleRounded((Rectangle){(float)sbX,(float)sbY,4.0f,(float)sbH},
                                 1.0f,4,(Color){ac.r,ac.g,ac.b,30});
            DrawRectangleRounded((Rectangle){(float)sbX,(float)sbTY,4.0f,(float)sbTH},
                                 1.0f,4,(Color){ac.r,ac.g,ac.b,140});
        }
        return;
    }

    // ── SHOP ──────────────────────────────────────────────────────────────────
    if (menuSub == 3) {
        Color ac = (Color){70,215,140,255};
        DrawSubPanel("SHOP", ac, t, sw, sh);
        int margin2=32, pw=sw-margin2*2, ph=sh-margin2*2;
        int px2=margin2, py2=margin2;

        static const Color SKIN_COLS[10] = {
            {  90,220,120,255 }, { 255, 80, 80,255 }, { 255,160, 40,255 },
            { 255,220, 40,255 }, {  60,200, 60,255 }, {  40,200,220,255 },
            {  60,120,255,255 }, { 160, 60,255,255 }, { 220, 60,200,255 },
            { 255,255,255,255 },
        };
        static const char* SKIN_NAMES[10] = {
            "Green","Red","Orange","Yellow","Lime",
            "Cyan","Blue","Violet","Magenta","White"
        };

        // ── coin balance header ───────────────────────────────────────────────
        int balY = py2 + 64;
        DrawCircle(px2+pw-80, balY+10, 9, CCOIN);
        DrawCircle(px2+pw-83, balY+7,  4, (Color){255,245,160,200});
        int balw = MeasureText(TextFormat("%d", coin_balance), 20);
        DrawText(TextFormat("%d", coin_balance), px2+pw-65, balY+1, 20, CCOIN);
        (void)balw;

        // ── tab bar ───────────────────────────────────────────────────────────
        int tabY  = balY + 34;
        int tabW  = (pw - 60) / 3;
        const char *tabLabels[] = { "SKINS", "UPGRADES", "PERKS" };
        Color tabAc[] = {
            (Color){ 90,220,120,255},
            (Color){255,200, 50,255},
            (Color){185, 90,255,255}
        };
        for (int i=0;i<3;i++) {
            int tx = px2+20 + i*(tabW+10);
            bool sel = (shopTab==i);
            float gp = 0.5f+0.5f*sinf(t*1.8f+(float)i*1.1f);
            if (sel) {
                for (int g=4;g>=1;g--)
                    DrawRectangleRounded((Rectangle){(float)(tx-g*2),(float)(tabY-g),(float)(tabW+g*4),(float)(34+g*2)},
                                        0.25f,6,(Color){tabAc[i].r,tabAc[i].g,tabAc[i].b,(uchar)(10*g)});
            }
            DrawRectangleRounded((Rectangle){(float)tx,(float)tabY,(float)tabW,34},
                                 0.25f,6, sel?(Color){20,16,34,220}:(Color){14,11,22,160});
            Color bord = sel ? tabAc[i] : (Color){tabAc[i].r,tabAc[i].g,tabAc[i].b,(uchar)(60+(int)(40*gp))};
            DrawRectangleRoundedLines((Rectangle){(float)tx,(float)tabY,(float)tabW,34},0.25f,6,bord);
            int tw=MeasureText(tabLabels[i],15);
            DrawText(tabLabels[i]+1,tx+tabW/2-tw/2+2,tabY+9+1,15,(Color){0,0,0,80});
            DrawText(tabLabels[i],  tx+tabW/2-tw/2,  tabY+9,  15,
                     sel?(Color){255,255,255,255}:(Color){tabAc[i].r,tabAc[i].g,tabAc[i].b,180});
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mp2=GetMousePosition();
                Rectangle tr={(float)tx,(float)tabY,(float)tabW,34};
                if (CheckCollisionPointRec(mp2,tr)) shopTab=i;
            }
        }

        // content zone
        int cZoneTop  = tabY + 44;
        int cZoneBot  = py2 + ph - 70;
        int cZoneH    = cZoneBot - cZoneTop;
        int cZoneX    = px2 + 12;
        int cZoneW    = pw - 24;

        // ── TAB 0: SKINS ─────────────────────────────────────────────────────
        if (shopTab == 0) {
            int cellSz = 110, cellGap = 16;
            int rowOff = (cZoneW - (5*cellSz + 4*cellGap)) / 2;
            for (int i=0;i<10;i++) {
                int col=i%5, row=i/5;
                int cx2 = cZoneX + rowOff + col*(cellSz+cellGap);
                int cy2 = cZoneTop + 8 + row*(cellSz+cellGap+28);
                bool owned = skin_owned[i];
                bool equip = (skin_equipped==i);
                float gp   = 0.5f+0.5f*sinf(t*2.0f+(float)i*0.7f);

                if (equip) {
                    for (int g=5;g>=1;g--)
                        DrawRectangleRounded(
                            (Rectangle){(float)(cx2-g*3),(float)(cy2-g*3),(float)(cellSz+g*6),(float)(cellSz+g*6)},
                            0.22f,8,(Color){SKIN_COLS[i].r,SKIN_COLS[i].g,SKIN_COLS[i].b,(uchar)(8*g)});
                }
                DrawRectangleRounded((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                     0.18f,8,(Color){16,13,26,200});
                Color bord2 = owned
                    ? (Color){SKIN_COLS[i].r,SKIN_COLS[i].g,SKIN_COLS[i].b,equip?255u:(uchar)(80+(int)(60*gp))}
                    : (Color){55,50,75,120};
                DrawRectangleRoundedLines((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                         0.18f,8,bord2);

                int pcx=cx2+cellSz/2, pcy=cy2+cellSz/2-8;
                Color pc2 = owned ? SKIN_COLS[i] : (Color){40,38,55,255};
                DrawCircle(pcx,pcy,18,pc2);
                DrawCircle(pcx-3,pcy-4,9,(Color){CLAMP255(pc2.r+55),CLAMP255(pc2.g+55),CLAMP255(pc2.b+55),120});
                DrawCircle(pcx-5,pcy-3,3,(Color){10,10,22,255});
                DrawCircle(pcx+5,pcy-3,3,(Color){10,10,22,255});
                if (!owned) {
                    DrawText("?", pcx-6,pcy-10,20,(Color){80,75,110,200});
                }

                int nw2=MeasureText(SKIN_NAMES[i],13);
                DrawText(SKIN_NAMES[i],cx2+cellSz/2-nw2/2,cy2+cellSz-22,13,
                         owned?(Color){200,195,225,220}:(Color){80,75,110,180});

                int btnY=cy2+cellSz+6, btnH=26;
                Rectangle btnR={(float)cx2,(float)btnY,(float)cellSz,(float)btnH};
                Vector2 mp2=GetMousePosition();
                bool bhov=CheckCollisionPointRec(mp2,btnR);

                if (equip) {
                    DrawRectangleRounded(btnR,0.3f,6,(Color){20,50,30,220});
                    DrawRectangleRoundedLines(btnR,0.3f,6,ac);
                    int ew=MeasureText("EQUIPPED",12);
                    DrawText("EQUIPPED",cx2+cellSz/2-ew/2,btnY+7,12,(Color){70,215,140,255});
                } else if (owned) {
                    Color bf=(Color){bhov?35:20,bhov?28:14,bhov?52:32,230};
                    DrawRectangleRounded(btnR,0.3f,6,bf);
                    DrawRectangleRoundedLines(btnR,0.3f,6,(Color){SKIN_COLS[i].r,SKIN_COLS[i].g,SKIN_COLS[i].b,bhov?200u:100u});
                    int ew=MeasureText("EQUIP",12);
                    DrawText("EQUIP",cx2+cellSz/2-ew/2,btnY+7,12,
                             (Color){CLAMP255(SKIN_COLS[i].r+50),CLAMP255(SKIN_COLS[i].g+50),CLAMP255(SKIN_COLS[i].b+50),220});
                    if (bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        skin_equipped=i; SaveShop();
                    }
                } else {
                    bool canBuy=(coin_balance>=200);
                    Color bf=canBuy?(bhov?(Color){30,50,25,230}:(Color){20,35,18,200}):(Color){20,18,28,160};
                    DrawRectangleRounded(btnR,0.3f,6,bf);
                    DrawRectangleRoundedLines(btnR,0.3f,6,canBuy?(Color){70,215,140,bhov?220u:100u}:(Color){50,46,68,80});
                    DrawCircle(cx2+cellSz/2-22,btnY+13,5,canBuy?CCOIN:(Color){80,70,40,180});
                    int pw2=MeasureText("200",12);
                    DrawText("200",cx2+cellSz/2-11,btnY+7,12,canBuy?(Color){255,215,0,220}:(Color){100,90,60,160});
                    (void)pw2;
                    if (canBuy && bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        coin_balance-=200; skin_owned[i]=1; skin_equipped=i;
                        SaveStats(); SaveShop();
                    }
                }
            }
        }

        // ── TAB 1: UPGRADES ──────────────────────────────────────────────────
        if (shopTab == 1) {
            struct { const char*name; const char*desc; int maxlvl; int*lvl; int cost; Color col; } upgrades[] = {
                { "SPEED BOOST", "Extends SPD item duration", 2, &spd_level, 400, IK_COL[IK_SPD] },
            };
            int nUpg = 1;
            static const char* spd_descs[] = { "4 seconds (base)", "6 seconds", "8 seconds" };

            for (int i=0;i<nUpg;i++) {
                int ux=cZoneX+20, uy=cZoneTop+12+i*140;
                int uw=cZoneW-40, uh=120;
                float gp=0.5f+0.5f*sinf(t*1.6f+(float)i*1.2f);

                DrawRectangleRounded((Rectangle){(float)ux,(float)uy,(float)uw,(float)uh},
                                     0.12f,8,(Color){16,13,26,200});
                DrawRectangleRoundedLines((Rectangle){(float)ux,(float)uy,(float)uw,(float)uh},
                                         0.12f,8,(Color){upgrades[i].col.r,upgrades[i].col.g,upgrades[i].col.b,
                                                         (uchar)(60+(int)(50*gp))});
                DrawRectangleRounded((Rectangle){(float)ux,(float)uy,4,(float)uh},
                                     1.0f,4,upgrades[i].col);

                // diamond icon
                int dix=ux+28, diy=uy+uh/2;
                DrawTriangle((Vector2){(float)dix,(float)(diy-16)},
                             (Vector2){(float)(dix+14),(float)diy},
                             (Vector2){(float)dix,(float)(diy+16)}, upgrades[i].col);
                DrawTriangle((Vector2){(float)dix,(float)(diy-16)},
                             (Vector2){(float)dix,(float)(diy+16)},
                             (Vector2){(float)(dix-14),(float)diy}, upgrades[i].col);

                DrawText(upgrades[i].name, ux+56,uy+14,18,upgrades[i].col);
                DrawText(upgrades[i].desc, ux+56,uy+36,14,(Color){140,135,170,210});

                DrawText(TextFormat("Level %d / %d  (%s)",*upgrades[i].lvl,upgrades[i].maxlvl,
                                    spd_descs[*upgrades[i].lvl]),
                         ux+56,uy+56,13,(Color){170,165,200,220});

                for (int l=0;l<upgrades[i].maxlvl;l++) {
                    int px3=ux+56+l*26, py3=uy+78;
                    bool filled=(l < *upgrades[i].lvl);
                    DrawCircle(px3,py3,8,filled?upgrades[i].col:(Color){30,26,44,200});
                    DrawCircleLines(px3,py3,8,upgrades[i].col);
                    if (filled) DrawCircle(px3,py3,4,(Color){255,255,255,180});
                }

                bool maxed=(*upgrades[i].lvl >= upgrades[i].maxlvl);
                int btnX2=ux+uw-180, btnY2=uy+uh/2-20, btnW2=160, btnH2=40;
                Rectangle btnR2={(float)btnX2,(float)btnY2,(float)btnW2,(float)btnH2};
                Vector2 mp2=GetMousePosition();
                bool bhov=CheckCollisionPointRec(mp2,btnR2);
                bool canBuy=(coin_balance>=upgrades[i].cost && !maxed);

                Color bf=maxed?(Color){20,18,30,160}:canBuy?(bhov?(Color){35,60,28,240}:(Color){20,40,18,200}):(Color){20,18,30,160};
                DrawRectangleRounded(btnR2,0.25f,8,bf);
                DrawRectangleRoundedLines(btnR2,0.25f,8,maxed?(Color){60,56,80,100}:canBuy?(Color){70,215,140,bhov?220u:120u}:(Color){60,56,80,100});

                if (maxed) {
                    int mw2=MeasureText("MAX LEVEL",14);
                    DrawText("MAX LEVEL",btnX2+btnW2/2-mw2/2,btnY2+13,14,(Color){120,115,150,180});
                } else {
                    DrawCircle(btnX2+28,btnY2+20,8,canBuy?CCOIN:(Color){70,60,30,160});
                    DrawText(TextFormat("%d",upgrades[i].cost),btnX2+40,btnY2+13,14,
                             canBuy?(Color){255,215,0,230}:(Color){100,90,55,160});
                    int uw2=MeasureText("UPGRADE",14);
                    DrawText("UPGRADE",btnX2+btnW2/2-uw2/2+20,btnY2+13,14,
                             canBuy?(Color){200,255,200,230}:(Color){90,85,115,160});
                    if (canBuy && bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        coin_balance-=upgrades[i].cost; (*upgrades[i].lvl)++;
                        SaveStats(); SaveShop();
                    }
                }
            }
        }

        // ── TAB 2: PERKS ─────────────────────────────────────────────────────
        if (shopTab == 2) {
            struct { const char*name; const char*desc; int*flag; int maxv; int cost; Color col; } perks[] = {
                { "EXTRA SLOT I",   "Carry one more item in your inventory",    &perk_slots,   2, 500, (Color){160,150,220,255} },
                { "EXTRA SLOT II",  "Carry two more items (requires Slot I)",   &perk_slots,   2, 1000, (Color){130,120,200,255} },
                { "COIN MAGNET",    "Automatically collect adjacent coins",     &perk_magnet,  1, 2000, CCOIN                    },
                { "SHIELD CHARGE",  "Start each round with a shield charge",    &perk_shield2, 1, 1500, (Color){ 80,150,255,255} },
            };
            int nPerks=4;

            for (int i=0;i<nPerks;i++) {
                int ux=cZoneX+20, uy=cZoneTop+10+i*108;
                int uw=cZoneW-40, uh=90;
                bool purchased=false;
                if (perks[i].maxv==1) purchased=(*perks[i].flag>=1);
                else purchased=(*perks[i].flag>=i+1);
                bool locked=(i==1 && perk_slots<1);
                float gp=0.5f+0.5f*sinf(t*1.6f+(float)i*0.9f);

                DrawRectangleRounded((Rectangle){(float)ux,(float)uy,(float)uw,(float)uh},
                                     0.12f,8,(Color){16,13,26,200});
                DrawRectangleRoundedLines((Rectangle){(float)ux,(float)uy,(float)uw,(float)uh},
                                         0.12f,8,(Color){perks[i].col.r,perks[i].col.g,perks[i].col.b,
                                                         purchased?180u:(uchar)(40+(int)(40*gp))});
                DrawRectangleRounded((Rectangle){(float)ux,(float)uy,4,(float)uh},1.0f,4,
                                     purchased?perks[i].col:(Color){perks[i].col.r,perks[i].col.g,perks[i].col.b,80});

                DrawText(perks[i].name, ux+18,uy+12,17,purchased?perks[i].col:(Color){perks[i].col.r,perks[i].col.g,perks[i].col.b,150});
                DrawText(locked?"Requires Extra Slot I first":perks[i].desc,
                         ux+18,uy+34,13,(Color){140,135,170,210});

                if (purchased) {
                    int pw3=MeasureText("ACTIVE",13);
                    DrawText("ACTIVE",ux+18,uy+56,13,(Color){70,215,140,220});
                    DrawCircle(ux+18+pw3+12,uy+62,5,(Color){70,215,140,200});
                } else {
                    // buy button
                    int btnX3=ux+uw-185, btnY3=uy+uh/2-18, btnW3=165, btnH3=36;
                    Rectangle btnR3={(float)btnX3,(float)btnY3,(float)btnW3,(float)btnH3};
                    Vector2 mp2=GetMousePosition();
                    bool bhov=(!locked)&&CheckCollisionPointRec(mp2,btnR3);
                    bool canBuy=(!locked)&&(coin_balance>=perks[i].cost);

                    Color bf=locked?(Color){18,16,28,130}:canBuy?(bhov?(Color){35,28,60,240}:(Color){22,18,42,200}):(Color){18,16,28,160};
                    DrawRectangleRounded(btnR3,0.25f,8,bf);
                    DrawRectangleRoundedLines(btnR3,0.25f,8,locked?(Color){45,42,62,80}:canBuy?(Color){perks[i].col.r,perks[i].col.g,perks[i].col.b,bhov?220u:110u}:(Color){50,46,70,80});

                    if (!locked) {
                        DrawCircle(btnX3+24,btnY3+18,7,canBuy?CCOIN:(Color){70,60,30,160});
                        DrawText(TextFormat("%d",perks[i].cost),btnX3+35,btnY3+11,14,canBuy?(Color){255,215,0,220}:(Color){100,90,55,150});
                        int bw2=MeasureText("UNLOCK",14);
                        DrawText("UNLOCK",btnX3+btnW3/2-bw2/2+18,btnY3+11,14,canBuy?(Color){220,215,255,230}:(Color){90,85,115,150});
                        if (canBuy && bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            coin_balance-=perks[i].cost;
                            if (perks[i].maxv==1) *perks[i].flag=1;
                            else (*perks[i].flag)++;
                            SaveStats(); SaveShop();
                        }
                    } else {
                        int bw2=MeasureText("LOCKED",14);
                        DrawText("LOCKED",btnX3+btnW3/2-bw2/2,btnY3+11,14,(Color){70,65,95,160});
                    }
                }
            }
        }

        return;
    }

    // ── FUTURE UPDATES ────────────────────────────────────────────────────────
    if (menuSub == 4) {
        Color ac = (Color){185,90,255,255};
        DrawSubPanel("FUTURE UPDATES", ac, t, sw, sh);
        int margin2=32,pw=sw-margin2*2,ph=sh-margin2*2;
        int px2=margin2,py2=margin2;
        float pp = 0.45f + 0.3f*sinf(t*1.4f);
        const char *cs = "Coming Soon";
        int csw2=MeasureText(cs,44);
        for(int i=0;i<8;i++) {
            float a=(float)i/8*3.14159f*2+t*0.4f;
            int dx2=(int)(cosf(a)*90), dy2=(int)(sinf(a)*45);
            DrawCircle(px2+pw/2+dx2, py2+ph/2+dy2, 3,
                       (Color){ac.r,ac.g,ac.b,(uchar)(25+(int)(35*pp))});
        }
        DrawText(cs, px2+pw/2-csw2/2+2, py2+ph/2-28+2, 44, (Color){0,0,0,100});
        DrawText(cs, px2+pw/2-csw2/2,   py2+ph/2-28,   44,
                 (Color){(uchar)(ac.r*pp+20),(uchar)(ac.g*pp+20),(uchar)(ac.b*pp+30),210});
        return;
    }

// Main Menu
    {
        float lf = 0.5f+0.5f*sinf(t*1.1f);
        DrawLine(0, sh/2-162, sw, sh/2-162,
                 (Color){(uchar)(40+30*lf),35,(uchar)(70+40*lf),60});
    }

    // ── TITLE ─────────────────────────────────────────────────────────────────
    {
        const char *TT = "DUNGEON CRAWLER";
        int ts = 68, ttw = MeasureText(TT, ts);
        int ty = 28;

        float gp = 0.5f+0.5f*sinf(t*1.1f);
        for (int g=7;g>=1;g--) {
            Color gc={(uchar)(50+30*gp),(uchar)(90+60*gp),255,(uchar)(8*g)};
            DrawText(TT, sw/2-ttw/2-g, ty-g, ts, gc);
            DrawText(TT, sw/2-ttw/2+g, ty+g, ts, gc);
        }
        DrawText(TT, sw/2-ttw/2+3, ty+4, ts, (Color){0,0,0,160});
        DrawText(TT, sw/2-ttw/2,   ty,   ts, (Color){230,225,250,255});

        int ulx1=sw/2-ttw/2-10, ulx2=sw/2+ttw/2+10;
        float ulf=0.5f+0.5f*sinf(t*2.0f);
        DrawLine(ulx1,   ty+ts+4,  ulx2,   ty+ts+4,
                 (Color){(uchar)(50+80*ulf),100,(uchar)(210+40*ulf),110});
        DrawLine(ulx1+30,ty+ts+9,  ulx2-30,ty+ts+9,
                 (Color){40,80,180,45});

        float pp=0.5f+0.5f*sinf(t*2.0f);
        float pp2=0.5f+0.5f*sinf(t*1.3f+1.2f);
        Color sc={(uchar)(200+50*pp),(uchar)(120+100*pp2),(uchar)(30+60*pp),255};
        const char *ST="PRESS  ENTER  TO  START";
        DrawText(ST, sw/2-MeasureText(ST,26)/2+1, ty+ts+18+1, 26, (Color){0,0,0,100});
        DrawText(ST, sw/2-MeasureText(ST,26)/2,   ty+ts+18,   26, sc);
    }

    // ── 4 NAV BUTTONS ────────────────────────────────────────────────────────
    int M = 14;                         
    int titleBottom = 152;              

    int cW = (int)(sw * 0.28f);        

    int centreGap = 148;                 
    int midY      = (titleBottom + sh - M) / 2; 
    int cHtop     = midY - titleBottom - M/2;   
    int cHbot     = sh - M - (midY + M/2) - 1;  

    if (cHtop < 80)  cHtop = 80;
    if (cHbot < 80)  cHbot = 80;

    int topY = titleBottom;
    int botY = midY + M/2;

    int rightCardX = sw - M - cW;   // right cards flush with right margin
    Rectangle btnTL = { (float)M,           (float)topY, (float)cW, (float)cHtop };
    Rectangle btnTR = { (float)rightCardX,  (float)topY, (float)cW, (float)cHtop };
    Rectangle btnBL = { (float)M,           (float)botY, (float)cW, (float)cHbot };
    Rectangle btnBR = { (float)rightCardX,  (float)botY, (float)cW, (float)cHbot };

    DrawNavCard(btnTL, "TUTORIAL",       (Color){90,175,255,255}, t, 0.0f);
    DrawNavCard(btnTR, "STATS",          (Color){255,200,50,255}, t, 0.9f);
    DrawNavCard(btnBL, "SHOP",           (Color){65,210,130,255}, t, 1.8f);
    DrawNavCard(btnBR, "FUTURE UPDATES", (Color){185,85,255,255}, t, 2.7f);

    // ── PLAY BUTTON – pure glowing text, title style ──────────────────────────
    {
        float cyc  = fmodf(t * 0.55f, 1.0f);
        Color cols[5] = { CTRES, CENE, CPORT, CWLK, CPL };
        float pos5 = cyc * 5.0f;
        int   idx0 = (int)pos5 % 5;
        int   idx1 = (idx0 + 1) % 5;
        float frac = pos5 - (float)(int)pos5;
        Color cA = cols[idx0], cB = cols[idx1];
        Color acPlay = {
            (uchar)(cA.r + (cB.r-cA.r)*frac),
            (uchar)(cA.g + (cB.g-cA.g)*frac),
            (uchar)(cA.b + (cB.b-cA.b)*frac), 255
        };

        int rightCardX2  = sw - M - cW;
        int centreColW = rightCardX2 - (7*M + cW);
        int pbW = centreColW - 16;
        int pbH = (int)(sh * 0.35f);
        int pbX = sw/2 - pbW/2;
        int pbY = midY - pbH/2 - 16;
        Rectangle pbRect = { (float)pbX,(float)pbY,(float)pbW,(float)pbH };

        Vector2 mp = GetMousePosition();
        bool hover = CheckCollisionPointRec(mp, pbRect);
        float gp2  = 0.5f + 0.5f*sinf(t*3.2f);

        int   playFS   = 72;
        float triH2    = playFS * 0.48f;
        int   playTW   = MeasureText("PLAY", playFS);
        float unitW    = triH2*2.0f + 18.0f + (float)playTW;
        float unitX    = sw/2.0f - unitW/2.0f;
        float unitCY   = (float)(midY); 

        float triCX = unitX + triH2;
        Vector2 tp0 = { triCX - triH2*0.65f, unitCY - triH2 };
        Vector2 tp1 = { triCX - triH2*0.65f, unitCY + triH2 };
        Vector2 tp2 = { triCX + triH2,        unitCY         };

        int plTx = (int)(triCX + triH2 + 18.0f);
        int plTy = (int)(unitCY - playFS/2);

        for (int g = 8; g >= 1; g--) {
            uchar al = hover ? (uchar)(18*g + (int)(8*g*gp2))
                             : (uchar)(10*g + (int)(4*g*gp2));
            Color gc = {acPlay.r, acPlay.g, acPlay.b, al};
            DrawText("PLAY", plTx - g, plTy - g, playFS, gc);
            DrawText("PLAY", plTx + g, plTy + g, playFS, gc);
            float hx = triCX + (tp2.x - triCX)*0.1f;
            DrawCircle((int)hx, (int)unitCY, triH2 + g*3, (Color){acPlay.r,acPlay.g,acPlay.b,(uchar)(al/3)});
        }

        DrawText("PLAY", plTx+3, plTy+4, playFS, (Color){0,0,0,160});

        Color triCol = hover ? (Color){255,255,255,255}
                             : (Color){acPlay.r,acPlay.g,acPlay.b,230};
        DrawTriangle(tp0, tp1, tp2, triCol);

        Color playCol = hover ? (Color){255,255,255,255}
                              : (Color){acPlay.r,acPlay.g,acPlay.b,235};
        DrawText("PLAY", plTx, plTy, playFS, playCol);

        float ulf2 = 0.5f + 0.5f*sinf(t*2.0f);
        int ulMid  = sw/2;
        int ulHalf = (int)(unitW/2) + 10;
        int ulY    = (int)(unitCY + triH2 + 12);
        DrawLine(ulMid - ulHalf,    ulY,   ulMid + ulHalf,    ulY,
                 (Color){acPlay.r, acPlay.g, acPlay.b,
                         hover ? (uchar)200 : (uchar)(80 + (int)(80*ulf2))});
        DrawLine(ulMid - ulHalf+30, ulY+5, ulMid + ulHalf-30, ulY+5,
                 (Color){acPlay.r, acPlay.g, acPlay.b, 35});
    }

    {
        int hy = midY + (int)(sh * 0.3f);
        const char *ET = eneon ? "[ E ]  Enemies: ON" : "[ E ]  Enemies: OFF";
        Color ec = eneon ? (Color){230,75,75,240} : (Color){120,115,160,210};
        int ew = MeasureText(ET,22);
        DrawRectangleRounded((Rectangle){(float)(sw/2-ew/2-14),(float)(hy-5),
                                          (float)(ew+28),30},0.5f,6,(Color){20,16,34,170});
        DrawText(ET, sw/2-ew/2+1, hy+1, 22, (Color){0,0,0,110});
        DrawText(ET, sw/2-ew/2,   hy,   22, ec);

        const char *FT = "[F11]  Fullscreen";
        int fw = MeasureText(FT,18);
        DrawText(FT, sw/2-fw/2+1, hy+36+1, 18, (Color){0,0,0,80});
        DrawText(FT, sw/2-fw/2,   hy+36,   18, (Color){95,90,135,185});
    }

    DrawText("v1.7", sw-48, sh-22, 13, (Color){50,45,72,180});
}

// ─── DRAW OVERLAY (death / next-round) ────────────────────────────────────────
static void DrawOverlay(float t) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, (Color){0,0,0,175});

    if (gs == GS_DEAD) {
        float pp = 0.5f + 0.5f*sinf(sttime * 5.0f);
        const char *M = "YOU  DIED";
        int ms = 64, mw2 = MeasureText(M, ms);
        DrawText(M, sw/2-mw2/2+2, sh/2-56+2, ms, (Color){0,0,0,140});
        DrawText(M, sw/2-mw2/2,   sh/2-56,   ms,
                 (Color){230,(uchar)(38+(int)(28*pp)),38,255});
        const char *S = TextFormat("Round %d   Coins %d   Treasures %d   Kills %d",
                                   roundn, totcoi, tottrs, kills_game);
        DrawText(S, sw/2-MeasureText(S,22)/2, sh/2+22, 22,
                 (Color){185,180,205,215});
        DrawText("ENTER to restart   |   Q for menu",
                 sw/2-MeasureText("ENTER to restart   |   Q for menu",20)/2,
                 sh/2+62, 20, (Color){140,135,165,195});

    } else if (gs == GS_NEXT) {
        const char *M = wasportal ? "PORTAL  USED !" : "TREASURE  FOUND !";
        Color mc     = wasportal ? CPORT : CTRES;
        int ms = 52, mw2 = MeasureText(M, ms);
        DrawText(M, sw/2-mw2/2+2, sh/2-48+2, ms, (Color){0,0,0,140});
        DrawText(M, sw/2-mw2/2,   sh/2-48,   ms, mc);

        const char *C = TextFormat("Coins : %d   Kills : %d", totcoi, kills_game);
        DrawText(C, sw/2-MeasureText(C,28)/2, sh/2+22, 28, CCOIN);
        DrawText("ENTER for next round",
                 sw/2-MeasureText("ENTER for next round",20)/2,
                 sh/2+64, 20, (Color){140,135,165,195});
    }
}

// ─── MAIN ─────────────────────────────────────────────────────────────────────
int main(void) {
    SetRandomSeed((unsigned)time(NULL));
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(SW, SH, "Dungeon Crawler");
    SetExitKey(KEY_NULL); 
    SetTargetFPS(60);
    LoadStats();
    LoadShop();
    SetTargetFPS(60);

    gs = GS_MENU; roundn = 1; totcoi = 0; tottrs = 0;
    eneon = true; pinvn = 0;
    memset(pars, 0, sizeof(pars));

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        float t  = GetTime();

        if (IsKeyPressed(KEY_F11)) ToggleFullscreenNow();
        UpdateCameraOffset();

        switch (gs) {

        case GS_MENU: {
            if (IsKeyPressed(KEY_ESCAPE)) {
                if (menuSub != 0) { if (menuSub == 2) statScroll = 0; menuSub = 0; break; }
            }
            if (menuSub == 0) {
                if (IsKeyPressed(KEY_E)) eneon = !eneon;
                if (IsKeyPressed(KEY_ENTER)) {
                    roundn=1; totcoi=0; tottrs=0; kills_game=0; pinvn=0;
                    GenRound(); gs = GS_PLAY; global_games_played++;
                }
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                int sw2 = GetScreenWidth(), sh2 = GetScreenHeight();
                Vector2 mp = GetMousePosition();

                if (menuSub != 0) {
                    int margin2=32, pw2=sw2-margin2*2, ph2=sh2-margin2*2;
                    int px2=margin2, py2=margin2;
                    int bkW=130, bkH=40;
                    Rectangle backR={(float)(px2+20),(float)(py2+ph2-bkH-16),
                                     (float)bkW,(float)bkH};
                    if (CheckCollisionPointRec(mp, backR)) { if (menuSub == 2) statScroll = 0; menuSub=0; break; }
                    Rectangle panelR={(float)px2,(float)py2,(float)pw2,(float)ph2};
                    if (!CheckCollisionPointRec(mp, panelR)) { if (menuSub == 2) statScroll = 0; menuSub=0; break; }
                } else {
                    // Card buttons 
                    int M2 = 14;
                    int titleBottom2 = 152;
                    int cW2  = (int)(sw2 * 0.38f);
                    int midY2 = (titleBottom2 + sh2 - M2) / 2;
                    int cHtop2 = midY2 - titleBottom2 - M2/2;
                    int cHbot2 = sh2 - M2 - (midY2 + M2/2) - 1;
                    if (cHtop2 < 80) cHtop2 = 80;
                    if (cHbot2 < 80) cHbot2 = 80;
                    int topY2 = titleBottom2;
                    int botY2 = midY2 + M2/2;

                    int rightCardX2 = sw2 - M2 - cW2;
                    Rectangle bTL={(float)M2,           (float)topY2,(float)cW2,(float)cHtop2};
                    Rectangle bTR={(float)rightCardX2,  (float)topY2,(float)cW2,(float)cHtop2};
                    Rectangle bBL={(float)M2,           (float)botY2,(float)cW2,(float)cHbot2};
                    Rectangle bBR={(float)rightCardX2,  (float)botY2,(float)cW2,(float)cHbot2};

                    if (CheckCollisionPointRec(mp,bTL)) { menuSub=1; break; }
                    if (CheckCollisionPointRec(mp,bTR)) { menuSub=2; break; }
                    if (CheckCollisionPointRec(mp,bBL)) { menuSub=3; break; }
                    if (CheckCollisionPointRec(mp,bBR)) { menuSub=4; break; }

                    // Play button
                    int rightCardX3  = sw2 - M2 - cW2;
                    int centreColW2  = rightCardX3 - (M2 + cW2);
                    int pbW2 = centreColW2 - 16;
                    int pbH2 = 130;
                    int pbX2 = sw2/2 - pbW2/2;
                    int pbY2 = midY2 - pbH2/2 - 16;
                    Rectangle pbR2={(float)pbX2,(float)pbY2,(float)pbW2,(float)pbH2};
                    if (CheckCollisionPointRec(mp, pbR2)) {
                        roundn=1; totcoi=0; tottrs=0; kills_game=0; pinvn=0;
                        GenRound(); global_games_played++; gs=GS_PLAY;
                    }
                }
            }
            break;
        }

        case GS_PLAY: {
            Rectangle pauseBtn = GetPauseButtonRect();

            if (IsKeyPressed(KEY_ESCAPE)) { gs = GS_PAUSE; break; } 
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                PointInRect(GetMousePosition(), pauseBtn)) {
                gs = GS_PAUSE;
                break;
            }

            if (IsKeyPressed(KEY_ONE))   UseItm(0);
            if (IsKeyPressed(KEY_TWO))   UseItm(1);
            if (IsKeyPressed(KEY_THREE)) UseItm(2);

            if (pspd > 0) pspd -= dt;
            if (pwlk > 0) pwlk -= dt;  

            { float md = (pspd > 0) ? pmd * 0.5f : pmd;
              pmt += dt;
              if (pmt >= md) {
                  bool moved = false;
                  if      (IsKeyDown(KEY_W)||IsKeyDown(KEY_UP))
                      { TryMove(0,-1); moved=true; }
                  else if (IsKeyDown(KEY_S)||IsKeyDown(KEY_DOWN))
                      { TryMove(0,+1); moved=true; }
                  else if (IsKeyDown(KEY_A)||IsKeyDown(KEY_LEFT))
                      { TryMove(-1,0); moved=true; }
                  else if (IsKeyDown(KEY_D)||IsKeyDown(KEY_RIGHT))
                      { TryMove(+1,0); moved=true; }
                  if (moved) pmt = 0;
              }
            }

            if (gs != GS_PLAY) break;

            // ── Coin Magnet perk ──────────────────────────────────────────────
            if (perk_magnet) {
                for (int i = 0; i < MAX_COI; i++) {
                    if (!coins[i].on) continue;
                    int dx = coins[i].x - px;
                    int dy = coins[i].y - py;
                    if (dx < -1 || dx > 1 || dy < -1 || dy > 1) continue; 
                    coins[i].on = false;
                    map[coins[i].y][coins[i].x] = '.';
                    totcoi++; global_totcoi++; coin_balance++;
                    Burst(WX(coins[i].x), WY(coins[i].y), CCOIN, 8, 2);
                }
            }

            UpdEnes(dt);
            UpdPars(dt);

            cam.target.x += (WX(px) - cam.target.x) * dt * 9.0f;
            cam.target.y += (WY(py) - cam.target.y) * dt * 9.0f;
            break;
        }

        case GS_PAUSE: {
            Rectangle panel, resumeBtn, menuBtn;
            GetPauseMenuLayout(&panel, &resumeBtn, &menuBtn);

            if (IsKeyPressed(KEY_ESCAPE)) gs = GS_PLAY;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 mp = GetMousePosition();
                if (PointInRect(mp, resumeBtn)) gs = GS_PLAY;
                else if (PointInRect(mp, menuBtn)) { gs = GS_MENU; menuSub = 0; }
            }

            UpdPars(dt);
            break;
        }

        case GS_DEAD:
        case GS_NEXT:
            sttime += dt;
            UpdPars(dt);
            if (sttime > 0.45f) {
                if (gs == GS_DEAD) {
                    if (IsKeyPressed(KEY_ENTER))
                        { roundn=1;totcoi=0;tottrs=0;kills_game=0;pinvn=0; global_games_played++; GenRound(); gs=GS_PLAY; }
                    if (IsKeyPressed(KEY_Q)) { gs = GS_MENU; menuSub = 0; }
                } else {
                    if (IsKeyPressed(KEY_ENTER))
                        { roundn++; GenRound(); gs=GS_PLAY; }
                }
            }
            break;
        }

        BeginDrawing();
        ClearBackground(CB);

        if (gs == GS_MENU) {
            DrawMenu(t);
        } else {

            BeginMode2D(cam);

            for (int y=0; y<mh; y++)
            for (int x=0; x<mw; x++)
                DrawTile(x, y, t);

            if (eneon) {
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

            { int cx = px*CELL + CELL/2, cy = py*CELL + CELL/2;
              static const Color SKIN_COLS[10] = {
                  {  90,220,120,255 }, 
                  { 255, 80, 80,255 }, 
                  { 255,160, 40,255 }, 
                  { 255,220, 40,255 }, 
                  {  60,200, 60,255 }, 
                  {  40,200,220,255 }, 
                  {  60,120,255,255 },
                  { 160, 60,255,255 }, 
                  { 220, 60,200,255 },
                  { 255,255,255,255 },
              };
              Color pc = pdead ? CPD : SKIN_COLS[skin_equipped];

              if (pwlk > 0) {
                  float pp = 0.5f + 0.5f*sinf(t*6.0f);
                  DrawCircle(cx, cy, CELL/2+8, (Color){180,80,255,(int)(30+30*pp)});
                  DrawCircleLines(cx, cy, CELL/2+8, (Color){180,80,255,(int)(120+80*pp)});
              }
              if (pshield) {
                  // item shield – solid pulsing blue aura (original)
                  float pp = 0.5f + 0.5f*sinf(t*5.0f);
                  DrawCircle(cx, cy, CELL/2+6, (Color){80,150,255,(int)(38+28*pp)});
                  DrawCircleLines(cx, cy, CELL/2+6,(Color){80,150,255,(int)(155+65*pp)});
              }
              if (pshield_perk) {
                  // perk shield – smaller, striped transparent blue
                  float pp = 0.5f + 0.5f*sinf(t*3.5f);
                  int ar = CELL/2+3;  // smaller radius than item shield
                  DrawCircle(cx, cy, ar, (Color){40,100,220,(int)(18+14*pp)});
                  DrawCircleLines(cx, cy, ar, (Color){100,180,255,(int)(90+50*pp)});
                  // 4 diagonal stripe segments around the circle
                  for (int s = 0; s < 4; s++) {
                      float a0 = (float)s * 1.5708f + t*1.2f;
                      float a1 = a0 + 0.55f;
                      Vector2 v0 = { cx + cosf(a0)*(ar-2), cy + sinf(a0)*(ar-2) };
                      Vector2 v1 = { cx + cosf(a1)*(ar-2), cy + sinf(a1)*(ar-2) };
                      Vector2 v2 = { cx + cosf(a0)*(ar+2), cy + sinf(a0)*(ar+2) };
                      Vector2 v3 = { cx + cosf(a1)*(ar+2), cy + sinf(a1)*(ar+2) };
                      DrawTriangle(v0, v1, v2, (Color){120,200,255,(int)(60+40*pp)});
                      DrawTriangle(v1, v3, v2, (Color){120,200,255,(int)(60+40*pp)});
                  }
              }
              if (pspd > 0) {
                  DrawCircle(cx, cy, CELL/2+3, (Color){80,220,80,28});
              }
              DrawCircle(cx, cy, CELL/2-4, pc);
              DrawCircle(cx-2, cy-3, CELL/4,
                         (Color){CLAMP255(pc.r+55),CLAMP255(pc.g+55),CLAMP255(pc.b+55),120});
              DrawCircle(cx-4, cy-2, 2, (Color){10,10,22,255});
              DrawCircle(cx+4, cy-2, 2, (Color){10,10,22,255});
            }

            DrawPars();
            EndMode2D();

            DrawHUD(t);
            
            if (gs == GS_PAUSE) DrawPauseMenu();
            if (gs == GS_DEAD || gs == GS_NEXT) DrawOverlay(t);
        }

        EndDrawing();
    }

    UpdateAndSaveStats();
    CloseWindow();
    return 0;
}