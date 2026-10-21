// game_map.c — round generation and tile/map rendering.
#include "game_map.h"
#include "globals.h"
#include "constants.h"
#include "types.h"
#include "game_items.h"      // IK_COL
#include "game_utils.h"      // UpdateCameraOffset
#include "game_boss.h"       // SpawnBoss
#include "game_particles.h"  // Burst (lava spread)
#include "game_save.h"       // UpdateAndSaveStats (lava death)
#include "game_sfx.h"        // PlaySfx
#include <math.h>
#include <string.h>          // memset, strtok
#include <stdlib.h>          // abs
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include "game_settings.h"
#include "game_challenges.h"
#ifdef DEBUG_ROOMS
int debugMapScenario = -1;
#endif

// Map layout uses its own stream so particles / AI / audio keep the global RNG.
static unsigned map_rng = 1u;
static int hiddenLeft=-1, hiddenTop=-1;
static int hiddenWidth=0, hiddenHeight=0;
static int hiddenMainWidth=-1;
static int hiddenEntryX=-1, hiddenEntryStartY=-1;
static int hiddenGateX=-1, hiddenGateY=-1;
static int hiddenKeyX=-1, hiddenKeyY=-1;
static bool hiddenRoomHasKey=false;
static bool artifactRoute[MAP_ROWS][MAP_COLS];
static int mazeDistance[MAP_ROWS][MAP_COLS];
static short mazePrevX[MAP_ROWS][MAP_COLS],mazePrevY[MAP_ROWS][MAP_COLS];
static int mazeQueueX[MAP_ROWS*MAP_COLS],mazeQueueY[MAP_ROWS*MAP_COLS];
#define MAX_LOCKED_OBJECTS 12
#define LOCKED_DESIGN_COUNT LOCKED_OBJECT_DESIGN_COUNT
#define MAX_ARTIFACT_LOCK_CHOICES LOCKED_DESIGN_COUNT
#define LOCKED_OBJECT_OPEN_SECONDS 0.50f
#define DRAGONBONE_CASKET_DESIGN_ID 8
#define OBSIDIAN_SUN_BOX_DESIGN_ID 11
typedef struct {
    int x,y,designId;
    unsigned keyMask;
    char rewardTile;
    double openingStartedAt;
    bool opening;
} LockedObjectState;
static LockedObjectState lockedObjects[MAX_LOCKED_OBJECTS];
static int lockedObjectCount=0;
static int universalKeyCounts[KEY_COUNT];
static int artifactLockDesignId=-1;
static unsigned artifactLockKeyMask=0;
static int artifactVaultKeyId=-1;
static int artifactVaultArtifactId=-1;
static bool artifactHasLockedReward=false;
static int artifactRoomPart=0;
static bool artifactFirstRoomKeyCollected=false;

static const char *const artifactAssignmentIds[ARTIFACT_COUNT]={
    "ember_crown","moon_compass","void_prism","sun_scarab","ancient_hourglass",
    "storm_core","blood_chalice","frost_lotus","serpent_reliquary",
    "astral_orrery","bone_lantern","abyss_anchor","phoenix_quill",
    "echo_mask","gilded_eye","thorn_heart","eclipse_halo","tide_pearl"
};
static const char *const lockedAssignmentIds[LOCKED_DESIGN_COUNT]={
    "original_chest_motif","chain_vault","rune_reliquary","iron_idol",
    "cursed_coffer","frostbite_reliquary","clockwork_heart",
    "abyssal_treasure_pod","dragonbone_casket","mimic_seed",
    "astral_star_cache","obsidian_sun_box","verdant_thorn_pod",
    "portal_keystone","eclipse_iris_seal","rift_chain_anchor"
};
static const char *const lockedNames[LOCKED_DESIGN_COUNT]={
    "Original Chest Motif","Chain Vault","Rune Reliquary","Iron Idol",
    "Cursed Coffer","Frostbite Reliquary","Clockwork Heart",
    "Abyssal Treasure Pod","Dragonbone Casket","Mimic Seed",
    "Astral Star Cache","Obsidian Sun Box","Verdant Thorn Pod",
    "Portal Keystone","Eclipse Iris Seal","Rift Chain Anchor"
};
// Fixed distribution; lock assignments are independent of room availability.
enum { ARTIFACT_MAIN_ROOM=1, ARTIFACT_SIDE_ROOM=2,
       ARTIFACT_BOTH_ROOMS=ARTIFACT_MAIN_ROOM|ARTIFACT_SIDE_ROOM };
static const unsigned char artifactLocations[ARTIFACT_COUNT]={
    ARTIFACT_MAIN_ROOM,ARTIFACT_BOTH_ROOMS,ARTIFACT_SIDE_ROOM,
    ARTIFACT_BOTH_ROOMS,ARTIFACT_MAIN_ROOM,ARTIFACT_BOTH_ROOMS,
    ARTIFACT_SIDE_ROOM,ARTIFACT_BOTH_ROOMS,ARTIFACT_MAIN_ROOM,
    ARTIFACT_BOTH_ROOMS,ARTIFACT_BOTH_ROOMS,ARTIFACT_SIDE_ROOM,
    ARTIFACT_BOTH_ROOMS,ARTIFACT_BOTH_ROOMS,ARTIFACT_SIDE_ROOM,
    ARTIFACT_BOTH_ROOMS,ARTIFACT_MAIN_ROOM,ARTIFACT_MAIN_ROOM
};
static const char *const keyAssignmentIds[KEY_COUNT]={
    "tutorial_book_key","starforge","rift_thorn","bone_reliquary","sunbreaker",
    "moonveil","verdant_coil","frostglass","clockheart","abyssal_trident",
    "dragonfang","storm_compass","mourning_bell","wardens_oath","magma_crown"
};
static const char *const keyNames[KEY_COUNT]={
    "Tutorial Book Key","Starforge","Rift Thorn","Bone Reliquary","Sunbreaker",
    "Moonveil","Verdant Coil","Frostglass","Clockheart","Abyssal Trident",
    "Dragonfang","Storm Compass","Mourning Bell","Warden's Oath","Magma Crown"
};
static int artifactLockChoices[ARTIFACT_COUNT][MAX_ARTIFACT_LOCK_CHOICES];
static int artifactLockChoiceCount[ARTIFACT_COUNT];
static unsigned lockedKeyMasks[LOCKED_DESIGN_COUNT];
static bool assignmentsLoaded=false;
static Texture2D keyArchiveTextures[KEY_COUNT];
static bool keyArchiveChecked[KEY_COUNT];
static Texture2D lockedArchiveTextures[LOCKED_DESIGN_COUNT];
static bool lockedArchiveChecked[LOCKED_DESIGN_COUNT];

static void SnapshotSpritePath(char *path,size_t capacity,const char *folder,
                               const char *stem,int id) {
    (void)folder;
    snprintf(path,capacity,"%sSnapshot/Library/%s_%02d.png",
             GetApplicationDirectory(),stem,id);
}

static char *TrimAssignmentText(char *text) {
    while(*text&&isspace((unsigned char)*text))text++;
    char *end=text+strlen(text);
    while(end>text&&isspace((unsigned char)end[-1]))*--end='\0';
    return text;
}

static int FindAssignmentId(const char *name,const char *const *catalog,int count) {
    for(int i=0;i<count;i++)if(strcmp(name,catalog[i])==0)return i;
    return -1;
}

static void LoadArtifactLockAssignments(void) {
    if(assignmentsLoaded)return;
    assignmentsLoaded=true;
    memset(artifactLockChoiceCount,0,sizeof(artifactLockChoiceCount));
    memset(lockedKeyMasks,0,sizeof(lockedKeyMasks));
    char path[512];
    snprintf(path,sizeof(path),"%sSnapshot/Library/artifact_lock_assignments.txt",GetApplicationDirectory());
    FILE *file=fopen(path,"r");
    if(!file)return;

    char line[512];
    while(fgets(line,sizeof(line),file)) {
        char *entry=TrimAssignmentText(line);
        if(!*entry||*entry=='#'||*entry==';')continue;
        char *equals=strchr(entry,'=');
        if(!equals)continue;
        *equals='\0';
        char *key=TrimAssignmentText(entry);
        char *values=TrimAssignmentText(equals+1);
        if(strncmp(key,"artifact.",9)==0) {
            int artifactId=FindAssignmentId(key+9,artifactAssignmentIds,ARTIFACT_COUNT);
            if(artifactId<0)continue;
            artifactLockChoiceCount[artifactId]=0;
            for(char *token=strtok(values,",");token;token=strtok(NULL,",")) {
                char *idText=TrimAssignmentText(token);
                int designId=FindAssignmentId(idText,lockedAssignmentIds,LOCKED_DESIGN_COUNT);
                if(designId<0)continue;
                bool duplicate=false;
                for(int i=0;i<artifactLockChoiceCount[artifactId];i++)
                    if(artifactLockChoices[artifactId][i]==designId)duplicate=true;
                if(!duplicate&&artifactLockChoiceCount[artifactId]<MAX_ARTIFACT_LOCK_CHOICES)
                    artifactLockChoices[artifactId][artifactLockChoiceCount[artifactId]++]=designId;
            }
        } else if(strncmp(key,"lock.",5)==0) {
            int designId=FindAssignmentId(key+5,lockedAssignmentIds,LOCKED_DESIGN_COUNT);
            if(designId<0)continue;
            unsigned mask=0;
            for(char *token=strtok(values,",");token;token=strtok(NULL,",")) {
                char *idText=TrimAssignmentText(token);
                int keyId=FindAssignmentId(idText,keyAssignmentIds,KEY_COUNT);
                if(keyId>=0)mask|=(1u<<keyId);
            }
            lockedKeyMasks[designId]=mask;
        }
    }
    fclose(file);
}

bool ArtifactAppearsInLocation(int artifactId,bool sideRoom) {
    return artifactId>=0&&artifactId<ARTIFACT_COUNT&&
           (artifactLocations[artifactId]&
            (sideRoom?ARTIFACT_SIDE_ROOM:ARTIFACT_MAIN_ROOM))!=0;
}

int ArtifactLockOptionCount(int artifactId) {
    LoadArtifactLockAssignments();
    if(artifactId<0||artifactId>=ARTIFACT_COUNT)return 0;
    return artifactLockChoiceCount[artifactId];
}

int ArtifactLockOptionAt(int artifactId,int option) {
    if(option<0||option>=ArtifactLockOptionCount(artifactId))return -1;
    return artifactLockChoices[artifactId][option];
}

const char *LockedObjectDisplayName(int designId) {
    if(designId<0||designId>=LOCKED_DESIGN_COUNT)return "Unknown lock";
    return lockedNames[designId];
}

unsigned LockedObjectAssignedKeys(int designId) {
    LoadArtifactLockAssignments();
    if(designId<0||designId>=LOCKED_DESIGN_COUNT)return 0;
    return lockedKeyMasks[designId];
}

const char *UniversalKeyDisplayName(int keyId) {
    if(keyId<0||keyId>=KEY_COUNT)return "Unknown key";
    return keyNames[keyId];
}

static bool LoadSnapshotTexture(Texture2D *texture,bool *checked,
                                const char *folder,const char *stem,int id) {
    if(*checked)return texture->id!=0;
    *checked=true;
    char path[512];
    SnapshotSpritePath(path,sizeof(path),folder,stem,id);
    if(!FileExists(path))return false;
    *texture=LoadTexture(path);
    if(texture->id==0)return false;
    SetTextureFilter(*texture,TEXTURE_FILTER_POINT);
    return true;
}

void ShutdownMapAssets(void) {
    for(int i=0;i<KEY_COUNT;i++)if(keyArchiveTextures[i].id)
        UnloadTexture(keyArchiveTextures[i]);
    for(int i=0;i<LOCKED_DESIGN_COUNT;i++)if(lockedArchiveTextures[i].id)
        UnloadTexture(lockedArchiveTextures[i]);
}

static bool DrawArchivedKeySprite(int keyId,int x,int y,int size) {
    if(keyId<0||keyId>=KEY_COUNT||
       !LoadSnapshotTexture(&keyArchiveTextures[keyId],&keyArchiveChecked[keyId],
                            "keys","key",keyId))return false;
    Texture2D texture=keyArchiveTextures[keyId];
    DrawTexturePro(texture,(Rectangle){0,(float)texture.height,
                                      (float)texture.width,-(float)texture.height},
                   (Rectangle){(float)x,(float)y,(float)size,(float)size},
                   (Vector2){0,0},0,WHITE);
    return true;
}


void ResetUniversalKeys(void) {
    memset(universalKeyCounts,0,sizeof(universalKeyCounts));
}

char UniversalKeyTileFromId(int keyId) {
    static const char extraTiles[5]={'b','c','d','e','f'};
    if(keyId<0||keyId>=KEY_COUNT)return '\0';
    if(keyId<10)return (char)('0'+keyId);
    return extraTiles[keyId-10];
}

int UniversalKeyIdFromTile(char tile) {
    static const char extraTiles[5]={'b','c','d','e','f'};
    if(tile>='0'&&tile<='9') {
        int id=tile-'0';
        return id<KEY_COUNT?id:-1;
    }
    for(int i=0;i<5;i++)if(tile==extraTiles[i])return 10+i;
    return -1;
}

int UniversalKeyCount(int keyId) {
    if(keyId<0||keyId>=KEY_COUNT)return 0;
    return universalKeyCounts[keyId];
}

bool CollectUniversalKey(int keyId) {
    if(keyId<0||keyId>=KEY_COUNT||universalKeyCounts[keyId]>=9)return false;
    universalKeyCounts[keyId]++;
    if(roomKind==RK_ARTIFACT&&artifactRoomPart==0&&keyId==artifactVaultKeyId)
        artifactFirstRoomKeyCollected=true;
    return true;
}

bool ConsumeUniversalKey(int keyId) {
    if(keyId<0||keyId>=KEY_COUNT||universalKeyCounts[keyId]<=0)return false;
    universalKeyCounts[keyId]--;
    return true;
}

void ResetLockedObjects(void) {
    lockedObjectCount=0;
}

bool RegisterLockedObjectWithKeyMaskAt(int x,int y,int designId,unsigned keyMask,char rewardTile) {
    if(x<0||y<0||x>=MAP_COLS||y>=MAP_ROWS||designId<0||designId>=LOCKED_DESIGN_COUNT||
       (keyMask>>KEY_COUNT)!=0||
       (rewardTile!='A'&&rewardTile!='T'&&rewardTile!='M'&&rewardTile!='%'))return false;
    for(int i=0;i<lockedObjectCount;i++) {
        if(lockedObjects[i].x==x&&lockedObjects[i].y==y) {
            lockedObjects[i]=(LockedObjectState){x,y,designId,keyMask,rewardTile,0.0,false};
            map[y][x]='Q';
            return true;
        }
    }
    if(lockedObjectCount>=MAX_LOCKED_OBJECTS)return false;
    lockedObjects[lockedObjectCount++]=(LockedObjectState){x,y,designId,keyMask,rewardTile,0.0,false};
    map[y][x]='Q';
    return true;
}

bool RegisterLockedObjectAt(int x,int y,int designId,int keyId,char rewardTile) {
    if(keyId<0||keyId>=KEY_COUNT)return false;
    return RegisterLockedObjectWithKeyMaskAt(x,y,designId,1u<<keyId,rewardTile);
}

bool BeginLockedObjectOpeningAt(int x,int y) {
    for(int i=0;i<lockedObjectCount;i++) {
        LockedObjectState *obj=&lockedObjects[i];
        if(obj->x!=x||obj->y!=y)continue;
        unsigned designBit=1u<<obj->designId;
        if((lockedObjectDiscoveredMask&designBit)==0) {
            lockedObjectDiscoveredMask|=designBit;
            SaveStats();
        }
        if(obj->opening)return true;
        int keyId=-1;
        for(int id=0;id<KEY_COUNT;id++)
            if((obj->keyMask&(1u<<id))&&UniversalKeyCount(id)>0){keyId=id;break;}
        if(obj->keyMask==0||keyId<0)return true;
        if(keyId>=0&&!ConsumeUniversalKey(keyId))return true;
        if(keyId>=0&&ChallengeRunActive())ChallengeConsumeKey(keyId);
        obj->opening=true;
        obj->openingStartedAt=GetTime();
        PlaySfx(SFX_ITEM_PICKUP);
        Burst(WX(x),WY(y),(Color){245,205,126,255},18,3);
        return true;
    }
    return false;
}

void UpdateLockedObjects(void) {
    double now=GetTime();
    for(int i=0;i<lockedObjectCount;) {
        LockedObjectState *obj=&lockedObjects[i];
        if(!obj->opening||now-obj->openingStartedAt<LOCKED_OBJECT_OPEN_SECONDS) {
            i++;
            continue;
        }
        map[obj->y][obj->x]=obj->rewardTile;
        lockedObjects[i]=lockedObjects[--lockedObjectCount];
    }
}

bool IsHiddenRoomTile(int x,int y) {
 return hiddenTop>=0 && x>=hiddenLeft && x<hiddenLeft+hiddenWidth &&
        y>=hiddenTop && y<hiddenTop+hiddenHeight;
}

static void ResetHiddenRoomAccess(void) {
    hiddenEntryX=hiddenEntryStartY=-1;
    hiddenGateX=hiddenGateY=-1;
    hiddenKeyX=hiddenKeyY=-1;
    hiddenRoomHasKey=false;
}

bool HasSideRoomKey(void) { return hiddenRoomHasKey; }

bool CollectSideRoomKeyAt(int x,int y) {
    if(x!=hiddenKeyX||y!=hiddenKeyY||hiddenRoomHasKey)return false;
    hiddenRoomHasKey=true;
    hiddenKeyX=hiddenKeyY=-1;
    return true;
}

bool OpenSideRoomGateAt(int x,int y) {
    if(x!=hiddenGateX||y!=hiddenGateY||!hiddenRoomHasKey||
       UniversalKeyCount(KEY_TUTORIAL_BOOK)<=0||map[y][x]!='G')
        return false;
    ConsumeUniversalKey(KEY_TUTORIAL_BOOK);
    map[y][x]='o';
    hiddenRoomHasKey=false;
    return true;
}

typedef struct { float x,y,scale,zoom,shakeX; } LockSpriteContext;
static LockSpriteContext lockSprite;

