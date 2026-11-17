static Rectangle musicCarouselViewport;
static float musicCarouselOffset;
extern float __real_GetMouseWheelMove(void);

static Rectangle MusicClip(Rectangle r){
    float x=fmaxf(r.x,musicCarouselViewport.x),y=fmaxf(r.y,musicCarouselViewport.y);
    return (Rectangle){x,y,fmaxf(0,fminf(r.x+r.width,musicCarouselViewport.x+musicCarouselViewport.width)-x),
        fmaxf(0,fminf(r.y+r.height,musicCarouselViewport.y+musicCarouselViewport.height)-y)};
}
static bool DrawSoundtrackCard(Rectangle r,int track,bool selected,bool previewPlaying,
                               bool unlocked,MusicAccess access,int price,bool *previewClick,float t){
    Color ac=MusicTrackAccent(track);bool locked=track>=0&&!unlocked;
    Vector2 mp=GetMousePosition();
    DrawRectangleRounded(r,.10f,8,selected?(Color){19,31,44,250}:(Color){15,14,28,245});
    DrawRectangleRoundedLinesEx(r,.10f,8,selected?2:1,
        (Color){ac.r,ac.g,ac.b,selected?245:120});
    Rectangle art={r.x+16,r.y+16,r.width-32,r.height-210};
    Rectangle clip=MusicClip(art);
    BeginScissorMode((int)clip.x,(int)clip.y,(int)clip.width,(int)clip.height);
    DrawMusicArtwork(art,track,t,selected);
    EndScissorMode();
    BeginScissorMode((int)musicCarouselViewport.x,(int)musicCarouselViewport.y,
        (int)musicCarouselViewport.width,(int)musicCarouselViewport.height);
    if(locked){
        DrawRectangleRounded(art,.1f,6,(Color){6,6,17,170});
        DrawText("LOCKED",(int)(art.x+(art.width-MeasureText("LOCKED",22))/2),
            (int)(art.y+(art.height-22)/2),22,(Color){230,214,235,255});
    }
    const char *title=track<0?"NO MUSIC":GetMusicTrackName(track);int fs=23;
    while(fs>17&&MeasureText(title,fs)>r.width-32)fs--;
    DrawText(title,(int)r.x+16,(int)(art.y+art.height+12),fs,(Color){241,236,248,255});
    const char *detail=track<0?"SILENT SLOT":unlocked?"ORIGINAL SCORE":
        access==MUSIC_ACCESS_PURCHASE?TextFormat("%d COINS",price):GetMusicTrackAccessLabel(track);
    int detailFs=17;while(detailFs>14&&MeasureText(detail,detailFs)>r.width-32)detailFs--;
    DrawText(detail,(int)r.x+16,(int)(art.y+art.height+43),detailFs,
        locked?(Color){219,157,180,245}:(Color){166,160,192,240});
    bool playPressed=false;
    if(track>=0){
        Rectangle play={r.x+16,r.y+r.height-128,96,48};
        DrawRectangleRounded(play,.24f,8,previewPlaying?(Color){16,68,58,255}:(Color){27,30,47,255});
        DrawRectangleRoundedLinesEx(play,.24f,8,1,ac);
        float cx=play.x+play.width/2,cy=play.y+play.height/2;
        if(previewPlaying){DrawRectangle((int)cx-9,(int)cy-11,6,22,ac);DrawRectangle((int)cx+3,(int)cy-11,6,22,ac);}
        else DrawPoly((Vector2){cx,cy},3,15,0,ac);
        playPressed=MobileTapReleased(play)&&CheckCollisionPointRec(mp,musicCarouselViewport);
        float duration=GetMusicTrackLength(track),played=GetSoundtrackPreviewTimePlayed(track);
        const char *time=duration>0?TextFormat("%d:%02d",(int)duration/60,(int)duration%60):"--:--";
        DrawText(time,(int)play.x+114,(int)play.y+14,20,(Color){177,197,211,245});
        if(previewPlaying){
            Rectangle bar={r.x+16,r.y+r.height-72,r.width-32,8};
            Rectangle hit={bar.x,bar.y-8,bar.width,24};
            if(MobileMenuPressStarted(hit))musicPreviewScrubTrack=track;
            if(musicPreviewScrubTrack==track){
                if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
                    float fraction=fmaxf(0,fminf(1,(mp.x-bar.x)/bar.width));
                    SeekSoundtrackPreview(track,duration*fraction);
                }else musicPreviewScrubTrack=-1;
            }
            float fraction=duration>0?fmaxf(0,fminf(1,played/duration)):0;
            DrawRectangleRounded(bar,1,4,(Color){45,44,64,255});
            if(fraction>0)DrawRectangleRounded((Rectangle){bar.x,bar.y,bar.width*fraction,bar.height},1,4,ac);
            DrawCircle((int)(bar.x+bar.width*fraction),(int)bar.y+4,6,WHITE);
        }
    }
    if(previewClick)*previewClick=playPressed;
    Rectangle equip={r.x+16,r.y+r.height-52,r.width-32,40};
    DrawRectangleRounded(equip,.24f,8,selected?(Color){ac.r/5,ac.g/5,ac.b/5,255}:(Color){25,24,40,255});
    DrawRectangleRoundedLinesEx(equip,.24f,8,1,ac);
    const char *label=selected?"SELECTED":locked?
        (access==MUSIC_ACCESS_PURCHASE?(coin_balance>=price?TextFormat("BUY %d",price):"NEED COINS"):"LOCKED"):"SELECT";
    DrawText(label,(int)(equip.x+(equip.width-MeasureText(label,20))/2),(int)equip.y+10,20,selected?WHITE:ac);
    return MobileTapReleased(equip)&&CheckCollisionPointRec(mp,musicCarouselViewport);
}

