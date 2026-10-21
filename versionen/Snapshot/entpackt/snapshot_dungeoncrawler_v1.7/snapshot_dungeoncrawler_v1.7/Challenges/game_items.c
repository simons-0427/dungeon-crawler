// game_items.c — shared item descriptor tables.
#include "game_items.h"

const char *IK_NAME[] = { "SPD", "SHD", "BMB", "WLK", "SWD", "GLD" };

Color IK_COL[] = {
    {  80,220, 80,255 },   // SPD
    {  80,150,255,255 },   // SHD
    { 255,140, 40,255 },   // BMB
    { 180, 80,255,255 },   // WLK
    { 255, 60,120,255 },   // SWD
    { 255,200, 40,255 }    // GLD (Gold Touch)
};

#include "globals.h"
#include "game_save.h"
#include "game_map.h"
#include "game_particles.h"
#include "game_sfx.h"
#include <stdio.h>
#include <string.h>
const char *ARTIFACT_NAME[ARTIFACT_COUNT] = {
 "Ember Crown", "Moon Compass", "Void Prism", "Sun Scarab", "Ancient Hourglass",
 "Storm Core", "Blood Chalice", "Frost Lotus", "Serpent Reliquary",
 "Astral Orrery", "Bone Lantern", "Abyss Anchor", "Phoenix Quill",
 "Echo Mask", "Gilded Eye", "Thorn Heart", "Eclipse Halo", "Tide Pearl"
};
int artifactOwned[ARTIFACT_COUNT] = {0};
int floorArtifact = 0;
static bool artifactRatingsLoaded=false;
static bool artifactPlayable[ARTIFACT_COUNT];
static Texture2D artifactTextures[ARTIFACT_COUNT];
static bool artifactTextureChecked[ARTIFACT_COUNT];
static Texture2D coinPileTexture;
static bool coinPileTextureChecked=false;

bool ArtifactDesignPlayable(int id) {
 if(id<0||id>=ARTIFACT_COUNT)return false;
 if(!artifactRatingsLoaded) {
  artifactRatingsLoaded=true;
  char path[512],line[160],state[24];
  snprintf(path,sizeof(path),"%sSnapshot/Library/artifact_ratings.txt",GetApplicationDirectory());
  FILE *file=fopen(path,"r");
  if(file) {
   int ratedId;
   while(fgets(line,sizeof(line),file))
    if(sscanf(line,"%d %23s",&ratedId,state)==2&&ratedId>=0&&ratedId<ARTIFACT_COUNT)
     artifactPlayable[ratedId]=strcmp(state,"ACCEPTED")==0||strcmp(state,"MAYBE/FIX")==0;
   fclose(file);
  } else for(int i=0;i<5;i++)artifactPlayable[i]=true;
  for(int i=5;i<ARTIFACT_COUNT;i++)if(artifactPlayable[i]) {
   snprintf(path,sizeof(path),"%sSnapshot/Library/artifact_%02d.png",GetApplicationDirectory(),i);
   if(!FileExists(path))artifactPlayable[i]=false;
  }
 }
 return artifactPlayable[id];
}

void ShutdownArtifactAssets(void) {
 for(int i=0;i<ARTIFACT_COUNT;i++)if(artifactTextures[i].id)UnloadTexture(artifactTextures[i]);
 if(coinPileTexture.id)UnloadTexture(coinPileTexture);
}

void RefreshSelectedCoinPile(void) {
 if(coinPileTexture.id)UnloadTexture(coinPileTexture);
 coinPileTexture=(Texture2D){0};
 coinPileTextureChecked=false;
}

