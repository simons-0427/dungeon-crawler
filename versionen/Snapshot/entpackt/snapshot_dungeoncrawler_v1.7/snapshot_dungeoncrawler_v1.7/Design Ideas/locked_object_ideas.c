// Standalone raylib sprite gallery. Build from Snapshot in PowerShell with:
// $env:PATH='C:\raylib\w64devkit\bin;'+$env:PATH
// gcc -std=c99 -O2 -Wall -Wextra -o locked_object_ideas.exe locked_object_ideas.c -IC:/raylib/raylib/src -LC:/raylib/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm
// Each of the five gallery files is self-contained. No game files are changed.
#define GALLERY 3 // 0 enemy, 1 door, 2 key, 3 locked object, 4 ladder

#include "raylib.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define P(r,g,b) ((Color){r,g,b,255})
#define A(r,g,b,a) ((Color){r,g,b,a})
#define BOX(x,y,w,h,c) DrawRectangle((x),(y),(w),(h),(c))
#define DOT(x,y,r,c) DrawCircle((x),(y),(r),(c))
#define LINE(x,y,u,v,w,c) DrawLineEx((Vector2){x,y},(Vector2){u,v},w,c)
#define MAX_DESIGNS 16
#define PAGE_SIZE 6
#define OPEN_DURATION 0.50f

static const char *galleryName[5] = {"ARTIFACT ROOM ENEMIES", "DOORS", "UNIVERSAL KEYS", "LOCKED OBJECTS", "ARTIFACT LADDERS"};
static const char *legacyWinnerFile[5] = {"Library/artifact_enemy_winner.txt", "Library/door_winner.txt", "Library/key_winner.txt", "Library/locked_object_winner.txt", "Library/ladder_winner.txt"};
static const char *selectionFile[5] = {"Library/artifact_enemy_selections.txt", "Library/door_selections.txt", "Library/key_selections.txt", "Library/locked_object_selections.txt", "Library/ladder_selections.txt"};
static const char *ratingFile[5] = {"Library/artifact_enemy_ratings.txt", "Library/door_ratings.txt", "Library/key_ratings.txt", "Library/locked_object_ratings.txt", "Library/ladder_ratings.txt"};
static const char *previewFile[5] = {"artifact_enemies_preview.png", "door_preview.png", "key_preview.png", "locked_object_preview.png", "ladder_preview.png"};
static const char *names[5][MAX_DESIGNS] = {
    {"Original Arcane Orb", "Eclipse Reaver", "Bone Crown", "Rift Stalker", "Prism Sentinel", "Ash Seraph"},
    {"Original Gate Motif", "Obsidian Iris", "Blood Cathedral", "Astral Portal", "Bone Maw"},
    {"Tutorial Book Key", "Starforge", "Rift Thorn", "Bone Reliquary", "Sunbreaker"},
    {"Original Chest Motif", "Chain Vault", "Rune Reliquary", "Iron Idol", "Cursed Coffer",
     "Frostbite Reliquary", "Clockwork Heart", "Abyssal Treasure Pod", "Dragonbone Casket",
     "Mimic Seed", "Astral Star Cache", "Obsidian Sun Box", "Verdant Thorn Pod",
     "Portal Keystone", "Eclipse Iris Seal", "Rift Chain Anchor"},
    {"Original Ladder", "Crystal Descent", "Chain Ascent", "Bone Stair", "Sunspire", "Void Rungs"}
};
static const char *notes[5][MAX_DESIGNS] = {
    {"Current violet enemy", "Eclipse + fractured ring", "Horned bone mask", "Living tear in space", "Runic armored construct", "Burning halo and wings"},
    {"Current menu gate style", "Rotating black aperture", "Blood red gothic door", "Floating star seal", "Fanged living entrance", ""},
    {"Diagonal rear-cover key", "Forged star crown", "Shadow crystal blade", "Ivory ceremonial key", "Solar double-bit key", ""},
    {"Existing tutorial chest", "Heavy chain lock", "Crystal archive chest", "Face-shaped iron safe", "Thorned cursed box",
     "Glacial blue ice vault", "Brass gear heart and dial", "Deep-sea shell with coral", "Ribbed ivory bone case",
     "Living seed with a toothy latch", "Star-cut violet cache", "Magma cracks in black glass", "Vine-wrapped flower pod",
     "Runic stone sealing a portal", "Bladed iris over the portal core", "Crossed chains pinning a rift"},
    {"Current artifact ladder", "Cyan crystal shaft", "Suspended brass links", "Rib-cage ladder", "Golden light steps", "Space-time descent"}
};
static const int count[5] = {6,5,5,16,6};

static void Tri(float x1,float y1,float x2,float y2,float x3,float y3,Color c) {
    // raylib culls clockwise triangles; screen coordinates increase downward.
    float cross=(x2-x1)*(y3-y1)-(y2-y1)*(x3-x1);
    if(cross>0.0f) DrawTriangle((Vector2){x1,y1},(Vector2){x3,y3},(Vector2){x2,y2},c);
    else DrawTriangle((Vector2){x1,y1},(Vector2){x2,y2},(Vector2){x3,y3},c);
}
static void Diamond(float x,float y,float r,Color c) {
    Tri(x,y-r,x+r,y,x,y+r,c); Tri(x,y-r,x,y+r,x-r,y,c);
}
static void Ring(int x,int y,float r,float thick,Color c) {
    DrawRing((Vector2){(float)x,(float)y},r-thick,r,0,360,28,c);
}
static void Spark(int x,int y,int r,Color c) {
    LINE(x-r,y,x+r,y,1,c); LINE(x,y-r,x,y+r,1,c); DOT(x,y,1,c);
}
static void Keyhole(int x,int y) {
    Ring(x,y,4,1,P(239,193,103));
    DOT(x,y-1,1,P(6,8,15));
    Tri(x-1,y,x+1,y,x,y+4,P(5,7,15));
    DOT(x,y-1,1,P(9,9,18));
}
static void Backdrop(int bg) {
    BOX(0,0,32,32,P(12,10,18));
    if (bg==0) {
        BOX(1,1,30,30,P(28,24,40));
        BOX(1,1,30,1,A(91,78,118,110));
        BOX(2,29,28,1,A(3,2,9,130));
    } else if (bg==1) {
        BOX(1,1,30,30,P(50,45,66));
        BOX(1,1,30,3,P(86,78,107)); BOX(1,1,3,30,P(80,73,101));
        BOX(1,28,30,3,P(25,20,38)); BOX(28,1,3,30,P(28,23,43));
    } else {
        BOX(1,1,30,30,P(43,27,59));
        BOX(2,2,28,28,P(31,24,44));
        DrawRectangleLines(1,1,30,30,P(139,80,188));
        DOT(5,5,1,P(214,146,255)); DOT(26,5,1,P(214,146,255));
        DOT(5,26,1,P(214,146,255)); DOT(26,26,1,P(214,146,255));
    }
}

