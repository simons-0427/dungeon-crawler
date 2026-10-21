// Standalone artifact design gallery. The first five icons are copied from
// game_items.c so the original designs remain available for selection.
// Build from Snapshot in PowerShell:
// $env:PATH='C:\raylib\w64devkit\bin;'+$env:PATH
// gcc -std=c99 -O2 -Wall -Wextra -o artifact_ideas.exe artifact_ideas.c -IC:/raylib/raylib/src -LC:/raylib/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm
#include "raylib.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ORIGINAL_COUNT 5
#define ARTIFACT_IDEAS 18
#define PAGE_SIZE 6
#define PAGE_COUNT 3
#define C(r,g,b) ((Color){r,g,b,255})
#define CA(r,g,b,a) ((Color){r,g,b,a})
void DrawOriginalArtifact(int id, Vector2 c, float size, bool collected) {
    if (id<0 || id>=ORIGINAL_COUNT) return;
    float r=size*0.5f, w=size/18.0f;
    Color palette[ORIGINAL_COUNT]={{255,172,55,255},{120,205,255,255},
        {230,90,255,255},{100,230,180,255},{245,212,135,255}};
    Color ink=collected ? palette[id] : (Color){100,94,118,255};
    Color light=collected ? (Color){255,245,220,255} : (Color){137,130,150,255};
    switch(id) {
        case 0: // crown
            DrawRectangle(c.x-r*.7f,c.y+r*.2f,r*1.4f,r*.4f,ink);
            for(int i=-1;i<=1;i++) {
                float x=c.x+i*r*.48f;
                DrawTriangle((Vector2){x,c.y-r*(i==0?.75f:.45f)},
                    (Vector2){x-r*.26f,c.y+r*.28f},(Vector2){x+r*.26f,c.y+r*.28f},ink);
            }
            DrawCircleV((Vector2){c.x,c.y+r*.36f},r*.13f,light);break;
        case 1: // compass
            DrawRing(c,r*.63f,r*.74f,0,360,48,ink);
            DrawLineEx((Vector2){c.x-r*.85f,c.y},(Vector2){c.x+r*.85f,c.y},w,ink);
            DrawLineEx((Vector2){c.x,c.y-r*.85f},(Vector2){c.x,c.y+r*.85f},w,ink);
            DrawTriangle((Vector2){c.x+r*.38f,c.y-r*.56f},
                (Vector2){c.x-r*.2f,c.y-r*.05f},(Vector2){c.x+r*.14f,c.y+r*.17f},light);
            DrawTriangle((Vector2){c.x-r*.38f,c.y+r*.56f},
                (Vector2){c.x+r*.2f,c.y+r*.05f},(Vector2){c.x-r*.14f,c.y-r*.17f},ink);break;
        case 2: // faceted prism
            DrawPoly(c,4,r*.85f,0,ink);
            DrawLineEx((Vector2){c.x,c.y-r*.85f},(Vector2){c.x-r*.25f,c.y},w,light);
            DrawLineEx((Vector2){c.x-r*.25f,c.y},(Vector2){c.x,c.y+r*.85f},w,light);
            DrawLineEx((Vector2){c.x-r*.85f,c.y},(Vector2){c.x+r*.85f,c.y},w,light);break;
        case 3: // scarab with six legs and solar disc
            for(int side=-1;side<=1;side+=2) for(int i=-1;i<=1;i++)
                DrawLineEx((Vector2){c.x+side*r*.25f,c.y+i*r*.23f},
                    (Vector2){c.x+side*r*.78f,c.y+i*r*.46f},w,ink);
            DrawEllipse(c.x,c.y+r*.1f,r*.43f,r*.55f,ink);
            DrawCircleV((Vector2){c.x,c.y-r*.64f},r*.19f,light);
            DrawLineEx((Vector2){c.x,c.y-r*.28f},(Vector2){c.x,c.y+r*.58f},w,light);break;
        case 4: // hourglass
            DrawRectangle(c.x-r*.65f,c.y-r*.76f,r*1.3f,r*.16f,ink);
            DrawRectangle(c.x-r*.65f,c.y+r*.6f,r*1.3f,r*.16f,ink);
            DrawLineEx((Vector2){c.x-r*.5f,c.y-r*.6f},(Vector2){c.x+r*.5f,c.y+r*.6f},w,ink);
            DrawLineEx((Vector2){c.x+r*.5f,c.y-r*.6f},(Vector2){c.x-r*.5f,c.y+r*.6f},w,ink);
            DrawTriangle((Vector2){c.x,c.y+r*.12f},(Vector2){c.x-r*.38f,c.y+r*.56f},
                (Vector2){c.x+r*.38f,c.y+r*.56f},light);break;
    }
}