static Vector2 LockSpritePoint(float x,float y) {
    float s=lockSprite.scale*lockSprite.zoom;
    return (Vector2){lockSprite.x+16.0f*lockSprite.scale+
                         (x-16.0f)*s+lockSprite.shakeX,
                     lockSprite.y+16.0f*lockSprite.scale+(y-16.0f)*s};
}
static void LockSpriteBox(float x,float y,float w,float h,Color c) {
    Vector2 p=LockSpritePoint(x,y);
    float s=lockSprite.scale*lockSprite.zoom;
    int rw=(int)lroundf(w*s),rh=(int)lroundf(h*s);
    if(rw<1)rw=1;
    if(rh<1)rh=1;
    DrawRectangle((int)lroundf(p.x),(int)lroundf(p.y),rw,rh,c);
}
static void LockSpriteDot(float x,float y,float r,Color c) {
    Vector2 p=LockSpritePoint(x,y);
    float s=lockSprite.scale*lockSprite.zoom;
    DrawCircle((int)lroundf(p.x),(int)lroundf(p.y),
               (int)fmaxf(1.0f,lroundf(r*s)),c);
}
static void LockSpriteLine(float x1,float y1,float x2,float y2,float width,Color c) {
    Vector2 a=LockSpritePoint(x1,y1),b=LockSpritePoint(x2,y2);
    DrawLineEx(a,b,fmaxf(1.0f,width*lockSprite.scale*lockSprite.zoom),c);
}
static void LockSpriteTri(float x1,float y1,float x2,float y2,float x3,float y3,Color c) {
    Vector2 a=LockSpritePoint(x1,y1),b=LockSpritePoint(x2,y2),d=LockSpritePoint(x3,y3);
    float cross=(b.x-a.x)*(d.y-a.y)-(b.y-a.y)*(d.x-a.x);
    if(cross>0.0f)DrawTriangle(a,d,b,c);else DrawTriangle(a,b,d,c);
}
static void LockSpriteDiamond(float x,float y,float r,Color c) {
    LockSpriteTri(x,y-r,x+r,y,x,y+r,c);
    LockSpriteTri(x,y-r,x,y+r,x-r,y,c);
}
static void LockSpriteRing(float x,float y,float r,float thick,Color c) {
    Vector2 p=LockSpritePoint(x,y);
    float s=lockSprite.scale*lockSprite.zoom;
    float outer=r*s,inner=fmaxf(0.0f,(r-thick)*s);
    DrawRing(p,inner,outer,0,360,28,c);
}
static void LockSpriteSpark(float x,float y,float r,Color c) {
    LockSpriteLine(x-r,y,x+r,y,1,c);
    LockSpriteLine(x,y-r,x,y+r,1,c);
    LockSpriteDot(x,y,1,c);
}
static void LockSpriteKeyhole(float x,float y) {
    LockSpriteRing(x,y,4,1,(Color){239,193,103,255});
    LockSpriteDot(x,y-1,1,(Color){6,8,15,255});
    LockSpriteTri(x-1,y,x+1,y,x,y+4,(Color){5,7,15,255});
    LockSpriteDot(x,y-1,1,(Color){9,9,18,255});
}

void DrawUniversalKeyIcon(int keyId,int x,int y,int size,float t) {
    if(size<=0||keyId<0||keyId>=KEY_COUNT)return;
    lockSprite=(LockSpriteContext){(float)x,(float)y,(float)size/32.0f,1.0f,0.0f};
    if(DrawArchivedKeySprite(keyId,x,y,size))return;
    float p=0.5f+0.5f*sinf(t*2.5f);
    LockSpriteDot(16,16,13,(Color){239,192,106,(uchar)(22+22*p)});
    if(keyId==KEY_TUTORIAL_BOOK) {
        LockSpriteLine(6,25,23,8,4,(Color){104,73,46,255});
        LockSpriteLine(6,24,23,7,2,(Color){252,215,132,255});
        LockSpriteRing(23,8,6,2,(Color){242,196,103,255});
        LockSpriteRing(23,8,3,1,(Color){106,229,213,255});
        LockSpriteDiamond(23,8,2,(Color){119,245,231,255});
        for(int i=0;i<3;i++)LockSpriteLine(7+i*3,24-i*3,10+i*3,27-i*3,2,
                                            (Color){252,215,132,255});
    } else if(keyId==KEY_STARFORGE) {
        LockSpriteLine(9,26,22,8,5,(Color){94,64,59,255});
        LockSpriteLine(9,26,22,8,3,(Color){255,206,116,255});
        LockSpriteRing(22,8,5,2,(Color){254,207,101,255});
        for(int i=0;i<5;i++) {
            float a=i*2.0f*PI/5.0f-PI/2.0f;
            LockSpriteTri(22+cosf(a)*5,8+sinf(a)*5,
                          22+cosf(a)*9,8+sinf(a)*9,
                          22+cosf(a+0.3f)*5,8+sinf(a+0.3f)*5,
                          (Color){255,230,154,255});
        }
        LockSpriteDiamond(22,8,3,(Color){244,97,108,255});
        LockSpriteBox(7,22,5,3,(Color){255,228,139,255});
        LockSpriteBox(9,25,5,3,(Color){255,228,139,255});
    } else if(keyId==KEY_RIFT_THORN) {
        LockSpriteLine(8,26,21,8,5,(Color){20,18,42,255});
        LockSpriteLine(8,26,21,8,2,(Color){173,81,249,255});
        LockSpriteTri(17,10,24,1,27,12,(Color){95,240,245,255});
        LockSpriteTri(17,10,27,12,21,16,(Color){32,131,194,255});
        LockSpriteDiamond(22,9,3,(Color){235,252,255,255});
        LockSpriteTri(6,21,11,25,4,29,(Color){161,66,242,255});
        LockSpriteTri(9,24,14,27,8,31,(Color){107,218,244,255});
    } else if(keyId==KEY_BONE_RELIQUARY) {
        LockSpriteLine(9,26,22,8,5,(Color){105,90,79,255});
        LockSpriteLine(9,26,22,8,3,(Color){232,218,180,255});
        LockSpriteRing(22,8,6,2,(Color){239,223,185,255});
        LockSpriteTri(17,7,22,2,27,7,(Color){244,232,201,255});
        LockSpriteBox(6,22,5,2,(Color){241,228,195,255});
        LockSpriteBox(8,25,5,2,(Color){241,228,195,255});
        LockSpriteDot(22,8,2,(Color){246,83,176,255});
    } else {
        LockSpriteLine(8,26,22,8,5,(Color){103,55,36,255});
        LockSpriteLine(8,26,22,8,3,(Color){255,199,78,255});
        LockSpriteRing(22,8,6,2,(Color){255,196,68,255});
        LockSpriteTri(18,4,22,0,26,4,(Color){255,232,132,255});
        LockSpriteTri(27,6,31,8,27,10,(Color){255,232,132,255});
        LockSpriteDiamond(22,8,3,(Color){255,95,52,255});
        LockSpriteBox(5,22,6,3,(Color){255,224,101,255});
        LockSpriteBox(8,25,7,3,(Color){255,224,101,255});
    }
    LockSpriteSpark(27,25,2,(Color){255,235,178,255});
}

void DrawTutorialBookKeyIcon(int x,int y,int size) {
    DrawUniversalKeyIcon(KEY_TUTORIAL_BOOK,x,y,size,(float)GetTime());
}

static const Color lockedObjectAccent[LOCKED_DESIGN_COUNT]={
    {249,214,129,255},{196,208,230,255},{104,210,237,255},
    {251,116,73,255},{244,128,167,255},{132,239,248,255},
    {255,211,103,255},{118,237,205,255},{255,91,77,255},
    {188,231,100,255},{220,174,255,255},{255,115,62,255},
    {250,130,195,255},{131,239,214,255},{229,164,249,255},
    {248,174,130,255}
};

static bool DrawArchivedLockedSprite(int designId) {
    if(designId<0||designId>=LOCKED_DESIGN_COUNT||
       !LoadSnapshotTexture(&lockedArchiveTextures[designId],
                            &lockedArchiveChecked[designId],
                            "locked_objects","locked",designId))return false;
    Texture2D texture=lockedArchiveTextures[designId];
    Vector2 topLeft=LockSpritePoint(0,0);
    float scale=lockSprite.scale*lockSprite.zoom;
    DrawTexturePro(texture,(Rectangle){0,(float)texture.height,
                                      (float)texture.width,-(float)texture.height},
                   (Rectangle){topLeft.x,topLeft.y,32.0f*scale,32.0f*scale},
                   (Vector2){0,0},0,WHITE);
    return true;
}

static void DrawObsidianSunBoxHornHighlights(void) {
    LockSpriteLine(4,14,8,8,1.3f,(Color){245,184,126,255});
    LockSpriteLine(8,8,13,14,1.3f,(Color){255,139,91,255});
    LockSpriteLine(28,14,24,8,1.3f,(Color){245,184,126,255});
    LockSpriteLine(24,8,19,14,1.3f,(Color){255,139,91,255});
}

static void DrawChallengeSealOverlay(int designId,float t) {
    bool bone=designId==DRAGONBONE_CASKET_DESIGN_ID;
    float pulse=photosensitivity?0.5f:0.5f+0.5f*sinf(t*3.0f);
    Color outer=bone?(Color){240,223,183,(uchar)(150+35*pulse)}:
                     (Color){255,125,76,(uchar)(170+35*pulse)};
    Color inner=bone?(Color){255,107,94,220}:(Color){248,200,123,210};
    LockSpriteRing(16,20,10,1.2f,outer);
    LockSpriteRing(16,20,7,0.8f,inner);
    for(int i=0;i<6;i++) {
        float angle=(float)i*PI/3.0f;
        LockSpriteDiamond(16+cosf(angle)*9,20+sinf(angle)*9,1.1f,inner);
    }
    LockSpriteLine(16,10,16,13,1,outer);
    LockSpriteLine(16,27,16,30,1,outer);
}

static void DrawChallengeObsidianSunBox(float t) {
    float pulse=photosensitivity?0.5f:0.5f+0.5f*sinf(t*3.0f);
    LockSpriteDot(16,20,15,(Color){255,130,78,(uchar)(25+24*pulse)});
    LockSpriteBox(3,12,26,18,(Color){9,7,17,255});
    LockSpriteBox(4,13,24,16,(Color){88,75,94,255});
    LockSpriteBox(6,15,20,12,(Color){12,10,24,255});
    LockSpriteTri(4,14,8,8,13,14,(Color){173,153,167,255});
    LockSpriteTri(28,14,24,8,19,14,(Color){145,128,151,255});
    LockSpriteBox(5,12,22,4,(Color){111,93,111,255});
    LockSpriteBox(7,13,18,1,(Color){232,188,160,255});
    LockSpriteLine(8,16,12,20,2,(Color){255,86,77,255});
    LockSpriteLine(12,20,10,25,2,(Color){255,158,83,255});
    LockSpriteLine(24,16,20,19,2,(Color){242,69,79,255});
    LockSpriteLine(20,19,22,25,2,(Color){255,168,78,255});
    LockSpriteRing(16,20,8,2,(Color){232,78,68,255});
    LockSpriteRing(16,20,7,0.8f,(Color){255,179,106,(uchar)(180+45*pulse)});
    LockSpriteDot(16,20,5,(Color){12,9,22,255});
    LockSpriteDiamond(16,20,3,(Color){255,166,70,255});
    LockSpriteKeyhole(16,20);
    LockSpriteBox(7,27,18,2,(Color){178,62,70,255});
    LockSpriteLine(4,16,4,27,1,(Color){184,145,146,255});
    LockSpriteLine(28,16,28,27,1,(Color){154,119,135,255});
}

static void DrawLockedObjectClosed(int id,float t) {
    if(id==OBSIDIAN_SUN_BOX_DESIGN_ID&&ChallengeRunActive()) {
        DrawChallengeObsidianSunBox(t);
        return;
    }
    if(DrawArchivedLockedSprite(id)) {
        if(id==OBSIDIAN_SUN_BOX_DESIGN_ID)DrawObsidianSunBoxHornHighlights();
        if(id==DRAGONBONE_CASKET_DESIGN_ID||id==OBSIDIAN_SUN_BOX_DESIGN_ID)
            DrawChallengeSealOverlay(id,t);
        return;
    }
    Color shadow={10,8,20,255};
    if(id==0) {
        LockSpriteBox(3,10,26,20,shadow);
        LockSpriteBox(4,11,24,18,(Color){126,96,82,255});
        LockSpriteBox(5,13,22,12,(Color){70,53,68,255});
        LockSpriteBox(3,10,26,6,(Color){168,129,96,255});
        LockSpriteBox(6,11,20,2,(Color){228,181,113,255});
        LockSpriteBox(8,17,3,10,(Color){193,150,100,255});
        LockSpriteBox(21,17,3,10,(Color){193,150,100,255});
        LockSpriteBox(13,14,6,10,(Color){206,154,83,255});
        LockSpriteKeyhole(16,19);
    } else if(id==1) {
        LockSpriteBox(3,10,26,20,shadow);
        LockSpriteBox(3,11,26,18,(Color){48,51,72,255});
        LockSpriteBox(5,13,22,14,(Color){19,22,38,255});
        for(int y=15;y<27;y+=5)LockSpriteLine(5,y,27,y+3,2,(Color){155,164,186,255});
        LockSpriteLine(6,10,26,29,3,(Color){170,169,168,255});
        LockSpriteLine(26,10,6,29,3,(Color){95,119,135,255});
        LockSpriteRing(16,19,7,2,(Color){207,179,111,255});
        LockSpriteKeyhole(16,19);
        LockSpriteDot(6,10,2,(Color){250,198,104,255});
        LockSpriteDot(26,29,2,(Color){250,198,104,255});
    } else if(id==2) {
        LockSpriteBox(3,10,26,20,shadow);
        LockSpriteBox(4,10,24,19,(Color){26,39,53,255});
        for(int i=0;i<3;i++) {
            LockSpriteDiamond(8+i*8,11,4,(Color){78,179,202,255});
            LockSpriteDiamond(8+i*8,25,3,(Color){78,179,202,255});
        }
        LockSpriteBox(7,14,18,9,(Color){12,19,33,255});
        LockSpriteDiamond(16,18,7,(Color){152,87,227,255});
        LockSpriteDiamond(16,18,4,(Color){31,26,56,255});
        LockSpriteKeyhole(16,18);
        LockSpriteSpark(5,6,2,(Color){167,246,252,255});
    } else if(id==5) {
        LockSpriteDot(16,19,14,(Color){43,188,222,34});
        LockSpriteBox(4,14,24,15,(Color){17,62,83,255});
        LockSpriteBox(6,16,20,11,(Color){28,117,142,255});
        LockSpriteTri(4,15,7,5,12,15,(Color){131,231,239,255});
        LockSpriteTri(12,15,15,2,18,15,(Color){192,249,248,255});
        LockSpriteTri(20,15,25,5,28,15,(Color){102,208,225,255});
        LockSpriteBox(4,13,24,4,(Color){158,240,244,255});
        LockSpriteBox(6,13,20,2,(Color){224,255,251,255});
        LockSpriteBox(7,18,18,7,(Color){13,42,63,255});
        LockSpriteLine(8,22,24,22,1,(Color){76,188,212,255});
        LockSpriteDiamond(16,20,5,(Color){45,176,227,255});
        LockSpriteDiamond(16,20,2,(Color){195,250,255,255});
        LockSpriteKeyhole(16,20);
        LockSpriteSpark(7,8,2,(Color){191,255,255,255});
        LockSpriteSpark(25,9,2,(Color){130,232,249,255});
    } else if(id==6) {
        LockSpriteBox(4,13,24,16,(Color){72,43,24,255});
        LockSpriteBox(6,15,20,12,(Color){121,76,34,255});
        LockSpriteBox(3,10,26,7,(Color){153,102,42,255});
        LockSpriteBox(5,11,22,3,(Color){237,190,91,255});
        LockSpriteBox(5,18,3,8,(Color){225,168,64,255});
        LockSpriteBox(24,18,3,8,(Color){170,107,41,255});
        LockSpriteRing(9,19,6,2,(Color){239,188,81,255});
        LockSpriteRing(9,19,3,1,(Color){67,48,37,255});
        LockSpriteRing(23,19,6,2,(Color){207,140,57,255});
        LockSpriteRing(23,19,3,1,(Color){67,48,37,255});
        LockSpriteRing(16,19,8,2,(Color){255,211,103,255});
        LockSpriteRing(16,19,5,1,(Color){91,55,33,255});
        LockSpriteBox(15,15,2,5,(Color){248,219,139,255});
        LockSpriteLine(16,19,20,17,1,(Color){255,226,135,255});
        LockSpriteKeyhole(16,23);
        LockSpriteDot(6,12,1,(Color){255,234,154,255});
        LockSpriteDot(26,12,1,(Color){255,234,154,255});
        (void)t;
    } else if(id==8) {
        LockSpriteBox(5,13,22,15,(Color){44,30,35,255});
        LockSpriteBox(8,16,16,10,(Color){84,45,51,255});
        for(int i=0;i<5;i++) {
            int x=7+i*4;
            LockSpriteLine(x,15,x+1,25,2,i%2?(Color){210,202,174,255}:
                                                    (Color){244,230,193,255});
            LockSpriteDot(x,15,2,(Color){242,226,188,255});
        }
        LockSpriteTri(5,15,8,6,12,15,(Color){226,218,186,255});
        LockSpriteTri(27,15,24,6,20,15,(Color){183,179,163,255});
        LockSpriteBox(7,13,18,3,(Color){218,209,177,255});
        LockSpriteBox(8,25,16,3,(Color){198,190,162,255});
        LockSpriteDiamond(16,20,6,(Color){141,37,54,255});
        LockSpriteDiamond(16,20,3,(Color){255,91,77,255});
        LockSpriteKeyhole(16,20);
        LockSpriteLine(5,28,27,28,2,(Color){117,82,73,255});
    } else if(id==11) {
        LockSpriteBox(4,13,24,16,(Color){20,20,29,255});
        LockSpriteBox(6,15,20,12,(Color){4,8,17,255});
        LockSpriteTri(4,14,8,8,13,14,(Color){63,61,76,255});
        LockSpriteTri(28,14,24,8,19,14,(Color){42,39,57,255});
        LockSpriteBox(5,12,22,4,(Color){50,48,61,255});
        LockSpriteBox(7,13,18,1,(Color){145,130,139,255});
        LockSpriteLine(8,16,12,20,2,(Color){226,51,54,255});
        LockSpriteLine(12,20,10,25,2,(Color){255,111,53,255});
        LockSpriteLine(24,16,20,19,2,(Color){209,42,53,255});
        LockSpriteLine(20,19,22,25,2,(Color){255,127,49,255});
        LockSpriteRing(16,20,8,2,(Color){175,43,54,255});
        LockSpriteDot(16,20,5,(Color){12,10,20,255});
        LockSpriteDiamond(16,20,3,(Color){255,115,42,255});
        LockSpriteKeyhole(16,20);
        LockSpriteBox(7,27,18,2,(Color){107,33,43,255});
        DrawObsidianSunBoxHornHighlights();
    } else if(id==13) {
        LockSpriteDot(16,16,14,(Color){57,220,211,39});
        LockSpriteTri(3,9,16,1,29,9,(Color){38,49,63,255});
        LockSpriteTri(7,9,16,3,25,9,(Color){124,151,150,255});
        LockSpriteBox(4,9,24,19,(Color){35,46,56,255});
        LockSpriteBox(6,10,20,16,(Color){69,89,96,255});
        LockSpriteBox(9,11,14,14,(Color){11,22,35,255});
        LockSpriteBox(3,12,4,13,(Color){111,139,139,255});
        LockSpriteBox(25,12,4,13,(Color){75,111,120,255});
        LockSpriteBox(5,26,22,3,(Color){41,55,65,255});
        LockSpriteBox(8,27,16,1,(Color){124,161,151,255});
        LockSpriteLine(8,13,8,23,1,(Color){134,229,205,255});
        LockSpriteLine(24,13,24,23,1,(Color){134,229,205,255});
        LockSpriteTri(16,8,11,15,16,13,(Color){84,207,189,255});
        LockSpriteTri(16,8,21,15,16,13,(Color){167,241,211,255});
        LockSpriteDiamond(16,18,6,(Color){43,118,122,255});
        LockSpriteDiamond(16,18,4,(Color){157,233,211,255});
        LockSpriteKeyhole(16,18);
        LockSpriteSpark(16,5,2,(Color){231,244,180,255});
    } else if(id==14) {
        LockSpriteDot(16,16,15,(Color){199,110,236,34});
        LockSpriteDot(16,16,13,(Color){35,33,59,255});
        LockSpriteRing(16,16,13,3,(Color){121,120,161,255});
        LockSpriteRing(16,16,10,1,(Color){219,190,223,255});
        for(int i=0;i<6;i++) {
            float a=(float)i*PI/3.0f;
            LockSpriteTri(16+cosf(a)*8,16+sinf(a)*8,
                          16+cosf(a+0.55f)*11,16+sinf(a+0.55f)*11,
                          16+cosf(a+1.05f)*4,16+sinf(a+1.05f)*4,
                          i%2?(Color){92,75,139,255}:(Color){151,126,177,255});
        }
        LockSpriteDot(16,16,6,(Color){9,12,31,255});
        LockSpriteRing(16,16,6,1,(Color){245,205,139,255});
        LockSpriteDiamond(16,16,3,(Color){230,145,245,255});
        LockSpriteKeyhole(16,16);
    } else if(id==15) {
        LockSpriteDot(16,16,14,(Color){167,74,222,45});
        LockSpriteBox(8,3,16,26,(Color){41,27,58,255});
        LockSpriteBox(10,5,12,22,(Color){7,8,28,255});
        LockSpriteBox(12,7,8,18,(Color){53,22,82,255});
        LockSpriteLine(7,3,7,29,3,(Color){133,78,81,255});
        LockSpriteLine(25,3,25,29,3,(Color){92,61,93,255});
        LockSpriteLine(7,5,25,27,3,(Color){85,66,82,255});
        LockSpriteLine(7,5,25,27,1,(Color){218,168,130,255});
        LockSpriteLine(25,5,7,27,3,(Color){85,66,82,255});
        LockSpriteLine(25,5,7,27,1,(Color){226,177,138,255});
        LockSpriteBox(11,12,10,9,(Color){53,42,53,255});
        LockSpriteBox(12,13,8,7,(Color){180,102,93,255});
        LockSpriteDiamond(16,16,4,(Color){244,156,119,255});
        LockSpriteKeyhole(16,16);
    } else {
        DrawLockedObjectClosed(0,t);
    }
    if(id==DRAGONBONE_CASKET_DESIGN_ID||id==OBSIDIAN_SUN_BOX_DESIGN_ID)
        DrawChallengeSealOverlay(id,t);
}

