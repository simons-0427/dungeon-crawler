#ifndef GAME_ITEMS_H
#define GAME_ITEMS_H

#include "raylib.h"   // Color

// Shared item descriptors, indexed by IKind (IK_SPD..IK_SWD).
// Coins / items / portals themselves live as arrays in globals.h
// (coins[], itms[], prts[]); their collision & pickup logic is handled
// inline by TryMove() (game_player.c) and GenRound() (game_map.c).
#define ARTIFACT_COUNT 18
extern const char *ARTIFACT_NAME[ARTIFACT_COUNT];
extern int artifactOwned[ARTIFACT_COUNT];
extern int floorArtifact;
bool ArtifactDesignPlayable(int id);
void ShutdownArtifactAssets(void);
void CollectArtifact(void);
void CollectCoinPile(void);
void DrawArtifactIcon(int id, Vector2 center, float size, bool collected);
void DrawCoinPileIcon(Vector2 center, float size);
void RefreshSelectedCoinPile(void);
extern const char  *IK_NAME[];   // short 3-letter labels
extern Color        IK_COL[];    // per-kind accent colours (swapped by colorblind mode)

#endif // GAME_ITEMS_H