static void Enemy(int id,float t) {
    float pulse=0.5f+0.5f*sinf(t*3.1f);
    if(id==0) {
        DOT(16,16,15,A(65,220,255,(unsigned char)(35+40*pulse)));
        DOT(16,16,13,P(126,45,170)); DOT(13,12,8,A(219,128,255,140));
        DOT(12,14,3,P(255,245,145)); DOT(20,14,3,P(255,245,145));
        DOT(12,14,1,P(30,8,45)); DOT(20,14,1,P(30,8,45));
        LINE(0,10,6,12,1,P(110,220,255)); LINE(0,16,4,16,1,P(110,220,255));
        LINE(0,22,6,20,1,P(110,220,255));
    } else if(id==1) {
        DOT(16,16,15,A(255,116,64,50)); Ring(16,16,14,2,P(246,162,66));
        for(int i=0;i<8;i++) { float a=(float)i*PI/4; float x=16+cosf(a)*14, y=16+sinf(a)*14;
            Tri(x,y,x-cosf(a-0.38f)*5,y-sinf(a-0.38f)*5,x+cosf(a+0.38f)*5,y+sinf(a+0.38f)*5,P(246,162,66)); }
        DOT(16,16,11,P(6,9,19)); DOT(16,16,7,P(17,23,39));
        Diamond(16,16,6,P(252,71,121)); Diamond(16,16,3,P(255,229,162));
        Spark(4,4,2,P(252,179,97)); Spark(28,28,2,P(252,179,97));
    } else if(id==2) {
        DOT(16,17,13,A(161,76,255,65));
        Tri(4,16,5,2,13,9,P(221,203,163)); Tri(28,16,27,2,19,9,P(221,203,163));
        Tri(8,7,16,3,24,7,P(241,226,192));
        DOT(16,17,11,P(48,37,65));
        Tri(8,9,16,6,16,25,P(225,211,183)); Tri(24,9,16,6,16,25,P(190,175,157));
        BOX(8,15,6,3,P(13,8,21)); BOX(18,15,6,3,P(13,8,21));
        DOT(11,16,2,P(255,60,179)); DOT(21,16,2,P(255,60,179));
        Tri(14,21,18,21,16,28,P(16,9,24));
    } else if(id==3) {
        DOT(16,16,15,A(55,235,243,45));
        Tri(7,1,22,6,10,13,P(86,242,241)); Tri(21,6,29,15,18,12,P(188,253,251));
        Tri(8,11,24,9,15,27,P(3,7,17)); Tri(24,9,27,24,15,27,P(10,13,29));
        Tri(6,16,12,18,4,30,P(9,12,27)); Tri(21,20,28,17,29,31,P(9,12,27));
        LINE(14,5,18,12,2,P(42,225,247)); LINE(18,12,14,19,2,P(213,254,255));
        LINE(14,19,19,28,2,P(42,225,247));
        DOT(10,14,2,P(250,79,211)); DOT(23,14,2,P(250,79,211));
    } else if(id==4) {
        Ring(16,16,15,2,A(74,232,224,155));
        Tri(16,1,22,8,16,7,P(143,220,216)); Tri(16,1,10,8,16,7,P(87,144,164));
        Tri(31,16,24,22,25,16,P(87,144,164)); Tri(31,16,24,10,25,16,P(143,220,216));
        Tri(16,31,10,24,16,25,P(87,144,164)); Tri(16,31,22,24,16,25,P(143,220,216));
        Tri(1,16,8,10,7,16,P(143,220,216)); Tri(1,16,8,22,7,16,P(87,144,164));
        DOT(16,16,10,P(24,36,56)); Ring(16,16,10,2,P(170,221,207));
        Diamond(16,16,7,P(169,69,255)); Diamond(16,16,3,P(244,229,255));
    } else {
        DOT(16,15,14,A(255,92,54,55));
        Tri(2,24,12,6,13,19,P(218,120,74)); Tri(30,24,20,6,19,19,P(255,171,85));
        Tri(4,25,12,15,10,29,P(119,57,75)); Tri(28,25,20,15,22,29,P(119,57,75));
        Ring(16,13,11,2,P(252,196,99));
        DOT(16,17,8,P(37,24,42));
        Tri(11,12,16,5,21,12,P(254,199,113));
        BOX(9,16,5,2,P(255,237,177)); BOX(18,16,5,2,P(255,237,177));
        Diamond(16,22,3,P(255,103,83));
    }
}

static void Door(int id,bool open,float t) {
    float p=0.5f+0.5f*sinf(t*2.0f);
    BOX(2,1,28,31,P(12,10,21));
    BOX(4,3,24,28,P(3,5,12));
    if(open) {
        DOT(16,16,11,A(57,221,247,(unsigned char)(70+40*p)));
        Diamond(16,16,8,P(13,28,46)); Diamond(16,16,3,P(161,246,251));
        BOX(2,2,4,29,P(91,77,100)); BOX(26,2,4,29,P(91,77,100));
        BOX(3,2,26,3,P(172,151,138));
        return;
    }
    if(id==0) {
        BOX(5,4,22,26,P(20,15,32)); BOX(6,5,9,24,P(43,31,52)); BOX(17,5,9,24,P(34,25,45));
        for(int y=8;y<28;y+=8) { BOX(8,y,5,2,P(104,78,74)); BOX(19,y,5,2,P(104,78,74)); }
        BOX(14,5,3,23,P(10,8,18)); Ring(16,16,5,2,P(171,119,91));
        DOT(16,12,2,P(211,194,168)); BOX(14,14,4,3,P(211,194,168));
        Keyhole(16,23);
    } else if(id==1) {
        Ring(16,16,13,3,P(101,230,222)); Ring(16,16,10,1,P(181,249,237));
        for(int i=0;i<7;i++) {float a=i*2*PI/7+t*0.14f;
            float x=16+cosf(a)*8, y=16+sinf(a)*8;
            Tri(x,y,16+cosf(a+0.7f)*8,16+sinf(a+0.7f)*8,16,16,P(47,59,83));}
        DOT(16,16,4,P(10,18,31)); Keyhole(16,16);
    } else if(id==2) {
        BOX(5,7,22,23,P(87,22,40));
        for(int i=0;i<3;i++){int x=8+i*7; BOX(x,9,4,18,P(23,12,30));
            Tri(x,9,x+2,3,x+4,9,P(168,64,72));}
        BOX(6,27,20,2,P(245,151,94));
        DOT(16,13,5,P(12,8,23));
        LINE(7,15,25,15,2,P(201,90,94));
        Keyhole(16,20); Spark(16,5,2,P(255,214,148));
    } else if(id==3) {
        for(int i=0;i<4;i++){float a=i*PI/2+t*0.08f;
            Diamond(16+cosf(a)*10,16+sinf(a)*10,4,P(101,153,229));}
        Ring(16,16,11,2,P(160,203,251)); DOT(16,16,9,P(24,21,56));
        Diamond(16,16,7,P(128,70,222)); Diamond(16,16,4,P(22,26,51));
        Keyhole(16,16); Spark(6,6,2,P(240,222,255)); Spark(27,27,2,P(240,222,255));
    } else {
        DOT(16,16,13,P(96,83,73)); DOT(16,16,10,P(15,9,23));
        for(int i=0;i<4;i++){
            int x=7+i*6;
            Tri(x,7,x+3,7,x+2,14,P(232,216,178));
            Tri(x,25,x+3,25,x+2,18,P(232,216,178));
        }
        DOT(9,15,2,P(251,72,117)); DOT(23,15,2,P(251,72,117));
        BOX(13,15,6,5,P(29,18,35)); Keyhole(16,18);
    }
    BOX(2,2,3,29,P(104,85,112)); BOX(27,2,3,29,P(76,61,91));
    BOX(3,2,26,3,P(171,144,141));
}

