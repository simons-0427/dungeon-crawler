// Room spreads built on the isolated guide maps in tutorial_book_maps.inc.c.

static void DrawTutorialStrategyRow(Rectangle row,const char *title,
                                    const char *body,TutorialBookStyle b) {
    DrawRectangleRounded(row,0.10f,4,TutorialColorA(b.paperDeep,42));
    int x=(int)row.x+MenuPx(b.u,8);
    int labelW=MenuPx(b.u,116);
    DrawTutorialTextFit(title,x,(int)row.y+MenuPx(b.u,8),
                        b.fsB,labelW,b.rub);
    int textX=x+labelW+MenuPx(b.u,5);
    DrawTutorialTextFit(body,textX,(int)row.y+MenuPx(b.u,8),
                        b.fsB-1,(int)(row.x+row.width)-textX-MenuPx(b.u,7),
                        b.ink);
}

static void DrawTutorialBossArenaPage(Rectangle page,TutorialBookStyle b,float t) {
    int x=(int)page.x+MenuPx(b.u,18);
    int w=(int)page.width-MenuPx(b.u,36);
    int y=DrawTutorialPageHeading(page,"BOSS ROOM",b);
    DrawTutorialTextFit("EXACT FIXED ARENA / RANDOM COINS AND ITEMS OMITTED",
                        x,y,b.fsB-1,w,b.inkDim);
    y+=b.fsB+MenuPx(b.u,6);
    int bottom=(int)(page.y+page.height-MenuPx(b.u,25));
    int panelH=(bottom-y)*68/100;
    DrawTutorialGuideMap((Rectangle){(float)x,(float)y,(float)w,(float)panelH},
                         0,t,b);
    y+=panelH+MenuPx(b.u,5);
    const char *titles[]={"1 / ARM PLATES","2 / FIRE CANNONS","3 / DODGE & HIT"};
    const char *bodies[]={
        "Step on silver plates, then lure the boss across them.",
        "Stand on a gold cannon when ready; each reloads in 30s.",
        "Dodge, then use sword or bomb in its 1s rest; shield contact hurts too."
    };
    int gap=MenuPx(b.u,4);
    int rowH=(bottom-y-gap*2)/3;
    for(int i=0;i<3;i++)
        DrawTutorialStrategyRow((Rectangle){(float)x,
                                (float)(y+i*(rowH+gap)),(float)w,(float)rowH},
                                titles[i],bodies[i],b);
    DrawTutorialPageNumber(page,"V",b);
}

static void DrawTutorialFullMapPage(Rectangle page,TutorialBookStyle b,float t,
                                    int kind,const char *number) {
    const char *title=kind==1?"SIDE ROOM / MAP":
                      kind==2?"ARTIFACT ROOM / MAP":"ARTIFACT VAULT / MAP";
    const char *subtitle=kind==1?"MAIN FLOOR > NARROW ENTRANCE > MAZE > REWARD":
                         kind==2?"OPEN FIRST PART / ENEMIES / LADDER":
                                 "SECOND PART / MAZE / REWARD / EXIT";
    const char *note=kind==1?
        "SAMPLE MAZE; THE GATE APPEARS IN HALF OF SIDE ROOMS.":
        kind==2?
        "SAMPLE WALLS; SPAWN IS ABOVE, LADDER AND ITEMS BELOW.":
        "SAMPLE MAZE; TREASURE APPEARS AFTER THE REWARD.";
    int x=(int)page.x+MenuPx(b.u,18);
    int w=(int)page.width-MenuPx(b.u,36);
    int y=DrawTutorialPageHeading(page,title,b);
    DrawTutorialTextFit(subtitle,x,y,b.fsB-1,w,b.inkDim);
    y+=b.fsB+MenuPx(b.u,6);
    int bottom=(int)(page.y+page.height-MenuPx(b.u,25));
    int noteH=MenuPx(b.u,16);
    int mapH=bottom-y-noteH-MenuPx(b.u,3);
    DrawTutorialGuideMap((Rectangle){(float)x,(float)y,(float)w,(float)mapH},
                         kind,t,b);
    DrawTutorialTextFit(note,x,bottom-noteH,b.fsB-2,w,b.inkDim);
    DrawTutorialPageNumber(page,number,b);
}

