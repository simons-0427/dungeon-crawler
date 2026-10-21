#ifndef GLOBALS_H
#define GLOBALS_H

#include "raylib.h"     // Camera2D
#include "constants.h"  // array-size macros
#include "types.h"      // GState, IKind, Par, Ene, ...
#include "advancements.h" // ACH_COUNT

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
extern float  pgold;              // Gold Touch remaining seconds (0 = inactive)
extern float  pfall;              // trapdoor fall timer (>0 = falling into hatch)
extern IKind  pinv[INV_SZ + 4];   // +4 for max perk_slots
extern int    pinvn;
extern int    inv_sel;            // selected inventory slot, -1 = none

// World objects
extern Ene     enes[MAX_ENE];
extern CoinObj coins[MAX_COI];
extern ItmObj  itms[MAX_ITM];
extern PrtObj  prts[MAX_PRT];
extern Par     pars[MAX_PAR];

// Per-game stats / flags
extern int   roundn, totcoi, tottrs;
extern bool  eneon, wasportal;
extern bool  coinRush;     // true iff roomKind == RK_COIN_RUSH (dense spawns, triple coins)
extern float sttime;
extern Camera2D cam;

// Level type of the current round + boss state (set in GenRound)
extern RoomKind roomKind;
extern Boss     boss;

// Challenge-room state (V1.6)
extern int   normn;        // completed NORMAL rounds this game — drives wall/trap density
extern int   forceRoom;    // -1 = auto; else RoomKind forced for the next GenRound (debug/tests)
extern float trapPhase;    // arrow-lane volley clock (RK_TRAP, ticks in GS_PLAY)
extern float lavaTimer;    // lava spread accumulator (RK_LAVA)
extern float lavaTick;     // seconds per lava spread step (set by GenLavaRound)
extern float lavaElapsed;  // active seconds spent in the current lava room
extern float bannerT;      // round-start room-type banner countdown

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
extern int global_use_gld;
extern int global_bosses_killed, global_boss_dmg;
extern int global_deaths_lava;
extern int global_deaths_boss;
extern int global_deaths_trapdoor;
extern int global_rooms_trap, global_rooms_dark, global_rooms_lava, global_rooms_coinrush;
extern int global_cannon_hits;
extern int hs_unbeaten;   // Unbeaten Champion rank 0..6 (round 5 + bosses L10..L50)
extern unsigned lockedObjectDiscoveredMask; // designs first encountered across runs

// Achievement reward claim flags (persisted to SHOP_FILE)
// The original 32 indices remain stable; selected proposals start at index 32.
extern int ach_claimed[ACH_COUNT][6]; // [achievement_idx][tier 0..5], 1 = reward collected

// Shop state (persisted to SHOP_FILE)
extern int skin_owned[SKIN_COUNT];
extern int skin_equipped;
extern int spd_level, shd_level, wlk_level, swd_level, bmb_level, gld_level;
extern int perk_slots, perk_magnet, perk_shield2;
extern int music_owned[MUSIC_TRACK_COUNT];

// Menu navigation state shared between game_ui.c and main.c
extern int menuSub;     // 0 = main menu, 1..8 = sub-screens (8 = challenges / seeded runs)
extern int statScroll;  // STATS scroll offset
extern int upgScroll;   // UPGRADES scroll offset

// User settings (persisted to SETTINGS_FILE)
// Audio track selections are bitmasks: bit 0..8 maps to the soundtrack cards.
// A zero mask means NO MUSIC; multiple bits create a track rotation.
extern float vol_master;    // 0.0–1.0, master volume
extern float vol_music;     // 0.0–1.0, music volume
extern float vol_sfx;       // 0.0–1.0, SFX volume
extern bool  audio_mute;    // mute all audio
extern unsigned int music_menu_tracks;     // bitmask; 0 = off
extern unsigned int music_gameplay_tracks; // bitmask; 0 = off

// Gameplay
extern int   par_density;   // 0=Low  1=Med  2=High particle burst count
extern float cam_smooth;    // camera follow speed multiplier (1–20, default 9)

// Graphics / UI
extern bool  show_fps;      // overlay FPS counter
extern float hud_opacity;   // HUD background alpha scale (0.2–1.0)
extern int   colorblind_mode; // CB_OFF / CB_REDGREEN / CB_BLUEYELLOW / CB_HIGHCONTRAST

// Runtime palette (see CTRAP etc. in constants.h)
extern Color pal_b, pal_f, pal_w, pal_wh, pal_wd;
extern Color pal_trap, pal_tres, pal_pl, pal_pd, pal_coin, pal_port;
extern Color pal_ene, pal_shd, pal_wlk, pal_swd, pal_boss, pal_plate, pal_cann, pal_fire;

// Quality-of-life
extern bool mouse_item_select;                 // off by default: no in-game slot pick/highlight
extern bool photosensitivity;                  // off by default: extra burst/strobe throttle
extern bool screen_shake;                      // off by default: camera kick on impacts
extern bool loud_sfx;                          // on by default: full-volume explosions/deaths
extern bool focus_pause;                       // on by default: pause when the window blurs
extern bool skip_play_anim;                    // off by default: skip dungeon-door start anim
extern bool exit_warning;                      // on by default: confirm before leaving a run
extern bool show_seed;                         // off by default: show run seed on the HUD
extern bool show_round_time;                   // show elapsed active time for this round
extern bool show_game_time;                    // show elapsed active time for this run
extern bool show_windows_console;              // show the separate Windows diagnostics console
extern int  console_log_detail;                // 0 errors, 1 warnings, 2 info, 3 debug
extern unsigned run_seed;                      // current run's map seed (0 = none yet)
extern int  keybinds[ACT_COUNT][KB_MAX_BINDS]; // 0 = empty slot; extras are player-added

#endif // GLOBALS_H
