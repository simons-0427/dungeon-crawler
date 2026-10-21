#ifndef GAME_SAVE_H
#define GAME_SAVE_H

#include <stdbool.h>
#include "game_map.h" // KEY_COUNT and RoomKind

enum { STATS_MAIN_RUN, STATS_CHALLENGE_RUN, STATS_SEEDED_RUN, STATS_RUN_MODE_COUNT };

typedef struct {
    int games, rounds, coins, kills, treasures;
} RunTotals;

typedef struct {
    int appeared, entered, skipped, gatesOpened;
} SideRoomTotals;

typedef struct {
    int portalsEntered, arenasSurvived, laddersUsed, vaultsReached;
    int treasuresCollected, fastestRecoveryMs;
} ArtifactRoomTotals;

typedef enum { STATS_ROOM_TRAP, STATS_ROOM_DARK, STATS_ROOM_LAVA,
               STATS_ROOM_COIN_RUSH, STATS_ROOM_TYPE_COUNT } StatsRoomType;
typedef enum { STATS_SHOP_SKINS, STATS_SHOP_UPGRADES, STATS_SHOP_PERKS,
               STATS_SHOP_MUSIC, STATS_SHOP_TYPE_COUNT } StatsShopType;
typedef enum { STATS_COIN_GROUND, STATS_COIN_ENEMY, STATS_COIN_BOSS,
               STATS_COIN_PILE, STATS_COIN_GOLD_TRAP,
               STATS_COIN_SOURCE_COUNT } StatsCoinSource;

typedef struct {
    double activePlaySeconds, longestRunSeconds, fastestTenSeconds, fastestBossSeconds;
    int runsReached5, runsReached10, runsReached20, tilesWalked;
    int roomStarts[STATS_ROOM_TYPE_COUNT], roomClears[STATS_ROOM_TYPE_COUNT];
    int shopSpent[STATS_SHOP_TYPE_COUNT], coinSources[STATS_COIN_SOURCE_COUNT];
    int coinsLeftOnDeath, goldTouchKills, bossFightsStarted, bossFightsWon;
    int unusedKeys, locksOpenedByKey[KEY_COUNT];
    int firstTryChallengeCompletions, challengeCompletionCount;
    long long challengeCompletionTimeMs;
    int dailyCurrentStreak, dailyLongestStreak, lastDailyDate;
} ExtendedStats;

typedef struct {
    RunTotals legacy;
    RunTotals mode[STATS_RUN_MODE_COUNT];
    SideRoomTotals side;
    ArtifactRoomTotals artifact;
    int keysCollected, keysSpent;
    ExtendedStats extra;
} StatsBreakdown;

const StatsBreakdown *GetStatsBreakdown(void);
void StatsFinishCurrentRoom(void);
void StatsTickRoom(float dt);
double StatsCurrentRunSeconds(void);
double StatsCurrentRoundSeconds(void);
void StatsSideRoomAppeared(void);
void StatsSideRoomEntered(void);
void StatsSideRoomGateOpened(void);
void StatsKeyCollected(void);
void StatsKeySpent(void);
void StatsArtifactPortalEntered(void);
void StatsArtifactLadderUsed(void);
void StatsArtifactVaultReached(void);
void StatsArtifactRecoveryTimed(void);
void StatsArtifactTreasureCollected(void);
void StatsRecordUnclassifiedCoins(int amount);
void StatsRunStarted(void);
void StatsRoundStarted(RoomKind kind, int round);
void StatsRoundCompleted(RoomKind kind, int round);
void StatsTileWalked(void);
void StatsShopSpent(StatsShopType kind, int amount);
void StatsCoinEarned(StatsCoinSource source, int amount);
void StatsGoldTouchKill(void);
void StatsBossStarted(void);
void StatsBossDefeated(void);
void StatsKeysDiscarded(int count);
void StatsLockOpened(int keyId);
void StatsChallengeCompleted(bool firstTry, int elapsedMs);
void StatsDailyStarted(void);
int StatsMostLethalRound(int *deaths);

// Stats / highscore persistence (STATS_FILE) and per-game bookkeeping.
void LoadStats(void);          // read highscores + lifetime totals from disk
void SaveStats(void);          // write them back
void UpdateAndSaveStats(void); // fold current run into highscores, then save
void AddEnemyKill(void);       // register a kill (per-game + lifetime + highscore)

// Legacy normal totals/highscores remain main-game only. Recognized counters also
// record separate main, challenge and seeded totals while enemies are enabled.
bool StatsEnabled(void);
void StatsInc(int *counter);           // *counter += 1 if StatsEnabled()
void StatsAdd(int *counter, int n);    // *counter += n if StatsEnabled()

#endif // GAME_SAVE_H
