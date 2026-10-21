#ifndef ADVANCEMENTS_H
#define ADVANCEMENTS_H

// Existing achievement indices 0..31 remain fixed for saved reward claims.
#define ACH_LEGACY_COUNT 32
#define ACH_ADDED_COUNT 15
#define ACH_COUNT (ACH_LEGACY_COUNT + ACH_ADDED_COUNT)
#define ACH_BASE_COUNT (ACH_COUNT - 2)
#define ACH_OVERACHIEVER_INDEX 8
#define ACH_PERFECTIONIST_INDEX 9

typedef struct {
    int value;
    int thresholds[6];
    int tierCount;
} AchievementProgress;

// Builds all requirements and values in their persisted claim-index order.
void BuildAchievementProgress(AchievementProgress out[ACH_COUNT]);

// Older advancement requirement tables retained for callers outside the UI.
extern const int TIERS_HS_KILLS[4];
extern const int TIERS_HS_COI[4];
extern const int TIERS_HS_TRS[4];
extern const int TIERS_TOTKILLS[4];
extern const int TIERS_GAMES[4];
extern const int TIERS_ROUNDS[4];

#endif // ADVANCEMENTS_H
