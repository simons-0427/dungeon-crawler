#ifndef TYPES_H
#define TYPES_H

#include "raylib.h"   // Color (used by Par)

// ─── PRIMITIVE ALIASES ────────────────────────────────────────────────────────
typedef unsigned char uchar;

// ─── ENUMS ────────────────────────────────────────────────────────────────────
// Item kinds. IK_NONE is the "no item" sentinel and also the count of real kinds.
typedef enum { IK_SPD=0, IK_SHD, IK_BMB, IK_WLK, IK_SWD, IK_NONE } IKind;

// Top-level game state machine.
typedef enum { GS_MENU, GS_PLAY, GS_PAUSE, GS_DEAD, GS_NEXT } GState;

// ─── WORLD OBJECTS ────────────────────────────────────────────────────────────
typedef struct { float x,y,vx,vy,life,ml; Color c; float sz; } Par;     // particle
typedef struct { float x,y,mt,cd; bool on; } Ene;                        // enemy
typedef struct { int x,y; bool on; } CoinObj;                           // coin
typedef struct { int x,y; bool on; IKind k; } ItmObj;                   // item pickup
typedef struct { int x,y; bool on; } PrtObj;                            // portal

#endif // TYPES_H
