// Standalone 32x32 coin pile gallery. Build from Snapshot with raylib.
// --export-sprites writes the individual transparent PNGs to Library.
// --capture-page=1 or 2 writes a gallery preview to Library.
#include "raylib.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DESIGN_COUNT 8
#define PAGE_SIZE 4
#define PAGE_COUNT 2
#define C(r,g,b) ((Color){r,g,b,255})

typedef struct { signed char x,y,shape,tone; } CoinSpec;

static const char *names[DESIGN_COUNT]={
    "Prospector's Spill","Royal Reserve","Vault Scatter","Sunburst Cache",
    "Gemmed Jackpot","Moonlit Gold","Pirate's Hoard","Crownfall Coins"
};
static const char *notes[DESIGN_COUNT]={
    "Wide heap, diamond and ruby glints","A tall pyramid of minted gold",
    "Coins spilling past a small sapphire","Stacked gold with a bright diamond",
    "Round coins and a few cut stones","Pale highlights across warm gold",
    "Dense amber hoard with emerald","Tumbled coins below a clear diamond"
};

// Back rows first, front rows last. Shapes: small flat, wide flat, upright, stack.
static const CoinSpec pile0[]={
    {8,18,1,1},{14,15,2,0},{20,17,0,0},{25,19,1,1},
    {5,22,0,0},{10,21,3,0},{16,20,1,0},{22,21,2,1},{28,23,0,0},
    {7,26,1,1},{13,25,0,0},{19,25,3,0},{25,26,1,0},{30,26,0,1}
};
static const CoinSpec pile1[]={
    {16,11,2,0},{12,15,0,0},{19,15,1,1},{8,19,2,0},{14,18,3,0},
    {21,18,2,0},{25,20,0,1},{5,24,0,1},{10,23,1,0},{16,22,2,0},
    {22,23,3,0},{27,24,1,1},{8,27,0,0},{15,27,1,0},{22,27,1,0}
};
static const CoinSpec pile2[]={
    {7,17,2,1},{13,18,1,0},{21,17,0,1},{26,19,1,0},{4,22,0,0},
    {10,22,3,0},{16,21,0,0},{23,22,2,0},{29,23,0,1},{6,26,1,1},
    {13,26,0,0},{19,25,3,0},{25,26,1,0},{30,27,0,0}
};
static const CoinSpec pile3[]={
    {8,13,3,0},{15,15,2,0},{23,17,1,1},{5,18,2,1},{11,19,3,0},
    {18,19,1,0},{26,21,0,0},{5,24,1,0},{12,24,2,0},{19,23,3,1},
    {25,25,1,0},{8,27,1,0},{15,27,0,1},{21,27,1,0}
};
static const CoinSpec pile4[]={
    {11,16,1,0},{18,15,2,1},{24,17,0,0},{7,20,2,0},{14,20,3,0},
    {21,20,2,0},{27,21,1,1},{4,25,0,1},{10,24,1,0},{16,24,2,0},
    {23,24,3,0},{28,26,0,0},{8,27,1,0},{15,27,0,1},{22,27,1,0}
};
static const CoinSpec pile5[]={
    {6,19,0,2},{13,16,2,3},{20,18,1,0},{27,20,0,2},{4,23,1,3},
    {10,22,3,0},{17,21,2,3},{24,23,1,0},{29,24,0,3},{7,26,1,0},
    {14,25,0,3},{20,25,3,0},{26,27,1,2},{12,28,0,0}
};
static const CoinSpec pile6[]={
    {13,14,2,1},{20,16,1,0},{8,18,3,1},{16,18,1,0},{25,19,2,1},
    {5,23,0,1},{11,22,2,0},{18,22,3,0},{24,22,1,1},{28,24,0,0},
    {7,27,1,1},{13,26,3,0},{19,26,0,0},{25,27,1,0}
};
static const CoinSpec pile7[]={
    {18,12,3,0},{12,15,2,0},{23,16,0,1},{8,19,1,0},{16,18,3,1},
    {22,19,2,0},{27,21,0,0},{5,23,0,1},{11,23,3,0},{18,22,1,0},
    {25,24,1,1},{7,27,1,0},{14,26,2,0},{20,27,3,0},{27,27,0,1}
};
static const CoinSpec *const piles[DESIGN_COUNT]={
    pile0,pile1,pile2,pile3,pile4,pile5,pile6,pile7
};
static const int pileCounts[DESIGN_COUNT]={
    sizeof(pile0)/sizeof(*pile0),sizeof(pile1)/sizeof(*pile1),
    sizeof(pile2)/sizeof(*pile2),sizeof(pile3)/sizeof(*pile3),
    sizeof(pile4)/sizeof(*pile4),sizeof(pile5)/sizeof(*pile5),
    sizeof(pile6)/sizeof(*pile6),sizeof(pile7)/sizeof(*pile7)
};