void DrawLockedObjectIcon(int designId,int x,int y,int size) {
    if(designId<0||designId>=LOCKED_DESIGN_COUNT||size<=0)return;
    LockSpriteContext previous=lockSprite;
    lockSprite=(LockSpriteContext){(float)x,(float)y,(float)size/32.0f,1.0f,0.0f};
    DrawLockedObjectClosed(designId,0.0f);
    lockSprite=previous;
}

static void DrawLockedObjectOpened(int designId,float progress,Color accent) {
    float lid=fminf(1.0f,fmaxf(0.0f,(progress-0.08f)/0.20f));
    int lift=(int)(lid*7.0f);
    if(designId==OBSIDIAN_SUN_BOX_DESIGN_ID&&ChallengeRunActive()) {
        DrawChallengeObsidianSunBox((float)GetTime());
        LockSpriteDot(16,20,3.0f+lid*6.0f,(Color){5,4,14,255});
        LockSpriteRing(16,20,7.0f+lid*5.0f,1.3f,
                       (Color){accent.r,accent.g,accent.b,(uchar)(240-125*lid)});
        LockSpriteSpark(16,20,2,(Color){255,218,160,255});
        return;
    }
    if(designId==13) {
        int spread=(int)(lid*5.0f);
        LockSpriteBox(6,9,20,20,(Color){29,45,56,255});
        LockSpriteBox(10,11,12,15,(Color){4,15,31,255});
        LockSpriteBox(15,10,2,18,(Color){accent.r,accent.g,accent.b,200});
        LockSpriteTri(16-spread,3,6-spread,10,16-spread,13,(Color){111,145,147,255});
        LockSpriteTri(16+spread,3,26+spread,10,16+spread,13,(Color){80,120,128,255});
        LockSpriteBox(5-spread,12,8,15,(Color){60,89,101,255});
        LockSpriteBox(19+spread,12,8,15,(Color){46,73,88,255});
        LockSpriteDiamond(16,18-(int)(lid*3),3,(Color){229,250,210,255});
        return;
    }
    if(designId==14) {
        LockSpriteRing(16,16,13,2,(Color){163,145,177,255});
        for(int i=0;i<6;i++) {
            float a=i*PI/3.0f+progress*0.3f;
            LockSpriteDiamond(16+cosf(a)*(7+lid*5),
                              16+sinf(a)*(7+lid*5),3,(Color){158,126,183,255});
        }
        LockSpriteDot(16,16,4+lid*6,(Color){8,10,30,255});
        LockSpriteRing(16,16,5+lid*6,1,accent);
        LockSpriteSpark(16,16,2,(Color){255,244,210,255});
        return;
    }
    if(designId==15) {
        int spread=(int)(lid*5.0f);
        LockSpriteBox(9,4,14,25,(Color){31,20,51,255});
        LockSpriteBox(12,6,8,20,(Color){11,9,35,255});
        LockSpriteLine(7-spread,3,7-spread,29,2,(Color){134,83,91,255});
        LockSpriteLine(25+spread,3,25+spread,29,2,(Color){134,83,91,255});
        LockSpriteLine(6-spread,5,13-spread,14,2,(Color){229,177,137,255});
        LockSpriteLine(26+spread,5,19+spread,14,2,(Color){229,177,137,255});
        LockSpriteLine(6-spread,27,13-spread,18,2,(Color){229,177,137,255});
        LockSpriteLine(26+spread,27,19+spread,18,2,(Color){229,177,137,255});
        LockSpriteDiamond(16,16,4+(int)(lid*3),accent);
        return;
    }
    LockSpriteBox(4,16,24,13,(Color){31,25,39,255});
    LockSpriteBox(6,18,20,7,(Color){9,9,20,255});
    LockSpriteBox(5,24,22,4,(Color){76,57,65,255});
    LockSpriteBox(4,23,24,2,(Color){173,128,87,255});
    LockSpriteBox(4,5-lift,24,7,(Color){106,78,82,255});
    LockSpriteBox(5,5-lift,22,2,(Color){218,177,121,255});
    LockSpriteBox(6,8-lift,20,2,(Color){55,42,60,255});
    LockSpriteLine(5,5-lift,3,2-lift,1,accent);
    LockSpriteLine(27,5-lift,29,2-lift,1,accent);
    LockSpriteBox(8,18,16,4,(Color){accent.r,accent.g,accent.b,185});
    LockSpriteDiamond(16,(int)(20.0f-lid*5.0f),4,accent);
    LockSpriteDiamond(16,(int)(20.0f-lid*5.0f),2,(Color){255,245,211,255});
}

static void DrawLockedObjectUnlockFx(float progress,float t,Color accent) {
    float ringRadius=3.0f+progress*13.0f;
    uchar alpha=(uchar)(180.0f*(1.0f-progress));
    LockSpriteRing(16,19,ringRadius,1,(Color){accent.r,accent.g,accent.b,alpha});
    LockSpriteSpark(16,19,2,(Color){255,246,208,255});
    for(int i=0;i<4;i++) {
        float a=t*7.0f+i*PI/2.0f;
        float r=5.0f+progress*13.0f;
        float x=16.0f+cosf(a)*r;
        float y=19.0f+sinf(a)*r*0.72f-progress*4.0f;
        LockSpriteSpark(x,y,1,(Color){accent.r,accent.g,accent.b,
                                      (uchar)(220.0f*(1.0f-progress))});
    }
}

bool DrawLockedObjectAt(int x,int y,int wx,int wy,float t) {
    LockedObjectState *obj=NULL;
    for(int i=0;i<lockedObjectCount;i++)
        if(lockedObjects[i].x==x&&lockedObjects[i].y==y) {obj=&lockedObjects[i];break;}
    if(!obj)return false;
    float progress=-1.0f,zoom=1.0f,shake=0.0f;
    if(obj->opening) {
        progress=(float)((GetTime()-obj->openingStartedAt)/LOCKED_OBJECT_OPEN_SECONDS);
        if(progress<0.0f)progress=0.0f;
        if(progress>1.0f)progress=1.0f;
        shake=progress<0.22f?sinf(progress*57.0f)*(1.0f-progress/0.22f)*1.1f:0.0f;
        zoom=progress<0.30f?1.0f+0.045f*sinf(progress*20.0f):
             1.0f-0.92f*((progress-0.30f)/0.70f);
        if(zoom<0.08f)zoom=0.08f;
    }
    float visualScale=1.0f;
    if(ChallengeRunActive()&&
       (obj->designId==DRAGONBONE_CASKET_DESIGN_ID||
        obj->designId==OBSIDIAN_SUN_BOX_DESIGN_ID))visualScale=1.18f;
    else if(obj->designId==PORTAL_ECLIPSE_IRIS_SEAL_DESIGN_ID)visualScale=1.12f;
    lockSprite=(LockSpriteContext){(float)wx,(float)wy,(float)CELL/32.0f,
                                   zoom*visualScale,shake};
    Color accent=lockedObjectAccent[obj->designId];
    float pulse=photosensitivity?0.5f:0.5f+0.5f*sinf(t*3.2f+x*0.3f+y*0.4f);
    LockSpriteDot(16,20,15,(Color){accent.r,accent.g,accent.b,(uchar)(28+22*pulse)});
    if(progress<0.0f||progress<0.07f)DrawLockedObjectClosed(obj->designId,t);
    else DrawLockedObjectOpened(obj->designId,progress,accent);
    if(progress>=0.0f)DrawLockedObjectUnlockFx(progress,t,accent);
    return true;
}

static bool IsHiddenRoomEntryTile(int x,int y) {
    return hiddenEntryX>=0&&x==hiddenEntryX&&y>=hiddenEntryStartY&&y<hiddenTop;
}

static void MapRngStart(unsigned run, int round) {
    unsigned x = run ^ ((unsigned)round * 0x9E3779B9u) ^ 0xA5A5A5A5u;
    if (x == 0u) x = 1u;
    map_rng = x;
    for (int i = 0; i < 4; i++) {
        x = map_rng;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        map_rng = x ? x : 1u;
    }
}

static int MapRand(int min, int max) {
    unsigned x = map_rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    map_rng = x ? x : 1u;
    int span = max - min + 1;
    if (span <= 1) return min;
    return min + (int)(x % (unsigned)span);
}

static int ChooseAssignedLock(int artifactId) {
    LoadArtifactLockAssignments();
    if(artifactId<0||artifactId>=ARTIFACT_COUNT||artifactLockChoiceCount[artifactId]<=0)
        return -1;
    int unseen[LOCKED_DESIGN_COUNT],unseenCount=0;
    for(int i=0;i<artifactLockChoiceCount[artifactId];i++) {
        int designId=artifactLockChoices[artifactId][i];
        if((lockedObjectDiscoveredMask&(1u<<designId))==0)
            unseen[unseenCount++]=designId;
    }
    if(unseenCount>0)return unseen[MapRand(0,unseenCount-1)];
    return artifactLockChoices[artifactId][MapRand(0,artifactLockChoiceCount[artifactId]-1)];
}

static bool ArtifactHasUndiscoveredLock(int artifactId) {
    LoadArtifactLockAssignments();
    for(int i=0;i<artifactLockChoiceCount[artifactId];i++)
        if((lockedObjectDiscoveredMask&
            (1u<<artifactLockChoices[artifactId][i]))==0)return true;
    return false;
}

static int ChooseArtifactForLocation(bool sideRoom) {
    int candidates[ARTIFACT_COUNT],count=0;
    // Discover unseen lock designs first, then prefer uncollected artifacts.
    for(int pass=0;pass<4;pass++) {
        count=0;
        bool requireUnseenLock=pass<2;
        bool requireOwned=pass==1||pass==3;
        for(int id=0;id<ARTIFACT_COUNT;id++)
            if(ArtifactDesignPlayable(id)&&ArtifactAppearsInLocation(id,sideRoom)&&
               (artifactOwned[id]!=0)==requireOwned&&
               (!requireUnseenLock||ArtifactHasUndiscoveredLock(id)))
                candidates[count++]=id;
        if(count>0)return candidates[MapRand(0,count-1)];
    }
    return -1;
}

static int ChooseAssignedKey(unsigned keyMask) {
    int total=0;
    for(int i=0;i<KEY_COUNT;i++)if(keyMask&(1u<<i))total++;
    if(total<=0)return -1;
    int selected=MapRand(0,total-1);
    for(int i=0;i<KEY_COUNT;i++)if(keyMask&(1u<<i)) {
        if(selected--==0)return i;
    }
    return -1;
}

static void SelectArtifactVaultContent(int artifactId) {
    artifactVaultArtifactId=artifactId;
    artifactLockDesignId=ChooseAssignedLock(artifactId);
    artifactLockKeyMask=artifactLockDesignId>=0?lockedKeyMasks[artifactLockDesignId]:0;
    artifactVaultKeyId=ChooseAssignedKey(artifactLockKeyMask);
    artifactHasLockedReward=artifactVaultKeyId>=0;
}

static unsigned MixNewRunSeed(void) {
    unsigned t = (unsigned)time(NULL);
    unsigned r = (unsigned)GetRandomValue(0, 0x7fffffff);
    unsigned c = (unsigned)(GetTime() * 1000000.0);
    unsigned s = t ^ (r * 2654435761u) ^ (c * 1597334677u);
    return s ? s : 0xC0FFEE01u;
}

void StartNewRunSeed(void) {
    run_seed = MixNewRunSeed();
    assignmentsLoaded=false;
    RefreshSelectedCoinPile();
    ResetUniversalKeys();
    ResetLockedObjects();
}

