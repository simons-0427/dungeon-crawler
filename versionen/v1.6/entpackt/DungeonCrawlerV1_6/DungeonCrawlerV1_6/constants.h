#ifndef CONSTANTS_H
#define CONSTANTS_H

// ─── SCREEN / GRID / LIMITS ───────────────────────────────────────────────────
#define SW        1280
#define SH        720
#define CELL      32
#define MAP_ROWS  55
#define MAP_COLS  80
#define MAX_ENE   32       // shared storage (boss adds still capped via BOSS_ADD_CAP)
#define MAX_NORMAL_ENE 8   // normal-mode spawn cap; Coin Rush uses more
#define MAX_COI   96       // Coin Rush packs the floor; normal modes use less
#define MAX_ITM   4
#define MAX_PRT   2
#define MAX_PAR   256
#define INV_SZ    1

// Number of selectable skins (used to size skin_owned[] and the shop grid).
// IDs stay append-only for save compatibility; the shop assigns rarity and
// purchase/achievement rules explicitly per category.
#define SKIN_COUNT 43

// Soundtrack slots. Existing tracks are 0..3; added slots are 4..8.
#define MUSIC_TRACK_COUNT       9
#define MUSIC_DEFAULT_TRACK     4
#define MUSIC_OVERACHIEVER_TRACK 7
#define MUSIC_NEON_PULSE_TRACK  8
#define MUSIC_DEFAULT_MENU_MASK ((1u << 0) | (1u << MUSIC_DEFAULT_TRACK))
#define MUSIC_DEFAULT_GAMEPLAY_MASK ((1u << 1) | (1u << 2) | (1u << 3))

// Persistence files (saved next to the executable / working directory).
// #ifndef-guarded so headless test builds can redirect them (-DSTATS_FILE=...)
// and can never clobber a real save, no matter which directory they run from.
#ifndef STATS_FILE
#define STATS_FILE     "dungeons_stats.txt"
#endif
#ifndef SHOP_FILE
#define SHOP_FILE      "dungeons_shop.txt"
#endif
#ifndef SETTINGS_FILE
#define SETTINGS_FILE  "dungeons_settings.txt"
#endif

// ─── BOSS TUNING (RK_BOSS rounds; see game_boss.c) ────────────────────────────
// These are the TIER-1 baseline values. Higher boss tiers (rounds 20/30/40/50+)
// scale tempo and unlock abilities via the TIERS[] table in game_boss.c.
#define BOSS_STALK_CD      0.42f   // seconds per step while stalking (faster still)
#define BOSS_CHARGE_MIN    6.5f    // min stalk seconds before the next charge
#define BOSS_CHARGE_VAR    4       // + 0..VAR random extra seconds (rarer, more dreadful)
#define BOSS_WINDUP_TIME   1.7f    // telegraphed charge-up before the dash (longer stand-still;
                                    // the charge INTERVAL above is unchanged, just this pause)
#define BOSS_DASH_STEP     0.055f  // seconds per tile during the dash (fast!)
#define BOSS_TIRED_TIME    1.0f    // exhausted pause after a dash (punish window)
#define BOSS_INV_TIME      1.0f    // invulnerability after taking a hit
#define BOSS_ADD_INTERVAL  10.0f    // seconds between add summons
#define BOSS_ADD_TELE      1.0f    // telegraph time before adds appear
#define BOSS_ADDS_PER_HP   2       // live add cap = this times the boss's full HP
#define BOSS_ADD_CAP       20      // bosses 4 and 5 stop at 20 simultaneous adds
#define BOSS_TRAMPLE_STEPS 4       // boss leaves a '+' trap every N steps

// Fire breath: chosen instead of a dash when the player is far away, so
// running to the other side of the arena is never fully safe.
#define BOSS_FIRE_MIN_DIST   4     // Chebyshev distance at/above which fire (not dash) is used
#define BOSS_FIRE_WINDUP_TIME 0.8f // telegraph before the beam ignites
#define BOSS_FIRE_DURATION    1.4f // how long the beam stays lethal
#define BOSS_FIRE_RANGE       8    // tier-1 baseline; each later boss adds 2 tiles

// Arena cannons ('C' tiles): step on one to fire a ball at the boss.
// Long reload — each shot is a deliberate, precious punish window.
#define CANNON_CD          30.0f   // reload seconds per cannon
#define CANNON_BALL_SPD    280.0f  // projectile speed in px/s (straight shot, no homing)

// Challenge rooms (RK_TRAP / RK_DARK / RK_LAVA / RK_COIN_RUSH)
// Trap and lava rooms are laid out like normal rooms (scattered walls, no
// enemies); their hazards are overlaid on that organic base.
#define ARROW_PERIOD       2.8f    // arrow lanes ('r'): one synchronized volley per cycle
#define ARROW_WARN         1.0f    // telegraph lead before a volley fires (generous — get out!)
#define ARROW_FIRE         1.0f    // lethal window while the arrows sweep across (only NOW kills)
#define TRAPDOOR_FALL_TIME 0.18f   // fall animation after stepping on 'h' (max ~200ms)
#define DEATH_FLOAT_TIME   0.70f   // non-trapdoor death: rise gently for this long
#define DEATH_FLOAT_PX     14.0f   // pixels of upward drift over DEATH_FLOAT_TIME
#define LAVA_ACCEL_RATE    0.16f  // additional spread steps/second gained each elapsed second
#define LAVA_MIN_TICK      0.05f   // fastest allowed interval between lava spread steps
#define DARK_LIGHT_R       3.0f    // dark rooms: fully lit radius (tiles)
#define DARK_DIM_R         5.5f    // dark rooms: dusk fades to black out here

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
#define CBOSS  ((Color){170,40,200,255})
#define CPLATE ((Color){95,210,185,255})
#define CCANN  ((Color){225,180,90,255})
#define CFIRE  ((Color){255,110,30,255})

#endif // CONSTANTS_H
