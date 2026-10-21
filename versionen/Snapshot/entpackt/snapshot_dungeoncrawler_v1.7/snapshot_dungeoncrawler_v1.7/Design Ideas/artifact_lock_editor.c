#include "raylib.h"

#include <stdio.h>
#include <string.h>

#define ARTIFACT_COUNT 18
#define ARTIFACT_DESIGN_COUNT ARTIFACT_COUNT
#define LOCK_COUNT 16
#define KEY_COUNT 15
#define GRID_COLUMNS 4

static const char *const artifactIds[ARTIFACT_COUNT]={
    "ember_crown","moon_compass","void_prism","sun_scarab","ancient_hourglass",
    "storm_core","blood_chalice","frost_lotus","serpent_reliquary",
    "astral_orrery","bone_lantern","abyss_anchor","phoenix_quill",
    "echo_mask","gilded_eye","thorn_heart","eclipse_halo","tide_pearl"
};
static const char *const artifactNames[ARTIFACT_COUNT]={
    "Ember Crown","Moon Compass","Void Prism","Sun Scarab","Ancient Hourglass",
    "Storm Core","Blood Chalice","Frost Lotus","Serpent Reliquary",
    "Astral Orrery","Bone Lantern","Abyss Anchor","Phoenix Quill",
    "Echo Mask","Gilded Eye","Thorn Heart","Eclipse Halo","Tide Pearl"
};
static const char *const lockIds[LOCK_COUNT]={
    "original_chest_motif","chain_vault","rune_reliquary","iron_idol",
    "cursed_coffer","frostbite_reliquary","clockwork_heart",
    "abyssal_treasure_pod","dragonbone_casket","mimic_seed",
    "astral_star_cache","obsidian_sun_box","verdant_thorn_pod",
    "portal_keystone","eclipse_iris_seal","rift_chain_anchor"
};
static const char *const lockNames[LOCK_COUNT]={
    "Original Chest Motif","Chain Vault","Rune Reliquary","Iron Idol",
    "Cursed Coffer","Frostbite Reliquary","Clockwork Heart",
    "Abyssal Treasure Pod","Dragonbone Casket","Mimic Seed",
    "Astral Star Cache","Obsidian Sun Box","Verdant Thorn Pod",
    "Portal Keystone","Eclipse Iris Seal","Rift Chain Anchor"
};
static const char *const keyIds[KEY_COUNT]={
    "tutorial_book_key","starforge","rift_thorn","bone_reliquary","sunbreaker",
    "moonveil","verdant_coil","frostglass","clockheart","abyssal_trident",
    "dragonfang","storm_compass","mourning_bell","wardens_oath","magma_crown"
};
static const char *const keyNames[KEY_COUNT]={
    "Tutorial Book Key","Starforge","Rift Thorn","Bone Reliquary","Sunbreaker",
    "Moonveil","Verdant Coil","Frostglass","Clockheart","Abyssal Trident",
    "Dragonfang","Storm Compass","Mourning Bell","Warden's Oath","Magma Crown"
};

static bool artifactLocks[ARTIFACT_COUNT][LOCK_COUNT];
static bool lockKeys[LOCK_COUNT][KEY_COUNT];
static bool lockAvailable[LOCK_COUNT];
static bool keyAvailable[KEY_COUNT];
static bool artifactDesignAvailable[ARTIFACT_DESIGN_COUNT];
static int lockRating[LOCK_COUNT];
static int keyRating[KEY_COUNT];
static int artifactDesignRating[ARTIFACT_DESIGN_COUNT];
static Texture2D lockTextures[LOCK_COUNT];
static Texture2D keyTextures[KEY_COUNT];
static Texture2D artifactDesignTextures[ARTIFACT_DESIGN_COUNT];
static int mode=0;
static int selectedArtifact=0;
static int selectedLock=0;
static double messageUntil=0.0;
static char message[128]="Changes are saved automatically.";

static void MakePath(char *path,size_t capacity,const char *relative) {
    snprintf(path,capacity,"%s%s",GetApplicationDirectory(),relative);
}

static int FindId(const char *name,const char *const *ids,int count) {
    for(int i=0;i<count;i++)if(strcmp(name,ids[i])==0)return i;
    return -1;
}

static int ReadRatedIds(const char *ratingFile,const char *selectionFile,
                        const char *winnerFile,int count,int *ratings) {
    for(int i=0;i<count;i++)ratings[i]=0;
    char path[512],line[160],state[24];
    MakePath(path,sizeof(path),ratingFile);
    FILE *file=fopen(path,"r");
    if(file) {
        int id;
        while(fgets(line,sizeof(line),file)) {
            if(sscanf(line,"%d %23s",&id,state)!=2||id<0||id>=count)continue;
            if(strcmp(state,"ACCEPTED")==0)ratings[id]=3;
            else if(strcmp(state,"MAYBE/FIX")==0)ratings[id]=2;
        }
        fclose(file);
        return 1;
    }

    MakePath(path,sizeof(path),selectionFile);
    file=fopen(path,"r");
    if(file) {
        while(fgets(line,sizeof(line),file)) {
            int id;
            if(sscanf(line,"%d",&id)==1&&id>=0&&id<count)ratings[id]=3;
        }
        fclose(file);
        return 1;
    }

    MakePath(path,sizeof(path),winnerFile);
    file=fopen(path,"r");
    if(file) {
        int id=-1;
        if(fscanf(file,"%d",&id)==1&&id>=0&&id<count)ratings[id]=3;
        fclose(file);
        return 1;
    }
    return 0;
}