static const char *artifactNames[ARTIFACT_IDEAS]={
    "Ember Crown","Moon Compass","Void Prism","Sun Scarab","Ancient Hourglass",
    "Storm Core","Blood Chalice","Frost Lotus","Serpent Reliquary",
    "Astral Orrery","Bone Lantern","Abyss Anchor","Phoenix Quill",
    "Echo Mask","Gilded Eye","Thorn Heart","Eclipse Halo","Tide Pearl"
};
static const char *artifactNotes[ARTIFACT_IDEAS]={
    "Original crown design","Original compass design","Original prism design",
    "Original scarab design","Original hourglass design",
    "Lightning trapped in brass","Ruby vessel and dark flame","Crystal petals around ice",
    "Emerald held by a serpent","Tiny orbiting star machine","Cold fire in carved ivory",
    "Void-forged hooked anchor","A feather of living fire","Split porcelain, two voices",
    "A golden watcher's iris","Living ruby in iron vines","Black sun with silver corona",
    "Moonlit pearl in ocean glass"
};
static const Color artifactAccent[ARTIFACT_IDEAS]={
    {255,172,55,255},{120,205,255,255},{230,90,255,255},
    {100,230,180,255},{245,212,135,255},{124,218,255,255},
    {245,82,110,255},{142,237,255,255},{115,232,153,255},
    {255,208,128,255},{198,231,255,255},{157,117,247,255},
    {255,155,76,255},{184,221,231,255},{255,215,111,255},
    {228,80,112,255},{225,228,250,255},{111,226,221,255}
};

static void Box(int x,int y,int w,int h,Color c){DrawRectangle(x,y,w,h,c);}
static void Dot(int x,int y,int r,Color c){DrawCircle(x,y,r,c);}
static void Line(int x,int y,int u,int v,int w,Color c){
    DrawLineEx((Vector2){(float)x,(float)y},(Vector2){(float)u,(float)v},w,c);
}
static void Tri(int x1,int y1,int x2,int y2,int x3,int y3,Color c){
    float cross=(float)(x2-x1)*(y3-y1)-(float)(y2-y1)*(x3-x1);
    if(cross>0)DrawTriangle((Vector2){x1,y1},(Vector2){x3,y3},(Vector2){x2,y2},c);
    else DrawTriangle((Vector2){x1,y1},(Vector2){x2,y2},(Vector2){x3,y3},c);
}
static void Diamond(int x,int y,int r,Color c){
    Tri(x,y-r,x+r,y,x,y+r,c);Tri(x,y-r,x,y+r,x-r,y,c);
}
static void Ring(int x,int y,int r,int width,Color c){
    DrawRing((Vector2){x,y},r-width,r,0,360,36,c);
}
static void Spark(int x,int y,int r,Color c){
    Line(x-r,y,x+r,y,1,c);Line(x,y-r,x,y+r,1,c);Dot(x,y,1,c);
}
static void Backdrop(int bg){
    Box(0,0,32,32,C(11,9,20));
    if(bg==0){
        Box(1,1,30,30,C(29,25,42));
        Box(1,1,30,2,C(72,61,91));
        Box(2,28,28,2,C(18,16,32));
    }else if(bg==1){
        Box(1,1,30,30,C(56,48,67));
        Box(1,1,30,3,C(100,89,112));
        Box(1,1,3,30,C(88,76,101));
        Box(1,28,30,3,C(29,24,43));
    }else{
        Box(1,1,30,30,C(40,27,57));
        Box(3,3,26,26,C(28,22,43));
        DrawRectangleLines(2,2,28,28,C(127,82,177));
        Dot(5,5,1,C(235,190,255));Dot(26,5,1,C(235,190,255));
        Dot(5,26,1,C(235,190,255));Dot(26,26,1,C(235,190,255));
    }
}