static void DrawTutorialGateStrip(Rectangle panel,float t,TutorialBookStyle b) {
    DrawRectangleRounded(panel,0.10f,5,TutorialColorA(b.paperDeep,48));
    int x=(int)panel.x,y=(int)panel.y,w=(int)panel.width;
    DrawTutorialTextFit("MAIN FLOOR KEY",x+MenuPx(b.u,8),y+MenuPx(b.u,5),
                        b.fsB-2,w/3-MenuPx(b.u,10),b.rub);
    DrawTutorialTextFit("OPTIONAL GATE",x+w/3+MenuPx(b.u,8),
                        y+MenuPx(b.u,5),b.fsB-2,w/3-MenuPx(b.u,12),b.rub);
    DrawTutorialTextFit("REWARD LOCK > KEY",x+w*2/3+MenuPx(b.u,8),
                        y+MenuPx(b.u,5),b.fsB-2,w/3-MenuPx(b.u,14),b.rub);
    int size=MenuPx(b.u,37);
    DrawUniversalKeyIcon(KEY_JADEEYE,x+w/6-size/2,
                         y+MenuPx(b.u,27),size,t);
    int gateX=x+w/2,gateY=y+MenuPx(b.u,45);
    Rectangle gate={(float)(gateX-size/2),(float)(gateY-size/2),
                    (float)size,(float)size};
    DrawRectangleRounded(gate,0.12f,4,(Color){29,38,51,255});
    DrawRectangleRoundedLines(gate,0.12f,4,(Color){199,172,108,255});
    for(int i=-1;i<=1;i++)
        DrawLine(gateX+i*size/4,gateY-size/3,gateX+i*size/4,
                 gateY+size/3,(Color){171,144,91,255});
    int artifact=TutorialExampleArtifact(true);
    if(artifact>=0)
        DrawTutorialLockAndKeyIcons(artifact,x+w*2/3+MenuPx(b.u,5),
                                    y+MenuPx(b.u,27),w/3-MenuPx(b.u,12),
                                    size,b);
    DrawText(">",x+w/3-MenuPx(b.u,7),gateY-MenuPx(b.u,7),b.fsH,b.inkDim);
    DrawText(">",x+w*2/3-MenuPx(b.u,7),gateY-MenuPx(b.u,7),b.fsH,b.inkDim);
}

static void DrawTutorialSideGuidePage(Rectangle page,TutorialBookStyle b,float t) {
    int x=(int)page.x+MenuPx(b.u,18);
    int w=(int)page.width-MenuPx(b.u,36);
    int y=DrawTutorialPageHeading(page,"INSIDE THE SIDE ROOM",b);
    DrawTutorialTextFit("HOW THE ENTRANCE, MAZE AND REWARD WORK",
                        x,y,b.fsB-1,w,b.inkDim);
    y+=b.fsB+MenuPx(b.u,6);
    int bottom=(int)(page.y+page.height-MenuPx(b.u,25));
    int stripH=MenuPx(b.u,72),chainH=MenuPx(b.u,68),gap=MenuPx(b.u,5);
    DrawTutorialGateStrip((Rectangle){(float)x,(float)y,(float)w,(float)stripH},t,b);
    y+=stripH+gap;
    const char *titles[]={"WHEN IT APPEARS","THE ROUTE","THE REWARD"};
    const char *bodies[]={
        "A normal round has a 2% Side Room chance. Half the entrances have a gate; its Jadeeye lies on the main floor.",
        "A 5 to 8 tile corridor leads to a 38 to 49 tile wide maze. Expect lava, spikes, trapdoors and timed arrow lanes, but no initial enemy wave.",
        "The reward lies far into the lower half. Its lock is chosen from the artifact's assignments; a matching key, if assigned, appears inside."
    };
    int cardH=(bottom-y-chainH-gap*3)/3;
    for(int i=0;i<3;i++)
        TutorialInfoCard((Rectangle){(float)x,(float)(y+i*(cardH+gap)),
                         (float)w,(float)cardH},titles[i],bodies[i],
                         (Color){53,137,127,255},b);
    DrawTutorialRewardChain((Rectangle){(float)x,(float)(bottom-chainH),
                            (float)w,(float)chainH},true,t,b);
    DrawTutorialPageNumber(page,"VIII",b);
}

static void DrawTutorialArtifactFlow(Rectangle panel,float t,TutorialBookStyle b) {
    DrawRectangleRounded(panel,0.10f,5,TutorialColorA(b.paperDeep,48));
    int x=(int)panel.x,y=(int)panel.y,w=(int)panel.width;
    const char *labels[]={"PORTAL SEAL","FAST WISPS","LADDER SUPPLIES"};
    for(int i=0;i<3;i++)
        DrawTutorialTextFit(labels[i],x+i*w/3+MenuPx(b.u,8),
                            y+MenuPx(b.u,6),b.fsB-2,
                            w/3-MenuPx(b.u,16),b.rub);
    int size=MenuPx(b.u,43);
    DrawLockedObjectIcon(PORTAL_ECLIPSE_IRIS_SEAL_DESIGN_ID,
                         x+w/6-size/2,y+MenuPx(b.u,28),size);
    DrawAzureWispPreview(x+w/2,y+MenuPx(b.u,49),t);
    int tileY=y+MenuPx(b.u,30);
    DrawTutorialMapSymbol((Rectangle){(float)(x+w*2/3+MenuPx(b.u,12)),
                                     (float)tileY,32.0f,32.0f},'I',IK_SWD,t);
    DrawTutorialMapSymbol((Rectangle){(float)(x+w*2/3+MenuPx(b.u,50)),
                                     (float)tileY,32.0f,32.0f},'I',IK_SWD,t);
    DrawTutorialMapSymbol((Rectangle){(float)(x+w*2/3+MenuPx(b.u,88)),
                                     (float)tileY,32.0f,32.0f},'I',IK_SHD,t);
    DrawText(">",x+w/3-MenuPx(b.u,7),tileY+MenuPx(b.u,8),b.fsH,b.inkDim);
    DrawText(">",x+w*2/3-MenuPx(b.u,7),tileY+MenuPx(b.u,8),b.fsH,b.inkDim);
}