static void LoadCandidateSprites(void) {
    ReadRatedIds("Library/key_ratings.txt","Library/key_selections.txt",
                 "Library/key_winner.txt",KEY_COUNT,keyRating);
    ReadRatedIds("Library/locked_object_ratings.txt",
                 "Library/locked_object_selections.txt",
                 "Library/locked_object_winner.txt",LOCK_COUNT,lockRating);
    ReadRatedIds("Library/artifact_ratings.txt","Library/artifact_selections.txt",
                 "Library/artifact_winner.txt",ARTIFACT_DESIGN_COUNT,artifactDesignRating);

    for(int i=0;i<KEY_COUNT;i++) {
        char relative[128],path[512];
        snprintf(relative,sizeof(relative),"Library/key_%02d.png",i);
        MakePath(path,sizeof(path),relative);
        if((keyRating[i]==2||keyRating[i]==3)&&FileExists(path)) {
            keyTextures[i]=LoadTexture(path);
            keyAvailable[i]=keyTextures[i].id!=0;
            if(keyAvailable[i])SetTextureFilter(keyTextures[i],TEXTURE_FILTER_POINT);
        }
    }
    for(int i=0;i<LOCK_COUNT;i++) {
        char relative[128],path[512];
        snprintf(relative,sizeof(relative),"Library/locked_%02d.png",i);
        MakePath(path,sizeof(path),relative);
        if((lockRating[i]==2||lockRating[i]==3)&&FileExists(path)) {
            lockTextures[i]=LoadTexture(path);
            lockAvailable[i]=lockTextures[i].id!=0;
            if(lockAvailable[i])SetTextureFilter(lockTextures[i],TEXTURE_FILTER_POINT);
        }
    }
    for(int i=0;i<ARTIFACT_DESIGN_COUNT;i++) {
        char relative[128],path[512];
        snprintf(relative,sizeof(relative),"Library/artifact_%02d.png",i);
        MakePath(path,sizeof(path),relative);
        if((artifactDesignRating[i]==2||artifactDesignRating[i]==3)&&FileExists(path)) {
            artifactDesignTextures[i]=LoadTexture(path);
            artifactDesignAvailable[i]=artifactDesignTextures[i].id!=0;
            if(artifactDesignAvailable[i])
                SetTextureFilter(artifactDesignTextures[i],TEXTURE_FILTER_POINT);
        }
    }
}

static char *Trim(char *text) {
    while(*text==' '||*text=='\t'||*text=='\r'||*text=='\n')text++;
    char *end=text+strlen(text);
    while(end>text&&(end[-1]==' '||end[-1]=='\t'||end[-1]=='\r'||end[-1]=='\n'))
        *--end='\0';
    return text;
}

static void LoadAssignments(void) {
    char path[512],line[512];
    MakePath(path,sizeof(path),"Library/artifact_lock_assignments.txt");
    FILE *file=fopen(path,"r");
    if(!file)return;

    while(fgets(line,sizeof(line),file)) {
        char *entry=Trim(line);
        if(!*entry||*entry=='#'||*entry==';')continue;
        char *equals=strchr(entry,'=');
        if(!equals)continue;
        *equals='\0';
        char *name=Trim(entry),*values=Trim(equals+1);
        if(strncmp(name,"artifact.",9)==0) {
            int artifact=FindId(name+9,artifactIds,ARTIFACT_COUNT);
            if(artifact<0)continue;
            for(char *token=strtok(values,",");token;token=strtok(NULL,",")) {
                int lock=FindId(Trim(token),lockIds,LOCK_COUNT);
                if(lock>=0)artifactLocks[artifact][lock]=true;
            }
        } else if(strncmp(name,"lock.",5)==0) {
            int lock=FindId(name+5,lockIds,LOCK_COUNT);
            if(lock<0)continue;
            for(char *token=strtok(values,",");token;token=strtok(NULL,",")) {
                int key=FindId(Trim(token),keyIds,KEY_COUNT);
                if(key>=0)lockKeys[lock][key]=true;
            }
        }
    }
    fclose(file);
}

static bool SaveAssignments(void) {
    char path[512];
    MakePath(path,sizeof(path),"Library/artifact_lock_assignments.txt");
    FILE *file=fopen(path,"w");
    if(!file)return false;
    fprintf(file,"# Artifact lock assignments. Edit with artifact_lock_editor.exe.\n");
    fprintf(file,"# Multiple artifact locks are possible variants; one is selected per artifact spawn.\n");
    fprintf(file,"# Multiple keys are alternatives; any listed key opens the locked object.\n");
    fprintf(file,"# Empty artifact lists mean no lock. Empty key lists keep a lock sealed.\n\n");
    for(int artifact=0;artifact<ARTIFACT_COUNT;artifact++) {
        fprintf(file,"artifact.%s=",artifactIds[artifact]);
        bool first=true;
        for(int lock=0;lock<LOCK_COUNT;lock++)if(artifactLocks[artifact][lock]) {
            fprintf(file,"%s%s",first?"":",",lockIds[lock]);
            first=false;
        }
        fputc('\n',file);
    }
    fputc('\n',file);
    for(int lock=0;lock<LOCK_COUNT;lock++) {
        fprintf(file,"lock.%s=",lockIds[lock]);
        bool first=true;
        for(int key=0;key<KEY_COUNT;key++)if(lockKeys[lock][key]) {
            fprintf(file,"%s%s",first?"":",",keyIds[key]);
            first=false;
        }
        fputc('\n',file);
    }
    bool ok=fclose(file)==0;
    return ok;
}

