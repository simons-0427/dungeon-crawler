// advancements.c — advancement tier requirement tables.
#include "advancements.h"
#include "globals.h"
#include "game_save.h"
#include "game_challenges.h"
#include "game_items.h"
#include <string.h>

const int TIERS_HS_KILLS[4] = { 5, 10, 20, 40 };
const int TIERS_HS_COI[4]   = { 50, 100, 250, 500 };
const int TIERS_HS_TRS[4]   = { 2, 5, 10, 20 };
const int TIERS_TOTKILLS[4] = { 100, 500, 2000, 5000 };
const int TIERS_GAMES[4]    = { 10, 50, 150, 300 };
const int TIERS_ROUNDS[4]   = { 25, 125, 400, 1000 };

void BuildAchievementProgress(AchievementProgress out[ACH_COUNT]) {
    const StatsBreakdown *stats = GetStatsBreakdown();
    const ExtendedStats *extra = &stats->extra;
    ChallengeStatsSummary challenges = ChallengeGetStatsSummary();
    int skins = 0, artifacts = 0, openedLocks = 0, keyKinds = 0;
    for (int i = 1; i < SKIN_COUNT; i++) if (skin_owned[i]) skins++;
    for (int i = 0; i < ARTIFACT_COUNT; i++) if (artifactOwned[i]) artifacts++;
    for (int i = 0; i < KEY_COUNT; i++) {
        openedLocks += extra->locksOpenedByKey[i];
        if (extra->locksOpenedByKey[i] > 0) keyKinds++;
    }
    int upgrades = spd_level + shd_level + wlk_level + swd_level + bmb_level + gld_level;
    int perks = perk_slots + perk_magnet + perk_shield2;

    AchievementProgress values[ACH_COUNT] = {
        [0]  = { global_games_played,    {25,50,100,250,500,1000}, 6 },
        [1]  = { global_rounds,          {100,250,800,2000,4000,10000}, 6 },
        [2]  = { global_totcoi,          {250,1000,5000,12000,40000,100000}, 6 },
        [3]  = { global_totkills,        {75,160,350,1000,2500,5000}, 6 },
        [4]  = { hs_trs,                 {10,20,25,30,40,50}, 6 },
        [5]  = { hs_coi,                 {20,50,75,100,150,250}, 6 },
        [6]  = { hs_kills,               {5,10,20,35,50,75}, 6 },
        [7]  = { global_tottrs,          {100,300,800,1500,4000,10000}, 6 },
        [10] = { global_kills_sword,     {25,100,300,750}, 4 },
        [11] = { global_kills_shield,    {15,60,200,500}, 4 },
        [12] = { global_portals,         {15,50,150,400}, 4 },
        [13] = { global_ghostwalk_walls, {10,40,120,300}, 4 },
        [14] = { global_inv_full,        {10,35,100,250}, 4 },
        [15] = { global_use_spd,         {20,75,200,500}, 4 },
        [16] = { global_use_shd,         {20,75,200,500}, 4 },
        [17] = { global_use_bmb,         {20,75,200,500}, 4 },
        [18] = { global_use_wlk,         {20,75,200,500}, 4 },
        [19] = { global_use_swd,         {20,75,200,500}, 4 },
        [20] = { skins,                  {1,5,15,25}, 4 },
        [21] = { upgrades,               {2,6,12,20}, 4 },
        [22] = { perks,                  {1,2,4,6}, 4 },
        [23] = { global_bosses_killed,   {6,18,48,90,180,300}, 6 },
        [24] = { hs_round,               {5,10,20,30,40,50}, 6 },
        [25] = { hs_coi_round,           {38,75,125,200}, 4 },
        [26] = { global_use_gld,         {5,20,60,150}, 4 },
        [27] = { global_rooms_trap,      {3,10,30,80}, 4 },
        [28] = { global_rooms_dark,      {3,10,30,80}, 4 },
        [29] = { global_rooms_lava,      {3,10,30,80}, 4 },
        [30] = { global_rooms_coinrush,  {2,8,25,60}, 4 },
        [31] = { global_cannon_hits,     {5,20,50,120}, 4 },

        // Selected proposals A01, A02, A04, A06, A14, A15, A17, A20,
        // A21, A22, A23, A24, A25, A29 and A30.
        [32] = { extra->runsReached10,          {10,30,75,150,350,750}, 6 },
        [33] = { extra->runsReached20,          {1,3,10,30,75,200}, 6 },
        [34] = { extra->tilesWalked,            {250,1000,15000,35000,75000,150000}, 6 },
        [35] = { extra->goldTouchKills,         {10,25,60,150}, 4 },
        [36] = { stats->side.entered,           {1,3,8,20}, 4 },
        [37] = { stats->side.gatesOpened,       {1,2,5,12}, 4 },
        [38] = { stats->artifact.portalsEntered,{1,3,8,20}, 4 },
        [39] = { artifacts,                     {1,3,5,10,14,18}, 6 },
        [40] = { stats->keysCollected,          {1,3,5,15,25,40}, 6 },
        [41] = { openedLocks,                   {1,3,6,10,20,30}, 6 },
        [42] = { keyKinds,                      {1,3,6,10}, 4 },
        [43] = { challenges.completions,        {1,3,5,8,13,20}, 6 },
        [44] = { challenges.distinctChallengesCompleted, {1,3,5,8,12,16}, 6 },
        [45] = { extra->dailyLongestStreak,     {2,3,5,7}, 4 },
        [46] = { challenges.distinctDailySeeds, {1,3,7,20,35,50}, 6 },
    };

    int earned = 0, maxed = 0;
    for (int i = 0; i < ACH_COUNT; i++) {
        if (i == ACH_OVERACHIEVER_INDEX || i == ACH_PERFECTIONIST_INDEX) continue;
        int tier = 0;
        for (int j = 0; j < values[i].tierCount; j++) {
            if (values[i].value >= values[i].thresholds[j]) tier++;
            else break;
        }
        earned += tier;
        if (tier == values[i].tierCount) maxed++;
    }
    values[ACH_OVERACHIEVER_INDEX] = (AchievementProgress){earned,{5,20,40,70,110,200},6};
    values[ACH_PERFECTIONIST_INDEX] = (AchievementProgress){maxed,{1,3,5,10,20,45},6};
    memcpy(out, values, sizeof(values));
}
