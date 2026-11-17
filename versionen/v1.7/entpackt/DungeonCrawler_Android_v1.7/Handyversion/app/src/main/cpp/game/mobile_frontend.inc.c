/* Phone front page and community links. Included from game_ui.c. */

static const char *MOBILE_PROJECT_URL = "https://smstark-projekt.de/en/projects/dungeon-crawler";
static const char *MOBILE_PIPELINE_URL = "https://smstark-projekt.de/en/project-pipeline";
static const char *MOBILE_IDEA_URL = "https://smstark-projekt.de/en/#contact";
static const char *MOBILE_WEBSITE_URL = "https://smstark-projekt.de/en/";

static void FrontCard(Rectangle r, Color ac, bool active) {
    DrawRectangleRounded(r, 0.10f, 8, active ? (Color){35,23,53,250} : (Color){17,13,29,244});
    DrawRectangleRoundedLines(r, 0.10f, 8, WithA(ac, active ? 215 : 90));
}

static void FrontLabel(const char *text, Rectangle r, int fs, Color color) {
    fs = TutorialFitFont(text, fs, (int)r.width-24);
    DrawText(text, (int)(r.x+(r.width-MeasureText(text,fs))*0.5f),
             (int)(r.y+(r.height-fs)*0.5f), fs, color);
}

static void FrontLink(Rectangle r, const char *label, const char *url, Color ac) {
    bool hover = CheckCollisionPointRec(GetMousePosition(), r);
    FrontCard(r, ac, hover);
    FrontLabel(label, r, 22, hover ? WHITE : WithA(ac,255));
    if (!MenuPlayBusy() && MobileTapReleased(r)) OpenURL(url);
}

static void DrawMobileHomeInfo(float t) {
    (void)t;
    MenuUI u=MakeUI();
    Rectangle icons[MN_COUNT];GetMainMenuRects(icons);
    int ts,tx,ty;
    LayoutTitle(u,icons[MN_FUTURE].x+icons[MN_FUTURE].width,&ts,&tx,&ty);
    Rectangle subtitle={0,(float)(ty+ts+MenuPx(u,20)),(float)u.sw,MenuPxf(u,22)};
    FrontLabel("THE SECRET CHAMBERS",subtitle,MenuPx(u,16),(Color){156,145,180,255});
    const char *version="v1.7.6";
    int fs=MenuPx(u,13);
    DrawText(version,u.sw-MeasureText(version,fs)-MenuPx(u,20),MenuPx(u,10),fs,(Color){130,125,165,220});
}

static void FrontSpark(Vector2 p, float radius, Color ac) {
    DrawLineEx((Vector2){p.x-radius,p.y},(Vector2){p.x+radius,p.y},2,ac);
    DrawLineEx((Vector2){p.x,p.y-radius},(Vector2){p.x,p.y+radius},2,ac);
}

static void DrawIdeaAnimation(Rectangle r,float t,Color ac) {
    float cx=r.x+r.width*0.5f,cy=r.y+r.height*0.49f+sinf(t*1.5f)*4;
    float size=fminf(r.height*0.27f,42);
    float pulse=0.5f+0.5f*sinf(t*1.6f);
    DrawCircleGradient((Vector2){cx,cy},size*2.7f,WithA(ac,30+(int)(12*pulse)),WithA(ac,0));
    for(int i=0;i<3;i++){
        float a=t*0.24f+i*2*PI/3;
        Vector2 p={cx+cosf(a)*size*2.0f,cy+sinf(a)*size*1.30f};
        FrontSpark(p,3+1.5f*sinf(t+i),WithA(ac,120));
    }
    DrawCircleV((Vector2){cx,cy-size*0.18f},size,(Color){36,28,55,255});
    DrawCircleLines((int)cx,(int)(cy-size*0.18f),size,ac);
    DrawCircleLines((int)cx,(int)(cy-size*0.18f),size-3,WithA(ac,50));
    DrawLineEx((Vector2){cx-size*0.34f,cy-size*0.16f},(Vector2){cx,cy+size*0.33f},3,ac);
    DrawLineEx((Vector2){cx+size*0.34f,cy-size*0.16f},(Vector2){cx,cy+size*0.33f},3,ac);
    DrawLineEx((Vector2){cx,cy+size*0.33f},(Vector2){cx,cy+size*0.77f},3,ac);
    for(int i=0;i<3;i++)
        DrawRectangleRounded((Rectangle){cx-size*0.46f,cy+size*(0.75f+i*0.16f),size*0.92f,4},0.5f,4,ac);
    for(int i=0;i<7;i++){
        float a=PI+i*PI/6;
        Vector2 p={cx+cosf(a)*size*1.35f,cy-size*0.18f+sinf(a)*size*1.35f};
        Vector2 q={cx+cosf(a)*size*1.55f,cy-size*0.18f+sinf(a)*size*1.55f};
        DrawLineEx(p,q,2,WithA(ac,130+(int)(60*pulse)));
    }
}

