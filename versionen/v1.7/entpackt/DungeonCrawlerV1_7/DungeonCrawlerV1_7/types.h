#ifndef TYPES_H
#define TYPES_H

#include "raylib.h"   // Color (used by Par)

// ─── PRIMITIVE ALIASES ────────────────────────────────────────────────────────
typedef unsigned char uchar;

// ─── ENUMS ────────────────────────────────────────────────────────────────────
// Item kinds. IK_NONE is the "no item" sentinel and also the count of real kinds.
// IK_GLD (Gold Touch) is Coin-Rush only — never rolled by random floor spawns.
typedef enum { IK_SPD=0, IK_SHD, IK_BMB, IK_WLK, IK_SWD, IK_GLD, IK_NONE } IKind;

// Top-level game state machine.
typedef enum { GS_MENU, GS_PLAY, GS_PAUSE, GS_DEAD, GS_NEXT } GState;

// Remappable gameplay actions. Extra keys can be added per action (KB_MAX_BINDS).
// The debug build ignores custom binds and keeps the hardcoded defaults.
typedef enum {
    ACT_MOVE_UP = 0,
    ACT_MOVE_DOWN,
    ACT_MOVE_LEFT,
    ACT_MOVE_RIGHT,
    ACT_USE_1,
    ACT_USE_2,
    ACT_USE_3,
    ACT_USE_4,
    ACT_USE_5,
    ACT_USE_SELECTED,
    ACT_PAUSE,
    ACT_FULLSCREEN,
    ACT_CONFIRM,        // start / retry / next round
    ACT_MENU_NEXT,      // tab through main-menu widgets
    ACT_MENU_LEFT,
    ACT_MENU_RIGHT,
    ACT_MENU_UP,
    ACT_MENU_DOWN,
    ACT_MENU_BACK,      // death / pause / submenu → main menu (default Q)
    ACT_COUNT
} ActionId;

// Level type of the current round (V1.6 "Chambers of Chaos").
// RK_BOSS every 10th round; challenge kinds (incl. Coin Rush) roll from round 6 on.
typedef enum {
    RK_NORMAL=0,
    RK_BOSS,
    RK_TRAP,
    RK_DARK,
    RK_LAVA,
    RK_COIN_RUSH,
    RK_ARTIFACT
} RoomKind;

// ─── WORLD OBJECTS ────────────────────────────────────────────────────────────
typedef struct { float x,y,vx,vy,life,ml; Color c; float sz; } Par;     // particle
typedef struct { float x,y,mt,cd; bool on; } Ene;                        // enemy
typedef struct { int x,y; bool on; } CoinObj;                           // coin
typedef struct { int x,y; bool on; IKind k; } ItmObj;                   // item pickup
typedef struct { int x,y; bool on; } PrtObj;                            // portal

// Boss behaviour states: fast stalking, then a telegraphed attack — either a
// straight-line melee dash (close range) or a fire breath (long range) —
// then a short exhausted pause (punish window).
typedef enum { BS_STALK=0, BS_WINDUP, BS_DASH, BS_TIRED, BS_FIRE_WINDUP, BS_FIRE } BState;

// Boss (RK_BOSS rounds). x,y = top-left tile of the 2x2 footprint.
typedef struct {
    int   x, y;
    float mt;                  // step timer (stalk + dash pacing)
    int   hp, maxhp;           // maxhp = prime from BOSS_HP[]
    float inv;                 // remaining invulnerability seconds after a hit
    int   state;               // BState
    float stT;                 // windup / tired / fire countdown
    float chargeT;             // stalk seconds until the next charge
    int   dashdx, dashdy;      // locked full aim vector (dash and fire breath)
    int   aimx, aimy;          // 0/1 footprint offset where the fire ray starts
    int   dashn;               // completed major-axis steps along that vector
    float addT;                // add-summon interval timer
    float teleT;               // telegraph countdown for pending adds (<=0 = none)
    int   telex[3], teley[3], telen;   // pending add spawn tiles (tier 4+: up to 3)
    int   steps;               // moves since the last trample trap
    int   chain;               // dashes already chained this charge (tier 3+)
    bool  on;
} Boss;

#endif // TYPES_H