void DrawCoinPileIcon(Vector2 center,float size) {
 if(!coinPileTextureChecked) {
  coinPileTextureChecked=true;
  char path[512];int selected=0;
  snprintf(path,sizeof(path),"%sSnapshot/Library/coin_pile_selected.txt",GetApplicationDirectory());
  FILE *file=fopen(path,"r");
  if(file) {
   if(fscanf(file,"%d",&selected)!=1||selected<0||selected>=8)selected=0;
   fclose(file);
  }
  snprintf(path,sizeof(path),"%sSnapshot/Library/coin_pile_%02d.png",
           GetApplicationDirectory(),selected);
  if(FileExists(path)) {
   coinPileTexture=LoadTexture(path);
   if(coinPileTexture.id)SetTextureFilter(coinPileTexture,TEXTURE_FILTER_POINT);
  }
 }
 if(coinPileTexture.id) {
  DrawTexturePro(coinPileTexture,
                 (Rectangle){0,(float)coinPileTexture.height,
                             (float)coinPileTexture.width,-(float)coinPileTexture.height},
                 (Rectangle){center.x-size*0.5f,center.y-size*0.5f,size,size},
                 (Vector2){0,0},0,WHITE);
  return;
 }
 DrawEllipse((int)center.x,(int)(center.y+size*0.22f),
             size*0.38f,size*0.14f,(Color){156,88,33,255});
 for(int i=-1;i<=1;i++){
  float x=center.x+i*size*0.2f;
  float y=center.y+size*(i==0?0.08f:0.17f);
  DrawCircle((int)x,(int)y,(int)(size*0.12f),(Color){240,174,54,255});
  DrawCircleLines((int)x,(int)y,size*0.09f,(Color){255,226,120,255});
 }
}

static bool DrawArchivedArtifactIcon(int id,Vector2 center,float size,bool collected) {
 if(!ArtifactDesignPlayable(id))return false;
 if(!artifactTextureChecked[id]) {
  char path[512];
  artifactTextureChecked[id]=true;
  snprintf(path,sizeof(path),"%sSnapshot/Library/artifact_%02d.png",GetApplicationDirectory(),id);
  if(FileExists(path)) {
   artifactTextures[id]=LoadTexture(path);
   if(artifactTextures[id].id)SetTextureFilter(artifactTextures[id],TEXTURE_FILTER_POINT);
  }
 }
 Texture2D texture=artifactTextures[id];
 if(!texture.id)return false;
 DrawTexturePro(texture,(Rectangle){0,(float)texture.height,
                                     (float)texture.width,-(float)texture.height},
                (Rectangle){center.x-size*0.5f,center.y-size*0.5f,size,size},
                (Vector2){0,0},0,collected?WHITE:(Color){120,115,130,255});
 return true;
}
void CollectCoinPile(void) {
 map[py][px]='.';
 coin_balance+=100;totcoi+=100;coins_round+=100;
 StatsAdd(&global_totcoi,100);
 Burst(WX(px),WY(py),(Color){255,203,82,255},28,3);
 PlaySfx(SFX_COIN_PICKUP);
 if(roomKind==RK_ARTIFACT)RevealArtifactTreasure();
 UpdateAndSaveStats();
}

void CollectArtifact(void) {
 if (floorArtifact < 0 || floorArtifact >= ARTIFACT_COUNT) return;
 if(artifactOwned[floorArtifact]){CollectCoinPile();return;}
 map[py][px] = '.';
 PlaySfx(SFX_ARTIFACT_PICKUP);
 artifactOwned[floorArtifact]=1;
 Burst(WX(px),WY(py),(Color){255,30,220,255},24,3);
 if(roomKind==RK_ARTIFACT) RevealArtifactTreasure();
 UpdateAndSaveStats();
}

