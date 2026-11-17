#include "mobile_platform.h"
#include "mobile_input.h"
#include "android_safe_area.h"
#include "game_challenges.h"
#include "globals.h"
#include "game_ui.h"
#include "game_map.h"
#include "game_save.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#ifdef PLATFORM_WEB
#include "../Web/web_platform.h"
#endif

#define CH_W CHALLENGE_MAP_WIDTH
#define CH_H CHALLENGE_MAP_HEIGHT
#define CH_COUNT 16
#define CH_CELLS (CH_W*CH_H)
#define CC(r,g,b) ((Color){r,g,b,255})

typedef struct {
    const char *name, *tag;
    Color accent, wall, floor;
} ChallengeSpec;

static const ChallengeSpec specs[CH_COUNT]={
    {"ECHO LABYRINTH","LIVING MAZE",{103,225,230,255},{34,72,83,255},{18,37,49,255}},
    {"CLOCKWORK HEIST","FORGED CHAMBERS",{246,185,96,255},{91,67,50,255},{42,34,31,255}},
    {"EMBER CATHEDRAL","RINGED TEMPLE",{255,117,78,255},{102,46,52,255},{47,25,36,255}},
    {"PRISM GALLERY","SPLIT HALLS",{168,164,255,255},{73,63,114,255},{31,29,58,255}},
    {"FROST SPINE","SERPENTINE PASS",{142,221,255,255},{62,97,128,255},{29,46,65,255}},
    {"VOID BLOOM","GROWING CAVERN",{217,125,247,255},{75,40,92,255},{35,20,53,255}},
    {"OBSIDIAN CITADEL","FOUR BASTIONS",{219,105,154,255},{82,43,72,255},{36,22,38,255}},
    {"SUNKEN ARCHIVE","DROWNED STACKS",{85,205,182,255},{41,89,91,255},{19,47,49,255}},
    {"THUNDER FORGE","CONDUCTOR WEB",{247,219,106,255},{91,83,53,255},{45,43,29,255}},
    {"MIRROR CRYPT","REFLECTED HALLS",{199,167,246,255},{78,67,108,255},{34,30,56,255}},
    {"CRIMSON AQUEDUCT","CANAL ISLANDS",{247,109,123,255},{102,52,65,255},{48,27,39,255}},
    {"STARFALL OBSERVATORY","CELESTIAL NODES",{143,196,255,255},{56,77,116,255},{25,37,65,255}},
    {"IRON UNDERCITY","TUNNEL JUNCTION",{208,157,117,255},{87,67,62,255},{40,34,36,255}},
    {"WRAITH GARDEN","HEDGE COURTS",{157,227,143,255},{60,88,67,255},{28,46,37,255}},
    {"ABYSSAL ENGINE","REACTOR RINGS",{107,231,213,255},{42,91,97,255},{20,45,55,255}},
    {"SHATTERED CROWN","BROKEN SPIRES",{246,185,151,255},{103,70,66,255},{49,34,40,255}}
};

#define CH_TIME_RECORD_LIMIT 10
#define CH_TIME_VISIBLE 5
#define SEED_SOURCE_DAILY 1u
#define SEED_SOURCE_CUSTOM 2u

typedef struct {
    int timeMs;
    int coins;
    int treasures;
    int kills;
    int hasRunStats;
} ChallengeScore;

typedef struct {
    int attempts;
    int completions;
    int timeCount;
    ChallengeScore scores[CH_TIME_RECORD_LIMIT];
} ChallengeProgress;

typedef struct {
    uint32_t seed;
    unsigned sourceFlags;
    unsigned attempts;
    int bestRound;
    int bestTreasures;
    int bestCoins;
    int bestKills;
    int bestTimeMs;
} SeedProgress;

static char tiles[CH_H][CH_W];
static char preview[CH_H][CH_W];
static bool safeRoute[CH_H][CH_W];
static int startX,startY;
static uint32_t rng;
static int menuPick=0,seedMode=0,customFocus=0,menuTab=0;
static int challengeScroll=0,pendingRunKind=0,activeRunKind=0,activeMission=0;
static uint32_t pendingRunSeed=0;
static uint32_t activeRunSeed=0;
static unsigned pendingSeedSource=0,activeSeedSource=0;
static float normalRunElapsed=0;
static int hasChallengeKey=0;
static int expectedChallengeKeyId=KEY_JADEEYE;
static int activeRunCompleted=0;
static char customSeed[25]="";
static int customSeedEraseKey=0;
static float customSeedEraseTimer=0.0f;
static uint32_t previewSeed=0;
static int previewPick=-1;
static uint32_t randomPreviewSeed=0;
static int seededScroll=0;
static float seededScrollRemainder=0.0f;
static int recordsLoaded=0;
static ChallengeProgress challengeStats[CH_COUNT];
static SeedProgress *seedScores=NULL;
static size_t seedProgressCount=0,seedProgressCapacity=0;

int ChallengeEditingSeed(void){return customFocus;}

static int ChallengeScoreBetter(const ChallengeScore *a,const ChallengeScore *b){
    if(a->timeMs!=b->timeMs)return a->timeMs<b->timeMs;
    if(a->hasRunStats!=b->hasRunStats)return a->hasRunStats>b->hasRunStats;
    if(a->coins!=b->coins)return a->coins>b->coins;
    if(a->treasures!=b->treasures)return a->treasures>b->treasures;
    return a->kills>b->kills;
}

