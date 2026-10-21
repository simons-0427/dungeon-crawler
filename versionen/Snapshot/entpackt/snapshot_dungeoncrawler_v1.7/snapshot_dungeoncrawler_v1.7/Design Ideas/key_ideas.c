// Standalone raylib sprite gallery. Build from Snapshot in PowerShell with:
// $env:PATH='C:\raylib\w64devkit\bin;'+$env:PATH
// gcc -std=c99 -O2 -Wall -Wextra -o key_ideas.exe key_ideas.c -IC:/raylib/raylib/src -LC:/raylib/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm
// Each of the five gallery files is self-contained. No game files are changed.
#define GALLERY 2 // 0 enemy, 1 door, 2 key, 3 locked object, 4 ladder

#include "raylib.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define P(r,g,b) ((Color){r,g,b,255})
#define A(r,g,b,a) ((Color){r,g,b,a})
#define BOX(x,y,w,h,c) DrawRectangle((x),(y),(w),(h),(c))
#define DOT(x,y,r,c) DrawCircle((x),(y),(r),(c))
#define LINE(x,y,u,v,w,c) DrawLineEx((Vector2){x,y},(Vector2){u,v},w,c)
#define MAX_DESIGNS 16
#define PAGE_SIZE 6

static const char *galleryName[5] = {"ARTIFACT ROOM ENEMIES", "DOORS", "UNIVERSAL KEYS", "LOCKED OBJECTS", "ARTIFACT LADDERS"};
static const char *legacyWinnerFile[5] = {"Library/artifact_enemy_winner.txt", "Library/door_winner.txt", "Library/key_winner.txt", "Library/locked_object_winner.txt", "Library/ladder_winner.txt"};
static const char *selectionFile[5] = {"Library/artifact_enemy_selections.txt", "Library/door_selections.txt", "Library/key_selections.txt", "Library/locked_object_selections.txt", "Library/ladder_selections.txt"};
static const char *ratingFile[5] = {"Library/artifact_enemy_ratings.txt", "Library/door_ratings.txt", "Library/key_ratings.txt", "Library/locked_object_ratings.txt", "Library/ladder_ratings.txt"};
static const char *previewFile[5] = {"artifact_enemies_preview.png", "door_preview.png", "key_preview.png", "locked_object_preview.png", "ladder_preview.png"};
static const char *names[5][MAX_DESIGNS] = {
    {"Original Arcane Orb", "Eclipse Reaver", "Bone Crown", "Rift Stalker", "Prism Sentinel", "Ash Seraph"},
    {"Original Gate Motif", "Obsidian Iris", "Blood Cathedral", "Astral Portal", "Bone Maw", ""},
    {"Tutorial Book Key", "Starforge", "Rift Thorn", "Bone Reliquary", "Sunbreaker",
     "Moonveil", "Verdant Coil", "Frostglass", "Clockheart", "Abyssal Trident",
     "Dragonfang", "Storm Compass", "Mourning Bell", "Warden's Oath", "Magma Crown"},
    {"Original Chest Motif", "Chain Vault", "Rune Reliquary", "Iron Idol", "Cursed Coffer", ""},
    {"Original Ladder", "Crystal Descent", "Chain Ascent", "Bone Stair", "Sunspire", "Void Rungs"}
};
static const char *notes[5][MAX_DESIGNS] = {
    {"Current violet enemy", "Eclipse + fractured ring", "Horned bone mask", "Living tear in space", "Runic armored construct", "Burning halo and wings"},
    {"Current menu gate style", "Rotating black aperture", "Blood red gothic door", "Floating star seal", "Fanged living entrance", ""},
    {"Diagonal rear-cover key", "Forged star crown", "Shadow crystal blade", "Ivory ceremonial key", "Solar double-bit key",
     "Silver crescent and moonstone", "Jade serpent wrapped around the bow", "Faceted ice crystal and frost teeth",
     "Brass gear crown with a ruby core", "Deep sea trident bow and coral teeth",
     "Dark dragon crest with ember eyes", "Compass star with storm-blue steel", "A tiny bell with a silver clapper",
     "Shield crest with a blue oathstone", "Obsidian shaft and molten crown"},
    {"Existing tutorial chest", "Heavy chain lock", "Crystal archive chest", "Face-shaped iron safe", "Thorned cursed box", ""},
    {"Current artifact ladder", "Cyan crystal shaft", "Suspended brass links", "Rib-cage ladder", "Golden light steps", "Space-time descent"}
};
static const int count[5] = {6,5,15,5,6};

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
    } else if(id==4) {
        LINE(8,26,22,8,5,P(103,55,36)); LINE(8,26,22,8,3,P(255,199,78));
        Ring(22,8,6,2,P(255,196,68));
        Tri(18,4,22,0,26,4,P(255,232,132));
        Tri(27,6,31,8,27,10,P(255,232,132));
        Diamond(22,8,3,P(255,95,52));
        BOX(5,22,6,3,P(255,224,101)); BOX(8,25,7,3,P(255,224,101));
    } else if(id==5) {
        // Crescent bow, moonstone eye and cool silver teeth.
        LINE(7,27,22,10,6,P(62,75,102)); LINE(7,27,22,10,3,P(193,220,241));
        Ring(22,8,7,2,P(210,229,242));
        Tri(17,7,19,1,23,6,P(231,244,255));
        Tri(17,9,18,15,23,10,P(136,180,221));
        Diamond(22,8,3,P(133,228,250)); DOT(21,7,1,P(255,255,255));
        BOX(6,23,6,2,P(218,235,248)); BOX(8,26,6,2,P(218,235,248));
    } else if(id==6) {
        // Jade serpent coils around its own eye; leaf-shaped bits form the teeth.
        LINE(7,27,21,10,6,P(30,66,48)); LINE(7,27,21,10,3,P(114,204,119));
        Ring(22,8,7,3,P(66,158,82));
        LINE(18,6,20,3,2,P(162,224,118)); LINE(20,3,24,4,2,P(162,224,118));
        LINE(24,4,26,8,2,P(162,224,118));
        DOT(24,6,1,P(255,222,103)); DOT(25,6,1,P(22,50,34));
        Diamond(21,9,2,P(187,236,121));
        Tri(6,22,12,23,8,28,P(128,207,109));
        Tri(9,25,15,26,11,31,P(79,168,89));
    } else if(id==7) {
        // Frostglass facets make a sharp crystal bow and icy double-bit.
        LINE(8,27,20,11,6,P(38,83,116)); LINE(8,27,20,11,2,P(137,225,244));
        Tri(17,10,21,1,23,10,P(224,252,255));
        Tri(17,10,23,10,20,15,P(70,184,220));
        Tri(21,2,28,7,23,10,P(148,222,244));
        Tri(23,10,28,7,27,14,P(63,147,197));
        Diamond(22,9,2,P(255,255,255));
        Tri(5,21,11,24,6,27,P(153,239,249));
        Tri(8,24,14,27,9,30,P(83,199,225));
    } else if(id==8) {
        // Clockwork crown with individual gear teeth, a dark dial and ruby hub.
        LINE(7,27,21,10,6,P(91,57,34)); LINE(7,27,21,10,3,P(219,163,75));
        Ring(22,8,6,2,P(243,192,94));
        for(int i=0;i<8;i++) {
            float a=i*PI/4.0f;
            Tri(22+cosf(a)*5,8+sinf(a)*5,
                22+cosf(a+0.26f)*8,8+sinf(a+0.26f)*8,
                22+cosf(a+0.52f)*5,8+sinf(a+0.52f)*5,P(227,164,73));
        }
        DOT(22,8,3,P(91,45,50)); Diamond(22,8,2,P(255,103,103));
        LINE(22,8,22,5,1,P(255,222,140)); LINE(22,8,24,9,1,P(255,222,140));
        BOX(6,23,6,2,P(247,201,106)); BOX(9,26,6,2,P(247,201,106));
    } else if(id==9) {
        // Three-pronged sea crown, pearl joint and jagged coral bits.
        LINE(8,27,21,11,6,P(21,76,82)); LINE(8,27,21,11,3,P(74,197,178));
        DOT(21,10,4,P(33,118,124)); DOT(21,10,2,P(124,238,212));
        LINE(18,9,15,3,3,P(82,208,194)); LINE(20,8,21,1,3,P(82,208,194));
        LINE(23,8,27,3,3,P(82,208,194));
        Tri(14,3,16,0,17,5,P(178,248,222));
        Tri(20,2,22,0,23,5,P(178,248,222));
        Tri(26,4,29,2,28,7,P(178,248,222));
        Tri(5,22,10,23,6,28,P(255,128,112));
        Tri(8,25,14,26,10,31,P(247,178,129));
    } else if(id==10) {
        // Dragon crest in dark iron, with swept horns and a hot ember eye.
        LINE(8,27,21,10,6,P(61,31,39)); LINE(8,27,21,10,3,P(166,69,68));
        Ring(22,8,6,2,P(154,64,65));
        Tri(18,8,15,1,22,5,P(204,90,68));
        Tri(23,6,29,1,27,9,P(125,44,53));
        Tri(18,10,16,15,22,13,P(205,102,67));
        DOT(23,7,2,P(255,164,72)); DOT(23,7,1,P(255,238,151));
        Tri(5,22,11,23,6,28,P(196,78,67));
        Tri(8,25,14,26,10,31,P(137,48,56));
    } else if(id==11) {
        // Four-point storm compass around a blue steel ring and golden north mark.
        LINE(8,27,21,10,6,P(42,58,93)); LINE(8,27,21,10,3,P(134,174,226));
        Ring(22,8,7,2,P(111,166,225));
        Tri(22,0,24,7,22,8,P(255,219,116));
        Tri(22,0,20,7,22,8,P(252,242,190));
        Tri(30,8,23,10,22,8,P(105,214,248));
        Tri(22,16,20,9,22,8,P(105,214,248));
        Tri(14,8,21,6,22,8,P(105,214,248));
        DOT(22,8,2,P(248,222,145));
        LINE(6,23,11,24,2,P(151,194,238));
        LINE(9,26,14,27,2,P(151,194,238));
    } else if(id==12) {
        // Small ceremonial bell replaces the usual ring-shaped bow.
        LINE(8,27,20,12,6,P(76,66,80)); LINE(8,27,20,12,3,P(193,190,205));
        Tri(16,9,18,5,22,4,P(194,182,211));
        Tri(22,4,27,7,27,12,P(224,215,235));
        Tri(16,9,27,12,16,13,P(164,155,181));
        BOX(16,12,12,2,P(220,210,228)); BOX(18,14,8,2,P(119,107,137));
        DOT(22,16,2,P(243,196,113)); DOT(22,17,1,P(255,235,173));
        BOX(6,22,6,2,P(223,215,229)); BOX(9,25,6,2,P(223,215,229));
    } else if(id==13) {
        // A compact shield crest set with an oathstone and polished silver shaft.
        LINE(8,27,20,12,6,P(54,67,92)); LINE(8,27,20,12,3,P(202,218,238));
        Tri(16,4,28,5,28,10,P(159,191,225));
        Tri(16,4,28,10,22,15,P(215,228,245));
        Tri(16,4,22,15,16,12,P(123,154,196));
        Diamond(22,8,3,P(91,208,231)); Diamond(22,8,1,P(232,255,255));
        LINE(18,6,21,4,1,P(255,255,255)); LINE(24,12,26,9,1,P(255,255,255));
        BOX(6,22,6,2,P(223,234,248)); BOX(9,25,6,2,P(223,234,248));
    } else {
        // Blackened shaft, lava seam and a jagged crown cut from volcanic glass.
        LINE(8,27,21,11,6,P(43,29,36)); LINE(8,27,21,11,3,P(111,63,53));
        Tri(16,10,17,2,21,7,P(67,42,49));
        Tri(17,8,20,0,22,8,P(165,70,48));
        Tri(20,8,24,1,24,9,P(216,91,48));
        Tri(23,8,28,3,27,10,P(151,56,47));
        LINE(21,10,24,7,2,P(255,149,53));
        LINE(18,14,20,12,2,P(244,103,45));
        DOT(23,9,2,P(255,194,75)); DOT(23,9,1,P(255,244,165));
        Tri(5,22,11,23,6,28,P(196,86,57));
        Tri(8,25,14,26,10,31,P(126,57,52));
    }
    Spark(27,25,2,P(255,235,178));
}

