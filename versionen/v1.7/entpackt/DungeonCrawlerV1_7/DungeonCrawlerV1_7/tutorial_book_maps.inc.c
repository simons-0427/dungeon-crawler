// Pure guide maps. They mirror the room generators without touching a live run.
// The boss arena is exact; procedural rooms use one representative topology.

#define GUIDE_MAP_W 68
#define GUIDE_MAP_H 48

typedef struct {
    char tiles[GUIDE_MAP_H][GUIDE_MAP_W];
    bool safe[GUIDE_MAP_H][GUIDE_MAP_W];
    int w,h,playerX,playerY,rewardX,rewardY,exitX,exitY;
} TutorialGuideMap;

static TutorialGuideMap guideMaps[4];
static bool guideMapsBuilt[4];

static unsigned GuideRandom(unsigned *state) {
    *state^=*state<<13;
    *state^=*state>>17;
    *state^=*state<<5;
    return *state;
}

static int GuideRange(unsigned *state,int low,int high) {
    return low+(int)(GuideRandom(state)%(unsigned)(high-low+1));
}

static void GuideFill(TutorialGuideMap *m,int w,int h,char tile) {
    memset(m,0,sizeof(*m));
    m->w=w;m->h=h;
    for(int y=0;y<h;y++)for(int x=0;x<w;x++)m->tiles[y][x]=tile;
}

static bool GuideOpen(char tile) {
    return tile!='#'&&tile!=' '&&tile!='D';
}

static void GuideMaze(TutorialGuideMap *m,int xMin,int yMin,int xMax,int yMax,
                      int rootX,int rootY,unsigned *seed) {
    int firstX=xMin+1,firstY=yMin+1;
    int cols=(xMax-1-firstX)/2+1,rows=(yMax-1-firstY)/2+1;
    for(int y=yMin+1;y<yMax;y++)for(int x=xMin+1;x<xMax;x++)m->tiles[y][x]='#';
    int rx=(rootX-firstX+1)/2,ry=(rootY-firstY+1)/2;
    if(rx<0)rx=0;
    if(rx>=cols)rx=cols-1;
    if(ry<0)ry=0;
    if(ry>=rows)ry=rows-1;
    int sx=firstX+rx*2,sy=firstY+ry*2;
    bool visited[GUIDE_MAP_H][GUIDE_MAP_W]={{false}};
    int stackX[GUIDE_MAP_H*GUIDE_MAP_W];
    int stackY[GUIDE_MAP_H*GUIDE_MAP_W];
    int top=0;
    visited[sy][sx]=true;m->tiles[sy][sx]='.';
    stackX[top]=sx;stackY[top++]=sy;
    const int dx[4]={0,1,0,-1},dy[4]={-1,0,1,0};
    while(top>0) {
        int x=stackX[top-1],y=stackY[top-1];
        int order[4]={0,1,2,3};
        for(int i=3;i>0;i--) {
            int j=GuideRange(seed,0,i),swap=order[i];
            order[i]=order[j];order[j]=swap;
        }
        bool advanced=false;
        for(int i=0;i<4;i++) {
            int d=order[i],nx=x+dx[d]*2,ny=y+dy[d]*2;
            if(nx<firstX||ny<firstY||nx>=firstX+cols*2||
               ny>=firstY+rows*2||visited[ny][nx])continue;
            m->tiles[y+dy[d]][x+dx[d]]='.';
            m->tiles[ny][nx]='.';
            visited[ny][nx]=true;
            stackX[top]=nx;stackY[top++]=ny;
            advanced=true;break;
        }
        if(!advanced)top--;
    }
}

