#ifndef GAME_MAP_H
#define GAME_MAP_H

#include "constants.h"   // CELL (for the inline tile<->world helpers)
#include "types.h"       // IKind

#define LOCKED_OBJECT_DESIGN_COUNT 16
#define PORTAL_ECLIPSE_IRIS_SEAL_DESIGN_ID 14

typedef enum {
    KEY_JADEEYE=0,
    KEY_STARFORGE,
    KEY_RIFT_THORN,
    KEY_BONE_RELIQUARY,
    KEY_SUNBREAKER,
    KEY_MOONVEIL,
    KEY_VERDANT_COIL,
    KEY_FROSTGLASS,
    KEY_CLOCKHEART,
    KEY_ABYSSAL_TRIDENT,
    KEY_DRAGONFANG,
    KEY_STORM_COMPASS,
    KEY_MOURNING_BELL,
    KEY_WARDENS_OATH,
    KEY_MAGMA_CROWN,
    KEY_COUNT
} UniversalKeyId;

// Tile-centre to world-pixel conversions. Kept inline in the header so every
// module (player, entities, ...) can use them without a call.
static inline float WX(int tx) { return tx * CELL + CELL * 0.5f; }
static inline float WY(int ty) { return ty * CELL + CELL * 0.5f; }

// Procedural round generation: fills map[][], places treasure/portals/coins/
// items/enemies and resets the player & camera for the current roundn.
void StartNewRunSeed(void);
void ResetSeededArtifactSequence(void);
void GenRound(void);
void EnterArtifactRoom(void);
void EnterArtifactVaultRoom(void);
bool CanEnterArtifactVaultRoom(void);
void RevealArtifactTreasure(void);
bool ArtifactAppearsInLocation(int artifactId,bool sideRoom);
int ArtifactLockOptionCount(int artifactId);
int ArtifactLockOptionAt(int artifactId,int option);
const char *LockedObjectDisplayName(int designId);
unsigned LockedObjectAssignedKeys(int designId);
const char *UniversalKeyDisplayName(int keyId);
void ResetUniversalKeys(void);
int UniversalKeyCount(int keyId);
bool CollectUniversalKey(int keyId);
bool ConsumeUniversalKey(int keyId);
char UniversalKeyTileFromId(int keyId);
int UniversalKeyIdFromTile(char tile);
void DrawUniversalKeyIcon(int keyId,int x,int y,int size,float t);
void DrawLockedObjectIcon(int designId,int x,int y,int size);
void ShutdownMapAssets(void);
void ResetLockedObjects(void);
bool RegisterLockedObjectAt(int x,int y,int designId,int keyId,char rewardTile);
bool RegisterLockedObjectWithKeyMaskAt(int x,int y,int designId,unsigned keyMask,char rewardTile);
bool BeginLockedObjectOpeningAt(int x,int y);
void UpdateLockedObjects(void);
bool DrawLockedObjectAt(int x,int y,int wx,int wy,float t);
#ifdef DEBUG_ROOMS
#define DEBUG_SECRET_ROOM (RK_ARTIFACT + 1)
#define DEBUG_PORTAL_ROOM (RK_ARTIFACT + 2)
extern int debugMapScenario;
#endif
bool IsHiddenRoomTile(int x,int y);
bool HasSideRoomKey(void);
bool CollectSideRoomKeyAt(int x,int y);
bool OpenSideRoomGateAt(int x,int y);

// Rendering.
void DrawTile(int x, int y, float t);   // one tile at grid (x,y)
void DrawWorldTile(int wx, int wy, char tile, IKind itemKind, float t);
void DrawTutorialBookKeyIcon(int x, int y, int size);
void DrawMap(float t);                   // whole visible map (loops DrawTile)

// Challenge-room mechanics (V1.6).
bool ArrowLaneLethal(void);     // Are arrow-sweep lanes ('r') firing this instant?
bool ArrowLanesPresent(void);   // Any active arrow-sweep dispensers on the current map.
void UpdLava(float dt);         // RK_LAVA: advance the rising lava front
void DrawDarkness(void);        // RK_DARK: fog overlay + treasure/portal glow

#endif // GAME_MAP_H