static void Locked(int id,bool open,float t) {
    (void)t;
    BOX(3,10,26,20,P(10,8,20));
    if(open) {
        BOX(4,19,24,10,P(55,43,70)); BOX(5,21,22,6,P(12,10,23));
        BOX(4,3,24,6,P(126,97,92)); BOX(5,5,22,3,P(59,41,60));
        Diamond(16,24,4,P(249,214,129)); Spark(16,16,3,P(124,239,237));
        return;
    }
    if(id==0) {
        BOX(4,11,24,18,P(126,96,82)); BOX(5,13,22,12,P(70,53,68));
        BOX(3,10,26,6,P(168,129,96)); BOX(6,11,20,2,P(228,181,113));
        BOX(8,17,3,10,P(193,150,100)); BOX(21,17,3,10,P(193,150,100));
        BOX(13,14,6,10,P(206,154,83)); Keyhole(16,19);
    } else if(id==1) {
        BOX(3,11,26,18,P(48,51,72)); BOX(5,13,22,14,P(19,22,38));
        for(int y=15;y<27;y+=5)LINE(5,y,27,y+3,2,P(155,164,186));
        LINE(6,10,26,29,3,P(170,169,168)); LINE(26,10,6,29,3,P(95,119,135));
        Ring(16,19,7,2,P(207,179,111)); Keyhole(16,19);
        DOT(6,10,2,P(250,198,104)); DOT(26,29,2,P(250,198,104));
    } else if(id==2) {
        BOX(4,10,24,19,P(26,39,53));
        for(int i=0;i<3;i++){Diamond(8+i*8,11,4,P(78,179,202));
            Diamond(8+i*8,25,3,P(78,179,202));}
        BOX(7,14,18,9,P(12,19,33));
        Diamond(16,18,7,P(152,87,227)); Diamond(16,18,4,P(31,26,56)); Keyhole(16,18);
        Spark(5,6,2,P(167,246,252));
    } else if(id==3) {
        BOX(3,9,26,20,P(75,67,76)); BOX(5,10,22,18,P(28,27,41));
        Tri(4,11,11,7,11,17,P(177,169,158)); Tri(28,11,21,7,21,17,P(177,169,158));
        BOX(9,15,5,2,P(251,116,73)); BOX(18,15,5,2,P(251,116,73));
        BOX(11,21,10,5,P(108,96,91)); Keyhole(16,22);
        BOX(4,26,24,3,P(161,130,94));
    } else {
        BOX(3,11,26,18,P(78,21,49)); BOX(5,13,22,14,P(26,12,32));
        Tri(4,12,9,1,11,13,P(129,51,70)); Tri(28,12,23,1,21,13,P(129,51,70));
        for(int i=0;i<4;i++){int x=5+i*6;
            Tri(x,27,x+2,21,x+4,27,P(236,172,158));}
        Ring(16,18,8,2,P(224,102,137));
        DOT(16,18,5,P(13,12,29)); Keyhole(16,18);
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

static void Sprite(int id,int bg,bool open,float t) {
    Backdrop(bg);
    if(GALLERY==0) Enemy(id,t);
    else if(GALLERY==1) Door(id,open,t);
    else if(GALLERY==2) Key(id,t);
    else if(GALLERY==3) Locked(id,open,t);
    else Ladder(id,t);
}

static void ArchiveRatedDesign(int id) {
    if(id<0||id>=count[GALLERY]||GALLERY!=2)return;
    char directory[512];
    snprintf(directory,sizeof(directory),"%sLibrary",GetApplicationDirectory());
    MakeDirectory(directory);
    char path[512];
    snprintf(path,sizeof(path),"%sLibrary/key_%02d.png",GetApplicationDirectory(),id);
    if(FileExists(path))return;
    RenderTexture2D target=LoadRenderTexture(32,32);
    if(target.id==0)return;
    SetTextureFilter(target.texture,TEXTURE_FILTER_POINT);
    BeginTextureMode(target);
    ClearBackground((Color){0,0,0,0});
    Key(id,0.0f);
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
    bool capture=false;
    bool archiveRated=false;
    int requestedPage=1;
    for(int arg=1;arg<argc;arg++){
        if(strcmp(argv[arg],"--capture")==0)capture=true;
        else if(strcmp(argv[arg],"--archive-rated")==0)archiveRated=true;
        else if(strncmp(argv[arg],"--capture-page=",15)==0){
            int value;
            if(sscanf(argv[arg]+15,"%d",&value)==1&&value>0)requestedPage=value;
            capture=true;
        }
    }
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1240,780,galleryName[GALLERY]);
    SetTargetFPS(60);
    ChangeDirectory(GetApplicationDirectory());
    char designDirectory[512];
    snprintf(designDirectory,sizeof(designDirectory),"%sLibrary",GetApplicationDirectory());
    MakeDirectory(designDirectory);
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
    const int pageCount=(count[GALLERY]+PAGE_SIZE-1)/PAGE_SIZE;
    int page=requestedPage-1;
    if(page>=pageCount)page=pageCount-1;
    bool open=false;
    while(!WindowShouldClose()) {
        if((IsKeyPressed(KEY_K) || IsKeyPressed(KEY_SPACE)) && (GALLERY==1 || GALLERY==3))open=!open;
        if(pageCount>1){
            if(IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_PAGE_DOWN))page=(page+1)%pageCount;
            if(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_PAGE_UP))page=(page+pageCount-1)%pageCount;
            float wheel=GetMouseWheelMove();
            if(wheel>0)page=(page+pageCount-1)%pageCount;
            else if(wheel<0)page=(page+1)%pageCount;
        }
        if(IsKeyPressed(KEY_ZERO)) {
            for(int i=0;i<count[GALLERY];i++)ratings[i]=0;
            SaveRatings(ratings);
        }
        int pageStart=page*PAGE_SIZE;
        for(int slot=0;slot<PAGE_SIZE&&pageStart+slot<count[GALLERY];slot++)
        if(IsKeyPressed(KEY_ONE+slot) || IsKeyPressed(KEY_KP_1+slot)){
            int value=(IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL))?1:
                ((IsKeyDown(KEY_LEFT_SHIFT)||IsKeyDown(KEY_RIGHT_SHIFT))?2:3);
            int id=pageStart+slot;
            ratings[id]=ratings[id]==value?0:value;
            SaveRatings(ratings);
        }
        float t=(float)GetTime();
        for(int i=0;i<count[GALLERY];i++)for(int bg=0;bg<3;bg++){
            BeginTextureMode(previews[i][bg]);
            Sprite(i,bg,open,t);
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
        DrawText("1-6 RATE  |  SHIFT: MAYBE / FIX  |  CTRL: REJECT  |  LEFT / RIGHT: PAGE",43,61,16,P(157,164,186));
        DrawText(TextFormat("PAGE %d / %d",page+1,pageCount),1090,30,16,P(119,224,215));
        if(GALLERY==1 || GALLERY==3) DrawText("K / SPACE: use key  |  same key opens both",730,61,16,P(119,224,215));
        else if(GALLERY==2) DrawText("One key opens doors and locked objects",830,61,16,P(119,224,215));
        Vector2 local={(mouse.x-ox)/scale,(mouse.y-oy)/scale};
        int visibleCount=count[GALLERY]-pageStart;
        if(visibleCount>PAGE_SIZE)visibleCount=PAGE_SIZE;
        for(int slot=0;slot<visibleCount;slot++){
            int i=pageStart+slot;
            int col=slot%3,row=slot/3,x=42+col*390,y=94+row*324;
            Rectangle hit={(float)x,(float)y,374,310};
            bool hot=CheckCollisionPointRec(local,hit);
            DrawRectangleRounded(hit,0.07f,6,hot?P(40,34,58):P(28,25,42));
            Color edge=ratings[i]==1?P(247,104,122):(ratings[i]==2?P(246,189,100):
                (ratings[i]==3?P(105,230,194):(hot?P(129,207,217):P(68,59,84))));
            DrawRectangleRoundedLinesEx(hit,0.07f,6,ratings[i]?3.0f:1.0f,edge);
            DrawText(TextFormat("%d  %s",i+1,names[GALLERY][i]),x+18,y+14,20,P(242,230,205));
            DrawText(notes[GALLERY][i],x+19,y+42,15,P(174,163,191));
            int mainBg=(GALLERY==1)?1:((GALLERY==4)?2:0);
            Rectangle src={0,0,32,-32},dst={(float)x+20,(float)y+73,160,160};
            DrawTexturePro(previews[i][mainBg].texture,src,dst,(Vector2){0,0},0,WHITE);
            DrawRectangleLinesEx(dst,2,P(108,98,127));
            const char *bgLabel[3]={"FLOOR","WALL","PAD"};
            for(int bg=0;bg<3;bg++){
                int px=x+213,py=y+73+bg*52;
                DrawTexturePro(previews[i][bg].texture,src,(Rectangle){(float)px,(float)py,32,32},(Vector2){0,0},0,WHITE);
                DrawText(bgLabel[bg],px+43,py+8,15,P(169,181,202));
            }
            DrawText("ACTUAL SIZE",x+212,y+235,13,P(128,223,215));
            const char *labels[3]={"REJECT","MAYBE / FIX","ACCEPT"};
            const int offsets[3]={16,123,242},widths[3]={100,112,116};
            for(int value=1;value<=3;value++){
                Rectangle button={(float)x+offsets[value-1],(float)y+261,
                                  (float)widths[value-1],34};
                if(RatingButton(button,labels[value-1],ratings[i],value,local)){
                    clicked=i;clickedValue=value;
                }
            }
        }
        int totals[4]={0};
        for(int i=0;i<count[GALLERY];i++)totals[ratings[i]]++;
        DrawText(TextFormat("REJECTED %d    MAYBE / FIX %d    ACCEPTED %d    |    0: clear ratings    LEFT / RIGHT: page    ESC: close",
                            totals[1],totals[2],totals[3]),42,748,15,P(164,166,184));
        EndMode2D();
        if(clicked>=0){
            ratings[clicked]=ratings[clicked]==clickedValue?0:clickedValue;
            SaveRatings(ratings);
        }
        if(capture) {
            char shot[512];
            const char *base=previewFile[GALLERY];
            if(page==0)snprintf(shot,sizeof(shot),"%s/Library/%s",GetApplicationDirectory(),base);
            else {
                const char *ext=strrchr(base,'.');
                int stem=ext?(int)(ext-base):(int)strlen(base);
                snprintf(shot,sizeof(shot),"%s/Library/%.*s_page_%d%s",GetApplicationDirectory(),
                         stem,base,page+1,ext?ext:"");
            }
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