static void GuideLoopsAndChambers(TutorialGuideMap *m,int xMin,int yMin,
                                  int xMax,int yMax,int loops,int chambers,
                                  unsigned *seed) {
    int opened=0;
    for(int tries=0;tries<loops*10&&opened<loops;tries++) {
        int x=GuideRange(seed,xMin,xMax),y=GuideRange(seed,yMin,yMax);
        if(m->tiles[y][x]!='#')continue;
        bool horizontal=GuideOpen(m->tiles[y][x-1])&&GuideOpen(m->tiles[y][x+1]);
        bool vertical=GuideOpen(m->tiles[y-1][x])&&GuideOpen(m->tiles[y+1][x]);
        if(horizontal||vertical){m->tiles[y][x]='.';opened++;}
    }
    for(int room=0;room<chambers;room++) {
        int ax=-1,ay=-1;
        for(int tries=0;tries<100;tries++) {
            int x=GuideRange(seed,xMin,xMax),y=GuideRange(seed,yMin,yMax);
            if(GuideOpen(m->tiles[y][x])){ax=x;ay=y;break;}
        }
        if(ax<0)continue;
        int rx=GuideRange(seed,2,3),ry=GuideRange(seed,1,2);
        for(int y=ay-ry;y<=ay+ry;y++)for(int x=ax-rx;x<=ax+rx;x++) {
            if(x<=xMin||x>=xMax||y<=yMin||y>=yMax)continue;
            int dx=x-ax,dy=y-ay;
            if(dx*dx*ry*ry+dy*dy*rx*rx<=rx*rx*ry*ry)m->tiles[y][x]='.';
        }
    }
}

static void GuideFarthest(TutorialGuideMap *m,int sx,int sy,int minY,
                          int *outX,int *outY) {
    int dist[GUIDE_MAP_H][GUIDE_MAP_W];
    int qx[GUIDE_MAP_H*GUIDE_MAP_W],qy[GUIDE_MAP_H*GUIDE_MAP_W];
    for(int y=0;y<m->h;y++)for(int x=0;x<m->w;x++)dist[y][x]=-1;
    int head=0,tail=0;
    qx[tail]=sx;qy[tail++]=sy;dist[sy][sx]=0;
    *outX=sx;*outY=sy;
    int best=-1;
    const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
    while(head<tail) {
        int x=qx[head],y=qy[head++];
        if(y>=minY&&dist[y][x]>best&&m->tiles[y][x]=='.') {
            best=dist[y][x];*outX=x;*outY=y;
        }
        for(int d=0;d<4;d++) {
            int nx=x+dx[d],ny=y+dy[d];
            if(nx<1||ny<1||nx>=m->w-1||ny>=m->h-1||
               dist[ny][nx]>=0||!GuideOpen(m->tiles[ny][nx]))continue;
            dist[ny][nx]=dist[y][x]+1;
            qx[tail]=nx;qy[tail++]=ny;
        }
    }
}

static bool GuideMarkPath(TutorialGuideMap *m,int sx,int sy,int tx,int ty) {
    short prevX[GUIDE_MAP_H][GUIDE_MAP_W];
    short prevY[GUIDE_MAP_H][GUIDE_MAP_W];
    int qx[GUIDE_MAP_H*GUIDE_MAP_W],qy[GUIDE_MAP_H*GUIDE_MAP_W];
    for(int y=0;y<m->h;y++)for(int x=0;x<m->w;x++)prevX[y][x]=-1;
    int head=0,tail=0;
    qx[tail]=sx;qy[tail++]=sy;prevX[sy][sx]=sx;prevY[sy][sx]=sy;
    const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
    while(head<tail&&prevX[ty][tx]<0) {
        int x=qx[head],y=qy[head++];
        for(int d=0;d<4;d++) {
            int nx=x+dx[d],ny=y+dy[d];
            if(nx<1||ny<1||nx>=m->w-1||ny>=m->h-1||
               prevX[ny][nx]>=0||!GuideOpen(m->tiles[ny][nx]))continue;
            prevX[ny][nx]=(short)x;prevY[ny][nx]=(short)y;
            qx[tail]=nx;qy[tail++]=ny;
        }
    }
    if(prevX[ty][tx]<0)return false;
    int x=tx,y=ty;
    for(int guard=0;guard<m->w*m->h;guard++) {
        m->safe[y][x]=true;
        if(x==sx&&y==sy)break;
        int oldX=x;
        x=prevX[y][x];y=prevY[y][oldX];
    }
    return true;
}