static void DrawUpdateAnimation(Rectangle r,int kind,float t,Color ac) {
    float cx=r.x+r.width*0.5f,cy=r.y+r.height*0.5f;
    float s=fminf(r.width,r.height)*0.32f;
    float pulse=0.5f+0.5f*sinf(t*2+kind);
    DrawCircleGradient((Vector2){cx,cy},s*1.65f,WithA(ac,22),WithA(ac,0));
    if(kind==0){
        // A small dungeon bat with gently flapping wings and blinking eyes.
        cy+=sinf(t*1.8f)*s*.08f;
        float wing=s*(.20f+.19f*sinf(t*3.1f));
        for(int side=-1;side<=1;side+=2){
            Vector2 a={cx+side*s*.15f,cy-s*.12f};
            Vector2 b={cx+side*s*1.04f,cy-s*.30f-wing};
            Vector2 c={cx+side*s*.80f,cy+s*.35f};
            Vector2 d={cx+side*s*.38f,cy+s*.18f};
            FillQuad(a,b,c,d,WithA(ac,145));
            DrawLineEx(a,b,2,ac);DrawLineEx(b,c,2,ac);
            DrawLineEx(c,d,2,ac);DrawLineEx(d,a,2,ac);
            DrawLineEx(a,c,1,WithA(ac,160));
        }
        DrawEllipse((int)cx,(int)(cy+s*.10f),s*.26f,s*.43f,ac);
        DrawCircleV((Vector2){cx,cy-s*.18f},s*.28f,ac);
        FillQuad((Vector2){cx-s*.25f,cy-s*.20f},(Vector2){cx-s*.27f,cy-s*.62f},
                 (Vector2){cx-s*.03f,cy-s*.35f},(Vector2){cx-s*.25f,cy-s*.20f},ac);
        FillQuad((Vector2){cx+s*.25f,cy-s*.20f},(Vector2){cx+s*.27f,cy-s*.62f},
                 (Vector2){cx+s*.03f,cy-s*.35f},(Vector2){cx+s*.25f,cy-s*.20f},ac);
        float eye=fmodf(t,4.7f)>.16f?s*.065f:1;
        DrawCircleV((Vector2){cx-s*.12f,cy-s*.19f},eye,(Color){24,18,37,255});
        DrawCircleV((Vector2){cx+s*.12f,cy-s*.19f},eye,(Color){24,18,37,255});
    }else if(kind==1){
        // A miniature room with a travelling spark and a glowing secret door.
        Rectangle room={cx-s,cy-s*.80f,s*2,s*1.60f};
        DrawRectangleRounded(room,.08f,4,(Color){21,28,43,255});
        DrawRectangleRoundedLines(room,.08f,4,ac);
        for(int i=1;i<4;i++){
            DrawLineEx((Vector2){room.x,room.y+i*room.height/4},
                       (Vector2){room.x+room.width,room.y+i*room.height/4},1,WithA(ac,55));
        }
        DrawRectangleRec((Rectangle){cx-s*.22f,cy+s*.22f,s*.44f,s*.58f},WithA(ac,90+(int)(60*pulse)));
        DrawRectangleLinesEx((Rectangle){cx-s*.22f,cy+s*.22f,s*.44f,s*.58f},2,ac);
        float a=t*.65f;
        FrontSpark((Vector2){cx+cosf(a)*s*.65f,cy+sinf(a)*s*.43f},3,ac);
        DrawRectangleRec((Rectangle){cx-s*.67f,cy-s*.47f,s*.32f,s*.32f},WithA(ac,140));
        DrawRectangleRec((Rectangle){cx+s*.35f,cy-s*.47f,s*.32f,s*.32f},WithA(ac,140));
    }else{
        // A floating gem suggests items and other player-submitted features.
        cy+=sinf(t*1.6f)*s*.09f;
        Vector2 a={cx,cy-s*.92f},b={cx-s*.70f,cy-s*.27f};
        Vector2 c={cx,cy+s*.90f},d={cx+s*.70f,cy-s*.27f};
        FillQuad(a,b,c,d,WithA(ac,90));
        DrawLineEx(a,b,2,ac);DrawLineEx(b,c,2,ac);
        DrawLineEx(c,d,2,ac);DrawLineEx(d,a,2,ac);
        DrawLineEx(a,c,1.5f,WithA(ac,190));
        DrawLineEx(b,d,1.5f,WithA(ac,190));
        FrontSpark((Vector2){cx+s*.90f,cy-s*.62f},3+2*pulse,WithA(ac,200));
        FrontSpark((Vector2){cx-s*.85f,cy+s*.56f},3+2*(1-pulse),WithA(ac,150));
    }
}