static void Tri(float x1,float y1,float x2,float y2,float x3,float y3,Color c){
    float cross=(x2-x1)*(y3-y1)-(y2-y1)*(x3-x1);
    Vector2 a={x1,y1},b={x2,y2},d={x3,y3};
    if(cross>0)DrawTriangle(a,d,b,c);else DrawTriangle(a,b,d,c);
}

static void DrawCoin(int x,int y,int shape,int tone){
    static const Color face[4]={
        {244,180,49,255},{207,132,43,255},{170,111,56,255},{246,204,104,255}
    };
    static const Color light[4]={
        {255,225,109,255},{247,180,66,255},{217,154,79,255},{255,234,155,255}
    };
    static const Color rim[4]={
        {121,69,29,255},{105,55,33,255},{91,54,37,255},{146,98,48,255}
    };
    if(shape==2){
        DrawCircle(x,y+1,4,rim[tone]);
        DrawCircle(x,y,3,face[tone]);
        DrawCircleLines(x,y,2,light[tone]);
        DrawPixel(x,y,light[tone]);
        DrawPixel(x-1,y-1,C(255,244,176));
        return;
    }
    int rx=shape==0?3:4;
    DrawEllipse(x,y+2,rx+1,shape==3?4:3,rim[tone]);
    if(shape==3){
        DrawRectangle(x-rx+1,y,rx*2-2,3,face[tone]);
        DrawLine(x-rx+1,y+2,x+rx-1,y+2,light[tone]);
    }
    DrawEllipse(x,y,rx,shape==0?2:3,face[tone]);
    DrawEllipseLines(x,y,rx,shape==0?2:3,light[tone]);
    DrawPixel(x-1,y-1,C(255,242,151));
}

static void DrawDiamond(int x,int y){
    Tri(x-5,y-1,x-2,y-5,x,y-1,C(60,143,171));
    Tri(x,y-1,x+2,y-5,x+5,y-1,C(174,240,250));
    Tri(x-5,y-1,x,y-1,x,y+5,C(56,178,210));
    Tri(x,y-1,x+5,y-1,x,y+5,C(111,221,242));
    Tri(x-2,y-5,x+2,y-5,x,y-1,C(240,253,249));
    DrawPixel(x,y-3,WHITE);
}

static void DrawGem(int x,int y,Color dark,Color bright){
    Tri(x,y-4,x+4,y,x,y+4,dark);
    Tri(x,y-4,x,y+4,x-4,y,dark);
    Tri(x,y-3,x+3,y,x,y+2,bright);
    DrawPixel(x-1,y-2,C(255,235,226));
}

static void DrawGlint(int x,int y,Color c){
    DrawLine(x-2,y,x+2,y,c);
    DrawLine(x,y-2,x,y+2,c);
    DrawPixel(x,y,WHITE);
}

static void DrawPile(int id){
    if(id<0||id>=DESIGN_COUNT)return;
    int baseX=id==2?18:id==3?14:16;
    int baseW=id==1?11:id==2?14:13;
    DrawEllipse(baseX,27,baseW,4,C(67,39,31));
    DrawEllipse(baseX,26,baseW-1,3,C(136,82,30));
    for(int i=0;i<pileCounts[id];i++){
        CoinSpec c=piles[id][i];
        DrawCoin(c.x,c.y,c.shape,c.tone);
    }
    switch(id){
        case 0:
            DrawDiamond(11,11);
            DrawGem(25,13,C(125,33,57),C(245,71,93));
            DrawGlint(5,13,C(255,244,180));
            break;
        case 1:
            DrawDiamond(17,7);
            DrawGem(9,14,C(80,42,119),C(181,108,245));
            DrawGlint(25,11,C(255,237,161));
            break;
        case 2:
            DrawDiamond(9,11);
            DrawGem(25,13,C(24,62,130),C(73,161,255));
            DrawPixel(30,17,C(238,224,129));
            break;
        case 3:
            DrawDiamond(24,11);
            DrawGem(16,11,C(133,36,46),C(248,87,81));
            DrawGlint(4,11,C(255,241,165));
            break;
        case 4:
            DrawDiamond(25,10);
            DrawGem(7,14,C(26,84,70),C(84,222,151));
            DrawGem(18,11,C(84,44,127),C(185,121,242));
            break;
        case 5:
            DrawDiamond(17,11);
            DrawGem(7,14,C(25,64,127),C(89,174,251));
            DrawGlint(27,13,C(239,242,191));
            break;
        case 6:
            DrawDiamond(8,11);
            DrawGem(24,12,C(30,92,65),C(107,234,124));
            DrawGlint(17,8,C(255,226,119));
            break;
        case 7:
            DrawDiamond(23,8);
            DrawGem(9,13,C(32,66,117),C(75,191,244));
            DrawGem(27,19,C(119,41,54),C(246,89,115));
            break;
    }
}