// ─── GENERATE ROUND: NORMAL ───────────────────────────────────────────────────
static void GenNormalRound(void) {
    // Map size grows with roundn (like V1.5). Walls/traps grow with normn only
    // (completed NORMAL floors — boss/challenge don't densify the maze).
    // Coin Rush: denser traps + enemies, more coins.
    if (ChallengeRunActive()) {
        mh = CHALLENGE_MAP_HEIGHT;
        mw = CHALLENGE_MAP_WIDTH;
    } else {
        mh = 13 + (roundn - 1); if (mh > 42) mh = 42;
        mw = 23 + 2 * (roundn - 1); if (mw > 76) mw = 76;
    }
    // Wall chance (#): 9% in round 1, then +1% per completed normal (normn).
    // Coin Rush: 10% + normn (max 30).
    int tp = 9 + normn;  if (tp > 34) tp = 34;
    if (coinRush) { tp = 10 + normn; if (tp > 30) tp = 30; }
    // Trap chance (+): fixed 5% band above walls (like V1.5).
    int xp = tp + 5;
    if (xp > 55) xp = 55;

    if (ChallengeRunActive()) {
        ChallengeGenerateNormalFloor(map, &px, &py, run_seed, roundn);
        // Keep the normal floor traps on the challenge's own wall layout.
        for (int y = 1; y < mh - 1; y++)
        for (int x = 1; x < mw - 1; x++) {
            if (map[y][x] == '.' && !ChallengeRouteSafe(x,y) &&
                MapRand(0, 99) < 9 + roundn*2)
                map[y][x] = '+';
        }
        for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            int sx = px + dx, sy = py + dy;
            if (sx > 0 && sx < mw-1 && sy > 0 && sy < mh-1 && map[sy][sx] == '+')
                map[sy][sx] = '.';
        }
    } else {
        for (int y = 0; y < mh; y++)
        for (int x = 0; x < mw; x++) {
            if (x==0||y==0||x==mw-1||y==mh-1) { map[y][x]='#'; continue; }
            int r = MapRand(0,99);
            map[y][x] = (r < tp) ? '#' : (r < xp) ? '+' : '.';
        }
        px = mw/2; py = mh/2;
    }
    map[py][px] = '.';
    // Clear a small spawn pocket so Coin Rush traps don't spawn-kill.
    if (coinRush) {
        for (int dy=-1; dy<=1; dy++) for (int dx=-1; dx<=1; dx++)
            if (py+dy>0 && py+dy<mh-1 && px+dx>0 && px+dx<mw-1) map[py+dy][px+dx] = '.';
    }

    if (!ChallengeRunActive()) for (;;) {
        int tx = MapRand(1,mw-2), ty = MapRand(1,mh-2);
        if (map[ty][tx]!='#' && !(tx==px&&ty==py)) { map[ty][tx]='T'; break; }
    }

    for (int i = 0; i < MAX_PRT; i++) prts[i].on = false;
    for (int i = 0; i < MAX_PRT; i++) {
        // Coin Rush: rarer free portals — you work for the loot.
        if (!ChallengeRunActive() && MapRand(0, coinRush ? 3 : 1)==0) {
            for (int t = 0; t < 300; t++) {
                int ox = MapRand(1,mw-2), oy = MapRand(1,mh-2);
                if (map[oy][ox]=='.') {
                    prts[i] = (PrtObj){ox, oy, true};
                    map[oy][ox] = 'O'; break;
                }
            }
        }
    }

    for (int i = 0; i < MAX_COI; i++) coins[i].on = false;
    // Normal: base 8–17, +1 coin per round. Coin Rush: twice that many piles.
    int nc = 8 + MapRand(0, 9) + (roundn - 1);
    if (nc < 1) nc = 1;
    if (coinRush) nc *= 2;
    if (nc > MAX_COI) nc = MAX_COI;
    for (int placed=0, t=0; placed<nc && t<1200; t++) {
        int cx = MapRand(1,mw-2), cy = MapRand(1,mh-2);
        if (map[cy][cx]=='.') {
            coins[placed++] = (CoinObj){cx, cy, true};
            map[cy][cx] = '$';
        }
    }

    for (int i = 0; i < MAX_ITM; i++) itms[i].on = false;
    int placed = 0;

    // Coin Rush: always one Gold Touch on the floor at/near the spawn.
    if (coinRush) {
        static const int off[8][2] = {
            {0,1},{1,0},{-1,0},{0,-1},{1,1},{-1,1},{1,-1},{-1,-1}
        };
        bool got = false;
        for (int d = 0; d < 8 && !got; d++) {
            int ix = px + off[d][0], iy = py + off[d][1];
            if (ix < 1 || iy < 1 || ix > mw - 2 || iy > mh - 2) continue;
            if (map[iy][ix] != '.') continue;
            itms[placed++] = (ItmObj){ix, iy, true, IK_GLD};
            map[iy][ix] = 'I';
            got = true;
        }
        // Fallback: put it on the spawn tile itself (player steps off & back to grab).
        if (!got) {
            itms[placed++] = (ItmObj){px, py, true, IK_GLD};
            map[py][px] = 'I';
        }
    }

    // Random floor loot (never includes Gold Touch). Leave room after the CR spawn item.
    int ni = coinRush ? (2 + MapRand(0,1)) : (2 + MapRand(0,1));
    if (placed + ni > MAX_ITM) ni = MAX_ITM - placed;
    for (int t = 0; placed < MAX_ITM && ni > 0 && t < 400; t++) {
        int ix = MapRand(1,mw-2), iy = MapRand(1,mh-2);
        if (map[iy][ix]=='.') {
            itms[placed++] = (ItmObj){ix, iy, true, (IKind)MapRand(0, IK_GLD - 1)};
            map[iy][ix] = 'I';
            ni--;
        }
    }

    for (int i = 0; i < MAX_ENE; i++) enes[i].on = false;
    if (eneon) {
        int ne;
        if (coinRush) {
            ne = 6 + roundn; if (ne > MAX_ENE) ne = MAX_ENE;
        } else {
            ne = ChallengeRunActive() ? 4 + roundn*3 : 1 + roundn/2;
            if (ne > (ChallengeRunActive() ? MAX_ENE : MAX_NORMAL_ENE))
                ne = ChallengeRunActive() ? MAX_ENE : MAX_NORMAL_ENE;
        }
        int minDist = 7;
        for (int i=0, t=0; i<ne && t<800; t++) {
            int ex = MapRand(1,mw-2), ey = MapRand(1,mh-2);
            if ((ChallengeRunActive() ? map[ey][ex]=='.' : map[ey][ex]!='#') &&
                abs(ex-px)+abs(ey-py) > minDist) {
                // Same chase tempo in Normal and Coin Rush
                float cd = 0.65f + MapRand(0, 35)*0.01f;
                enes[i++] = (Ene){(float)ex,(float)ey, 0, cd, true};
            }
        }
    }
}

// ─── GENERATE ROUND: BOSS ARENA (every 10th round) ────────────────────────────
// Fully open arena — no interior walls, just the border. Nowhere to hide:
// dodging the dash and the fire breath is pure movement skill. Spike plates
// and cannons are the only fixtures; no treasure, no portals — the exit
// treasure only appears once the boss is dead (see BossDie).
static void GenBossArena(void) {
    mw = 27; mh = 19;

    for (int y = 0; y < mh; y++)
    for (int x = 0; x < mw; x++)
        map[y][x] = (x==0||y==0||x==mw-1||y==mh-1) ? '#' : '.';

    // Spike plates: the player arms them by stepping on them ('^' -> '*');
    // only armed plates hurt the boss.
    static const int PLT[4][2] = { {13,5}, {13,13}, {4,9}, {22,9} };
    for (int p = 0; p < 4; p++)
        map[PLT[p][1]][PLT[p][0]] = '^';

    // Cannons: stand on one to fire a ball at the boss (see game_boss.c).
    map[mh/2][2]    = 'C';
    map[mh/2][mw-3] = 'C';

    px = mw/2; py = mh-3;       // player south, boss north (SpawnBoss)

    for (int i = 0; i < MAX_PRT; i++) prts[i].on = false;

    for (int i = 0; i < MAX_COI; i++) coins[i].on = false;
    int bossCoins = coinRush ? 36 : 6;
    if (bossCoins > MAX_COI) bossCoins = MAX_COI;
    for (int placed=0, t=0; placed<bossCoins && t<900; t++) {
        int cx = MapRand(1,mw-2), cy = MapRand(1,mh-2);
        if (map[cy][cx]=='.' && !(cx==px&&cy==py) && map[cy][cx]!='^' && map[cy][cx]!='C') {
            coins[placed++] = (CoinObj){cx, cy, true};
            map[cy][cx] = '$';
        }
    }

    // Three items, the first guaranteed to be a sword.
    for (int i = 0; i < MAX_ITM; i++) itms[i].on = false;
    for (int placed=0, t=0; placed<3 && t<400; t++) {
        int ix = MapRand(1,mw-2), iy = MapRand(2,mh-2);
        if (map[iy][ix]=='.' && abs(ix-px)+abs(iy-py) > 3) {
            IKind k = (placed == 0) ? IK_SWD : (IKind)MapRand(0, IK_GLD - 1);
            itms[placed++] = (ItmObj){ix, iy, true, k};
            map[iy][ix] = 'I';
        }
    }

    // Regular enemies stay out of the arena; the boss summons his own adds.
    for (int i = 0; i < MAX_ENE; i++) enes[i].on = false;

    SpawnBoss();
}

// ─── CHALLENGE-ROOM HELPERS ───────────────────────────────────────────────────
// Scatter n coins onto random free floor tiles (clears any prior placement).
static void PlaceCoins(int n) {
    for (int i = 0; i < MAX_COI; i++) coins[i].on = false;
    for (int y = 1; y < mh-1; y++)
    for (int x = 1; x < mw-1; x++)
        if (map[y][x] == '$') map[y][x] = '.';
    if (n > MAX_COI) n = MAX_COI;
    for (int placed=0, t=0; placed<n && t<800; t++) {
        int cx = MapRand(1,mw-2), cy = MapRand(1,mh-2);
        if (map[cy][cx]=='.' && !(cx==px&&cy==py)) {
            coins[placed++] = (CoinObj){cx, cy, true};
            map[cy][cx] = '$';
        }
    }
}

// Scatter n items; kind0 forces the first item's kind (IK_NONE = random).
static void PlaceItems(int n, IKind kind0) {
    for (int i = 0; i < MAX_ITM; i++) itms[i].on = false;
    if (n > MAX_ITM) n = MAX_ITM;
    for (int placed=0, t=0; placed<n && t<600; t++) {
        int ix = MapRand(1,mw-2), iy = MapRand(1,mh-2);
        if (map[iy][ix]=='.' && abs(ix-px)+abs(iy-py) > 3) {
            IKind k = (placed == 0 && kind0 != IK_NONE)
                    ? kind0 : (IKind)MapRand(0, IK_GLD - 1);
            itms[placed++] = (ItmObj){ix, iy, true, k};
            map[iy][ix] = 'I';
        }
    }
}

// ─── GENERATE ROUND: TRAP ROOM (no enemies) ───────────────────────────────────
// Looks like a normal room (scattered walls, current-progression size) but with
// no enemies. The danger is overlaid: plenty of static spike traps '+', static
// lava pools 'L' (they don't spread — that's the lava room's job), and horizontal
// arrow lanes 'r' fed by a dispenser 'D' at the far-left of each lane. Each lane
// fires a synchronized west->east volley; only the brief fire window is lethal,
// after a generous telegraph. Triple coins.
static void GenTrapRound(void) {
    // Same footprint as a normal room of the current round.
    mh = 13 + (roundn - 1); if (mh > 42) mh = 42;
    mw = 23 + 2 * (roundn - 1); if (mw > 76) mw = 76;

    // Organic scattered-wall base (density tracks normn).
    int tp = 10 + normn/2; if (tp > 26) tp = 26;
    for (int y = 0; y < mh; y++)
    for (int x = 0; x < mw; x++) {
        if (x==0||y==0||x==mw-1||y==mh-1) { map[y][x]='#'; continue; }
        map[y][x] = (MapRand(0,99) < tp) ? '#' : '.';
    }

    px = mw/2; py = mh-2;                         // south; treasure far north
    for (int dy=-1; dy<=1; dy++) for (int dx=-1; dx<=1; dx++)
        if (py+dy>0 && py+dy<mh-1 && px+dx>0 && px+dx<mw-1) map[py+dy][px+dx]='.';

    // Arrow lanes: a few full-width rows carved clear. A dispenser 'D' sits in
    // the far-left wall (x=0) and fires arrows east across the 'r' cells. Kept well
    // clear of the spawn row so you never start inside a volley.
    for (int y = 2; y <= mh-4; y += 3 + MapRand(0,1)) {
        if (y >= py-1) break;
        map[y][0] = 'D';                                  // dispenser in left wall
        for (int x = 1; x < mw-1; x++) map[y][x] = 'r';   // the lane it feeds
    }

    // Static spike traps on the remaining floor — a proper minefield.
    // Density grows with normn; Coin Rush starts denser (still leave corridors).
    int spikePct = coinRush ? (22 + normn/2) : (13 + normn/2);
    if (spikePct > 35) spikePct = 35;
    for (int y = 1; y < mh-1; y++)
    for (int x = 1; x < mw-1; x++)
        if (map[y][x]=='.' && MapRand(0,99) < spikePct) map[y][x] = '+';

    // Trapdoors: floor-lookalike hatches that drop you into the void on step.
    // Sparse enough to weave around; denser in Coin Rush trap variants.
    int hatchPct = coinRush ? (9 + normn/4) : (6 + normn/4);
    if (hatchPct > 14) hatchPct = 14;
    for (int y = 1; y < mh-1; y++)
    for (int x = 1; x < mw-1; x++)
        if (map[y][x]=='.' && MapRand(0,99) < hatchPct) map[y][x] = 'h';

    // Static lava pools — a handful of small blobs to weave around.
    int pools = 3 + normn/3;
    for (int p = 0, t = 0; p < pools && t < 300; t++) {
        int lx = MapRand(2, mw-3), ly = MapRand(2, mh-4);
        if (map[ly][lx] != '.') continue;
        int r = MapRand(0,1);
        for (int dy = 0; dy <= r; dy++) for (int dx = 0; dx <= r; dx++) {
            int nx = lx+dx, ny = ly+dy;
            if (nx>0 && nx<mw-1 && ny>0 && ny<mh-1 && map[ny][nx]=='.') map[ny][nx]='L';
        }
        p++;
    }

    // Re-clear the spawn pocket so no overlay landed on top of the player.
    for (int dy=-1; dy<=1; dy++) for (int dx=-1; dx<=1; dx++)
        if (py+dy>0 && py+dy<mh-1 && px+dx>0 && px+dx<mw-1) map[py+dy][px+dx]='.';

    // Always leave a safe exit north to the first arrow lane (nothing substantial:
    // only clear walls/hazards on the spawn column between pocket and that line).
    {
        int firstLaneY = -1;
        for (int y = py - 1; y >= 2; y--) {
            for (int x = 1; x < mw - 1; x++)
                if (map[y][x] == 'r' || map[y][x] == 'D') { firstLaneY = y; break; }
            if (firstLaneY >= 0) break;
        }
        // Path tiles: just south of the first lane, up through spawn.
        int yStop = (firstLaneY >= 0) ? (firstLaneY + 1) : 2;
        if (yStop < 1) yStop = 1;
        for (int y = py; y >= yStop; y--) {
            char c = map[y][px];
            if (c == '#' || c == '+' || c == 'L' || c == 'h')
                map[y][px] = '.';
        }
    }

    for (int t = 0; ; t++) {
        int tx = MapRand(1,mw-2), ty = MapRand(1,2);
        if (map[ty][tx]=='.') { map[ty][tx]='T'; break; }
        if (t > 300) { map[1][mw/2] = 'T'; break; }
    }

    // No portals in a trap room — you don't get to skip the gauntlet.
    for (int i = 0; i < MAX_PRT; i++) prts[i].on = false;

    PlaceCoins(coinRush ? (50 + MapRand(0, 16) + roundn)
                        : (24 + MapRand(0, 8)));
    PlaceItems(2, IK_NONE);
    for (int i = 0; i < MAX_ENE; i++) enes[i].on = false;
}

// ─── GENERATE ROUND: DARK ROOM ────────────────────────────────────────────────
// Same maze + enemy count as Normal, under fog (DrawDarkness). Coins use the
// Normal formula with base 20 instead of 8: 20 + 0..9 + (roundn-1).
static void GenDarkRound(void) {
    GenNormalRound();
    // Enemies left as GenNormalRound placed them (same as Normal).
    int nc = 20 + MapRand(0, 9) + (roundn - 1);
    if (nc < 1) nc = 1;
    if (nc > MAX_COI) nc = MAX_COI;
    PlaceCoins(nc);
}

// ─── GENERATE ROUND: THE FLOOR IS LAVA (no enemies) ───────────────────────────
// Looks like a normal room (scattered walls, current-progression size), no
// enemies — but the lava floods up from the south each tick (UpdLava). It
// converts every adjacent floor/trap/coin/item tile; walls block it, so the
// scattered walls form brief safe pockets on the way north. Spawn south, treasure
// north, a light spike scatter, and a guaranteed speed item to outrun the front.
static void GenLavaRound(void) {
    mh = 13 + (roundn - 1); if (mh > 42) mh = 42;
    mw = 23 + 2 * (roundn - 1); if (mw > 76) mw = 76;

    int tp = 10 + normn/2; if (tp > 24) tp = 24;
    for (int y = 0; y < mh; y++)
    for (int x = 0; x < mw; x++) {
        if (x==0||y==0||x==mw-1||y==mh-1) { map[y][x]='#'; continue; }
        map[y][x] = (MapRand(0,99) < tp) ? '#' : '.';
    }

    // Light spike scatter so the flee north isn't a straight sprint.
    int lavaSpikes = coinRush ? (9 + normn/3) : (4 + normn/3);
    if (lavaSpikes > 18) lavaSpikes = 18;
    for (int y = 1; y < mh-1; y++)
    for (int x = 1; x < mw-1; x++)
        if (map[y][x]=='.' && MapRand(0,99) < lavaSpikes) map[y][x] = '+';

    px = mw/2; py = mh-3;
    for (int dy=-1; dy<=1; dy++) for (int dx=-1; dx<=1; dx++)
        if (py+dy>0 && py+dy<mh-1 && px+dx>0 && px+dx<mw-1) map[py+dy][px+dx] = '.';

    // Lava seeds: full south row + the lowest stretch of both flanks. The front
    // creeps up from here (see UpdLava). Seeded AFTER the pocket clear so the
    // south row below the spawn is already hot.
    for (int x = 1; x < mw-1; x++) if (map[mh-2][x] != '#') map[mh-2][x] = 'L';
    for (int k = 0; k < 3; k++) {
        if (map[mh-3-k][1]    == '.') map[mh-3-k][1]    = 'L';
        if (map[mh-3-k][mw-2] == '.') map[mh-3-k][mw-2] = 'L';
    }
    lavaTick = 1.2f - 0.03f * (roundn/10);
    if (lavaTick < 0.85f) lavaTick = 0.85f;

    for (int t = 0; ; t++) {
        int tx = MapRand(1,mw-2), ty = MapRand(1,2);
        if (map[ty][tx]=='.') { map[ty][tx]='T'; break; }
        if (t > 300) { map[1][mw/2] = 'T'; break; }
    }
    // No portals — the only way out is the run north to the treasure.
    for (int i = 0; i < MAX_PRT; i++) prts[i].on = false;

    PlaceCoins(coinRush ? (50 + MapRand(0, 16) + roundn)
                        : (24 + MapRand(0, 8)));
    PlaceItems(2, IK_SPD);                        // speed is the key item here
    for (int i = 0; i < MAX_ENE; i++) enes[i].on = false;
}