static void DrawMobileFutureUpdates(float t,int sw,int sh) {
    Color ac={190,132,255,255};
    Rectangle back=DrawSubPanel("FUTURE UPDATES",ac,t,sw,sh);
    if(MobileTapReleased(back)){menuSub=0;return;}
    Rectangle panel=GetMenuSubPanelRect();
    float pad=20,gap=20;
    float top=panel.y+76;
    float bottom=back.y-18;
    float width=panel.width-pad*2;
    float leftW=(width-gap)*.43f;
    Rectangle hero={panel.x+pad,top,leftW,bottom-top};
    FrontCard(hero,ac,false);
    DrawRectangleRounded((Rectangle){hero.x+1,hero.y+1,hero.width-2,4},.5f,4,WithA(ac,160));
    DrawText("HELP SHAPE THE GAME",(int)hero.x+24,(int)hero.y+21,18,ac);
    float artH=fminf(158,hero.height*.35f);
    DrawIdeaAnimation((Rectangle){hero.x+20,hero.y+42,hero.width-40,artH},t,ac);
    float textY=hero.y+artH+49;
    DrawTutorialTextFit("GOT A DUNGEON IDEA?",(int)hero.x+24,(int)textY,29,(int)hero.width-48,WHITE);
    DrawTutorialTextWrapped("New enemies, rooms or features?\nSend your game suggestions through the website.",
        (int)hero.x+24,(int)textY+45,21,(int)hero.width-48,
        (int)(hero.y+hero.height-92-textY-45),7,(Color){179,170,204,255});
    Rectangle submit={hero.x+22,hero.y+hero.height-78,hero.width-44,56};
    FrontLink(submit,"SUBMIT AN IDEA  >",MOBILE_IDEA_URL,ac);

    float rightX=hero.x+hero.width+gap,rightW=panel.x+panel.width-pad-rightX;
    DrawText("IDEAS WELCOME",(int)rightX+2,(int)top+3,19,(Color){185,176,208,255});
    float rowTop=top+36,rowGap=12,rowH=(bottom-rowTop-rowGap*2)/3;
    const char *names[]={"ENEMIES","ROOMS","FEATURES"};
    const char *details[]={
        "Which creatures should lurk in the dungeon?",
        "Traps, secrets or a completely new room.",
        "Items, skins or a new way to play."
    };
    Color colors[]={{111,211,178,255},{113,182,255,255},{220,166,255,255}};
    for(int i=0;i<3;i++){
        Rectangle row={rightX,rowTop+i*(rowH+rowGap),rightW,rowH};
        FrontCard(row,colors[i],false);
        float iconW=fminf(row.height-16,100);
        DrawUpdateAnimation((Rectangle){row.x+12,row.y+8,iconW,row.height-16},i,t,colors[i]);
        float textX=row.x+iconW+30;
        float textW=row.width-iconW-50;
        DrawTutorialTextFit(names[i],(int)textX,(int)row.y+17,24,(int)textW,colors[i]);
        DrawTutorialTextWrapped(details[i],(int)textX,(int)row.y+52,20,
                                (int)textW,(int)row.height-60,5,(Color){180,177,198,255});
    }
    float linksX=back.x+back.width+22;
    float linkGap=12,linkW=(panel.x+panel.width-pad-linksX-linkGap*2)/3;
    FrontLink((Rectangle){linksX,back.y,linkW,back.height},"PROJECT  >",MOBILE_PROJECT_URL,(Color){113,182,255,255});
    FrontLink((Rectangle){linksX+linkW+linkGap,back.y,linkW,back.height},"PIPELINE  >",MOBILE_PIPELINE_URL,ac);
    FrontLink((Rectangle){linksX+2*(linkW+linkGap),back.y,linkW,back.height},"WEBSITE  >",MOBILE_WEBSITE_URL,(Color){111,211,178,255});
}