static void GuideScatter(TutorialGuideMap *m,unsigned *seed,char tile,
                         int count,int xMin,int yMin,int xMax,int yMax) {
    int spawnGuard=tile=='e'?(m->w==64?12:10):6;
    int rewardGuard=tile=='e'&&m->w==64?7:3;
    for(int placed=0,tries=0;placed<count&&tries<count*70;tries++) {
        int x=GuideRange(seed,xMin,xMax),y=GuideRange(seed,yMin,yMax);
        if(m->tiles[y][x]!='.'||
           ((tile=='L'||tile=='+'||tile=='h')&&m->safe[y][x])||
           abs(x-m->playerX)+abs(y-m->playerY)<spawnGuard||
           abs(x-m->rewardX)+abs(y-m->rewardY)<rewardGuard)continue;
        if(tile=='e') {
            bool crowded=false;
            for(int yy=y-2;yy<=y+2&&!crowded;yy++)for(int xx=x-2;xx<=x+2;xx++)
                if(xx>=0&&yy>=0&&xx<m->w&&yy<m->h&&
                   abs(xx-x)+abs(yy-y)<3&&m->tiles[yy][xx]=='e')crowded=true;
            if(crowded)continue;
        }
        m->tiles[y][x]=tile;placed++;
    }
}

static void GuidePlaceExampleKey(TutorialGuideMap *m) {
    int best=-1,keyX=-1,keyY=-1;
    for(int y=1;y<m->h-1;y++)for(int x=1;x<m->w-1;x++) {
        if(!m->safe[y][x]||m->tiles[y][x]!='.')continue;
        int fromStart=abs(x-m->playerX)+abs(y-m->playerY);
        int fromReward=abs(x-m->rewardX)+abs(y-m->rewardY);
        if(fromStart<7||fromReward<6)continue;
        int score=fromStart<fromReward?fromStart:fromReward;
        if(score>best){best=score;keyX=x;keyY=y;}
    }
    if(keyX>=0)m->tiles[keyY][keyX]='K';
}

static void GuideBuildBoss(TutorialGuideMap *m) {
    GuideFill(m,27,19,'.');
    for(int y=0;y<19;y++)for(int x=0;x<27;x++)
        if(x==0||x==26||y==0||y==18)m->tiles[y][x]='#';
    const int plates[4][2]={{13,5},{13,13},{4,9},{22,9}};
    for(int i=0;i<4;i++)m->tiles[plates[i][1]][plates[i][0]]='^';
    m->tiles[9][2]='C';m->tiles[9][24]='C';
    m->playerX=13;m->playerY=16;
    m->rewardX=13;m->rewardY=3;
}

static void GuideBuildSide(TutorialGuideMap *m) {
    unsigned seed=0x51DE0007u;
    GuideFill(m,48,46,' ');
    for(int y=0;y<=4;y++)for(int x=0;x<48;x++)
        m->tiles[y][x]=(x==0||x==47||y==0||y==4)?'#':'.';
    for(int y=11;y<46;y++)for(int x=5;x<=42;x++)m->tiles[y][x]='#';
    GuideMaze(m,5,11,42,45,24,12,&seed);
    GuideLoopsAndChambers(m,6,12,41,44,24,6,&seed);
    for(int y=4;y<=11;y++) {
        m->tiles[y][23]='#';m->tiles[y][24]='.';m->tiles[y][25]='#';
    }
    m->tiles[4][24]='G';
    m->tiles[2][19]='k';
    m->playerX=31;m->playerY=2;
    for(int lane=0;lane<2;lane++) {
        int y=lane?35:25;
        m->tiles[y][5]='D';
        for(int x=6;x<=41;x++)m->tiles[y][x]='r';
    }
    GuideFarthest(m,24,12,29,&m->rewardX,&m->rewardY);
    GuideMarkPath(m,24,12,m->rewardX,m->rewardY);
    GuideScatter(m,&seed,'L',24,6,12,41,44);
    GuideScatter(m,&seed,'+',22,6,12,41,44);
    GuideScatter(m,&seed,'h',8,6,12,41,44);
    GuideScatter(m,&seed,'$',23,6,12,41,44);
    GuidePlaceExampleKey(m);
    m->tiles[m->rewardY][m->rewardX]='A';
}

