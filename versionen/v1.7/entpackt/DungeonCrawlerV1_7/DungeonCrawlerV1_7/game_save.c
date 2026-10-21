// game_save.c — stats/highscore persistence and per-game kill bookkeeping.
#include "game_save.h"
#include "globals.h"
#include "constants.h"   // STATS_FILE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "game_items.h"
#include "game_map.h"
#include "game_challenges.h"

static StatsBreakdown statsBreakdown;
static bool sideRoomPending=false,sideRoomEntered=false;
static bool artifactRoomActive=false;
static float artifactRoomElapsed=0.0f;
static bool mainRunActive=false,bossTiming=false,deathRecorded=false;
static double runElapsed=0.0,bossElapsed=0.0;
static double roundElapsed=0.0;
static unsigned reachedMilestones=0;
typedef struct { int round,deaths; } DeathRoundEntry;
static DeathRoundEntry *deathRounds=NULL;
static int deathRoundCount=0,deathRoundCapacity=0;

const StatsBreakdown *GetStatsBreakdown(void) { return &statsBreakdown; }
void StatsRecordUnclassifiedCoins(int amount) { if(amount>0)statsBreakdown.legacy.coins+=amount; }

static int CurrentStatsMode(void)
{
    int mode=ChallengeRunMode();
    return mode>=0&&mode<STATS_RUN_MODE_COUNT?mode:STATS_MAIN_RUN;
}

static bool RoomStatsEnabled(void)
{
    return eneon && ChallengeRunMode()!=STATS_CHALLENGE_RUN;
}

void StatsFinishCurrentRoom(void)
{
    if(sideRoomPending&&!sideRoomEntered)statsBreakdown.side.skipped++;
    sideRoomPending=sideRoomEntered=false;
    artifactRoomActive=false;
    artifactRoomElapsed=0.0f;
    bossTiming=false;
}

void StatsTickRoom(float dt)
{
    if(artifactRoomActive&&dt>0.0f)artifactRoomElapsed+=dt;
    if(dt>0.0f){
        runElapsed+=dt;
        roundElapsed+=dt;
    }
    if(mainRunActive&&dt>0.0f){
        ExtendedStats *e=&statsBreakdown.extra;
        e->activePlaySeconds+=dt;
        if(runElapsed>e->longestRunSeconds)e->longestRunSeconds=runElapsed;
        if(bossTiming)bossElapsed+=dt;
    }
}

double StatsCurrentRunSeconds(void){return runElapsed;}
double StatsCurrentRoundSeconds(void){return roundElapsed;}

void StatsRunStarted(void)
{
    mainRunActive=StatsEnabled();
    runElapsed=roundElapsed=bossElapsed=0.0;
    bossTiming=deathRecorded=false;
    reachedMilestones=0;
}

static int RoomStatsIndex(RoomKind kind)
{
    switch(kind){
        case RK_TRAP:return STATS_ROOM_TRAP;
        case RK_DARK:return STATS_ROOM_DARK;
        case RK_LAVA:return STATS_ROOM_LAVA;
        case RK_COIN_RUSH:return STATS_ROOM_COIN_RUSH;
        default:return -1;
    }
}

void StatsRoundStarted(RoomKind kind,int round)
{
    roundElapsed=0.0;
    if(!mainRunActive)return;
    ExtendedStats *e=&statsBreakdown.extra;
    int idx=RoomStatsIndex(kind);
    if(idx>=0)e->roomStarts[idx]++;
    deathRecorded=false;
    if(round>=5&&!(reachedMilestones&1u)){e->runsReached5++;reachedMilestones|=1u;}
    if(round>=10&&!(reachedMilestones&2u)){e->runsReached10++;reachedMilestones|=2u;}
    if(round>=20&&!(reachedMilestones&4u)){e->runsReached20++;reachedMilestones|=4u;}
}

void StatsRoundCompleted(RoomKind kind,int round)
{
    if(!mainRunActive)return;
    ExtendedStats *e=&statsBreakdown.extra;
    int idx=RoomStatsIndex(kind);
    if(idx>=0)e->roomClears[idx]++;
    if(round==10&&(e->fastestTenSeconds<=0.0||runElapsed<e->fastestTenSeconds))
        e->fastestTenSeconds=runElapsed;
}

