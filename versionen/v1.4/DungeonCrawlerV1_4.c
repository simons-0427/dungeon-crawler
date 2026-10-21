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
static float  pshield_dur;   // remaining seconds; >=1e8 = unlimited
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
#define SKIN_COUNT 30
static int  skin_owned[SKIN_COUNT];
static int  skin_equipped = 0;
static int  spd_level = 0; 
static int  shd_level = 0; 
static int  wlk_level = 0;  
static int  swd_level = 0;  
static int  bmb_level = 0;   
static int  perk_slots = 0;
static int  perk_magnet = 0;
static int  perk_shield2 = 0;

//  ─── Advancement-Requirements ───────────────────────────────────────────────────────────────
const int TIERS_HS_KILLS[4]   = { 5, 10, 20, 40 };
const int TIERS_HS_COI[4]     = { 50, 100, 250, 500 };
const int TIERS_HS_TRS[4]     = { 2, 5, 10, 20 };
const int TIERS_TOTKILLS[4]   = { 100, 500, 2000, 5000 };
const int TIERS_GAMES[4]      = { 10, 50, 150, 300 };
const int TIERS_ROUNDS[4]     = { 25, 125, 400, 1000 };

static void LoadShop(void) {
    FILE *f = fopen(SHOP_FILE, "r");
    if (f) {
        fscanf(f, "%d %d %d %d %d %d %d %d %d %d",
               &skin_equipped,
               &spd_level, &shd_level, &wlk_level, &swd_level, &bmb_level,
               &perk_slots, &perk_magnet, &perk_shield2,
               skin_owned+0);
        for (int i=1;i<SKIN_COUNT;i++) fscanf(f, "%d", &skin_owned[i]);
        fclose(f);
    }
    skin_owned[0] = 1;
}

