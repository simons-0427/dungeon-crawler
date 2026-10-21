#ifndef GAME_SAVE_H
#define GAME_SAVE_H

#include <stdbool.h>

// Stats / highscore persistence (STATS_FILE) and per-game bookkeeping.
void LoadStats(void);          // read highscores + lifetime totals from disk
void SaveStats(void);          // write them back
void UpdateAndSaveStats(void); // fold current run into highscores, then save
void AddEnemyKill(void);       // register a kill (per-game + lifetime + highscore)

// Lifetime stats only count when enemies are ON (eneon).
// Enemies OFF = practice: no highscores / global counters / achievement progress.
bool StatsEnabled(void);
void StatsInc(int *counter);           // *counter += 1 if StatsEnabled()
void StatsAdd(int *counter, int n);    // *counter += n if StatsEnabled()

#endif // GAME_SAVE_H