static int CountArtifactLocks(int artifact) {
    int total=0;
    for(int i=0;i<LOCK_COUNT;i++)if(artifactLocks[artifact][i])total++;
    return total;
}

static int CountLockKeys(int lock) {
    int total=0;
    for(int i=0;i<KEY_COUNT;i++)if(lockKeys[lock][i])total++;
    return total;
}

static int CountLockArtifacts(int lock) {
    int total=0;
    for(int i=0;i<ARTIFACT_COUNT;i++)if(artifactLocks[i][lock])total++;
    return total;
}

static int CountKeyLocks(int key) {
    int total=0;
    for(int i=0;i<LOCK_COUNT;i++)if(lockKeys[i][key])total++;
    return total;
}

static int CountKeyArtifacts(int key) {
    int total=0;
    for(int artifact=0;artifact<ARTIFACT_COUNT;artifact++) {
        bool reachable=false;
        for(int lock=0;lock<LOCK_COUNT;lock++)
            if(artifactLocks[artifact][lock]&&lockKeys[lock][key])reachable=true;
        if(reachable)total++;
    }
    return total;
}

static int CountArtifactsWithLocks(void) {
    int total=0;
    for(int i=0;i<ARTIFACT_COUNT;i++)if(CountArtifactLocks(i)>0)total++;
    return total;
}

static int CountAllArtifactLockLinks(void) {
    int total=0;
    for(int i=0;i<LOCK_COUNT;i++)total+=CountLockArtifacts(i);
    return total;
}

static int CountAllLockKeyLinks(void) {
    int total=0;
    for(int i=0;i<KEY_COUNT;i++)total+=CountKeyLocks(i);
    return total;
}

static void DrawArtifactGlyph(int id,Vector2 center,float size,Color ink,Color light) {
    float r=size*0.5f,w=size/18.0f;
    switch(id) {
        case 0:
            DrawRectangle((int)(center.x-r*.7f),(int)(center.y+r*.2f),
                          (int)(r*1.4f),(int)(r*.4f),ink);
            for(int i=-1;i<=1;i++) {
                float x=center.x+i*r*.48f;
                DrawTriangle((Vector2){x,center.y-r*(i==0?.75f:.45f)},
                    (Vector2){x-r*.26f,center.y+r*.28f},
                    (Vector2){x+r*.26f,center.y+r*.28f},ink);
            }
            DrawCircleV((Vector2){center.x,center.y+r*.36f},r*.13f,light);
            break;
        case 1:
            DrawRing(center,r*.63f,r*.74f,0,360,48,ink);
            DrawLineEx((Vector2){center.x-r*.85f,center.y},
                       (Vector2){center.x+r*.85f,center.y},w,ink);
            DrawLineEx((Vector2){center.x,center.y-r*.85f},
                       (Vector2){center.x,center.y+r*.85f},w,ink);
            DrawTriangle((Vector2){center.x+r*.38f,center.y-r*.56f},
                (Vector2){center.x-r*.2f,center.y-r*.05f},
                (Vector2){center.x+r*.14f,center.y+r*.17f},light);
            DrawTriangle((Vector2){center.x-r*.38f,center.y+r*.56f},
                (Vector2){center.x+r*.2f,center.y+r*.05f},
                (Vector2){center.x-r*.14f,center.y-r*.17f},ink);
            break;
        case 2:
            DrawPoly(center,4,r*.85f,0,ink);
            DrawLineEx((Vector2){center.x,center.y-r*.85f},
                       (Vector2){center.x-r*.25f,center.y},w,light);
            DrawLineEx((Vector2){center.x-r*.25f,center.y},
                       (Vector2){center.x,center.y+r*.85f},w,light);
            DrawLineEx((Vector2){center.x-r*.85f,center.y},
                       (Vector2){center.x+r*.85f,center.y},w,light);
            break;
        case 3:
            for(int side=-1;side<=1;side+=2)for(int i=-1;i<=1;i++)
                DrawLineEx((Vector2){center.x+side*r*.25f,center.y+i*r*.23f},
                    (Vector2){center.x+side*r*.78f,center.y+i*r*.46f},w,ink);
            DrawEllipse((int)center.x,(int)(center.y+r*.1f),r*.43f,r*.55f,ink);
            DrawCircleV((Vector2){center.x,center.y-r*.64f},r*.19f,light);
            DrawLineEx((Vector2){center.x,center.y-r*.28f},
                       (Vector2){center.x,center.y+r*.58f},w,light);
            break;
        case 4:
            DrawRectangle((int)(center.x-r*.65f),(int)(center.y-r*.76f),
                          (int)(r*1.3f),(int)(r*.16f),ink);
            DrawRectangle((int)(center.x-r*.65f),(int)(center.y+r*.6f),
                          (int)(r*1.3f),(int)(r*.16f),ink);
            DrawLineEx((Vector2){center.x-r*.5f,center.y-r*.6f},
                       (Vector2){center.x+r*.5f,center.y+r*.6f},w,ink);
            DrawLineEx((Vector2){center.x+r*.5f,center.y-r*.6f},
                       (Vector2){center.x-r*.5f,center.y+r*.6f},w,ink);
            DrawTriangle((Vector2){center.x,center.y+r*.12f},
                (Vector2){center.x-r*.38f,center.y+r*.56f},
                (Vector2){center.x+r*.38f,center.y+r*.56f},light);
            break;
    }
}