static void Key(int id,float t) {
    float p=0.5f+0.5f*sinf(t*2.5f);
    DOT(16,16,13,A(239,192,106,(unsigned char)(22+22*p)));
    if(id==0) {
        // Rear-cover diagonal shaft, gold bow, teal diamond, three teeth.
        LINE(6,25,23,8,4,P(104,73,46)); LINE(6,24,23,7,2,P(252,215,132));
        Ring(23,8,6,2,P(242,196,103)); Ring(23,8,3,1,P(106,229,213));
        Diamond(23,8,2,P(119,245,231));
        for(int i=0;i<3;i++)LINE(7+i*3,24-i*3,10+i*3,27-i*3,2,P(252,215,132));
    } else if(id==1) {
        LINE(9,26,22,8,5,P(94,64,59)); LINE(9,26,22,8,3,P(255,206,116));
        Ring(22,8,5,2,P(254,207,101));
        for(int i=0;i<5;i++){float a=i*2*PI/5-PI/2;
            Tri(22+cosf(a)*5,8+sinf(a)*5,22+cosf(a)*9,8+sinf(a)*9,22+cosf(a+0.3f)*5,8+sinf(a+0.3f)*5,P(255,230,154));}
        Diamond(22,8,3,P(244,97,108));
        BOX(7,22,5,3,P(255,228,139)); BOX(9,25,5,3,P(255,228,139));
    } else if(id==2) {
        LINE(8,26,21,8,5,P(20,18,42)); LINE(8,26,21,8,2,P(173,81,249));
        Tri(17,10,24,1,27,12,P(95,240,245));
        Tri(17,10,27,12,21,16,P(32,131,194));
        Diamond(22,9,3,P(235,252,255));
        Tri(6,21,11,25,4,29,P(161,66,242)); Tri(9,24,14,27,8,31,P(107,218,244));
    } else if(id==3) {
        LINE(9,26,22,8,5,P(105,90,79)); LINE(9,26,22,8,3,P(232,218,180));
        Ring(22,8,6,2,P(239,223,185));
        Tri(17,7,22,2,27,7,P(244,232,201));
        BOX(6,22,5,2,P(241,228,195)); BOX(8,25,5,2,P(241,228,195));
        DOT(22,8,2,P(246,83,176));
    } else {
        LINE(8,26,22,8,5,P(103,55,36)); LINE(8,26,22,8,3,P(255,199,78));
        Ring(22,8,6,2,P(255,196,68));
        Tri(18,4,22,0,26,4,P(255,232,132));
        Tri(27,6,31,8,27,10,P(255,232,132));
        Diamond(22,8,3,P(255,95,52));
        BOX(5,22,6,3,P(255,224,101)); BOX(8,25,7,3,P(255,224,101));
    }
    Spark(27,25,2,P(255,235,178));
}