// Shared collection/world glyphs: each artifact has its own recognizable shape.
void DrawArtifactIcon(int id, Vector2 c, float size, bool collected) {
    if (id<0 || id>=ARTIFACT_COUNT) return;
    if(DrawArchivedArtifactIcon(id,c,size,collected))return;
    float r=size*0.5f, w=size/18.0f;
    Color palette[ARTIFACT_COUNT]={{255,172,55,255},{120,205,255,255},
        {230,90,255,255},{100,230,180,255},{245,212,135,255},
        {120,215,255,255},{212,71,111,255},{170,234,247,255},
        {101,211,129,255},{183,146,255,255},{222,210,165,255},
        {109,190,227,255},{253,139,77,255},{198,137,245,255},
        {235,198,112,255},{129,213,127,255},{235,170,248,255},
        {104,218,225,255}};
    Color ink=collected ? palette[id] : (Color){100,94,118,255};
    Color light=collected ? (Color){255,245,220,255} : (Color){137,130,150,255};
    switch(id) {
        case 0: // crown
            DrawRectangle(c.x-r*.7f,c.y+r*.2f,r*1.4f,r*.4f,ink);
            for(int i=-1;i<=1;i++) {
                float x=c.x+i*r*.48f;
                DrawTriangle((Vector2){x,c.y-r*(i==0?.75f:.45f)},
                    (Vector2){x-r*.26f,c.y+r*.28f},(Vector2){x+r*.26f,c.y+r*.28f},ink);
            }
            DrawCircleV((Vector2){c.x,c.y+r*.36f},r*.13f,light);break;
        case 1: // compass
            DrawRing(c,r*.63f,r*.74f,0,360,48,ink);
            DrawLineEx((Vector2){c.x-r*.85f,c.y},(Vector2){c.x+r*.85f,c.y},w,ink);
            DrawLineEx((Vector2){c.x,c.y-r*.85f},(Vector2){c.x,c.y+r*.85f},w,ink);
            DrawTriangle((Vector2){c.x+r*.38f,c.y-r*.56f},
                (Vector2){c.x-r*.2f,c.y-r*.05f},(Vector2){c.x+r*.14f,c.y+r*.17f},light);
            DrawTriangle((Vector2){c.x-r*.38f,c.y+r*.56f},
                (Vector2){c.x+r*.2f,c.y+r*.05f},(Vector2){c.x-r*.14f,c.y-r*.17f},ink);break;
        case 2: // faceted prism
            DrawPoly(c,4,r*.85f,0,ink);
            DrawLineEx((Vector2){c.x,c.y-r*.85f},(Vector2){c.x-r*.25f,c.y},w,light);
            DrawLineEx((Vector2){c.x-r*.25f,c.y},(Vector2){c.x,c.y+r*.85f},w,light);
            DrawLineEx((Vector2){c.x-r*.85f,c.y},(Vector2){c.x+r*.85f,c.y},w,light);break;
        case 3: // scarab with six legs and solar disc
            for(int side=-1;side<=1;side+=2) for(int i=-1;i<=1;i++)
                DrawLineEx((Vector2){c.x+side*r*.25f,c.y+i*r*.23f},
                    (Vector2){c.x+side*r*.78f,c.y+i*r*.46f},w,ink);
            DrawEllipse(c.x,c.y+r*.1f,r*.43f,r*.55f,ink);
            DrawCircleV((Vector2){c.x,c.y-r*.64f},r*.19f,light);
            DrawLineEx((Vector2){c.x,c.y-r*.28f},(Vector2){c.x,c.y+r*.58f},w,light);break;
        case 4: // hourglass
            DrawRectangle(c.x-r*.65f,c.y-r*.76f,r*1.3f,r*.16f,ink);
            DrawRectangle(c.x-r*.65f,c.y+r*.6f,r*1.3f,r*.16f,ink);
            DrawLineEx((Vector2){c.x-r*.5f,c.y-r*.6f},(Vector2){c.x+r*.5f,c.y+r*.6f},w,ink);
            DrawLineEx((Vector2){c.x+r*.5f,c.y-r*.6f},(Vector2){c.x-r*.5f,c.y+r*.6f},w,ink);
            DrawTriangle((Vector2){c.x,c.y+r*.12f},(Vector2){c.x-r*.38f,c.y+r*.56f},
                (Vector2){c.x+r*.38f,c.y+r*.56f},light);break;
        default:
            DrawRing(c,r*.48f,r*.65f,0,360,24,ink);
            DrawPoly(c,4+(id%3),r*.42f,0,ink);
            DrawCircleV(c,r*.13f,light);
            break;
    }
}
