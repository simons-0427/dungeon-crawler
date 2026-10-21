#ifndef GAME_ITEMS_H
#define GAME_ITEMS_H

#include "raylib.h"   // Color

// Shared item descriptors, indexed by IKind (IK_SPD..IK_SWD).
// Coins / items / portals themselves live as arrays in globals.h
// (coins[], itms[], prts[]); their collision & pickup logic is handled
// inline by TryMove() (game_player.c) and GenRound() (game_map.c).
extern const char  *IK_NAME[];   // short 3-letter labels
extern const Color  IK_COL[];    // per-kind accent colours

#endif // GAME_ITEMS_H