static bool MusicMobileButton(Rectangle r,const char *text,Color ac,bool selected){
    DrawRectangleRounded(r,.24f,8,selected?(Color){18,53,64,255}:(Color){20,21,37,245});
    DrawRectangleRoundedLinesEx(r,.24f,8,selected?2:1,ac);
    int fs=21;while(fs>16&&MeasureText(text,fs)>r.width-20)fs--;
    DrawText(text,(int)(r.x+(r.width-MeasureText(text,fs))/2),(int)(r.y+(r.height-fs)/2),fs,selected?WHITE:ac);
    return MobileTapReleased(r);
}

static void DrawMobileSoundtrackMenu(float t,int sw,int sh){
    (void)sw;(void)sh;
    Color ac={95,205,240,255};DrawSubPanel("SOUNDTRACK SHOP",ac,t,GetScreenWidth(),GetScreenHeight());
    Rectangle panel=GetMenuSubPanelRect();float x=panel.x+20,w=panel.width-40,y=panel.y+72;
    float resetW=220,gap=12,tabW=(w-resetW-2*gap)/2;
    bool save=false;
    if(MusicMobileButton((Rectangle){x,y,tabW,50},"HOME MENU",ac,musicShopContext==0))musicShopContext=0;
    if(MusicMobileButton((Rectangle){x+tabW+gap,y,tabW,50},"GAMEPLAY",ac,musicShopContext==1))musicShopContext=1;
    if(MusicMobileButton((Rectangle){x+w-resetW,y,resetW,50},"RESET DEFAULTS",(Color){185,154,230,255},false)){
        music_menu_tracks=MUSIC_DEFAULT_MENU_MASK;music_gameplay_tracks=MUSIC_DEFAULT_GAMEPLAY_MASK;save=true;
    }
    musicCarouselViewport=(Rectangle){x,y+68,w,panel.y+panel.height-108-(y+68)};
    int count=GetMusicTrackCount()+1;float cardW=310,cardGap=20;
    float cardH=musicCarouselViewport.height-8;
    if(cardH>400)cardH=400;
    float total=count*(cardW+cardGap)-cardGap,max=fmaxf(0,total-w);
    Vector2 drag=MobileMenuDragDelta(musicCarouselViewport);
    if(musicPreviewScrubTrack<0)musicCarouselOffset-=drag.x;
    if(CheckCollisionPointRec(GetMousePosition(),musicCarouselViewport)&&drag.x==0)
        musicCarouselOffset-=__real_GetMouseWheelMove()*100;
    if(IsKeyPressed(KEY_RIGHT))musicCarouselOffset+=cardW+cardGap;
    if(IsKeyPressed(KEY_LEFT))musicCarouselOffset-=cardW+cardGap;
    if(musicCarouselOffset<0)musicCarouselOffset=0;if(musicCarouselOffset>max)musicCarouselOffset=max;
    const int order[MUSIC_TRACK_COUNT+1]={0,MUSIC_DEFAULT_TRACK,1,2,3,5,6,MUSIC_NEON_PULSE_TRACK,MUSIC_OVERACHIEVER_TRACK,-1};
    unsigned *active=musicShopContext==0?&music_menu_tracks:&music_gameplay_tracks;
    BeginScissorMode((int)musicCarouselViewport.x,(int)musicCarouselViewport.y,
        (int)musicCarouselViewport.width,(int)musicCarouselViewport.height);
    for(int i=0;i<count;i++){
        Rectangle card={x+i*(cardW+cardGap)-musicCarouselOffset,musicCarouselViewport.y+4,cardW,cardH};
        if(card.x+card.width<x||card.x>x+w)continue;
        int track=order[i];bool unlocked=track<0||IsMusicTrackUnlocked(track);
        MusicAccess access=track<0?MUSIC_ACCESS_INCLUDED:GetMusicTrackAccess(track);
        int price=track<0?0:GetMusicTrackPrice(track);
        bool selected=track<0?*active==0:unlocked&&(*active&(1u<<track));
        bool preview=track>=0&&GetSoundtrackPreviewTrack()==track,previewClick=false;
        bool clicked=DrawSoundtrackCard(card,track,selected,preview,unlocked,access,price,&previewClick,t);
        if(previewClick)SetSoundtrackPreviewTrack(preview?-1:track);
        if(clicked){
            if(track<0){*active=0;save=true;}
            else if(unlocked){*active^=1u<<track;save=true;}
            else if(access==MUSIC_ACCESS_PURCHASE&&coin_balance>=price){
                coin_balance-=price;music_owned[track]=1;StatsShopSpent(STATS_SHOP_MUSIC,price);
                *active|=1u<<track;SaveStats();SaveShop();save=true;
            }
        }
    }
    EndScissorMode();
    float trackY=musicCarouselViewport.y+musicCarouselViewport.height+8;
    DrawRectangleRounded((Rectangle){x,trackY,w,6},1,4,(Color){38,43,63,255});
    float thumb=w*w/total;
    DrawRectangleRounded((Rectangle){x+(w-thumb)*(max>0?musicCarouselOffset/max:0),trackY,thumb,6},1,4,ac);


    float by=panel.y+panel.height-60;
    if(MusicMobileButton((Rectangle){x+w-126,by,54,42},"<",ac,false))musicCarouselOffset=fmaxf(0,musicCarouselOffset-cardW-cardGap);
    if(MusicMobileButton((Rectangle){x+w-60,by,54,42},">",ac,false))musicCarouselOffset=fminf(max,musicCarouselOffset+cardW+cardGap);
    if(save)SaveSettings();
}
