// Additional field-guide pages. Included by game_ui.c after the shared book art.

static void TutorialInfoCard(Rectangle card, const char *title, const char *body,
                             Color accent, TutorialBookStyle b) {
    DrawRectangleRounded(card, 0.10f, 5, TutorialColorA(b.paperDeep, 37));
    DrawRectangle((int)card.x, (int)card.y + MenuPx(b.u,4),
                  (int)fmaxf(2.0f, MenuPx(b.u,3)),
                  (int)card.height - MenuPx(b.u,8),
                  TutorialColorA(accent, 205));
    int x=(int)card.x+MenuPx(b.u,11);
    int width=(int)card.width-MenuPx(b.u,19);
    int titleY=(int)card.y+MenuPx(b.u,5);
    DrawTutorialTextFit(title,x,titleY,b.fsH,width,accent);
    int bodyY=titleY+b.fsH+MenuPx(b.u,3);
    int bodyH=(int)(card.y+card.height)-bodyY-MenuPx(b.u,5);
    if(bodyH>0)
        DrawTutorialTextWrapped(body,x,bodyY,b.fsB-1,width,bodyH,
                                MenuPx(b.u,2),b.inkDim);
}

static int TutorialExampleArtifact(bool sideRoom) {
    int fallback=-1;
    for(int id=0;id<ARTIFACT_COUNT;id++)if(ArtifactDesignPlayable(id)&&
        ArtifactAppearsInLocation(id,sideRoom)) {
        if(fallback<0)fallback=id;
        if(ArtifactLockOptionCount(id)>0)return id;
    }
    return fallback;
}

static int TutorialExampleKey(int artifactId) {
    if(artifactId<0||ArtifactLockOptionCount(artifactId)<=0)return -1;
    unsigned keys=LockedObjectAssignedKeys(ArtifactLockOptionAt(artifactId,0));
    for(int keyId=0;keyId<KEY_COUNT;keyId++)
        if(keys&(1u<<keyId))return keyId;
    return -1;
}

#include "tutorial_book_maps.inc.c"

static void DrawTutorialBossRosterPage(Rectangle page,TutorialBookStyle b,float t) {
    int x=(int)page.x+MenuPx(b.u,18);
    int w=(int)page.width-MenuPx(b.u,36);
    int y=DrawTutorialPageHeading(page,"KNOW THE BOSSES",b);
    DrawTutorialTextFit("SECONDS PER TILE: LOWER IS FASTER / 1s IMMUNITY AFTER A HIT",
                        x,y,b.fsB-1,w,b.inkDim);
    y+=b.fsB+MenuPx(b.u,7);
    int bottom=(int)(page.y+page.height-MenuPx(b.u,25));
    int gap=MenuPx(b.u,5);
    int rowH=(bottom-y-gap*4)/5;
    Color accents[5]={{153,56,169,255},{168,64,69,255},
                      {190,101,43,255},{177,86,35,255},
                      {69,143,150,255}};
    for(int index=0;index<5;index++) {
        BossGuideInfo info=BossGuideForTier(index);
        int rowY=y+index*(rowH+gap);
        Rectangle row={(float)x,(float)rowY,(float)w,(float)rowH};
        DrawRectangleRounded(row,0.10f,5,TutorialColorA(b.paperDeep,42));
        DrawRectangle(x,rowY+5,3,rowH-10,accents[index]);
        int iconX=x+MenuPx(b.u,36);
        int iconY=rowY+rowH/2;
        DrawCircle(iconX,iconY,MenuPx(b.u,27),
                   TutorialColorA(accents[index],45));
        DrawBossPreview((float)iconX,(float)iconY,
                        (float)MenuPx(b.u,17),index,t);
        int tx=x+MenuPx(b.u,70);
        int tw=w-MenuPx(b.u,119);
        DrawTutorialTextFit(TextFormat("%s  /  ROUND %d%s  /  HP %d",
                            info.name,info.round,index==4?"+":"",info.hp),
                            tx,rowY+MenuPx(b.u,6),b.fsH,tw,accents[index]);
        DrawTutorialTextFit(TextFormat("MOVE %.2fs/tile   DASH %.3fs/tile",
                            info.stalkStep,info.dashStep),tx,
                            rowY+MenuPx(b.u,23),b.fsB-1,tw,b.ink);
        const char *ability=info.doubleDash?
            TextFormat("DOUBLE DASH / FIRE %d TILES / TRAMPLE",info.fireRange):
            info.fire?
            TextFormat("DASH / FIRE %d TILES / TRAMPLE",info.fireRange):
            "DASH / TRAMPLE";
        DrawTutorialTextFit(ability,tx,rowY+MenuPx(b.u,38),b.fsB-1,tw,b.ink);
        DrawTutorialTextFit(TextFormat("ADDS 1-%d / %.1fs, CAP %d   AIM >= %.1fs",
                            info.addsPerWave,info.addInterval,info.addCap,
                            info.chargeMin),tx,rowY+MenuPx(b.u,53),b.fsB-1,
                            tw,b.inkDim);
        DrawEnemyIcon(x+w-MenuPx(b.u,22),rowY+rowH/2,t);
    }
    DrawTutorialPageNumber(page,"VI",b);
}