static void SetMessage(const char *text) {
    snprintf(message,sizeof(message),"%s",text);
    messageUntil=GetTime()+2.4;
}

static void ClearCurrent(void) {
    if(mode==0) {
        memset(artifactLocks[selectedArtifact],0,sizeof(artifactLocks[selectedArtifact]));
        if(SaveAssignments())SetMessage("No locked object assigned to this artifact.");
        else SetMessage("Could not save artifact_lock_assignments.txt.");
    } else if(mode==1) {
        memset(lockKeys[selectedLock],0,sizeof(lockKeys[selectedLock]));
        if(SaveAssignments())SetMessage("All keys removed from this locked object.");
        else SetMessage("Could not save artifact_lock_assignments.txt.");
    }
}

static void DrawWrappedText(const char *text,int x,int y,int fontSize,
                            int maxWidth,Color color) {
    char line[128]="";
    const char *cursor=text;
    int lineY=y;
    while(*cursor) {
        char word[64];
        int n=0;
        while(*cursor&&*cursor!=' '&&n<(int)sizeof(word)-1)word[n++]=*cursor++;
        word[n]='\0';
        if(*cursor==' ')cursor++;
        char candidate[128];
        snprintf(candidate,sizeof(candidate),"%s%s%s",line,line[0]?" ":"",word);
        if(line[0]&&MeasureText(candidate,fontSize)>maxWidth) {
            DrawText(line,x,lineY,fontSize,color);
            lineY+=fontSize+3;
            snprintf(line,sizeof(line),"%s",word);
        } else snprintf(line,sizeof(line),"%s",candidate);
    }
    if(line[0])DrawText(line,x,lineY,fontSize,color);
}

static void DrawCard(Rectangle bounds,int id,const char *name,Texture2D texture,
                     int rating,bool assigned,bool selected) {
    bool highlighted=assigned||selected;
    Color bg=highlighted?(Color){35,74,78,255}:(Color){31,34,49,255};
    Color edge=highlighted?(Color){105,230,194,255}:(Color){76,82,105,255};
    DrawRectangleRounded(bounds,0.10f,7,bg);
    DrawRectangleRoundedLinesEx(bounds,0.10f,7,highlighted?2.5f:1.2f,edge);
    Rectangle source={0,(float)texture.height,(float)texture.width,-(float)texture.height};
    Rectangle dest={bounds.x+12,bounds.y+32,82,82};
    DrawTexturePro(texture,source,dest,(Vector2){0,0},0,WHITE);
    DrawRectangleLinesEx(dest,1.0f,(Color){103,108,132,255});
    DrawText(TextFormat("%02d",id+1),(int)bounds.x+13,(int)bounds.y+8,13,
             (Color){156,169,190,255});
    DrawWrappedText(name,(int)bounds.x+105,(int)bounds.y+34,15,119,
                    (Color){239,237,247,255});
    const char *status=assigned?"ASSIGNED":(selected?"SELECTED":(rating==2?"MAYBE / FIX":"ACCEPTED"));
    Color statusColor=highlighted?(Color){105,230,194,255}:
                      (rating==2?(Color){246,189,100,255}:(Color){150,166,187,255});
    DrawText(status,(int)bounds.x+105,(int)bounds.y+112,11,statusColor);
}

static void DrawNoLockCard(Rectangle bounds,bool selected) {
    Color bg=selected?(Color){35,74,78,255}:(Color){39,36,51,255};
    Color edge=selected?(Color){105,230,194,255}:(Color){194,160,112,255};
    DrawRectangleRounded(bounds,0.10f,7,bg);
    DrawRectangleRoundedLinesEx(bounds,0.10f,7,selected?3.0f:1.8f,edge);
    Vector2 center={bounds.x+53,bounds.y+72};
    DrawCircleLines((int)center.x,(int)center.y,24,edge);
    DrawCircleLines((int)center.x,(int)center.y,10,edge);
    DrawLineEx((Vector2){center.x-20,center.y+20},
               (Vector2){center.x+20,center.y-20},4.0f,edge);
    DrawText("NO LOCKED OBJECT",(int)bounds.x+105,(int)bounds.y+35,14,
             (Color){245,236,214,255});
    DrawWrappedText("Artifact stays unlocked",(int)bounds.x+105,
                    (int)bounds.y+61,12,119,(Color){187,193,205,255});
    DrawText(selected?"SELECTED":"CLICK TO SELECT",
             (int)bounds.x+105,(int)bounds.y+112,11,edge);
}

