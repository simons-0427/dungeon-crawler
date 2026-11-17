#include "mobile_debug.h"
#include "game/globals.h"
#include "game/game_shop.h"
#include "game/game_save.h"
#include "game/game_ui.h"
#include "game/game_items.h"
#include "game/advancements.h"

#ifdef MOBILE_MAXED_DEBUG
void __real_BuildAchievementProgress(AchievementProgress out[ACH_COUNT]);
void __wrap_BuildAchievementProgress(AchievementProgress out[ACH_COUNT]){
    __real_BuildAchievementProgress(out);
    for(int i=0;i<ACH_COUNT;i++)
        if(out[i].tierCount>0)out[i].value=out[i].thresholds[out[i].tierCount-1];
}
#endif

void MobileApplyDebugProfile(void){
#ifdef MOBILE_MAXED_DEBUG
    for(int i=0;i<SKIN_COUNT;i++)skin_owned[i]=1;
    for(int i=0;i<MUSIC_TRACK_COUNT;i++)music_owned[i]=1;
    for(int i=0;i<ACH_COUNT;i++)for(int j=0;j<6;j++)ach_claimed[i][j]=1;
    for(int i=0;i<ARTIFACT_COUNT;i++)artifactOwned[i]=true;
    lockedObjectDiscoveredMask=~0u;
    spd_level=shd_level=wlk_level=bmb_level=gld_level=5;
    swd_level=3;
    perk_slots=4;perk_magnet=perk_shield2=1;
    if(coin_balance<999999)coin_balance=999999;
    SaveShop();SaveStats();
#endif
}

void MobileDebugUpdate(void){
#ifdef MOBILE_MAXED_DEBUG
    // A full test inventory is available at the beginning of every new run.
    static GState previous=GS_MENU;
    if(gs==GS_PLAY&&previous!=GS_PLAY&&pinvn==0){
        static const IKind items[]={IK_SPD,IK_SHD,IK_WLK,IK_GLD,IK_SWD};
        pinvn=5;
        for(int i=0;i<pinvn;i++)pinv[i]=items[i];
    }
    previous=gs;
#endif
}