static void DrawTutorialRewardChain(Rectangle panel,bool sideRoom,float t,
                                    TutorialBookStyle b) {
    Color accent=sideRoom?(Color){53,137,127,255}:(Color){124,76,172,255};
    DrawRectangleRounded(panel,0.10f,5,TutorialColorA(b.paperDeep,52));
    int x=(int)panel.x,y=(int)panel.y,w=(int)panel.width;
    int rightX=x+w*52/100;
    DrawTutorialTextFit("EXAMPLE FROM CURRENT ASSIGNMENTS",x+MenuPx(b.u,8),
                        y+MenuPx(b.u,5),b.fsB-2,
                        rightX-x-MenuPx(b.u,15),accent);
    int artifact=TutorialExampleArtifact(sideRoom);
    if(artifact<0)return;
    int keyId=TutorialExampleKey(artifact);
    DrawTutorialTextFit(keyId>=0?UniversalKeyDisplayName(keyId):"NO KEY ASSIGNED",
                        rightX,y+MenuPx(b.u,5),b.fsB-2,
                        x+w-rightX-MenuPx(b.u,8),accent);
    int iconSize=MenuPx(b.u,31);
    DrawArtifactIcon(artifact,(Vector2){(float)(x+MenuPx(b.u,22)),
                                      (float)(y+MenuPx(b.u,43))},
                     (float)iconSize,true);
    int nameX=x+MenuPx(b.u,43);
    DrawTutorialTextFit(ARTIFACT_NAME[artifact],nameX,y+MenuPx(b.u,38),
                        b.fsB,rightX-nameX-MenuPx(b.u,9),b.ink);
    DrawTutorialLockAndKeyIcons(artifact,rightX,y+MenuPx(b.u,24),
                                x+w-rightX-MenuPx(b.u,7),
                                MenuPx(b.u,36),b);
    (void)t;
}

#include "tutorial_room_pages.inc.c"

static void DrawTutorialChallengePage(Rectangle page,TutorialBookStyle b,float t) {
    int x=(int)page.x+MenuPx(b.u,18);
    int w=(int)page.width-MenuPx(b.u,36);
    int y=DrawTutorialPageHeading(page,"CHALLENGES",b);
    DrawTutorialTextFit("THREE ROUNDS / EACH CHEST HAS ONE FIXED KEY",
                        x,y,b.fsB-1,w,b.inkDim);
    y+=b.fsB+MenuPx(b.u,8);
    int bottom=(int)(page.y+page.height-MenuPx(b.u,25));
    int footerH=MenuPx(b.u,65);
    int gap=MenuPx(b.u,7);
    int rowH=(bottom-y-footerH-gap*3)/3;
    for(int stage=1;stage<=3;stage++) {
        int rowY=y+(stage-1)*(rowH+gap);
        Rectangle row={(float)x,(float)rowY,(float)w,(float)rowH};
        DrawRectangleRounded(row,0.10f,5,TutorialColorA(b.paperDeep,43));
        int iconSize=rowH-MenuPx(b.u,12);
        if(iconSize>48)iconSize=48;
        if(iconSize<12)iconSize=12;
        int iconY=rowY+(rowH-iconSize)/2;
        int lockX=x+MenuPx(b.u,38);
        int keyX=x+w-MenuPx(b.u,14)-iconSize;
        DrawText(TextFormat("%d",stage),x+MenuPx(b.u,11),
                 rowY+(rowH-b.fsH)/2,b.fsH,b.rub);
        DrawLockedObjectIcon(ChallengeChestDesignForStage(stage),lockX,iconY,iconSize);
        DrawText(">",x+w/2,rowY+(rowH-b.fsH)/2,b.fsH,b.goldLo);
        DrawUniversalKeyIcon(ChallengeKeyForStage(stage),keyX,iconY,iconSize,t);
        int labelX=lockX+iconSize+MenuPx(b.u,6);
        int labelW=x+w/2-labelX-MenuPx(b.u,4);
        DrawTutorialTextFit(LockedObjectDisplayName(
                             ChallengeChestDesignForStage(stage)),labelX,
                             rowY+(rowH-b.fsB)/2,b.fsB,labelW,b.ink);
        int keyLabelX=x+w/2+MenuPx(b.u,15);
        int keyLabelW=keyX-keyLabelX-MenuPx(b.u,5);
        DrawTutorialTextFit(UniversalKeyDisplayName(
                             ChallengeKeyForStage(stage)),keyLabelX,
                             rowY+(rowH-b.fsB)/2,b.fsB,keyLabelW,b.ink);
    }
    Rectangle footer={(float)x,(float)(bottom-footerH),(float)w,(float)footerH};
    TutorialInfoCard(footer,"HIGH SCORE",
        "Only the best five runs are shown. The framed first place is the best result.",
        b.rub,b);
    DrawTutorialPageNumber(page,"XVII",b);
}

