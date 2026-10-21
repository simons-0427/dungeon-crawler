// game_save.c — stats/highscore persistence and per-game kill bookkeeping.
#include "game_save.h"
#include "globals.h"
#include "constants.h"   // STATS_FILE
#include <stdio.h>

bool StatsEnabled(void)
{
    // Practice mode when enemies are toggled off — do not record stats.
    return eneon;
}

void StatsInc(int *counter)
{
    if (counter && StatsEnabled()) (*counter)++;
}

void StatsAdd(int *counter, int n)
{
    if (counter && StatsEnabled()) (*counter) += n;
}

void LoadStats(void) {
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
    // Line 3: V1.6 stats (missing in old saves → values stay 0)
    fscanf(f, "%d %d %d", &global_bosses_killed, &global_boss_dmg, &global_deaths_lava);
    // Line 4: V1.6 achievement counters (missing in old saves → values stay 0)
    fscanf(f, "%d %d %d %d %d %d %d %d %d",
           &global_use_gld,
           &global_rooms_trap, &global_rooms_dark, &global_rooms_lava, &global_rooms_coinrush,
           &global_cannon_hits, &hs_unbeaten, &global_deaths_boss, &global_deaths_trapdoor);
    if (hs_unbeaten < 0) hs_unbeaten = 0;
    if (hs_unbeaten > 6) hs_unbeaten = 6;
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
    fprintf(f, "%d %d %d\n", global_bosses_killed, global_boss_dmg, global_deaths_lava);
    fprintf(f, "%d %d %d %d %d %d %d %d %d\n",
            global_use_gld,
            global_rooms_trap, global_rooms_dark, global_rooms_lava, global_rooms_coinrush,
            global_cannon_hits, hs_unbeaten, global_deaths_boss, global_deaths_trapdoor);
    fclose(f);
}

void UpdateAndSaveStats(void) {
    // Enemies OFF: practice run — leave highscores and lifetime stats untouched.
    // Still persist coin_balance / shop currency via SaveStats so practice coins
    // are not lost, but without folding run scores into hs_*.
    if (!StatsEnabled()) {
        SaveStats();
        return;
    }
    if (roundn   > hs_round)     hs_round     = roundn;
    if (tottrs   > hs_trs)       hs_trs       = tottrs;
    if (totcoi   > hs_coi)       hs_coi       = totcoi;
    if (kills_game > hs_kills)   hs_kills     = kills_game;
    if (coins_round > hs_coi_round) hs_coi_round = coins_round;
    // Unbeaten Champion tier 1: reach round 5
    if (hs_round >= 5 && hs_unbeaten < 1) hs_unbeaten = 1;
    SaveStats();
}

void AddEnemyKill(void) {
    kills_game++;
    if (!StatsEnabled()) return;
    global_totkills++;
    if (kills_game > hs_kills) hs_kills = kills_game;
}
