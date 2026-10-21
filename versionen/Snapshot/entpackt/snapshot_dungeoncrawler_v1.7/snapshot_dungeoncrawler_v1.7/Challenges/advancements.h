#ifndef ADVANCEMENTS_H
#define ADVANCEMENTS_H

// Advancement / achievement tier thresholds.
// (The achievement screen itself is rendered in DrawMenu() / game_ui.c, which
//  currently uses its own inline tier tables; these shared tables are kept here
//  as the canonical requirement data for future tier-logic.)
extern const int TIERS_HS_KILLS[4];
extern const int TIERS_HS_COI[4];
extern const int TIERS_HS_TRS[4];
extern const int TIERS_TOTKILLS[4];
extern const int TIERS_GAMES[4];
extern const int TIERS_ROUNDS[4];

#endif // ADVANCEMENTS_H
