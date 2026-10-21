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
    int sw=GetScreenWidth(),sh=GetScreenHeight();
    Rectangle badge={(float)(sw/2-166),(float)(sh-36),332,29};
    DrawRectangleRounded(badge,0.28f,6,CC(15,12,28));
    DrawRectangleRoundedLinesEx(badge,0.28f,6,1,
        activeRunKind==1?specs[activeMission].accent:CC(109,216,221));
    const char *label=activeRunKind==1?
        TextFormat("%s   LEVEL %d/%d   %02d:%02d",specs[activeMission].name,
                   roundn,CHALLENGE_FLOORS,
                   (int)normalRunElapsed/60,(int)normalRunElapsed%60):
        TextFormat("SEEDED RUN   SEED %u",run_seed);
    int fs=13;
    DrawText(label,sw/2-MeasureText(label,fs)/2,sh-28,fs,CC(239,230,227));
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
    int fs=16;
    DrawText(label,(int)(r.x+r.width/2-MeasureText(label,fs)/2),
             (int)r.y+((int)r.height-fs)/2,fs,active?CC(243,238,228):CC(105,99,116));
    return active&&hot&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
static void DrawCardMotif(int kind,int cx,int cy,int cardW,int cardH,Color accent){
    int midX=cx+cardW/2,midY=cy+cardH/2+8;
    int radius=(cardH-88)/2;
    if(radius>70)radius=70;
    if(radius<12)radius=12;
    Color dim=(Color){accent.r,accent.g,accent.b,58};
    Color soft=(Color){accent.r,accent.g,accent.b,128};
    DrawCircle(midX,midY,radius+12,(Color){accent.r,accent.g,accent.b,12});
    if(kind>=6){
        int points=3+(kind%6);
        DrawPolyLinesEx((Vector2){midX,midY},points,(float)radius,kind*19,
                        2.0f,soft);
        DrawPoly((Vector2){midX,midY},points,radius*0.58f,kind*19,dim);
        for(int i=0;i<points;i++){
            float angle=((float)i/points)*6.2831853f+kind*0.332f;
            int px=midX+(int)(cosf(angle)*radius*0.72f);
            int py=midY+(int)(sinf(angle)*radius*0.72f);
            DrawLineEx((Vector2){midX,midY},(Vector2){px,py},2,soft);
            DrawCircle(px,py,3,accent);
        }
        DrawCircle(midX,midY,7,accent);
        DrawCircle(midX,midY,3,CC(19,16,32));
        return;
    }
    if(kind==0){
        for(int row=-2;row<=2;row++){
            int yy=midY+row*radius/3;
            DrawLineEx((Vector2){midX-radius,yy},
                       (Vector2){midX+radius/2,yy},2,row&1?soft:dim);
        }
        for(int col=-2;col<=2;col++){
            int xx=midX+col*radius/3;
            DrawLineEx((Vector2){xx,midY-radius},
                       (Vector2){xx,midY+radius/2},2,col&1?dim:soft);
        }
        DrawCircle(midX-radius/2,midY+radius/2,5,accent);
        DrawCircle(midX+radius/2,midY-radius/2,5,accent);
    }else if(kind==1){
        DrawRing((Vector2){midX,midY},radius*0.55f,radius*0.72f,0,360,40,soft);
        DrawCircleLines(midX,midY,radius,dim);
        for(int i=0;i<12;i++){
            float a=(float)i*6.2831853f/12.0f;
            Vector2 p={(float)midX+cosf(a)*radius*0.78f,
                       (float)midY+sinf(a)*radius*0.78f};
            DrawPoly(p,4,5,45,soft);
        }
        DrawPoly((Vector2){midX,midY},6,radius*0.42f,30,accent);
        DrawCircle(midX,midY,radius/5,CC(25,23,34));
    }else if(kind==2){
        DrawRing((Vector2){midX,midY},radius*0.74f,radius*0.83f,0,360,48,dim);
        DrawRing((Vector2){midX,midY},radius*0.43f,radius*0.51f,0,360,48,soft);
        DrawPoly((Vector2){midX,midY},4,radius*0.34f,45,accent);
        for(int i=0;i<4;i++){
            float a=(float)i*1.5707963f;
            DrawCircle(midX+(int)(cosf(a)*radius*0.8f),
                       midY+(int)(sinf(a)*radius*0.8f),5,accent);
        }
    }else if(kind==3){
        DrawPoly((Vector2){midX,midY},4,radius,45,dim);
        DrawPoly((Vector2){midX-radius/3,midY-radius/6},3,radius*0.44f,30,soft);
        DrawPoly((Vector2){midX+radius/3,midY+radius/6},3,radius*0.44f,90,accent);
        DrawLineEx((Vector2){midX-radius,midY+radius/2},
                   (Vector2){midX+radius,midY-radius/2},2,soft);
    }else if(kind==4){
        for(int i=0;i<5;i++){
            int yy=midY-radius+i*radius/2;
            int side=(i&1)?midX-radius:midX+radius;
            DrawLineEx((Vector2){midX-(i&1?radius:0),yy},
                       (Vector2){side,yy},3,i&1?soft:dim);
            if(i<4)DrawLineEx((Vector2){side,yy},
                              (Vector2){side,yy+radius/2},3,soft);
        }
        DrawPoly((Vector2){midX,midY},6,radius/3,30,accent);
    }else{
        for(int i=0;i<7;i++){
            float a=(float)i*6.2831853f/7.0f;
            Vector2 tip={(float)midX+cosf(a)*radius,
                         (float)midY+sinf(a)*radius};
            DrawLineEx((Vector2){midX,midY},tip,4,soft);
            DrawCircleV(tip,5,accent);
        }
        DrawRing((Vector2){midX,midY},radius*0.19f,radius*0.44f,0,360,32,accent);
        DrawCircle(midX,midY,radius/6,CC(18,10,29));
    }
}
void ChallengeDrawMenu(Rectangle panel,float t){
    (void)t;
    ChallengeLoadData();
    int x=(int)panel.x,y=(int)panel.y,w=(int)panel.width,h=(int)panel.height;
    if(w<620||h<530){
        DrawText("Enlarge window for Challenges.",x+20,y+96,18,CC(230,210,190));
        return;
    }
    if(!customFocus&&IsKeyPressed(KEY_TAB)){
        menuTab=1-menuTab;previewPick=-1;seededScroll=0;seededScrollRemainder=0.0f;MenuNavExit();
    }
    int lx=x+20,lw=w*55/100,rx=lx+lw+14,rw=x+w-20-rx;
    int top=y+111,bottom=y+h-66;
    Rectangle t1={(float)lx,(float)(y+64),(float)((w-54)/2),35};
    Rectangle t2={(float)(lx+(w-54)/2+10),(float)(y+64),(float)((w-54)/2),35};
    if(Button(t1,menuTab?"CHALLENGES":"CHALLENGES  *",CC(230,126,190),1)){
        menuTab=0;customFocus=0;previewPick=-1;seededScroll=0;seededScrollRemainder=0.0f;MenuNavExit();
    }
    if(Button(t2,menuTab?"SEEDED RUNS  *":"SEEDED RUNS",CC(109,216,221),1)){
        menuTab=1;customFocus=0;previewPick=-1;seededScroll=0;seededScrollRemainder=0.0f;MenuNavExit();
    }
    if(menuTab==0){
        uint32_t seed=ActiveSeed();
        if(previewPick!=menuPick||previewSeed!=seed){
            BuildTopology(menuPick,seed,1);
            memcpy(preview,tiles,sizeof(preview));
            previewPick=menuPick;previewSeed=seed;
        }
        int vy=top+24,vh=bottom-vy,gap=9,ch=140,cw=(lw-25)/2;
        int total=((CH_COUNT+1)/2)*(ch+gap)-gap,max=total-vh;
        if(max<0)max=0;
        Rectangle viewport={(float)lx,(float)vy,(float)(lw-10),(float)vh};
        if(CheckCollisionPointRec(GetMousePosition(),viewport))
            challengeScroll-=(int)(GetMouseWheelMove()*72);
        if(IsKeyPressed(KEY_PAGE_DOWN))challengeScroll+=vh-60;
        if(IsKeyPressed(KEY_PAGE_UP))challengeScroll-=vh-60;
        if(IsKeyPressed(KEY_DOWN))challengeScroll+=ch+gap;
        if(IsKeyPressed(KEY_UP))challengeScroll-=ch+gap;
        if(challengeScroll<0)challengeScroll=0;
        if(challengeScroll>max)challengeScroll=max;
        BeginScissorMode(lx,vy,lw-10,vh);
        for(int i=0;i<CH_COUNT;i++){
            int cx=lx+(i%2)*(cw+gap),cy=vy+(i/2)*(ch+gap)-challengeScroll;
            if(cy+ch<vy||cy>bottom)continue;
            Rectangle card={(float)cx,(float)cy,(float)cw,(float)ch};
            bool hot=CheckCollisionPointRec(GetMousePosition(),card)&&
                     CheckCollisionPointRec(GetMousePosition(),viewport);
            Color a=specs[i].accent;
            DrawRectangleRounded(card,0.1f,6,menuPick==i?CC(39,34,54):
                                 hot?CC(32,29,45):CC(23,21,34));
            DrawRectangleRoundedLinesEx(card,0.1f,6,menuPick==i?2:1,
                                        menuPick==i?a:(Color){a.r,a.g,a.b,hot?160:70});
            DrawRectangle(cx+10,cy+12,4,ch-24,a);
            DrawCardMotif(i,cx,cy,cw,ch,a);
            DrawText(TextFormat("%02d",i+1),cx+24,cy+12,15,a);
            DrawText(specs[i].name,cx+47,cy+12,cw<310?15:18,CC(237,232,232));
            DrawText(specs[i].tag,cx+24,cy+40,13,a);
            int statusY=cy+ch-23;
            if(challengeStats[i].completions>0){
                char best[24];
                FormatRunTime(challengeStats[i].scores[0].timeMs,best,sizeof(best));
                DrawText("COMPLETED",cx+24,statusY,12,CC(142,220,160));
                DrawText(best,cx+cw-12-MeasureText(best,12),statusY,12,
                         CC(237,230,218));
            }else{
                DrawText("NOT COMPLETED",cx+24,statusY,12,CC(160,157,178));
            }
            if(hot&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
                menuPick=i;previewPick=-1;
            }
        }
        EndScissorMode();
        DrawRectangle(lx+lw-6,vy,4,vh,CC(38,35,52));
        if(max>0){
            int thumb=vh*vh/total;
            if(thumb<28)thumb=28;
            DrawRectangle(lx+lw-6,vy+(vh-thumb)*challengeScroll/max,4,thumb,
                          specs[menuPick].accent);
        }
        Rectangle detail={(float)rx,(float)(top+24),(float)rw,(float)(bottom-top-24)};
        DrawRectangleRounded(detail,0.06f,8,CC(24,22,36));
        DrawRectangleRoundedLinesEx(detail,0.06f,8,1,specs[menuPick].accent);
        int dx=rx+18,dy=top+40;
        DrawText(specs[menuPick].name,dx,dy,rw<390?18:21,CC(247,231,211));
        DrawText(specs[menuPick].tag,dx,dy+28,14,specs[menuPick].accent);
        ChallengeProgress *progress=&challengeStats[menuPick];
        char bestTime[24];
        if(progress->timeCount)FormatRunTime(progress->scores[0].timeMs,bestTime,sizeof(bestTime));
        else snprintf(bestTime,sizeof(bestTime),"--:--");
        int buttonY=bottom-47;
        bool compactScores=h<656;
        int boardTitleY;
        if(compactScores){
            DrawText("CHALLENGE STATS",dx,dy+50,14,specs[menuPick].accent);
            DrawLine(dx,dy+67,dx+rw-36,dy+67,(Color){78,70,95,190});
            DrawText("STATUS",dx+5,dy+72,12,CC(164,158,181));
            const char *status=progress->completions?"COMPLETED":"NOT COMPLETED";
            DrawText(status,dx+rw-41-MeasureText(status,12),dy+72,12,
                     progress->completions?CC(142,220,160):CC(236,230,223));
            char runCount[48],bestLabel[48];
            snprintf(runCount,sizeof(runCount),"CLEARS %d  ATT %d",
                     progress->completions,progress->attempts);
            snprintf(bestLabel,sizeof(bestLabel),"BEST %s",bestTime);
            DrawText(runCount,dx+5,dy+88,12,CC(164,158,181));
            DrawText(bestLabel,dx+rw-41-MeasureText(bestLabel,12),dy+88,12,
                     CC(236,230,223));
            boardTitleY=dy+104;
        }else{
            int statsTop=dy+54,statsRowY=dy+78,statsRowH=21;
            DrawText("CHALLENGE STATS",dx,statsTop,14,specs[menuPick].accent);
            DrawLine(dx,statsTop+18,dx+rw-36,statsTop+18,(Color){78,70,95,190});
            const char *statLabels[4]={"STATUS","COMPLETIONS","ATTEMPTS","BEST TIME"};
            char statValues[4][32];
            snprintf(statValues[0],sizeof(statValues[0]),"%s",
                     progress->completions?"COMPLETED":"NOT COMPLETED");
            snprintf(statValues[1],sizeof(statValues[1]),"%d",progress->completions);
            snprintf(statValues[2],sizeof(statValues[2]),"%d",progress->attempts);
            snprintf(statValues[3],sizeof(statValues[3]),"%s",bestTime);
            for(int row=0;row<4;row++){
                int rowY=statsRowY+row*statsRowH;
                DrawRectangle(dx,rowY-2,rw-36,statsRowH,
                              row&1?CC(28,26,41):CC(32,29,46));
                DrawText(statLabels[row],dx+5,rowY,13,CC(164,158,181));
                int valueW=MeasureText(statValues[row],13);
                Color valueColor=(row==0&&progress->completions>0)?CC(142,220,160):CC(236,230,223);
                DrawText(statValues[row],dx+rw-41-valueW,rowY,13,valueColor);
            }
            boardTitleY=statsRowY+4*statsRowH+4;
        }
        int boardStartY=boardTitleY+19;
        int scoreRowHeight=24;
        int scoreHeaderY=boardStartY;
        int scoreListY=scoreHeaderY+17;
        int minimumMapSpace=16+CH_H+6+8;
        int rowsAvailable=(buttonY-(compactScores?5:8)-scoreListY-
                           (compactScores?0:minimumMapSpace))/scoreRowHeight;
        if(rowsAvailable<0)rowsAvailable=0;
        int scoreRows=progress->timeCount;
        if(scoreRows>CH_TIME_VISIBLE)scoreRows=CH_TIME_VISIBLE;
        if(scoreRows>rowsAvailable)scoreRows=rowsAvailable;
        int boardRowsUsed=scoreRows>0?scoreRows:1;
        int scoreTimeX=dx+rw*19/100,scoreCoinsX=dx+rw*47/100;
        int scoreTreasuresX=dx+rw*65/100,scoreKillsX=dx+rw*84/100;
        DrawText("TOP 5 HIGH SCORES",dx,boardTitleY,14,specs[menuPick].accent);
        DrawText("TIME",scoreTimeX,scoreHeaderY,13,CC(178,171,195));
        DrawText("COINS",scoreCoinsX,scoreHeaderY,13,CC(178,171,195));
        DrawText("TREAS.",scoreTreasuresX,scoreHeaderY,13,CC(178,171,195));
        DrawText("KILLS",scoreKillsX,scoreHeaderY,13,CC(178,171,195));
        if(scoreRows>0){
            for(int row=0;row<scoreRows;row++){
                int rowY=scoreListY+row*scoreRowHeight;
                Rectangle scoreCard={(float)dx,(float)(rowY-2),
                                     (float)(rw-36),(float)(scoreRowHeight-1)};
                DrawRectangleRounded(scoreCard,0.16f,4,
                                     row==0?CC(52,41,56):
                                     row&1?CC(28,26,41):CC(32,29,46));
                if(row==0)DrawRectangleRoundedLinesEx(scoreCard,0.16f,4,2,
                                                      specs[menuPick].accent);
                DrawText(TextFormat("%02d",row+1),dx+6,rowY,14,
                         row==0?specs[menuPick].accent:CC(167,160,184));
                if(row==0&&scoreTimeX-dx>=53)
                    DrawText("BEST",dx+25,rowY+3,9,specs[menuPick].accent);
                char timeText[24];
                FormatRunTime(progress->scores[row].timeMs,timeText,sizeof(timeText));
                DrawText(timeText,scoreTimeX,rowY,14,CC(242,235,225));
                const ChallengeScore *score=&progress->scores[row];
                const char *coins=score->hasRunStats?TextFormat("%d",score->coins):"--";
                const char *treasures=score->hasRunStats?TextFormat("%d",score->treasures):"--";
                const char *kills=score->hasRunStats?TextFormat("%d",score->kills):"--";
                DrawText(coins,scoreCoinsX,rowY,14,CC(242,235,225));
                DrawText(treasures,scoreTreasuresX,rowY,14,CC(242,235,225));
                DrawText(kills,scoreKillsX,rowY,14,CC(242,235,225));
            }
        }else{
            DrawText("NO SCORES YET",dx+6,scoreListY,14,CC(135,130,151));
        }
        int mapTitleY=scoreListY+boardRowsUsed*scoreRowHeight+8;
        int mapY=mapTitleY+18;
        if(buttonY-8-mapY>=CH_H){
            int mapScale=(rw-42)/CH_W;
            int heightScale=(buttonY-8-mapY)/CH_H;
            if(mapScale>heightScale)mapScale=heightScale;
            if(mapScale>9)mapScale=9;
            if(mapScale<1)mapScale=1;
            DrawText("MAP TOPOLOGY",dx,mapTitleY,14,specs[menuPick].accent);
            DrawRectangle(dx-3,mapY-3,CH_W*mapScale+6,CH_H*mapScale+6,CC(9,9,22));
            DrawMenuMap(dx,mapY,mapScale);
            DrawRectangleLines(dx-3,mapY-3,CH_W*mapScale+6,CH_H*mapScale+6,
                               specs[menuPick].accent);
        }
        Rectangle start={(float)dx,(float)(bottom-47),(float)(rw-36),37};
        if(Button(start,"START CHALLENGE  >",specs[menuPick].accent,1))StartChallenge();
    }else{
        int d,m,yr;
        uint32_t daily=DailySeed(&d,&m,&yr);
        Rectangle info={(float)lx,(float)(top+24),(float)(lw-10),(float)(bottom-top-24)};
        DrawRectangleRounded(info,0.06f,8,CC(23,27,42));
        DrawRectangleRoundedLinesEx(info,0.06f,8,1,CC(109,216,221));
        int ix=lx+24,iy=top+47;
        int tableWidth=lw-48;
        int tableSlack=tableWidth-293;
        if(tableSlack<0)tableSlack=0;
        const int seedTrackWidth=5;
        /* Keep the compact layout readable at the minimum supported window
           size, then expand the same columns smoothly as the frame grows. */
        int sourceX=ix;
        int seedX=ix+58+tableSlack*12/100-14+tableSlack*14/679;
        int roundX=ix+130+tableSlack*42/100-14+tableSlack*14/679;
        int coinsX=ix+173+tableSlack*59/100-20+tableSlack*20/679;
        int treasuresX=ix+215+tableSlack*77/100-25+tableSlack*25/679;
        int tableContentRight=ix+tableWidth-seedTrackWidth-25;
        int timeX=tableContentRight-MeasureText("TIME",13);
        DrawText("SEED HIGHSCORES",ix,iy,24,CC(234,247,247));
        DrawLine(ix,iy+36,ix+tableWidth,iy+36,CC(67,116,137));
        int headerY=iy+47,rowStart=iy+71,rowHeight=34,tableBottom=bottom-20;
        const char *roundHeading=tableWidth<450?"RND":"ROUND";
        const char *coinsHeading=tableWidth<450?"COI":"COINS";
        const char *treasuresHeading=tableWidth<450?"TRS":"TREAS.";
        DrawText("TYPE",sourceX,headerY,13,CC(158,169,184));
        DrawText("SEED",seedX,headerY,13,CC(158,169,184));
        DrawText(roundHeading,roundX,headerY,13,CC(158,169,184));
        DrawText(coinsHeading,coinsX,headerY,13,CC(158,169,184));
        DrawText(treasuresHeading,treasuresX,headerY,13,CC(158,169,184));
        DrawText("TIME",timeX,headerY,13,CC(158,169,184));
        DrawLine(ix,rowStart-5,ix+tableWidth,rowStart-5,CC(67,63,83));
        Rectangle seedViewport={(float)ix,(float)rowStart,(float)tableWidth,
                                (float)(tableBottom-rowStart)};
        Rectangle seedPanelBounds={(float)lx,(float)(top+24),(float)(lw-10),
                                   (float)(bottom-top-24)};
        int visibleSeedRows=(tableBottom-rowStart)/rowHeight;
        if(visibleSeedRows<0)visibleSeedRows=0;
        if(CheckCollisionPointRec(GetMousePosition(),seedPanelBounds)){
            seededScrollRemainder-=GetMouseWheelMove()*3.0f;
            int wheelRows=(int)seededScrollRemainder;
            if(wheelRows){
                seededScroll+=wheelRows;
                seededScrollRemainder-=wheelRows;
            }
        }
        if(IsKeyPressed(KEY_PAGE_DOWN)){
            seededScroll+=visibleSeedRows>0?visibleSeedRows:1;
            seededScrollRemainder=0.0f;
        }
        if(IsKeyPressed(KEY_PAGE_UP)){
            seededScroll-=visibleSeedRows>0?visibleSeedRows:1;
            seededScrollRemainder=0.0f;
        }
        int maxSeedScroll=(int)seedProgressCount-visibleSeedRows;
        if(maxSeedScroll<0)maxSeedScroll=0;
        if(seededScroll<0)seededScroll=0;
        if(seededScroll>maxSeedScroll)seededScroll=maxSeedScroll;
        if((seededScroll==0&&seededScrollRemainder<0.0f)||
           (seededScroll==maxSeedScroll&&seededScrollRemainder>0.0f))
            seededScrollRemainder=0.0f;
        BeginScissorMode((int)seedViewport.x,(int)seedViewport.y,
                         (int)seedViewport.width,(int)seedViewport.height);
        if(seedProgressCount==0){
            DrawText("NO SAVED SEED RUNS",ix+6,rowStart+8,14,CC(142,154,173));
        }else{
            for(int row=0;row<visibleSeedRows;row++){
                size_t index=(size_t)(seededScroll+row);
                if(index>=seedProgressCount)break;
                SeedProgress *record=&seedScores[index];
                int rowY=rowStart+row*rowHeight;
                DrawRectangle(ix,rowY,tableWidth,rowHeight-2,
                              row&1?CC(25,30,44):CC(29,34,49));
                const char *source=(record->sourceFlags==(SEED_SOURCE_DAILY|SEED_SOURCE_CUSTOM))?
                    "BOTH":(record->sourceFlags&SEED_SOURCE_DAILY)?"DAILY":"CUSTOM";
                Color sourceColor=(record->sourceFlags&SEED_SOURCE_DAILY)?
                    CC(232,188,115):CC(126,216,204);
                DrawText(source,sourceX+2,rowY+8,14,sourceColor);
                DrawText(TextFormat("%08X",record->seed),seedX,rowY+8,14,CC(221,226,235));
                DrawText(record->bestRound>0?TextFormat("%d",record->bestRound):"--",
                         roundX,rowY+8,14,CC(236,230,218));
                DrawText(record->bestRound>0?TextFormat("%d",record->bestCoins):"--",
                         coinsX,rowY+8,14,CC(236,230,218));
                DrawText(record->bestRound>0?TextFormat("%d",record->bestTreasures):"--",
                         treasuresX,rowY+8,14,CC(236,230,218));
                char seedTime[24];
                if(record->bestTimeMs>0)FormatRunTime(record->bestTimeMs,seedTime,sizeof(seedTime));
                else snprintf(seedTime,sizeof(seedTime),"--:--");
                int seedTimeX=tableContentRight-MeasureText(seedTime,14);
                DrawText(seedTime,seedTimeX,rowY+8,14,CC(236,230,218));
            }
        }
        EndScissorMode();
        DrawRectangle(ix+tableWidth-seedTrackWidth,rowStart,seedTrackWidth,
                      tableBottom-rowStart,CC(37,41,57));
        if(visibleSeedRows>0&&(int)seedProgressCount>visibleSeedRows){
            int total=(int)seedProgressCount;
            int thumb=(tableBottom-rowStart)*visibleSeedRows/total;
            if(thumb<22)thumb=22;
            DrawRectangle(ix+tableWidth-seedTrackWidth,
                          rowStart+((tableBottom-rowStart)-thumb)*seededScroll/maxSeedScroll,
                          seedTrackWidth,thumb,CC(109,216,221));
        }
        Rectangle detail={(float)rx,(float)(top+24),(float)rw,(float)(bottom-top-24)};
        DrawRectangleRounded(detail,0.06f,8,CC(24,22,36));
        DrawRectangleRoundedLinesEx(detail,0.06f,8,1,CC(109,216,221));
        int dx=rx+19,dy=top+46;
        DrawText("CHOOSE YOUR SEED",dx,dy,21,CC(234,247,247));
        Rectangle db={(float)dx,(float)(dy+48),(float)((rw-49)/2),37};
        Rectangle cb={(float)(dx+(rw-49)/2+10),(float)(dy+48),(float)((rw-49)/2),37};
        if(Button(db,seedMode?"DAILY":"DAILY  *",CC(236,181,100),1)){
            seedMode=0;customFocus=0;
        }
        if(Button(cb,seedMode?"CUSTOM  *":"CUSTOM",CC(129,222,213),1)){
            seedMode=1;customFocus=1;
        }
        if(!customFocus&&IsKeyPressed(KEY_D))seedMode=0;
        int activate=!customFocus&&IsKeyPressed(KEY_C);
        if(activate){seedMode=1;customFocus=1;}
        if(!customFocus){customSeedEraseKey=0;customSeedEraseTimer=0.0f;}
        if(!seedMode){
            DrawText("TODAY",dx,dy+111,14,CC(218,181,112));
            DrawText(TextFormat("%02d.%02d.%04d",d,m,yr),dx,dy+137,23,
                     CC(255,220,144));
            DrawText(TextFormat("SEED %u",daily),dx,dy+183,16,
                     CC(218,181,112));
        }else{
            DrawText("YOUR SEED",dx,dy+110,14,CC(130,219,212));
            Rectangle entry={(float)dx,(float)(dy+139),(float)(rw-38),39};
            DrawRectangleRounded(entry,0.14f,6,CC(12,12,25));
            DrawRectangleRoundedLinesEx(entry,0.14f,6,2,
                customFocus?CC(137,229,215):CC(80,75,98));
            DrawText(customSeed[0]?customSeed:"Enter a number or text seed",
                     dx+12,dy+149,16,
                     customSeed[0]?CC(229,233,237):CC(105,104,120));
            if(CheckCollisionPointRec(GetMousePosition(),entry)&&
               IsMouseButtonPressed(MOUSE_BUTTON_LEFT))customFocus=1;
            if(customFocus&&!activate){
                int key=GetCharPressed();
                while(key>0){
                    size_t n=strlen(customSeed);
                    if(key>=32&&key<=126&&n<24){
                        customSeed[n]=(char)key;customSeed[n+1]='\0';
                    }
                    key=GetCharPressed();
                }
                UpdateCustomSeedErase();
                if((IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_KP_ENTER))&&customSeed[0])
                    customFocus=0;
            }
            if(customSeed[0]){
                uint32_t seed=ActiveSeed();
                DrawText(TextFormat("USED SEED %u",seed),dx,dy+201,16,
                         CC(130,219,212));
                DrawText(TextFormat("HEX %08X",seed),dx,dy+226,14,
                         CC(166,183,197));
            }
        }
        Rectangle start={(float)dx,(float)(bottom-47),(float)(rw-38),37};
        int valid=!seedMode||customSeed[0];
        if(Button(start,"START SEEDED RUN  >",CC(109,216,221),valid))StartChallenge();
    }
}

void ChallengeHandleMenuConfirm(void){
    if(customFocus)return;
    if(menuTab==1&&seedMode&&!customSeed[0])return;
    StartChallenge();
}
