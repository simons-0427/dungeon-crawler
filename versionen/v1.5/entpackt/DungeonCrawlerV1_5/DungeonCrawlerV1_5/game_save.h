#ifndef GAME_SAVE_H
#define GAME_SAVE_H

// Stats / highscore persistence (STATS_FILE) and per-game bookkeeping.
void LoadStats(void);          // read highscores + lifetime totals from disk
void SaveStats(void);          // write them back
void UpdateAndSaveStats(void); // fold current run into highscores, then save
void AddEnemyKill(void);       // register a kill (per-game + lifetime + highscore)

#endif // GAME_SAVE_H