static void DrawUsageRow(Rectangle bounds,int id,const char *name,
                         Texture2D texture,int count,const char *detail) {
    Color edge={75,82,105,255};
    Color muted={157,164,186,255};
    DrawRectangleRounded(bounds,0.12f,5,(Color){31,34,49,255});
    DrawRectangleRoundedLinesEx(bounds,0.12f,5,1.0f,edge);
    Rectangle imageBounds={bounds.x+4,bounds.y+2,32,32};
    if(texture.id) {
        Rectangle source={0,(float)texture.height,(float)texture.width,
                          -(float)texture.height};
        DrawTexturePro(texture,source,imageBounds,(Vector2){0,0},0,WHITE);
    } else {
        DrawRectangleRounded(imageBounds,0.12f,4,(Color){47,48,62,255});
        DrawText(TextFormat("%02d",id+1),(int)imageBounds.x+5,
                 (int)imageBounds.y+10,11,muted);
    }
    DrawText(name,(int)bounds.x+44,(int)bounds.y+4,13,(Color){239,237,247,255});
    DrawText(detail,(int)bounds.x+44,(int)bounds.y+21,10,muted);
    const char *number=TextFormat("%d",count);
    DrawText(number,(int)(bounds.x+bounds.width-19-MeasureText(number,19)),
             (int)bounds.y+8,19,
             count>0?(Color){105,230,194,255}:muted);
}

static void DrawUsageOverview(Rectangle leftPanel,Rectangle rightPanel,
                              Color panel,Color muted) {
    DrawRectangleRounded(leftPanel,0.035f,5,panel);
    DrawRectangleRounded(rightPanel,0.035f,5,panel);
    DrawText("OVERVIEW",(int)leftPanel.x+17,(int)leftPanel.y+15,16,
             (Color){185,198,221,255});
    DrawText("ARTIFACTS WITH LOCKS",(int)leftPanel.x+17,230,13,muted);
    DrawText(TextFormat("%d / %d",CountArtifactsWithLocks(),ARTIFACT_COUNT),
             (int)leftPanel.x+17,252,29,(Color){105,230,194,255});
    DrawText("LOCK CHOICES",(int)leftPanel.x+17,326,13,muted);
    DrawText(TextFormat("%d",CountAllArtifactLockLinks()),
             (int)leftPanel.x+17,348,29,(Color){105,230,194,255});
    DrawText("KEY LINKS",(int)leftPanel.x+17,422,13,muted);
    DrawText(TextFormat("%d",CountAllLockKeyLinks()),
             (int)leftPanel.x+17,444,29,(Color){105,230,194,255});
    DrawWrappedText("Counts reflect saved assignments. An artifact can have several possible locked objects. A key can open several locked objects.",
                    (int)leftPanel.x+17,537,13,(int)leftPanel.width-34,muted);

    DrawText("ASSIGNMENT USAGE",(int)rightPanel.x+18,
             (int)rightPanel.y+17,18,(Color){220,226,243,255});
    DrawText("All designs are listed. Counts update when you change an assignment.",
             (int)rightPanel.x+18,(int)rightPanel.y+47,13,muted);
    const int lockX=(int)rightPanel.x+18;
    const int keyX=lockX+501;
    DrawText("LOCKED OBJECTS",lockX,242,15,(Color){220,226,243,255});
    DrawText("ARTIFACTS",lockX+389,243,12,muted);
    DrawText("KEYS",keyX,242,15,(Color){220,226,243,255});
    DrawText("LOCKS",keyX+432,243,12,muted);
    for(int i=0;i<LOCK_COUNT;i++) {
        Rectangle row={(float)lockX,(float)(276+i*35),492,34};
        DrawUsageRow(row,i,lockNames[i],lockTextures[i],
                     CountLockArtifacts(i),TextFormat("%d keys assigned",CountLockKeys(i)));
    }
    for(int i=0;i<KEY_COUNT;i++) {
        Rectangle row={(float)keyX,(float)(276+i*35),492,34};
        DrawUsageRow(row,i,keyNames[i],keyTextures[i],
                     CountKeyLocks(i),TextFormat("%d artifacts reachable",CountKeyArtifacts(i)));
    }
}

