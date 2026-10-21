#ifndef GLOBALS_H
#define GLOBALS_H

#include "raylib.h"     // Camera2D
#include "constants.h"  // array-size macros
#include "types.h"      // GState, IKind, Par, Ene, ...

// ─── ALL MUTABLE GAME STATE ───────────────────────────────────────────────────
// Defined exactly once in globals.c; declared extern everywhere else.

// Top-level state + map
extern GState gs;
extern char   map[MAP_ROWS][MAP_COLS];
extern int    mw, mh;

// Player
extern int    px, py;
extern float  pmt, pmd;
extern bool   pdead, pshield, pshield_perk;
extern float  pshield_dur;        // remaining seconds; >=1e8 = unlimited
extern float  pspd;
extern float  pwlk;
extern IKind  pinv[INV_SZ + 4];   // +4 for max perk_slots
extern int    pinvn;

// World objects
extern Ene     enes[MAX_ENE];
extern CoinObj coins[MAX_COI];
extern ItmObj  itms[MAX_ITM];
extern PrtObj  prts[MAX_PRT];
extern Par     pars[MAX_PAR];

// Per-game stats / flags
extern int   roundn, totcoi, tottrs;
extern bool  eneon, wasportal;
extern float sttime;
extern Camera2D cam;

// Highscores & lifetime totals (persisted to STATS_FILE)
extern int hs_round, hs_trs, hs_coi, hs_kills;
extern int hs_coi_round;                   // most coins collected in a single round
extern int global_totcoi, global_totkills, global_tottrs, global_rounds, global_games_played;
extern int coin_balance, kills_game;
extern int coins_round;                    // coins this round (resets per round, not persisted)

// Extended lifetime stats (persisted to STATS_FILE)
extern int global_deaths_enemy, global_deaths_trap;
extern int global_kills_sword, global_kills_shield;
extern int global_portals;
extern int global_ghostwalk_walls;
extern int global_inv_full;
extern int global_use_spd, global_use_shd, global_use_bmb, global_use_wlk, global_use_swd;

// Achievement reward claim flags (persisted to SHOP_FILE)
extern int ach_claimed[23][6]; // [achievement_idx][tier 0..5], 1 = reward collected

// Shop state (persisted to SHOP_FILE)
extern int skin_owned[SKIN_COUNT];
extern int skin_equipped;
extern int spd_level, shd_level, wlk_level, swd_level, bmb_level;
extern int perk_slots, perk_magnet, perk_shield2;

// Menu navigation state shared between game_ui.c and main.c
extern int menuSub;     // 0 = main menu, 1..6 = sub-screens
extern int statScroll;  // STATS scroll offset
extern int upgScroll;   // UPGRADES scroll offset

// User settings (persisted to SETTINGS_FILE)
// Audio — infrastructure only; no audio engine yet.
extern float vol_master;    // 0.0–1.0, master volume
extern float vol_music;     // 0.0–1.0, music volume
extern float vol_sfx;       // 0.0–1.0, SFX volume
extern bool  audio_mute;    // mute all audio

// Gameplay
extern int   par_density;   // 0=Low  1=Med  2=High particle burst count
extern float cam_smooth;    // camera follow speed multiplier (1–20, default 9)
extern int   mobile_control_mode; // MOBILE_CONTROL_SPLIT/FLOAT/DPAD

// Graphics / UI
extern bool  show_fps;      // overlay FPS counter
extern float hud_opacity;   // HUD background alpha scale (0.2–1.0)

#endif // GLOBALS_H