static void GuideBuildArtifact(TutorialGuideMap *m) {
    unsigned seed=0xA17F39D1u;
    GuideFill(m,64,42,'.');
    for(int y=0;y<m->h;y++)for(int x=0;x<m->w;x++) {
        if(x==0||y==0||x==m->w-1||y==m->h-1)m->tiles[y][x]='#';
        else if(GuideRange(&seed,0,99)<24)m->tiles[y][x]='#';
    }
    m->playerX=32;m->playerY=2;
    m->rewardX=48;m->rewardY=38;
    for(int y=1;y<=4;y++)for(int x=30;x<=34;x++)m->tiles[y][x]='.';
    for(int y=36;y<=40;y++)for(int x=46;x<=50;x++)m->tiles[y][x]='.';
    if(!GuideMarkPath(m,32,2,48,38)) {
        for(int y=2;y<=25;y++)m->tiles[y][32]='.';
        for(int x=32;x<=48;x++)m->tiles[25][x]='.';
        for(int y=25;y<=38;y++)m->tiles[y][48]='.';
        GuideMarkPath(m,32,2,48,38);
    }
    GuideScatter(m,&seed,'L',44,1,1,62,40);
    GuideScatter(m,&seed,'+',84,1,1,62,40);
    GuideScatter(m,&seed,'$',38,1,1,62,40);
    GuideScatter(m,&seed,'e',30,1,1,62,40);
    GuidePlaceExampleKey(m);
    m->tiles[38][47]='I';m->tiles[38][49]='I';
    m->tiles[39][48]='I';m->tiles[38][48]='l';
}

static void GuideBuildVault(TutorialGuideMap *m) {
    unsigned seed=0x6D2B79F5u;
    GuideFill(m,54,38,'#');
    m->playerX=3;m->playerY=3;
    GuideMaze(m,0,0,53,37,3,3,&seed);
    GuideLoopsAndChambers(m,1,1,52,36,15,4,&seed);
    GuideFarthest(m,m->playerX,m->playerY,1,&m->rewardX,&m->rewardY);
    GuideFarthest(m,m->rewardX,m->rewardY,1,&m->exitX,&m->exitY);
    GuideMarkPath(m,m->playerX,m->playerY,m->rewardX,m->rewardY);
    GuideMarkPath(m,m->rewardX,m->rewardY,m->exitX,m->exitY);
    GuideScatter(m,&seed,'L',54,1,1,52,36);
    GuideScatter(m,&seed,'+',48,1,1,52,36);
    GuideScatter(m,&seed,'$',34,1,1,52,36);
    GuideScatter(m,&seed,'e',16,1,1,52,36);
    GuidePlaceExampleKey(m);
    m->tiles[m->rewardY][m->rewardX]='A';
    if(m->exitX!=m->rewardX||m->exitY!=m->rewardY)
        m->tiles[m->exitY][m->exitX]='T';
}

static TutorialGuideMap *GuideMapForKind(int kind) {
    if(kind<0||kind>3)return NULL;
    if(!guideMapsBuilt[kind]) {
        TutorialGuideMap *m=&guideMaps[kind];
        if(kind==0)GuideBuildBoss(m);
        else if(kind==1)GuideBuildSide(m);
        else if(kind==2)GuideBuildArtifact(m);
        else GuideBuildVault(m);
        guideMapsBuilt[kind]=true;
    }
    return &guideMaps[kind];
}