static void DrawNewArtifact(int id,float t){
    float pulse=.5f+.5f*sinf(t*3.0f+id);
    Color a=artifactAccent[id];
    Dot(16,16,15,CA(a.r,a.g,a.b,(unsigned char)(20+25*pulse)));
    switch(id){
    case 5: // Storm Core
        Ring(16,16,13,2,C(107,117,150));Ring(16,16,10,2,C(213,187,116));
        for(int i=0;i<6;i++){
            float q=i*PI/3.0f;
            int x=16+(int)(cosf(q)*13),y=16+(int)(sinf(q)*13);
            Dot(x,y,2,C(188,216,228));
        }
        Line(8,18,13,9,2,C(118,231,255));
        Line(13,9,16,17,2,C(245,253,255));
        Line(16,17,22,11,2,C(118,231,255));
        Line(22,11,19,24,2,C(245,253,255));
        Spark(25,5,2,C(229,249,255));break;
    case 6: // Blood Chalice
        Ring(16,11,13,2,C(193,159,109));
        Tri(7,8,25,8,20,20,C(177,137,101));
        Tri(7,8,20,20,12,20,C(98,57,65));
        Box(9,8,14,3,C(229,74,91));Box(11,11,10,2,C(250,120,119));
        Line(16,20,16,26,3,C(226,180,111));
        Box(9,26,14,3,C(191,145,96));Dot(16,11,3,C(253,82,112));
        Spark(24,5,2,C(255,184,163));break;
    case 7: // Frost Lotus
        for(int i=0;i<8;i++){
            float q=i*PI/4.0f;
            int x=16+(int)(cosf(q)*12),y=16+(int)(sinf(q)*12);
            Tri(16,16,x+(int)(sinf(q)*3),y-(int)(cosf(q)*3),
                x-(int)(sinf(q)*3),y+(int)(cosf(q)*3),
                i&1?C(94,198,232):C(178,242,255));
        }
        Diamond(16,16,6,C(234,253,255));Diamond(16,16,3,C(117,215,250));
        Spark(4,5,2,C(224,249,255));break;
    case 8: // Serpent Reliquary
        Ring(16,16,12,3,C(64,117,86));
        DrawCircleSectorLines((Vector2){16,16},9,35,325,32,C(135,226,145));
        Dot(22,10,4,C(95,197,116));Dot(23,9,1,C(255,218,120));
        Line(24,12,28,14,1,C(242,112,119));
        Diamond(16,16,6,C(27,96,81));Diamond(16,16,3,C(131,250,190));
        Spark(7,25,2,C(157,251,184));break;
    case 9: // Astral Orrery
        Ring(16,16,13,1,C(187,148,98));Ring(16,16,9,2,C(244,207,138));
        Line(5,11,27,21,1,C(214,185,135));
        Line(6,23,25,8,1,C(150,198,224));
        Dot(16,16,5,C(45,91,125));Dot(15,14,2,C(151,230,249));
        Dot(26,10,3,C(248,222,160));Dot(7,24,2,C(180,207,249));
        Spark(7,6,2,C(255,244,200));break;
    case 10: // Bone Lantern
        Box(8,6,16,21,C(178,164,148));Box(10,8,12,17,C(20,32,47));
        for(int i=0;i<3;i++){Line(10+i*5,8,10+i*5,25,2,C(222,216,188));}
        Box(11,4,10,3,C(238,229,196));Box(9,27,14,2,C(185,167,144));
        Tri(16,10,11,22,21,22,C(90,208,249));
        Tri(16,14,13,22,19,22,C(218,248,255));
        Spark(26,6,2,C(179,237,255));break;
    case 11: // Abyss Anchor
        Ring(16,9,5,2,C(180,139,243));
        Line(16,13,16,25,4,C(90,66,153));
        Line(16,14,16,25,2,C(199,163,251));
        Line(5,18,27,18,3,C(146,106,215));
        Tri(16,29,8,24,7,17,C(116,74,188));
        Tri(16,29,24,24,25,17,C(84,53,148));
        Diamond(16,18,4,C(231,178,255));break;
    case 12: // Phoenix Quill
        Line(7,27,24,5,2,C(255,238,174));
        Tri(12,19,21,3,28,5,C(252,164,67));
        Tri(12,19,27,6,24,17,C(245,77,65));
        Tri(12,19,21,4,18,22,C(255,218,112));
        Line(13,19,23,7,1,C(255,248,200));
        Spark(26,3,2,C(255,234,147));
        Dot(7,27,2,C(196,74,56));break;
    case 13: // Echo Mask
        Ring(16,16,13,2,C(106,184,198));
        Tri(7,7,16,5,16,27,C(224,222,218));
        Tri(25,7,16,5,16,27,C(88,132,150));
        Tri(9,9,16,7,16,25,C(241,235,221));
        Tri(23,9,16,7,16,25,C(128,193,205));
        Dot(12,15,2,C(26,47,65));Dot(20,15,2,C(29,61,77));
        Line(13,22,19,22,1,C(40,71,84));
        Spark(27,5,2,C(171,243,248));break;
    case 14: // Gilded Eye
        Ring(16,16,14,2,C(177,127,62));
        Tri(3,16,16,8,29,16,C(242,193,97));
        Tri(3,16,16,24,29,16,C(211,145,71));
        DrawEllipse(16,16,10,6,C(38,45,57));
        Dot(16,16,5,C(244,210,113));Dot(16,16,2,C(30,29,42));
        Dot(14,14,1,C(255,250,200));
        for(int i=0;i<3;i++)Line(7+i*9,8,6+i*10,4,1,C(244,213,148));
        break;
    case 15: // Thorn Heart
        for(int i=0;i<6;i++){
            int y=6+i*4;
            Line(4,y,28,31-y,2,C(68,139,86));
        }
        Dot(12,12,6,C(178,57,85));Dot(20,12,6,C(226,70,92));
        Tri(6,14,26,14,16,27,C(213,58,85));
        Diamond(16,17,5,C(250,151,151));
        Spark(25,6,2,C(255,210,167));break;
    case 16: // Eclipse Halo
        Ring(16,16,14,2,C(216,217,237));
        for(int i=0;i<8;i++){
            float q=i*PI/4.0f;
            int x=16+(int)(cosf(q)*12),y=16+(int)(sinf(q)*12);
            Dot(x,y,1,C(252,223,170));
        }
        Dot(16,16,10,C(46,40,68));Dot(17,15,8,C(10,10,27));
        DrawCircleSectorLines((Vector2){16,16},11,220,340,22,C(241,228,190));
        Spark(27,6,2,C(243,240,255));break;
    case 17: // Tide Pearl
        Tri(4,23,9,8,16,27,C(42,123,150));
        Tri(28,23,23,8,16,27,C(80,195,201));
        Ring(16,17,12,2,C(95,224,218));
        Dot(16,16,8,C(99,181,191));Dot(16,15,6,C(222,246,232));
        Dot(13,12,2,C(255,255,248));
        Line(5,26,27,26,2,C(55,138,165));
        Spark(25,5,2,C(214,250,247));break;
    }
}