static int SeedScoreCompare(const void *left,const void *right){
    const SeedProgress *a=(const SeedProgress*)left;
    const SeedProgress *b=(const SeedProgress*)right;
    if(a->bestRound!=b->bestRound)return a->bestRound>b->bestRound?-1:1;
    if(a->bestCoins!=b->bestCoins)return a->bestCoins>b->bestCoins?-1:1;
    if(a->bestTreasures!=b->bestTreasures)return a->bestTreasures>b->bestTreasures?-1:1;
    if(a->bestKills!=b->bestKills)return a->bestKills>b->bestKills?-1:1;
    if(a->bestTimeMs!=b->bestTimeMs)return a->bestTimeMs<b->bestTimeMs?-1:1;
    return a->seed<b->seed?-1:a->seed>b->seed?1:0;
}

static SeedProgress *FindSeedScore(uint32_t seed){
    for(size_t i=0;i<seedProgressCount;i++)if(seedScores[i].seed==seed)return &seedScores[i];
    return NULL;
}

static SeedProgress *GetSeedScore(uint32_t seed){
    SeedProgress *record=FindSeedScore(seed);
    if(record)return record;
    if(seedProgressCount==seedProgressCapacity){
        size_t next=seedProgressCapacity?seedProgressCapacity*2:16;
        SeedProgress *grown=(SeedProgress*)realloc(seedScores,next*sizeof(*seedScores));
        if(!grown)return NULL;
        seedScores=grown;seedProgressCapacity=next;
    }
    record=&seedScores[seedProgressCount++];
    memset(record,0,sizeof(*record));
    record->seed=seed;
    return record;
}

static void SaveChallengeData(void){
    if(!recordsLoaded)return;
    FILE *file=fopen(CHALLENGE_DATA_FILE,"w");
    if(!file)return;
    fprintf(file,"DCHAL 2\n");
    for(int i=0;i<CH_COUNT;i++){
        const ChallengeProgress *p=&challengeStats[i];
        fprintf(file,"C %d %d %d %d",i,p->attempts,p->completions,p->timeCount);
        for(int j=0;j<p->timeCount;j++){
            const ChallengeScore *s=&p->scores[j];
            fprintf(file," %d %d %d %d %d",s->timeMs,s->coins,s->treasures,
                    s->kills,s->hasRunStats);
        }
        fputc('\n',file);
    }
    for(size_t i=0;i<seedProgressCount;i++){
        const SeedProgress *p=&seedScores[i];
        fprintf(file,"S %u %u %u %d %d %d %d %d\n",p->seed,p->sourceFlags,
                p->attempts,p->bestRound,p->bestTreasures,p->bestCoins,
                p->bestKills,p->bestTimeMs);
    }
    fclose(file);
#ifdef PLATFORM_WEB
    WebPersist();
#endif
}

void ChallengeLoadData(void){
    if(recordsLoaded)return;
    recordsLoaded=1;
    FILE *file=fopen(CHALLENGE_DATA_FILE,"r");
    if(!file)return;
    char header[16];int version=0;
    if(fscanf(file,"%15s %d",header,&version)!=2||strcmp(header,"DCHAL")!=0||
       (version!=1&&version!=2)){
        fclose(file);return;
    }
    char type;
    while(fscanf(file," %c",&type)==1){
        if(type=='C'){
            int index=0,attempts=0,completions=0,count=0;
            if(fscanf(file," %d %d %d %d",&index,&attempts,&completions,&count)!=4)break;
            if(index<0||index>=CH_COUNT||count<0||count>CH_TIME_RECORD_LIMIT){
                int ch;while((ch=fgetc(file))!='\n'&&ch!=EOF){};
                continue;
            }
            ChallengeProgress *p=&challengeStats[index];
            p->attempts=attempts>0?attempts:0;
            p->completions=completions>0?completions:0;
            p->timeCount=count;
            for(int i=0;i<count;i++){
                ChallengeScore *s=&p->scores[i];
                if(version==1){
                    if(fscanf(file," %d",&s->timeMs)!=1){p->timeCount=i;break;}
                    s->hasRunStats=0;
                }else if(fscanf(file," %d %d %d %d %d",&s->timeMs,&s->coins,
                                &s->treasures,&s->kills,&s->hasRunStats)!=5){
                    p->timeCount=i;break;
                }
            }
        }else if(type=='S'){
            unsigned seed=0,flags=0,attempts=0;
            SeedProgress p={0};
            if(fscanf(file," %u %u %u %d %d %d %d %d",&seed,&flags,&attempts,
                      &p.bestRound,&p.bestTreasures,&p.bestCoins,&p.bestKills,&p.bestTimeMs)!=8){
                int ch;while((ch=fgetc(file))!='\n'&&ch!=EOF){};
                continue;
            }
            SeedProgress *dst=GetSeedScore((uint32_t)seed);
            if(dst){
                p.seed=(uint32_t)seed;p.sourceFlags=flags&(SEED_SOURCE_DAILY|SEED_SOURCE_CUSTOM);
                p.attempts=attempts;*dst=p;
            }
        }else{
            int ch;while((ch=fgetc(file))!='\n'&&ch!=EOF){}
        }
    }
    fclose(file);
    if(seedProgressCount>1)
        qsort(seedScores,seedProgressCount,sizeof(*seedScores),SeedScoreCompare);
}