void StatsTileWalked(void){if(mainRunActive)statsBreakdown.extra.tilesWalked++;}
void StatsShopSpent(StatsShopType kind,int amount)
{
    if(kind>=0&&kind<STATS_SHOP_TYPE_COUNT&&amount>0)
        statsBreakdown.extra.shopSpent[kind]+=amount;
}
void StatsCoinEarned(StatsCoinSource source,int amount)
{
    if(StatsEnabled()&&source>=0&&source<STATS_COIN_SOURCE_COUNT&&amount>0)
        statsBreakdown.extra.coinSources[source]+=amount;
}
void StatsGoldTouchKill(void){if(StatsEnabled())statsBreakdown.extra.goldTouchKills++;}
void StatsBossStarted(void)
{
    if(!mainRunActive)return;
    statsBreakdown.extra.bossFightsStarted++;
    bossTiming=true;bossElapsed=0.0;
}
void StatsBossDefeated(void)
{
    if(!bossTiming)return;
    ExtendedStats *e=&statsBreakdown.extra;
    e->bossFightsWon++;
    if(bossElapsed>0.0&&(e->fastestBossSeconds<=0.0||bossElapsed<e->fastestBossSeconds))
        e->fastestBossSeconds=bossElapsed;
    bossTiming=false;
}
void StatsKeysDiscarded(int count){if(count>0)statsBreakdown.extra.unusedKeys+=count;}
void StatsLockOpened(int keyId)
{
    if(keyId>=0&&keyId<KEY_COUNT)statsBreakdown.extra.locksOpenedByKey[keyId]++;
}
void StatsChallengeCompleted(bool firstTry,int elapsedMs)
{
    ExtendedStats *e=&statsBreakdown.extra;
    if(firstTry)e->firstTryChallengeCompletions++;
    if(elapsedMs>0){e->challengeCompletionCount++;e->challengeCompletionTimeMs+=elapsedMs;}
    SaveStats();
}
void StatsDailyStarted(void)
{
    time_t now=time(NULL);
    struct tm *current=localtime(&now);
    if(!current)return;
    int date=(current->tm_year+1900)*10000+(current->tm_mon+1)*100+current->tm_mday;
    ExtendedStats *e=&statsBreakdown.extra;
    if(date==e->lastDailyDate)return;
    bool consecutive=false;
    if(e->lastDailyDate>0){
        struct tm previous={0};
        previous.tm_year=e->lastDailyDate/10000-1900;
        previous.tm_mon=(e->lastDailyDate/100)%100-1;
        previous.tm_mday=e->lastDailyDate%100;
        previous.tm_hour=12;previous.tm_isdst=-1;
        time_t last=mktime(&previous);
        struct tm today=*current;
        today.tm_hour=12;today.tm_min=today.tm_sec=0;today.tm_isdst=-1;
        time_t thisDay=mktime(&today);
        double gap=difftime(thisDay,last);
        consecutive=gap>12.0*3600.0&&gap<36.0*3600.0;
    }
    e->dailyCurrentStreak=consecutive?e->dailyCurrentStreak+1:1;
    if(e->dailyCurrentStreak>e->dailyLongestStreak)
        e->dailyLongestStreak=e->dailyCurrentStreak;
    e->lastDailyDate=date;
    SaveStats();
}
int StatsMostLethalRound(int *deaths)
{
    int bestRound=0,bestDeaths=0;
    for(int i=0;i<deathRoundCount;i++)
        if(deathRounds[i].deaths>bestDeaths){
            bestRound=deathRounds[i].round;bestDeaths=deathRounds[i].deaths;
        }
    if(deaths)*deaths=bestDeaths;
    return bestRound;
}

void StatsSideRoomAppeared(void)
{
    if(!RoomStatsEnabled())return;
    sideRoomPending=true;sideRoomEntered=false;
    statsBreakdown.side.appeared++;
}

void StatsSideRoomEntered(void)
{
    if(!sideRoomPending||sideRoomEntered)return;
    sideRoomEntered=true;
    statsBreakdown.side.entered++;
}

void StatsSideRoomGateOpened(void)
{
    if(RoomStatsEnabled())statsBreakdown.side.gatesOpened++;
}