static void GuidePlayer(Vector2 center,float cell) {
    float radius=fmaxf(5.2f,cell*0.40f);
    Color skin=GetSkinColor(0);
    DrawCircleV(center,radius,skin);
    DrawCircleV((Vector2){center.x-radius*0.25f,center.y-radius*0.28f},
                radius*0.42f,TutorialColorA(TutorialLighten(skin,0.28f),120));
    float eyeRadius=fmaxf(1.15f,radius*0.17f);
    DrawCircleV((Vector2){center.x-radius*0.37f,center.y-radius*0.16f},
                eyeRadius,(Color){10,10,22,255});
    DrawCircleV((Vector2){center.x+radius*0.37f,center.y-radius*0.16f},
                eyeRadius,(Color){10,10,22,255});
}

static void GuideGate(Vector2 c,float size) {
    float x=c.x-size*0.5f,y=c.y-size*0.5f,s=size/32.0f;
    DrawRectangleRec((Rectangle){x+2*s,y+s,28*s,31*s},(Color){12,10,21,255});
    DrawRectangleRec((Rectangle){x+5*s,y+4*s,22*s,26*s},(Color){20,15,32,255});
    DrawRectangleRec((Rectangle){x+6*s,y+5*s,9*s,24*s},(Color){43,31,52,255});
    DrawRectangleRec((Rectangle){x+17*s,y+5*s,9*s,24*s},(Color){34,25,45,255});
    DrawRectangleRec((Rectangle){x+14*s,y+5*s,3*s,23*s},(Color){10,8,18,255});
    DrawRing((Vector2){c.x,c.y+7*s},3*s,5*s,0,360,18,
             (Color){239,193,103,255});
    DrawCircleV((Vector2){c.x,c.y+6*s},fmaxf(1.0f,s),
                (Color){6,8,15,255});
    DrawRectangleRec((Rectangle){x+15*s,y+23*s,2*s,4*s},
                     (Color){6,8,15,255});
    DrawRectangleRec((Rectangle){x+2*s,y+2*s,3*s,29*s},
                     (Color){104,85,112,255});
    DrawRectangleRec((Rectangle){x+27*s,y+2*s,3*s,29*s},
                     (Color){76,61,91,255});
    DrawRectangleRec((Rectangle){x+3*s,y+2*s,26*s,3*s},
                     (Color){171,144,141,255});
}

static void GuideWisp(Vector2 c,float size,float t) {
    float s=size/32.0f;
    float pulse=photosensitivity?0.5f:0.5f+0.5f*sinf(t*3.1f);
    DrawCircleV(c,size*0.55f,(Color){102,220,255,(uchar)(25+15*pulse)});
    DrawRing(c,12*s,15*s,0,360,24,(Color){155,235,255,(uchar)(95+35*pulse)});
    DrawTriangle((Vector2){c.x-5*s,c.y-7*s},(Vector2){c.x-8*s,c.y-14*s},
                 (Vector2){c.x-s,c.y-8*s},(Color){36,107,149,255});
    DrawTriangle((Vector2){c.x+4*s,c.y-7*s},(Vector2){c.x+9*s,c.y-13*s},
                 (Vector2){c.x+7*s,c.y-4*s},(Color){54,150,173,255});
    DrawCircleV((Vector2){c.x,c.y+s},11*s,(Color){30,67,103,255});
    DrawCircleV((Vector2){c.x-4*s,c.y-3*s},6*s,(Color){108,205,216,88});
    float eye=fmaxf(0.9f,2.4f*s);
    DrawCircleV((Vector2){c.x-4*s,c.y-s},eye,(Color){255,230,139,255});
    DrawCircleV((Vector2){c.x+4*s,c.y-s},eye,(Color){255,230,139,255});
    DrawTriangle((Vector2){c.x-3*s,c.y+8*s},
                 (Vector2){c.x+s,c.y+12*s},
                 (Vector2){c.x+2*s,c.y+6*s},(Color){12,43,76,255});
    DrawLineEx((Vector2){c.x-14*s,c.y-4*s},
               (Vector2){c.x-10*s,c.y-3*s},fmaxf(1.0f,s),
               (Color){101,223,232,255});
    DrawLineEx((Vector2){c.x+10*s,c.y+6*s},
               (Vector2){c.x+14*s,c.y+6*s},fmaxf(1.0f,s),
               (Color){141,242,232,255});
}