ChallengeStatsSummary ChallengeGetStatsSummary(void){
    ChallengeLoadData();
    ChallengeStatsSummary result={0};
    for(int i=0;i<CH_COUNT;i++){
        result.attempts+=challengeStats[i].attempts;
        result.completions+=challengeStats[i].completions;
        if(challengeStats[i].completions>0)result.distinctChallengesCompleted++;
    }
    for(size_t i=0;i<seedProgressCount;i++){
        const SeedProgress *p=&seedScores[i];
        result.seededRuns+=(int)p->attempts;
        if(p->attempts){
            result.distinctSeeds++;
            if(p->sourceFlags&SEED_SOURCE_DAILY)result.distinctDailySeeds++;
        }
    }
    return result;
}

static void SaveSeedAttempt(uint32_t seed,unsigned source){
    SeedProgress *record=GetSeedScore(seed);
    if(!record)return;
    record->sourceFlags|=source;
    record->attempts++;
    SaveChallengeData();
}

static void RecordSeedProgress(void){
    if(activeRunKind!=2)return;
    SeedProgress *record=GetSeedScore(activeRunSeed);
    if(!record)return;
    record->sourceFlags|=activeSeedSource;
    int elapsed=(int)lroundf(normalRunElapsed*1000.0f);
    int better=roundn>record->bestRound||
        (roundn==record->bestRound&&totcoi>record->bestCoins)||
        (roundn==record->bestRound&&totcoi==record->bestCoins&&tottrs>record->bestTreasures)||
        (roundn==record->bestRound&&totcoi==record->bestCoins&&tottrs==record->bestTreasures&&kills_game>record->bestKills)||
        (roundn==record->bestRound&&totcoi==record->bestCoins&&tottrs==record->bestTreasures&&kills_game==record->bestKills&&
         elapsed>0&&(record->bestTimeMs==0||elapsed<record->bestTimeMs));
    if(better){
        record->bestRound=roundn;
        record->bestCoins=totcoi;
        record->bestTreasures=tottrs;
        record->bestKills=kills_game;
        record->bestTimeMs=elapsed;
    }
    if(seedProgressCount>1)
        qsort(seedScores,seedProgressCount,sizeof(*seedScores),SeedScoreCompare);
    SaveChallengeData();
}

static void FormatRunTime(int milliseconds,char *out,size_t capacity){
    if(milliseconds<0)milliseconds=0;
    int seconds=milliseconds/1000;
    int minutes=seconds/60;
    seconds%=60;
    snprintf(out,capacity,"%02d:%02d",minutes,seconds);
}

static void EraseLastCustomSeedChar(void){
    size_t n=strlen(customSeed);
    if(n)customSeed[n-1]='\0';
}

static void UpdateCustomSeedErase(void){
    bool controlDown=IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL);
    if(controlDown&&IsKeyDown(KEY_DELETE)){
        customSeed[0]='\0';
        customSeedEraseKey=0;
        customSeedEraseTimer=0.0f;
        return;
    }

    int eraseKey=IsKeyDown(KEY_BACKSPACE)?KEY_BACKSPACE:
                 IsKeyDown(KEY_DELETE)?KEY_DELETE:0;
    if(!eraseKey){
        customSeedEraseKey=0;
        customSeedEraseTimer=0.0f;
        return;
    }
    if(IsKeyPressed(eraseKey)||eraseKey!=customSeedEraseKey){
        EraseLastCustomSeedChar();
        customSeedEraseKey=eraseKey;
        customSeedEraseTimer=0.34f;
        return;
    }

    customSeedEraseTimer-=GetFrameTime();
    if(customSeedEraseTimer<=0.0f){
        EraseLastCustomSeedChar();
        customSeedEraseTimer=0.055f;
    }
}

static uint32_t Mix(uint32_t x){
    x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;
    return x?x:0x6d2b79f5u;
}
static uint32_t Next(void){
    rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
    return rng;
}
static int Rand(int lo,int hi){return lo+(int)(Next()%(uint32_t)(hi-lo+1));}
static int InBounds(int x,int y){return x>0&&y>0&&x<CH_W-1&&y<CH_H-1;}
static void Dig(int x,int y){if(InBounds(x,y))tiles[y][x]='.';}
static void DigRect(int x1,int y1,int x2,int y2){
    for(int y=y1;y<=y2;y++)for(int x=x1;x<=x2;x++)Dig(x,y);
}
static void DigPath(int x1,int y1,int x2,int y2){
    if(Rand(0,1)){
        int d=x2>=x1?1:-1;
        for(int x=x1;x!=x2+d;x+=d)Dig(x,y1);
        d=y2>=y1?1:-1;
        for(int y=y1;y!=y2+d;y+=d)Dig(x2,y);
    }else{
        int d=y2>=y1?1:-1;
        for(int y=y1;y!=y2+d;y+=d)Dig(x1,y);
        d=x2>=x1?1:-1;
        for(int x=x1;x!=x2+d;x+=d)Dig(x,y2);
    }
}
static void Blank(void){
    for(int y=0;y<CH_H;y++)for(int x=0;x<CH_W;x++)tiles[y][x]='#';
}