static void LockedClosed(int id,float t) {
    if(id==0) {
        BOX(3,10,26,20,P(10,8,20));
        BOX(4,11,24,18,P(126,96,82)); BOX(5,13,22,12,P(70,53,68));
        BOX(3,10,26,6,P(168,129,96)); BOX(6,11,20,2,P(228,181,113));
        BOX(8,17,3,10,P(193,150,100)); BOX(21,17,3,10,P(193,150,100));
        BOX(13,14,6,10,P(206,154,83)); Keyhole(16,19);
    } else if(id==1) {
        BOX(3,10,26,20,P(10,8,20));
        BOX(3,11,26,18,P(48,51,72)); BOX(5,13,22,14,P(19,22,38));
        for(int y=15;y<27;y+=5)LINE(5,y,27,y+3,2,P(155,164,186));
        LINE(6,10,26,29,3,P(170,169,168)); LINE(26,10,6,29,3,P(95,119,135));
        Ring(16,19,7,2,P(207,179,111)); Keyhole(16,19);
        DOT(6,10,2,P(250,198,104)); DOT(26,29,2,P(250,198,104));
    } else if(id==2) {
        BOX(3,10,26,20,P(10,8,20));
        BOX(4,10,24,19,P(26,39,53));
        for(int i=0;i<3;i++){Diamond(8+i*8,11,4,P(78,179,202));
            Diamond(8+i*8,25,3,P(78,179,202));}
        BOX(7,14,18,9,P(12,19,33));
        Diamond(16,18,7,P(152,87,227)); Diamond(16,18,4,P(31,26,56)); Keyhole(16,18);
        Spark(5,6,2,P(167,246,252));
    } else if(id==3) {
        BOX(3,10,26,20,P(10,8,20));
        BOX(3,9,26,20,P(75,67,76)); BOX(5,10,22,18,P(28,27,41));
        Tri(4,11,11,7,11,17,P(177,169,158)); Tri(28,11,21,7,21,17,P(177,169,158));
        BOX(9,15,5,2,P(251,116,73)); BOX(18,15,5,2,P(251,116,73));
        BOX(11,21,10,5,P(108,96,91)); Keyhole(16,22);
        BOX(4,26,24,3,P(161,130,94));
    } else if(id==4) {
        BOX(3,10,26,20,P(10,8,20));
        BOX(3,11,26,18,P(78,21,49)); BOX(5,13,22,14,P(26,12,32));
        Tri(4,12,9,1,11,13,P(129,51,70)); Tri(28,12,23,1,21,13,P(129,51,70));
        for(int i=0;i<4;i++){int x=5+i*6;
            Tri(x,27,x+2,21,x+4,27,P(236,172,158));}
        Ring(16,18,8,2,P(224,102,137));
        DOT(16,18,5,P(13,12,29)); Keyhole(16,18);
    } else if(id==5) {
        DOT(16,19,14,A(43,188,222,34));
        BOX(4,14,24,15,P(17,62,83)); BOX(6,16,20,11,P(28,117,142));
        Tri(4,15,7,5,12,15,P(131,231,239)); Tri(12,15,15,2,18,15,P(192,249,248));
        Tri(20,15,25,5,28,15,P(102,208,225));
        BOX(4,13,24,4,P(158,240,244)); BOX(6,13,20,2,P(224,255,251));
        BOX(7,18,18,7,P(13,42,63)); LINE(8,22,24,22,1,P(76,188,212));
        Diamond(16,20,5,P(45,176,227)); Diamond(16,20,2,P(195,250,255)); Keyhole(16,20);
        Spark(7,8,2,P(191,255,255)); Spark(25,9,2,P(130,232,249));
    } else if(id==6) {
        BOX(4,13,24,16,P(72,43,24)); BOX(6,15,20,12,P(121,76,34));
        BOX(3,10,26,7,P(153,102,42)); BOX(5,11,22,3,P(237,190,91));
        BOX(5,18,3,8,P(225,168,64)); BOX(24,18,3,8,P(170,107,41));
        Ring(9,19,6,2,P(239,188,81)); Ring(9,19,3,1,P(67,48,37));
        Ring(23,19,6,2,P(207,140,57)); Ring(23,19,3,1,P(67,48,37));
        Ring(16,19,8,2,P(255,211,103)); Ring(16,19,5,1,P(91,55,33));
        BOX(15,15,2,5,P(248,219,139)); LINE(16,19,20,17,1,P(255,226,135));
        Keyhole(16,23); DOT(6,12,1,P(255,234,154)); DOT(26,12,1,P(255,234,154));
        (void)t;
    } else if(id==7) {
        DOT(16,20,13,A(42,181,178,38));
        BOX(5,15,22,13,P(14,64,77)); BOX(7,17,18,9,P(20,112,118));
        for(int i=0;i<5;i++) {
            int x=7+i*4;
            Tri(x,15,x+2,5-(i==2?2:0),x+4,15,i%2?P(51,170,168):P(91,217,196));
            LINE(x+1,16,x+2,23,1,P(142,232,205));
        }
        BOX(4,14,24,3,P(101,209,188)); BOX(7,22,18,4,P(8,45,62));
        DOT(7,14,2,P(248,110,91)); DOT(25,14,2,P(255,154,104));
        DOT(16,19,5,P(37,155,178)); Diamond(16,19,3,P(182,248,222)); Keyhole(16,20);
        Tri(5,25,2,22,7,23,P(233,129,104)); Tri(27,25,25,22,30,23,P(233,129,104));
    } else if(id==8) {
        BOX(5,13,22,15,P(44,30,35)); BOX(8,16,16,10,P(84,45,51));
        for(int i=0;i<5;i++) {
            int x=7+i*4;
            LINE(x,15,x+1,25,2,i%2?P(210,202,174):P(244,230,193));
            DOT(x,15,2,P(242,226,188));
        }
        Tri(5,15,8,6,12,15,P(226,218,186)); Tri(27,15,24,6,20,15,P(183,179,163));
        BOX(7,13,18,3,P(218,209,177)); BOX(8,25,16,3,P(198,190,162));
        Diamond(16,20,6,P(141,37,54)); Diamond(16,20,3,P(255,91,77)); Keyhole(16,20);
        LINE(5,28,27,28,2,P(117,82,73)); (void)t;
    } else if(id==9) {
        DOT(16,19,14,A(115,206,56,34));
        BOX(5,13,22,15,P(56,100,43)); BOX(7,15,18,11,P(113,150,56));
        Tri(4,16,7,6,12,15,P(119,174,67)); Tri(28,16,25,6,20,15,P(73,143,66));
        BOX(6,13,20,4,P(146,184,69)); LINE(7,16,25,16,2,P(29,64,34));
        for(int i=0;i<5;i++) {
            int x=7+i*4;
            Tri(x,16,x+2,16,x+1,21,P(248,236,191));
        }
        DOT(9,11,2,P(255,77,91)); DOT(23,11,2,P(255,77,91));
        DOT(9,11,1,P(255,241,153)); DOT(23,11,1,P(255,241,153));
        BOX(12,21,8,6,P(54,89,45)); BOX(14,21,4,6,P(187,153,74)); Keyhole(16,23);
        Tri(4,24,2,21,7,22,P(163,211,81)); Tri(28,24,25,22,30,21,P(163,211,81));
    } else if(id==10) {
        DOT(16,19,15,A(127,92,255,32));
        Tri(4,17,9,7,16,12,P(61,45,112)); Tri(28,17,23,7,16,12,P(106,71,162));
        Tri(7,25,12,13,16,14,P(48,37,96)); Tri(25,25,20,13,16,14,P(82,58,132));
        BOX(8,15,16,12,P(30,24,67)); DrawRectangleLines(8,15,16,12,P(168,119,242));
        for(int i=0;i<4;i++) { float a=i*PI/2+t*0.12f; Spark((int)(16+cosf(a)*12),(int)(17+sinf(a)*10),1,P(251,218,139)); }
        Diamond(16,19,7,P(94,61,186)); Diamond(16,19,4,P(204,156,255));
        Ring(16,20,4,1,P(255,215,131)); Keyhole(16,20); Spark(16,5,2,P(255,232,157));
    } else if(id==11) {
        BOX(4,13,24,16,P(20,20,29)); BOX(6,15,20,12,P(4,8,17));
        Tri(4,14,8,8,13,14,P(63,61,76)); Tri(28,14,24,8,19,14,P(42,39,57));
        BOX(5,12,22,4,P(50,48,61)); BOX(7,13,18,1,P(145,130,139));
        LINE(8,16,12,20,2,P(226,51,54)); LINE(12,20,10,25,2,P(255,111,53));
        LINE(24,16,20,19,2,P(209,42,53)); LINE(20,19,22,25,2,P(255,127,49));
        Ring(16,20,8,2,P(175,43,54)); DOT(16,20,5,P(12,10,20));
        Diamond(16,20,3,P(255,115,42)); Keyhole(16,20);
        BOX(7,27,18,2,P(107,33,43));
    } else if(id==12) {
        DOT(16,20,14,A(88,203,94,30));
        BOX(7,14,18,15,P(31,81,49)); BOX(9,16,14,11,P(58,139,66));
        Tri(8,16,2,8,14,15,P(47,121,64)); Tri(24,16,30,8,18,15,P(90,162,73));
        Tri(9,24,3,27,12,18,P(39,109,59)); Tri(23,24,29,27,20,18,P(57,130,63));
        BOX(8,14,16,3,P(127,190,82));
        LINE(7,20,13,23,2,P(158,210,96)); LINE(25,20,19,23,2,P(111,188,80));
        for(int i=0;i<4;i++) { int x=6+i*7; Tri(x,16,x+2,11,x+4,16,P(196,224,125)); }
        DOT(16,20,6,P(132,49,111)); DOT(16,20,3,P(250,101,188));
        Keyhole(16,20); Spark(16,6,2,P(212,255,161));
    } else if(id==13) {
        DOT(16,16,14,A(57,220,211,39));
        Tri(3,9,16,1,29,9,P(38,49,63));
        Tri(7,9,16,3,25,9,P(124,151,150));
        BOX(4,9,24,19,P(35,46,56));
        BOX(6,10,20,16,P(69,89,96));
        BOX(9,11,14,14,P(11,22,35));
        BOX(3,12,4,13,P(111,139,139)); BOX(25,12,4,13,P(75,111,120));
        BOX(5,26,22,3,P(41,55,65)); BOX(8,27,16,1,P(124,161,151));
        for(int side=0;side<2;side++) {
            int x=side?24:8;
            LINE(x,13,x,23,1,P(134,229,205));
            DOT(x,12,1,P(238,227,168)); DOT(x,24,1,P(238,227,168));
        }
        Tri(16,8,11,15,16,13,P(84,207,189));
        Tri(16,8,21,15,16,13,P(167,241,211));
        Diamond(16,18,6,P(43,118,122));
        Diamond(16,18,4,P(157,233,211));
        Keyhole(16,18); Spark(16,5,2,P(231,244,180));
    } else if(id==14) {
        DOT(16,16,15,A(199,110,236,34));
        DOT(16,16,13,P(35,33,59)); Ring(16,16,13,3,P(121,120,161));
        Ring(16,16,10,1,P(219,190,223));
        for(int i=0;i<6;i++) {
            float a=(float)i*PI/3.0f;
            int x=(int)(16+cosf(a)*8), y=(int)(16+sinf(a)*8);
            Tri(x,y,(float)(16+cosf(a+0.55f)*11),
                (float)(16+sinf(a+0.55f)*11),
                (float)(16+cosf(a+1.05f)*4),
                (float)(16+sinf(a+1.05f)*4),
                i%2?P(92,75,139):P(151,126,177));
        }
        DOT(16,16,6,P(9,12,31)); Ring(16,16,6,1,P(245,205,139));
        Diamond(16,16,3,P(230,145,245)); Keyhole(16,16);
        for(int i=0;i<4;i++) {
            int x=i%2?27:5, y=i/2?27:5;
            DOT(x,y,2,P(56,58,80)); DOT(x,y,1,P(250,216,155));
        }
        BOX(1,14,3,4,P(142,119,150)); BOX(28,14,3,4,P(142,119,150));
    } else {
        DOT(16,16,14,A(167,74,222,45));
        BOX(8,3,16,26,P(41,27,58));
        BOX(10,5,12,22,P(7,8,28));
        BOX(12,7,8,18,P(53,22,82));
        LINE(7,3,7,29,3,P(133,78,81)); LINE(25,3,25,29,3,P(92,61,93));
        LINE(7,5,25,27,3,P(85,66,82));
        LINE(7,5,25,27,1,P(218,168,130));
        LINE(25,5,7,27,3,P(85,66,82));
        LINE(25,5,7,27,1,P(226,177,138));
        for(int i=0;i<4;i++) {
            Diamond(9+i*5,7+i*5,2,P(184,139,119));
            Diamond(23-i*5,7+i*5,2,P(192,150,123));
        }
        BOX(11,12,10,9,P(53,42,53));
        BOX(12,13,8,7,P(180,102,93));
        Diamond(16,16,4,P(244,156,119)); Keyhole(16,16);
        DOT(16,3,2,P(218,172,110)); DOT(16,29,2,P(218,172,110));
    }
}

