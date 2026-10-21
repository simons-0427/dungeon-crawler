// Standalone raylib sprite gallery. Build from Snapshot in PowerShell with:
// $env:PATH='C:\raylib\w64devkit\bin;'+$env:PATH
// gcc -std=c99 -O2 -Wall -Wextra -o artifact_enemies.exe artifact_enemies.c -IC:/raylib/raylib/src -LC:/raylib/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm
// Each of the five gallery files is self-contained. No game files are changed.
#define GALLERY 0 // 0 enemy, 1 door, 2 key, 3 locked object, 4 ladder

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
#define MAX_DESIGNS 12
#define PAGE_SIZE 6

static const char *galleryName[5] = {"ARTIFACT ROOM ENEMIES", "DOORS", "UNIVERSAL KEYS", "LOCKED OBJECTS", "ARTIFACT LADDERS"};
static const char *legacyWinnerFile[5] = {"Library/artifact_enemy_winner.txt", "Library/door_winner.txt", "Library/key_winner.txt", "Library/locked_object_winner.txt", "Library/ladder_winner.txt"};
static const char *selectionFile[5] = {"Library/artifact_enemy_selections.txt", "Library/door_selections.txt", "Library/key_selections.txt", "Library/locked_object_selections.txt", "Library/ladder_selections.txt"};
static const char *ratingFile[5] = {"Library/artifact_enemy_ratings.txt", "Library/door_ratings.txt", "Library/key_ratings.txt", "Library/locked_object_ratings.txt", "Library/ladder_ratings.txt"};
static const char *previewFile[5] = {"artifact_enemies_preview.png", "door_preview.png", "key_preview.png", "locked_object_preview.png", "ladder_preview.png"};
static const char *names[5][MAX_DESIGNS] = {
    {"Original Arcane Orb", "Azure Wisp", "Bone Crown", "Ivory Revenant", "Moon Seraph", "Ash Seraph",
     "Obsidian Spawn", "Grave Seraph", "Halfmoon Husk", "Chainbound", "Riftscar", "Cinder Reliquary"},
    {"Original Gate Motif", "Obsidian Iris", "Blood Cathedral", "Astral Portal", "Bone Maw", ""},
    {"Tutorial Book Key", "Starforge", "Rift Thorn", "Bone Reliquary", "Sunbreaker", ""},
    {"Original Chest Motif", "Chain Vault", "Rune Reliquary", "Iron Idol", "Cursed Coffer", ""},
    {"Original Ladder", "Crystal Descent", "Chain Ascent", "Bone Stair", "Sunspire", "Void Rungs"}
};
static const char *notes[5][MAX_DESIGNS] = {
    {"Current violet enemy", "Blue spirit orb with amber eyes", "Horned bone mask", "Broken bone mask over violet shadow",
     "Cold halo and folded wings", "Burning halo and wings",
     "Black brute split by molten cracks", "Bone-winged specter with a fanged mask",
     "Ivory crescent over a shadowed face", "Dark orb held by broken iron",
     "A split shadow with mismatched eyes", "Charred sphere hiding a bone mask"},
    {"Current menu gate style", "Rotating black aperture", "Blood red gothic door", "Floating star seal", "Fanged living entrance", ""},
    {"Diagonal rear-cover key", "Forged star crown", "Shadow crystal blade", "Ivory ceremonial key", "Solar double-bit key", ""},
    {"Existing tutorial chest", "Heavy chain lock", "Crystal archive chest", "Face-shaped iron safe", "Thorned cursed box", ""},
    {"Current artifact ladder", "Cyan crystal shaft", "Suspended brass links", "Rib-cage ladder", "Golden light steps", "Space-time descent"}
};
static const int count[5] = {12,5,5,5,6};

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

        // Azure Wisp: a compact spirit orb with fire-like wisps and warm eyes.
        DOT(16,16,14,A(51,213,240,(unsigned char)(31+27*pulse)));
        Tri(11,9,8,2,15,8,P(36,107,149));
        Tri(20,9,25,3,23,12,P(54,150,173));
        Tri(9,19,2,16,8,23,P(39,119,148));
        DOT(16,17,11,P(30,67,103));
        DOT(12,13,6,A(108,205,216,88));
        DOT(12,15,3,P(255,230,139)); DOT(20,15,3,P(255,230,139));
        DOT(12,15,1,P(18,47,72)); DOT(20,15,1,P(18,47,72));
        Tri(13,24,17,28,18,22,P(12,43,76));
        LINE(0,12,5,13,1,P(101,223,232));
        LINE(0,19,5,18,1,P(101,223,232));
        LINE(26,9,30,7,1,P(101,223,232));
        Spark(27,22,1,P(141,242,232));
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

        // Ivory Revenant: a broken bone mask over a living violet shadow.
        DOT(16,17,13,A(170,104,230,(unsigned char)(27+25*pulse)));
        DOT(16,18,11,P(47,31,65));
        Tri(6,13,4,3,14,10,P(230,212,178));
        Tri(26,13,28,4,19,10,P(202,185,169));
        Tri(11,9,16,4,20,9,P(245,227,191));
        Tri(8,11,16,8,15,25,P(220,202,178));
        Tri(24,11,17,8,17,25,P(171,160,164));
        LINE(15,10,13,14,1,P(111,89,112));
        LINE(19,10,18,14,1,P(96,77,107));
        BOX(9,15,5,3,P(14,9,27)); BOX(18,15,5,3,P(14,9,27));
        DOT(11,16,2,P(122,235,255)); DOT(21,16,2,P(122,235,255));
        Tri(14,21,18,21,16,27,P(19,10,34));
        LINE(10,25,7,27,1,P(231,207,183));
        LINE(22,25,25,27,1,P(192,176,176));
    } else if(id==4) {

        // Moon Seraph: a dark face framed by a broken cold halo and folded wings.
        DOT(16,15,14,A(107,187,255,(unsigned char)(26+23*pulse)));
        Tri(3,24,11,8,12,20,P(102,169,211));
        Tri(29,24,21,8,20,20,P(151,205,235));
        Tri(4,24,10,18,9,29,P(58,95,147));
        Tri(28,24,22,18,23,29,P(74,112,161));
        DrawRing((Vector2){16,13},9,11,204,334,18,P(204,230,239));
        DrawRing((Vector2){16,13},9,11,26,156,18,P(204,230,239));
        DOT(16,17,8,P(27,27,52));
        Tri(11,11,16,6,21,11,P(205,232,240));
        BOX(9,16,5,2,P(242,249,229)); BOX(18,16,5,2,P(242,249,229));
        Diamond(16,23,2,P(111,215,244));
        DOT(5,7,1,P(204,236,245)); DOT(27,7,1,P(204,236,245));
    } else if(id==5) {

        DOT(16,15,14,A(255,92,54,55));
        Tri(2,24,12,6,13,19,P(218,120,74)); Tri(30,24,20,6,19,19,P(255,171,85));
        Tri(4,25,12,15,10,29,P(119,57,75)); Tri(28,25,20,15,22,29,P(119,57,75));
        Ring(16,13,11,2,P(252,196,99));
        DOT(16,17,8,P(37,24,42));
        Tri(11,12,16,5,21,12,P(254,199,113));
        BOX(9,16,5,2,P(255,237,177)); BOX(18,16,5,2,P(255,237,177));
        Diamond(16,22,3,P(255,103,83));
    } else if(id==6) {

        // Obsidian Spawn: heavy black shoulders, molten cracks and a biting maw.
        DOT(16,17,15,A(238,91,22,(unsigned char)(24+22*pulse)));
        DOT(6,20,6,P(23,22,32)); DOT(26,20,6,P(23,22,32));
        Tri(7,12,5,2,13,10,P(35,31,43)); Tri(25,12,27,2,19,10,P(40,34,45));
        DOT(16,18,11,P(17,17,26));
        Tri(12,8,16,2,20,8,P(51,43,49));
        LINE(8,11,13,14,2,P(249,111,30)); LINE(19,12,25,9,2,P(255,141,42));
        LINE(15,9,17,13,2,P(244,75,25)); LINE(10,25,13,22,2,P(232,83,25));
        LINE(22,25,19,22,2,P(232,83,25));
        Tri(9,15,14,16,11,19,P(255,166,48)); Tri(23,15,18,16,21,19,P(255,166,48));
        DOT(11,17,1,P(61,18,14)); DOT(21,17,1,P(61,18,14));
        Tri(9,21,23,21,16,28,P(7,7,14));
        Tri(11,21,14,21,12,25,P(255,209,125)); Tri(18,21,21,21,20,25,P(255,209,125));
        Tri(15,27,16,24,18,27,P(255,181,90));
    } else if(id==7) {

        // Grave Seraph: skeletal wings, bone mask and hungry golden eyes.
        DOT(16,17,15,A(153,128,199,(unsigned char)(26+20*pulse)));
        Tri(11,17,1,4,5,22,P(115,94,142)); Tri(9,22,2,28,7,19,P(211,194,180));
        Tri(21,17,31,4,27,22,P(143,113,160)); Tri(23,22,30,28,25,19,P(233,211,187));
        LINE(3,12,10,17,2,P(230,211,183)); LINE(29,12,22,17,2,P(230,211,183));
        Tri(7,12,5,2,13,10,P(233,220,192)); Tri(25,12,27,2,19,10,P(205,197,185));
        DOT(16,18,10,P(35,29,55));
        Tri(8,13,16,9,16,25,P(219,209,190)); Tri(24,13,16,9,16,25,P(178,174,184));
        LINE(8,14,14,16,2,P(49,36,62)); LINE(24,14,18,16,2,P(49,36,62));
        Tri(9,16,14,17,11,19,P(255,210,112)); Tri(23,16,18,17,21,19,P(255,210,112));
        DOT(11,18,1,P(53,33,61)); DOT(21,18,1,P(53,33,61));
        Tri(11,21,21,21,16,28,P(17,10,32));
        Tri(11,21,14,21,13,26,P(246,231,211)); Tri(18,21,21,21,19,26,P(246,231,211));
        Tri(15,28,16,24,18,28,P(246,231,211));
    } else if(id==8) {

        // Halfmoon Husk: a half-buried ivory face inside a rounded shadow.
        DOT(16,17,15,A(164,124,206,(unsigned char)(25+21*pulse)));
        DOT(16,17,12,P(49,37,73));
        DOT(13,13,7,A(138,98,177,70));
        DrawRing((Vector2){16,17},10,12,117,260,20,P(223,208,189));
        Tri(7,11,13,7,13,26,P(227,211,191));
        Tri(13,9,16,12,13,19,P(177,161,168));
        BOX(9,15,5,3,P(17,12,33));
        Tri(18,15,23,15,20,19,P(244,112,189));
        DOT(11,16,2,P(239,107,181));
        DOT(20,17,1,P(42,18,56));
        LINE(13,22,17,24,1,P(212,190,188));
        Diamond(18,25,2,P(110,76,143));
        DOT(5,8,1,P(200,150,231));
    } else if(id==9) {

        // Chainbound: a round jailer with a broken diagonal iron restraint.
        DOT(16,17,15,A(178,74,112,(unsigned char)(24+21*pulse)));
        DOT(16,17,12,P(37,34,52));
        DOT(12,12,7,A(109,88,121,68));
        LINE(5,8,27,27,3,P(92,96,109));
        LINE(5,7,27,26,1,P(190,185,174));
        BOX(6,7,3,3,P(45,48,62)); BOX(23,23,3,3,P(45,48,62));
        BOX(10,11,3,3,P(150,148,150)); BOX(19,19,3,3,P(150,148,150));
        Tri(7,15,14,16,10,20,P(245,103,117));
        Tri(25,15,18,16,22,20,P(245,103,117));
        DOT(11,17,1,P(30,10,31)); DOT(21,17,1,P(30,10,31));
        LINE(9,22,13,25,2,P(154,153,160));
        LINE(22,10,26,7,2,P(154,153,160));
        Diamond(16,25,2,P(186,66,94));
        DOT(3,24,1,P(222,119,142)); DOT(29,8,1,P(222,119,142));
    } else if(id==10) {

        // Riftscar: one orb cut by a luminous tear, with eyes on either side.
        DOT(16,17,15,A(83,230,213,(unsigned char)(24+23*pulse)));
        DOT(16,17,12,P(50,37,91));
        DOT(11,12,6,A(121,95,188,79));
        Tri(4,11,1,7,7,10,P(65,181,198));
        Tri(28,24,31,28,25,26,P(96,178,219));
        LINE(13,6,18,11,3,P(41,174,202));
        LINE(18,11,13,18,3,P(41,174,202));
        LINE(13,18,19,25,3,P(41,174,202));
        LINE(14,7,18,11,1,P(186,250,234));
        LINE(18,11,14,18,1,P(186,250,234));
        LINE(14,18,19,24,1,P(186,250,234));
        LINE(6,12,12,14,2,P(28,24,65));
        LINE(26,12,20,14,2,P(28,24,65));
        Tri(6,15,13,16,9,20,P(255,204,113));
        Tri(26,15,19,16,23,20,P(245,141,218));
        DOT(9,17,1,P(38,24,70)); DOT(23,17,1,P(38,24,70));
        Tri(13,23,18,25,15,28,P(19,14,49));
        LINE(8,23,11,26,1,P(109,86,169));
        DOT(26,6,1,P(167,246,238));
    } else {

        // Cinder Reliquary: a scorched round shell concealing a bone face.
        DOT(16,17,15,A(232,94,54,(unsigned char)(20+23*pulse)));
        DOT(16,17,12,P(91,37,42));
        DOT(12,12,7,A(188,78,69,68));
        LINE(7,9,12,12,2,P(38,18,30));
        LINE(25,9,20,12,2,P(38,18,30));
        Tri(8,12,16,8,16,23,P(62,27,38));
        Tri(24,12,16,8,16,23,P(44,24,37));
        Tri(9,19,16,22,11,27,P(215,196,166));
        Tri(23,19,16,22,21,27,P(177,166,157));
        Tri(11,25,16,29,21,25,P(201,184,164));
        Tri(8,14,14,16,10,19,P(192,249,198));
        Tri(24,14,18,16,22,19,P(192,249,198));
        DOT(10,17,1,P(29,52,43)); DOT(22,17,1,P(29,52,43));
        LINE(4,23,7,22,1,P(244,129,79));
        LINE(25,8,28,6,1,P(244,129,79));
        DOT(27,25,1,P(255,166,99));
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
    bool capture=argc>1 && (strcmp(argv[1],"--capture")==0 ||
                            strncmp(argv[1],"--capture-page=",15)==0);
    int pageCount=(count[GALLERY]+PAGE_SIZE-1)/PAGE_SIZE;
    int page=0;
    if(argc>1 && strncmp(argv[1],"--capture-page=",15)==0) {
        int requestedPage=0;
        if(sscanf(argv[1],"--capture-page=%d",&requestedPage)==1 &&
           requestedPage>=1 && requestedPage<=pageCount)page=requestedPage-1;
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
    bool open=false;
    while(!WindowShouldClose()) {
        float wheel=GetMouseWheelMove();
        if(IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_PAGE_DOWN)||wheel<0.0f)
            page=(page+1)%pageCount;
        if(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_PAGE_UP)||wheel>0.0f)
            page=(page+pageCount-1)%pageCount;
        if((IsKeyPressed(KEY_K) || IsKeyPressed(KEY_SPACE)) && (GALLERY==1 || GALLERY==3))open=!open;
        if(IsKeyPressed(KEY_ZERO)) {
            for(int i=0;i<count[GALLERY];i++)ratings[i]=0;
            SaveRatings(ratings);
        }
        for(int slot=0;slot<PAGE_SIZE;slot++) {
            int i=page*PAGE_SIZE+slot;
            if(i>=count[GALLERY])break;
            if(!IsKeyPressed(KEY_ONE+slot) && !IsKeyPressed(KEY_KP_1+slot))continue;
            int value=(IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL))?1:
                ((IsKeyDown(KEY_LEFT_SHIFT)||IsKeyDown(KEY_RIGHT_SHIFT))?2:3);
            ratings[i]=ratings[i]==value?0:value;
            SaveRatings(ratings);
        }
        float t=capture?0.0f:(float)GetTime();
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
        Vector2 local={(mouse.x-ox)/scale,(mouse.y-oy)/scale};
        int clicked=-1,clickedValue=0;
        BeginDrawing();
        ClearBackground(P(9,9,17));
        DrawRectangleGradientV(0,0,sw,sh,P(12,13,25),P(21,13,29));
        BeginMode2D((Camera2D){.offset={(float)ox,(float)oy},.target={0,0},.rotation=0,.zoom=scale});
        DrawText(galleryName[GALLERY],42,25,30,P(242,221,181));
        if(pageCount>1) {
            DrawText("1-6: RATE  |  SHIFT: MAYBE/FIX  |  CTRL: REJECT  |  LEFT/RIGHT OR WHEEL: PAGE",
                     43,61,16,P(157,164,186));
            DrawText(TextFormat("PAGE %d / %d",page+1,pageCount),1077,28,21,P(132,231,217));
        } else {
            DrawText("1-6: ACCEPT  |  SHIFT+1-6: MAYBE / FIX  |  CTRL+1-6: REJECT",43,61,16,P(157,164,186));
        }
        if(GALLERY==1 || GALLERY==3) DrawText("K / SPACE: use key  |  same key opens both",730,61,16,P(119,224,215));
        else if(GALLERY==2) DrawText("One key opens doors and locked objects",830,61,16,P(119,224,215));
        for(int slot=0;slot<PAGE_SIZE;slot++){
            int i=page*PAGE_SIZE+slot;
            if(i>=count[GALLERY])break;
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
        DrawText(TextFormat("REJECTED %d    MAYBE / FIX %d    ACCEPTED %d    |    0: clear ratings    ESC: close",
                            totals[1],totals[2],totals[3]),42,748,15,P(164,166,184));
        EndMode2D();
        if(clicked>=0){
            ratings[clicked]=ratings[clicked]==clickedValue?0:clickedValue;
            SaveRatings(ratings);
        }
        if(capture) {
            char shot[512],filename[128];
            if(page==0)snprintf(filename,sizeof(filename),"%s",previewFile[GALLERY]);
            else {
                const char *extension=strrchr(previewFile[GALLERY],'.');
                int stemLength=extension?(int)(extension-previewFile[GALLERY]):
                                          (int)strlen(previewFile[GALLERY]);
                snprintf(filename,sizeof(filename),"%.*s_page_%d.png",stemLength,
                         previewFile[GALLERY],page+1);
            }
            snprintf(shot,sizeof(shot),"%s/Library/%s",GetApplicationDirectory(),filename);
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
