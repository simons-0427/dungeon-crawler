#ifndef CONSTANTS_H
#define CONSTANTS_H

// ─── SCREEN / GRID / LIMITS ───────────────────────────────────────────────────
#define SW        1280
#define SH        720
#define CELL      32
#define MAP_ROWS  96
#define MAP_COLS  80
#define MAX_ENE   32       // shared storage (boss adds still capped via BOSS_ADD_CAP)
#define MAX_NORMAL_ENE 8   // normal-mode spawn cap; Coin Rush uses more
#define MAX_COI   96       // Coin Rush packs the floor; normal modes use less
#define MAX_ITM   4
#define MAX_PRT   2
#define MAX_PAR   256
#define INV_SZ    1
#define KB_MAX_BINDS 4   // extra keys the player can add per action
// Mouse buttons live in the same bind slots as keys: value = -(button+1).
// 0 stays "empty". Keyboard keys are always > 0.
#define BIND_FROM_MOUSE(btn)  (-((int)(btn) + 1))
#define MOUSE_FROM_BIND(k)    (-(k) - 1)
#define BIND_IS_MOUSE(k)      ((k) < 0)
#define BIND_MOUSE_MAX_BTN    6   // LEFT..BACK in raylib

// Number of selectable skins (used to size skin_owned[] and the shop grid).
// IDs stay append-only for save compatibility; the shop assigns rarity and
// purchase/achievement rules explicitly per category.
#define SKIN_COUNT 52

// Soundtrack slots. Existing tracks are 0..3; added slots are 4..8.
#define MUSIC_TRACK_COUNT       9
#define MUSIC_DEFAULT_TRACK     4
#define MUSIC_OVERACHIEVER_TRACK 7
#define MUSIC_NEON_PULSE_TRACK  8
#define MUSIC_DEFAULT_MENU_MASK ((1u << 0) | (1u << MUSIC_DEFAULT_TRACK))
#define MUSIC_DEFAULT_GAMEPLAY_MASK ((1u << 1) | (1u << 2) | (1u << 3))

// Persistence files (saved under saves in the working directory).
// #ifndef-guarded so headless test builds can redirect them (-DSTATS_FILE=...)
// and can never clobber a real save, no matter which directory they run from.
#ifdef DEBUG_ROOMS
#define SAVE_FOLDER "Debug/saves/"
#else
#define SAVE_FOLDER "saves/"
#endif
#ifndef STATS_FILE
#define STATS_FILE     SAVE_FOLDER "dungeons_stats.txt"
#endif
#ifndef SHOP_FILE
#define SHOP_FILE      SAVE_FOLDER "dungeons_shop.txt"
#endif
#ifndef SETTINGS_FILE
#define SETTINGS_FILE  SAVE_FOLDER "dungeons_settings.txt"
#endif
#ifndef CHALLENGE_DATA_FILE
#define CHALLENGE_DATA_FILE SAVE_FOLDER "dungeons_challenges.txt"
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
// Runtime-swappable (colorblind modes). Storage lives in globals.c.
#define CB     pal_b
#define CF     pal_f
#define CW     pal_w
#define CWH    pal_wh
#define CWD    pal_wd
#define CTRAP  pal_trap
#define CTRES  pal_tres
#define CPL    pal_pl
#define CPD    pal_pd
#define CCOIN  pal_coin
#define CPORT  pal_port
#define CENE   pal_ene
#define CSHD   pal_shd
#define CWLK   pal_wlk
#define CSWD   pal_swd
#define CBOSS  pal_boss
#define CPLATE pal_plate
#define CCANN  pal_cann
#define CFIRE  pal_fire

#define CB_OFF     0
#define CB_REDGREEN 1   // deuteranopia + protanopia
#define CB_BLUEYELLOW 2 // tritanopia
#define CB_HIGHCONTRAST 3

#endif // CONSTANTS_H