static void Backdrop(int bg){
    DrawRectangle(0,0,32,32,C(12,10,18));
    if(bg==0){
        DrawRectangle(1,1,30,30,C(28,24,40));
        DrawRectangle(1,1,30,1,C(71,61,90));
        DrawRectangle(2,29,28,1,C(15,12,26));
    }else if(bg==1){
        DrawRectangle(1,1,30,30,C(50,45,66));
        DrawRectangle(1,1,30,3,C(86,78,107));
        DrawRectangle(1,28,30,3,C(25,20,38));
    }else{
        DrawRectangle(1,1,30,30,C(43,27,59));
        DrawRectangle(2,2,28,28,C(31,24,44));
        DrawRectangleLines(1,1,30,30,C(139,80,188));
    }
}

static void LoadRatings(int ratings[DESIGN_COUNT]){
    memset(ratings,0,sizeof(int)*DESIGN_COUNT);
    FILE *file=fopen("Library/coin_pile_ratings.txt","r");
    if(!file)return;
    char line[160],state[24];int id;
    while(fgets(line,sizeof(line),file)){
        if(sscanf(line,"%d %23s",&id,state)!=2||id<0||id>=DESIGN_COUNT)continue;
        if(strcmp(state,"REJECTED")==0)ratings[id]=1;
        else if(strcmp(state,"MAYBE/FIX")==0)ratings[id]=2;
        else if(strcmp(state,"ACCEPTED")==0)ratings[id]=3;
    }
    fclose(file);
}

static void SaveRatings(const int ratings[DESIGN_COUNT]){
    FILE *file=fopen("Library/coin_pile_ratings.txt","w");
    if(!file)return;
    const char *states[]={"","REJECTED","MAYBE/FIX","ACCEPTED"};
    for(int i=0;i<DESIGN_COUNT;i++)if(ratings[i]>0)
        fprintf(file,"%d %s %s\n",i,states[ratings[i]],names[i]);
    fclose(file);
}

static int LoadSelection(void){
    FILE *file=fopen("Library/coin_pile_selected.txt","r");
    if(!file)return -1;
    int selected=0;
    if(fscanf(file,"%d",&selected)!=1||selected<0||selected>=DESIGN_COUNT)
        selected=-1;
    fclose(file);
    return selected;
}

static void SaveSelection(int selected){
    if(selected<0||selected>=DESIGN_COUNT)return;
    FILE *file=fopen("Library/coin_pile_selected.txt","w");
    if(!file)return;
    fprintf(file,"%d %s\n",selected,names[selected]);
    fclose(file);
}

static void ExportSprites(const int ratings[DESIGN_COUNT]){
    (void)ratings;
    RenderTexture2D target=LoadRenderTexture(32,32);
    if(target.id==0)return;
    for(int i=0;i<DESIGN_COUNT;i++){
        char path[256];
        snprintf(path,sizeof(path),"Library/coin_pile_%02d.png",i);
        if(FileExists(path))continue;
        BeginTextureMode(target);
        ClearBackground(BLANK);
        DrawPile(i);
        EndTextureMode();
        Image image=LoadImageFromTexture(target.texture);
        if(image.data){ExportImage(image,path);UnloadImage(image);}
    }
    UnloadRenderTexture(target);
}