static void GuideLadder(Vector2 c,float size,float t) {
    float x=c.x-size*0.5f,y=c.y-size*0.5f,s=size/32.0f;
    float glow=photosensitivity?0.5f:0.5f+0.5f*sinf(t*3.6f);
    DrawCircleV(c,size*0.48f,(Color){176,72,255,(uchar)(32+38*glow)});
    DrawRectangleRec((Rectangle){x+s,y+s,size-2*s,size-2*s},
                     (Color){55,34,72,255});
    DrawRectangleLinesEx((Rectangle){x+s,y+s,size-2*s,size-2*s},
                         fmaxf(1.0f,s),(Color){231,143,255,255});
    DrawRectangleRec((Rectangle){x+6*s,y+5*s,20*s,22*s},(Color){5,4,11,255});
    DrawRectangleRec((Rectangle){x+9*s,y+7*s,3*s,16*s},(Color){210,177,123,255});
    DrawRectangleRec((Rectangle){x+20*s,y+7*s,3*s,16*s},(Color){210,177,123,255});
    for(int rung=0;rung<4;rung++)
        DrawRectangleRec((Rectangle){x+11*s,y+(9+rung*4)*s,10*s,
                                     fmaxf(1.0f,2*s)},(Color){245,214,155,255});
    DrawLineEx((Vector2){x+11*s,y+23*s},(Vector2){x+16*s,y+27*s},
               fmaxf(1.0f,2*s),(Color){220,130,255,255});
    DrawLineEx((Vector2){x+16*s,y+27*s},(Vector2){x+21*s,y+23*s},
               fmaxf(1.0f,2*s),(Color){220,130,255,255});
}

static void GuidePickup(Vector2 c,float size,IKind kind) {
    Color color=IK_COL[kind];
    DrawCircleV(c,size*0.58f,TutorialColorA(color,50));
    DrawTutorialDiamond(c.x,c.y,size*0.48f,color);
    DrawTutorialItemGlyph(c.x,c.y,size*0.75f,kind,
                          TutorialLighten(color,0.30f));
}