void StatsKeyCollected(void)
{
    statsBreakdown.keysCollected++;
}

void StatsKeySpent(void)
{
    statsBreakdown.keysSpent++;
}

void StatsArtifactPortalEntered(void)
{
    if(!RoomStatsEnabled())return;
    statsBreakdown.artifact.portalsEntered++;
    artifactRoomActive=true;
    artifactRoomElapsed=0.0f;
}

void StatsArtifactLadderUsed(void)
{
    if(!artifactRoomActive)return;
    statsBreakdown.artifact.arenasSurvived++;
    statsBreakdown.artifact.laddersUsed++;
}

void StatsArtifactVaultReached(void)
{
    if(artifactRoomActive)statsBreakdown.artifact.vaultsReached++;
}

void StatsArtifactRecoveryTimed(void)
{
    if(!artifactRoomActive)return;
    int elapsedMs=(int)(artifactRoomElapsed*1000.0f+0.5f);
    if(elapsedMs<1)elapsedMs=1;
    if(statsBreakdown.artifact.fastestRecoveryMs==0||
       elapsedMs<statsBreakdown.artifact.fastestRecoveryMs)
        statsBreakdown.artifact.fastestRecoveryMs=elapsedMs;
}

void StatsArtifactTreasureCollected(void)
{
    if(artifactRoomActive)statsBreakdown.artifact.treasuresCollected++;
}

static RunTotals LegacyTotals(void)
{
    return (RunTotals){global_games_played,global_rounds,global_totcoi,
                       global_totkills,global_tottrs};
}

static void RecordMainGameDeath(void)
{
    if(!StatsEnabled()||deathRecorded)return;
    deathRecorded=true;
    mainRunActive=false;bossTiming=false;
    if(roundn>0){
        int i=0;
        for(;i<deathRoundCount;i++)if(deathRounds[i].round==roundn)break;
        if(i==deathRoundCount){
            if(deathRoundCount==deathRoundCapacity){
                int next=deathRoundCapacity?deathRoundCapacity*2:16;
                DeathRoundEntry *grown=realloc(deathRounds,(size_t)next*sizeof(*deathRounds));
                if(grown){deathRounds=grown;deathRoundCapacity=next;}
            }
            if(deathRoundCount<deathRoundCapacity){
                deathRounds[deathRoundCount++]=(DeathRoundEntry){roundn,0};
            }
        }
        if(i<deathRoundCount)deathRounds[i].deaths++;
    }
    int left=0;
    for(int i=0;i<MAX_COI;i++)if(coins[i].on)left+=coinRush?2:1;
    for(int y=0;y<mh;y++)for(int x=0;x<mw;x++)if(map[y][x]=='%')left+=100;
    statsBreakdown.extra.coinsLeftOnDeath+=left;
}

bool StatsEnabled(void)
{
    // The normal lifetime totals only describe ordinary runs.
    return eneon && ChallengeRunMode() == 0;
}

void StatsInc(int *counter)
{
    if(!counter||!eneon)return;
    RunTotals *mode=&statsBreakdown.mode[CurrentStatsMode()];
    if(counter==&global_games_played)mode->games++;
    else if(counter==&global_rounds)mode->rounds++;
    else if(counter==&global_totcoi)mode->coins++;
    else if(counter==&global_totkills)mode->kills++;
    else if(counter==&global_tottrs)mode->treasures++;
    if(StatsEnabled())(*counter)++;
}

void StatsAdd(int *counter, int n)
{
    if(!counter||!eneon)return;
    if(counter==&global_totcoi)statsBreakdown.mode[CurrentStatsMode()].coins+=n;
    if(StatsEnabled())(*counter)+=n;
}