static void MazeTree(void){
    int stackX[CH_CELLS],stackY[CH_CELLS],top=0;
    Dig(1,1);stackX[top]=1;stackY[top++]=1;
    while(top){
        int x=stackX[top-1],y=stackY[top-1];
        const int dx[4]={0,2,0,-2},dy[4]={-2,0,2,0};
        int choices[4],n=0;
        for(int d=0;d<4;d++)if(InBounds(x+dx[d],y+dy[d])&&
            tiles[y+dy[d]][x+dx[d]]=='#')choices[n++]=d;
        if(!n){top--;continue;}
        int d=choices[Rand(0,n-1)];
        Dig(x+dx[d]/2,y+dy[d]/2);Dig(x+dx[d],y+dy[d]);
        stackX[top]=x+dx[d];stackY[top++]=y+dy[d];
    }
    for(int i=0;i<22;i++){
        int x=Rand(3,CH_W-4),y=Rand(3,CH_H-4);
        if(tiles[y][x]=='#'&&((tiles[y][x-1]=='.'&&tiles[y][x+1]=='.')||
            (tiles[y-1][x]=='.'&&tiles[y+1][x]=='.')))Dig(x,y);
    }
    startX=startY=1;
}
static void ClockworkRooms(void){
    int cx[3][3],cy[3][3];
    for(int ry=0;ry<3;ry++)for(int rx=0;rx<3;rx++){
        int x=2+rx*13+Rand(0,2),y=2+ry*10+Rand(0,2);
        int w=Rand(7,9),h=Rand(5,7);
        DigRect(x,y,x+w,y+h);cx[ry][rx]=x+w/2;cy[ry][rx]=y+h/2;
    }
    for(int ry=0;ry<3;ry++)for(int rx=0;rx<3;rx++){
        if(rx<2)DigPath(cx[ry][rx],cy[ry][rx],cx[ry][rx+1],cy[ry][rx+1]);
        if(ry<2)DigPath(cx[ry][rx],cy[ry][rx],cx[ry+1][rx],cy[ry+1][rx]);
    }
    startX=cx[0][0];startY=cy[0][0];
}
static void EmberRings(void){
    const int mx=CH_W/2,my=CH_H/2;
    DigRect(mx-2,my-2,mx+2,my+2);
    for(int r=5;r<=14;r+=3){
        for(int x=mx-r;x<=mx+r;x++){Dig(x,my-r);Dig(x,my+r);}
        for(int y=my-r;y<=my+r;y++){Dig(mx-r,y);Dig(mx+r,y);}
        int a=Rand(-r+2,r-2),b=Rand(-r+2,r-2);
        DigRect(mx-1,my-r,mx+1,my-r+1);
        DigRect(mx-1,my+r-1,mx+1,my+r);
        DigRect(mx-r,my-1,mx-r+1,my+1);
        DigRect(mx+r-1,my-1,mx+r,my+1);
        DigRect(mx+a-1,my-r-1,mx+a+1,my-r+1);
        DigRect(mx-r-1,my+b-1,mx-r+1,my+b+1);
    }
    DigRect(mx-1,1,mx+1,CH_H-2);
    DigRect(1,my-1,CH_W-2,my+1);
    for(int i=0;i<18;i++){
        int x=Rand(4,CH_W-5),y=Rand(4,CH_H-5);
        if(tiles[y][x]=='.')DigRect(x-1,y-1,x+1,y+1);
    }
    startX=mx;startY=my;
}
static void DivideHall(int x1,int y1,int x2,int y2,int depth){
    int w=x2-x1+1,h=y2-y1+1;
    if(depth>=7||w<7||h<7)return;
    int vertical=(w>h+3)?1:(h>w+3)?0:Rand(0,1);
    if(vertical){
        int x=Rand(x1+2,x2-2),gap=Rand(y1,y2);
        for(int y=y1;y<=y2;y++)if(y!=gap)tiles[y][x]='#';
        DivideHall(x1,y1,x-1,y2,depth+1);
        DivideHall(x+1,y1,x2,y2,depth+1);
    }else{
        int y=Rand(y1+2,y2-2),gap=Rand(x1,x2);
        for(int x=x1;x<=x2;x++)if(x!=gap)tiles[y][x]='#';
        DivideHall(x1,y1,x2,y-1,depth+1);
        DivideHall(x1,y+1,x2,y2,depth+1);
    }
}
static void PrismDivision(void){
    DigRect(1,1,CH_W-2,CH_H-2);
    DivideHall(1,1,CH_W-2,CH_H-2,0);
    startX=startY=1;
}
static void FrostSpine(void){
    for(int lane=0;lane<7;lane++){
        int y=3+lane*4;
        DigRect(3,y,CH_W-4,y+1);
        if(lane<6){
            int cx=(lane&1)?4:CH_W-5;
            DigRect(cx-1,y,cx+1,y+5);
        }
        for(int b=0;b<3;b++){
            int x=Rand(7,CH_W-8);
            if(Rand(0,1))DigRect(x,y-2,x+2,y);
            else DigRect(x,y+1,x+2,y+3);
        }
    }
    startX=3;startY=3;
}
static void VoidWalk(void){
    int x=CH_W/2,y=CH_H/2,dug=0;
    for(int steps=0;steps<12000&&dug<650;steps++){
        for(int yy=y-1;yy<=y+1;yy++)for(int xx=x-1;xx<=x+1;xx++){
            if(InBounds(xx,yy)&&tiles[yy][xx]=='#'){Dig(xx,yy);dug++;}
        }
        int d=Rand(0,3);
        x+=d==0?1:d==1?-1:0;
        y+=d==2?1:d==3?-1:0;
        if(x<2)x=2;
        if(x>CH_W-3)x=CH_W-3;
        if(y<2)y=2;
        if(y>CH_H-3)y=CH_H-3;
    }
    startX=CH_W/2;startY=CH_H/2;
}
static void DigCircle(int cx,int cy,int radius){
    for(int y=cy-radius;y<=cy+radius;y++)for(int x=cx-radius;x<=cx+radius;x++)
        if((x-cx)*(x-cx)+(y-cy)*(y-cy)<=radius*radius)Dig(x,y);
}
static void ObsidianCitadel(void){
    DigRect(16,11,24,19);
    const int bx[4]={3,30,3,30},by[4]={3,3,22,22};
    for(int i=0;i<4;i++){
        int x=bx[i]+Rand(0,2),y=by[i]+Rand(0,2);
        DigRect(x,y,x+7,y+5);DigPath(x+3,y+2,20,15);
    }
    DigRect(19,2,21,28);DigRect(3,14,37,16);
    startX=20;startY=15;
}
static void SunkenArchive(void){
    DigRect(2,2,CH_W-3,CH_H-3);
    for(int x=6;x<=34;x+=5)for(int y=4;y<=26;y++)
        if(y!=6&&y!=15&&y!=24&&y!=6+(x*3)%5)tiles[y][x]='#';
    for(int y=6;y<=24;y+=9)DigRect(2,y,CH_W-3,y+1);
    for(int i=0;i<6;i++){
        int x=4+i*6;DigRect(x,3,x+2,5);DigRect(x,25,x+2,27);
    }
    startX=3;startY=3;
}
static void ThunderForge(void){
    DigCircle(20,15,5);
    const int cx[8]={20,30,35,30,20,10,5,10};
    const int cy[8]={4,6,15,24,26,24,15,6};
    for(int i=0;i<8;i++){
        DigCircle(cx[i],cy[i],Rand(2,3));DigPath(20,15,cx[i],cy[i]);
        if(i>0)DigPath(cx[i-1],cy[i-1],cx[i],cy[i]);
    }
    DigPath(cx[7],cy[7],cx[0],cy[0]);
    startX=20;startY=15;
}
static void MirrorCrypt(void){
    MazeTree();
    for(int y=1;y<CH_H-1;y++)for(int x=1;x<CH_W/2;x++)
        tiles[y][CH_W-1-x]=tiles[y][x];
    DigRect(19,2,21,28);
    for(int y=5;y<CH_H-3;y+=7)DigRect(17,y,23,y+1);
    startX=20;startY=15;
}
static void CrimsonAqueduct(void){
    DigRect(2,2,CH_W-3,CH_H-3);
    for(int y=6;y<=24;y+=6){
        for(int yy=y;yy<=y+1;yy++)for(int x=2;x<CH_W-2;x++)tiles[yy][x]='#';
        for(int bridge=0;bridge<4;bridge++){
            int x=4+bridge*10+Rand(0,3);DigRect(x,y-1,x+1,y+2);
        }
    }
    DigRect(19,2,21,28);
    startX=3;startY=3;
}
static void StarfallObservatory(void){
    const int cx[9]={20,7,14,29,35,7,15,28,35};
    const int cy[9]={15,5,4,4,7,24,27,26,23};
    for(int i=0;i<9;i++){
        DigCircle(cx[i],cy[i],i?Rand(2,3):5);
        if(i)DigPath(20,15,cx[i],cy[i]);
    }
    for(int i=1;i<5;i++)DigPath(cx[i],cy[i],cx[i+4],cy[i+4]);
    startX=20;startY=15;
}
static void IronUndercity(void){
    for(int y=5;y<=25;y+=10)DigRect(2,y,38,y+2);
    for(int x=8;x<=32;x+=12)DigRect(x,2,x+2,28);
    for(int y=2;y<=22;y+=10)for(int x=3;x<=31;x+=14){
        int w=Rand(4,6),h=Rand(3,5);DigRect(x,y,x+w,y+h);
    }
    startX=9;startY=6;
}
static void WraithGarden(void){
    DigRect(2,2,38,28);
    for(int x=8;x<=32;x+=8)for(int y=3;y<=27;y++)
        if(y!=5+(x%3)&&y!=15&&y!=24)tiles[y][x]='#';
    for(int y=8;y<=24;y+=8)for(int x=3;x<=37;x++)
        if(x!=5+(y%5)&&x!=20&&x!=34)tiles[y][x]='#';
    DigRect(18,13,22,17);
    startX=20;startY=15;
}
static void AbyssalEngine(void){
    DigCircle(20,15,4);
    for(int r=7;r<=13;r+=3)for(int y=2;y<=28;y++)for(int x=2;x<=38;x++){
        int dx=x-20,dy=y-15,d=dx*dx+dy*dy;
        if(d>=r*r-2*r&&d<=r*r+2*r)Dig(x,y);
    }
    DigRect(19,2,21,28);DigRect(3,14,37,16);
    for(int i=0;i<12;i++)DigCircle(Rand(5,35),Rand(4,26),1);
    startX=20;startY=15;
}
static void ShatteredCrown(void){
    int cx[12],cy[12],n=0;
    for(int row=0;row<3;row++)for(int col=0;col<4;col++){
        int x=5+col*10+Rand(-1,1),y=5+row*10+Rand(-1,1);
        cx[n]=x;cy[n]=y;n++;DigRect(x-2,y-2,x+2,y+2);
    }
    for(int i=1;i<n;i++)DigPath(cx[i-1],cy[i-1],cx[i],cy[i]);
    for(int i=0;i<4;i++)DigPath(cx[i],cy[i],cx[i+8],cy[i+8]);
    startX=cx[0];startY=cy[0];
}

