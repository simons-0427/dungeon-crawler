// game_save.c — stats/highscore persistence and per-game kill bookkeeping.
#include "game_save.h"
#include "globals.h"
#include "constants.h"   // STATS_FILE
#include "web_persist.h"
#include <stdio.h>

void LoadStats(void) {
    WebPersistLoad(STATS_FILE);
    FILE *f = fopen(STATS_FILE, "r");
    if (!f) return;
    // Line 1: original 10 fields
    fscanf(f, "%d %d %d %d %d %d %d %d %d %d",
           &hs_round, &hs_trs, &hs_coi, &global_totcoi,
           &hs_kills, &global_totkills, &global_tottrs, &global_rounds,
           &coin_balance, &global_games_played);
    // Line 2: extended stats (missing in old saves → values stay 0)
    fscanf(f, "%d %d %d %d %d %d %d %d %d %d %d %d %d",
           &hs_coi_round,
           &global_deaths_enemy, &global_deaths_trap,
           &global_kills_sword,  &global_kills_shield,
           &global_portals, &global_ghostwalk_walls, &global_inv_full,
           &global_use_spd, &global_use_shd, &global_use_bmb,
           &global_use_wlk, &global_use_swd);
    fclose(f);
}

void SaveStats(void) {
    FILE *f = fopen(STATS_FILE, "w");
    if (!f) return;
    fprintf(f, "%d %d %d %d %d %d %d %d %d %d\n",
            hs_round, hs_trs, hs_coi, global_totcoi,
            hs_kills, global_totkills, global_tottrs, global_rounds,
            coin_balance, global_games_played);
    fprintf(f, "%d %d %d %d %d %d %d %d %d %d %d %d %d\n",
            hs_coi_round,
            global_deaths_enemy, global_deaths_trap,
            global_kills_sword,  global_kills_shield,
            global_portals, global_ghostwalk_walls, global_inv_full,
            global_use_spd, global_use_shd, global_use_bmb,
            global_use_wlk, global_use_swd);
    fclose(f);
    WebPersistSave(STATS_FILE);
}

void UpdateAndSaveStats(void) {
    if (roundn   > hs_round)     hs_round     = roundn;
    if (tottrs   > hs_trs)       hs_trs       = tottrs;
    if (totcoi   > hs_coi)       hs_coi       = totcoi;
    if (kills_game > hs_kills)   hs_kills     = kills_game;
    if (coins_round > hs_coi_round) hs_coi_round = coins_round;
    SaveStats();
}

void AddEnemyKill(void) {
    kills_game++;
    global_totkills++;
    if (kills_game > hs_kills) hs_kills = kills_game;
}