static const Color lockedAccent[MAX_DESIGNS] = {
        {249,214,129,255}, {196,208,230,255}, {104,210,237,255}, {251,116,73,255},
        {244,128,167,255}, {132,239,248,255}, {255,211,103,255}, {118,237,205,255},
        {255,91,77,255}, {188,231,100,255}, {220,174,255,255}, {255,115,62,255},
        {250,130,195,255}, {131,239,214,255}, {229,164,249,255},
        {248,174,130,255}
    };
static void LockedOpened(int id,float progress,float t) {
    float lid = fminf(1.0f,fmaxf(0.0f,(progress-0.08f)/0.20f));
    int lift=(int)(lid*7.0f);
    Color glow=lockedAccent[id];
    if(id==13) {
        int spread=(int)(lid*5.0f);
        BOX(6,9,20,20,P(29,45,56));
        BOX(10,11,12,15,P(4,15,31));
        BOX(15,10,2,18,A(glow.r,glow.g,glow.b,(unsigned char)(90+130*lid)));
        Tri(16-spread,3,6-spread,10,16-spread,13,P(111,145,147));
        Tri(16+spread,3,26+spread,10,16+spread,13,P(80,120,128));
        BOX(5-spread,12,8,15,P(60,89,101));
        BOX(19+spread,12,8,15,P(46,73,88));
        LINE(8-spread,14,8-spread,23,1,glow);
        LINE(24+spread,14,24+spread,23,1,glow);
        Diamond(16,18-(int)(lid*3),3,P(229,250,210));
        return;
    }
    if(id==14) {
        DOT(16,16,14,A(glow.r,glow.g,glow.b,65));
        Ring(16,16,13,2,P(163,145,177));
        for(int i=0;i<6;i++) {
            float a=i*PI/3.0f+t*0.3f;
            int x=(int)(16+cosf(a)*(7+lid*5));
            int y=(int)(16+sinf(a)*(7+lid*5));
            Diamond(x,y,3,P(158,126,183));
        }
        DOT(16,16,(int)(4+lid*6),P(8,10,30));
        Ring(16,16,5+lid*6,1,glow);
        Spark(16,16,2,P(255,244,210));
        return;
    }
    if(id==15) {
        int spread=(int)(lid*5.0f);
        BOX(9,4,14,25,P(31,20,51));
        BOX(12,6,8,20,P(11,9,35));
        LINE(7-spread,3,7-spread,29,2,P(134,83,91));
        LINE(25+spread,3,25+spread,29,2,P(134,83,91));
        LINE(6-spread,5,13-spread,14,2,P(229,177,137));
        LINE(26+spread,5,19+spread,14,2,P(229,177,137));
        LINE(6-spread,27,13-spread,18,2,P(229,177,137));
        LINE(26+spread,27,19+spread,18,2,P(229,177,137));
        Diamond(16,16,4+(int)(lid*3),glow);
        Diamond(16,16,2,P(255,237,214));
        return;
    }
    BOX(4,16,24,13,P(31,25,39)); BOX(6,18,20,7,P(9,9,20));
    BOX(5,24,22,4,P(76,57,65)); BOX(4,23,24,2,P(173,128,87));
    BOX(4,5-lift,24,7,P(106,78,82));
    BOX(5,5-lift,22,2,P(218,177,121));
    BOX(6,8-lift,20,2,P(55,42,60));
    LINE(5,5-lift,3,2-lift,1,glow); LINE(27,5-lift,29,2-lift,1,glow);
    BOX(8,18,16,4,A(glow.r,glow.g,glow.b,(unsigned char)(100+110*lid)));
    Diamond(16,(int)(20.0f-lid*5.0f),4,glow);
    Diamond(16,(int)(20.0f-lid*5.0f),2,P(255,245,211));
    (void)t;
}