static int distMap[CH_H][CH_W];
static void Distances(void){
    int qx[CH_CELLS],qy[CH_CELLS],head=0,tail=0;
    for(int y=0;y<CH_H;y++)for(int x=0;x<CH_W;x++)distMap[y][x]=-1;
    distMap[startY][startX]=0;
    qx[tail]=startX;qy[tail++]=startY;
    const int dx[4]={0,1,0,-1},dy[4]={-1,0,1,0};
    while(head<tail){
        int x=qx[head],y=qy[head++];
        for(int d=0;d<4;d++){
            int xx=x+dx[d],yy=y+dy[d];
            if(InBounds(xx,yy)&&tiles[yy][xx]!='#'&&distMap[yy][xx]<0){
                distMap[yy][xx]=distMap[y][x]+1;
                qx[tail]=xx;qy[tail++]=yy;
            }
        }
    }
}
static void BuildTopology(int style,unsigned seed,int round){
    rng=Mix(seed^((uint32_t)(style+1)*0x9e3779b9u)^
            ((uint32_t)round*0x85ebca6bu));
    Blank();
    switch(style){
        case 0:MazeTree();break;
        case 1:ClockworkRooms();break;
        case 2:EmberRings();break;
        case 3:PrismDivision();break;
        case 4:FrostSpine();break;
        case 5:VoidWalk();break;
        case 6:ObsidianCitadel();break;
        case 7:SunkenArchive();break;
        case 8:ThunderForge();break;
        case 9:MirrorCrypt();break;
        case 10:CrimsonAqueduct();break;
        case 11:StarfallObservatory();break;
        case 12:IronUndercity();break;
        case 13:WraithGarden();break;
        case 14:AbyssalEngine();break;
        default:ShatteredCrown();break;
    }
    for(int tries=0;tries<CH_CELLS;tries++){
        Distances();
        int xMissing=-1,yMissing=-1;
        for(int y=1;y<CH_H-1&&xMissing<0;y++)for(int x=1;x<CH_W-1;x++)
            if(tiles[y][x]=='.'&&distMap[y][x]<0){xMissing=x;yMissing=y;break;}
        if(xMissing<0)break;
        DigPath(startX,startY,xMissing,yMissing);
    }
}
static void MarkSafePath(int sx,int sy,int tx,int ty,int *gateX,int *gateY){
    short prevX[CH_H][CH_W],prevY[CH_H][CH_W];
    int qx[CH_CELLS],qy[CH_CELLS],head=0,tail=0;
    for(int y=0;y<CH_H;y++)for(int x=0;x<CH_W;x++)prevX[y][x]=-1;
    prevX[sy][sx]=(short)sx;prevY[sy][sx]=(short)sy;
    qx[tail]=sx;qy[tail++]=sy;
    const int dx[4]={0,1,0,-1},dy[4]={-1,0,1,0};
    while(head<tail&&prevX[ty][tx]<0){
        int x=qx[head],y=qy[head++];
        for(int d=0;d<4;d++){
            int xx=x+dx[d],yy=y+dy[d];
            if(InBounds(xx,yy)&&tiles[yy][xx]!='#'&&prevX[yy][xx]<0){
                prevX[yy][xx]=(short)x;prevY[yy][xx]=(short)y;
                qx[tail]=xx;qy[tail++]=yy;
            }
        }
    }
    if(prevX[ty][tx]<0)return;
    if(gateX&&gateY){*gateX=prevX[ty][tx];*gateY=prevY[ty][tx];}
    int x=tx,y=ty;
    for(int guard=0;guard<CH_CELLS;guard++){
        safeRoute[y][x]=true;
        if(x==sx&&y==sy)break;
        int oldX=x;
        x=prevX[y][x];y=prevY[y][oldX];
    }
}
int ChallengeChestDesignForStage(int stage) {
    return stage<=1?0:stage==2?8:11;
}