static void DrawArtifact(int id,float t){
    if(id<ORIGINAL_COUNT)DrawOriginalArtifact(id,(Vector2){16,16},25,true);
    else DrawNewArtifact(id,t);
}

static void ArchiveRatedDesign(int id) {
    if(id<0||id>=ARTIFACT_IDEAS)return;
    char directory[512],path[512];
    snprintf(directory,sizeof(directory),"%sLibrary",GetApplicationDirectory());
    MakeDirectory(directory);
    snprintf(path,sizeof(path),"%sLibrary/artifact_%02d.png",GetApplicationDirectory(),id);
    if(FileExists(path))return;
    RenderTexture2D target=LoadRenderTexture(32,32);
    if(target.id==0)return;
    SetTextureFilter(target.texture,TEXTURE_FILTER_POINT);
    BeginTextureMode(target);
    ClearBackground((Color){0,0,0,0});
    DrawArtifact(id,0.0f);
    EndTextureMode();
    Image image=LoadImageFromTexture(target.texture);
    if(image.data){ExportImage(image,path);UnloadImage(image);}
    UnloadRenderTexture(target);
}

// 0 = unrated, 1 = rejected, 2 = maybe/fix, 3 = accepted.
static void LoadRatings(int ratings[ARTIFACT_IDEAS]){
    for(int i=0;i<ARTIFACT_IDEAS;i++)ratings[i]=0;
    FILE *f=fopen("Library/artifact_ratings.txt","r");
    if(f){
        char line[160],state[24];
        int id;
        while(fgets(line,sizeof(line),f)){
            if(sscanf(line,"%d %23s",&id,state)!=2||id<0||id>=ARTIFACT_IDEAS)continue;
            if(strcmp(state,"REJECTED")==0)ratings[id]=1;
            else if(strcmp(state,"MAYBE/FIX")==0)ratings[id]=2;
            else if(strcmp(state,"ACCEPTED")==0)ratings[id]=3;
        }
        fclose(f);
        return;
    }
    f=fopen("Library/artifact_selections.txt","r");
    if(f){
        char line[160];
        int id;
        while(fgets(line,sizeof(line),f))
            if(sscanf(line,"%d",&id)==1&&id>=0&&id<ARTIFACT_IDEAS)ratings[id]=3;
        fclose(f);
    }
}
static void SaveRatings(const int ratings[ARTIFACT_IDEAS]){
    for(int i=0;i<ARTIFACT_IDEAS;i++)
        if(ratings[i]==2||ratings[i]==3)ArchiveRatedDesign(i);
    FILE *f=fopen("Library/artifact_ratings.txt","w");
    if(!f)return;
    const char *states[4]={"", "REJECTED", "MAYBE/FIX", "ACCEPTED"};
    for(int i=0;i<ARTIFACT_IDEAS;i++)if(ratings[i]>0&&ratings[i]<=3)
        fprintf(f,"%d %s %s\n",i,states[ratings[i]],artifactNames[i]);
    fclose(f);
}
static bool RatingButton(Rectangle rect,const char *label,int current,int value,Vector2 mouse){
    bool hot=CheckCollisionPointRec(mouse,rect),active=current==value;
    Color accent=value==1?C(247,104,122):(value==2?C(246,189,100):C(105,230,194));
    Color fill=active?(value==1?C(91,43,56):(value==2?C(102,75,42):C(39,91,77))):
        (hot?C(65,57,78):C(43,41,59));
    DrawRectangleRounded(rect,0.18f,5,fill);
    DrawRectangleRoundedLinesEx(rect,0.18f,5,active?2.0f:1.0f,
        active||hot?accent:C(81,74,96));
    int width=MeasureText(label,15);
    DrawText(label,(int)(rect.x+(rect.width-width)*0.5f),(int)rect.y+9,15,
        active?C(255,248,234):(hot?C(237,231,238):C(173,170,187)));
    return hot&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

int main(int argc,char **argv){
    int capturePage=-1;
    bool archiveRated=argc>1&&strcmp(argv[1],"--archive-rated")==0;
    if(argc>1&&strncmp(argv[1],"--capture-page=",15)==0)
        capturePage=atoi(argv[1]+15)-1;
    if(capturePage>=PAGE_COUNT)capturePage=PAGE_COUNT-1;
    SetConfigFlags(FLAG_MSAA_4X_HINT|FLAG_WINDOW_RESIZABLE);
    InitWindow(1240,780,"Artifact Designs - Dungeon Crawler");
    SetTargetFPS(60);
    char designDirectory[512];
    snprintf(designDirectory,sizeof(designDirectory),"%sLibrary",GetApplicationDirectory());
    MakeDirectory(designDirectory);
    ChangeDirectory(GetApplicationDirectory());
    RenderTexture2D previews[PAGE_SIZE][3];
    for(int i=0;i<PAGE_SIZE;i++)for(int bg=0;bg<3;bg++){
        previews[i][bg]=LoadRenderTexture(32,32);
        SetTextureFilter(previews[i][bg].texture,TEXTURE_FILTER_POINT);
    }
    int page=capturePage>=0?capturePage:0;
    int ratings[ARTIFACT_IDEAS];
    LoadRatings(ratings);
    for(int i=0;i<ARTIFACT_IDEAS;i++)
        if(ratings[i]==2||ratings[i]==3)ArchiveRatedDesign(i);
    if(archiveRated){
        for(int i=0;i<PAGE_SIZE;i++)for(int bg=0;bg<3;bg++)
            UnloadRenderTexture(previews[i][bg]);
        CloseWindow();
        return 0;
    }
    while(!WindowShouldClose()){
        float wheel=GetMouseWheelMove();
        if(IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_PAGE_DOWN)||wheel<0)
            page=(page+1)%PAGE_COUNT;
        if(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_PAGE_UP)||wheel>0)
            page=(page+PAGE_COUNT-1)%PAGE_COUNT;
        if(IsKeyPressed(KEY_ZERO)){
            for(int i=0;i<ARTIFACT_IDEAS;i++)ratings[i]=0;
            SaveRatings(ratings);
        }
        for(int i=0;i<PAGE_SIZE;i++)
            if(IsKeyPressed(KEY_ONE+i)||IsKeyPressed(KEY_KP_1+i)){
                int value=(IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL))?1:
                    ((IsKeyDown(KEY_LEFT_SHIFT)||IsKeyDown(KEY_RIGHT_SHIFT))?2:3);
                int id=page*PAGE_SIZE+i;
                ratings[id]=ratings[id]==value?0:value;
                SaveRatings(ratings);
            }
        float t=(float)GetTime();
        for(int i=0;i<PAGE_SIZE;i++)for(int bg=0;bg<3;bg++){
            BeginTextureMode(previews[i][bg]);
            Backdrop(bg);DrawArtifact(page*PAGE_SIZE+i,t);
            EndTextureMode();
        }
        int sw=GetScreenWidth(),sh=GetScreenHeight();
        float scale=fminf((float)sw/1240.0f,(float)sh/780.0f);
        if(scale<0.4f)scale=0.4f;
        int ox=(int)((sw-1240*scale)*.5f),oy=(int)((sh-780*scale)*.5f);
        Vector2 mouse=GetMousePosition();
        Vector2 local={(mouse.x-ox)/scale,(mouse.y-oy)/scale};
        int clicked=-1,clickedValue=0;
        BeginDrawing();
        ClearBackground(C(9,9,17));
        DrawRectangleGradientV(0,0,sw,sh,C(12,13,25),C(21,13,29));
        BeginMode2D((Camera2D){.offset={(float)ox,(float)oy},.target={0,0},.rotation=0,.zoom=scale});
        DrawText("ARTIFACT DESIGNS",42,23,29,C(244,223,187));
        DrawText("1-6: ACCEPT  |  SHIFT+1-6: MAYBE / FIX  |  CTRL+1-6: REJECT",
                 43,60,16,C(161,169,190));
        DrawText(TextFormat("PAGE %d / %d",page+1,PAGE_COUNT),1070,29,20,C(132,231,217));
        for(int i=0;i<PAGE_SIZE;i++){
            int id=page*PAGE_SIZE+i;
            int col=i%3,row=i/3,x=42+col*390,y=94+row*324;
            Rectangle card={(float)x,(float)y,374,310};
            bool hot=CheckCollisionPointRec(local,card);
            DrawRectangleRounded(card,.07f,6,hot?C(41,35,59):C(28,25,42));
            Color edge=ratings[id]==1?C(247,104,122):(ratings[id]==2?C(246,189,100):
                (ratings[id]==3?C(105,230,194):(hot?C(131,209,220):C(70,61,87))));
            DrawRectangleRoundedLinesEx(card,.07f,6,ratings[id]?3:1,edge);
            DrawText(TextFormat("%02d  %s",id+1,artifactNames[id]),x+18,y+13,19,
                     C(244,232,210));
            DrawText(artifactNotes[id],x+19,y+42,15,C(179,168,195));
            DrawText(id<ORIGINAL_COUNT?"ORIGINAL DESIGN":"ADDITIONAL DESIGN",
                     x+19,y+238,13,artifactAccent[id]);
            Rectangle src={0,0,32,-32},dst={(float)x+20,(float)y+72,160,160};
            DrawTexturePro(previews[i][0].texture,src,dst,(Vector2){0,0},0,WHITE);
            DrawRectangleLinesEx(dst,2,C(108,98,127));
            const char *labels[3]={"FLOOR","WALL","PAD"};
            for(int bg=0;bg<3;bg++){
                int px=x+213,py=y+73+bg*52;
                DrawTexturePro(previews[i][bg].texture,src,
                    (Rectangle){(float)px,(float)py,32,32},(Vector2){0,0},0,WHITE);
                DrawText(labels[bg],px+43,py+8,15,C(169,181,202));
            }
            DrawText("ACTUAL SIZE",x+212,y+235,13,C(128,223,215));
            const char *ratingLabels[3]={"REJECT","MAYBE / FIX","ACCEPT"};
            const int offsets[3]={16,123,242},widths[3]={100,112,116};
            for(int value=1;value<=3;value++){
                Rectangle button={(float)x+offsets[value-1],(float)y+261,
                                  (float)widths[value-1],34};
                if(RatingButton(button,ratingLabels[value-1],ratings[id],value,local)){
                    clicked=id;clickedValue=value;
                }
            }
        }
        DrawText("LEFT / RIGHT: page  |  0: clear ratings  |  ESC: close",42,747,15,
                 C(144,144,164));
        int totals[4]={0};
        for(int i=0;i<ARTIFACT_IDEAS;i++)totals[ratings[i]]++;
        DrawText(TextFormat("REJECTED %d   MAYBE / FIX %d   ACCEPTED %d",
                            totals[1],totals[2],totals[3]),760,747,15,C(228,199,140));
        EndMode2D();
        if(clicked>=0){
            ratings[clicked]=ratings[clicked]==clickedValue?0:clickedValue;
            SaveRatings(ratings);
        }
        if(capturePage>=0){
            char shot[512];
            snprintf(shot,sizeof(shot),"%s/Library/artifact_ideas_page_%d.png",
                     GetApplicationDirectory(),page+1);
            rlDrawRenderBatchActive();
            Image frame=LoadImageFromScreen();
            ExportImage(frame,shot);
            UnloadImage(frame);
        }
        EndDrawing();
        if(capturePage>=0)break;
    }
    for(int i=0;i<PAGE_SIZE;i++)for(int bg=0;bg<3;bg++)
        UnloadRenderTexture(previews[i][bg]);
    CloseWindow();
    return 0;
}