static void LockedUnlockFx(float progress,float t,Color accent) {
    float ringRadius=3.0f+progress*13.0f;
    unsigned char alpha=(unsigned char)(180.0f*(1.0f-progress));
    Ring(16,19,ringRadius,1.0f,A(accent.r,accent.g,accent.b,alpha));
    Spark(16,19,2,P(255,246,208));
    for(int i=0;i<4;i++) {
        float a=t*7.0f+i*PI/2.0f;
        float r=5.0f+progress*13.0f;
        int x=(int)(16.0f+cosf(a)*r);
        int y=(int)(19.0f+sinf(a)*r*0.72f-progress*4.0f);
        Spark(x,y,1,A(accent.r,accent.g,accent.b,(unsigned char)(220.0f*(1.0f-progress))));
    }
}

static void Ladder(int id,float t) {
    float p=0.5f+0.5f*sinf(t*2.0f);
    if(id==0) {
        DOT(16,16,15,A(176,72,255,(unsigned char)(32+38*p)));
        BOX(1,1,30,30,P(55,34,72)); DrawRectangleLines(1,1,30,30,P(231,143,255));
        DrawRectangleLines(3,3,26,26,P(118,77,151));
        BOX(6,5,20,22,P(5,4,11)); DrawRectangleLines(6,5,20,22,P(155,92,195));
        BOX(9,7,3,16,P(210,177,123)); BOX(20,7,3,16,P(210,177,123));
        for(int i=0;i<4;i++)BOX(11,9+i*4,10,2,P(245,214,155));
        LINE(11,23,16,27,2,P(220,130,255)); LINE(16,27,21,23,2,P(220,130,255));
        DOT(5,5,1,P(255,220,170)); DOT(26,5,1,P(255,220,170));
        DOT(5,26,1,P(255,220,170)); DOT(26,26,1,P(255,220,170));
    } else if(id==1) {
        BOX(4,2,24,28,P(5,15,29)); Ring(16,16,14,2,P(98,213,230));
        Tri(5,4,10,2,11,27,P(82,202,221)); Tri(27,4,22,2,21,27,P(157,251,246));
        for(int i=0;i<4;i++){
            int y=7+i*5; Diamond(16,y,5,P(87,173,218)); Diamond(16,y,2,P(189,253,246));
        }
        BOX(8,5,2,23,P(239,245,235)); BOX(22,5,2,23,P(239,245,235));
    } else if(id==2) {
        BOX(4,2,24,28,P(11,10,22));
        for(int side=0;side<2;side++) for(int i=0;i<6;i++){
            int x=side?24:8, y=4+i*5;
            Ring(x,y,3,1,P(202,172,112));
        }
        for(int i=0;i<5;i++){
            int y=7+i*5;
            BOX(9,y,14,3,P(164,137,94)); BOX(10,y,12,1,P(239,214,150));
        }
        Ring(16,16,13,1,P(89,79,116));
    } else if(id==3) {
        BOX(3,2,26,28,P(18,16,25));
        BOX(8,4,3,24,P(235,222,185)); BOX(21,4,3,24,P(186,176,162));
        for(int i=0;i<5;i++){
            int y=6+i*5;
            DOT(16,y,5,P(217,206,175)); BOX(12,y,8,2,P(245,232,198));
        }
        Tri(5,5,11,1,11,10,P(204,191,175)); Tri(27,5,21,1,21,10,P(204,191,175));
        DOT(16,29,2,P(242,104,175));
    } else if(id==4) {
        BOX(4,2,24,28,P(30,22,31));
        Tri(16,0,9,7,23,7,P(250,209,103));
        BOX(7,7,3,22,P(186,133,72)); BOX(22,7,3,22,P(249,201,98));
        for(int i=0;i<4;i++){
            int y=9+i*5;
            BOX(10,y,12,3,P(250,205,111)); BOX(11,y,10,1,P(255,245,179));
        }
        DOT(16,4,2,P(255,247,202));
        LINE(5,28,27,28,2,P(238,167,66));
    } else {
        BOX(3,2,26,28,P(6,7,21));
        Ring(16,16,13,2,P(131,91,225));
        for(int i=0;i<5;i++){
            int y=6+i*5;
            LINE(9,y,23,y,2,P(94,235,235));
            Diamond(9,y,2,P(206,245,255)); Diamond(23,y,2,P(206,245,255));
        }
        LINE(8,4,7,28,2,P(167,93,247)); LINE(24,4,25,28,2,P(167,93,247));
        Diamond(16,16,3,P(237,147,255));
    }
}

static void Sprite(int id,int bg,bool open,float t,float openingProgress) {
    Backdrop(bg);
    if(GALLERY==0) Enemy(id,t);
    else if(GALLERY==1) Door(id,open,t);
    else if(GALLERY==2) Key(id,t);
    else if(GALLERY==3) {
        if(openingProgress>=1.0f)return;
        if(openingProgress<0.0f) {
            LockedClosed(id,t);
            return;
        }
        float shake=openingProgress<0.22f?
            sinf(openingProgress*57.0f)*(1.0f-openingProgress/0.22f)*1.1f:0.0f;
        float rotation=openingProgress<0.20f?
            sinf(openingProgress*49.0f)*4.0f*(1.0f-openingProgress/0.20f):0.0f;
        float shrink=openingProgress<0.30f?1.0f+0.045f*sinf(openingProgress*20.0f):
            1.0f-0.92f*((openingProgress-0.30f)/0.70f);
        if(shrink<0.08f)shrink=0.08f;
        Camera2D camera={.offset={16,16},.target={16.0f-shake,16},.rotation=rotation,.zoom=shrink};
        BeginMode2D(camera);
        if(openingProgress<0.07f)LockedClosed(id,t);
        else LockedOpened(id,openingProgress,t);
        EndMode2D();
        LockedUnlockFx(openingProgress,t,lockedAccent[id]);
    } else Ladder(id,t);
}