int ChallengeKeyForStage(int stage) {
    return stage<=1?KEY_JADEEYE:stage==2?KEY_BONE_RELIQUARY:KEY_DRAGONFANG;
}

void ChallengeGenerateNormalFloor(char out[MAP_ROWS][MAP_COLS],int *spawnX,int *spawnY,
                                  unsigned seed,int floor){
    BuildTopology(activeMission,seed,floor);
    memset(safeRoute,0,sizeof(safeRoute));
    Distances();
    int tx=startX,ty=startY,maxDist=-1;
    for(int y=1;y<CH_H-1;y++)for(int x=1;x<CH_W-1;x++)
        if(tiles[y][x]=='.'&&distMap[y][x]>maxDist){
            maxDist=distMap[y][x];tx=x;ty=y;
        }
    int kx=startX,ky=startY,best=-1;
    for(int y=1;y<CH_H-1;y++)for(int x=1;x<CH_W-1;x++){
        int distance=distMap[y][x];
        int fromTreasure=abs(tx-x)+abs(ty-y);
        if(tiles[y][x]!='.'||distance<maxDist/3||distance>maxDist*3/4||
           fromTreasure<6)continue;
        int score=distance+fromTreasure/2;
        if(score>best){best=score;kx=x;ky=y;}
    }
    if(kx==startX&&ky==startY){
        for(int y=1;y<CH_H-1;y++)for(int x=1;x<CH_W-1;x++)
            if(tiles[y][x]=='.'&&distMap[y][x]>best&&
               (x!=tx||y!=ty)){best=distMap[y][x];kx=x;ky=y;}
    }
    MarkSafePath(startX,startY,kx,ky,NULL,NULL);
    MarkSafePath(kx,ky,tx,ty,NULL,NULL);
    int stage=floor<1?1:floor>3?3:floor;
    int keyId=ChallengeKeyForStage(stage);
    int designId=ChallengeChestDesignForStage(stage);
    tiles[ky][kx]=UniversalKeyTileFromId(keyId);
    tiles[ty][tx]='T';
    for(int y=0;y<CH_H;y++)for(int x=0;x<CH_W;x++)out[y][x]=tiles[y][x];
    ResetUniversalKeys();
    ResetLockedObjects();
    expectedChallengeKeyId=keyId;
    RegisterLockedObjectAt(tx,ty,designId,keyId,'T');
    *spawnX=startX;*spawnY=startY;
    hasChallengeKey=0;
}
int ChallengeRouteSafe(int x,int y){
    return x>=0&&y>=0&&x<CH_W&&y<CH_H&&safeRoute[y][x];
}
int ChallengeHasKey(void){return hasChallengeKey;}
void ChallengeCollectKey(int keyId){hasChallengeKey=keyId==expectedChallengeKeyId;}
void ChallengeConsumeKey(int keyId){if(keyId==expectedChallengeKeyId)hasChallengeKey=0;}
float ChallengeElapsed(void){return normalRunElapsed;}
static uint32_t DailySeed(int *day,int *month,int *year){
    time_t now=time(NULL);
    struct tm *local=localtime(&now);
    int d=local?local->tm_mday:1,m=local?local->tm_mon+1:1,y=local?local->tm_year+1900:2026;
    if(day)*day=d;
    if(month)*month=m;
    if(year)*year=y;
    return Mix((uint32_t)y*0x9e3779b9u^(uint32_t)m*0x85ebca6bu^
               (uint32_t)d*0xc2b2ae35u);
}
static uint32_t CustomSeed(const char *s){
    if(!s[0])return 0;
    char *end=NULL;
    int base=(s[0]=='0'&&(s[1]=='x'||s[1]=='X'))?16:10;
    unsigned long long number=strtoull(s,&end,base);
    if(s[0]!='-'&&end!=s&&*end=='\0'&&number<=UINT32_MAX)
        return (uint32_t)number;
    uint32_t hash=2166136261u;
    for(const unsigned char *p=(const unsigned char*)s;*p;p++){
        hash^=*p;hash*=16777619u;
    }
    return hash;
}
static uint32_t FreshRandomSeed(void){
    uint32_t a=(uint32_t)GetRandomValue(0,0x7fffffff);
    uint32_t b=(uint32_t)GetRandomValue(0,0x7fffffff);
    return Mix(a^(b<<1)^(uint32_t)(unsigned long long)(GetTime()*1000000.0));
}
static uint32_t ActiveSeed(void){
    if(menuTab==0){
        if(!randomPreviewSeed)randomPreviewSeed=FreshRandomSeed();
        return randomPreviewSeed;
    }
    return seedMode?CustomSeed(customSeed):DailySeed(NULL,NULL,NULL);
}
int ChallengeTakeRunStart(unsigned *seed){
    int kind=pendingRunKind;
    if(!kind)return 0;
    pendingRunKind=0;
    activeRunKind=kind;activeMission=menuPick;normalRunElapsed=0;hasChallengeKey=0;
    activeRunSeed=pendingRunSeed;activeSeedSource=pendingSeedSource;activeRunCompleted=0;
    if(seed)*seed=pendingRunSeed;
    if(kind==1){
        challengeStats[activeMission].attempts++;
        SaveChallengeData();
    }else if(kind==2){
        SaveSeedAttempt(activeRunSeed,activeSeedSource);
        if(activeSeedSource&SEED_SOURCE_DAILY)StatsDailyStarted();
    }
    return kind;
}
void ChallengeClearRunMode(void){
    if(activeRunKind==2)RecordSeedProgress();
    activeRunKind=0;pendingRunKind=0;activeRunCompleted=0;
}
int ChallengeRunActive(void){return activeRunKind==1;}
int ChallengeRunMode(void){return activeRunKind;}
void ChallengeTickRun(float dt){if(activeRunKind!=0&&dt>0.0f)normalRunElapsed+=dt;}
void ChallengeRestartRun(void){
    if(activeRunKind==1&&activeMission>=0&&activeMission<CH_COUNT){
        challengeStats[activeMission].attempts++;
        SaveChallengeData();
    }else if(activeRunKind==2){
        RecordSeedProgress();
        SaveSeedAttempt(activeRunSeed,activeSeedSource);
    }
    normalRunElapsed=0;hasChallengeKey=0;activeRunCompleted=0;
}
void ChallengeRecordCompletion(void){
    if(activeRunKind!=1||activeRunCompleted||activeMission<0||activeMission>=CH_COUNT)return;
    ChallengeProgress *p=&challengeStats[activeMission];
    int elapsed=(int)lroundf(normalRunElapsed*1000.0f);
    if(elapsed<1)elapsed=1;
    p->completions++;
    ChallengeScore newScore={elapsed,totcoi,tottrs,kills_game,1};
    if(p->timeCount<CH_TIME_RECORD_LIMIT){
        p->scores[p->timeCount++]=newScore;
    }else if(ChallengeScoreBetter(&newScore,&p->scores[p->timeCount-1])){
        p->scores[p->timeCount-1]=newScore;
    }
    for(int i=p->timeCount-1;i>0&&ChallengeScoreBetter(&p->scores[i],&p->scores[i-1]);i--){
        ChallengeScore swap=p->scores[i];p->scores[i]=p->scores[i-1];p->scores[i-1]=swap;
    }
    activeRunCompleted=1;
    SaveChallengeData();
    StatsChallengeCompleted(p->attempts==1,elapsed);
}
void ChallengeSaveSeedRunProgress(void){RecordSeedProgress();}
void ChallengeDrawRunOverlay(void){
    if(activeRunKind==0)return;
    int safeRight=0;AndroidGetSafeInsets(NULL,NULL,&safeRight,NULL);
    Color accent=activeRunKind==1?specs[activeMission].accent:CC(109,216,221);
    const char *title=activeRunKind==1?specs[activeMission].name:"SEEDED RUN";
    int fs=20;float width=fmaxf(340,MeasureText(title,fs)+32);
    Rectangle badge={GetScreenWidth()-safeRight-18-width,100,width,66};
    DrawRectangleRounded(badge,.18f,6,CC(15,12,28));
    DrawRectangleRoundedLinesEx(badge,.18f,6,1,accent);
    DrawText(title,(int)badge.x+16,(int)badge.y+10,fs,accent);
    const char *detail=activeRunKind==1?
        TextFormat("LEVEL %d/%d     %02d:%02d",roundn,CHALLENGE_FLOORS,(int)normalRunElapsed/60,(int)normalRunElapsed%60):
        TextFormat("SEED %u",run_seed);
    DrawText(detail,(int)badge.x+16,(int)badge.y+37,20,CC(239,230,227));
}
static void StartChallenge(void){
    uint32_t seed=ActiveSeed();
    if(menuTab==1&&seedMode&&!customSeed[0])return;
    pendingRunKind=menuTab==0?1:2;
    pendingRunSeed=seed;
    pendingSeedSource=menuTab==1?(seedMode?SEED_SOURCE_CUSTOM:SEED_SOURCE_DAILY):0;
    if(menuTab==0){randomPreviewSeed=0;previewPick=-1;}
    menuSub=0;customFocus=0;MenuNavExit();
    MenuPlayBegin();
}