// ─── REACHABILITY GUARANTEE ───────────────────────────────────────────────────
// After generation, make sure the treasure (and, in a lava room, the guaranteed
// speed item) can actually be reached from the spawn. BFS over everything that
// isn't a wall/dispenser — traps and lava are hazards, not topology. Any
// walled-in target gets an L-shaped corridor carved to it: rare, allowed to be
// ugly, because the alternative is a soft-lock (a guaranteed death in a lava
// room, where the advancing front punishes a dead end with no way through).
static void EnsureReachable(void) {
    static bool seen[MAP_ROWS][MAP_COLS];
    static int  qx[MAP_ROWS*MAP_COLS], qy[MAP_ROWS*MAP_COLS];
    memset(seen, 0, sizeof(seen));
    int qn = 0, qh = 0;
    qx[qn] = px; qy[qn] = py; qn++; seen[py][px] = true;
    while (qh < qn) {
        int x = qx[qh], y = qy[qh]; qh++;
        static const int D4[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
        for (int d = 0; d < 4; d++) {
            int nx = x + D4[d][0], ny = y + D4[d][1];
            if (nx < 1 || ny < 1 || nx > mw-2 || ny > mh-2) continue;
            if (seen[ny][nx] || map[ny][nx] == ' ' || map[ny][nx] == '#' || map[ny][nx] == 'D') continue;
            seen[ny][nx] = true;
            qx[qn] = nx; qy[qn] = ny; qn++;
        }
    }

    for (int ty = 1; ty < mh-1; ty++)
    for (int tx = 1; tx < mw-1; tx++) {
        bool target = (map[ty][tx] == 'T') || (map[ty][tx] == 'M') ||
                      (map[ty][tx] == 'l') ||
                      (roomKind == RK_LAVA && itms[0].on &&
                       itms[0].x == tx && itms[0].y == ty);
        if (!target || seen[ty][tx]) continue;
        // Carve an L: down the spawn's column, then along the target's row.
        // Only walls fall — hazards/coins on the path stay as they are.
        int step = (ty > py) ? 1 : -1;
        for (int y = py; y != ty; y += step)
            if (map[y][px] == '#' || map[y][px] == 'D') map[y][px] = '.';
        step = (tx > px) ? 1 : -1;
        for (int x = px; x != tx; x += step)
            if (map[ty][x] == '#' || map[ty][x] == 'D') map[ty][x] = '.';
    }
}

// ─── GENERATE ROUND ───────────────────────────────────────────────────────────
// Loot uses the existing pickup arrays, including magnet/item interactions.
static bool PlaceRoomCoin(int x,int y) {
    if(map[y][x]!='.')return false;
    for(int i=0;i<MAX_COI;i++)if(!coins[i].on) {
        coins[i]=(CoinObj){x,y,true};map[y][x]='$';return true;
    }
    return false;
}
static bool PlaceRoomItem(int x,int y,IKind kind) {
    if(map[y][x]!='.')return false;
    for(int i=0;i<MAX_ITM;i++)if(!itms[i].on) {
        itms[i]=(ItmObj){x,y,true,kind};map[y][x]='I';return true;
    }
    return false;
}

// Randomized depth-first maze generation. Every room starts from walls and
// receives its own seeded topology, then gets a few randomly carved chambers.
static void GenerateRandomMaze(int xMin,int yMin,int xMax,int yMax,int *rootX,int *rootY) {
    int firstX=xMin+1,firstY=yMin+1;
    int cols=(xMax-1-firstX)/2+1,rows=(yMax-1-firstY)/2+1;
    if(cols<1||rows<1)return;
    for(int y=yMin+1;y<yMax;y++)for(int x=xMin+1;x<xMax;x++)map[y][x]='#';

    int rx=(*rootX-firstX+1)/2,ry=(*rootY-firstY+1)/2;
    if(rx<0)rx=0;
    if(rx>=cols)rx=cols-1;
    if(ry<0)ry=0;
    if(ry>=rows)ry=rows-1;
    int sx=firstX+rx*2,sy=firstY+ry*2;
    bool visited[MAP_ROWS][MAP_COLS]={{false}};
    int stackX[MAP_ROWS*MAP_COLS],stackY[MAP_ROWS*MAP_COLS],top=0;
    visited[sy][sx]=true;map[sy][sx]='.';
    stackX[top]=sx;stackY[top++]=sy;
    static const int dx[4]={0,1,0,-1},dy[4]={-1,0,1,0};
    while(top>0) {
        int x=stackX[top-1],y=stackY[top-1],order[4]={0,1,2,3};
        for(int i=3;i>0;i--) {int j=MapRand(0,i),v=order[i];order[i]=order[j];order[j]=v;}
        bool advanced=false;
        for(int i=0;i<4;i++) {
            int d=order[i],nx=x+dx[d]*2,ny=y+dy[d]*2;
            if(nx<firstX||ny<firstY||nx>=firstX+cols*2||ny>=firstY+rows*2||visited[ny][nx])continue;
            map[y+dy[d]][x+dx[d]]='.';map[ny][nx]='.';visited[ny][nx]=true;
            stackX[top]=nx;stackY[top++]=ny;advanced=true;break;
        }
        if(!advanced)top--;
    }
    *rootX=sx;*rootY=sy;
}

static bool MazeOpen(int x,int y) {
    return map[y][x]!='#'&&map[y][x]!=' '&&map[y][x]!='D';
}
static int MazeFlood(int sx,int sy,int xMin,int yMin,int xMax,int yMax) {
    for(int y=yMin;y<=yMax;y++)for(int x=xMin;x<=xMax;x++)mazeDistance[y][x]=-1;
    if(sx<xMin||sy<yMin||sx>xMax||sy>yMax||!MazeOpen(sx,sy))return 0;
    int head=0,tail=0,maxDistance=0;
    mazeDistance[sy][sx]=0;mazePrevX[sy][sx]=-1;mazePrevY[sy][sx]=-1;
    mazeQueueX[tail]=sx;mazeQueueY[tail++]=sy;
    static const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
    while(head<tail) {
        int x=mazeQueueX[head],y=mazeQueueY[head++];
        for(int d=0;d<4;d++) {
            int nx=x+dx[d],ny=y+dy[d];
            if(nx<xMin||ny<yMin||nx>xMax||ny>yMax||mazeDistance[ny][nx]>=0||!MazeOpen(nx,ny))continue;
            mazeDistance[ny][nx]=mazeDistance[y][x]+1;
            mazePrevX[ny][nx]=(short)x;mazePrevY[ny][nx]=(short)y;
            if(mazeDistance[ny][nx]>maxDistance)maxDistance=mazeDistance[ny][nx];
            mazeQueueX[tail]=nx;mazeQueueY[tail++]=ny;
        }
    }
    return maxDistance;
}

static bool PlaceSideRoomKey(int mainW,int mainH) {
    MazeFlood(px,py,1,1,mainW-2,mainH-2);
    int keyX=-1,keyY=-1,count=0;
    for(int y=1;y<mainH-1;y++)for(int x=1;x<mainW-1;x++) {
        bool occupied=false;
        for(int i=0;i<MAX_ENE;i++)
            if(enes[i].on&&(int)(enes[i].x+0.5f)==x&&
               (int)(enes[i].y+0.5f)==y) {occupied=true;break;}
        if(map[y][x]=='.'&&mazeDistance[y][x]>0&&!occupied) {
            count++;
            if(MapRand(1,count)==1){keyX=x;keyY=y;}
        }
    }

    if(keyX<0)return false;
    map[keyY][keyX]=UniversalKeyTileFromId(KEY_TUTORIAL_BOOK);
    hiddenKeyX=keyX;hiddenKeyY=keyY;
    hiddenRoomHasKey=false;
    return true;
}

static bool MazeChooseFarthest(int xMin,int yMin,int xMax,int yMax,int minDistance,int *outX,int *outY) {
    int maxDistance=0;
    for(int y=yMin;y<=yMax;y++)for(int x=xMin;x<=xMax;x++)
        if(mazeDistance[y][x]>maxDistance)maxDistance=mazeDistance[y][x];
    if(minDistance>maxDistance)minDistance=maxDistance;
    int wiggle=maxDistance/7;if(wiggle>12)wiggle=12;
    int threshold=maxDistance-MapRand(0,wiggle);
    if(threshold<minDistance)threshold=minDistance;
    int seen=0;
    for(int y=yMin;y<=yMax;y++)for(int x=xMin;x<=xMax;x++)
        if(mazeDistance[y][x]>=threshold&&mazeDistance[y][x]>=minDistance) {
            seen++;
            if(MapRand(1,seen)==1){*outX=x;*outY=y;}
    }
    return seen>0;
}

static bool MazeChooseFarthestLowerHalf(int xMin,int yMin,int xMax,int yMax,
                                        int minDistance,int *outX,int *outY) {
    int lowerMinY=yMin+(yMax-yMin+1)/2;
    int maxDistance=-1;
    for(int y=lowerMinY;y<=yMax;y++)for(int x=xMin;x<=xMax;x++)
        if(map[y][x]=='.'&&mazeDistance[y][x]>=0&&mazeDistance[y][x]>maxDistance)
            maxDistance=mazeDistance[y][x];
    if(maxDistance<0) {
        int fallbackX=-1,fallbackY=-1,fallbackDistance=-1;
        for(int y=lowerMinY;y<=yMax;y++)for(int x=xMin;x<=xMax;x++)
            if(MazeOpen(x,y)&&mazeDistance[y][x]>fallbackDistance) {
                fallbackX=x;fallbackY=y;fallbackDistance=mazeDistance[y][x];
            }
        if(fallbackX<0)return false;
        *outX=fallbackX;*outY=fallbackY;
        map[fallbackY][fallbackX]='.';
        return true;
    }
    if(minDistance>maxDistance)minDistance=maxDistance;
    int wiggle=maxDistance/7;if(wiggle>12)wiggle=12;
    int threshold=maxDistance-MapRand(0,wiggle);
    if(threshold<minDistance)threshold=minDistance;
    int seen=0;
    for(int y=lowerMinY;y<=yMax;y++)for(int x=xMin;x<=xMax;x++)
        if(map[y][x]=='.'&&mazeDistance[y][x]>=threshold&&
           mazeDistance[y][x]>=minDistance) {
            seen++;
            if(MapRand(1,seen)==1){*outX=x;*outY=y;}
        }
    return seen>0;
}

static void MazeMarkPath(int x,int y) {
    for(int guard=0;guard<MAP_ROWS*MAP_COLS;guard++) {
        artifactRoute[y][x]=true;
        int px0=mazePrevX[y][x],py0=mazePrevY[y][x];
        if(px0<0||py0<0)break;
        x=px0;y=py0;
    }
}

static void AddMazeFeatures(int xMin,int yMin,int xMax,int yMax,int chambers,int loops) {
    int opened=0;
    for(int tries=0;tries<loops*8&&opened<loops;tries++) {
        int x=MapRand(xMin,xMax),y=MapRand(yMin,yMax);
        if(map[y][x]!='#')continue;
        bool horizontal=MazeOpen(x-1,y)&&MazeOpen(x+1,y);
        bool vertical=MazeOpen(x,y-1)&&MazeOpen(x,y+1);
        if(horizontal||vertical){map[y][x]='.';opened++;}
    }
    for(int r=0;r<chambers;r++) {
        int ax=-1,ay=-1;
        for(int tries=0;tries<100;tries++) {
            int x=MapRand(xMin,xMax),y=MapRand(yMin,yMax);
            if(MazeOpen(x,y)){ax=x;ay=y;break;}
        }
        if(ax<0)continue;
        int rx=MapRand(2,5),ry=MapRand(1,4);
        for(int y=ay-ry;y<=ay+ry;y++)for(int x=ax-rx;x<=ax+rx;x++) {
            if(x<=xMin||y<=yMin||x>=xMax||y>=yMax)continue;
            int dx=x-ax,dy=y-ay;
            if(dx*dx*ry*ry+dy*dy*rx*rx<=rx*rx*ry*ry)map[y][x]='.';
        }
        map[ay][ax]='.';
    }
}

// Side rooms keep a denser maze core, with a few smaller chambers mixed in.
static void AddHiddenRoomMazeFeatures(int xMin,int yMin,int xMax,int yMax) {
    int loops=18+MapRand(0,17),opened=0;
    for(int tries=0;tries<loops*10&&opened<loops;tries++) {
        int x=MapRand(xMin,xMax),y=MapRand(yMin,yMax);
        if(map[y][x]!='#')continue;
        bool horizontal=MazeOpen(x-1,y)&&MazeOpen(x+1,y);
        bool vertical=MazeOpen(x,y-1)&&MazeOpen(x,y+1);
        if(horizontal||vertical){map[y][x]='.';opened++;}
    }

    int chambers=5+MapRand(0,5);
    for(int r=0;r<chambers;r++) {
        int ax=-1,ay=-1;
        for(int tries=0;tries<100;tries++) {
            int x=MapRand(xMin,xMax),y=MapRand(yMin,yMax);
            if(MazeOpen(x,y)){ax=x;ay=y;break;}
        }
        if(ax<0)continue;
        int rx=2+MapRand(0,2),ry=1+MapRand(0,2);
        for(int y=ay-ry;y<=ay+ry;y++)for(int x=ax-rx;x<=ax+rx;x++) {
            if(x<=xMin||y<=yMin||x>=xMax||y>=yMax)continue;
            int dx=x-ax,dy=y-ay;
            if(dx*dx*ry*ry+dy*dy*rx*rx<=rx*rx*ry*ry)map[y][x]='.';
        }
        map[ay][ax]='.';
    }
}

static void ShuffleRoomTiles(int xs[],int ys[],int count) {
    for(int i=count-1;i>0;i--) {
        int j=MapRand(0,i),v=xs[i];xs[i]=xs[j];xs[j]=v;
        v=ys[i];ys[i]=ys[j];ys[j]=v;
    }
}

static int RoomCandidates(int xMin,int yMin,int xMax,int yMax,int xs[],int ys[]) {
    int count=0;
    for(int y=yMin;y<=yMax;y++)for(int x=xMin;x<=xMax;x++)
        if(map[y][x]=='.'&&!artifactRoute[y][x]){xs[count]=x;ys[count++]=y;}
    return count;
}

static int PlaceLavaPools(int xs[],int ys[],int count,int goal) {
    int placed=0;
    for(int i=0;i<count&&placed<goal;i++) {
        int x=xs[i],y=ys[i];
        if(map[y][x]!='.'||MapRand(0,99)>=42)continue;
        map[y][x]='L';placed++;
        int spread=MapRand(1,5);
        for(int k=0;k<spread&&placed<goal;k++) {
            int nx=x+MapRand(-2,2),ny=y+MapRand(-2,2);
            if(nx>0&&ny>0&&nx<mw-1&&ny<mh-1&&
               map[ny][nx]=='.'&&!artifactRoute[ny][nx]){map[ny][nx]='L';placed++;}
        }
    }
    for(int i=0;i<count&&placed<goal;i++)
        if(map[ys[i]][xs[i]]=='.'){map[ys[i]][xs[i]]='L';placed++;}
    return placed;
}

static int PlaceRoomHazard(char tile,int goal,int xs[],int ys[],int count) {
    int placed=0;
    for(int i=count-1;i>=0&&placed<goal;i--)
        if(map[ys[i]][xs[i]]=='.'){map[ys[i]][xs[i]]=tile;placed++;}
    return placed;
}

static bool GeneratedKeyReachable[MAP_ROWS][MAP_COLS];
static int generatedKeyQX[MAP_ROWS*MAP_COLS],generatedKeyQY[MAP_ROWS*MAP_COLS];
static void FloodGeneratedKeyReachability(void) {
    memset(GeneratedKeyReachable,0,sizeof(GeneratedKeyReachable));
    if(px<1||py<1||px>=mw-1||py>=mh-1)return;
    int head=0,tail=0;
    generatedKeyQX[tail]=px;generatedKeyQY[tail++]=py;
    GeneratedKeyReachable[py][px]=true;
    const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
    while(head<tail) {
        int x=generatedKeyQX[head],y=generatedKeyQY[head++];
        for(int d=0;d<4;d++) {
            int nx=x+dx[d],ny=y+dy[d];
            if(nx<1||ny<1||nx>=mw-1||ny>=mh-1||GeneratedKeyReachable[ny][nx])continue;
            char c=map[ny][nx];
            if(c==' '||c=='#'||c=='D'||c=='Q')continue;
            GeneratedKeyReachable[ny][nx]=true;
            generatedKeyQX[tail]=nx;generatedKeyQY[tail++]=ny;
        }
    }
}

static bool PlaceGeneratedKey(int keyId,int xMin,int yMin,int xMax,int yMax,
                              int avoidX,int avoidY,int minAvoidDistance,
                              bool preferArtifactRoute,int minSpawnDistance) {
    if(keyId<0||keyId>=KEY_COUNT||UniversalKeyTileFromId(keyId)=='\0')return false;
    FloodGeneratedKeyReachability();
    for(int pass=0;pass<(preferArtifactRoute?2:1);pass++) {
        bool requireRoute=preferArtifactRoute&&pass==0;
        int chosenX=-1,chosenY=-1,seen=0;
        for(int y=yMin;y<=yMax;y++)for(int x=xMin;x<=xMax;x++) {
            if(x<1||y<1||x>=mw-1||y>=mh-1||map[y][x]!='.'||
               !GeneratedKeyReachable[y][x])continue;
            if(abs(x-px)+abs(y-py)<minSpawnDistance)continue;
            if(avoidX>=0&&abs(x-avoidX)+abs(y-avoidY)<minAvoidDistance)continue;
            if(requireRoute&&!artifactRoute[y][x])continue;
            bool occupied=false;
            for(int i=0;i<MAX_ENE;i++)if(enes[i].on&&
               (int)(enes[i].x+0.5f)==x&&(int)(enes[i].y+0.5f)==y) {occupied=true;break;}
            if(occupied)continue;
            seen++;
            if(MapRand(1,seen)==1){chosenX=x;chosenY=y;}
        }
        if(chosenX>=0) {
            map[chosenY][chosenX]=UniversalKeyTileFromId(keyId);
            return true;
        }
    }
    return false;
}

// Generated after enemies: the larger side shrine has a maze entrance and no
// initial enemy spawns. Its entry gate may block both the player and enemies.
static void AddHiddenRoom(void) {
    if(roomKind!=RK_NORMAL && roomKind!=RK_TRAP)return;
    int oldH=mh,oldW=mw;
    int roomH=34+MapRand(0,10),roomW=38+MapRand(0,12);
    if(oldH+roomH>MAP_ROWS)roomH=MAP_ROWS-oldH;
    if(roomH<16)return;
    int expandedW=oldW;
    if(expandedW<roomW+4)expandedW=roomW+4;
    if(expandedW>MAP_COLS-1)return;
    int maxLeft=expandedW-roomW-2;
    int left=MapRand(2,maxLeft),right=left+roomW;
    int doorLo=left+2,doorHi=right-3;
    if(doorHi>oldW-2)doorHi=oldW-2;
    if(doorLo>doorHi)return;
    int doorSteps=(doorHi-doorLo)/2;
    int doorX=doorLo+2*MapRand(0,doorSteps);
    int entryLen=5+MapRand(0,4);
    int entryEndY=oldH+entryLen-1;
    int roomTop=entryEndY+1;
    int roomHLimit=MAP_ROWS-roomTop-2;
    if(roomH>roomHLimit)roomH=roomHLimit;
    if(roomH<16)return;
    hiddenLeft=left;hiddenTop=roomTop+1;hiddenMainWidth=oldW;
    hiddenWidth=roomW;hiddenHeight=roomH;
    hiddenEntryX=doorX;hiddenEntryStartY=oldH-1;
    for(int y=0;y<oldH;y++)for(int x=oldW;x<expandedW;x++)map[y][x]=' ';
    mw=expandedW;
    mh=roomTop+roomH+2;
    for(int y=oldH;y<mh;y++)for(int x=0;x<mw;x++)map[y][x]=' ';
    for(int y=roomTop;y<mh;y++){map[y][left-1]='#';map[y][right]='#';}
    for(int x=left-1;x<=right;x++){map[roomTop][x]='#';map[mh-1][x]='#';}
    map[oldH-1][doorX]='.';
    map[roomTop][doorX]='.';
    for(int y=py;y<oldH-1;y++)
        if(map[y][px]=='#'||map[y][px]=='D'||map[y][px]==' ')map[y][px]='.';
    int lo=px<doorX?px:doorX,hi=px>doorX?px:doorX;
    for(int x=lo;x<=hi;x++)
        if(map[oldH-2][x]=='#'||map[oldH-2][x]=='D'||map[oldH-2][x]==' ')map[oldH-2][x]='.';

    int rootX=doorX,rootY=roomTop+1;
    GenerateRandomMaze(left-1,roomTop,right,mh-1,&rootX,&rootY);
    int xMin=left,yMin=roomTop+1,xMax=right-1,yMax=mh-2;
    AddHiddenRoomMazeFeatures(xMin,yMin,xMax,yMax);
    int sweepRows[2]={-1,-1};
    int sweepGoal=1+MapRand(0,1),sweepsPlaced=0;
    for(int tries=0;tries<100&&sweepsPlaced<sweepGoal;tries++) {
        int y=MapRand(yMin+3,yMax-3);
        if(abs(y-rootY)<6)continue;
        bool spaced=true;
        for(int i=0;i<sweepsPlaced;i++)
            if(abs(y-sweepRows[i])<6){spaced=false;break;}
        if(!spaced)continue;
        map[y][xMin-1]='D';
        for(int x=xMin;x<=xMax;x++)map[y][x]='r';
        sweepRows[sweepsPlaced++]=y;
    }
    for(int y=oldH;y<=entryEndY;y++) {
        map[y][doorX]='.';
        map[y][doorX-1]='#';map[y][doorX+1]='#';
    }
    memset(artifactRoute,0,sizeof(artifactRoute));
    MazeFlood(rootX,rootY,xMin,yMin,xMax,yMax);
    int targetX=rootX,targetY=rootY;
    MazeChooseFarthestLowerHalf(xMin,yMin,xMax,yMax,20,&targetX,&targetY);
    MazeMarkPath(targetX,targetY);
    for(int y=oldH-1;y<=rootY;y++)artifactRoute[y][doorX]=true;

    int xs[MAP_ROWS*MAP_COLS],ys[MAP_ROWS*MAP_COLS];
    int candidates=RoomCandidates(xMin,yMin,xMax,yMax,xs,ys);
    ShuffleRoomTiles(xs,ys,candidates);
    PlaceLavaPools(xs,ys,candidates,16+MapRand(0,17));
    PlaceRoomHazard('+',17+MapRand(0,15),xs,ys,candidates);
    PlaceRoomHazard('h',5+MapRand(0,7),xs,ys,candidates);

    floorArtifact=ChooseArtifactForLocation(true);
    for(int i=0,tries=0;i<28&&tries<1800;tries++) {
        int x=MapRand(xMin,xMax),y=MapRand(yMin,yMax);
        if((x==rootX&&y==rootY)||(x==targetX&&y==targetY))continue;
        if(PlaceRoomCoin(x,y))i++;
    }
    for(int i=0,tries=0;i<2&&tries<700;tries++) {
        int x=MapRand(xMin,xMax),y=MapRand(yMin,yMax);
        if((x==rootX&&y==rootY)||(x==targetX&&y==targetY))continue;
        if(PlaceRoomItem(x,y,(IKind)MapRand(0,IK_GLD-1)))break;
    }
    char rewardTile=floorArtifact<0||artifactOwned[floorArtifact]?'%':'A';
    map[targetY][targetX]=rewardTile;
    int designId=ChooseAssignedLock(floorArtifact);
    if(designId>=0) {
        unsigned keyMask=lockedKeyMasks[designId];
        int keyId=ChooseAssignedKey(keyMask);
        if(keyId>=0&&
           !PlaceGeneratedKey(keyId,xMin,yMin,xMax,yMax,
                              targetX,targetY,6,true,4))
            PlaceGeneratedKey(keyId,xMin,yMin,xMax,yMax,
                              targetX,targetY,0,false,0);
        RegisterLockedObjectWithKeyMaskAt(targetX,targetY,designId,keyMask,rewardTile);
    }
    if(MapRand(1,100)<=50&&PlaceSideRoomKey(oldW,oldH)) {
        hiddenGateX=doorX;hiddenGateY=oldH-1;
        map[hiddenGateY][hiddenGateX]='G';
    }
}

static int artifactExitX,artifactExitY;
static int artifactLadderX=-1,artifactLadderY=-1;
void RevealArtifactTreasure(void) {
    if(artifactExitX<0||artifactExitY<0)return;
    map[artifactExitY][artifactExitX]='T';
}
static unsigned artifactRoomSerial=0u;
void ResetSeededArtifactSequence(void){artifactRoomSerial=0u;}
static unsigned artifactRoomSeed=1u;

// The first portal section is an open, enemy-heavy arena, not a maze.
static void GenArtifactRoom(void) {
    artifactRoomSerial++;
    unsigned seed=run_seed ^ map_rng ^ ((unsigned)roundn*0x85EBCA6Bu) ^
                  (artifactRoomSerial*0x9E3779B9u) ^ 0xA17F39D1u;
    if(seed==0u)seed=0xC0FFEE01u;
    artifactRoomSeed=seed;
    artifactRoomPart=0;
    artifactFirstRoomKeyCollected=false;
    artifactExitX=artifactExitY=-1;
    MapRngStart(seed,(int)(artifactRoomSerial^(seed>>16)));
    SelectArtifactVaultContent(ChooseArtifactForLocation(false));

    mw=55+MapRand(0,20);mh=37+MapRand(0,12);
    memset(map,' ',sizeof(map));memset(enes,0,sizeof(enes));
    memset(coins,0,sizeof(coins));memset(itms,0,sizeof(itms));memset(prts,0,sizeof(prts));
    memset(artifactRoute,0,sizeof(artifactRoute));
    for(int y=0;y<mh;y++)for(int x=0;x<mw;x++) {
        if(x==0||y==0||x==mw-1||y==mh-1)map[y][x]='#';
        else map[y][x]=MapRand(0,99)<24?'#':'.';
    }

    px=mw/2+MapRand(-2,2);py=2;
    int ladderSide=MapRand(0,2);
    int ladderX=(ladderSide==0?mw/4:ladderSide==1?mw/2:3*mw/4)+MapRand(-2,2);
    int ladderY=mh-4;
    artifactLadderX=ladderX;artifactLadderY=ladderY;
    for(int y=py-2;y<=py+2;y++)for(int x=px-2;x<=px+2;x++)
        if(x>0&&x<mw-1&&y>0&&y<mh-1)map[y][x]='.';
    for(int y=ladderY-2;y<=ladderY+2;y++)
        for(int x=ladderX-2;x<=ladderX+2;x++)
            if(x>0&&x<mw-1&&y>0&&y<mh-1)map[y][x]='.';

    int xMin=1,yMin=1,xMax=mw-2,yMax=mh-2;
    MazeFlood(px,py,xMin,yMin,xMax,yMax);
    if(mazeDistance[ladderY][ladderX]<0) {
        int bendY=py+(ladderY-py)*2/3;
        for(int y=py;y<=bendY;y++)map[y][px]='.';
        int step=ladderX>=px?1:-1;
        for(int x=px;x!=ladderX+step;x+=step)map[bendY][x]='.';
        for(int y=bendY;y<=ladderY;y++)map[y][ladderX]='.';
    }
    MazeFlood(px,py,xMin,yMin,xMax,yMax);
    MazeMarkPath(ladderX,ladderY);

    int xs[MAP_ROWS*MAP_COLS],ys[MAP_ROWS*MAP_COLS];
    int candidates=RoomCandidates(xMin,yMin,xMax,yMax,xs,ys);
    ShuffleRoomTiles(xs,ys,candidates);
    PlaceLavaPools(xs,ys,candidates,(mw*mh)/120+MapRand(20,28));
    PlaceRoomHazard('+',(mw*mh)/50+MapRand(32,44),xs,ys,candidates);

    // Keep the ladder landing and its three guaranteed pickups hazard-free.
    for(int y=ladderY-2;y<=ladderY+2;y++)
        for(int x=ladderX-2;x<=ladderX+2;x++)
            if(x>0&&x<mw-1&&y>0&&y<mh-1)map[y][x]='.';
    PlaceRoomItem(ladderX-1,ladderY,IK_SWD);
    PlaceRoomItem(ladderX+1,ladderY,IK_SWD);
    PlaceRoomItem(ladderX,ladderY+1,IK_SHD);
    map[ladderY][ladderX]='l';
    if(artifactHasLockedReward&&
       !PlaceGeneratedKey(artifactVaultKeyId,xMin,yMin,xMax,yMax,
                          ladderX,ladderY,7,true,6))
        PlaceGeneratedKey(artifactVaultKeyId,xMin,yMin,xMax,yMax,
                          ladderX,ladderY,0,false,0);

    for(int i=0,tries=0;i<38&&tries<5000;tries++) {
        int x=MapRand(xMin,xMax),y=MapRand(yMin,yMax);
        if(abs(x-px)+abs(y-py)<5||abs(x-ladderX)+abs(y-ladderY)<6)continue;
        if(PlaceRoomCoin(x,y))i++;
    }
    candidates=0;
    for(int y=yMin;y<=yMax;y++)for(int x=xMin;x<=xMax;x++)
        if(map[y][x]=='.'&&abs(x-px)+abs(y-py)>=12&&
           abs(x-ladderX)+abs(y-ladderY)>=7) {xs[candidates]=x;ys[candidates++]=y;}
    ShuffleRoomTiles(xs,ys,candidates);
    if(eneon) {
        int enemyGoal=29+MapRand(0,3);
        int placed=0;
        for(int k=0;k<candidates&&placed<enemyGoal&&placed<MAX_ENE;k++) {
            int x=xs[k],y=ys[k];
            if(abs(x-px)+abs(y-py)<12||
               abs(x-ladderX)+abs(y-ladderY)<7)continue;
            bool spaced=true;
            for(int j=0;j<placed;j++)
                if(abs((int)(enes[j].x+0.5f)-x)+abs((int)(enes[j].y+0.5f)-y)<4) {spaced=false;break;}
            if(!spaced)continue;
            enes[placed++]=(Ene){(float)x,(float)y,0,0.42f+MapRand(0,13)*0.01f,true};
        }
    }
}

// The lower portal section reuses the current seed but gets an independent stream.
static void GenArtifactVaultRoom(bool placeMissedKey) {
    unsigned seed=artifactRoomSeed^0x6D2B79F5u;
    if(seed==0u)seed=0xC0FFEE01u;
    MapRngStart(seed,(int)(artifactRoomSerial^(seed>>16)));
    mw=43+MapRand(0,22);mh=31+MapRand(0,14);
    memset(map,' ',sizeof(map));memset(enes,0,sizeof(enes));
    memset(coins,0,sizeof(coins));memset(itms,0,sizeof(itms));memset(prts,0,sizeof(prts));
    memset(artifactRoute,0,sizeof(artifactRoute));
    floorArtifact=artifactVaultArtifactId;
    for(int y=0;y<mh;y++)for(int x=0;x<mw;x++)map[y][x]='#';

    int xMin=1,yMin=1,xMax=mw-2,yMax=mh-2;
    int cellsX=(xMax-xMin)/2+1,cellsY=(yMax-yMin)/2+1;
    px=xMin+MapRand(0,cellsX-1)*2;py=yMin+MapRand(0,cellsY-1)*2;
    GenerateRandomMaze(0,0,mw-1,mh-1,&px,&py);
    AddMazeFeatures(xMin,yMin,xMax,yMax,2+MapRand(0,5),6+MapRand(0,18));

    MazeFlood(px,py,xMin,yMin,xMax,yMax);
    int artifactX=px,artifactY=py;
    MazeChooseFarthest(xMin,yMin,xMax,yMax,24,&artifactX,&artifactY);
    MazeMarkPath(artifactX,artifactY);
    MazeFlood(artifactX,artifactY,xMin,yMin,xMax,yMax);
    artifactExitX=artifactX;artifactExitY=artifactY;
    MazeChooseFarthest(xMin,yMin,xMax,yMax,16,&artifactExitX,&artifactExitY);
    MazeMarkPath(artifactExitX,artifactExitY);

    int xs[MAP_ROWS*MAP_COLS],ys[MAP_ROWS*MAP_COLS];
    int candidates=RoomCandidates(xMin,yMin,xMax,yMax,xs,ys);
    ShuffleRoomTiles(xs,ys,candidates);
    PlaceLavaPools(xs,ys,candidates,(mw*mh)/52+MapRand(18,26));
    PlaceRoomHazard('+',(mw*mh)/70+MapRand(20,28),xs,ys,candidates);

    for(int i=0,tries=0;i<34&&tries<2600;tries++) {
        int x=MapRand(xMin,xMax),y=MapRand(yMin,yMax);
        if((x==px&&y==py)||(x==artifactX&&y==artifactY)||
           (x==artifactExitX&&y==artifactExitY))continue;
        if(PlaceRoomCoin(x,y))i++;
    }
    for(int i=0,tries=0;i<2&&tries<900;tries++) {
        int x=MapRand(xMin,xMax),y=MapRand(yMin,yMax);
        if((x==px&&y==py)||(x==artifactX&&y==artifactY)||
           (x==artifactExitX&&y==artifactExitY))continue;
        if(PlaceRoomItem(x,y,(IKind)MapRand(0,IK_GLD-1)))i++;
    }
    char rewardTile=floorArtifact<0||artifactOwned[floorArtifact]?'%':'A';
    if(artifactLockDesignId>=0) {
        if(!RegisterLockedObjectWithKeyMaskAt(artifactX,artifactY,artifactLockDesignId,
                                              artifactLockKeyMask,rewardTile))
            map[artifactY][artifactX]=rewardTile;
    } else map[artifactY][artifactX]=rewardTile;
    map[py][px]='.';

    candidates=0;
    for(int y=yMin;y<=yMax;y++)for(int x=xMin;x<=xMax;x++)
        if(map[y][x]=='.'&&
           abs(x-px)+abs(y-py)>=10&&(x!=artifactX||y!=artifactY)&&
           (x!=artifactExitX||y!=artifactExitY)) {xs[candidates]=x;ys[candidates++]=y;}
    ShuffleRoomTiles(xs,ys,candidates);
    if(eneon)for(int i=0;i<16&&i<MAX_ENE&&i<candidates;i++)
        enes[i]=(Ene){(float)xs[i],(float)ys[i],0,0.55f,true};
    if(placeMissedKey&&
       !PlaceGeneratedKey(artifactVaultKeyId,xMin,yMin,xMax,yMax,
                          artifactX,artifactY,6,true,4))
        PlaceGeneratedKey(artifactVaultKeyId,xMin,yMin,xMax,yMax,
                          artifactX,artifactY,0,false,0);
}

void EnterArtifactVaultRoom(void) {
    if(roomKind!=RK_ARTIFACT||artifactRoomPart!=0)return;
    bool placeMissedKey=artifactHasLockedReward&&!artifactFirstRoomKeyCollected;
    artifactRoomPart=1;
    ResetLockedObjects();
    GenArtifactVaultRoom(placeMissedKey);
    memset(pars,0,sizeof(pars));pmt=0;sttime=0;bannerT=2.6f;
    UpdateCameraOffset();cam.target=(Vector2){WX(px),WY(py)};
}

bool CanEnterArtifactVaultRoom(void) {
    return roomKind==RK_ARTIFACT&&artifactRoomPart==0;
}

void EnterArtifactRoom(void) {
#ifdef DEBUG_ROOMS
 debugMapScenario=RK_ARTIFACT;
#endif
  ResetUniversalKeys();
  ResetLockedObjects();
 hiddenLeft=hiddenTop=-1;hiddenWidth=hiddenHeight=0;hiddenMainWidth=-1;ResetHiddenRoomAccess();roomKind=RK_ARTIFACT;coinRush=false;boss.on=false;
 trapPhase=lavaTimer=lavaElapsed=0;pgold=0;
 int n=0;for(int i=0;i<pinvn;i++)if(pinv[i]!=IK_GLD)pinv[n++]=pinv[i];
 pinvn=n;if(inv_sel>=n)inv_sel=n-1;
 GenArtifactRoom();memset(pars,0,sizeof(pars));pmt=0;sttime=0;bannerT=2.6f;
 UpdateCameraOffset();cam.target=(Vector2){WX(px),WY(py)};
}
void GenRound(void) {
#ifdef DEBUG_ROOMS
    debugMapScenario=forceRoom;
    if (forceRoom==DEBUG_SECRET_ROOM || forceRoom==DEBUG_PORTAL_ROOM) forceRoom=RK_NORMAL;
#endif
    ResetUniversalKeys();
    ResetLockedObjects();
    hiddenLeft=hiddenTop=-1;hiddenWidth=hiddenHeight=0;hiddenMainWidth=-1;
    ResetHiddenRoomAccess();
    MapRngStart(run_seed, roundn);

    if(!ChallengeRunActive())StatsInc(&global_rounds);

    // forceRoom is one-shot: consumed for this floor only.
    if (ChallengeRunActive()) {
        roomKind = RK_NORMAL;
        forceRoom = -1;
    } else if (forceRoom >= 0) {
        roomKind = (RoomKind)forceRoom;
        forceRoom = -1;
    } else if (roundn % 10 == 0) {
        roomKind = RK_BOSS;
    } else if (roundn >= 6) {
        // 5% trap, 5% dark, 5% lava, 5% coin rush — rest normal.
        int r = MapRand(0, 99);
        if      (r < 5)  roomKind = RK_TRAP;
        else if (r < 10) roomKind = RK_DARK;
        else if (r < 15) roomKind = RK_LAVA;
        else if (r < 20) roomKind = RK_COIN_RUSH;
        else             roomKind = RK_NORMAL;
    } else {
        roomKind = RK_NORMAL;
    }

    // Gameplay flag used by spawns / coin value / HUD (true only on RK_COIN_RUSH).
    coinRush = (roomKind == RK_COIN_RUSH);

    // Gold Touch is Coin-Rush only — strip any carried GLD outside that floor.
    if (!coinRush) {
        int w = 0;
        for (int i = 0; i < pinvn; i++)
            if (pinv[i] != IK_GLD) pinv[w++] = pinv[i];
        pinvn = w;
        if (pinvn <= 0) inv_sel = -1;
        else if (inv_sel >= pinvn) inv_sel = pinvn - 1;
        pgold = 0;   // midas form ends when you leave Coin Rush
    }

    boss.on   = false;
    trapPhase = 0; lavaTimer = 0; lavaElapsed = 0;
    bannerT   = (roomKind == RK_NORMAL) ? 0 : 2.6f;

    switch (roomKind) {
        case RK_ARTIFACT: GenArtifactRoom(); break;
        case RK_BOSS:      GenBossArena();   break;
        case RK_TRAP:      GenTrapRound();   break;
        case RK_DARK:      GenDarkRound();   break;
        case RK_LAVA:      GenLavaRound();   break;
        case RK_COIN_RUSH: GenNormalRound(); break;  // same generator, denser via coinRush
        default:           GenNormalRound(); break;
    }

    EnsureReachable();   // no walled-in treasure, no soft-locks
    if(roomKind!=RK_ARTIFACT && !ChallengeRunActive()) {
        int secretChance=2,portalChance=2;
#ifdef DEBUG_ROOMS
        secretChance=debugMapScenario==DEBUG_SECRET_ROOM ? 100 : debugSecretChance;
        portalChance=debugMapScenario==DEBUG_PORTAL_ROOM ? 100 : debugPortalChance;
#endif
        if(roomKind==RK_NORMAL && MapRand(1,100)<=secretChance) AddHiddenRoom();
        bool portalEligible=roomKind!=RK_BOSS&&roomKind!=RK_COIN_RUSH;
        if(portalEligible&&MapRand(1,100)<=portalChance) {
            int xs[MAP_ROWS*MAP_COLS],ys[MAP_ROWS*MAP_COLS],n=0;
            for(int y=1;y<mh-1;y++) for(int x=1;x<mw-1;x++)
                if(map[y][x]=='.' && !IsHiddenRoomTile(x,y) &&
                   !IsHiddenRoomEntryTile(x,y) && (x!=px||y!=py)) {
                    xs[n]=x;ys[n++]=y;
                }
            if(n>0) {
                int i=MapRand(0,n-1),portalX=xs[i],portalY=ys[i];
                map[portalY][portalX]='M';
                EnsureReachable();
                if(MapRand(0,1)==0) {
                    LoadArtifactLockAssignments();
                    unsigned keyMask=lockedKeyMasks[PORTAL_ECLIPSE_IRIS_SEAL_DESIGN_ID];
                    int keyId=ChooseAssignedKey(keyMask);
                    int keyRight=hiddenMainWidth>0?hiddenMainWidth-2:mw-2;
                    int keyBottom=hiddenEntryStartY>0?hiddenEntryStartY-1:mh-2;
                    if(keyId>=0)
                        if(!PlaceGeneratedKey(keyId,1,1,keyRight,keyBottom,
                                              portalX,portalY,5,false,4))
                            PlaceGeneratedKey(keyId,1,1,keyRight,keyBottom,
                                              portalX,portalY,0,false,0);
                    RegisterLockedObjectWithKeyMaskAt(portalX,portalY,
                        PORTAL_ECLIPSE_IRIS_SEAL_DESIGN_ID,keyMask,'M');
                }
            }
        }
    }

    UpdateCameraOffset();
    cam.target   = (Vector2){WX(px), WY(py)};
    cam.zoom     = 1.0f;
    cam.rotation = 0;

    pdead  = false; pmt = 0; pmd = 0.13f;
    pspd   = 0;
    pwlk   = 0;
    pgold  = 0;
    pfall  = 0;
    pshield = false; pshield_dur = 0;
    if (roundn == 1) pshield_perk = (perk_shield2 > 0);
    sttime = 0;
    memset(pars, 0, sizeof(pars));
}

// ─── DRAW TILE ────────────────────────────────────────────────────────────────
static void DrawHiddenRoomGate(int wx,int wy,float t,bool open) {
    DrawRectangle(wx+2,wy+1,28,31,(Color){12,10,21,255});
    DrawRectangle(wx+4,wy+3,24,28,(Color){3,5,12,255});
    if(open) {
        float pulse=photosensitivity?0.5f:0.5f+0.5f*sinf(t*2.0f);
        DrawCircle(wx+16,wy+16,11,
                   (Color){57,221,247,(uchar)(70+40*pulse)});
        DrawTriangle((Vector2){wx+16,wy+8},(Vector2){wx+24,wy+16},
                     (Vector2){wx+16,wy+24},(Color){13,28,46,255});
        DrawTriangle((Vector2){wx+16,wy+8},(Vector2){wx+16,wy+24},
                     (Vector2){wx+8,wy+16},(Color){13,28,46,255});
        DrawCircle(wx+16,wy+16,3,(Color){161,246,251,255});
        DrawRectangle(wx+2,wy+2,4,29,(Color){91,77,100,255});
        DrawRectangle(wx+26,wy+2,4,29,(Color){91,77,100,255});
        DrawRectangle(wx+3,wy+2,26,3,(Color){172,151,138,255});
        return;
    }

    DrawRectangle(wx+5,wy+4,22,26,(Color){20,15,32,255});
    DrawRectangle(wx+6,wy+5,9,24,(Color){43,31,52,255});
    DrawRectangle(wx+17,wy+5,9,24,(Color){34,25,45,255});
    for(int y=8;y<28;y+=8) {
        DrawRectangle(wx+8,wy+y,5,2,(Color){104,78,74,255});
        DrawRectangle(wx+19,wy+y,5,2,(Color){104,78,74,255});
    }
    DrawRectangle(wx+14,wy+5,3,23,(Color){10,8,18,255});
    DrawRing((Vector2){(float)wx+16,(float)wy+16},3,5,0,360,18,
             (Color){171,119,91,255});
    DrawCircle(wx+16,wy+12,2,(Color){211,194,168,255});
    DrawRectangle(wx+14,wy+14,4,3,(Color){211,194,168,255});
    DrawRing((Vector2){(float)wx+16,(float)wy+23},3,5,0,360,18,
             (Color){239,193,103,255});
    DrawCircle(wx+16,wy+22,1,(Color){6,8,15,255});
    DrawRectangle(wx+15,wy+23,2,4,(Color){6,8,15,255});
    DrawRectangle(wx+2,wy+2,3,29,(Color){104,85,112,255});
    DrawRectangle(wx+27,wy+2,3,29,(Color){76,61,91,255});
    DrawRectangle(wx+3,wy+2,26,3,(Color){171,144,141,255});
}

void DrawWorldTile(int wx, int wy, char dc, IKind itemKind, float t) {
    int x = wx / CELL, y = wy / CELL;
    if (dc == ' ') {
        if(hiddenTop>=0&&hiddenMainWidth>=0&&
           (x>=hiddenMainWidth||y>=hiddenTop))
            DrawRectangle(wx,wy,CELL,CELL,CB);
        return;
    }

    DrawRectangle(wx, wy, CELL, CELL, CB);

    if (dc=='#') {
        DrawRectangle(wx+1, wy+1, CELL-2, CELL-2, CW);
        DrawRectangle(wx+1, wy+1, CELL-2, 3, CWH);
        DrawRectangle(wx+1, wy+1, 3, CELL-2, CWH);
        DrawRectangle(wx+1, wy+CELL-4, CELL-2, 3, CWD);
        DrawRectangle(wx+CELL-4, wy+1, 3, CELL-2, CWD);
    } else {
        if(dc=='D') {
            // Dispensers are mounted into the left wall, with the machine
            // drawn over the same masonry treatment as the surrounding tiles.
            DrawRectangle(wx+1, wy+1, CELL-2, CELL-2, CW);
            DrawRectangle(wx+1, wy+1, CELL-2, 3, CWH);
            DrawRectangle(wx+1, wy+1, 3, CELL-2, CWH);
            DrawRectangle(wx+1, wy+CELL-4, CELL-2, 3, CWD);
            DrawRectangle(wx+CELL-4, wy+1, 3, CELL-2, CWD);
        } else {
            DrawRectangle(wx+1, wy+1, CELL-2, CELL-2, CF);
        }

        int ladderDx=abs(x-artifactLadderX),ladderDy=abs(y-artifactLadderY);
        bool ladderPad=roomKind==RK_ARTIFACT&&artifactRoomPart==0&&
                       artifactLadderX>=0&&ladderDx<=2&&ladderDy<=2;
        if(ladderPad) {
            bool rim=ladderDx==2||ladderDy==2;
            Color pad=rim?(Color){43,27,59,255}:(Color){31,24,44,255};
            Color edge=rim?(Color){139,80,188,255}:(Color){77,57,101,255};
            DrawRectangle(wx+1,wy+1,CELL-2,CELL-2,pad);
            DrawRectangleLines(wx+1,wy+1,CELL-2,CELL-2,edge);
            if(rim) {
                DrawPixel(wx+5,wy+5,(Color){214,146,255,255});
                DrawPixel(wx+CELL-6,wy+5,(Color){214,146,255,255});
                DrawPixel(wx+5,wy+CELL-6,(Color){214,146,255,255});
                DrawPixel(wx+CELL-6,wy+CELL-6,(Color){214,146,255,255});
            }
        }

        if (dc=='Q') {
            DrawLockedObjectAt(x,y,wx,wy,t);
        } else if (dc=='G') {
            DrawHiddenRoomGate(wx,wy,t,false);
        } else if (dc=='o') {
            DrawHiddenRoomGate(wx,wy,t,true);
        } else if (dc=='+') {
            DrawRectangle(wx + 3, wy + 3, CELL - 6, CELL - 6, (Color){20, 15, 25, 255});
            DrawRectangleLines(wx + 3, wy + 3, CELL - 6, CELL - 6, (Color){60, 50, 70, 255});
            for(int i = 0; i < 3; i++) {
                for(int j = 0; j < 3; j++) {
                    int spx = wx + 8 + i * 8;
                    int spy = wy + 8 + j * 8;
                    DrawCircle(spx, spy, 2, (Color){190, 190, 210, 255});
                    DrawRectangle(spx - 1, spy - 2, 2, 2, CTRAP);
                }
            }

        } else if (dc=='l') {
            float glow=photosensitivity?0.5f:0.5f+0.5f*sinf(t*3.6f);
            DrawCircle(wx+CELL/2,wy+CELL/2,15,
                       (Color){176,72,255,(uchar)(32+38*glow)});
            DrawRectangle(wx+1,wy+1,CELL-2,CELL-2,(Color){55,34,72,255});
            DrawRectangleLines(wx+1,wy+1,CELL-2,CELL-2,(Color){231,143,255,255});
            DrawRectangleLines(wx+3,wy+3,CELL-6,CELL-6,(Color){118,77,151,255});
            DrawRectangle(wx+6,wy+5,CELL-12,CELL-10,(Color){5,4,11,255});
            DrawRectangleLines(wx+6,wy+5,CELL-12,CELL-10,(Color){155,92,195,255});
            DrawRectangle(wx+9,wy+7,3,CELL-16,(Color){210,177,123,255});
            DrawRectangle(wx+20,wy+7,3,CELL-16,(Color){210,177,123,255});
            for(int rung=0;rung<4;rung++)
                DrawRectangle(wx+11,wy+9+rung*4,10,2,(Color){245,214,155,255});
            DrawLineEx((Vector2){wx+11,wy+23},(Vector2){wx+16,wy+27},2,
                       (Color){220,130,255,255});
            DrawLineEx((Vector2){wx+16,wy+27},(Vector2){wx+21,wy+23},2,
                       (Color){220,130,255,255});
            DrawCircle(wx+5,wy+5,1,(Color){255,220,170,255});
            DrawCircle(wx+CELL-6,wy+5,1,(Color){255,220,170,255});
            DrawCircle(wx+5,wy+CELL-6,1,(Color){255,220,170,255});
            DrawCircle(wx+CELL-6,wy+CELL-6,1,(Color){255,220,170,255});

        } else if (dc=='h') {
            // Closed trapdoor: still floor-like, but a notch cooler/darker than
            // CF {28,24,40} so careful eyes catch it — not a loud trap tile.
            DrawRectangle(wx+1, wy+1, CELL-2, CELL-2, (Color){20, 19, 32, 255});
            // faint mid seam
            DrawLine(wx+5, wy+CELL/2, wx+CELL-5, wy+CELL/2, (Color){14, 12, 22, 100});
            // soft corner ticks
            DrawPixel(wx+4, wy+4, (Color){42, 38, 56, 150});
            DrawPixel(wx+CELL-5, wy+4, (Color){42, 38, 56, 150});
            DrawPixel(wx+4, wy+CELL-5, (Color){42, 38, 56, 130});
            DrawPixel(wx+CELL-5, wy+CELL-5, (Color){42, 38, 56, 130});
            // whisper of a rim (still quiet)
            DrawRectangleLines(wx+2, wy+2, CELL-4, CELL-4, (Color){16, 14, 26, 55});

        } else if (dc=='H') {
            // Open hatch — black pit after the player steps on it.
            DrawRectangle(wx+1, wy+1, CELL-2, CELL-2, (Color){8, 6, 12, 255});
            DrawRectangle(wx+4, wy+4, CELL-8, CELL-8, (Color){2, 1, 4, 255});
            DrawRectangleLines(wx+3, wy+3, CELL-6, CELL-6, (Color){55, 48, 70, 180});
            float p = photosensitivity ? 0.4f : (0.5f + 0.5f*sinf(t*10.0f + x + y));
            DrawCircle(wx+CELL/2, wy+CELL/2, (int)(5+2*p),
                       (Color){20, 12, 30, (uchar)(40+40*p)});

        } else if (dc=='r') {
            // Arrow-sweep lane (fed west->east by the 'D' dispenser). Only the
            // brief fire window is lethal, and the warning escalates in two
            // unmistakable stages: first a big pulsing "!" on every lane tile
            // (get out NOW), then the red arrows — the things that kill.
            // Idle: faint chevrons so the lane itself stays readable.
            float cyc       = fmodf(trapPhase, ARROW_PERIOD);
            float fireStart = ARROW_PERIOD - ARROW_FIRE;
            float warnStart = fireStart - ARROW_WARN;
            bool  lethal    = cyc >= fireStart;
            bool  warn      = cyc >= warnStart && cyc < fireStart;

            if (warn) {
                // Stage 1: "!" — loud, yellow, impossible to miss.
                float wp = photosensitivity ? (0.5f + 0.25f*sinf(t*3.0f))
                                            : (0.5f + 0.5f*sinf(t*16.0f));
                DrawRectangle(wx+2, wy+2, CELL-4, CELL-4,
                              (Color){235, 200, 60, (uchar)(14 + 20*wp)});
                const char *ex = "!";
                int fs = 22;
                DrawText(ex, wx + CELL/2 - MeasureText(ex, fs)/2, wy + CELL/2 - fs/2,
                         fs, (Color){255, 220, 60, (uchar)(190 + 65*wp)});
            } else {
                // Idle: faint chevrons. Fire: the red killers, with a bright
                // head racing east along the lane.
                Color ac = lethal ? (Color){255, 80, 60, 255}
                                  : (Color){95, 85, 78, 140};
                if (lethal) {
                    float head = (cyc - fireStart) / ARROW_FIRE;   // west (0) -> east (1)
                    int dispenserX=x;
                    while(dispenserX>=0&&map[y][dispenserX]!='D')dispenserX--;
                    if(dispenserX<0)dispenserX=1;
                    int laneEnd=x;
                    while(laneEnd+1<mw&&map[y][laneEnd+1]=='r')laneEnd++;
                    int laneStart=dispenserX+1;
                    int laneSpan=laneEnd-laneStart;
                    float fx=laneSpan>0?(float)(x-laneStart)/(float)laneSpan:0.0f;
                    if (fabsf(fx - head) < 0.10f) ac = (Color){255, 235, 180, 255};
                }
                int my = wy + CELL/2;
                for (int a = 0; a < 2; a++) {   // chevrons always point east
                    int ox = wx + 6 + a*12;
                    DrawTriangle((Vector2){ox, my-5}, (Vector2){ox+7, my}, (Vector2){ox, my+5}, ac);
                }
                if (lethal) {
                    float p = photosensitivity ? 0.4f : (0.5f + 0.5f*sinf(t*12.0f));
                    DrawCircle(wx+CELL/2, wy+CELL/2, (int)(10+3*p),
                               (Color){255, 80, 60, (uchar)(16+22*p)});
                }
            }

        } else if (dc=='D') {
            // Arrow dispenser: a wall-set device at the far-left of a lane. Dim
            // while idle, charges amber during the telegraph, muzzle-flashes as
            // it looses the volley — so you always know a sweep is coming.
            float cyc       = fmodf(trapPhase, ARROW_PERIOD);
            float fireStart = ARROW_PERIOD - ARROW_FIRE;
            float warnStart = fireStart - ARROW_WARN;
            bool  lethal    = cyc >= fireStart;
            bool  warn      = cyc >= warnStart && cyc < fireStart;
            float wp        = warn ? (cyc - warnStart) / ARROW_WARN : 0.0f;   // 0->1 charge

            // Housing.
            DrawRectangle(wx+1, wy+1, CELL-2, CELL-2, (Color){40, 34, 30, 255});
            DrawRectangleLines(wx+1, wy+1, CELL-2, CELL-2, CCANN);
            // Muzzle pointing east.
            Color mc = lethal ? (Color){255, 235, 180, 255}
                     : warn   ? (Color){235, (uchar)(150+80*wp), 60, 255}
                     :          (Color){120, 95, 70, 255};
            DrawRectangle(wx+CELL-12, wy+CELL/2-4, 10, 8, mc);
            DrawTriangle((Vector2){wx+CELL-3, wy+CELL/2-6},
                         (Vector2){wx+CELL+1, wy+CELL/2},
                         (Vector2){wx+CELL-3, wy+CELL/2+6}, mc);
            if (lethal) {
                float p = 0.5f + 0.5f*sinf(t*20.0f);
                DrawCircle(wx+CELL-2, wy+CELL/2, (int)(6+4*p),
                           (Color){255, 210, 120, (uchar)(120+80*p)});
            } else if (warn) {
                DrawCircle(wx+CELL/2, wy+CELL/2, (int)(4+4*wp),
                           (Color){235, 150, 60, (uchar)(60+120*wp)});
            }

        } else if (dc=='L') {
            // Lava: hot, animated, lethal.
            float p  = 0.5f + 0.5f*sinf(t*2.2f + x*0.9f + y*1.3f);
            DrawRectangle(wx+1, wy+1, CELL-2, CELL-2, (Color){185, 55, 12, 255});
            DrawRectangle(wx+3, wy+3, CELL-6, CELL-6,
                          (Color){235, (uchar)(90+50*p), 22, 255});
            DrawCircle(wx+CELL/2 + (int)(6*sinf(t*1.1f + x)),
                       wy+CELL/2 + (int)(5*cosf(t*1.4f + y)),
                       5, (Color){255, (uchar)(175+60*p), 60, (uchar)(130+80*p)});

        } else if (dc=='^' || dc=='*') {
            // Spike plate (boss rounds): '^' dormant until the player steps on
            // it, '*' armed and glowing — the boss takes damage on it.
            bool armed = (dc=='*');
            Color base = armed ? (Color){10, 52, 47, 255}  : (Color){22, 22, 30, 255};
            Color rim  = armed
                       ? (Color){CLAMP255(CPLATE.r + 45), CLAMP255(CPLATE.g + 35),
                                 CLAMP255(CPLATE.b + 35), 255}
                       : (Color){55, 55, 70, 255};
            Color spk  = armed
                       ? (Color){CLAMP255(CPLATE.r + 60), CLAMP255(CPLATE.g + 45),
                                 CLAMP255(CPLATE.b + 45), 255}
                       : (Color){95, 100, 115, 255};
            if (armed) {
                float p = photosensitivity ? 0.72f
                         : (0.5f + 0.5f*sinf(t * 2.5f + x*0.6f + y*0.4f));
                DrawCircle(wx + CELL/2, wy + CELL/2, (int)(15 + 4*p),
                           (Color){CPLATE.r, CPLATE.g, CPLATE.b,
                                   (uchar)(30 + 36*p)});
            }
            DrawRectangle(wx + 3, wy + 3, CELL - 6, CELL - 6, base);
            DrawRectangleLines(wx + 3, wy + 3, CELL - 6, CELL - 6, rim);
            for (int i = 0; i < 3; i++) {
                float sx0 = wx + 8 + i * 8;
                DrawTriangle((Vector2){sx0, wy + 9},
                             (Vector2){sx0 + 3, wy + CELL - 9},
                             (Vector2){sx0 - 3, wy + CELL - 9}, spk);
            }
            if (armed) {
                float p = photosensitivity ? 0.72f
                         : (0.5f + 0.5f*sinf(t * 2.5f + x*0.6f + y*0.4f));
                DrawCircleLines(wx + CELL/2, wy + CELL/2, (int)(12 + 3*p),
                                (Color){CPLATE.r, CPLATE.g, CPLATE.b,
                                        (uchar)(170 + 75*p)});
            }

        } else if (dc=='C') {
            // Cannon base pad — heavily highlighted floor tile; the barrel,
            // reload ring and countdown are drawn on top by game_boss.c.
            float p = 0.5f + 0.5f*sinf(t * 2.0f + x*0.4f + y*0.4f);
            DrawCircle(wx+CELL/2, wy+CELL/2, (int)(16+3*p),
                       (Color){CCANN.r, CCANN.g, CCANN.b, (uchar)(30+18*p)});
            DrawRectangle(wx + 2, wy + 2, CELL - 4, CELL - 4, (Color){46, 36, 24, 255});
            DrawRectangleLines(wx + 1, wy + 1, CELL - 2, CELL - 2, CCANN);
            DrawRectangleLines(wx + 2, wy + 2, CELL - 4, CELL - 4, (Color){80, 70, 62, 255});
            DrawCircleLines(wx + CELL/2, wy + CELL/2, 13, (Color){CCANN.r, CCANN.g, CCANN.b, 220});

        } else if (dc=='k') {
            DrawUniversalKeyIcon(KEY_TUTORIAL_BOOK,wx+2,wy+2,CELL-4,t);
        } else if (UniversalKeyIdFromTile(dc)>=0) {
            DrawUniversalKeyIcon(UniversalKeyIdFromTile(dc),wx+2,wy+2,CELL-4,t);
        } else if (dc=='g') {
            DrawRectangle(wx+3,wy+2,CELL-6,CELL-4,(Color){28,27,39,255});
            DrawRectangleLines(wx+3,wy+2,CELL-6,CELL-4,
                               (Color){226,178,96,255});
            DrawRectangle(wx+6,wy+5,CELL-12,CELL-10,(Color){62,50,58,255});
            DrawLine(wx+7,wy+8,wx+25,wy+24,(Color){236,195,117,255});
            DrawLine(wx+25,wy+8,wx+7,wy+24,(Color){236,195,117,255});
            DrawCircle(wx+16,wy+16,5,(Color){241,205,129,255});
            DrawCircle(wx+16,wy+15,2,(Color){23,20,31,255});
            DrawRectangle(wx+15,wy+16,2,4,(Color){23,20,31,255});
        } else if (dc=='T') {
            float p = 0.5f + 0.5f*sinf(t * 3.0f);
            Color tc = CTRES; tc.a = (uchar)(200 + 55*p);
            DrawRectangle(wx+6,  wy+9,  CELL-12, CELL-14, tc);
            DrawRectangle(wx+6,  wy+9,  CELL-12, 4, (Color){255,240,180,230});
            DrawCircle(wx+CELL/2, wy+CELL/2+2, 3, (Color){200,150,30,255});
            DrawCircle(wx+CELL/2, wy+CELL/2,
                       (int)(12 + 5*p), (Color){255,200,50,(int)(18+20*p)});
        } else if (dc=='$') {
            float p = sinf(t * 4.0f + x*0.5f + y*0.7f);
            int r = (int)(5 + p*1.5f);
            // Coin Rush: two coins side-by-side (visual = double value).
            if (coinRush) {
                int ox = 5;
                for (int s = -1; s <= 1; s += 2) {
                    int cx = wx + CELL/2 + s * ox;
                    int cy = wy + CELL/2;
                    DrawCircle(cx+1, cy+2, r - 1, (Color){160,120,0,80});
                    DrawCircle(cx,   cy,   r - 1, CCOIN);
                    DrawCircle(cx-1, cy-1, (int)((r-1)*0.4f)+1, (Color){255,245,160,200});
                }
            } else {
                DrawCircle(wx+CELL/2+1, wy+CELL/2+2, r, (Color){160,120,0,80});
                DrawCircle(wx+CELL/2,   wy+CELL/2,   r, CCOIN);
                DrawCircle(wx+CELL/2-1, wy+CELL/2-1,
                           (int)(r*0.4f)+1, (Color){255,245,160,200});
            }

        } else if (dc=='I') {
            IKind k = itemKind;
            if (k != IK_NONE) {
                Color ic = IK_COL[k];
                // Keep the live-game pickup pulse identical to V1_6.  The
                // brighter, longer-light tutorial pulse is intentionally
                // implemented only in game_ui.c.
                float basePulse = 0.5f + 0.5f*sinf(t*3.0f + (float)(x+y)*0.3f);
                float p = 0.38f + 0.62f * basePulse;
                DrawTriangle(
                    (Vector2){wx+CELL/2, wy+4},
                    (Vector2){wx+CELL-4, wy+CELL/2},
                    (Vector2){wx+CELL/2, wy+CELL-4}, ic);
                DrawTriangle(
                    (Vector2){wx+CELL/2, wy+4},
                    (Vector2){wx+CELL/2, wy+CELL-4},
                    (Vector2){wx+4,      wy+CELL/2}, ic);
                Color gc = ic; gc.a = (uchar)(55 * p);
                DrawCircle(wx+CELL/2, wy+CELL/2, (int)(14+3*p), gc);
            }

        } else if(dc=='A') {
            int cx=wx+CELL/2,cy=wy+CELL/2;
            DrawCircle(cx,cy,14,(Color){255,30,210,55});
            DrawArtifactIcon(floorArtifact,(Vector2){cx,cy},25,true);
        } else if(dc=='%') {
            int cx=wx+CELL/2,cy=wy+CELL/2;
            DrawCircle(cx,cy,14,(Color){255,202,87,35});
            DrawCoinPileIcon((Vector2){(float)cx,(float)cy},(float)CELL);
        } else if(dc=='M') {
            float p=photosensitivity?0.5f:0.5f+0.5f*sinf(t*2);
            int cx=wx+CELL/2,cy=wy+CELL/2;
            DrawCircle(cx,cy,17,(Color){255,0,205,100});
            DrawCircle(cx,cy,12,(Color){130,0,110,255});
            DrawCircleLines(cx,cy,14+p*2,(Color){255,0,220,255});
            DrawCircleLines(cx,cy,10,(Color){255,65,240,255});
            DrawCircle(cx,cy,5,(Color){255,200,250,255});
        } else if (dc=='O') {
            float p = 0.5f + 0.5f*sinf(t*2.0f + (float)(x+y)*0.2f);
            DrawCircle(wx+CELL/2, wy+CELL/2,
                       (int)(12+5*p), (Color){80,160,255,(int)(65+50*p)});
            DrawCircle(wx+CELL/2, wy+CELL/2,
                       (int)(7+2*p),  (Color){160,210,255,(int)(140+60*p)});
            DrawCircleLines(wx+CELL/2, wy+CELL/2,
                            (int)(12+5*p), (Color){120,190,255,200});
        }
    }
}

void DrawTile(int x, int y, float t) {
    char c = map[y][x];
    IKind k = IK_NONE;
    if (c == 'I') {
        for (int i = 0; i < MAX_ITM; i++)
            if (itms[i].on && itms[i].x == x && itms[i].y == y) { k = itms[i].k; break; }
    }
    DrawWorldTile(x * CELL, y * CELL, c, k, t);
}

// ─── DRAW MAP ─────────────────────────────────────────────────────────────────
void DrawMap(float t) {
    for (int y = 0; y < mh; y++)
    for (int x = 0; x < mw; x++)
        DrawTile(x, y, t);
}

// ─── ARROW LANES (RK_TRAP) ────────────────────────────────────────────────────
// Every ARROW_PERIOD seconds the lanes ('r') fire one synchronized volley that
// is lethal for the last ARROW_FIRE seconds of the cycle. Safe the rest of the
// time — cross between volleys.
bool ArrowLaneLethal(void) {
    float cyc = fmodf(trapPhase, ARROW_PERIOD);
    return cyc >= (ARROW_PERIOD - ARROW_FIRE);
}

bool ArrowLanesPresent(void) {
    if(roomKind==RK_TRAP)return true;
    for(int y=0;y<mh;y++)for(int x=0;x<mw;x++)
        if(map[y][x]=='D')return true;
    return false;
}

// ─── LAVA SPREAD (RK_LAVA) ────────────────────────────────────────────────────
// The room starts at its existing lavaTick rate, then gains spread speed
// linearly over active play time. Walls block the flow. Reaching the player
// kills (shield saves once and keeps that tile dry for a moment — but the front
// keeps coming).
void UpdLava(float dt) {
    if (roomKind != RK_LAVA || gs != GS_PLAY) return;
    lavaElapsed += dt;
    float spreadRate = 1.0f/lavaTick + LAVA_ACCEL_RATE*lavaElapsed;
    float currentTick = 1.0f/spreadRate;
    if (currentTick < LAVA_MIN_TICK) currentTick = LAVA_MIN_TICK;
    lavaTimer += dt;
    if (lavaTimer < currentTick) return;
    lavaTimer -= currentTick;

    static int qx[MAP_ROWS*MAP_COLS], qy[MAP_ROWS*MAP_COLS];
    int qn = 0;
    for (int y = 1; y < mh-1; y++)
    for (int x = 1; x < mw-1; x++) {
        char c = map[y][x];
        if (c!='.' && c!='+' && c!='$' && c!='I') continue;
        if (map[y-1][x]=='L' || map[y+1][x]=='L' ||
            map[y][x-1]=='L' || map[y][x+1]=='L') {
            qx[qn] = x; qy[qn] = y; qn++;
        }
    }

    for (int i = 0; i < qn; i++) {
        int x = qx[i], y = qy[i];
        char c = map[y][x];
        if (c == '$') for (int k=0;k<MAX_COI;k++)
            if (coins[k].on && coins[k].x==x && coins[k].y==y) { coins[k].on=false; break; }
        if (c == 'I') for (int k=0;k<MAX_ITM;k++)
            if (itms[k].on && itms[k].x==x && itms[k].y==y) { itms[k].on=false; break; }

        if (x == px && y == py) {
            if (pshield || pshield_perk) {
                if (pshield) pshield = false;
                else pshield_perk = false;
                map[y][x] = '.';                 // the shield keeps this tile dry
                Burst(WX(x), WY(y), CSHD, 22, 4);
                PlaySfx(SFX_SHIELD_TRIGGER);
                continue;
            }
            Burst(WX(x), WY(y), CFIRE, 32, 5);
            PlaySfx(SFX_DEATH_TRAP);
            AddScreenShake(12.0f, 0.28f);
            StatsInc(&global_deaths_lava);
            pdead = true; gs = GS_DEAD; sttime = 0;
            UpdateAndSaveStats();
        }
        map[y][x] = 'L';
        if ((i & 3) == 0) Burst(WX(x), WY(y), CFIRE, 4, 2);   // sparse sparks
    }
}

// ─── DARKNESS OVERLAY (RK_DARK) ───────────────────────────────────────────────
// Drawn in world space after map/entities/player: full sight within
// DARK_LIGHT_R tiles, dusk out to DARK_DIM_R, near-black beyond. Treasure and
// portals glow through; coins leave a faint glint; enemies beyond the light
// betray themselves only by their eyes.
void DrawDarkness(void) {
    for (int y = 0; y < mh; y++)
    for (int x = 0; x < mw; x++) {
        float dx = (float)(x - px), dy = (float)(y - py);
        float d  = sqrtf(dx*dx + dy*dy);
        int a;
        if      (d <= DARK_LIGHT_R) a = 0;
        else if (d <  DARK_DIM_R)   a = (int)((d - DARK_LIGHT_R) / (DARK_DIM_R - DARK_LIGHT_R) * 235);
        else                        a = 247;
        if (a > 0) DrawRectangle(x*CELL, y*CELL, CELL, CELL, (Color){0,0,0,(uchar)a});

        char c = map[y][x];
        if (c == 'T') {
            float p = 0.5f + 0.5f*sinf((float)GetTime()*3.0f);
            DrawCircle(x*CELL+CELL/2, y*CELL+CELL/2, (int)(14+5*p),
                       (Color){CTRES.r, CTRES.g, CTRES.b, (uchar)(40+40*p)});
            DrawCircle(x*CELL+CELL/2, y*CELL+CELL/2, 4, CTRES);
        } else if (c == 'M') {
            DrawCircle(x*CELL+CELL/2,y*CELL+CELL/2,15,(Color){255,0,220,100});
            DrawCircle(x*CELL+CELL/2,y*CELL+CELL/2,4,(Color){255,100,240,255});
        } else if (c == 'O') {
            float p = 0.5f + 0.5f*sinf((float)GetTime()*2.0f + x);
            DrawCircle(x*CELL+CELL/2, y*CELL+CELL/2, (int)(12+4*p),
                       (Color){CPORT.r, CPORT.g, CPORT.b, (uchar)(45+45*p)});
            DrawCircle(x*CELL+CELL/2, y*CELL+CELL/2, 3, (Color){160,210,255,220});
        } else if (c == '$' && a > 120) {
            DrawCircle(x*CELL+CELL/2, y*CELL+CELL/2, 2, (Color){255,215,0,80});
        }
    }

    // Glowing eyes: the only warning of an enemy outside the light.
    if (eneon) {
        for (int i = 0; i < MAX_ENE; i++) {
            if (!enes[i].on) continue;
            int ex = (int)(enes[i].x+0.5f), ey = (int)(enes[i].y+0.5f);
            float dd = sqrtf((float)((ex-px)*(ex-px) + (ey-py)*(ey-py)));
            if (dd > DARK_DIM_R - 1.0f && dd <= 9.0f) {
                DrawCircle(ex*CELL+CELL/2-4, ey*CELL+CELL/2-2, 2, (Color){255,60,60,220});
                DrawCircle(ex*CELL+CELL/2+4, ey*CELL+CELL/2-2, 2, (Color){255,60,60,220});
            }
        }
    }
}