static bool PointIn(Rectangle bounds,Vector2 point) {
    return CheckCollisionPointRec(point,bounds);
}

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(1380,900,"Artifact Lock Editor - Dungeon Crawler");
    SetTargetFPS(60);
    char libraryDirectory[512];
    snprintf(libraryDirectory,sizeof(libraryDirectory),"%sLibrary",GetApplicationDirectory());
    MakeDirectory(libraryDirectory);
    LoadCandidateSprites();
    LoadAssignments();
    for(int i=0;i<LOCK_COUNT;i++)if(lockAvailable[i]){selectedLock=i;break;}
    for(int i=0;i<ARTIFACT_COUNT;i++)if(artifactDesignAvailable[i]){selectedArtifact=i;break;}

    const Color background={17,19,30,255};
    const Color panel={24,27,41,255};
    const Color muted={157,164,186,255};
    const Rectangle tabArtifacts={28,78,290,44};
    const Rectangle tabKeys={328,78,290,44};
    const Rectangle tabUsage={628,78,250,44};
    const Rectangle saveButton={1183,76,165,46};
    const Rectangle clearButton={992,76,176,46};
    const Rectangle leftPanel={28,169,270,691};
    const Rectangle rightPanel={310,169,1040,691};
    float artifactListScroll=0.0f;
    float lockListScroll=0.0f;

    while(!WindowShouldClose()) {
        Vector2 mouse=GetMousePosition();
        bool clicked=IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        Rectangle sidebarClip={leftPanel.x+10,leftPanel.y+43,leftPanel.width-20,
                               leftPanel.height-54};
        int artifactOptions[ARTIFACT_COUNT];
        int artifactOptionCount=0;
        for(int i=0;i<ARTIFACT_COUNT;i++)
            if(artifactDesignAvailable[i])artifactOptions[artifactOptionCount++]=i;
        float artifactRowHeight=48.0f;
        float artifactListMaxScroll=artifactOptionCount*artifactRowHeight-sidebarClip.height;
        if(artifactListMaxScroll<0.0f)artifactListMaxScroll=0.0f;
        int availableLockCount=0;
        for(int i=0;i<LOCK_COUNT;i++)if(lockAvailable[i])availableLockCount++;
        float lockListMaxScroll=availableLockCount*48.0f-sidebarClip.height;
        if(lockListMaxScroll<0.0f)lockListMaxScroll=0.0f;
        if(IsKeyPressed(KEY_ESCAPE))break;
        if(IsKeyDown(KEY_LEFT_CONTROL)&&IsKeyPressed(KEY_S)) {
            if(SaveAssignments())SetMessage("Assignments saved.");
            else SetMessage("Could not save artifact_lock_assignments.txt.");
        }
        if(clicked&&PointIn(tabArtifacts,mouse))mode=0;
        if(clicked&&PointIn(tabKeys,mouse))mode=1;
        if(clicked&&PointIn(tabUsage,mouse))mode=2;
        if(clicked&&PointIn(saveButton,mouse)) {
            if(SaveAssignments())SetMessage("Assignments saved.");
            else SetMessage("Could not save artifact_lock_assignments.txt.");
        }
        if(mode!=2&&clicked&&PointIn(clearButton,mouse))ClearCurrent();

        if(mode==0&&PointIn(sidebarClip,mouse))
            artifactListScroll-=GetMouseWheelMove()*artifactRowHeight;
        else if(mode!=0)artifactListScroll=0.0f;
        if(mode==1&&PointIn(sidebarClip,mouse))
            lockListScroll-=GetMouseWheelMove()*48.0f;
        else if(mode!=1)lockListScroll=0.0f;
        if(artifactListScroll<0.0f)artifactListScroll=0.0f;
        if(artifactListScroll>artifactListMaxScroll)artifactListScroll=artifactListMaxScroll;
        if(lockListScroll<0.0f)lockListScroll=0.0f;
        if(lockListScroll>lockListMaxScroll)lockListScroll=lockListMaxScroll;

        if(clicked&&PointIn(sidebarClip,mouse)) {
            if(mode==0) {
                int index=(int)((mouse.y-sidebarClip.y+artifactListScroll)/artifactRowHeight);
                if(index>=0&&index<artifactOptionCount)selectedArtifact=artifactOptions[index];
            } else if(mode==1) {
                int index=(int)((mouse.y-sidebarClip.y+lockListScroll)/48.0f);
                int visible=0;
                for(int i=0;i<LOCK_COUNT;i++)if(lockAvailable[i]) {
                    if(index==visible){selectedLock=i;break;}
                    visible++;
                }
            }
        }

        int candidates[ARTIFACT_DESIGN_COUNT];
        int candidateCount=0;
        if(mode==0) {
            for(int i=0;i<LOCK_COUNT;i++)if(lockAvailable[i])candidates[candidateCount++]=i;
        } else if(mode==1) {
            for(int i=0;i<KEY_COUNT;i++)if(keyAvailable[i])candidates[candidateCount++]=i;
        }
        const float cardW=238.0f,cardH=140.0f,gapX=10.0f,gapY=10.0f;
        const float gridX=328.0f,gridY=245.0f;
        Rectangle gridClip={gridX,gridY,rightPanel.width-36.0f,
                            rightPanel.y+rightPanel.height-19.0f-gridY};
        Rectangle noLockCard={gridX,gridY,cardW,cardH};
        if(mode!=2&&clicked&&PointIn(gridClip,mouse)&&(mode==1||artifactOptionCount>0)) {
            if(mode==0&&PointIn(noLockCard,mouse)) {
                memset(artifactLocks[selectedArtifact],0,sizeof(artifactLocks[selectedArtifact]));
                if(SaveAssignments())SetMessage("No locked object assigned to this artifact.");
                else SetMessage("Could not save artifact_lock_assignments.txt.");
            } else {
                for(int slot=0;slot<candidateCount;slot++) {
                    int gridIndex=slot+(mode==0?1:0);
                    int col=gridIndex%GRID_COLUMNS,row=gridIndex/GRID_COLUMNS;
                    Rectangle card={gridX+col*(cardW+gapX),gridY+row*(cardH+gapY),
                                    cardW,cardH};
                    if(!PointIn(card,mouse))continue;
                    int id=candidates[slot];
                    if(mode==0)artifactLocks[selectedArtifact][id]=!artifactLocks[selectedArtifact][id];
                    else lockKeys[selectedLock][id]=!lockKeys[selectedLock][id];
                    if(SaveAssignments())SetMessage("Selection saved.");
                    else SetMessage("Could not save artifact_lock_assignments.txt.");
                    break;
                }
            }
        }

        BeginDrawing();
        ClearBackground(background);
        DrawText("ARTIFACT LOCK EDITOR",30,22,30,(Color){242,235,255,255});
        DrawText("Assign locked objects to artifacts and keys to locked objects.",
                 32,54,15,muted);
        DrawRectangleRounded(tabArtifacts,0.18f,6,mode==0?(Color){53,82,92,255}:panel);
        DrawRectangleRounded(tabKeys,0.18f,6,mode==1?(Color){53,82,92,255}:panel);
        DrawRectangleRounded(tabUsage,0.18f,6,mode==2?(Color){53,82,92,255}:panel);
        DrawText("ARTIFACTS  >  LOCK OBJECTS",(int)tabArtifacts.x+19,
                 (int)tabArtifacts.y+13,15,(Color){238,238,248,255});
        DrawText("LOCK OBJECTS  >  KEYS",(int)tabKeys.x+19,
                 (int)tabKeys.y+13,15,(Color){238,238,248,255});
        DrawText("USAGE OVERVIEW",(int)tabUsage.x+19,
                 (int)tabUsage.y+13,15,(Color){238,238,248,255});
        if(mode!=2) {
            DrawRectangleRounded(clearButton,0.18f,6,(Color){75,54,65,255});
            DrawText(mode==0?"REMOVE ALL LOCKS":"REMOVE ALL KEYS",
                     (int)clearButton.x+19,(int)clearButton.y+15,13,
                     (Color){248,205,211,255});
        }
        DrawRectangleRounded(saveButton,0.18f,6,(Color){39,105,91,255});
        DrawText("SAVE",(int)saveButton.x+57,(int)saveButton.y+14,16,WHITE);
        if(GetTime()<messageUntil)DrawText(message,660,140,13,(Color){142,228,202,255});
        else DrawText(mode==2?"Usage counts update as assignments change.":
                      "Click a card to add or remove an assignment. Changes save automatically.",
                      660,140,13,muted);

        if(mode==2) {
            DrawUsageOverview(leftPanel,rightPanel,panel,muted);
            DrawText("ESC: close  |  CTRL+S: save",1083,871,13,muted);
            EndDrawing();
            continue;
        }

        DrawRectangleRounded(leftPanel,0.035f,5,panel);
        DrawRectangleRounded(rightPanel,0.035f,5,panel);
        DrawText(mode==0?TextFormat("ARTIFACT IDEAS  (%d)",artifactOptionCount):"LOCKED OBJECTS",
                 (int)leftPanel.x+17,
                 (int)leftPanel.y+15,16,(Color){185,198,221,255});
        DrawText(mode==0?"LOCK OBJECT PICKS":"KEY PICKS",
                 (int)rightPanel.x+18,(int)rightPanel.y+17,16,
                 (Color){185,198,221,255});

        if(mode==0) {
            BeginScissorMode((int)sidebarClip.x,(int)sidebarClip.y,
                             (int)sidebarClip.width,(int)sidebarClip.height);
            int rowIndex=0;
            for(int i=0;i<ARTIFACT_COUNT;i++)if(artifactDesignAvailable[i]) {
                Rectangle row={sidebarClip.x,
                    sidebarClip.y+rowIndex*artifactRowHeight-artifactListScroll,
                    sidebarClip.width,44.0f};
                bool active=i==selectedArtifact;
                DrawRectangleRounded(row,0.14f,5,active?(Color){43,67,79,255}:
                                     (Color){31,34,49,255});
                DrawRectangleRoundedLinesEx(row,0.14f,5,active?2.0f:1.0f,
                    active?(Color){116,209,210,255}:(Color){67,73,94,255});
                Rectangle source={0,(float)artifactDesignTextures[i].height,
                    (float)artifactDesignTextures[i].width,
                    -(float)artifactDesignTextures[i].height};
                Rectangle dest={row.x+6,row.y+4,36,36};
                if(artifactDesignTextures[i].id)
                    DrawTexturePro(artifactDesignTextures[i],source,dest,(Vector2){0,0},0,WHITE);
                else if(i<5)
                    DrawArtifactGlyph(i,(Vector2){dest.x+18,dest.y+18},34,
                                      (Color){120,205,255,255},WHITE);
                DrawText(artifactNames[i],(int)row.x+49,(int)row.y+5,12,
                         (Color){241,237,249,255});
                Color ratingColor=artifactDesignRating[i]==2?
                    (Color){246,189,100,255}:(Color){105,230,194,255};
                DrawText(artifactDesignRating[i]==2?"MAYBE / FIX":"ACCEPTED",
                         (int)row.x+49,(int)row.y+25,9,ratingColor);
                DrawText(TextFormat("%d locks",CountArtifactLocks(i)),
                         (int)row.x+164,(int)row.y+25,9,muted);
                rowIndex++;
            }
            EndScissorMode();
            if(artifactOptionCount==0)
                DrawText("No ACCEPTED or MAYBE / FIX artifact designs.",
                         (int)sidebarClip.x+6,(int)sidebarClip.y+16,11,muted);
        } else if(mode==1) {
            BeginScissorMode((int)sidebarClip.x,(int)sidebarClip.y,
                             (int)sidebarClip.width,(int)sidebarClip.height);
            int rowIndex=0;
            for(int i=0;i<LOCK_COUNT;i++)if(lockAvailable[i]) {
                Rectangle row={sidebarClip.x,sidebarClip.y+rowIndex*48.0f-lockListScroll,
                               sidebarClip.width,42.0f};
                bool active=i==selectedLock;
                DrawRectangleRounded(row,0.14f,5,active?(Color){43,67,79,255}:
                                     (Color){31,34,49,255});
                DrawRectangleRoundedLinesEx(row,0.14f,5,active?2.0f:1.0f,
                    active?(Color){116,209,210,255}:(Color){67,73,94,255});
                Rectangle source={0,(float)lockTextures[i].height,
                                  (float)lockTextures[i].width,
                                  -(float)lockTextures[i].height};
                Rectangle dest={row.x+6,row.y+3,36,36};
                DrawTexturePro(lockTextures[i],source,dest,(Vector2){0,0},0,WHITE);
                DrawRectangleLinesEx(dest,1.0f,(Color){103,108,132,255});
                DrawText(lockNames[i],(int)row.x+49,(int)row.y+5,12,
                         (Color){241,237,249,255});
                DrawText(lockRating[i]==2?"MAYBE / FIX":"ACCEPTED",
                         (int)row.x+49,(int)row.y+26,9,
                         lockRating[i]==2?(Color){246,189,100,255}:
                                          (Color){105,230,194,255});
                DrawText(TextFormat("%d keys",CountLockKeys(i)),
                         (int)row.x+177,(int)row.y+26,9,muted);
                rowIndex++;
            }
            EndScissorMode();
            if(rowIndex==0)DrawText("No eligible locked objects found.",
                (int)sidebarClip.x+8,(int)sidebarClip.y+14,13,muted);
        }

        if(mode==0) {
            int lockCount=CountArtifactLocks(selectedArtifact);
            DrawText(lockCount==0?
                     TextFormat("%s  |  No locked object assigned",artifactNames[selectedArtifact]):
                     TextFormat("%s  |  %d lock choices; one is selected per spawn",
                                artifactNames[selectedArtifact],lockCount),
                     (int)rightPanel.x+19,(int)rightPanel.y+49,14,
                     (Color){226,222,238,255});
        } else if(mode==1) {
            DrawText(CountLockKeys(selectedLock)>0?
                     TextFormat("%s  |  %d keys; any assigned key opens it",
                                lockNames[selectedLock],CountLockKeys(selectedLock)):
                     TextFormat("%s  |  No key assigned: remains sealed",
                                lockNames[selectedLock]),
                    (int)rightPanel.x+19,(int)rightPanel.y+49,14,
                    (Color){226,222,238,255});
        }
        if(candidateCount==0) {
            if(mode==0)
                DrawWrappedText("No eligible locked object sprites. Rate designs ACCEPTED or MAYBE / FIX, then launch their gallery.",
                                (int)(gridX+cardW+gapX+8),
                                (int)gridY+18,15,725,muted);
            else
                DrawText("No eligible archived sprites. Rate designs ACCEPTED or MAYBE / FIX, then launch their gallery.",
                         (int)rightPanel.x+24,(int)rightPanel.y+115,15,muted);
        }
        BeginScissorMode((int)gridClip.x,(int)gridClip.y,
                         (int)gridClip.width,(int)gridClip.height);
        if(mode==0)DrawNoLockCard(noLockCard,CountArtifactLocks(selectedArtifact)==0);
        for(int slot=0;slot<candidateCount;slot++) {
            int gridIndex=slot+(mode==0?1:0);
            int col=gridIndex%GRID_COLUMNS,row=gridIndex/GRID_COLUMNS,id=candidates[slot];
            Rectangle card={gridX+col*(cardW+gapX),gridY+row*(cardH+gapY),
                            cardW,cardH};
            if(mode==0)
                DrawCard(card,id,lockNames[id],lockTextures[id],lockRating[id],
                         artifactLocks[selectedArtifact][id],false);
            else
                DrawCard(card,id,keyNames[id],keyTextures[id],keyRating[id],
                         lockKeys[selectedLock][id],false);
        }
        EndScissorMode();

        DrawText(mode==0&&artifactListMaxScroll>0.0f?
                 "MOUSE WHEEL: scroll artifact list   |   Only ACCEPTED / MAYBE designs with archived PNGs are selectable.":
                 (mode==1&&lockListMaxScroll>0.0f?
                  "MOUSE WHEEL: scroll locked objects   |   Only ACCEPTED / MAYBE designs with archived PNGs are selectable.":
                  "Only current ACCEPTED and MAYBE / FIX ratings with archived PNGs are selectable."),
                 31,871,13,muted);
        DrawText("ESC: close  |  CTRL+S: save",1083,871,13,muted);
        EndDrawing();
    }

    for(int i=0;i<LOCK_COUNT;i++)if(lockTextures[i].id)UnloadTexture(lockTextures[i]);
    for(int i=0;i<KEY_COUNT;i++)if(keyTextures[i].id)UnloadTexture(keyTextures[i]);
    CloseWindow();
    return 0;
}