static void DrawMenuMap(int x,int y,int size){
    for(int yy=0;yy<CH_H;yy++)for(int xx=0;xx<CH_W;xx++){
        char c=preview[yy][xx];
        Color color=(c=='#')?specs[menuPick].wall:specs[menuPick].floor;
        DrawRectangle(x+xx*size,y+yy*size,size,size,color);
    }
    DrawRectangle(x+startX*size,y+startY*size,size,size,CC(245,245,230));
}
static int Button(Rectangle r,const char *label,Color accent,int active){
    bool hot=CheckCollisionPointRec(GetMousePosition(),r);
    DrawRectangleRounded(r,0.18f,6,active?
        (hot?CC(59,53,77):CC(42,37,60)):CC(25,23,36));
    DrawRectangleRoundedLinesEx(r,0.18f,6,active?2:1,
        active?accent:CC(66,60,78));
    int fs=22;
    DrawText(label,(int)(r.x+r.width/2-MeasureText(label,fs)/2),
             (int)r.y+((int)r.height-fs)/2,fs,active?CC(243,238,228):CC(105,99,116));
    return active&&MobileTapReleased(r);
}
#include "mobile_challenge_menu.inc.c"

void ChallengeHandleMenuConfirm(void){
    if(customFocus)return;
    if(menuTab==1&&seedMode&&!customSeed[0])return;
    StartChallenge();
}
