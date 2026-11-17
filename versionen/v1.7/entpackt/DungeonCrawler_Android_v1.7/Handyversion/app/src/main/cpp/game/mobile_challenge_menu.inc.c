extern float __real_GetMouseWheelMove(void);
// Mobile landscape layout. Progress storage and challenge generation stay shared.
static int ChFit(const char *text,int font,int width){
    while(font>16&&MeasureText(text,font)>width)font--;
    return font;
}
static void ChPanel(Rectangle r,Color accent){
    DrawRectangleRounded(r,.06f,8,CC(22,21,35));
    DrawRectangleRoundedLinesEx(r,.06f,8,1,(Color){accent.r,accent.g,accent.b,150});
}
static void ChScrollIndicator(Rectangle view,float scroll,float total,Color accent){
    if(total<=view.height)return;
    float height=fmaxf(32,view.height*view.height/total);
    float y=view.y+(view.height-height)*scroll/(total-view.height);
    DrawRectangleRounded((Rectangle){view.x+view.width-5,view.y,5,view.height},1,4,CC(43,39,57));
    DrawRectangleRounded((Rectangle){view.x+view.width-5,y,5,height},1,4,accent);
}

void ChallengeDrawMenu(Rectangle panel,float t){
    (void)t;
    if(MobilePollSeedInput(customSeed,sizeof(customSeed))){seedMode=1;customFocus=0;}
    ChallengeLoadData();
    float x=panel.x+20,w=panel.width-40;
    float tabY=panel.y+68,top=tabY+68,bottom=panel.y+panel.height-70;
    float gap=18;
    if(IsKeyPressed(KEY_TAB)){menuTab=1-menuTab;MenuNavExit();}
    Rectangle tabs[2]={{x,tabY,(w-12)/2,52},{x+(w+12)/2,tabY,(w-12)/2,52}};
    if(Button(tabs[0],menuTab==0?"CHALLENGES  *":"CHALLENGES",CC(230,126,190),1)){
        menuTab=0;customFocus=0;MenuNavExit();
    }
    if(Button(tabs[1],menuTab==1?"SEEDED RUNS  *":"SEEDED RUNS",CC(109,216,221),1)){
        menuTab=1;customFocus=0;MenuNavExit();
    }
    if(menuTab==0){
        uint32_t seed=ActiveSeed();
        if(previewPick!=menuPick||previewSeed!=seed){
            BuildTopology(menuPick,seed,1);memcpy(preview,tiles,sizeof(preview));
            previewPick=menuPick;previewSeed=seed;
        }
        float lw=fmaxf(300,w*.34f),rx=x+lw+gap,rw=w-lw-gap;
        Rectangle list={x,top,lw,bottom-top};
        const int cardH=102,cardGap=12;
        float total=CH_COUNT*(cardH+cardGap)-cardGap;
        float maxScroll=fmaxf(0,total-list.height);
        Vector2 drag=MobileMenuDragDelta(list);
        challengeScroll-=(int)drag.y;
        if(CheckCollisionPointRec(GetMousePosition(),list)&&drag.y==0)
            challengeScroll-=(int)(__real_GetMouseWheelMove()*72);
        if(IsKeyPressed(KEY_PAGE_DOWN))challengeScroll+=(int)list.height-cardH;
        if(IsKeyPressed(KEY_PAGE_UP))challengeScroll-=(int)list.height-cardH;
        if(challengeScroll<0)challengeScroll=0;if(challengeScroll>maxScroll)challengeScroll=(int)maxScroll;
        BeginScissorMode((int)list.x,(int)list.y,(int)list.width,(int)list.height);
        for(int i=0;i<CH_COUNT;i++){
            Rectangle card={x,top+i*(cardH+cardGap)-challengeScroll,lw-14,cardH};
            if(card.y+card.height<list.y||card.y>bottom)continue;
            Color a=specs[i].accent;
            DrawRectangleRounded(card,.10f,8,menuPick==i?CC(43,34,55):CC(25,23,38));
            DrawRectangleRoundedLinesEx(card,.10f,8,menuPick==i?2:1,
                menuPick==i?a:(Color){a.r,a.g,a.b,75});
            DrawText(TextFormat("%02d",i+1),(int)card.x+14,(int)card.y+17,24,a);
            int nameFs=ChFit(specs[i].name,22,(int)card.width-78);
            DrawText(specs[i].name,(int)card.x+65,(int)card.y+16,nameFs,CC(241,235,239));
            DrawText(specs[i].tag,(int)card.x+65,(int)card.y+46,17,a);
            const char *status=challengeStats[i].completions?"COMPLETED":"NOT COMPLETED";
            DrawText(status,(int)card.x+65,(int)card.y+76,16,
                challengeStats[i].completions?CC(142,220,160):CC(153,147,170));
            if(MobileTapReleased(card)&&CheckCollisionPointRec(GetMousePosition(),list)){
                menuPick=i;previewPick=-1;
            }
        }
        EndScissorMode();ChScrollIndicator(list,challengeScroll,total,specs[menuPick].accent);
        Rectangle detail={rx,top,rw,bottom-top};Color a=specs[menuPick].accent;ChPanel(detail,a);
        int dx=(int)rx+20,dy=(int)top+18;
        DrawText(specs[menuPick].name,dx,dy,ChFit(specs[menuPick].name,27,(int)rw-40),CC(248,235,222));
        DrawText(specs[menuPick].tag,dx,dy+34,18,a);
        ChallengeProgress *p=&challengeStats[menuPick];char best[24];
        if(p->timeCount)FormatRunTime(p->scores[0].timeMs,best,sizeof(best));else snprintf(best,sizeof(best),"--:--");
        DrawText(TextFormat("CLEARS %d    ATTEMPTS %d    BEST %s",p->completions,p->attempts,best),dx,dy+67,20,CC(201,192,215));
        int bodyY=dy+112,bodyBottom=(int)bottom-88;
        int scale=(int)fminf((rw-64)*.46f/CH_W,(float)(bodyBottom-bodyY)/CH_H);
        if(scale<1)scale=1;if(scale>9)scale=9;
        DrawText("MAP PREVIEW",dx,bodyY-24,18,a);
        DrawMenuMap(dx,bodyY,scale);
        DrawRectangleLines(dx-2,bodyY-2,CH_W*scale+4,CH_H*scale+4,a);
        int sx=dx+CH_W*scale+24,scoreW=(int)(rx+rw-20)-sx;
        DrawText("TOP 5 RUNS",sx,bodyY-24,20,a);
        int colTime=sx+32,colCoins=sx+scoreW*40/100,colTrs=sx+scoreW*62/100,colKills=sx+scoreW*82/100;
        DrawText("TIME",colTime,bodyY+5,16,CC(158,153,178));
        DrawText("COINS",colCoins,bodyY+5,16,CC(158,153,178));
        DrawText("TRS",colTrs,bodyY+5,16,CC(158,153,178));
        DrawText("KILLS",colKills,bodyY+5,16,CC(158,153,178));
        int rows=p->timeCount;if(rows>5)rows=5;
        for(int row=0;row<rows;row++){
            ChallengeScore *s=&p->scores[row];int yy=bodyY+34+row*34;
            DrawRectangle(sx,yy-3,scoreW,31,row==0?CC(52,39,57):CC(30,27,43));
            DrawText(TextFormat("%d",row+1),sx+4,yy,18,a);
            char time[24];FormatRunTime(s->timeMs,time,sizeof(time));
            DrawText(time,colTime,yy,18,CC(241,235,225));
            DrawText(s->hasRunStats?TextFormat("%d",s->coins):"--",colCoins,yy,18,CC(241,235,225));
            DrawText(s->hasRunStats?TextFormat("%d",s->treasures):"--",colTrs,yy,18,CC(241,235,225));
            DrawText(s->hasRunStats?TextFormat("%d",s->kills):"--",colKills,yy,18,CC(241,235,225));
        }
        if(!rows)DrawText("NO RUNS YET",sx,bodyY+42,20,CC(143,136,161));
        Rectangle start={rx+20,bottom-72,rw-40,56};
        if(Button(start,"START CHALLENGE",a,1))StartChallenge();
    }else{
        Color a=CC(109,216,221);float lw=w*.47f,rx=x+lw+gap,rw=w-lw-gap;
        Rectangle setup={x,top,lw,bottom-top},records={rx,top,rw,bottom-top};ChPanel(setup,a);ChPanel(records,a);
        int dx=(int)x+22,dy=(int)top+18,day,month,year;uint32_t daily=DailySeed(&day,&month,&year);
        DrawText("CHOOSE YOUR SEED",dx,dy,26,CC(235,247,244));
        float modeW=(lw-56)/2;
        Rectangle dailyButton={x+22,top+65,modeW,52},customButton={x+34+modeW,top+65,modeW,52};
        if(Button(dailyButton,seedMode?"DAILY":"DAILY  *",CC(237,186,110),1)){seedMode=0;customFocus=0;}
        if(Button(customButton,seedMode?"CUSTOM  *":"CUSTOM",a,1)){
            seedMode=1;customFocus=0;MobileRequestSeedInput(customSeed);
        }
        if(!seedMode){
            DrawText("TODAY'S RUN",dx,(int)top+143,20,CC(218,181,112));
            DrawText(TextFormat("%02d.%02d.%04d",day,month,year),dx,(int)top+176,32,CC(255,220,144));
            DrawText(TextFormat("SEED %u",daily),dx,(int)top+225,24,CC(229,205,164));

        }else{
            DrawText("YOUR SEED",dx,(int)top+141,18,a);
            Rectangle entry={x+22,top+175,lw-44,62};
            DrawRectangleRounded(entry,.16f,6,CC(11,13,26));DrawRectangleRoundedLinesEx(entry,.16f,6,2,a);
            const char *text=customSeed[0]?customSeed:"Enter a number or text seed";
            DrawText(text,dx+14,(int)entry.y+20,ChFit(text,23,(int)entry.width-28),customSeed[0]?CC(231,238,239):CC(126,137,155));
            if(MobileTapReleased(entry)){customFocus=0;MobileRequestSeedInput(customSeed);}
            if(customSeed[0])DrawText(TextFormat("USED SEED %u",ActiveSeed()),dx,(int)top+264,22,a);
        }
        Rectangle start={x+22,bottom-72,lw-44,56};
        if(Button(start,"START SEEDED RUN",a,!seedMode||customSeed[0]))StartChallenge();
        DrawText("YOUR SAVED RUNS",(int)rx+22,dy,26,CC(235,247,244));

        Rectangle list={rx+18,top+64,rw-36,bottom-top-82};
        const int rowH=92,rowGap=10;float total=(float)seedProgressCount*(rowH+rowGap)-rowGap;
        if(total<0)total=0;
        static float seedPixels;
        Vector2 drag=MobileMenuDragDelta(list);seedPixels-=drag.y;
        if(CheckCollisionPointRec(GetMousePosition(),list)&&drag.y==0)seedPixels-=__real_GetMouseWheelMove()*70;
        float max=fmaxf(0,total-list.height);if(seedPixels<0)seedPixels=0;if(seedPixels>max)seedPixels=max;
        BeginScissorMode((int)list.x,(int)list.y,(int)list.width,(int)list.height);
        for(size_t i=0;i<seedProgressCount;i++){
            Rectangle row={list.x,list.y+i*(rowH+rowGap)-seedPixels,list.width-12,rowH};
            if(row.y+row.height<list.y||row.y>list.y+list.height)continue;
            SeedProgress *p=&seedScores[i];
            DrawRectangleRounded(row,.12f,6,CC(29,33,48));
            int xx=(int)row.x+14,yy=(int)row.y+12;
            DrawText(TextFormat("SEED %u",p->seed),xx,yy,23,CC(231,235,241));
            const char *source=(p->sourceFlags&SEED_SOURCE_DAILY)?"DAILY":"CUSTOM";
            DrawText(source,(int)(row.x+row.width)-14-MeasureText(source,16),yy+3,16,a);
            DrawText(TextFormat("ROUND %d   COINS %d   TREASURES %d",p->bestRound,p->bestCoins,p->bestTreasures),xx,yy+31,18,CC(190,190,207));
            char time[24];if(p->bestTimeMs>0)FormatRunTime(p->bestTimeMs,time,sizeof(time));else snprintf(time,sizeof(time),"--:--");
            DrawText(TextFormat("BEST %s    KILLS %d",time,p->bestKills),xx,yy+58,17,CC(151,163,185));
            if(MobileTapReleased(row)&&CheckCollisionPointRec(GetMousePosition(),list)){
                snprintf(customSeed,sizeof(customSeed),"%u",p->seed);seedMode=1;customFocus=0;
            }
        }
        if(!seedProgressCount)DrawText("NO SAVED RUNS",(int)list.x+8,(int)list.y+18,20,CC(142,157,178));
        EndScissorMode();ChScrollIndicator(list,seedPixels,total,a);
    }
}
