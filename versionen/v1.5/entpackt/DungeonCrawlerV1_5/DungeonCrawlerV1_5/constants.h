#ifndef CONSTANTS_H
#define CONSTANTS_H

// ─── SCREEN / GRID / LIMITS ───────────────────────────────────────────────────
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

// Number of selectable skins (used to size skin_owned[] and the shop grid).
// 0-29 = purchasable (Common/Uncommon/Rare/Epic), 30-35 = Legendary/achievement-
// locked skins (Chronovore/Deep Cartographer/Midas/Blade Legend/Constellation/
// Ascendant), auto-unlocked at achievement Tier 6 — see CheckAchievementSkinUnlocks.
#define SKIN_COUNT 36

// Persistence files (saved next to the executable / working directory).
#define STATS_FILE     "dungeons_stats.txt"
#define SHOP_FILE      "dungeons_shop.txt"
#define SETTINGS_FILE  "dungeons_settings.txt"

// Clamp an int expression to a valid 0..255 colour-channel byte.
#define CLAMP255(v) ((unsigned char)((v)<0?0:((v)>255?255:(v))))

// ─── COLOUR PALETTE ───────────────────────────────────────────────────────────
// (Compound literals — only expanded inside .c files that also include raylib.h.)
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

#endif // CONSTANTS_H