static void ArchiveRatedDesign(int id) {
    if(id<0||id>=count[GALLERY]||GALLERY!=3)return;
    char directory[512];
    snprintf(directory,sizeof(directory),"%sLibrary",GetApplicationDirectory());
    MakeDirectory(directory);
    char path[512];
    snprintf(path,sizeof(path),"%sLibrary/locked_%02d.png",GetApplicationDirectory(),id);
    if(FileExists(path))return;
    RenderTexture2D target=LoadRenderTexture(32,32);
    if(target.id==0)return;
    SetTextureFilter(target.texture,TEXTURE_FILTER_POINT);
    BeginTextureMode(target);
    ClearBackground((Color){0,0,0,0});
    LockedClosed(id,0.0f);
    EndTextureMode();
    Image image=LoadImageFromTexture(target.texture);
    if(image.data){ExportImage(image,path);UnloadImage(image);}
    UnloadRenderTexture(target);
}

// 0 = unrated, 1 = rejected, 2 = maybe/fix, 3 = accepted.
static void LoadRatings(int ratings[MAX_DESIGNS]) {
    for(int i=0;i<MAX_DESIGNS;i++)ratings[i]=0;
    FILE *f=fopen(ratingFile[GALLERY],"r");
    if(f){
        char line[160],state[24];
        int id;
        while(fgets(line,sizeof(line),f)){
            if(sscanf(line,"%d %23s",&id,state)!=2||id<0||id>=count[GALLERY])continue;
            if(strcmp(state,"REJECTED")==0)ratings[id]=1;
            else if(strcmp(state,"MAYBE/FIX")==0)ratings[id]=2;
            else if(strcmp(state,"ACCEPTED")==0)ratings[id]=3;
        }
        fclose(f);
        return;
    }
    f=fopen(selectionFile[GALLERY],"r");
    if(f){
        char line[160];
        int id;
        while(fgets(line,sizeof(line),f))
            if(sscanf(line,"%d",&id)==1&&id>=0&&id<count[GALLERY])ratings[id]=3;
        fclose(f);
        return;
    }
    f=fopen(legacyWinnerFile[GALLERY],"r");
    if(f){
        int id=-1;
        if(fscanf(f,"%d",&id)==1&&id>=0&&id<count[GALLERY])ratings[id]=3;
        fclose(f);
    }
}
static void SaveRatings(const int ratings[MAX_DESIGNS]) {
    for(int i=0;i<count[GALLERY];i++)
        if(ratings[i]==2||ratings[i]==3)ArchiveRatedDesign(i);
    FILE *f=fopen(ratingFile[GALLERY],"w");
    if(!f)return;
    const char *states[4]={"", "REJECTED", "MAYBE/FIX", "ACCEPTED"};
    for(int i=0;i<count[GALLERY];i++)if(ratings[i]>0&&ratings[i]<=3)
        fprintf(f,"%d %s %s\n",i,states[ratings[i]],names[GALLERY][i]);
    fclose(f);
}
static bool RatingButton(Rectangle rect,const char *label,int current,int value,Vector2 mouse){
    bool hot=CheckCollisionPointRec(mouse,rect),active=current==value;
    Color accent=value==1?P(247,104,122):(value==2?P(246,189,100):P(105,230,194));
    Color fill=active?(value==1?P(91,43,56):(value==2?P(102,75,42):P(39,91,77))):
        (hot?P(65,57,78):P(43,41,59));
    DrawRectangleRounded(rect,0.18f,5,fill);
    DrawRectangleRoundedLinesEx(rect,0.18f,5,active?2.0f:1.0f,
        active||hot?accent:P(81,74,96));
    int width=MeasureText(label,15);
    DrawText(label,(int)(rect.x+(rect.width-width)*0.5f),(int)rect.y+9,15,
        active?P(255,248,234):(hot?P(237,231,238):P(173,170,187)));
    return hot&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

int main(int argc,char **argv) {
    bool capture=argc>1 && strncmp(argv[1],"--capture",9)==0;
    bool captureOpen=argc>1 && strcmp(argv[1],"--capture-open")==0;
    bool archiveRated=argc>1 && strcmp(argv[1],"--archive-rated")==0;
    int page=0;
    if(argc>1 && strncmp(argv[1],"--capture-page=",15)==0) {
        int requestedPage=atoi(argv[1]+15);
        if(requestedPage>0)page=requestedPage-1;
    }
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1240,780,galleryName[GALLERY]);
    SetTargetFPS(60);
    ChangeDirectory(GetApplicationDirectory());
    char designDirectory[512];
    snprintf(designDirectory,sizeof(designDirectory),"%sLibrary",GetApplicationDirectory());
    MakeDirectory(designDirectory);
    int pageCount=(count[GALLERY]+PAGE_SIZE-1)/PAGE_SIZE;
    if(page>=pageCount)page=pageCount-1;
    RenderTexture2D previews[MAX_DESIGNS][3];
    for(int i=0;i<count[GALLERY];i++)for(int bg=0;bg<3;bg++){
        previews[i][bg]=LoadRenderTexture(32,32);
        SetTextureFilter(previews[i][bg].texture,TEXTURE_FILTER_POINT);
    }
    int ratings[MAX_DESIGNS];
    LoadRatings(ratings);
    for(int i=0;i<count[GALLERY];i++)
        if(ratings[i]==2||ratings[i]==3)ArchiveRatedDesign(i);
    if(archiveRated){
        for(int i=0;i<count[GALLERY];i++)for(int bg=0;bg<3;bg++)
            UnloadRenderTexture(previews[i][bg]);
        CloseWindow();
        return 0;
    }
    bool open=false;
    double openStartedAt=captureOpen?GetTime()-0.20:-1.0;
    while(!WindowShouldClose()) {
        if((IsKeyPressed(KEY_K) || IsKeyPressed(KEY_SPACE)) && (GALLERY==1 || GALLERY==3)) {
            if(GALLERY==1)open=!open;
            else openStartedAt=GetTime();
        }
        if(GALLERY==3 && IsKeyPressed(KEY_R))openStartedAt=-1.0;
        float wheel=GetMouseWheelMove();
        if(IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_PAGE_DOWN)||wheel<0.0f)
            page=(page+1)%pageCount;
        if(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_PAGE_UP)||wheel>0.0f)
            page=(page+pageCount-1)%pageCount;
        if(IsKeyPressed(KEY_ZERO)) {
            for(int i=0;i<count[GALLERY];i++)ratings[i]=0;
            SaveRatings(ratings);
        }
        int visibleCount=count[GALLERY]-page*PAGE_SIZE;
        if(visibleCount>PAGE_SIZE)visibleCount=PAGE_SIZE;
        for(int i=0;i<visibleCount;i++) if(IsKeyPressed(KEY_ONE+i) || IsKeyPressed(KEY_KP_1+i)){
            int id=page*PAGE_SIZE+i;
            int value=(IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL))?1:
                ((IsKeyDown(KEY_LEFT_SHIFT)||IsKeyDown(KEY_RIGHT_SHIFT))?2:3);
            ratings[id]=ratings[id]==value?0:value;
            SaveRatings(ratings);
        }
        float t=(float)GetTime();
        float openingProgress=openStartedAt<0.0?-1.0f:
            (t-(float)openStartedAt>=OPEN_DURATION?1.0f:(t-(float)openStartedAt)/OPEN_DURATION);
        for(int i=0;i<count[GALLERY];i++)for(int bg=0;bg<3;bg++){
            BeginTextureMode(previews[i][bg]);
            Sprite(i,bg,open,t,openingProgress);
            EndTextureMode();
        }
        int sw=GetScreenWidth(),sh=GetScreenHeight();
        float scale=fminf((float)sw/1240.0f,(float)sh/780.0f);
        if(scale<0.4f)scale=0.4f;
        int ox=(int)((sw-1240*scale)*0.5f),oy=(int)((sh-780*scale)*0.5f);
        Vector2 mouse=GetMousePosition();
        int clicked=-1,clickedValue=0;
        BeginDrawing();
        ClearBackground(P(9,9,17));
        DrawRectangleGradientV(0,0,sw,sh,P(12,13,25),P(21,13,29));
        BeginMode2D((Camera2D){.offset={(float)ox,(float)oy},.target={0,0},.rotation=0,.zoom=scale});
        DrawText(galleryName[GALLERY],42,25,30,P(242,221,181));
        DrawText(TextFormat("1-%d ACCEPT  |  SHIFT MAYBE / FIX  |  CTRL REJECT",visibleCount),43,61,15,P(157,164,186));
        if(GALLERY==1) DrawText("K / SPACE: use key  |  same key opens both",730,61,16,P(119,224,215));
        else if(GALLERY==3) {
            const char *actionHint=openStartedAt<0.0?"K / SPACE: open | R: restore objects":
                (t-(float)openStartedAt>=OPEN_DURATION?
                 "OPENED | R: restore objects | K / SPACE: replay":
                 "OPENING | R: restore objects");
            DrawText(actionHint,650,61,15,P(119,224,215));
        }
        else if(GALLERY==2) DrawText("One key opens doors and locked objects",830,61,16,P(119,224,215));
        Vector2 local={(mouse.x-ox)/scale,(mouse.y-oy)/scale};
        int mainBg=(GALLERY==1)?1:((GALLERY==4)?2:0);
        for(int slot=0;slot<visibleCount;slot++){
            int id=page*PAGE_SIZE+slot;
            int col=slot%3,row=slot/3,x=42+col*390,y=94+row*324;
            Rectangle hit={(float)x,(float)y,374,310};
            bool hot=CheckCollisionPointRec(local,hit);
            DrawRectangleRounded(hit,0.07f,6,hot?P(40,34,58):P(28,25,42));
            Color edge=ratings[id]==1?P(247,104,122):(ratings[id]==2?P(246,189,100):
                (ratings[id]==3?P(105,230,194):(hot?P(129,207,217):P(68,59,84))));
            DrawRectangleRoundedLinesEx(hit,0.07f,6,ratings[id]?3.0f:1.0f,edge);
            DrawText(TextFormat("%02d  %s",id+1,names[GALLERY][id]),x+18,y+14,20,P(242,230,205));
            DrawText(notes[GALLERY][id],x+19,y+42,15,P(174,163,191));
            Rectangle src={0,0,32,-32},dst={(float)x+20,(float)y+73,160,160};
            DrawTexturePro(previews[id][mainBg].texture,src,dst,(Vector2){0,0},0,WHITE);
            DrawRectangleLinesEx(dst,2,P(108,98,127));
            const char *bgLabel[3]={"FLOOR","WALL","PAD"};
            for(int bg=0;bg<3;bg++){
                int px=x+213,py=y+73+bg*52;
                DrawTexturePro(previews[id][bg].texture,src,(Rectangle){(float)px,(float)py,32,32},(Vector2){0,0},0,WHITE);
                DrawText(bgLabel[bg],px+43,py+8,15,P(169,181,202));
            }
            DrawText("ACTUAL SIZE",x+212,y+235,13,P(128,223,215));
            const char *labels[3]={"REJECT","MAYBE / FIX","ACCEPT"};
            const int offsets[3]={16,123,242},widths[3]={100,112,116};
            for(int value=1;value<=3;value++){
                Rectangle button={(float)x+offsets[value-1],(float)y+261,
                                  (float)widths[value-1],34};
                if(RatingButton(button,labels[value-1],ratings[id],value,local)){
                    clicked=id;clickedValue=value;
                }
            }
        }
        int totals[4]={0};
        for(int i=0;i<count[GALLERY];i++)totals[ratings[i]]++;
        DrawText(TextFormat("REJECTED %d    MAYBE / FIX %d    ACCEPTED %d    |    0: clear ratings    ESC: close",
                            totals[1],totals[2],totals[3]),42,748,14,P(164,166,184));
        DrawText(TextFormat("PAGE %d / %d    LEFT / RIGHT OR WHEEL",page+1,pageCount),900,748,14,P(128,223,215));
        EndMode2D();
        if(clicked>=0){
            ratings[clicked]=ratings[clicked]==clickedValue?0:clickedValue;
            SaveRatings(ratings);
        }
        if(capture) {
            char shot[512];
            if(captureOpen)snprintf(shot,sizeof(shot),"%s/Library/locked_object_open_animation_preview_page_%d.png",
                                    GetApplicationDirectory(),page+1);
            else if(page==0)snprintf(shot,sizeof(shot),"%s/Library/%s",GetApplicationDirectory(),previewFile[GALLERY]);
            else snprintf(shot,sizeof(shot),"%s/Library/locked_object_preview_page_%d.png",
                          GetApplicationDirectory(),page+1);
            rlDrawRenderBatchActive();
            Image frame=LoadImageFromScreen();
            ExportImage(frame,shot);
            UnloadImage(frame);
        }
        EndDrawing();
        if(capture)break;
    }
    for(int i=0;i<count[GALLERY];i++)for(int bg=0;bg<3;bg++)UnloadRenderTexture(previews[i][bg]);
    CloseWindow();
    return 0;
}
