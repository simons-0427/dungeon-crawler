#ifndef GAME_BOSS_H
#define GAME_BOSS_H

// Boss fight for RK_BOSS rounds (every 10th round): a 2x2 chase boss with
// prime-number HP (3,5,7,11,13). Damage sources: sword (with retreat
// teleport), spike plates '^', bomb, and shield contact. Killing the boss
// spawns the exit portal + a coin reward; there is no treasure/portal before.

void SpawnBoss(void);              // called by GenRound for boss rounds
void UpdBoss(float dt);            // chase, plates, add summons, trample, timers
void DrawBoss(float t);            // world-space (inside BeginMode2D)
void DrawBossPreview(float cx, float cy, float radius, int pick, float t);
void DrawBossBar(void);            // screen-space HUD health bar

const char *BossDisplayName(void); // HUD/banner name for the current boss tier pick

bool BossCovers(int tx, int ty);   // tile inside the live boss footprint?
bool BossInRange(int rng);         // footprint within Chebyshev rng of player?

// Apply dmg unless i-framed. knockTiles > 0 pushes the boss away from the
// player; teleport does the sword retreat instead. Returns true if the hit
// landed (false while invulnerable or when no boss is active).
bool HitBoss(int dmg, int knockTiles, bool teleport);

#endif // GAME_BOSS_H