static bool RatingButton(Rectangle r,const char *label,int active,int value,
                         Vector2 mouse){
    bool hot=CheckCollisionPointRec(mouse,r);
    Color accent=value==1?C(247,104,122):value==2?C(246,189,100):C(105,230,194);
    Color fill=active==value?value==1?C(91,43,56):value==2?C(102,75,42):C(39,91,77):
               hot?C(65,57,78):C(43,41,59);
    DrawRectangleRounded(r,0.18f,5,fill);
    DrawRectangleRoundedLinesEx(r,0.18f,5,active==value?2:1,
                                active==value||hot?accent:C(81,74,96));
    int textW=MeasureText(label,15);
    DrawText(label,(int)(r.x+(r.width-textW)*0.5f),(int)r.y+9,15,
             active==value?C(255,248,234):C(182,177,190));
    return hot&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

int main(int argc,char **argv){
    int capturePage=-1;
    bool exportOnly=false;
    for(int i=1;i<argc;i++){
        if(strncmp(argv[i],"--capture-page=",15)==0)
            capturePage=atoi(argv[i]+15)-1;
        else if(strcmp(argv[i],"--export-sprites")==0)exportOnly=true;
    }
    if(capturePage>=PAGE_COUNT)capturePage=PAGE_COUNT-1;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1240,780,"Coin Pile Designs - Dungeon Crawler");
    SetTargetFPS(60);
    ChangeDirectory(GetApplicationDirectory());
    MakeDirectory("Library");
    int ratings[DESIGN_COUNT];
    LoadRatings(ratings);
    int selected=LoadSelection();
    if(selected>=0)SaveSelection(selected);
    ExportSprites(ratings);
    if(exportOnly){CloseWindow();return 0;}
    RenderTexture2D previews[PAGE_SIZE][3];
    for(int slot=0;slot<PAGE_SIZE;slot++)for(int bg=0;bg<3;bg++){
        previews[slot][bg]=LoadRenderTexture(32,32);
        SetTextureFilter(previews[slot][bg].texture,TEXTURE_FILTER_POINT);
    }
    int page=capturePage>=0?capturePage:0;
    while(!WindowShouldClose()){
        float wheel=GetMouseWheelMove();
        if(IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_PAGE_DOWN)||wheel<0)
            page=(page+1)%PAGE_COUNT;
        if(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_PAGE_UP)||wheel>0)
            page=(page+PAGE_COUNT-1)%PAGE_COUNT;
        for(int slot=0;slot<PAGE_SIZE;slot++){
            bool pressed=IsKeyPressed(KEY_ONE+slot)||IsKeyPressed(KEY_KP_1+slot);
            if(!pressed)continue;
            int id=page*PAGE_SIZE+slot;
            if(IsKeyDown(KEY_LEFT_ALT)||IsKeyDown(KEY_RIGHT_ALT)){
                selected=id;
                SaveSelection(selected);
                continue;
            }
            int value=(IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL))?1:
                      (IsKeyDown(KEY_LEFT_SHIFT)||IsKeyDown(KEY_RIGHT_SHIFT))?2:3;
            ratings[id]=ratings[id]==value?0:value;
            SaveRatings(ratings);
        }
        for(int slot=0;slot<PAGE_SIZE;slot++)for(int bg=0;bg<3;bg++){
            BeginTextureMode(previews[slot][bg]);
            Backdrop(bg);
            DrawPile(page*PAGE_SIZE+slot);
            EndTextureMode();
        }
        int sw=GetScreenWidth(),sh=GetScreenHeight();
        float scale=fminf((float)sw/1240.0f,(float)sh/780.0f);
        if(scale<0.4f)scale=0.4f;
        int ox=(int)((sw-1240*scale)*0.5f),oy=(int)((sh-780*scale)*0.5f);
        Vector2 mouse=GetMousePosition();
        Vector2 local={(mouse.x-ox)/scale,(mouse.y-oy)/scale};
        int clicked=-1,clickedValue=0,selectedClick=-1;
        BeginDrawing();
        ClearBackground(C(11,9,19));
        DrawRectangleGradientV(0,0,sw,sh,C(14,12,25),C(27,17,34));
        BeginMode2D((Camera2D){.offset={(float)ox,(float)oy},.target={0,0},
                               .rotation=0,.zoom=scale});
        DrawText("COIN PILE DESIGNS",40,22,29,C(245,222,177));
        DrawText("1-4 ACCEPT  |  SHIFT+1-4 MAYBE/FIX  |  CTRL+1-4 REJECT  |  ALT+1-4 SELECT",
                 42,58,16,C(164,171,189));
        DrawText(TextFormat("PAGE %d / %d",page+1,PAGE_COUNT),1060,29,20,
                 C(131,229,216));
        for(int slot=0;slot<PAGE_SIZE;slot++){
            int id=page*PAGE_SIZE+slot;
            int x=40+(slot%2)*590,y=94+(slot/2)*320;
            Rectangle card={(float)x,(float)y,570,302};
            bool hot=CheckCollisionPointRec(local,card);
            Color edge=ratings[id]==1?C(247,104,122):ratings[id]==2?C(246,189,100):
                       ratings[id]==3?C(105,230,194):hot?C(129,207,217):C(76,64,88);
            DrawRectangleRounded(card,0.07f,6,hot?C(41,35,59):C(28,25,42));
            DrawRectangleRoundedLinesEx(card,0.07f,6,ratings[id]?3:1,edge);
            DrawText(TextFormat("%02d  %s",id+1,names[id]),x+20,y+13,20,
                     C(245,230,205));
            DrawText(notes[id],x+21,y+43,15,C(183,172,193));
            Rectangle selectButton={(float)x+438,(float)y+12,112,30};
            bool selectHot=CheckCollisionPointRec(local,selectButton);
            bool isSelected=id==selected;
            DrawRectangleRounded(selectButton,0.18f,5,
                                 isSelected?C(100,74,37):
                                 selectHot?C(70,56,47):C(44,40,51));
            DrawRectangleRoundedLinesEx(selectButton,0.18f,5,isSelected?2:1,
                                        isSelected?C(255,216,111):C(139,118,88));
            const char *selectText=isSelected?"SELECTED":"SELECT";
            DrawText(selectText,
                     (int)selectButton.x+(112-MeasureText(selectText,14))/2,
                     (int)selectButton.y+8,14,
                     isSelected?C(255,232,158):C(220,199,160));
            if(selectHot&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))selectedClick=id;
            Rectangle src={0,0,32,-32};
            Rectangle dst={(float)x+22,(float)y+73,180,180};
            DrawTexturePro(previews[slot][0].texture,src,dst,(Vector2){0,0},0,WHITE);
            DrawRectangleLinesEx(dst,2,C(111,100,127));
            const char *labels[3]={"FLOOR","WALL","PAD"};
            for(int bg=0;bg<3;bg++){
                int px=x+241,py=y+77+bg*54;
                DrawTexturePro(previews[slot][bg].texture,src,
                               (Rectangle){(float)px,(float)py,40,40},
                               (Vector2){0,0},0,WHITE);
                DrawText(labels[bg],px+51,py+13,15,C(171,184,200));
            }
            DrawText("32 x 32 TILE",x+241,y+239,13,C(130,222,214));
            const char *labelsButton[3]={"REJECT","MAYBE / FIX","ACCEPT"};
            const int buttonX[3]={20,200,386},buttonW[3]={166,170,164};
            for(int value=1;value<=3;value++){
                Rectangle button={(float)x+buttonX[value-1],(float)y+260,
                                  (float)buttonW[value-1],32};
                if(RatingButton(button,labelsButton[value-1],ratings[id],value,local)){
                    clicked=id;clickedValue=value;
                }
            }
        }
        int totals[4]={0};
        for(int i=0;i<DESIGN_COUNT;i++)totals[ratings[i]]++;
        DrawText("LEFT / RIGHT OR WHEEL: page   ESC: close",42,748,15,C(153,149,171));
        DrawText(TextFormat("REJECTED %d   MAYBE / FIX %d   ACCEPTED %d",
                            totals[1],totals[2],totals[3]),810,748,15,
                 C(232,205,147));
        EndMode2D();
        if(clicked>=0){
            ratings[clicked]=ratings[clicked]==clickedValue?0:clickedValue;
            SaveRatings(ratings);
        }
        if(selectedClick>=0){
            selected=selectedClick;
            SaveSelection(selected);
        }
        if(capturePage>=0){
            char path[256];
            snprintf(path,sizeof(path),"Library/coin_pile_preview_page_%d.png",page+1);
            rlDrawRenderBatchActive();
            Image frame=LoadImageFromScreen();
            if(frame.data){ExportImage(frame,path);UnloadImage(frame);}
        }
        EndDrawing();
        if(capturePage>=0)break;
    }
    for(int slot=0;slot<PAGE_SIZE;slot++)for(int bg=0;bg<3;bg++)
        UnloadRenderTexture(previews[slot][bg]);
    CloseWindow();
    return 0;
}
