// game_items.c — shared item descriptor tables.
#include "game_items.h"

const char *IK_NAME[] = { "SPD", "SHD", "BMB", "WLK", "SWD", "GLD" };

const Color IK_COL[] = {
    {  80,220, 80,255 },   // SPD
    {  80,150,255,255 },   // SHD
    { 255,140, 40,255 },   // BMB
    { 180, 80,255,255 },   // WLK
    { 255, 60,120,255 },   // SWD
    { 255,200, 40,255 }    // GLD (Gold Touch)
};