void LoadStats(void) {
    FILE *f = fopen(STATS_FILE, "r");
    if (!f) return;
    // Line 1: original 10 fields
    fscanf(f, "%d %d %d %d %d %d %d %d %d %d",
           &hs_round, &hs_trs, &hs_coi, &global_totcoi,
           &hs_kills, &global_totkills, &global_tottrs, &global_rounds,
           &coin_balance, &global_games_played);
    // Line 2: extended stats (missing in old saves → values stay 0)
    fscanf(f, "%d %d %d %d %d %d %d %d %d %d %d %d %d",
           &hs_coi_round,
           &global_deaths_enemy, &global_deaths_trap,
           &global_kills_sword,  &global_kills_shield,
           &global_portals, &global_ghostwalk_walls, &global_inv_full,
           &global_use_spd, &global_use_shd, &global_use_bmb,
           &global_use_wlk, &global_use_swd);
    // Line 3: V1.6 stats (missing in old saves → values stay 0)
    fscanf(f, "%d %d %d", &global_bosses_killed, &global_boss_dmg, &global_deaths_lava);
    // Line 4: V1.6 achievement counters (missing in old saves → values stay 0)
    fscanf(f, "%d %d %d %d %d %d %d %d %d",
           &global_use_gld,
           &global_rooms_trap, &global_rooms_dark, &global_rooms_lava, &global_rooms_coinrush,
           &global_cannon_hits, &hs_unbeaten, &global_deaths_boss, &global_deaths_trapdoor);
    if (hs_unbeaten < 0) hs_unbeaten = 0;
    if (hs_unbeaten > 6) hs_unbeaten = 6;
    for(int i=0;i<ARTIFACT_COUNT;i++) {
        int owned=0;
        if(fscanf(f,"%d",&owned)==1) artifactOwned[i]=(owned==1);
    }
    unsigned discovered=0;
    if(fscanf(f,"%u",&discovered)==1)
        lockedObjectDiscoveredMask=discovered&((1u<<LOCKED_OBJECT_DESIGN_COUNT)-1u);
    char marker[24]={0};int version=0;
    if(fscanf(f,"%23s %d",marker,&version)==2&&strcmp(marker,"RUNSTATS")==0&&
       (version>=1&&version<=4)){
        StatsBreakdown loaded={0};
        RunTotals *r=&loaded.legacy;
        bool valid=fscanf(f,"%d %d %d %d %d",&r->games,&r->rounds,&r->coins,&r->kills,&r->treasures)==5;
        for(int i=0;i<STATS_RUN_MODE_COUNT&&valid;i++){
            r=&loaded.mode[i];
            valid=fscanf(f,"%d %d %d %d %d",&r->games,&r->rounds,&r->coins,&r->kills,&r->treasures)==5;
        }
        SideRoomTotals *s=&loaded.side;
        int retiredSideArtifacts=0,retiredRoomArtifacts=0;
        int oldSideKeysCollected=0,oldSideKeysSpent=0;
        if(valid&&version==1)valid=fscanf(f,"%d %d %d %d %d %d %d",&s->appeared,&s->entered,
            &s->skipped,&s->gatesOpened,&oldSideKeysCollected,&oldSideKeysSpent,&retiredSideArtifacts)==7;
        else if(valid&&version==2)valid=fscanf(f,"%d %d %d %d %d %d",&s->appeared,&s->entered,
            &s->skipped,&s->gatesOpened,&oldSideKeysCollected,&oldSideKeysSpent)==6;
        else if(valid)valid=fscanf(f,"%d %d %d %d",&s->appeared,&s->entered,
            &s->skipped,&s->gatesOpened)==4;
        if(version<=2){
            loaded.keysCollected=oldSideKeysCollected;
            loaded.keysSpent=oldSideKeysSpent;
        }
        ArtifactRoomTotals *a=&loaded.artifact;
        if(valid&&version==1)valid=fscanf(f,"%d %d %d %d %d %d %d",&a->portalsEntered,
            &a->arenasSurvived,&a->laddersUsed,&a->vaultsReached,&retiredRoomArtifacts,
            &a->treasuresCollected,&a->fastestRecoveryMs)==7;
        else if(valid)valid=fscanf(f,"%d %d %d %d %d %d",&a->portalsEntered,
            &a->arenasSurvived,&a->laddersUsed,&a->vaultsReached,&a->treasuresCollected,
            &a->fastestRecoveryMs)==6;
        if(valid&&version>=3)valid=fscanf(f,"%d %d",&loaded.keysCollected,&loaded.keysSpent)==2;
        DeathRoundEntry *parsedDeaths=NULL;
        int parsedDeathCount=0;
        if(valid&&version==4){
            ExtendedStats *e=&loaded.extra;
            valid=fscanf(f,"%lf %lf %lf %lf",&e->activePlaySeconds,&e->longestRunSeconds,
                &e->fastestTenSeconds,&e->fastestBossSeconds)==4;
            if(valid)valid=fscanf(f,"%d %d %d %d",&e->runsReached5,&e->runsReached10,
                &e->runsReached20,&e->tilesWalked)==4;
            for(int i=0;i<STATS_ROOM_TYPE_COUNT&&valid;i++)
                valid=fscanf(f,"%d",&e->roomStarts[i])==1;
            for(int i=0;i<STATS_ROOM_TYPE_COUNT&&valid;i++)
                valid=fscanf(f,"%d",&e->roomClears[i])==1;
            for(int i=0;i<STATS_SHOP_TYPE_COUNT&&valid;i++)
                valid=fscanf(f,"%d",&e->shopSpent[i])==1;
            for(int i=0;i<STATS_COIN_SOURCE_COUNT&&valid;i++)
                valid=fscanf(f,"%d",&e->coinSources[i])==1;
            if(valid)valid=fscanf(f,"%d %d %d %d",&e->coinsLeftOnDeath,&e->goldTouchKills,
                &e->bossFightsStarted,&e->bossFightsWon)==4;
            if(valid)valid=fscanf(f,"%d",&e->unusedKeys)==1;
            for(int i=0;i<KEY_COUNT&&valid;i++)
                valid=fscanf(f,"%d",&e->locksOpenedByKey[i])==1;
            if(valid)valid=fscanf(f,"%d %d %lld",&e->firstTryChallengeCompletions,
                &e->challengeCompletionCount,&e->challengeCompletionTimeMs)==3;
            if(valid)valid=fscanf(f,"%d %d %d",&e->dailyCurrentStreak,
                &e->dailyLongestStreak,&e->lastDailyDate)==3;
            if(valid)valid=fscanf(f,"%d",&parsedDeathCount)==1&&
                parsedDeathCount>=0&&parsedDeathCount<=100000;
            if(valid&&parsedDeathCount>0){
                parsedDeaths=malloc((size_t)parsedDeathCount*sizeof(*parsedDeaths));
                if(!parsedDeaths)valid=false;
            }
            for(int i=0;i<parsedDeathCount&&valid;i++)
                valid=fscanf(f,"%d %d",&parsedDeaths[i].round,&parsedDeaths[i].deaths)==2;
        }
        if(valid){
            statsBreakdown=loaded;
            if(version==4){
                free(deathRounds);
                deathRounds=parsedDeaths;
                deathRoundCount=deathRoundCapacity=parsedDeathCount;
            }
        }else{
            free(parsedDeaths);
            statsBreakdown.legacy=LegacyTotals();
        }
    }else statsBreakdown.legacy=LegacyTotals();
    fclose(f);
}

