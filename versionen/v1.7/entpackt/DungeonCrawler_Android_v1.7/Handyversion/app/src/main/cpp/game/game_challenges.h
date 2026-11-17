#ifndef GAME_CHALLENGES_H
#define GAME_CHALLENGES_H

#include "raylib.h"
#include "constants.h"

#define CHALLENGE_MAP_WIDTH 41
#define CHALLENGE_MAP_HEIGHT 31
#define CHALLENGE_FLOORS 3

typedef struct {
    int attempts, completions, distinctChallengesCompleted;
    int seededRuns, distinctSeeds, distinctDailySeeds;
} ChallengeStatsSummary;
ChallengeStatsSummary ChallengeGetStatsSummary(void);

void ChallengeLoadData(void);
void ChallengeDrawMenu(Rectangle panel, float t);
void ChallengeHandleMenuConfirm(void);
int ChallengeEditingSeed(void);
int ChallengeTakeRunStart(unsigned *seed);
void ChallengeClearRunMode(void);
void ChallengeRecordCompletion(void);
void ChallengeSaveSeedRunProgress(void);
int ChallengeRunActive(void);
int ChallengeRunMode(void);
int ChallengeChestDesignForStage(int stage);
int ChallengeKeyForStage(int stage);
void ChallengeGenerateNormalFloor(char out[MAP_ROWS][MAP_COLS], int *spawnX, int *spawnY,
                                  unsigned seed, int floor);
int ChallengeRouteSafe(int x, int y);
int ChallengeHasKey(void);
void ChallengeCollectKey(int keyId);
void ChallengeConsumeKey(int keyId);
float ChallengeElapsed(void);
void ChallengeTickRun(float dt);
void ChallengeRestartRun(void);
void ChallengeDrawRunOverlay(void);

#endif
