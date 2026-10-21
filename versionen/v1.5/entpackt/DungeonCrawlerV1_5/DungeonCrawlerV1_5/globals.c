// globals.c — single definition site for every shared mutable global.
// (Headers only declare them extern; this is where the storage actually lives.)
#include "globals.h"

// Top-level state + map
GState gs = GS_MENU;
char   map[MAP_ROWS][MAP_COLS];
int    mw, mh;

// Player
int    px, py;
float  pmt, pmd;
bool   pdead, pshield, pshield_perk;
float  pshield_dur;
float  pspd;
float  pwlk;
IKind  pinv[INV_SZ + 4];
int    pinvn;

// World objects
Ene     enes[MAX_ENE];
CoinObj coins[MAX_COI];
ItmObj  itms[MAX_ITM];
PrtObj  prts[MAX_PRT];
Par     pars[MAX_PAR];

// Per-game stats / flags
int   roundn, totcoi, tottrs;
bool  eneon, wasportal;
float sttime;
Camera2D cam;

// Highscores & lifetime totals
int hs_round = 0, hs_trs = 0, hs_coi = 0, hs_kills = 0;
int hs_coi_round = 0;
int global_totcoi = 0, global_totkills = 0, global_tottrs = 0, global_rounds = 0, global_games_played = 0;
int coin_balance = 0, kills_game = 0;
int coins_round = 0;

// Extended lifetime stats
int global_deaths_enemy = 0, global_deaths_trap = 0;
int global_kills_sword = 0, global_kills_shield = 0;
int global_portals    = 0;
int global_ghostwalk_walls = 0;
int global_inv_full   = 0;
int global_use_spd = 0, global_use_shd = 0, global_use_bmb = 0, global_use_wlk = 0, global_use_swd = 0;

// Achievement reward claim flags
int ach_claimed[23][6] = {{0}};

// Shop state
int skin_owned[SKIN_COUNT];
int skin_equipped = 0;
int spd_level = 0, shd_level = 0, wlk_level = 0, swd_level = 0, bmb_level = 0;
int perk_slots = 0, perk_magnet = 0, perk_shield2 = 0;

// Menu navigation state
int menuSub = 0;
int statScroll = 0;
int upgScroll = 0;

// User settings (defaults; overridden by LoadSettings at startup)
float vol_master  = 1.0f;
float vol_music   = 0.8f;
float vol_sfx     = 1.0f;
bool  audio_mute  = false;
int   par_density = 1;      // Medium
float cam_smooth  = 9.0f;
bool  show_fps    = false;
float hud_opacity = 1.0f;