void SaveStats(void) {
    FILE *f = fopen(STATS_FILE, "w");
    if (!f) return;
    fprintf(f, "%d %d %d %d %d %d %d %d %d %d\n",
            hs_round, hs_trs, hs_coi, global_totcoi,
            hs_kills, global_totkills, global_tottrs, global_rounds,
            coin_balance, global_games_played);
    fprintf(f, "%d %d %d %d %d %d %d %d %d %d %d %d %d\n",
            hs_coi_round,
            global_deaths_enemy, global_deaths_trap,
            global_kills_sword,  global_kills_shield,
            global_portals, global_ghostwalk_walls, global_inv_full,
            global_use_spd, global_use_shd, global_use_bmb,
            global_use_wlk, global_use_swd);
    fprintf(f, "%d %d %d\n", global_bosses_killed, global_boss_dmg, global_deaths_lava);
    fprintf(f, "%d %d %d %d %d %d %d %d %d\n",
            global_use_gld,
            global_rooms_trap, global_rooms_dark, global_rooms_lava, global_rooms_coinrush,
            global_cannon_hits, hs_unbeaten, global_deaths_boss, global_deaths_trapdoor);
    for(int i=0;i<ARTIFACT_COUNT;i++) fprintf(f,"%d%c",artifactOwned[i],i==ARTIFACT_COUNT-1 ? '\n' : ' ');
    fprintf(f,"%u\n",lockedObjectDiscoveredMask);
    fprintf(f,"RUNSTATS 4\n");
    const RunTotals *r=&statsBreakdown.legacy;
    fprintf(f,"%d %d %d %d %d\n",r->games,r->rounds,r->coins,r->kills,r->treasures);
    for(int i=0;i<STATS_RUN_MODE_COUNT;i++){
        r=&statsBreakdown.mode[i];
        fprintf(f,"%d %d %d %d %d\n",r->games,r->rounds,r->coins,r->kills,r->treasures);
    }
    const SideRoomTotals *s=&statsBreakdown.side;
    fprintf(f,"%d %d %d %d\n",s->appeared,s->entered,s->skipped,s->gatesOpened);
    const ArtifactRoomTotals *a=&statsBreakdown.artifact;
    fprintf(f,"%d %d %d %d %d %d\n",a->portalsEntered,a->arenasSurvived,a->laddersUsed,
        a->vaultsReached,a->treasuresCollected,a->fastestRecoveryMs);
    fprintf(f,"%d %d\n",statsBreakdown.keysCollected,statsBreakdown.keysSpent);
    const ExtendedStats *e=&statsBreakdown.extra;
    fprintf(f,"%.6f %.6f %.6f %.6f\n",e->activePlaySeconds,e->longestRunSeconds,
        e->fastestTenSeconds,e->fastestBossSeconds);
    fprintf(f,"%d %d %d %d\n",e->runsReached5,e->runsReached10,e->runsReached20,e->tilesWalked);
    for(int i=0;i<STATS_ROOM_TYPE_COUNT;i++)fprintf(f,"%d ",e->roomStarts[i]);
    for(int i=0;i<STATS_ROOM_TYPE_COUNT;i++)fprintf(f,"%d%c",e->roomClears[i],i==STATS_ROOM_TYPE_COUNT-1?'\n':' ');
    for(int i=0;i<STATS_SHOP_TYPE_COUNT;i++)fprintf(f,"%d ",e->shopSpent[i]);
    for(int i=0;i<STATS_COIN_SOURCE_COUNT;i++)fprintf(f,"%d%c",e->coinSources[i],
        i==STATS_COIN_SOURCE_COUNT-1?'\n':' ');
    fprintf(f,"%d %d %d %d\n",e->coinsLeftOnDeath,e->goldTouchKills,
        e->bossFightsStarted,e->bossFightsWon);
    fprintf(f,"%d ",e->unusedKeys);
    for(int i=0;i<KEY_COUNT;i++)fprintf(f,"%d%c",e->locksOpenedByKey[i],i==KEY_COUNT-1?'\n':' ');
    fprintf(f,"%d %d %lld\n",e->firstTryChallengeCompletions,e->challengeCompletionCount,
        e->challengeCompletionTimeMs);
    fprintf(f,"%d %d %d\n",e->dailyCurrentStreak,e->dailyLongestStreak,e->lastDailyDate);
    fprintf(f,"%d\n",deathRoundCount);
    for(int i=0;i<deathRoundCount;i++)
        fprintf(f,"%d %d\n",deathRounds[i].round,deathRounds[i].deaths);
    fclose(f);
}

void UpdateAndSaveStats(void) {
    if(gs==GS_DEAD)RecordMainGameDeath();
    if(gs==GS_DEAD||gs==GS_NEXT)StatsFinishCurrentRoom();
    // Practice and challenge runs keep their own progression. Persist currency
    // and collection changes without folding these runs into normal highscores.
    if (!StatsEnabled()) {
        SaveStats();
        return;
    }
    if (roundn   > hs_round)     hs_round     = roundn;
    if (tottrs   > hs_trs)       hs_trs       = tottrs;
    if (totcoi   > hs_coi)       hs_coi       = totcoi;
    if (kills_game > hs_kills)   hs_kills     = kills_game;
    if (coins_round > hs_coi_round) hs_coi_round = coins_round;
    // Unbeaten Champion tier 1: reach round 5
    if (hs_round >= 5 && hs_unbeaten < 1) hs_unbeaten = 1;
    SaveStats();
}

void AddEnemyKill(void) {
    kills_game++;
    StatsInc(&global_totkills);
    if (!StatsEnabled()) return;
    if (kills_game > hs_kills) hs_kills = kills_game;
}