static void DrawTutorialGuideMap(Rectangle panel,int kind,float t,
                                 TutorialBookStyle b) {
    TutorialGuideMap *m=GuideMapForKind(kind);
    if(!m)return;
    Color accent=kind==0?(Color){192,85,74,255}:
                 kind==1?(Color){88,166,151,255}:
                         (Color){149,105,201,255};
    DrawRectangleRounded(panel,0.06f,6,(Color){24,28,39,255});
    DrawRectangleRoundedLinesEx(panel,0.06f,6,1.5f,accent);
    const char *heading=kind==0?"FIXED ARENA / 27 x 19 TILES":
                        kind==1?"OPTIONAL GATE / MAIN FLOOR ABOVE, MAZE BELOW":
                        kind==2?"FIRST PART / OPEN FLOOR, LADDER BELOW":
                                "VAULT / MAZE, REWARD, THEN EXIT";
    int labelFs=b.fsB-1;
    DrawTutorialTextFit(heading,(int)panel.x+MenuPx(b.u,9),
                        (int)panel.y+MenuPx(b.u,6),labelFs,
                        (int)panel.width-MenuPx(b.u,18),
                        TutorialLighten(accent,0.3f));
    int top=(int)panel.y+MenuPx(b.u,22);
    int footer=MenuPx(b.u,22);
    float cell=fminf((panel.width-MenuPx(b.u,16))/m->w,
                     ((int)(panel.y+panel.height)-top-footer)/(float)m->h);
    if(cell<2.0f)return;
    float gx=panel.x+(panel.width-m->w*cell)*0.5f;
    float gy=top+((panel.y+panel.height-footer-top)-m->h*cell)*0.5f;
    for(int y=0;y<m->h;y++)for(int x=0;x<m->w;x++) {
        char tile=m->tiles[y][x];
        Rectangle r={gx+x*cell,gy+y*cell,cell+0.25f,cell+0.25f};
        if(kind==0) {
            DrawTutorialMapSymbol((Rectangle){r.x,r.y,cell,cell},tile,IK_NONE,t);
            continue;
        }
        Color fill=tile=='#'?(Color){102,109,124,255}:
                   tile==' '?(Color){18,21,31,255}:
                   (Color){55,49,67,255};
        if(kind==2&&abs(x-m->rewardX)<=2&&abs(y-m->rewardY)<=2)
            fill=(abs(x-m->rewardX)==2||abs(y-m->rewardY)==2)?
                 (Color){43,27,59,255}:(Color){31,24,44,255};
        DrawRectangleRec(r,fill);
        if(kind==2&&abs(x-m->rewardX)<=2&&abs(y-m->rewardY)<=2)
            DrawRectangleLinesEx((Rectangle){r.x,r.y,cell,cell},
                                 0.65f,(Color){139,80,188,255});
        if(tile=='#'&&cell>=6.0f)
            DrawRectangleRec((Rectangle){r.x,r.y,r.width,1.0f},
                             (Color){151,155,163,255});
        if(tile=='L')DrawRectangleRec((Rectangle){r.x+cell*0.16f,r.y+cell*0.16f,
                                      cell*0.68f,cell*0.68f},(Color){244,96,36,255});
        else if(tile=='+'||tile=='^') {
            DrawRectangleRec((Rectangle){r.x+cell*0.25f,r.y+cell*0.25f,
                                          cell*0.5f,cell*0.5f},
                              tile=='^'?(Color){192,200,211,255}:
                                        (Color){231,183,191,255});
        } else if(tile=='h')DrawRectangleRec((Rectangle){r.x+cell*0.15f,r.y+cell*0.15f,
                                      cell*0.7f,cell*0.7f},(Color){12,11,22,255});
        else if(tile=='r')DrawRectangleRec((Rectangle){r.x,r.y+cell*0.40f,
                                     cell,cell*0.20f},(Color){210,91,79,255});
        else if(tile=='D')DrawRectangleRec(r,(Color){145,66,61,255});
        else if(tile=='C')DrawCircleV((Vector2){r.x+cell*0.5f,r.y+cell*0.5f},
                              cell*0.42f,(Color){218,166,78,255});
        else if(tile=='$')DrawCircleV((Vector2){r.x+cell*0.5f,r.y+cell*0.5f},
                              cell*0.25f,(Color){244,206,89,255});
    }
    if(kind!=0) {
        bool sideGateVisible=kind!=1||((int)(t/3.5f)%2)==0;
        int artifact=TutorialExampleArtifact(kind==1);
        int keyId=TutorialExampleKey(artifact);
        int lockId=artifact>=0&&ArtifactLockOptionCount(artifact)>0?
                   ArtifactLockOptionAt(artifact,0):-1;
        for(int y=0;y<m->h;y++)for(int x=0;x<m->w;x++) {
            char tile=m->tiles[y][x];
            if(tile!='e'&&tile!='I'&&tile!='l'&&tile!='A'&&
               tile!='T'&&tile!='G'&&tile!='k'&&tile!='K')continue;
            Vector2 c={gx+(x+0.5f)*cell,gy+(y+0.5f)*cell};
            if(tile=='e')GuideWisp(c,fmaxf(10.5f,cell*1.40f),t);
            else if(tile=='I') {
                IKind itemKind=kind==2&&x==m->rewardX&&y==m->rewardY+1?
                               IK_SHD:IK_SWD;
                GuidePickup(c,fmaxf(10.0f,cell*1.25f),itemKind);
            } else if(tile=='l')GuideLadder(c,fmaxf(14.0f,cell*1.75f),t);
            else if(tile=='G') {
                if(sideGateVisible)GuideGate(c,fmaxf(13.0f,cell*1.55f));
            }
            else if(tile=='k'||tile=='K') {
                if(tile=='k'&&!sideGateVisible)continue;
                int key=tile=='k'?KEY_JADEEYE:keyId;
                int size=(int)fmaxf(13.0f,cell*1.6f);
                if(key>=0)DrawUniversalKeyIcon(key,(int)(c.x-size*0.5f),
                                               (int)(c.y-size*0.5f),size,t);
            } else if(tile=='A') {
                int size=(int)fmaxf(kind==2?18.0f:14.0f,
                                    cell*(kind==2?2.2f:1.7f));
                if(lockId>=0)DrawLockedObjectIcon(lockId,(int)(c.x-size*0.5f),
                                                   (int)(c.y-size*0.5f),size);
                else if(artifact>=0)DrawArtifactIcon(artifact,c,size*0.55f,true);
            } else if(tile=='T') {
                float sz=fmaxf(10.0f,cell*1.30f);
                DrawRectangleRec((Rectangle){c.x-sz*0.36f,c.y-sz*0.27f,
                                             sz*0.72f,sz*0.57f},
                                 (Color){192,142,62,255});
                DrawRectangleRec((Rectangle){c.x-sz*0.36f,c.y-sz*0.27f,
                                             sz*0.72f,sz*0.13f},
                                 (Color){255,228,158,255});
                DrawCircleV((Vector2){c.x,c.y+sz*0.05f},
                            fmaxf(1.0f,sz*0.08f),(Color){94,57,31,255});
            }
        }
    }
    Vector2 player={(float)(gx+(m->playerX+0.5f)*cell),
                    (float)(gy+(m->playerY+0.5f)*cell)};
    GuidePlayer(player,cell);
    if(kind==0) {
        float bossX=gx+(m->rewardX+0.5f)*cell;
        float bossY=gy+(m->rewardY+0.5f)*cell;
        DrawBossPreview(bossX,bossY,cell*0.85f,((int)(t/4.0f))%5,t);
    }
    const char *legend=kind==0?"GREEN PLAYER   RED BOSS   SILVER PLATES   GOLD CANNONS":
                       kind==1?"GATE + KEYS   RED ARROWS   ORANGE LAVA   GOLD REWARD":
                       kind==2?"CYAN WISPS   GOLD KEY   GREEN ITEMS   PURPLE LADDER":
                               "OPTIONAL KEY   GOLD REWARD   LILAC EXIT AFTER PICKUP";
    DrawTutorialTextFit(legend,(int)panel.x+MenuPx(b.u,8),
                        (int)(panel.y+panel.height)-MenuPx(b.u,17),
                        b.fsB-2,(int)panel.width-MenuPx(b.u,16),
                        (Color){190,203,208,255});
    if(kind==1) {
        int calloutW=(int)(gx-panel.x)-MenuPx(b.u,12);
        if(calloutW>38) {
            int labelX=(int)panel.x+MenuPx(b.u,7);
            DrawTutorialTextFit("MAIN FLOOR",labelX,(int)(gy+cell),
                                b.fsB-3,calloutW,(Color){185,220,203,255});
            DrawTutorialTextFit("GATE",labelX,(int)(gy+cell*5),
                                b.fsB-3,calloutW,(Color){242,206,132,255});
            DrawTutorialTextFit("CORRIDOR",labelX,(int)(gy+cell*9),
                                b.fsB-3,calloutW,(Color){185,220,203,255});
            DrawTutorialTextFit("MAZE",labelX,(int)(gy+cell*22),
                                b.fsB-3,calloutW,(Color){185,220,203,255});
        }
    }
}