static void SaveShop(void) {
    FILE *f = fopen(SHOP_FILE, "w");
    if (f) {
        fprintf(f, "%d %d %d %d %d %d %d %d %d",
                skin_equipped,
                spd_level, shd_level, wlk_level, swd_level, bmb_level,
                perk_slots, perk_magnet, perk_shield2);
        for (int i=0;i<SKIN_COUNT;i++) fprintf(f, " %d", skin_owned[i]);
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
void DrawCustomStar(float cx, float cy, float outerRadius, float innerRadius, Color color) {
    Vector2 points[11];
    for (int i = 0; i < 10; i++) {
        float angle = -3.14159265f / 2.0f + i * (3.14159265f / 5.0f);
        float r = (i % 2 == 0) ? outerRadius : innerRadius;
        points[i] = (Vector2){ cx + cosf(angle) * r, cy + sinf(angle) * r };
    }
    points[10] = points[0];
    
    for (int i = 0; i < 10; i++) {
        DrawTriangle((Vector2){cx, cy}, points[i + 1], points[i], color);
    }
}

void DrawCustomStarLines(float cx, float cy, float outerRadius, float innerRadius, Color color) {
    Vector2 points[10];
    for (int i = 0; i < 10; i++) {
        float angle = -3.14159265f / 2.0f + i * (3.14159265f / 5.0f);
        float r = (i % 2 == 0) ? outerRadius : innerRadius;
        points[i] = (Vector2){ cx + cosf(angle) * r, cy + sinf(angle) * r };
    }
    for (int i = 0; i < 10; i++) {
        DrawLineV(points[i], points[(i + 1) % 10], color);
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
    pshield = false; pshield_dur = 0;                 
    if (roundn == 1) pshield_perk = (perk_shield2 > 0);  
    sttime = 0;
    memset(pars, 0, sizeof(pars));
}

// ─── USE ITEM ─────────────────────────────────────────────────────────────────
static const float SPD_DUR[] = { 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
static const float SHD_DUR[] = { 5.0f, 10.0f, 20.0f, 30.0f, 45.0f, -1.0f };
static const float WLK_DUR[] = { 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f };
static const int   SWD_RNG[] = { 1, 2, 3, 4 };
static const int   BMB_RNG[] = { 1, 2, 3, 4, 5, 6 };

static const int SPD_COST[] = {  200,  250,  350, 500, 600 };
static const int SHD_COST[] = {  250,  400,  600, 750, 1000};
static const int WLK_COST[] = {  500,  750,  1000, 1250, 1500};   
static const int SWD_COST[] = {  500,  1000, 1500 };         
static const int BMB_COST[] = {  250,  500, 750, 1000, 1250};

static void UseItm(int slot) {
    if (slot >= pinvn || slot >= INV_SZ + 2) return;
    IKind k = pinv[slot];
    switch (k) {
        case IK_SPD:
            pspd = SPD_DUR[spd_level];
            Burst(WX(px), WY(py), IK_COL[IK_SPD], 16, 3);
            break;

        case IK_SHD: {
            float dur = SHD_DUR[shd_level];
            pshield_dur = (dur < 0) ? 1e9f : dur;
            pshield = true;
            Burst(WX(px), WY(py), CSHD, 18, 3);
            break;
        }

        case IK_BMB: {
            int r = BMB_RNG[bmb_level];
            for (int dy=-r; dy<=r; dy++)
            for (int dx=-r; dx<=r; dx++) {
                int nx=px+dx, ny=py+dy;
                if (nx>=0&&ny>=0&&nx<mw&&ny<mh && map[ny][nx]=='+') {
                    map[ny][nx]='.';
                    Burst(WX(nx), WY(ny), IK_COL[IK_BMB], 8, 3);
                }
            }
            Burst(WX(px), WY(py), IK_COL[IK_BMB], 12, 4);
            break;
        }

        case IK_WLK:
            pwlk = WLK_DUR[wlk_level];
            Burst(WX(px), WY(py), CWLK, 20, 3);
            break;

        case IK_SWD: {
            int rng = SWD_RNG[swd_level];
            if (eneon) {
                for (int i = 0; i < MAX_ENE; i++) {
                    Ene *e = &enes[i]; if (!e->on) continue;
                    int ex = (int)(e->x+0.5f), ey = (int)(e->y+0.5f);
                    if (abs(ex-px) <= rng && abs(ey-py) <= rng) {
                        e->on = false;
                        AddEnemyKill();
                        Burst(WX(ex), WY(ey), CENE, 20, 4);
                        Burst(WX(ex), WY(ey), CSWD, 12, 5);
                    }
                }
            }
            Burst(WX(px), WY(py), CSWD, 28, 5);
            break;
        }

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
    (void)t;
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
    DrawText(TextFormat("Balance: %d", coin_balance), 338, 56, 13, (Color){255,215,0,200});

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
        if (pshield_dur >= 1e8f)
            DrawText("SHIELD", efx, efy, 18, (Color){80,150,255,255});
        else
            DrawText(TextFormat("SHIELD  %.1fs", pshield_dur), efx, efy, 18, (Color){80,150,255,255});
        efx += 165;
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
static int upgScroll  = 0;
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

static Color GetSkinColor(int id) {
    float gt = GetTime();
    switch (id) {
        case 22: { // Pyro — intense Red->Orange->Yellow
            float p = 0.5f + 0.5f*sinf(gt * 3.5f);
            return (Color){255, (uchar)(10+(int)(200*p*p)), (uchar)(30*(p*p*p)), 255};
        }
        case 23: { // Phantom — Violet->Cyan->White, ghostly pulsing alpha 40-130
            float p  = 0.5f + 0.5f*sinf(gt * 1.8f);
            float p2 = 0.5f + 0.5f*sinf(gt * 3.0f);
            return (Color){(uchar)(120+(int)(135*p)),
                           (uchar)( 60+(int)(195*p)),
                           255,
                           (uchar)( 40+(int)( 90*p2))};
        }
        case 24: { // Cyber — smooth dark Cyan/Pink with more black
            float phase = sinf(gt * 1.5f);
            float pos   = phase > 0.0f ? powf(phase,  1.8f) : 0.0f;
            float neg   = phase < 0.0f ? powf(-phase, 1.8f) : 0.0f;
            return (Color){(uchar)(neg*210 + 6),
                           (uchar)(pos*150 + 5),
                           (uchar)(pos*250*0.6f + neg*90*0.5f + 8),
                           255};
        }
        case 25: { // Chroma — full rainbow loop
            return ColorFromHSV(fmodf(gt * 60.0f, 360.0f), 1.0f, 1.0f);
        }
        case 26: { // Biohazard — intense pulsing toxic green
            float p = 0.5f + 0.5f*sinf(gt * 3.5f);
            float pp = p * p;
            return (Color){(uchar)(5+(int)(25*pp)),
                           (uchar)(140+(int)(115*p)),
                           (uchar)(0+(int)(18*pp)), 255};
        }
        case 27: { // Alien — bright emoji green
            return (Color){138, 200, 66, 255};
        }
        case 28: { // Skull — cream white
            return (Color){228, 222, 205, 255};
        }
        case 29: { // Angel — pulses warm gold <-> pure white
            float p = 0.5f + 0.5f*sinf(gt * 1.8f);
            return (Color){255,
                           (uchar)(240+(int)(15*p)),
                           (uchar)(160+(int)(95*p)), 255};
        }
    }
    static const Color SKIN_TABLE[22] = {
        // Common (0-13)
        {  90,220,120,255 }, { 255, 80, 80,255 }, { 255,160, 40,255 },
        { 255,220, 40,255 }, {  60,200, 60,255 }, {  40,200,220,255 },
        {  60,120,255,255 }, { 160, 60,255,255 }, { 220, 60,200,255 },
        { 255,255,255,255 },
        { 150, 90, 40,255 }, {  30, 28, 35,255 }, { 255,120,195,255 },
        { 155,155,162,255 },
        // Uncommon (14-21) — intensified to match their names (Void unchanged)
        {  22,170, 42,255 }, { 235, 12, 55,255 }, { 255,118,  0,255 },
        { 255,190, 15,255 }, {  10,180,135,255 }, { 110,205,245,255 },
        {  16, 78,245,255 }, {  10,  5, 20,255 },
    };
    if (id >= 0 && id < 22) return SKIN_TABLE[id];
    return (Color){200,200,200,255};
}

// ─── EPIC SKIN FACES (shared by in-game player + shop preview) ────────────────
// Filled rotated ellipse (used for slanted alien eyes / skull sockets).
static void DrawEllipseRotF(float cx, float cy, float rx, float ry, float ang, Color col) {
    const int SEG = 24;
    Vector2 pts[SEG + 1];
    float ca = cosf(ang), sa = sinf(ang);
    for (int i = 0; i <= SEG; i++) {
        float th = (float)i / SEG * 2.0f * PI;
        float ex = cosf(th) * rx, ey = sinf(th) * ry;
        pts[i] = (Vector2){ cx + ex*ca - ey*sa, cy + ex*sa + ey*ca };
    }
    for (int i = 0; i < SEG; i++)
        DrawTriangle((Vector2){cx, cy}, pts[i + 1], pts[i], col);
}

// Thick elliptical arc (used for the perspective halo ring) drawn as a band of
// short segments between angle a0..a1.
static void DrawHaloArc(float cx, float cy, float rx, float ry,
                        float a0, float a1, float thick, Color col) {
    const int SEG = 24;
    Vector2 prev = {0};
    for (int i = 0; i <= SEG; i++) {
        float th = a0 + (a1 - a0) * ((float)i / SEG);
        Vector2 p = { cx + cosf(th)*rx, cy + sinf(th)*ry };
        if (i > 0) DrawLineEx(prev, p, thick, col);
        prev = p;
    }
}

// Halo ellipse geometry shared by the back (behind head) and front (over brow)
// passes so the two arcs line up into one continuous ring.
static void HaloGeom(float cx, float cy, float R, float t,
                     float *hx, float *hy, float *rx, float *ry) {
    *hx = cx;
    *hy = cy - R*0.95f + sinf(t * 2.2f) * (R * 0.14f);
    *rx = R * 1.06f;
    *ry = R * 0.40f;
}

// Epic-skin elements that belong BEHIND the body circle (drawn before it):
// the angel's wings, outer glow and the rear half of the halo ring.
static void DrawEpicBack(int id, float cx, float cy, float R, float t) {
    if (id == 29) {
        DrawCircleV((Vector2){cx, cy}, R*1.35f, (Color){255,200,50,16});

        float wf = sinf(t * 3.0f) * (R * 0.25f);
        Color wo = (Color){255,245,210, 90};   // outer (translucent)
        Color wi = (Color){255,250,230,210};   // inner (brighter)
        // left wing (both windings against culling)
        DrawTriangle((Vector2){cx-R*0.50f,cy-R*0.20f},(Vector2){cx-R*1.45f,cy-R*0.50f-wf},(Vector2){cx-R*1.25f,cy+R*0.45f+wf}, wo);
        DrawTriangle((Vector2){cx-R*0.50f,cy-R*0.20f},(Vector2){cx-R*1.25f,cy+R*0.45f+wf},(Vector2){cx-R*1.45f,cy-R*0.50f-wf}, wo);
        DrawTriangle((Vector2){cx-R*0.50f,cy-R*0.10f},(Vector2){cx-R*1.10f,cy-R*0.25f-wf},(Vector2){cx-R*1.00f,cy+R*0.35f+wf}, wi);
        DrawTriangle((Vector2){cx-R*0.50f,cy-R*0.10f},(Vector2){cx-R*1.00f,cy+R*0.35f+wf},(Vector2){cx-R*1.10f,cy-R*0.25f-wf}, wi);
        // right wing
        DrawTriangle((Vector2){cx+R*0.50f,cy-R*0.20f},(Vector2){cx+R*1.45f,cy-R*0.50f-wf},(Vector2){cx+R*1.25f,cy+R*0.45f+wf}, wo);
        DrawTriangle((Vector2){cx+R*0.50f,cy-R*0.20f},(Vector2){cx+R*1.25f,cy+R*0.45f+wf},(Vector2){cx+R*1.45f,cy-R*0.50f-wf}, wo);
        DrawTriangle((Vector2){cx+R*0.50f,cy-R*0.10f},(Vector2){cx+R*1.10f,cy-R*0.25f-wf},(Vector2){cx+R*1.00f,cy+R*0.35f+wf}, wi);
        DrawTriangle((Vector2){cx+R*0.50f,cy-R*0.10f},(Vector2){cx+R*1.00f,cy+R*0.35f+wf},(Vector2){cx+R*1.10f,cy-R*0.25f-wf}, wi);

        // rear half of the halo (upper arc, θ 180°..360°) — head will be drawn
        // over it so the part directly behind the skull is hidden.
        float hx, hy, rx, ry;  HaloGeom(cx, cy, R, t, &hx, &hy, &rx, &ry);
        DrawHaloArc(hx, hy, rx, ry, PI, 2.0f*PI, R*0.22f, (Color){255,200,50,70});
        DrawHaloArc(hx, hy, rx, ry, PI, 2.0f*PI, R*0.12f, (Color){255,214,72,235});
    }
}

// ─── SKULL SKIN (id 28) ───────────────────────────────────────────────────────
// Fills the skull silhouette (cranium + cheekbones + tapering jaw + chin) from a
// hand-tuned half-contour mirrored about the vertical axis.  Drawn as a triangle
// fan from the centre (both windings, so back-face culling never drops a slice).
static void DrawSkullShape(float cx, float cy, float R, Color col) {
    // half outline (x>=0), top → chin, in units of R
    static const float CONT[][2] = {
        { 0.00f, -1.05f }, { 0.42f, -0.99f }, { 0.72f, -0.82f },
        { 0.91f, -0.55f }, { 1.00f, -0.22f }, { 1.01f,  0.10f },  // temple / cheekbone
        { 0.90f,  0.34f }, { 0.72f,  0.52f },                     // cheek taper
        { 0.64f,  0.72f }, { 0.60f,  0.95f }, { 0.50f,  1.12f },
        { 0.30f,  1.21f }, { 0.00f,  1.23f },                     // chin
    };
    int n = (int)(sizeof(CONT) / sizeof(CONT[0]));
    // closed control polygon: right side top→chin, then mirrored left side
    Vector2 ctrl[40];
    int m = 0;
    for (int i = 0;   i < n;  i++) ctrl[m++] = (Vector2){ cx + CONT[i][0]*R, cy + CONT[i][1]*R };
    for (int i = n-2; i >= 1; i--) ctrl[m++] = (Vector2){ cx - CONT[i][0]*R, cy + CONT[i][1]*R };

    // Catmull-Rom smoothing → glatte geschlossene Silhouette, gefüllt als Fan
    // (jedes Dreieck in beiden Windungen, damit Culling nie eine Scheibe verwirft)
    Vector2 c = { cx, cy };
    Vector2 first = {0}, prev = {0};
    int started = 0;
    const int SUB = 8;
    for (int i = 0; i < m; i++) {
        Vector2 p0 = ctrl[(i - 1 + m) % m], p1 = ctrl[i];
        Vector2 p2 = ctrl[(i + 1) % m],     p3 = ctrl[(i + 2) % m];
        for (int s = 0; s < SUB; s++) {
            float u = (float)s / SUB, u2 = u*u, u3 = u2*u;
            Vector2 q = {
                0.5f*((2*p1.x) + (-p0.x+p2.x)*u + (2*p0.x-5*p1.x+4*p2.x-p3.x)*u2 + (-p0.x+3*p1.x-3*p2.x+p3.x)*u3),
                0.5f*((2*p1.y) + (-p0.y+p2.y)*u + (2*p0.y-5*p1.y+4*p2.y-p3.y)*u2 + (-p0.y+3*p1.y-3*p2.y+p3.y)*u3)
            };
            if (started) { DrawTriangle(c, q, prev, col); DrawTriangle(c, prev, q, col); }
            else { first = q; started = 1; }
            prev = q;
        }
    }
    DrawTriangle(c, first, prev, col);
    DrawTriangle(c, prev, first, col);
}

// Full skull skin: silhouette + soft shading, big angled eye sockets, a heart
// shaped nasal cavity and a real tooth row.  Replaces the plain body circle.
static void DrawSkull(float cx, float cy, float R, float t) {
    (void)t;
    Color bone   = (Color){231, 226, 212, 255};
    Color boneLo = (Color){193, 187, 172, 255};   // shadow tone
    Color boneHi = (Color){247, 244, 235, 255};   // highlight
    Color socket = (Color){ 13, 11, 13, 255 };
    Color dark   = (Color){ 24, 20, 21, 255 };

    // 1) silhouette: shadow rim slightly offset down, then the bright skull on top
    DrawSkullShape(cx, cy + R*0.03f, R*1.03f, boneLo);
    DrawSkullShape(cx, cy, R, bone);

    // 2) soft top sheen (layered translucent ellipses → pseudo gradient)
    for (int i = 0; i < 4; i++) {
        float k = 1.0f - i*0.18f;
        DrawEllipseRotF(cx - R*0.10f, cy - R*0.46f, R*0.58f*k, R*0.44f*k, 0.0f,
                        (Color){boneHi.r, boneHi.g, boneHi.b, 42});
    }
    // 3) faint shadow under the cheekbones / above the jaw
    DrawEllipseRotF(cx, cy + R*0.58f, R*0.52f, R*0.26f, 0.0f, (Color){boneLo.r,boneLo.g,boneLo.b,110});

    // 4) eye sockets — large ovals angled outward-up (mirrored), soft recessed rim
    DrawEllipseRotF(cx - R*0.44f, cy - R*0.07f, R*0.48f, R*0.42f,  0.32f, (Color){150,144,132,120});
    DrawEllipseRotF(cx + R*0.44f, cy - R*0.07f, R*0.48f, R*0.42f, -0.32f, (Color){150,144,132,120});
    DrawEllipseRotF(cx - R*0.44f, cy - R*0.08f, R*0.42f, R*0.36f,  0.32f, socket);
    DrawEllipseRotF(cx + R*0.44f, cy - R*0.08f, R*0.42f, R*0.36f, -0.32f, socket);
    // tiny depth glints low in the sockets
    DrawCircleV((Vector2){cx - R*0.34f, cy + R*0.10f}, R*0.05f, (Color){55,51,53,150});
    DrawCircleV((Vector2){cx + R*0.34f, cy + R*0.10f}, R*0.05f, (Color){55,51,53,150});

    // 5) nasal cavity — inverted-heart (triangle + two lobes)
    DrawTriangle((Vector2){cx,            cy + R*0.44f},
                 (Vector2){cx - R*0.15f,  cy + R*0.20f},
                 (Vector2){cx + R*0.15f,  cy + R*0.20f}, dark);
    DrawTriangle((Vector2){cx,            cy + R*0.44f},
                 (Vector2){cx + R*0.15f,  cy + R*0.20f},
                 (Vector2){cx - R*0.15f,  cy + R*0.20f}, dark);
    DrawCircleV((Vector2){cx - R*0.08f, cy + R*0.22f}, R*0.075f, dark);
    DrawCircleV((Vector2){cx + R*0.08f, cy + R*0.22f}, R*0.075f, dark);

    // 6) teeth — dark mouth backdrop with bright teeth + thin gaps
    float mx = cx - R*0.46f, mouthW = R*0.92f;
    float myT = cy + R*0.62f, myB = cy + R*1.00f;
    DrawRectangleRounded((Rectangle){mx, myT, mouthW, myB - myT}, 0.35f, 6, dark);
    int nt = 6;
    float tw = mouthW / nt;
    for (int k = 0; k < nt; k++) {
        float tx = mx + k*tw;
        DrawRectangleRounded(
            (Rectangle){tx + R*0.03f, myT + R*0.03f, tw - R*0.06f, (myB - myT) - R*0.06f},
            0.30f, 4, boneHi);
    }
}

// Draws the facial details for epic skins (27-29) on top of a body circle of
// radius R centred at (cx,cy).  All offsets scale with R so the same code works
// for the in-game player (R≈12) and the larger shop preview (R≈18).
static void DrawEpicFace(int id, float cx, float cy, float R, float t) {
    if (id == 27) {
        // ── Alien (emoji): light-green head, big slanted black eyes,
        //    two antennae with bulbs, small smile ──
        float ab = sinf(t * 2.0f) * (R * 0.16f);     // antenna bob
        Color stalk = (Color){ 78, 150, 44, 240 };
        Color bulb  = (Color){ 150, 220, 80, 255 };
        DrawLineEx((Vector2){cx - R*0.34f, cy - R*0.72f},
                   (Vector2){cx - R*0.64f, cy - R*1.16f - ab}, R*0.11f, stalk);
        DrawCircleV((Vector2){cx - R*0.64f, cy - R*1.16f - ab}, R*0.18f, bulb);
        DrawLineEx((Vector2){cx + R*0.34f, cy - R*0.72f},
                   (Vector2){cx + R*0.64f, cy - R*1.16f + ab}, R*0.11f, stalk);
        DrawCircleV((Vector2){cx + R*0.64f, cy - R*1.16f + ab}, R*0.18f, bulb);

        // soft top-left sheen
        DrawCircleV((Vector2){cx - R*0.30f, cy - R*0.40f}, R*0.30f, (Color){185,235,125,90});

        // big slanted almond eyes (black), mirrored
        DrawEllipseRotF(cx - R*0.40f, cy - R*0.02f, R*0.42f, R*0.25f,  0.52f, (Color){10,14,10,255});
        DrawEllipseRotF(cx + R*0.40f, cy - R*0.02f, R*0.42f, R*0.25f, -0.52f, (Color){10,14,10,255});
        // subtle glints
        DrawCircleV((Vector2){cx - R*0.52f, cy - R*0.12f}, R*0.07f, (Color){255,255,255,190});
        DrawCircleV((Vector2){cx + R*0.30f, cy - R*0.12f}, R*0.07f, (Color){255,255,255,190});

        // small smile
        Color mouth = (Color){ 42, 92, 28, 235 };
        float mcx = cx, mcy = cy + R*0.30f, mr = R*0.18f;
        Vector2 prev = {0};
        for (int s = 0; s <= 8; s++) {
            float th = PI/3.0f + (PI/3.0f) * (s / 8.0f);   // 60°..120° (smile arc)
            Vector2 p = { mcx + cosf(th)*mr, mcy + sinf(th)*mr };
            if (s > 0) DrawLineEx(prev, p, R*0.09f, mouth);
            prev = p;
        }
    }
    else if (id == 29) {
        // ── Angel: golden eyes + front half of the halo ring ──
        // (wings, glow and the rear halo arc are drawn in DrawEpicBack)
        DrawCircleV((Vector2){cx-R*0.33f, cy-R*0.06f}, R*0.16f, (Color){255,210,80,255});
        DrawCircleV((Vector2){cx+R*0.33f, cy-R*0.06f}, R*0.16f, (Color){255,210,80,255});

        // front half of the halo (lower arc, θ 0°..180°) — sits over the brow,
        // above the eyes so it never covers them.
        float hx, hy, rx, ry;  HaloGeom(cx, cy, R, t, &hx, &hy, &rx, &ry);
        DrawHaloArc(hx, hy, rx, ry, 0.0f, PI, R*0.22f, (Color){255,200,50,70});
        DrawHaloArc(hx, hy, rx, ry, 0.0f, PI, R*0.12f, (Color){255,214,72,235});
    }
}

// ─── RARE SKIN FACES (shop preview) ──────────────────────────────────────────
// Behind-body effects for rare skins (22-26): auras, flames, tails, halos.
static void DrawRareBack(int id, float cx, float cy, float R, float t) {
    switch (id) {
        case 22: { // Pyro — pulsing ember aura + rising flame tongues
            float p = 0.5f + 0.5f*sinf(t*6.0f);
            DrawCircleV((Vector2){cx, cy}, R*(1.32f+0.10f*p), (Color){255,70,0,30});
            DrawCircleV((Vector2){cx, cy}, R*(1.12f+0.08f*p), (Color){255,150,0,42});
            for (int f=-1; f<=1; f++) {
                float fl = sinf(t*7.0f + f*1.7f);
                float bx = cx + f*R*0.55f, by = cy - R*0.70f;
                float tipx = bx + fl*R*0.18f;
                float tipy = by - R*(0.85f + 0.32f*(0.5f+0.5f*fl));
                DrawTriangle((Vector2){bx-R*0.30f, by},(Vector2){tipx, tipy},(Vector2){bx+R*0.30f, by},(Color){255,90,0,180});
                DrawTriangle((Vector2){bx-R*0.30f, by},(Vector2){bx+R*0.30f, by},(Vector2){tipx, tipy},(Color){255,90,0,180});
                DrawTriangle((Vector2){bx-R*0.16f, by},(Vector2){tipx, tipy+R*0.20f},(Vector2){bx+R*0.16f, by},(Color){255,215,70,225});
                DrawTriangle((Vector2){bx-R*0.16f, by},(Vector2){bx+R*0.16f, by},(Vector2){tipx, tipy+R*0.20f},(Color){255,215,70,225});
            }
            break;
        }
        case 23: { // Phantom — classic floating sheet-ghost silhouette + aura
            DrawCircleV((Vector2){cx, cy}, R*1.34f, (Color){150,90,255,26});
            Color gb = (Color){206,190,255,175};
            // full lavender silhouette (head + lower body) so the whole ghost
            // reads uniformly; the translucent body circle only tints it.
            DrawCircleV((Vector2){cx, cy}, R, gb);
            DrawRectangleRounded((Rectangle){cx-R*0.96f, cy-R*0.05f, R*1.92f, R*1.0f}, 0.45f, 8, gb);
            // three softly bobbing scalloped bumps along the hem
            for (int k=-1;k<=1;k++) {
                float bob = sinf(t*3.0f + k*1.1f) * R*0.10f;
                DrawCircleV((Vector2){cx + k*R*0.63f, cy + R*0.92f + bob}, R*0.34f, gb);
            }
            break;
        }
        case 24: { // Cyber — neon scan rings + back glow
            float p = 0.5f + 0.5f*sinf(t*4.0f);
            DrawCircleV((Vector2){cx, cy}, R*1.30f, (Color){0,220,255,26});
            DrawCircleLines((int)cx,(int)cy,(int)(R*(1.15f+0.12f*p)),(Color){0,240,255,120});
            DrawCircleLines((int)cx,(int)cy,(int)(R*1.34f),(Color){255,40,160,70});
            break;
        }
        case 25: { // Chroma — soft white aura + orbiting prismatic sparkles
            DrawCircleV((Vector2){cx, cy}, R*1.30f, (Color){255,255,255,22});
            for (int k=0;k<5;k++){
                float ang = t*0.8f + k*(PI*2.0f/5.0f);
                float sxp = cx + cosf(ang)*R*1.24f;
                float syp = cy + sinf(ang)*R*1.24f;
                float tw  = 0.5f+0.5f*sinf(t*4.0f + k*1.3f);
                Color c   = ColorFromHSV(fmodf(k*72.0f + t*120.0f,360.0f),1.0f,1.0f);
                float sr  = R*(0.15f + 0.10f*tw);
                Color cc  = (Color){c.r,c.g,c.b,(uchar)(110+120*tw)};
                DrawLineEx((Vector2){sxp-sr,syp},(Vector2){sxp+sr,syp}, R*0.05f, cc);
                DrawLineEx((Vector2){sxp,syp-sr},(Vector2){sxp,syp+sr}, R*0.05f, cc);
            }
            break;
        }
        case 26: { // Biohazard — toxic pulsing aura + bubbling drips
            float p = 0.5f + 0.5f*sinf(t*5.0f);
            DrawCircleV((Vector2){cx, cy}, R*(1.26f+0.10f*p), (Color){80,255,0,30});
            for (int d=-1; d<=1; d++) {
                float dy = fmodf(t*3.0f + d*2.1f, 2.0f);
                float bx = cx + d*R*0.5f;
                float by = cy + R*0.9f + dy*R*0.5f;
                DrawCircleV((Vector2){bx, by}, R*0.12f*(1.0f-dy*0.4f),
                            (Color){120,255,30,(uchar)(160*(1.0f-dy*0.5f))});
            }
            break;
        }
    }
}

// Front facial details for rare skins (22-26), drawn over the body circle.
static void DrawRareFace(int id, float cx, float cy, float R, float t) {
    switch (id) {
        case 22: { // Pyro — fierce angry eyes + furrowed molten brows
            DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.50f}, R*0.30f, (Color){255,230,120,90});
            DrawEllipseRotF(cx-R*0.34f, cy-R*0.02f, R*0.26f, R*0.16f, -0.40f, (Color){35,10,0,255});
            DrawEllipseRotF(cx+R*0.34f, cy-R*0.02f, R*0.26f, R*0.16f,  0.40f, (Color){35,10,0,255});
            DrawCircleV((Vector2){cx-R*0.34f, cy}, R*0.07f, (Color){255,240,180,255});
            DrawCircleV((Vector2){cx+R*0.34f, cy}, R*0.07f, (Color){255,240,180,255});
            DrawLineEx((Vector2){cx-R*0.56f, cy-R*0.32f},(Vector2){cx-R*0.14f, cy-R*0.16f}, R*0.11f, (Color){70,18,0,255});
            DrawLineEx((Vector2){cx+R*0.56f, cy-R*0.32f},(Vector2){cx+R*0.14f, cy-R*0.16f}, R*0.11f, (Color){70,18,0,255});
            break;
        }
        case 23: { // Phantom — big oval ghost eyes + round wailing mouth
            float p = 0.5f+0.5f*sinf(t*2.5f);
            DrawEllipseRotF(cx-R*0.32f, cy-R*0.10f, R*0.16f, R*0.23f, 0.0f, (Color){48,16,92,255});
            DrawEllipseRotF(cx+R*0.32f, cy-R*0.10f, R*0.16f, R*0.23f, 0.0f, (Color){48,16,92,255});
            DrawCircleV((Vector2){cx-R*0.29f, cy-R*0.17f}, R*0.05f, (Color){240,230,255,(uchar)(170+70*p)});
            DrawCircleV((Vector2){cx+R*0.35f, cy-R*0.17f}, R*0.05f, (Color){240,230,255,(uchar)(170+70*p)});
            DrawEllipseRotF(cx, cy+R*0.42f, R*0.12f, R*0.18f, 0.0f, (Color){48,16,92,255});
            break;
        }
        case 24: { // Cyber — glowing visor band + sliding scan dot + antenna
            float p = 0.5f+0.5f*sinf(t*5.0f);
            DrawRectangleRounded((Rectangle){cx-R*0.62f, cy-R*0.20f, R*1.24f, R*0.42f}, 0.6f, 6, (Color){8,18,28,255});
            DrawRectangleRounded((Rectangle){cx-R*0.58f, cy-R*0.11f, R*1.16f, R*0.17f}, 0.9f, 6, (Color){0,240,255,(uchar)(180+60*p)});
            float sx = cx - R*0.50f + (0.5f+0.5f*sinf(t*4.0f))*R*1.0f;
            DrawCircleV((Vector2){sx, cy-R*0.02f}, R*0.07f, (Color){255,255,255,255});
            DrawLineEx((Vector2){cx+R*0.66f, cy-R*0.52f},(Vector2){cx+R*0.95f, cy-R*0.98f}, R*0.08f, (Color){255,40,160,255});
            DrawCircleV((Vector2){cx+R*0.95f, cy-R*0.98f}, R*0.10f, (Color){255,60,180,255});
            break;
        }
        case 25: { // Chroma — glossy eyes + big twinkling star-shine
            DrawCircleV((Vector2){cx-R*0.30f, cy-R*0.05f}, R*0.15f, (Color){255,255,255,245});
            DrawCircleV((Vector2){cx+R*0.30f, cy-R*0.05f}, R*0.15f, (Color){255,255,255,245});
            DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.03f}, R*0.07f, (Color){30,30,55,255});
            DrawCircleV((Vector2){cx+R*0.32f, cy-R*0.03f}, R*0.07f, (Color){30,30,55,255});
            DrawCircleV((Vector2){cx-R*0.31f, cy-R*0.07f}, R*0.03f, (Color){255,255,255,255});
            DrawCircleV((Vector2){cx+R*0.29f, cy-R*0.07f}, R*0.03f, (Color){255,255,255,255});
            float p  = 0.5f+0.5f*sinf(t*4.5f);
            Color sh = (Color){255,255,255,(uchar)(160+90*p)};
            float sr = R*(0.24f+0.10f*p);
            float scx = cx+R*0.42f, scy = cy-R*0.44f;
            DrawLineEx((Vector2){scx-sr,scy},(Vector2){scx+sr,scy}, R*0.05f, sh);
            DrawLineEx((Vector2){scx,scy-sr},(Vector2){scx,scy+sr}, R*0.05f, sh);
            DrawLineEx((Vector2){scx-sr*0.6f,scy-sr*0.6f},(Vector2){scx+sr*0.6f,scy+sr*0.6f}, R*0.035f, sh);
            DrawLineEx((Vector2){scx-sr*0.6f,scy+sr*0.6f},(Vector2){scx+sr*0.6f,scy-sr*0.6f}, R*0.035f, sh);
            break;
        }
        case 26: { // Biohazard — glowing trefoil symbol
            Color sym  = (Color){20,42,0,255};
            Color glow = (Color){185,255,45,255};
            for (int k=0;k<3;k++){
                float ang = -PI/2.0f + k*(PI*2.0f/3.0f);
                float bx = cx + cosf(ang)*R*0.42f;
                float by = cy + sinf(ang)*R*0.42f + R*0.05f;
                DrawCircleV((Vector2){bx,by}, R*0.27f, sym);
                DrawCircleV((Vector2){bx,by}, R*0.15f, glow);
            }
            DrawCircleV((Vector2){cx, cy+R*0.05f}, R*0.17f, sym);
            DrawCircleV((Vector2){cx, cy+R*0.05f}, R*0.08f, (Color){10,22,0,255});
            break;
        }
    }
}

static void GetMenuIconRects(Rectangle *settingsR, Rectangle *futureR) {
    if (settingsR) *settingsR = (Rectangle){ 10.0f, 16.0f, 76.0f, 76.0f };
    if (futureR)   *futureR   = (Rectangle){ 96.0f, 16.0f, 76.0f, 76.0f };
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

        // ── Controls table ────────────────────────────────────────────────────
        {
            int ctrlX2 = col1x;
            int ctrlY2 = topy + 32 + 6*44 + 8;
            DrawText("CONTROLS", ctrlX2, ctrlY2, 15, (Color){ac.r,ac.g,ac.b,210});
            DrawLine(ctrlX2, ctrlY2+19, ctrlX2+290, ctrlY2+19, (Color){ac.r,ac.g,ac.b,55});
            struct { const char *key; const char *desc; } ctrl[] = {
                { "WASD / Arrows", "Move player" },
                { "1 / 2 / 3",    "Use item" },
                { "E  (menu)",     "Toggle enemies" },
                { "ENTER",         "Start / next round" },
                { "ESC",           "Pause / back" },
                { "F11",           "Fullscreen" },
            };
            for (int i = 0; i < 6; i++) {
                int ry3 = ctrlY2 + 26 + i * 28;
                Color rowbg2 = (i%2==0) ? (Color){20,16,34,110} : (Color){14,11,24,60};
                DrawRectangleRounded((Rectangle){(float)(ctrlX2-4),(float)(ry3-2),296,22},
                                     0.15f, 5, rowbg2);
                DrawRectangleRounded((Rectangle){(float)(ctrlX2-4),(float)(ry3-2),3,22},
                                     1.0f, 4, (Color){ac.r,ac.g,ac.b,90});
                DrawText(ctrl[i].key,  ctrlX2+6,   ry3+2, 13, (Color){ac.r,ac.g,ac.b,210});
                DrawText(ctrl[i].desc, ctrlX2+160, ry3+2, 13, (Color){155,150,185,200});
            }
        }

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
                if (CheckCollisionPointRec(mp2,tr)) { shopTab=i; upgScroll=0; }
            }
        }

        int cZoneTop  = tabY + 44;
        int cZoneX    = px2 + 12;
        int cZoneW    = pw - 24;

        // ── TAB 0: SKINS ─────────────────────────────────────────────────────
        if (shopTab == 0) {
            static const char* SKIN_NAMES[SKIN_COUNT] = {
                // Common (0-13)
                "Green","Red","Orange","Yellow","Lime",
                "Cyan","Blue","Violet","Magenta","White",
                "Brown","Black","Pink","Grey",
                // Uncommon (14-22)
                "Forest","Crimson","Amber","Gold","Viridian",
                "Glacial","Sapphire","Void",
                // Rare (22-26)
                "Pyro","Phantom","Cyber","Chroma","Biohazard",
                // Epic (27-29)
                "Alien","Skull","Angel"
            };
            static const int SKIN_PRICES[SKIN_COUNT] = {
                // Common (0-13)
                200, 200, 200, 200, 200,
                200, 200, 200, 200, 200,
                200, 200, 200, 200,
                // Uncommon (14-21)
                400, 400, 400, 400, 400,
                400, 400, 400,
                // Rare (22-26)
                600, 600, 750, 750, 600,
                // Epic (27-29)
                1000, 1000, 1000,
            };

            int cellSz  = 110, cellGap = 16;
            int perRow  = 5;
            int rowOff  = (cZoneW - (perRow * cellSz + (perRow-1) * cellGap)) / 2;

            int catHdrH  = 28; 
            int catGap   = 8;

            int skinRowH     = cellSz + cellGap + 28 + 10;
            int commonRows   = 3;
            int uncommonRows = 2;
            int rareRows     = 1;
            int epicRows     = 1;
            int totalSkinH   = catHdrH + catGap + commonRows * skinRowH + 20
                             + catHdrH + catGap + uncommonRows * skinRowH + 20
                             + catHdrH + catGap + rareRows * skinRowH + 20
                             + catHdrH + catGap + epicRows * skinRowH + 16;

            int skinZoneTop = cZoneTop;
            int skinZoneBot = py2 + ph - 70;
            int availH_skin = skinZoneBot - skinZoneTop;
            static int skinScroll = 0;

            int maxScrollSkin = totalSkinH - availH_skin;
            if (maxScrollSkin < 0) maxScrollSkin = 0;

            float wheelS = GetMouseWheelMoveV().y;
            if (wheelS != 0.0f) { skinScroll -= (int)(wheelS * 40.0f); }
            if (IsKeyDown(KEY_UP))   skinScroll -= 7;
            if (IsKeyDown(KEY_DOWN)) skinScroll += 7;
            if (skinScroll < 0) skinScroll = 0;
            if (skinScroll > maxScrollSkin) skinScroll = maxScrollSkin;

            BeginScissorMode(cZoneX, skinZoneTop, cZoneW, availH_skin);

            int curY = skinZoneTop - skinScroll + 8;

            // ── COMMON skins ─────────────────────────────────────────────────
            {
                float lgp = 0.5f + 0.5f*sinf(t*1.5f);
                Color comAc = ac; 
                int lbY = curY;
                DrawRectangleRounded((Rectangle){(float)(cZoneX+rowOff-8),(float)(lbY-2),(float)(perRow*cellSz+(perRow-1)*cellGap+16),(float)(catHdrH)},
                                     0.3f,6,(Color){comAc.r/8,comAc.g/8,comAc.b/8,200});
                DrawRectangleRoundedLines((Rectangle){(float)(cZoneX+rowOff-8),(float)(lbY-2),(float)(perRow*cellSz+(perRow-1)*cellGap+16),(float)(catHdrH)},
                                          0.3f,6,(Color){comAc.r,comAc.g,comAc.b,(uchar)(60+(int)(40*lgp))});
                const char *comLbl = "COMMON";
                DrawText(comLbl, cZoneX+rowOff, lbY+4, 17, (Color){comAc.r,comAc.g,comAc.b,230});
                curY += catHdrH + catGap;

                for (int i=0; i<14; i++) {
                    int col=i%perRow, row=i/perRow;
                    int cx2 = cZoneX + rowOff + col*(cellSz+cellGap);
                    int cy2 = curY + row*skinRowH;

                    bool owned = skin_owned[i];
                    bool equip = (skin_equipped==i);
                    float gp   = 0.5f+0.5f*sinf(t*2.0f+(float)i*0.7f);
                    Color sc2  = GetSkinColor(i);

                    if (equip) {
                        for (int g=5;g>=1;g--)
                            DrawRectangleRounded(
                                (Rectangle){(float)(cx2-g*3),(float)(cy2-g*3),(float)(cellSz+g*6),(float)(cellSz+g*6)},
                                0.22f,8,(Color){sc2.r,sc2.g,sc2.b,(uchar)(8*g)});
                    }
                    DrawRectangleRounded((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                         0.18f,8,(Color){16,13,26,200});
                    Color bord2 = owned
                        ? (Color){sc2.r,sc2.g,sc2.b,equip?255u:(uchar)(80+(int)(60*gp))}
                        : (Color){55,50,75,120};
                    DrawRectangleRoundedLines((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                             0.18f,8,bord2);

                    int pcx=cx2+cellSz/2, pcy=cy2+cellSz/2-8;
                    DrawCircle(pcx,pcy,18,sc2);
                    DrawCircle(pcx-3,pcy-4,9,(Color){CLAMP255(sc2.r+55),CLAMP255(sc2.g+55),CLAMP255(sc2.b+55),120});
                    DrawCircle(pcx-5,pcy-3,3,(Color){10,10,22,255});
                    DrawCircle(pcx+5,pcy-3,3,(Color){10,10,22,255});

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
                        Color bf2=(Color){bhov?35:20,bhov?28:14,bhov?52:32,230};
                        DrawRectangleRounded(btnR,0.3f,6,bf2);
                        DrawRectangleRoundedLines(btnR,0.3f,6,(Color){sc2.r,sc2.g,sc2.b,bhov?200u:100u});
                        int ew=MeasureText("EQUIP",12);
                        DrawText("EQUIP",cx2+cellSz/2-ew/2,btnY+7,12,
                                 (Color){CLAMP255(sc2.r+50),CLAMP255(sc2.g+50),CLAMP255(sc2.b+50),220});
                        if (bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { skin_equipped=i; SaveShop(); }
                    } else {
                        int price = SKIN_PRICES[i];
                        bool canBuy=(coin_balance>=price);
                        Color bf2=canBuy?(bhov?(Color){30,50,25,230}:(Color){20,35,18,200}):(Color){20,18,28,160};
                        DrawRectangleRounded(btnR,0.3f,6,bf2);
                        DrawRectangleRoundedLines(btnR,0.3f,6,canBuy?(Color){70,215,140,bhov?220u:100u}:(Color){50,46,68,80});
                        DrawCircle(cx2+cellSz/2-26,btnY+13,5,canBuy?CCOIN:(Color){80,70,40,180});
                        char priceStr[16]; snprintf(priceStr, sizeof(priceStr), "%d", price);
                        int pw2=MeasureText(priceStr,12);
                        DrawText(priceStr,cx2+cellSz/2-16,btnY+7,12,canBuy?(Color){255,215,0,220}:(Color){100,90,60,160});
                        (void)pw2;
                        if (canBuy && bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            coin_balance-=price; skin_owned[i]=1; skin_equipped=i;
                            SaveStats(); SaveShop();
                        }
                    }
                }
                curY += commonRows * skinRowH + 20;
            }

            // ── UNCOMMON skins ───────────────────────────────────────────────────
            {
                float lgp = 0.5f + 0.5f*sinf(t*1.5f + 0.8f);
                Color uncommonAc = (Color){255,180,0,255}; 
                int lbY = curY;
                DrawRectangleRounded((Rectangle){(float)(cZoneX+rowOff-8),(float)(lbY-2),(float)(perRow*cellSz+(perRow-1)*cellGap+16),(float)(catHdrH)},
                                     0.3f,6,(Color){uncommonAc.r/8,uncommonAc.g/8,uncommonAc.b/8,200});
                DrawRectangleRoundedLines((Rectangle){(float)(cZoneX+rowOff-8),(float)(lbY-2),(float)(perRow*cellSz+(perRow-1)*cellGap+16),(float)(catHdrH)},
                                          0.3f,6,(Color){uncommonAc.r,uncommonAc.g,uncommonAc.b,(uchar)(60+(int)(40*lgp))});
                const char *uncommonLbl = "UNCOMMON";
                DrawText(uncommonLbl, cZoneX+rowOff, lbY+4, 17, (Color){uncommonAc.r,uncommonAc.g,uncommonAc.b,230});
                curY += catHdrH + catGap;

                for (int ri=0; ri<8; ri++) {
                    int i = ri + 14;
                    int col=ri%perRow, row=ri/perRow;
                    int cx2 = cZoneX + rowOff + col*(cellSz+cellGap);
                    int cy2 = curY + row*skinRowH;

                    bool owned = skin_owned[i];
                    bool equip = (skin_equipped==i);
                    float gp   = 0.5f+0.5f*sinf(t*2.0f+(float)i*0.7f);
                    Color sc2  = GetSkinColor(i);
                    Color uncommonAcSkin = (Color){255,180,0,255};

                    // uncommon glow effect
                    if (owned) {
                        for (int g=4;g>=1;g--)
                            DrawRectangleRounded(
                                (Rectangle){(float)(cx2-g*2),(float)(cy2-g*2),(float)(cellSz+g*4),(float)(cellSz+g*4)},
                                0.22f,8,(Color){uncommonAcSkin.r,uncommonAcSkin.g,uncommonAcSkin.b,(uchar)(4*g+(int)(3*g*gp))});
                    }
                    if (equip) {
                        for (int g=5;g>=1;g--)
                            DrawRectangleRounded(
                                (Rectangle){(float)(cx2-g*3),(float)(cy2-g*3),(float)(cellSz+g*6),(float)(cellSz+g*6)},
                                0.22f,8,(Color){sc2.r,sc2.g,sc2.b,(uchar)(8*g)});
                    }
                    DrawRectangleRounded((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                         0.18f,8,(Color){20,14,30,220});
                    Color bord2 = owned
                        ? (Color){sc2.r,sc2.g,sc2.b,equip?255u:(uchar)(100+(int)(80*gp))}
                        : (Color){80,60,40,120};
                    DrawRectangleRoundedLines((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                             0.18f,8,bord2);
                    // Uncommon inner border sparkle
                    if (owned) {
                        DrawRectangleRoundedLines((Rectangle){(float)(cx2+3),(float)(cy2+3),(float)(cellSz-6),(float)(cellSz-6)},
                                                  0.18f,8,(Color){uncommonAcSkin.r,uncommonAcSkin.g,uncommonAcSkin.b,(uchar)(30+(int)(20*gp))});
                    }
                   
                    int pcx=cx2+cellSz/2, pcy=cy2+cellSz/2-8;
                    DrawCircle(pcx,pcy,18,sc2);
                    DrawCircle(pcx-3,pcy-4,9,(Color){CLAMP255(sc2.r+55),CLAMP255(sc2.g+55),CLAMP255(sc2.b+55),120});
                    DrawCircle(pcx-5,pcy-3,3,(Color){10,10,22,255});
                    DrawCircle(pcx+5,pcy-3,3,(Color){10,10,22,255});

                    int nw2=MeasureText(SKIN_NAMES[i],13);
                    DrawText(SKIN_NAMES[i],cx2+cellSz/2-nw2/2,cy2+cellSz-22,13,
                             owned?(Color){220,210,240,230}:(Color){90,80,110,180});

                    int btnY=cy2+cellSz+6, btnH=26;
                    Rectangle btnR={(float)cx2,(float)btnY,(float)cellSz,(float)btnH};
                    Vector2 mp2=GetMousePosition();
                    bool bhov=CheckCollisionPointRec(mp2,btnR);

                    if (equip) {
                        DrawRectangleRounded(btnR,0.3f,6,(Color){40,30,10,220});
                        DrawRectangleRoundedLines(btnR,0.3f,6,(Color){uncommonAcSkin.r,uncommonAcSkin.g,0,255});
                        int ew=MeasureText("EQUIPPED",12);
                        DrawText("EQUIPPED",cx2+cellSz/2-ew/2,btnY+7,12,(Color){70,215,140,255});
                    } else if (owned) {
                        Color bf2=(Color){bhov?50:30,bhov?40:22,bhov?18:10,230};
                        DrawRectangleRounded(btnR,0.3f,6,bf2);
                        DrawRectangleRoundedLines(btnR,0.3f,6,(Color){sc2.r,sc2.g,sc2.b,bhov?200u:100u});
                        int ew=MeasureText("EQUIP",12);
                        DrawText("EQUIP",cx2+cellSz/2-ew/2,btnY+7,12,
                                 (Color){CLAMP255(sc2.r+50),CLAMP255(sc2.g+50),CLAMP255(sc2.b+50),220});
                        if (bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { skin_equipped=i; SaveShop(); }
                    } else {
                        int price = SKIN_PRICES[i];
                        bool canBuy=(coin_balance>=price);
                        Color bf2=canBuy?(bhov?(Color){50,42,10,230}:(Color){35,28,8,200}):(Color){20,18,28,160};
                        DrawRectangleRounded(btnR,0.3f,6,bf2);
                        DrawRectangleRoundedLines(btnR,0.3f,6,canBuy?(Color){uncommonAcSkin.r,uncommonAcSkin.g,0,bhov?220u:100u}:(Color){60,50,30,80});
                        DrawCircle(cx2+cellSz/2-26,btnY+13,5,canBuy?CCOIN:(Color){80,70,40,180});
                        char priceStr[16]; snprintf(priceStr, sizeof(priceStr), "%d", price);
                        DrawText(priceStr,cx2+cellSz/2-16,btnY+7,12,canBuy?(Color){255,215,0,230}:(Color){100,90,60,160});
                        if (canBuy && bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            coin_balance-=price; skin_owned[i]=1; skin_equipped=i;
                            SaveStats(); SaveShop();
                        }
                    }
                }
                curY += uncommonRows * skinRowH + 20;
            }

            // ── RARE skins ───────────────────────────────────────────────────────
            {
                float lgp = 0.5f + 0.5f*sinf(t*1.5f + 1.6f);
                Color rareAc = (Color){255, 80, 80, 255};
                int lbY = curY;
                DrawRectangleRounded((Rectangle){(float)(cZoneX+rowOff-8),(float)(lbY-2),(float)(perRow*cellSz+(perRow-1)*cellGap+16),(float)(catHdrH)},
                                     0.3f,6,(Color){rareAc.r/8,rareAc.g/8,rareAc.b/8,200});
                DrawRectangleRoundedLines((Rectangle){(float)(cZoneX+rowOff-8),(float)(lbY-2),(float)(perRow*cellSz+(perRow-1)*cellGap+16),(float)(catHdrH)},
                                          0.3f,6,(Color){rareAc.r,rareAc.g,rareAc.b,(uchar)(60+(int)(40*lgp))});
                const char *rareLbl = "RARE";
                DrawText(rareLbl, cZoneX+rowOff, lbY+4, 17, (Color){rareAc.r,rareAc.g,rareAc.b,230});
                curY += catHdrH + catGap;

                for (int ri=0; ri<5; ri++) {
                    int i = ri + 22;
                    int col=ri%perRow, row=ri/perRow;
                    int cx2 = cZoneX + rowOff + col*(cellSz+cellGap);
                    int cy2 = curY + row*skinRowH;

                    bool owned = skin_owned[i];
                    bool equip = (skin_equipped==i);
                    float gp   = 0.5f+0.5f*sinf(t*2.0f+(float)i*0.7f);
                    Color sc2  = GetSkinColor(i);

                    // rare glow effect (stronger than uncommon, slightly toned down)
                    if (owned) {
                        for (int g=5;g>=1;g--)
                            DrawRectangleRounded(
                                (Rectangle){(float)(cx2-g*3),(float)(cy2-g*3),(float)(cellSz+g*6),(float)(cellSz+g*6)},
                                0.22f,8,(Color){rareAc.r,rareAc.g,rareAc.b,(uchar)(4*g+(int)(3*g*gp))});
                    }
                    if (equip) {
                        for (int g=6;g>=1;g--)
                            DrawRectangleRounded(
                                (Rectangle){(float)(cx2-g*3),(float)(cy2-g*3),(float)(cellSz+g*6),(float)(cellSz+g*6)},
                                0.22f,8,(Color){sc2.r,sc2.g,sc2.b,(uchar)(10*g)});
                    }
                    DrawRectangleRounded((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                         0.18f,8,(Color){28,10,10,230});
                    // Border: animated color preview even when unowned
                    Color bord2 = owned
                        ? (Color){sc2.r,sc2.g,sc2.b,equip?255u:(uchar)(120+(int)(100*gp))}
                        : (Color){sc2.r,sc2.g,sc2.b,55};
                    DrawRectangleRoundedLines((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                             0.18f,8,bord2);
                    // Inner double border (owned only)
                    if (owned) {
                        DrawRectangleRoundedLines((Rectangle){(float)(cx2+3),(float)(cy2+3),(float)(cellSz-6),(float)(cellSz-6)},
                                                  0.18f,8,(Color){rareAc.r,rareAc.g,rareAc.b,(uchar)(50+(int)(35*gp))});
                        DrawRectangleRoundedLines((Rectangle){(float)(cx2+6),(float)(cy2+6),(float)(cellSz-12),(float)(cellSz-12)},
                                                  0.18f,8,(Color){rareAc.r,rareAc.g,rareAc.b,(uchar)(20+(int)(15*gp))});
                    }

                    int pcx=cx2+cellSz/2, pcy=cy2+cellSz/2-8;
                    DrawRareBack(i, (float)pcx, (float)pcy, 18.0f, t);
                    DrawCircle(pcx,pcy,18,sc2);
                    DrawCircle(pcx-3,pcy-4,9,(Color){CLAMP255(sc2.r+55),CLAMP255(sc2.g+55),CLAMP255(sc2.b+55),90});
                    DrawRareFace(i, (float)pcx, (float)pcy, 18.0f, t);

                    int nw2=MeasureText(SKIN_NAMES[i],13);
                    DrawText(SKIN_NAMES[i],cx2+cellSz/2-nw2/2,cy2+cellSz-22,13,
                             owned?(Color){235,200,200,240}:(Color){155,140,175,190});

                    int btnY=cy2+cellSz+6, btnH=26;
                    Rectangle btnR={(float)cx2,(float)btnY,(float)cellSz,(float)btnH};
                    Vector2 mp2=GetMousePosition();
                    bool bhov=CheckCollisionPointRec(mp2,btnR);

                    if (equip) {
                        DrawRectangleRounded(btnR,0.3f,6,(Color){50,10,10,220});
                        DrawRectangleRoundedLines(btnR,0.3f,6,rareAc);
                        int ew=MeasureText("EQUIPPED",12);
                        DrawText("EQUIPPED",cx2+cellSz/2-ew/2,btnY+7,12,(Color){70,215,140,255});
                    } else if (owned) {
                        Color bf2=(Color){bhov?60:35,bhov?15:10,bhov?15:10,230};
                        DrawRectangleRounded(btnR,0.3f,6,bf2);
                        DrawRectangleRoundedLines(btnR,0.3f,6,(Color){sc2.r,sc2.g,sc2.b,bhov?220u:120u});
                        int ew=MeasureText("EQUIP",12);
                        DrawText("EQUIP",cx2+cellSz/2-ew/2,btnY+7,12,
                                 (Color){CLAMP255(sc2.r+50),CLAMP255(sc2.g+50),CLAMP255(sc2.b+50),220});
                        if (bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { skin_equipped=i; SaveShop(); }
                    } else {
                        int price = SKIN_PRICES[i];
                        bool canBuy=(coin_balance>=price);
                        Color bf2=canBuy?(bhov?(Color){60,18,10,230}:(Color){40,12,8,200}):(Color){20,18,28,160};
                        DrawRectangleRounded(btnR,0.3f,6,bf2);
                        DrawRectangleRoundedLines(btnR,0.3f,6,canBuy?(Color){rareAc.r,rareAc.g,rareAc.b,bhov?220u:100u}:(Color){60,40,40,80});
                        DrawCircle(cx2+cellSz/2-26,btnY+13,5,canBuy?CCOIN:(Color){80,70,40,180});
                        char priceStr[16]; snprintf(priceStr, sizeof(priceStr), "%d", price);
                        DrawText(priceStr,cx2+cellSz/2-16,btnY+7,12,canBuy?(Color){255,215,0,230}:(Color){100,90,60,160});
                        if (canBuy && bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            coin_balance-=price; skin_owned[i]=1; skin_equipped=i;
                            SaveStats(); SaveShop();
                        }
                    }
                }
                curY += rareRows * skinRowH + 20;
            }

            // ── EPIC skins ───────────────────────────────────────────────────────
            {
                float lgp = 0.5f + 0.5f*sinf(t*1.5f + 2.4f);
                Color epicAc = (Color){180, 100, 255, 255};
                int lbY = curY;
                DrawRectangleRounded((Rectangle){(float)(cZoneX+rowOff-8),(float)(lbY-2),(float)(perRow*cellSz+(perRow-1)*cellGap+16),(float)(catHdrH)},
                                     0.3f,6,(Color){epicAc.r/8,epicAc.g/8,epicAc.b/8,200});
                DrawRectangleRoundedLines((Rectangle){(float)(cZoneX+rowOff-8),(float)(lbY-2),(float)(perRow*cellSz+(perRow-1)*cellGap+16),(float)(catHdrH)},
                                          0.3f,6,(Color){epicAc.r,epicAc.g,epicAc.b,(uchar)(60+(int)(40*lgp))});
                const char *epicLbl = "EPIC";
                DrawText(epicLbl, cZoneX+rowOff, lbY+4, 17, (Color){epicAc.r,epicAc.g,epicAc.b,230});
                curY += catHdrH + catGap;

                for (int ri=0; ri<3; ri++) {
                    int i = ri + 27;
                    int col=ri%perRow, row=ri/perRow;
                    int cx2 = cZoneX + rowOff + col*(cellSz+cellGap);
                    int cy2 = curY + row*skinRowH;

                    bool owned = skin_owned[i];
                    bool equip = (skin_equipped==i);
                    float gp   = 0.5f+0.5f*sinf(t*2.0f+(float)i*0.7f);
                    Color sc2  = GetSkinColor(i);

                    // epic glow effect (stronger than rare, slightly toned down)
                    if (owned) {
                        for (int g=6;g>=1;g--)
                            DrawRectangleRounded(
                                (Rectangle){(float)(cx2-g*3),(float)(cy2-g*3),(float)(cellSz+g*6),(float)(cellSz+g*6)},
                                0.22f,8,(Color){epicAc.r,epicAc.g,epicAc.b,(uchar)(4*g+(int)(3*g*gp))});
                    }
                    if (equip) {
                        for (int g=7;g>=1;g--)
                            DrawRectangleRounded(
                                (Rectangle){(float)(cx2-g*3),(float)(cy2-g*3),(float)(cellSz+g*6),(float)(cellSz+g*6)},
                                0.22f,8,(Color){sc2.r,sc2.g,sc2.b,(uchar)(9*g)});
                    }
                    DrawRectangleRounded((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                         0.18f,8,(Color){20,10,30,235});
                    Color bord2 = owned
                        ? (Color){sc2.r,sc2.g,sc2.b,equip?255u:(uchar)(120+(int)(100*gp))}
                        : (Color){epicAc.r,epicAc.g,epicAc.b,60};
                    DrawRectangleRoundedLines((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                             0.18f,8,bord2);
                    if (owned) {
                        DrawRectangleRoundedLines((Rectangle){(float)(cx2+3),(float)(cy2+3),(float)(cellSz-6),(float)(cellSz-6)},
                                                  0.18f,8,(Color){epicAc.r,epicAc.g,epicAc.b,(uchar)(55+(int)(40*gp))});
                        DrawRectangleRoundedLines((Rectangle){(float)(cx2+6),(float)(cy2+6),(float)(cellSz-12),(float)(cellSz-12)},
                                                  0.18f,8,(Color){epicAc.r,epicAc.g,epicAc.b,(uchar)(25+(int)(18*gp))});
                    }

                    int pcx=cx2+cellSz/2, pcy=cy2+cellSz/2-8;
                    DrawEpicBack(i, (float)pcx, (float)pcy, 18.0f, t);
                    if (i == 28) {
                        DrawSkull((float)pcx, (float)pcy, 18.0f, t);
                    } else {
                        DrawCircle(pcx,pcy,18,sc2);
                        DrawEpicFace(i, (float)pcx, (float)pcy, 18.0f, t);
                    }

                    int nw2=MeasureText(SKIN_NAMES[i],13);
                    DrawText(SKIN_NAMES[i],cx2+cellSz/2-nw2/2,cy2+cellSz-22,13,
                             owned?(Color){225,205,245,240}:(Color){160,145,185,190});

                    int btnY=cy2+cellSz+6, btnH=26;
                    Rectangle btnR={(float)cx2,(float)btnY,(float)cellSz,(float)btnH};
                    Vector2 mp2=GetMousePosition();
                    bool bhov=CheckCollisionPointRec(mp2,btnR);

                    if (equip) {
                        DrawRectangleRounded(btnR,0.3f,6,(Color){35,12,55,220});
                        DrawRectangleRoundedLines(btnR,0.3f,6,epicAc);
                        int ew=MeasureText("EQUIPPED",12);
                        DrawText("EQUIPPED",cx2+cellSz/2-ew/2,btnY+7,12,(Color){70,215,140,255});
                    } else if (owned) {
                        Color bf2=(Color){bhov?45:28,bhov?20:12,bhov?68:42,230};
                        DrawRectangleRounded(btnR,0.3f,6,bf2);
                        DrawRectangleRoundedLines(btnR,0.3f,6,(Color){sc2.r,sc2.g,sc2.b,bhov?220u:120u});
                        int ew=MeasureText("EQUIP",12);
                        DrawText("EQUIP",cx2+cellSz/2-ew/2,btnY+7,12,
                                 (Color){CLAMP255(sc2.r+50),CLAMP255(sc2.g+50),CLAMP255(sc2.b+50),220});
                        if (bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { skin_equipped=i; SaveShop(); }
                    } else {
                        int price = SKIN_PRICES[i];
                        bool canBuy=(coin_balance>=price);
                        Color bf2=canBuy?(bhov?(Color){45,18,68,230}:(Color){32,12,48,200}):(Color){20,18,28,160};
                        DrawRectangleRounded(btnR,0.3f,6,bf2);
                        DrawRectangleRoundedLines(btnR,0.3f,6,canBuy?(Color){epicAc.r,epicAc.g,epicAc.b,bhov?220u:100u}:(Color){55,45,70,80});
                        DrawCircle(cx2+cellSz/2-26,btnY+13,5,canBuy?CCOIN:(Color){80,70,40,180});
                        char priceStr[16]; snprintf(priceStr, sizeof(priceStr), "%d", price);
                        DrawText(priceStr,cx2+cellSz/2-16,btnY+7,12,canBuy?(Color){255,215,0,230}:(Color){100,90,60,160});
                        if (canBuy && bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            coin_balance-=price; skin_owned[i]=1; skin_equipped=i;
                            SaveStats(); SaveShop();
                        }
                    }
                }
            }

            EndScissorMode();

            if (maxScrollSkin > 0) {
                Color aca = (Color){255,205,55,255};
                int sbX  = cZoneX + cZoneW - 10;
                int sbY  = skinZoneTop;
                int sbH  = availH_skin;
                int sbTH = (int)((float)availH_skin / totalSkinH * sbH);
                if (sbTH < 20) sbTH = 20;
                int sbTY = sbY + (int)((float)skinScroll / maxScrollSkin * (sbH - sbTH));
                DrawRectangleRounded((Rectangle){(float)sbX,(float)sbY,4.0f,(float)sbH},
                                     1.0f,4,(Color){aca.r,aca.g,aca.b,30});
                DrawRectangleRounded((Rectangle){(float)sbX,(float)sbTY,4.0f,(float)sbTH},
                                     1.0f,4,(Color){aca.r,aca.g,aca.b,140});
            }
        }

        // ── TAB 1: UPGRADES ──────────────────────────────────────────────────
    if (shopTab == 1) {
        static const char* s_spd[] = { "3s (base)","4s","5s","6s","7s","8s" };
        static const char* s_shd[] = { "5s (base)","10s","20s","30s","45s","Unlimited" };
        static const char* s_wlk[] = { "0.5s (base)","1s","1.5s","2s","2.5s","3s" };
        static const char* s_swd[] = { "Range 1 (base)","Range 2","Range 3","Range 4" };
        static const char* s_bmb[] = { "Range 1 (base)","Range 2","Range 3","Range 4","Range 5","Range 6" };

        struct {
            const char *name, *desc;
            int maxlvl;
            int *lvl;
            const int *cost;
            Color col;
            const char **descs;
        } upgrades[] = {
            { "SPEED BOOST",  "Extends SPD item duration",        5, &spd_level, SPD_COST, IK_COL[IK_SPD], s_spd },
            { "SHIELD",       "Extends SHD item duration",        5, &shd_level, SHD_COST, IK_COL[IK_SHD], s_shd },
            { "GHOSTWALK",    "Extends WLK item duration",        5, &wlk_level, WLK_COST, IK_COL[IK_WLK], s_wlk },
            { "SWORD REACH",  "Extends SWD kill radius",          3, &swd_level, SWD_COST, IK_COL[IK_SWD], s_swd },
            { "BOMB RADIUS",  "Increases BMB explosion radius",   5, &bmb_level, BMB_COST, IK_COL[IK_BMB], s_bmb },
        };
        
        int nUpg   = 5;
        int cardH  = 100;
        int cardGap = 8;

        // ── Scrollbar logic
        int upgZoneTop = cZoneTop;
        int upgZoneBot = py2 + ph - 70; 
        int availH_upg = upgZoneBot - upgZoneTop;
        
        int topScrollOffset = 8; 
        int totalH_upg = topScrollOffset + (nUpg * (cardH + cardGap)); 

        int maxScrollUpg = totalH_upg - availH_upg;
        if (maxScrollUpg < 0) maxScrollUpg = 0;

        float wheel = GetMouseWheelMoveV().y;
        if (wheel != 0.0f) { upgScroll -= (int)(wheel * 40.0f); }
        if (IsKeyDown(KEY_UP))   upgScroll -= 7;
        if (IsKeyDown(KEY_DOWN)) upgScroll += 7;
        if (upgScroll < 0) upgScroll = 0;
        if (upgScroll > maxScrollUpg) upgScroll = maxScrollUpg;

        BeginScissorMode(cZoneX, upgZoneTop, cZoneW, availH_upg);
        
        for (int i = 0; i < nUpg; i++) {
            int uy = (upgZoneTop + topScrollOffset) + i * (cardH + cardGap) - upgScroll;
            
            int ux = cZoneX + 20;
            int uw = cZoneW - 40;
            int uh = cardH;
            
            if (uy + uh < upgZoneTop || uy > upgZoneBot) continue;
            
            float gp = 0.5f + 0.5f * sinf(t * 1.6f + (float)i * 1.2f);

            DrawRectangleRounded((Rectangle){(float)ux,(float)uy,(float)uw,(float)uh},
                                 0.12f,8,(Color){16,13,26,200});
            DrawRectangleRoundedLines((Rectangle){(float)ux,(float)uy,(float)uw,(float)uh},
                                         0.12f,8,(Color){upgrades[i].col.r,upgrades[i].col.g,upgrades[i].col.b,
                                                         (uchar)(60+(int)(50*gp))});
            DrawRectangleRounded((Rectangle){(float)ux,(float)uy,4,(float)uh},
                                 1.0f,4,upgrades[i].col);

            int dix = ux + 22, diy = uy + uh / 2;
            DrawTriangle((Vector2){(float)dix,(float)(diy-12)},
                         (Vector2){(float)(dix+11),(float)diy},
                         (Vector2){(float)dix,(float)(diy+12)}, upgrades[i].col);
            DrawTriangle((Vector2){(float)dix,(float)(diy-12)},
                         (Vector2){(float)dix,(float)(diy+12)},
                         (Vector2){(float)(dix-11),(float)diy}, upgrades[i].col);

            DrawText(upgrades[i].name, ux+46,uy+10,16,upgrades[i].col);
            DrawText(upgrades[i].desc, ux+46,uy+30,12,(Color){140,135,170,210});

            DrawText(TextFormat("Lv %d/%d  %s",
                                *upgrades[i].lvl, upgrades[i].maxlvl,
                                upgrades[i].descs[*upgrades[i].lvl]),
                     ux+46,uy+48,12,(Color){170,165,200,220});

            int visMax = upgrades[i].maxlvl;
            for (int l = 0; l < visMax; l++) {
                int px3 = ux + 46 + l * 20, py3 = uy + 72;
                bool filled = (l < *upgrades[i].lvl);
                DrawCircle(px3,py3,6,filled ? upgrades[i].col : (Color){30,26,44,200});
                DrawCircleLines(px3,py3,6,upgrades[i].col);
                if (filled) DrawCircle(px3,py3,3,(Color){255,255,255,180});
            }

            bool maxed = (*upgrades[i].lvl >= upgrades[i].maxlvl);
            int btnX2 = ux + uw - 175, btnY2 = uy + uh / 2 - 18, btnW2 = 155, btnH2 = 36;
            Rectangle btnR2 = {(float)btnX2,(float)btnY2,(float)btnW2,(float)btnH2};
            Vector2 mp2 = GetMousePosition();
            bool bhov = CheckCollisionPointRec(mp2,btnR2);
            bool canBuy = (coin_balance >= upgrades[i].cost[*upgrades[i].lvl] && !maxed);

            Color bf = maxed ? (Color){20,18,30,160}
                             : canBuy ? (bhov ? (Color){35,60,28,240} : (Color){20,40,18,200})
                                      : (Color){20,18,30,160};
            DrawRectangleRounded(btnR2,0.25f,8,bf);
            DrawRectangleRoundedLines(btnR2,0.25f,8,
                maxed ? (Color){60,56,80,100}
                      : canBuy ? (Color){70,215,140,bhov ? 220u : 120u}
                               : (Color){60,56,80,100});

            if (maxed) {
                int mw3 = MeasureText("MAX",13);
                DrawText("MAX",btnX2+btnW2/2-mw3/2,btnY2+11,13,(Color){120,115,150,180});
            } else {
                DrawCircle(btnX2+24,btnY2+18,7,canBuy ? CCOIN : (Color){70,60,30,160});
                DrawText(TextFormat("%d",upgrades[i].cost[*upgrades[i].lvl]),btnX2+34,btnY2+11,13,
                         canBuy ? (Color){255,215,0,230} : (Color){100,90,55,160});
                int uw3 = MeasureText("UPGRADE",13);
                DrawText("UPGRADE",btnX2+btnW2/2-uw3/2+16,btnY2+11,13,
                         canBuy ? (Color){200,255,200,230} : (Color){90,85,115,160});
                if (canBuy && bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    coin_balance -= upgrades[i].cost[*upgrades[i].lvl]; 
                    (*upgrades[i].lvl)++;
                    SaveStats(); SaveShop();
                }
            }
        }
        EndScissorMode();

        // ── Scrollbar render 
        if (maxScrollUpg > 0) {
            Color upgAc = (Color){255,200,50,255};
            int sbX  = cZoneX + cZoneW - 10;
            int sbY  = upgZoneTop;
            int sbH  = availH_upg;
            int sbTH = (int)((float)availH_upg / totalH_upg * sbH);
            if (sbTH < 20) sbTH = 20;
            int sbTY = sbY + (int)((float)upgScroll / maxScrollUpg * (sbH - sbTH));
            
            DrawRectangleRounded((Rectangle){(float)sbX,(float)sbY,4.0f,(float)sbH},
                                 1.0f,4,(Color){upgAc.r,upgAc.g,upgAc.b,30});
            DrawRectangleRounded((Rectangle){(float)sbX,(float)sbTY,4.0f,(float)sbTH},
                                 1.0f,4,(Color){upgAc.r,upgAc.g,upgAc.b,140});
        }
    }

        // ── TAB 2: PERKS ─────────────────────────────────────────────────────
        if (shopTab == 2) {
            struct { const char*name; const char*desc; int*flag; int maxv; int cost; Color col; } perks[] = {
                { "EXTRA SLOT I",   "Carry one more item in your inventory",    &perk_slots,   2, 1000, (Color){160,150,220,255} },
                { "EXTRA SLOT II",  "Carry two more items (requires Slot I)",   &perk_slots,   2, 2000, (Color){130,120,200,255} },
                { "COIN MAGNET",    "Automatically collect adjacent coins",     &perk_magnet,  1, 2500, CCOIN                    },
                { "SHIELD CHARGE",  "Start each round with a shield charge",    &perk_shield2, 1, 3000, (Color){ 80,150,255,255} },
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
// ── ACHIEVEMENTS ────────────────────────────────────────────────
    static int advScroll = 0; 
    if (menuSub == 4) {
        Color ac = (Color){55, 205, 255, 255}; 
        DrawSubPanel("ACHIEVEMENTS", ac, t, sw, sh);
        int margin2 = 32, pw = sw - margin2 * 2, ph = sh - margin2 * 2;
        int px2 = margin2, py2 = margin2;

        int zoneTop    = py2 + 64;
        int zoneBottom = py2 + ph - 70;
        int availH     = zoneBottom - zoneTop;

        int rowH = availH / 7;
        if (rowH < 56) rowH = 56; 

        struct {
            const char *label;
            int current_val;
            int tiers[3];
            Color col;
        } achievements[] = {
            { "Total Games Played",              global_games_played, { 50, 100, 200 },        (Color){200, 185, 255, 255} },
            { "Total Rounds Played",             global_rounds,       { 500, 1000, 2000 },     (Color){200, 195, 235, 255} },
            { "Total Coins Collected",           global_totcoi,       { 100, 200, 500 },       (Color){255, 215, 0, 255} },
            { "Total Enemy Kills",               global_totkills,     { 50, 150, 300 },        (Color){220, 90, 90, 255} },
            { "Most Treasures in One Game",      hs_trs,              { 10, 20, 25 },          CTRES },
            { "Most Coins in One Game",          hs_coi,              { 50, 100, 150 },        (Color){255, 215, 0, 255} },
            { "Most Kills in One Game",         hs_kills,            { 10, 25, 50 },          (Color){220, 75, 75, 255} }
            };
        int numachievements = sizeof(achievements) / sizeof(achievements[0]);
        int totalRowsH = numachievements * rowH; 

        int maxScroll = totalRowsH - availH;
        if (maxScroll < 0) maxScroll = 0;
        float wheel = GetMouseWheelMoveV().y;
        if (wheel != 0.0f) { advScroll -= (int)(wheel * 40.0f); }
        if (IsKeyDown(KEY_UP))   advScroll -= 7;
        if (IsKeyDown(KEY_DOWN)) advScroll += 7;
        if (advScroll < 0) advScroll = 0;
        if (advScroll > maxScroll) advScroll = maxScroll;
        int cx = px2 + 40;
        BeginScissorMode(px2, zoneTop, pw, availH);
        for (int i = 0; i < numachievements; i++) {
            int ry2 = zoneTop + i * rowH - advScroll;
            if (ry2 + rowH < zoneTop || ry2 > zoneBottom) continue;
            Color rowbg = (i % 2 == 0) ? (Color){20, 16, 34, 160} : (Color){15, 12, 26, 90};
            DrawRectangleRounded(
                (Rectangle){(float)(cx - 8), (float)(ry2 - 4), (float)(pw - 64), (float)(rowH - 6)},
                0.12f, 6, rowbg);
           
            DrawRectangleRounded(
                (Rectangle){(float)(cx - 8), (float)(ry2 - 4), 4.0f, (float)(rowH - 6)},
                1.0f, 4, achievements[i].col);
            DrawText(achievements[i].label, cx + 6, ry2 + 2, 14, (Color){140, 135, 170, 210});
            int achieved_tier = 0;
            for (int j = 0; j < 3; j++) {
                if (achievements[i].current_val >= achievements[i].tiers[j]) {
                    achieved_tier = j + 1;
                } else {
                    break; 
                }
            }
            const char* progressText;
            if (achieved_tier < 3) {
                progressText = TextFormat("%d / %d", achievements[i].current_val, achievements[i].tiers[achieved_tier]);
            } else {
                progressText = TextFormat("%d (MAX)", achievements[i].current_val);
            }
            DrawText(progressText, cx + 6, ry2 + 18, 18, achievements[i].col);

            float pct = 0.0f;
            if (achieved_tier < 3) {
                int current_tier_start = (achieved_tier == 0) ? 0 : achievements[i].tiers[achieved_tier - 1];
                int next_tier_target = achievements[i].tiers[achieved_tier];
                int range = next_tier_target - current_tier_start;
                if (range > 0) {
                    pct = (float)(achievements[i].current_val - current_tier_start) / range;
                }
            } else {
                pct = 1.0f;
            }
            if (pct < 0.0f) pct = 0.0f;
            if (pct > 1.0f) pct = 1.0f;

            int barW = 180;
            int barH = 5;
            int barX = cx + 6;
            int barY = ry2 + 41;

            DrawRectangleRounded((Rectangle){(float)barX, (float)barY, (float)barW, (float)barH}, 1.0f, 4, (Color){45, 40, 65, 255});
            if (pct > 0.0f) {
                DrawRectangleRounded((Rectangle){(float)barX, (float)barY, (float)barW * pct, (float)barH}, 1.0f, 4, achievements[i].col);
            }

            int starSpacing = 36; 
            int numStars = 3;
            int totalStarsSpan = (numStars - 1) * starSpacing; 
            int starStartX = cx + (pw - 64) - totalStarsSpan - 32;
            int starCenterY = (ry2 - 4) + (rowH - 6) / 2; 

            int borderPaddingX = 20; 
            int borderPaddingY = 18;

            if (achieved_tier == 3) {
                Rectangle borderRect = {
                    (float)(starStartX - borderPaddingX),
                    (float)(starCenterY - borderPaddingY),
                    (float)(totalStarsSpan + (2 * borderPaddingX)), 
                    (float)(2 * borderPaddingY)                     
                };
                DrawRectangleRoundedLines(borderRect, 0.3f, 4, (Color){255, 215, 0, 220});
            }
            for (int s = 0; s < 3; s++) {
                int starX = starStartX + s * starSpacing;
                if (s < achieved_tier) {
                    DrawCustomStar((float)starX, (float)starCenterY, 13.0f, 5.5f, (Color){255, 205, 55, 255});
                } else {
                    DrawCustomStarLines((float)starX, (float)starCenterY, 13.0f, 5.5f, (Color){80, 75, 105, 120});
                }
            }
        }
        EndScissorMode();
        
        if (maxScroll > 0) {
            int sbX  = px2 + pw - 10;
            int sbY  = zoneTop;
            int sbH  = availH;
            int sbTH = (int)((float)availH / totalRowsH * sbH);
            if (sbTH < 20) sbTH = 20;
            int sbTY = sbY + (int)((float)advScroll / maxScroll * (sbH - sbTH));
            DrawRectangleRounded((Rectangle){(float)sbX, (float)sbY, 4.0f, (float)sbH},
                                 1.0f, 4, (Color){ac.r, ac.g, ac.b, 30});
            DrawRectangleRounded((Rectangle){(float)sbX, (float)sbTY, 4.0f, (float)sbTH},
                                 1.0f, 4, (Color){ac.r, ac.g, ac.b, 140});
        }
        return;
    }
    // ── FUTURE UPDATES ────────────────────────────────────────────────────────
    if (menuSub == 5) {
        Color ac = (Color){185,90,255,255};
        Rectangle backR = DrawSubPanel("FUTURE UPDATES", ac, t, sw, sh);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(GetMousePosition(), backR))
            { menuSub = 0; return; }

        int margin2=32, pw=sw-margin2*2, ph=sh-margin2*2;
        int px2=margin2, py2=margin2;

        // ── Update data ──────────────────────────────────────────────────────
        const char *ver_str  = "v1.5";
        const char *upd_name = "Legacy of Stars";
        static const char *features[] = {
            "Advancement Overhaul: All Advancements expanded to 6 tiers with 5-star display",
            "New Advancements: 10+ new Advancements added across new categories",
            "Advancement Rewards: Each tier now grants Coins, with Tier 6 unlocking exclusive Skins",
            "Music System: Background music and sound effects added via raylib AudioStream",
            "Settings Expanded: Audio sliders, gameplay toggles and UI options added to the Settings menu",
            "New Skins: 8 additional Skins added to the Shop",
            "New Legendary Skins: 3 Skins exclusive to Tier 6 Advancement completion",
            "Code Architecture: Codebase split into multiple .c/.h modules mirroring professional game structure",
            "Advancement Names: All Advancements receive unique names and lore-flavored titles",
            "Lowering Skin Prices so that they will be easier to afford"
        };
        int feat_count = (int)(sizeof(features)/sizeof(features[0]));

        float pp  = 0.5f + 0.5f*sinf(t*1.4f);
        float pp2 = 0.5f + 0.5f*sinf(t*0.9f + 1.1f);

        // ── LEFT COLUMN: version card ─────────────────────────────────────────
        int contentTop = py2 + 66;
        int colGap = 28;
        int colL_w = 280;
        int colL_x = px2 + 24;

        int vcard_h = 180;
        Rectangle vcard = { (float)colL_x, (float)contentTop, (float)colL_w, (float)vcard_h };

        for (int g = 6; g >= 1; g--) {
            float gf = (float)g / 6.0f * pp2;
            DrawRectangleRounded(
                (Rectangle){vcard.x-g*2, vcard.y-g*2, vcard.width+g*4, vcard.height+g*4},
                0.18f, 8,
                (Color){ac.r, ac.g, ac.b, (uchar)(8.0f*gf)});
        }
        DrawRectangleRounded(vcard, 0.14f, 8, (Color){22,14,38,248});
        DrawRectangleRounded(
            (Rectangle){vcard.x+1, vcard.y+1, vcard.width-2, 38},
            0.14f, 8, (Color){ac.r/6, ac.g/6, ac.b/5, 255});
        Color vbdr = { (uchar)(ac.r*(0.6f+0.4f*pp)), (uchar)(ac.g*(0.6f+0.4f*pp)),
                       (uchar)(ac.b*(0.6f+0.4f*pp)), 220 };
        DrawRectangleRoundedLines(vcard, 0.14f, 8, vbdr);
        DrawCornerAccents(vcard, (Color){ac.r,ac.g,ac.b,180}, 14);

        const char *nxt = "NEXT UPDATE";
        int nxtw = MeasureText(nxt, 13);
        DrawText(nxt, (int)(vcard.x + vcard.width/2 - nxtw/2), (int)(vcard.y+12), 13,
                 (Color){ac.r,ac.g,ac.b,200});

        int vfs = 46;
        int vw  = MeasureText(ver_str, vfs);
        int vx  = (int)(vcard.x + vcard.width/2 - vw/2);
        int vy  = (int)(vcard.y + 46);
        for (int g=5;g>=1;g--) {
            uchar ga = (uchar)(13*g*(0.5f+0.5f*pp));
            DrawText(ver_str, vx-g, vy-g, vfs, (Color){ac.r,ac.g,ac.b,ga});
            DrawText(ver_str, vx+g, vy+g, vfs, (Color){ac.r,ac.g,ac.b,ga});
        }
        DrawText(ver_str, vx+2, vy+3, vfs, (Color){0,0,0,130});
        DrawText(ver_str, vx,   vy,   vfs, (Color){235,225,255,255});

        DrawLine((int)(vcard.x+20), (int)(vcard.y+105), (int)(vcard.x+vcard.width-20),
                 (int)(vcard.y+105), (Color){ac.r,ac.g,ac.b,55});

        int unfs = 15;
        int unw  = MeasureText(upd_name, unfs);
        DrawText(upd_name,
                 (int)(vcard.x + vcard.width/2 - unw/2),
                 (int)(vcard.y + 116), unfs,
                 (Color){(uchar)CLAMP255(ac.r+50),(uchar)CLAMP255(ac.g+55),(uchar)CLAMP255(ac.b+30),230});

        float pdp = 0.5f+0.5f*sinf(t*2.5f);
        int dot_y = (int)(vcard.y + 148);
        int dot_cx = (int)(vcard.x + vcard.width/2);
        for (int d=-1;d<=1;d++) {
            float df = (d==0) ? pdp : (0.3f+0.2f*sinf(t*2.5f + d*1.1f));
            DrawCircle(dot_cx + d*18, dot_y, (int)(4.0f*df)+2,
                       (Color){ac.r,ac.g,ac.b,(uchar)(60+140*df)});
            DrawCircle(dot_cx + d*18, dot_y, (int)(2.0f*df)+1,
                       (Color){230,215,255,(uchar)(120+120*df)});
        }

        // ── LEFT COLUMN: ETA badge below version card ─────────────────────────
        int eta_y = contentTop + vcard_h + 16;
        Rectangle etacard = { (float)colL_x, (float)eta_y, (float)colL_w, 52 };
        DrawRectangleRounded(etacard, 0.28f, 8, (Color){16,10,28,220});
        DrawRectangleRoundedLines(etacard, 0.28f, 8, (Color){ac.r,ac.g,ac.b,60});
        const char *eta_lbl = "STATUS";
        const char *eta_val = "IN DEVELOPMENT";
        int elw = MeasureText(eta_lbl, 11);
        int evw = MeasureText(eta_val, 14);
        DrawText(eta_lbl, (int)(etacard.x+etacard.width/2-elw/2), (int)(etacard.y+7),
                 11, (Color){120,110,155,200});
        DrawText(eta_val, (int)(etacard.x+etacard.width/2-evw/2), (int)(etacard.y+24),
                 14, (Color){(uchar)(100+60*pp),(uchar)(200+40*pp2),(uchar)(130+60*pp),255});

        for (int i=0;i<5;i++) {
            float a = (float)i/5*2*PI + t*0.55f;
            int orx = (int)(etacard.x + etacard.width/2 + cosf(a)*148);
            int ory = (int)(eta_y + 26 + sinf(a)*22);
            float pf = 0.5f+0.5f*sinf(t*2.0f+i*1.3f);
            DrawCircle(orx, ory, (int)(2.0f+pf), (Color){ac.r,ac.g,ac.b,(uchar)(30+50*pf)});
        }

        int colR_x = colL_x + colL_w + colGap;
        int colR_w = px2 + pw - colR_x - 24;
        int listTop = contentTop;

        const char *feat_hdr = "NEW FEATURES";
        DrawText(feat_hdr, colR_x, listTop, 14, (Color){ac.r,ac.g,ac.b,180});
        DrawLine(colR_x, listTop+18, colR_x+colR_w, listTop+18,
                 (Color){ac.r,ac.g,ac.b,45});

        int rowH2  = 44;
        int listY0 = listTop + 26;

        for (int i = 0; i < feat_count; i++) {
            int ry = listY0 + i * rowH2;
            if (ry + rowH2 > py2 + ph - 70) break;

            float rpp = 0.5f + 0.5f*sinf(t*1.3f + i*0.42f);

            Color rowbg = (i%2==0) ? (Color){20,14,34,150} : (Color){14,10,24,80};
            DrawRectangleRounded(
                (Rectangle){(float)colR_x, (float)(ry-4), (float)colR_w, (float)(rowH2-4)},
                0.10f, 6, rowbg);

            Color barCol = { (uchar)(ac.r*(0.5f+0.5f*rpp)),
                             (uchar)(ac.g*(0.5f+0.5f*rpp)),
                             (uchar)(ac.b*(0.5f+0.5f*rpp)), 255 };
            DrawRectangleRounded(
                (Rectangle){(float)colR_x, (float)(ry-4), 3.0f, (float)(rowH2-4)},
                1.0f, 4, barCol);

            int bx2 = colR_x + 14;
            int by2 = ry + rowH2/2 - 6;
            DrawRectanglePro(
                (Rectangle){(float)bx2, (float)by2, 8.0f, 8.0f},
                (Vector2){4.0f, 4.0f}, 45.0f,
                (Color){ac.r, ac.g, ac.b, (uchar)(140 + (int)(80*rpp))});
            DrawRectanglePro(
                (Rectangle){(float)bx2, (float)by2, 4.0f, 4.0f},
                (Vector2){2.0f, 2.0f}, 45.0f,
                (Color){230,215,255,(uchar)(160+80*rpp)});

            const char *full = features[i];
            const char *colon = strchr(full, ':');
            if (colon) {
                int tagLen = (int)(colon - full + 1);
                char tag[64]; if (tagLen > 63) tagLen=63;
                strncpy(tag, full, tagLen); tag[tagLen]='\0';
                int tagW = MeasureText(tag, 17);
                DrawText(tag, bx2+16, ry+2, 17,
                         (Color){(uchar)CLAMP255(ac.r+55),(uchar)CLAMP255(ac.g+55),
                                 (uchar)CLAMP255(ac.b+30), 255});
                DrawText(colon+1, bx2+16+tagW+4, ry+2, 17,
                         (Color){175,168,205,215});
            } else {
                DrawText(full, bx2+16, ry+2, 17, (Color){175,168,205,215});
            }

            if (i == 0) {
                char badge[16]; snprintf(badge,16,"%d FEATURES", feat_count);
                int bw = MeasureText(badge, 12);
                DrawRectangleRounded(
                    (Rectangle){(float)(colR_x+colR_w-bw-18),(float)(listTop-4),(float)(bw+16),22},
                    0.5f, 6, (Color){ac.r/5,ac.g/5,ac.b/4,200});
                DrawRectangleRoundedLines(
                    (Rectangle){(float)(colR_x+colR_w-bw-18),(float)(listTop-4),(float)(bw+16),22},
                    0.5f, 6, (Color){ac.r,ac.g,ac.b,100});
                DrawText(badge, colR_x+colR_w-bw-10, listTop, 12,
                         (Color){ac.r,ac.g,ac.b,200});
            }
        }

        return;
    }

    // ── SETTINGS ─────────────────────────────────────────────────────────────
    if (menuSub == 6) {
        Color ac = (Color){180,160,255,255};
        Rectangle backR = DrawSubPanel("SETTINGS", ac, t, sw, sh);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(GetMousePosition(), backR))
            { menuSub = 0; return; }

        int margin2=32;
        int px2=margin2, py2=margin2;
        int contentX = px2 + 40;
        int contentTop = py2 + 72;

        // ── Enemies toggle ────────────────────────────────────────────────────
        {
            int tW = 320, tH = 60;
            Rectangle toggleR = { (float)contentX, (float)contentTop, (float)tW, (float)tH };
            DrawRectangleRounded(toggleR, 0.18f, 8, (Color){16,13,26,200});
            DrawRectangleRoundedLines(toggleR, 0.18f, 8, (Color){ac.r,ac.g,ac.b,80});
            DrawText("ENEMIES", contentX+12, contentTop+10, 17, ac);
            DrawText("Enable or disable enemies for the next game",
                     contentX+12, contentTop+33, 13, (Color){130,125,165,200});

            int btnX = contentX + tW + 20;
            Rectangle btnR2 = { (float)btnX, (float)(contentTop+10), 110.0f, 40.0f };
            bool hovB = CheckCollisionPointRec(GetMousePosition(), btnR2);
            Color btnC = eneon ? (Color){50,12,12,220} : (Color){12,40,20,200};
            Color btnBdr = eneon ? (Color){220,65,65,200} : (Color){65,200,65,180};
            DrawRectangleRounded(btnR2, 0.3f, 8, btnC);
            DrawRectangleRoundedLines(btnR2, 0.3f, 8, btnBdr);
            const char *etxt = eneon ? "ON" : "OFF";
            int etw = MeasureText(etxt, 18);
            DrawText(etxt, btnX+55-etw/2, contentTop+21, 18,
                     eneon ? (Color){255,100,100,255} : (Color){80,220,80,255});
            if (hovB && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) eneon = !eneon;
        }

        DrawText("MORE COMING SOON...",
                 contentX, contentTop + 80, 14, (Color){110,105,150,190});
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

    // ── TOP-LEFT ICON BUTTONS ─────────────────────────────────────────────────
    {
        Rectangle settR, futR;
        GetMenuIconRects(&settR, &futR);
        Vector2 mp2 = GetMousePosition();

        // Settings gear icon
        {
            bool hov = CheckCollisionPointRec(mp2, settR);
            Color ic  = hov ? (Color){230,220,255,255} : (Color){160,148,210,200};
            Color bgc = hov ? (Color){30,24,50,230}    : (Color){16,12,28,190};
            DrawRectangleRounded(settR, 0.18f, 8, bgc);
            DrawRectangleRoundedLines(settR, 0.18f, 8,
                hov ? (Color){190,170,255,210} : (Color){95,85,140,150});

            int gcx2 = (int)(settR.x + settR.width/2);
            int gcy2 = (int)(settR.y + settR.height/2);
            // 8 gear teeth (static)
            for (int k=0; k<8; k++) {
                float a = k * PI/4.0f;
                float tx3 = gcx2 + cosf(a)*21.0f;
                float ty4 = gcy2 + sinf(a)*21.0f;
                DrawRectanglePro((Rectangle){tx3, ty4, 11.0f, 8.0f},
                                 (Vector2){5.5f, 4.0f}, a*180.0f/PI, ic);
            }
            DrawCircle(gcx2, gcy2, 18, ic);
            DrawCircle(gcx2, gcy2, 13, bgc);
            DrawCircle(gcx2, gcy2,  7, ic);
            DrawCircle(gcx2, gcy2,  4, bgc);
        }

        // Future Updates rocket icon
        {
            bool hov = CheckCollisionPointRec(mp2, futR);
            Color ic  = hov ? (Color){225,175,255,255} : (Color){160,95,220,200};
            Color bgc = hov ? (Color){36,14,52,230}    : (Color){20,9,32,190};
            DrawRectangleRounded(futR, 0.18f, 8, bgc);
            DrawRectangleRoundedLines(futR, 0.18f, 8,
                hov ? (Color){210,130,255,210} : (Color){110,55,165,150});

            int rcx2 = (int)(futR.x + futR.width/2);
            int rcy2 = (int)(futR.y + futR.height/2);
            // Nose cone
            DrawTriangle((Vector2){(float)rcx2,      (float)(rcy2-26)},
                         (Vector2){(float)(rcx2-9),  (float)(rcy2-8)},
                         (Vector2){(float)(rcx2+9),  (float)(rcy2-8)}, ic);
            // Body
            DrawRectangle(rcx2-9, rcy2-8, 18, 22, ic);
            // Window porthole
            DrawCircle(rcx2, rcy2+2, 5, bgc);
            DrawCircleLines(rcx2, rcy2+2, 5, (Color){230,215,255,220});
            // Left fin
            DrawTriangle((Vector2){(float)(rcx2-9),  (float)(rcy2+8)},
                         (Vector2){(float)(rcx2-17), (float)(rcy2+14)},
                         (Vector2){(float)(rcx2-9),  (float)(rcy2+14)}, ic);
            // Right fin
            DrawTriangle((Vector2){(float)(rcx2+9),  (float)(rcy2+8)},
                         (Vector2){(float)(rcx2+9),  (float)(rcy2+14)},
                         (Vector2){(float)(rcx2+17), (float)(rcy2+14)}, ic);
            // Exhaust (static)
            DrawCircle(rcx2, rcy2+18, 6, (Color){255,140,50,200});
            DrawCircle(rcx2, rcy2+23, 4, (Color){255,240,100,210});
        }
    }

    // ── 4 NAV BUTTONS ────────────────────────────────────────────────────────
    int M = 14;                         
    int titleBottom = 152;              

    int cW = (int)(sw * 0.28f);        
               
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
    DrawNavCard(btnBR, "ACHIEVEMENTS", (Color){185,85,255,255}, t, 2.7f);

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

    {
        const char *ver = "v1.4";
        int vw2 = MeasureText(ver, 13);
        int vrx = sw - vw2 - 20, vry = 10;
        DrawRectangleRounded((Rectangle){(float)(vrx-6),(float)(vry-4),(float)(vw2+12),22},
                             0.4f, 6, (Color){18,14,30,210});
        DrawRectangleRoundedLines((Rectangle){(float)(vrx-6),(float)(vry-4),(float)(vw2+12),22},
                                  0.4f, 6, (Color){65,60,90,160});
        DrawText(ver, vrx, vry, 13, (Color){130,125,165,220});
    }
}

// ─── DRAW OVERLAY (death / next-round) ────────────────────────────────────────
static void DrawOverlay(float t) {
    (void)t;
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
                if (menuSub != 0) { if (menuSub == 2) statScroll = 0; if (menuSub == 3) upgScroll = 0; menuSub = 0; break; }
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
                    if (CheckCollisionPointRec(mp, backR)) { if (menuSub == 2) statScroll = 0; if (menuSub == 3) upgScroll = 0; menuSub=0; break; }
                    Rectangle panelR={(float)px2,(float)py2,(float)pw2,(float)ph2};
                    if (!CheckCollisionPointRec(mp, panelR)) { if (menuSub == 2) statScroll = 0; if (menuSub == 3) upgScroll = 0; menuSub=0; break; }
                } else {
                    // Icon buttons (top-left)
                    {
                        Rectangle settR2, futR2;
                        GetMenuIconRects(&settR2, &futR2);
                        if (CheckCollisionPointRec(mp, settR2)) { menuSub = 6; break; }
                        if (CheckCollisionPointRec(mp, futR2))  { menuSub = 5; break; }
                    }
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
            if (pshield && pshield_dur < 1e8f) {
                pshield_dur -= dt;
                if (pshield_dur <= 0) { pshield = false; pshield_dur = 0; }
            }

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
              Color pc = pdead ? CPD : GetSkinColor(skin_equipped);

              if (pwlk > 0) {
                  float pp = 0.5f + 0.5f*sinf(t*6.0f);
                  DrawCircle(cx, cy, CELL/2+8, (Color){180,80,255,(int)(30+30*pp)});
                  DrawCircleLines(cx, cy, CELL/2+8, (Color){180,80,255,(int)(120+80*pp)});
              }
              if (pshield) {
                  float pp = 0.5f + 0.5f*sinf(t*5.0f);
                  DrawCircle(cx, cy, CELL/2+6, (Color){80,150,255,(int)(38+28*pp)});
                  DrawCircleLines(cx, cy, CELL/2+6,(Color){80,150,255,(int)(155+65*pp)});
              }
              if (pshield_perk) {
                  float pp = 0.5f + 0.5f*sinf(t*3.5f);
                  int ar = CELL/2+3;
                  DrawCircle(cx, cy, ar, (Color){40,100,220,(int)(18+14*pp)});
                  DrawCircleLines(cx, cy, ar, (Color){100,180,255,(int)(90+50*pp)});
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
              if (skin_equipped >= 27 && skin_equipped <= 29)
                  DrawEpicBack(skin_equipped, (float)cx, (float)cy, (float)(CELL/2-4), t);
              else if (skin_equipped >= 22 && skin_equipped <= 26)
                  DrawRareBack(skin_equipped, (float)cx, (float)cy, (float)(CELL/2-4), t);

              if (skin_equipped == 28) {
                  DrawSkull((float)cx, (float)cy, (float)(CELL/2-4), t);
              } else {
                  DrawCircle(cx, cy, CELL/2-4, pc);

                  if (skin_equipped == 27 || skin_equipped == 29) {
                      DrawEpicFace(skin_equipped, (float)cx, (float)cy, (float)(CELL/2-4), t);
                  } else if (skin_equipped >= 22 && skin_equipped <= 26) {
                      DrawCircle(cx-2, cy-3, CELL/5,
                                 (Color){CLAMP255(pc.r+55),CLAMP255(pc.g+55),CLAMP255(pc.b+55),90});
                      DrawRareFace(skin_equipped, (float)cx, (float)cy, (float)(CELL/2-4), t);
                  } else {
                      DrawCircle(cx-2, cy-3, CELL/4,
                                 (Color){CLAMP255(pc.r+55),CLAMP255(pc.g+55),CLAMP255(pc.b+55),
                                         (uchar)(pc.a < 100 ? pc.a/2 : 120)});
                      uchar eyeA = (pc.a < 100) ? pc.a : 255;
                      DrawCircle(cx-4, cy-2, 2, (Color){10,10,22,eyeA});
                      DrawCircle(cx+4, cy-2, 2, (Color){10,10,22,eyeA});
                  }
              }
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