static void DrawTutorialArtifactGuidePage(Rectangle page,TutorialBookStyle b,float t) {
    int x=(int)page.x+MenuPx(b.u,18);
    int w=(int)page.width-MenuPx(b.u,36);
    int y=DrawTutorialPageHeading(page,"FIRST ARTIFACT ROOM",b);
    DrawTutorialTextFit("PORTAL > WISPS > LADDER > VAULT",
                        x,y,b.fsB-1,w,b.inkDim);
    y+=b.fsB+MenuPx(b.u,6);
    int bottom=(int)(page.y+page.height-MenuPx(b.u,25));
    int stripH=MenuPx(b.u,84),gap=MenuPx(b.u,5);
    DrawTutorialArtifactFlow((Rectangle){(float)x,(float)y,(float)w,(float)stripH},t,b);
    y+=stripH+gap;
    const char *titles[]={"FIND A PORTAL","ENTER THE FIRST PART",
                          "SURVIVE THE WISPS","TAKE THE LADDER"};
    const char *bodies[]={
        "Normal, Trap, Dark and Lava rounds have a 2% portal chance. Half the portals use Eclipse Iris Seal. Others are not locked.",
        "The room is 55 to 74 by 37 to 48 tiles. Its mostly open floor gets scattered walls, lava and spike traps. A path to the ladder is ensured.",
        "The game aims to place 29 to 32 Azure Wisps, moving every 0.42 to 0.55 seconds. Clear space around the spawn and ladder gives you a start.",
        "Two swords and one shield wait beside the ladder. If a reward key appears, collect it before descending; a missed one returns below."
    };
    int cardH=(bottom-y-gap*3)/4;
    for(int i=0;i<4;i++)
        TutorialInfoCard((Rectangle){(float)x,(float)(y+i*(cardH+gap)),
                         (float)w,(float)cardH},titles[i],bodies[i],
                         (Color){124,76,172,255},b);
    DrawTutorialPageNumber(page,"X",b);
}

static void DrawTutorialVaultGuidePage(Rectangle page,TutorialBookStyle b,float t) {
    int x=(int)page.x+MenuPx(b.u,18);
    int w=(int)page.width-MenuPx(b.u,36);
    int y=DrawTutorialPageHeading(page,"THE ARTIFACT VAULT",b);
    DrawTutorialTextFit("LADDER > MAZE > REWARD > EXIT TREASURE",
                        x,y,b.fsB-1,w,b.inkDim);
    y+=b.fsB+MenuPx(b.u,6);
    int bottom=(int)(page.y+page.height-MenuPx(b.u,25));
    int gap=MenuPx(b.u,5),chainH=MenuPx(b.u,68);
    const char *titles[]={"THE LOWER MAZE","THE SECOND WAVE",
                          "MISSED KEY RETURNS","CLAIM THE REWARD"};
    const char *bodies[]={
        "The vault is 43 to 64 by 31 to 44 tiles. Its maze has extra openings and chambers. Lava and spikes fill some of the free paths.",
        "Up to 16 Azure Wisps begin in the maze. Each moves once every 0.55 seconds; use walls and turns to separate them.",
        "If the artifact's key was available in part one and you missed it, another appears in this vault. Only a key assigned to its lock opens it.",
        "The reward is far from the spawn. An artifact you already own becomes a 100-Coin pile. The exit Treasure appears only after you claim it."
    };
    int cardH=(bottom-y-chainH-gap*4)/4;
    for(int i=0;i<4;i++)
        TutorialInfoCard((Rectangle){(float)x,(float)(y+i*(cardH+gap)),
                         (float)w,(float)cardH},titles[i],bodies[i],
                         (Color){124,76,172,255},b);
    DrawTutorialRewardChain((Rectangle){(float)x,(float)(bottom-chainH),
                            (float)w,(float)chainH},false,t,b);
    DrawTutorialPageNumber(page,"XII",b);
}