static void DrawTutorialProgressPage(Rectangle page,TutorialBookStyle b) {
    int x=(int)page.x+MenuPx(b.u,18);
    int w=(int)page.width-MenuPx(b.u,36);
    int y=DrawTutorialPageHeading(page,"PROGRESS & SHOP",b);
    DrawTutorialTextFit("WHAT A RUN EARNS AND WHAT STAYS WITH YOU",
                        x,y,b.fsB-1,w,b.inkDim);
    y+=b.fsB+MenuPx(b.u,8);
    const char *titles[]={"COINS","UPGRADES","SKINS & MUSIC","STATS & ACHIEVEMENTS"};
    const char *bodies[]={
        "Collect coins on the floor. A repeated artifact becomes a 100-Coin pile.",
        "Spend coins in the Shop to improve your item options between runs.",
        "Buy a skin, equip it in the Shop and choose the music you want to hear.",
        "Your discoveries, records and achievements remain visible after a run ends."
    };
    Color accents[]={{177,130,35,255},{45,142,128,255},
                     {132,92,177,255},{168,94,73,255}};
    int bottom=(int)(page.y+page.height-MenuPx(b.u,25));
    int gap=MenuPx(b.u,7);
    int rowH=(bottom-y-gap*3)/4;
    for(int i=0;i<4;i++) {
        Rectangle card={(float)x,(float)(y+i*(rowH+gap)),(float)w,(float)rowH};
        TutorialInfoCard(card,titles[i],bodies[i],accents[i],b);
    }
    DrawTutorialPageNumber(page,"XVIII",b);
}

static void DrawTutorialRecordsPage(Rectangle page,TutorialBookStyle b) {
    int x=(int)page.x+MenuPx(b.u,18);
    int w=(int)page.width-MenuPx(b.u,36);
    int y=DrawTutorialPageHeading(page,"RECORDS & DISCOVERIES",b);
    DrawTutorialTextFit("WHAT THE GAME REMEMBERS BETWEEN RUNS",
                        x,y,b.fsB-1,w,b.inkDim);
    y+=b.fsB+MenuPx(b.u,8);
    const char *titles[]={"ARTIFACT COLLECTION","WHERE TO FIND IT",
                          "LOCK DISCOVERIES","CHALLENGE RECORDS"};
    const char *bodies[]={
        "Collected artifacts stay in your collection. Finding one again gives a 100-Coin pile at that reward spot.",
        "Hover an artifact in Stats to see whether it appears in the Artifact Room, Side Room or both.",
        "Previously unseen Locked Objects are preferred when the game chooses an artifact reward. This persists across runs.",
        "Challenges have their own records. Their high score lists show only the best five runs, with first place framed."
    };
    Color accents[]={{128,79,162,255},{57,131,144,255},
                     {165,105,57,255},{76,130,90,255}};
    int bottom=(int)(page.y+page.height-MenuPx(b.u,25));
    int gap=MenuPx(b.u,7);
    int rowH=(bottom-y-gap*3)/4;
    for(int i=0;i<4;i++) {
        Rectangle card={(float)x,(float)(y+i*(rowH+gap)),(float)w,(float)rowH};
        TutorialInfoCard(card,titles[i],bodies[i],accents[i],b);
    }
    DrawTutorialPageNumber(page,"XIX",b);
}

static void DrawTutorialHelpPage(Rectangle page,TutorialBookStyle b) {
    int x=(int)page.x+MenuPx(b.u,18);
    int w=(int)page.width-MenuPx(b.u,36);
    int y=DrawTutorialPageHeading(page,"WHEN YOU GET STUCK",b);
    DrawTutorialTextFit("FIND THE NEXT STEP WITHOUT LEAVING THE BOOK",
                        x,y,b.fsB-1,w,b.inkDim);
    y+=b.fsB+MenuPx(b.u,8);
    const char *titles[]={"THE BOSS EXIT IS MISSING","THE SIDE GATE IS SHUT",
                          "THE ARTIFACT PORTAL IS SEALED","A CHEST WILL NOT OPEN",
                          "THE ARTIFACT IS A COIN PILE"};
    const char *bodies[]={
        "Defeat the boss. Use arena plates and cannons. See pages V-VI.",
        "Find the Jadeeye on the main floor. See pages VII-VIII.",
        "Find a key assigned to Eclipse Iris Seal. Others are not locked. See pages IX-X.",
        "Match the lock image to its key on page XVI. A lock without a key stays sealed.",
        "You already collected it. The pile is worth 100 Coins. See pages XIII-XIV."
    };
    int bottom=(int)(page.y+page.height-MenuPx(b.u,25));
    int gap=MenuPx(b.u,5);
    int rowH=(bottom-y-gap*4)/5;
    for(int i=0;i<5;i++) {
        Rectangle card={(float)x,(float)(y+i*(rowH+gap)),(float)w,(float)rowH};
        TutorialInfoCard(card,titles[i],bodies[i],b.rub,b);
    }
    DrawTutorialPageNumber(page,"XX",b);
}
