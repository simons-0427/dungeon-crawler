// 18 selectable player skin concepts. Rejected designs have been removed.
// Build from Snapshot in PowerShell:
// $env:PATH='C:\raylib\w64devkit\bin;'+$env:PATH
// gcc -std=c99 -O2 -Wall -Wextra -o player_skin_ideas.exe player_skin_ideas.c -IC:/raylib/raylib/src -LC:/raylib/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm
// Run player_skin_ideas.exe. Left/right changes page; rate each design by click.

#include "raylib.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define C(r,g,b) ((Color){r,g,b,255})
#define CA(r,g,b,a) ((Color){r,g,b,a})
#define B(x,y,w,h,c) DrawRectangle(x,y,w,h,c)
#define O(x,y,r,c) DrawCircle(x,y,r,c)
#define L(x,y,u,v,w,c) DrawLineEx((Vector2){x,y},(Vector2){u,v},w,c)
#define N_SKINS 18
#define PAGE_SIZE 6
#define PAGE_COUNT ((N_SKINS + PAGE_SIZE - 1) / PAGE_SIZE)

static const char *skinNames[N_SKINS]={
    "Ember Fox", "Veil Dancer", "Grave Lantern",
    "Glass Echo", "Ossuary Knight", "Grave Hare",
    "Aurora Aegis", "Neon Mantis", "Glass Seraph", "Gilt Jackal",
    "Cinder Sovereign", "Gilded Oracle", "Wraith Duelist", "Seraphic Vanguard",
    "Xeno Prime", "Ember Marrow", "Celestial Ascendant", "Crimson Blade Warden"
};
static const char *skinNotes[N_SKINS]={
    "Fire-tailed trickster", "Porcelain mask and veil", "Haunted brass lantern",
    "Shattered mirror spirit", "Bone-plated guardian", "Long-eared crypt spirit",
    "Shield of aurora glass", "Bladed mantis traveler", "Faceted crystal wings", "Regal gold jackal",
    "Flame crown, layered mask and sparks", "Gilded face, coin crown and orbiting motes",
    "Torn ghost hood, chain links and cold eyes", "Feathered armor, jeweled halo and wings",
    "Segmented alien helm, antenna bulbs and biolights", "Carved bone crown, ember cracks and teeth",
    "Prismatic halo, radiant armor and star sigil", "Crimson mask, etched plates and crossed blades"
};
static const char *families[N_SKINS]={
    "PYRO", "PHANTOM", "PHANTOM", "PHANTOM", "SKULL", "SKULL", "ASCENDANT", "ALIEN", "ANGEL", "MIDAS",
    "PYRO", "MIDAS", "PHANTOM", "ANGEL", "ALIEN", "SKULL", "ASCENDANT", "BLADE LEGEND"
};
static const Color familyColor[N_SKINS]={
    {255,134,72,255},{178,178,255,255},{178,178,255,255},{178,178,255,255},
    {226,215,185,255},{226,215,185,255},{236,227,154,255},{120,237,182,255},
    {248,228,177,255},{255,204,97,255},{255,134,72,255},{255,204,97,255},
    {178,178,255,255},{248,228,177,255},{120,237,182,255},{226,215,185,255},
    {236,227,154,255},{255,91,91,255}
};

static void T(float x1,float y1,float x2,float y2,float x3,float y3,Color color){
    float cross=(x2-x1)*(y3-y1)-(y2-y1)*(x3-x1);
    if(cross>0)DrawTriangle((Vector2){x1,y1},(Vector2){x3,y3},(Vector2){x2,y2},color);
    else DrawTriangle((Vector2){x1,y1},(Vector2){x2,y2},(Vector2){x3,y3},color);
}
static void D(float x,float y,float r,Color color){
    T(x,y-r,x+r,y,x,y+r,color);T(x,y-r,x,y+r,x-r,y,color);
}
static void R(int x,int y,float radius,float width,Color color){
    DrawRing((Vector2){(float)x,(float)y},radius-width,radius,0,360,28,color);
}
static void Star(int x,int y,int r,Color color){
    L(x-r,y,x+r,y,1,color);L(x,y-r,x,y+r,1,color);O(x,y,1,color);
}
static void Base(Color rim,Color face){
    O(16,17,13,rim);O(16,16,10,face);
}
static void Backdrop(int bg){
    B(0,0,32,32,C(12,10,18));
    if(bg==0){B(1,1,30,30,C(28,24,40));B(1,1,30,1,CA(91,78,118,110));}
    else if(bg==1){
        B(1,1,30,30,C(50,45,66));B(1,1,30,3,C(86,78,107));B(1,1,3,30,C(80,73,101));
        B(1,28,30,3,C(25,20,38));B(28,1,3,30,C(28,23,43));
    }else{
        B(1,1,30,30,C(43,27,59));B(2,2,28,28,C(31,24,44));
        DrawRectangleLines(1,1,30,30,C(139,80,188));
        O(5,5,1,C(214,146,255));O(26,5,1,C(214,146,255));
        O(5,26,1,C(214,146,255));O(26,26,1,C(214,146,255));
    }
}

static void DrawSkin(int id,float t){
    float p=0.5f+0.5f*sinf(t*2.5f);
    switch(id){
    case 0: // Ember Fox: ears, long split tail, fox muzzle.
        T(8,10,7,1,15,8,C(232,91,57));T(24,10,25,1,17,8,C(255,155,69));
        T(10,26,2,21,4,8,C(255,113,62));T(22,27,31,18,27,6,C(255,191,76));
        Base(C(170,72,55),C(241,114,58));
        T(10,19,22,19,16,27,C(255,226,158));
        O(11,14,2,C(35,20,31));O(21,14,2,C(35,20,31));
        D(16,20,2,C(40,24,30));O(29,6,1,C(255,235,177));
        break;
    case 1: // Veil Dancer: porcelain mask floating in layered smoke.
        T(9,8,2,29,17,24,C(101,78,154));T(23,8,30,29,15,24,C(151,119,207));
        O(16,14,12,CA(127,99,197,58));
        T(9,9,23,9,21,24,C(235,220,229));T(9,9,21,24,11,24,C(176,160,203));
        L(8,7,23,7,2,C(226,193,247));
        O(12,15,2,C(34,27,60));O(20,15,2,C(34,27,60));
        L(16,19,19,22,1,C(101,77,145));
        break;
    case 2: // Grave Lantern: brass cage containing a ghost flame.
        L(16,0,16,5,2,C(202,175,110));R(16,5,3,1,C(211,187,126));
        T(7,8,16,3,25,8,C(175,144,94));B(7,8,18,18,C(105,79,85));
        B(9,10,14,14,C(12,20,35));
        T(12,22,16,9,21,22,C(117,222,237));T(14,20,17,12,19,20,C(218,250,251));
        B(6,9,3,16,C(222,191,129));B(23,9,3,16,C(173,138,99));
        B(7,25,18,3,C(215,175,113));O(14,17,1,C(23,38,54));O(19,17,1,C(23,38,54));
        break;
    case 3: // Glass Echo: disjoint reflections and an offset mirrored face.
        T(3,9,16,2,11,25,C(93,207,219));T(29,8,21,3,20,29,C(180,230,255));
        T(11,5,24,10,16,28,C(34,55,84));
        D(16,15,10,C(123,217,239));D(17,15,7,C(23,44,72));
        T(9,14,17,10,16,24,C(182,251,248));
        B(11,14,4,2,C(22,31,60));B(19,14,4,2,C(22,31,60));
        Star(28,25,2,C(218,252,255));
        break;
    case 4: // Ossuary Knight: jaw shaped faceplate and plated pauldron.
        T(1,16,10,9,10,25,C(121,117,125));T(31,16,22,9,22,25,C(189,179,162));
        Base(C(112,108,112),C(40,39,52));
        T(7,9,16,4,25,9,C(238,224,194));B(8,9,16,13,C(229,213,183));
        B(10,14,5,3,C(19,20,36));B(18,14,5,3,C(19,20,36));
        B(12,21,8,5,C(204,188,165));
        for(int i=0;i<3;i++)B(13+i*2,22,1,3,C(78,64,74));
        break;
    case 5: // Grave Hare: antler-like ears and small skull mask.
        T(8,13,6,0,13,7,C(221,211,193));T(24,13,26,0,19,7,C(210,192,187));
        T(7,8,3,4,14,15,C(151,132,153));T(25,8,29,4,18,15,C(151,132,153));
        Base(C(106,88,112),C(225,212,192));
        T(10,20,22,20,16,28,C(203,184,178));
        O(12,15,2,C(94,57,128));O(20,15,2,C(94,57,128));
        B(15,20,2,3,C(51,33,66));
        break;
    case 6: // Aurora Aegis: large kite shield with sweeping upper crests.
        T(4,16,9,3,15,14,C(106,226,214));T(28,16,23,3,17,14,C(230,146,244));
        D(16,16,13,C(238,226,174));D(16,16,10,C(50,67,93));
        T(16,6,25,16,16,27,C(127,238,223));T(16,6,16,27,7,16,C(128,159,245));
        D(16,16,5,C(255,248,212));D(16,16,2,C(232,153,255));
        break;
    case 7: // Neon Mantis: antennae, blade arms, triangular face.
        L(12,9,8,1,2,C(130,232,153));L(20,9,24,1,2,C(130,232,153));
        O(8,1,2,C(232,244,125));O(24,1,2,C(232,244,125));
        L(11,18,2,22,4,C(50,172,136));L(21,18,30,22,4,C(50,172,136));
        T(2,22,5,10,11,18,C(180,244,144));T(30,22,27,10,21,18,C(180,244,144));
        T(6,10,16,5,26,10,C(63,155,127));T(6,10,26,10,16,28,C(37,102,105));
        B(9,14,5,2,C(255,105,162));B(18,14,5,2,C(255,105,162));
        D(16,21,3,C(171,235,126));
        break;
    case 8: // Glass Seraph: angular crystal wings, distinct from feathered Angel.
        T(12,15,2,3,7,24,C(153,220,244));T(20,15,30,3,25,24,C(191,245,251));
        T(10,18,3,26,5,10,C(76,180,212));T(22,18,29,26,27,10,C(104,218,226));
        D(16,16,11,C(238,239,220));D(16,16,8,C(97,194,220));
        D(16,16,5,C(249,252,238));
        B(11,15,4,2,C(207,166,95));B(18,15,4,2,C(207,166,95));
        Star(16,2,3,C(255,223,147));
        break;
    case 9: // Gilt Jackal: long ears, angled golden animal mask.
        T(7,11,8,0,15,9,C(236,186,91));T(25,11,24,0,17,9,C(255,217,116));
        Base(C(119,75,54),C(225,168,75));
        T(6,9,16,5,16,27,C(251,207,117));T(26,9,16,5,16,27,C(194,125,59));
        T(11,19,21,19,16,29,C(56,39,48));
        B(9,14,5,2,C(28,29,39));B(18,14,5,2,C(28,29,39));
        D(16,21,2,C(255,238,169));
        break;
    case 10: { // Cinder Sovereign: layered Pyro crown, mask and live ember seams.
        O(16,17,15,CA(255,70,22,(unsigned char)(32+24*p)));
        T(3,18,8,6,13,15,C(119,30,39));T(29,18,24,6,19,15,C(189,49,34));
        T(5,13,10,2,14,11,C(255,100,38));T(12,11,16,0,20,11,C(255,205,79));
        T(18,11,23,2,27,13,C(255,125,38));
        T(7,12,14,8,16,13,C(77,29,37));T(25,12,18,8,16,13,C(125,34,32));
        Base(C(124,39,42),C(38,24,34));
        T(5,12,16,7,27,12,C(165,48,35));B(7,12,18,3,C(209,73,38));
        L(8,14,14,16,2,C(255,183,77));L(24,14,18,16,2,C(255,183,77));
        T(8,16,14,17,12,19,C(255,225,139));T(24,16,18,17,20,19,C(255,225,139));
        O(11,17,1,C(255,255,211));O(21,17,1,C(255,255,211));
        D(16,13,3,C(255,205,72));D(16,13,1,C(255,249,178));
        T(9,21,14,23,12,27,C(207,95,46));T(23,21,18,23,20,27,C(228,116,45));
        for(int i=0;i<4;i++)B(13+i*2,23,1,3,C(255,204,126));
        L(9,20,12,19,1,C(255,98,50));L(23,20,20,19,1,C(255,98,50));
        for(int i=0;i<4;i++){
            float a=t*1.8f+i*PI/2.0f;
            Star((int)(16+cosf(a)*14),(int)(15+sinf(a)*12),1,C(255,202,94));
        }
        break;
    }
    case 11: { // Gilded Oracle: Midas-inspired coin crown, filigree and jewel mask.
        O(16,17,15,CA(255,191,49,(unsigned char)(25+20*p)));
        O(4,16,4,C(180,119,43));O(4,16,2,C(255,220,115));
        O(28,16,4,C(164,104,42));O(28,16,2,C(255,226,128));
        T(5,12,8,2,12,11,C(255,190,57));T(11,11,16,1,20,11,C(255,224,117));
        T(20,11,24,2,27,12,C(235,167,48));B(8,10,16,3,C(187,120,42));
        for(int i=0;i<5;i++){
            int x=8+i*4;
            D(x,8-(i==2?2:0),2,i==2?C(245,89,115):(i%2?C(95,206,225):C(154,233,150)));
        }
        Base(C(142,95,45),C(37,31,39));
        D(16,16,10,C(212,153,53));D(16,16,7,C(51,39,43));
        T(8,13,16,10,14,18,C(255,222,128));T(24,13,18,10,18,18,C(255,222,128));
        O(11,16,2,C(250,244,190));O(21,16,2,C(250,244,190));
        O(11,16,1,C(36,83,69));O(21,16,1,C(36,83,69));
        D(16,19,3,C(66,189,155));D(16,19,1,C(228,255,212));
        L(7,22,11,20,1,C(255,215,105));L(25,22,21,20,1,C(255,215,105));
        L(11,25,21,25,1,C(199,137,56));
        for(int i=0;i<4;i++){
            float a=t*0.7f+i*PI/2.0f;
            O((int)(16+cosf(a)*15),(int)(16+sinf(a)*12),1,C(255,220,110));
        }
        break;
    }
    case 12: { // Wraith Duelist: Phantom hood, split porcelain face and spectral chain.
        O(16,17,15,CA(132,99,255,(unsigned char)(35+20*p)));
        T(3,24,5,5,15,8,C(53,43,104));T(29,24,27,5,17,8,C(91,70,157));
        T(4,24,10,21,8,29,C(95,82,169));T(28,24,22,21,24,29,C(129,102,200));
        T(7,12,14,6,16,10,C(194,180,243));T(25,12,18,6,16,10,C(112,93,196));
        Base(C(92,77,152),C(35,29,72));
        T(7,11,16,7,16,25,C(237,231,247));T(25,11,16,7,16,25,C(176,169,219));
        T(6,23,13,19,10,30,C(102,79,170));T(26,23,19,19,22,30,C(77,63,146));
        O(11,15,2,C(49,34,99));O(21,15,2,C(36,35,100));
        O(11,15,1,C(157,244,255));O(21,15,1,C(213,211,255));
        O(16,21,2,C(43,30,86));O(16,21,1,C(104,239,255));
        L(8,13,12,11,1,C(224,210,255));L(24,13,20,11,1,C(173,159,245));
        for(int i=0;i<4;i++){
            int x=4+i*2,y=17+(i%2)*3;
            R(x,y,2,1,C(211,197,255));
        }
        Star(27,7,2,C(166,248,255));Star(5,9,1,C(231,222,255));
        break;
    }
    case 13: { // Seraphic Vanguard: Angel wings, engraved helm and a gemmed halo.
        float bob=sinf(t*2.0f)*1.0f;
        R(16,8,7,2,C(255,214,104));R(16,8,5,1,C(255,248,208));
        for(int i=0;i<8;i++){
            float a=t*0.2f+i*PI/4.0f;
            int x=(int)(16+cosf(a)*7),y=(int)(8+sinf(a)*7);
            O(x,y,1,C(255,226,135));
        }
        T(3,20,5,7,14,15,C(196,211,240));T(29,20,27,7,18,15,C(245,233,197));
        for(int i=0;i<3;i++){
            int y=13+i*4;
            T(5,y+4,8,y-2,14,y+3,i%2?C(230,238,255):C(255,244,210));
            T(27,y+4,24,y-2,18,y+3,i%2?C(255,244,210):C(230,238,255));
            L(8,y+2,13,y+3,1,C(255,255,244));L(24,y+2,19,y+3,1,C(255,255,244));
        }
        Base(C(182,150,83),C(247,237,213));
        T(7,11,16,5,25,11,C(255,250,229));T(9,11,16,8,23,11,C(222,190,117));
        T(8,13,14,12,16,18,C(255,254,240));T(24,13,18,12,16,18,C(235,226,207));
        O(11,16,2,C(126,176,221));O(21,16,2,C(126,176,221));
        O(11,16,1,C(255,255,255));O(21,16,1,C(255,255,255));
        D(16,12+(int)bob,3,C(96,200,221));D(16,12+(int)bob,1,C(244,255,245));
        L(10,21,14,23,1,C(181,140,74));L(22,21,18,23,1,C(181,140,74));
        B(13,25,6,1,C(184,143,78));Star(16,3,1,C(255,255,233));
        T(8,19,2,10,10,14,C(197,218,248));T(24,19,30,10,22,14,C(255,239,194));
        T(8,22,2,17,10,19,C(231,239,255));T(24,22,30,17,22,19,C(255,250,223));
        T(9,25,4,22,12,21,C(168,202,242));T(23,25,28,22,20,21,C(235,218,176));
        L(4,15,8,17,1,C(255,255,246));L(28,15,24,17,1,C(255,255,246));
        R(16,7,7,1,C(255,222,119));R(16,7,5,1,C(255,250,215));
        O(16,0,1,C(255,255,239));
        break;
    }
    case 14: { // Xeno Prime: Alien antennae, layered shell and three lit eyes.
        float antenna=sinf(t*2.2f)*1.5f;
        O(16,18,15,CA(100,255,101,(unsigned char)(28+24*p)));
        L(10,9,6,2+(int)antenna,2,C(73,151,66));L(22,9,26,2-(int)antenna,2,C(73,151,66));
        O(6,2+(int)antenna,3,C(149,245,95));O(26,2-(int)antenna,3,C(123,229,102));
        O(6,2+(int)antenna,1,C(238,255,176));O(26,2-(int)antenna,1,C(238,255,176));
        T(3,15,7,7,14,11,C(89,165,72));T(29,15,25,7,18,11,C(134,196,74));
        Base(C(84,143,66),C(48,111,62));
        T(6,11,16,6,26,11,C(167,219,94));T(8,11,16,9,24,11,C(214,243,145));
        T(7,13,14,12,16,19,C(80,157,80));T(25,13,18,12,16,19,C(56,127,76));
        T(7,15,14,14,12,17,C(13,22,28));T(25,15,18,14,20,17,C(13,22,28));
        O(11,15,1,C(202,255,150));O(21,15,1,C(202,255,150));
        D(16,18,2,C(235,251,122));O(16,18,1,C(255,255,222));
        B(10,21,12,2,C(29,71,59));
        for(int i=0;i<5;i++)B(11+i*2,22,1,2,C(145,220,112));
        O(7,21,1,C(175,255,131));O(25,21,1,C(175,255,131));
        L(8,25,12,27,1,C(100,196,90));L(24,25,20,27,1,C(100,196,90));
        break;
    }
    case 15: { // Ember Marrow: Skull-inspired carved bone and glowing fracture lines.
        O(16,16,15,CA(255,106,39,(unsigned char)(24+20*p)));
        T(4,13,7,4,13,12,C(223,214,189));T(28,13,25,4,19,12,C(244,231,198));
        T(7,9,11,2,14,10,C(255,220,128));T(25,9,21,2,18,10,C(255,220,128));
        Base(C(139,131,117),C(236,226,201));
        T(6,12,16,8,16,25,C(250,243,220));T(26,12,16,8,16,25,C(211,199,175));
        B(8,13,7,6,C(43,34,42));B(17,13,7,6,C(43,34,42));
        O(11,16,2,C(255,109,41));O(21,16,2,C(255,109,41));
        O(11,16,1,C(255,235,119));O(21,16,1,C(255,235,119));
        T(13,19,19,19,16,23,C(73,53,57));
        for(int i=0;i<4;i++)B(13+i*2,22,1,3,C(247,236,211));
        L(8,12,11,14,1,C(255,247,213));L(24,12,21,14,1,C(255,247,213));
        L(15,9,14,12,1,C(191,67,48));L(14,12,16,14,1,C(255,135,53));
        L(22,20,25,23,1,C(168,59,51));D(16,10,2,C(255,178,62));
        for(int i=0;i<4;i++){
            float a=t*1.4f+i*PI/2.0f;
            Star((int)(16+cosf(a)*14),(int)(14+sinf(a)*12),1,C(255,171,75));
        }
        break;
    }
    case 16: { // Celestial Ascendant: breathing gold aura and rotating prismatic halo.
        float pulse=0.5f+0.5f*sinf(t*1.6f);
        O(16,17,15,CA(255,235,160,(unsigned char)(24+25*pulse)));
        for(int i=0;i<10;i++){
            float a=(float)i*2.0f*PI/10.0f+t*0.45f;
            Color hue=ColorFromHSV(fmodf(t*28.0f+i*36.0f,360.0f),0.64f,1.0f);
            DrawRing((Vector2){16,15},14,16,a*RAD2DEG,(a+PI/8.0f)*RAD2DEG,3,hue);
            L(16+(int)(cosf(a)*11),15+(int)(sinf(a)*11),
              16+(int)(cosf(a)*13),15+(int)(sinf(a)*13),1,C(255,242,187));
        }
        T(2,21,5,9,14,16,C(251,234,178));T(30,21,27,9,18,16,C(236,230,255));
        for(int i=0;i<3;i++){
            int y=14+i*4;
            T(5,y+4,8,y-1,14,y+3,C(255,248,210));
            T(27,y+4,24,y-1,18,y+3,C(215,224,255));
        }
        Base(C(218,183,108),C(250,245,225));
        T(8,11,16,5,24,11,C(255,255,242));T(10,11,16,8,22,11,C(255,219,124));
        O(11,16,2,C(255,255,240));O(21,16,2,C(255,255,240));
        O(11,16,1,C(113,197,237));O(21,16,1,C(233,170,99));
        D(16,19,4,C(255,215,107));D(16,19,2,C(255,255,241));
        for(int i=0;i<3;i++)Star(5+i*11,5+(i%2)*2,1,C(255,250,221));
        break;
    }
    case 17: { // Crimson Blade Warden: Blade Legend mask and crossed steel weapons.
        O(16,17,15,CA(255,42,34,(unsigned char)(30+20*p)));
        L(4,27,27,4,4,C(57,62,78));L(5,27,28,4,1,C(222,230,236));
        L(28,27,5,4,4,C(75,48,58));L(27,27,4,4,1,C(255,127,89));
        B(2,25,7,2,C(147,96,56));B(23,25,7,2,C(179,113,54));
        B(5,7,22,4,C(50,27,42));B(7,7,18,2,C(181,46,48));
        T(7,9,12,3,16,9,C(122,35,48));T(25,9,20,3,16,9,C(195,50,46));
        Base(C(115,37,48),C(36,23,34));
        T(5,12,16,7,27,12,C(155,37,44));B(8,12,16,2,C(200,56,49));
        L(7,14,14,16,2,C(43,17,26));L(25,14,18,16,2,C(43,17,26));
        T(8,16,14,17,12,19,C(255,115,75));T(24,16,18,17,20,19,C(255,115,75));
        O(11,17,1,C(255,235,156));O(21,17,1,C(255,235,156));
        L(9,20,13,21,1,C(249,72,63));L(23,20,19,21,1,C(249,72,63));
        T(11,22,21,22,16,27,C(126,42,50));
        for(int i=0;i<4;i++)B(13+i*2,23,1,2,C(225,187,153));
        L(16,10,14,13,1,C(255,181,106));D(16,11,2,C(255,202,105));
        Star(4,8,1,C(255,160,110));Star(28,8,1,C(255,160,110));
        break;
    }
    }
    // A small family-colored sparkle adds motion without covering the face.
    if((id%3)==1)Star(28,(int)(5+2*p),1,familyColor[id]);
}

// 0 = unrated, 1 = rejected, 2 = maybe/fix, 3 = accepted.
static void LoadRatings(int ratings[N_SKINS]){
    for(int i=0;i<N_SKINS;i++)ratings[i]=0;
    FILE *f=fopen("Library/player_skin_ratings.txt","r");
    if(f){
        char line[160],state[24];
        int id;
        while(fgets(line,sizeof(line),f)){
            if(sscanf(line,"%d %23s",&id,state)!=2||id<0||id>=N_SKINS)continue;
            if(strcmp(state,"REJECTED")==0)ratings[id]=1;
            else if(strcmp(state,"MAYBE/FIX")==0)ratings[id]=2;
            else if(strcmp(state,"ACCEPTED")==0)ratings[id]=3;
        }
        fclose(f);
        return;
    }
    f=fopen("Library/player_skin_selections.txt","r");
    if(f){
        char line[160];
        int id;
        while(fgets(line,sizeof(line),f))
            if(sscanf(line,"%d",&id)==1&&id>=0&&id<N_SKINS)ratings[id]=3;
        fclose(f);
        return;
    }
    f=fopen("Library/player_skin_winner.txt","r");
    if(f){
        int id=-1;
        if(fscanf(f,"%d",&id)==1&&id>=0&&id<N_SKINS)ratings[id]=3;
        fclose(f);
    }
}
static void SaveRatings(const int ratings[N_SKINS]){
    FILE *f=fopen("Library/player_skin_ratings.txt","w");
    if(!f)return;
    const char *states[4]={"", "REJECTED", "MAYBE/FIX", "ACCEPTED"};
    for(int i=0;i<N_SKINS;i++)if(ratings[i]>0&&ratings[i]<=3)
        fprintf(f,"%d %s %s\n",i,states[ratings[i]],skinNames[i]);
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
    if(argc>1 && strncmp(argv[1],"--capture-page=",15)==0)
        capturePage=atoi(argv[1]+15)-1;
    if(capturePage>=PAGE_COUNT)capturePage=PAGE_COUNT-1;
    SetConfigFlags(FLAG_MSAA_4X_HINT|FLAG_WINDOW_RESIZABLE);
    InitWindow(1240,780,"New player skins - 18 ideas");
    SetTargetFPS(60);
    ChangeDirectory(GetApplicationDirectory());
    char designDirectory[512];
    snprintf(designDirectory,sizeof(designDirectory),"%sLibrary",GetApplicationDirectory());
    MakeDirectory(designDirectory);
    RenderTexture2D previews[PAGE_SIZE][3];
    for(int i=0;i<PAGE_SIZE;i++)for(int bg=0;bg<3;bg++){
        previews[i][bg]=LoadRenderTexture(32,32);
        SetTextureFilter(previews[i][bg].texture,TEXTURE_FILTER_POINT);
    }
    int page=capturePage>=0?capturePage:0;
    int ratings[N_SKINS];
    LoadRatings(ratings);
    while(!WindowShouldClose()){
        float wheel=GetMouseWheelMove();
        if(IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_PAGE_DOWN)||wheel<0)page=(page+1)%PAGE_COUNT;
        if(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_PAGE_UP)||wheel>0)page=(page+PAGE_COUNT-1)%PAGE_COUNT;
        if(IsKeyPressed(KEY_ZERO)){
            for(int i=0;i<N_SKINS;i++)ratings[i]=0;
            SaveRatings(ratings);
        }
        for(int i=0;i<PAGE_SIZE;i++)if(IsKeyPressed(KEY_ONE+i)||IsKeyPressed(KEY_KP_1+i)){
            int value=(IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL))?1:
                ((IsKeyDown(KEY_LEFT_SHIFT)||IsKeyDown(KEY_RIGHT_SHIFT))?2:3);
            int id=page*PAGE_SIZE+i;
            ratings[id]=ratings[id]==value?0:value;
            SaveRatings(ratings);
        }
        float t=(float)GetTime();
        for(int i=0;i<PAGE_SIZE;i++)for(int bg=0;bg<3;bg++){
            BeginTextureMode(previews[i][bg]);
            Backdrop(bg);DrawSkin(page*PAGE_SIZE+i,t);
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
        ClearBackground(C(9,9,17));
        DrawRectangleGradientV(0,0,sw,sh,C(12,13,25),C(21,13,29));
        BeginMode2D((Camera2D){.offset={(float)ox,(float)oy},.target={0,0},.rotation=0,.zoom=scale});
        DrawText("NEW PLAYER SKINS",42,22,30,C(244,223,187));
        DrawText("1-6: ACCEPT  |  SHIFT+1-6: MAYBE / FIX  |  CTRL+1-6: REJECT",43,59,16,C(161,169,190));
        DrawText(TextFormat("PAGE %d / %d",page+1,PAGE_COUNT),1077,28,21,C(132,231,217));
        for(int i=0;i<PAGE_SIZE;i++){
            int id=page*PAGE_SIZE+i;
            int col=i%3,row=i/3,x=42+col*390,y=94+row*324;
            Rectangle card={(float)x,(float)y,374,310};
            bool hot=CheckCollisionPointRec(local,card);
            DrawRectangleRounded(card,0.07f,6,hot?C(41,35,59):C(28,25,42));
            Color edge=ratings[id]==1?C(247,104,122):(ratings[id]==2?C(246,189,100):
                (ratings[id]==3?C(105,230,194):(hot?C(131,209,220):C(70,61,87))));
            DrawRectangleRoundedLinesEx(card,0.07f,6,ratings[id]?3:1,edge);
            DrawText(TextFormat("%d  %s",id+1,skinNames[id]),x+18,y+13,19,C(244,232,210));
            DrawText(skinNotes[id],x+19,y+41,15,C(179,168,195));
            DrawText(families[id],x+19,y+238,13,familyColor[id]);
            Rectangle src={0,0,32,-32},dst={(float)x+20,(float)y+72,160,160};
            DrawTexturePro(previews[i][0].texture,src,dst,(Vector2){0,0},0,WHITE);
            DrawRectangleLinesEx(dst,2,C(108,98,127));
            const char *bgLabel[3]={"FLOOR","WALL","PAD"};
            for(int bg=0;bg<3;bg++){
                int px=x+213,py=y+73+bg*52;
                DrawTexturePro(previews[i][bg].texture,src,
                    (Rectangle){(float)px,(float)py,32,32},(Vector2){0,0},0,WHITE);
                DrawText(bgLabel[bg],px+43,py+8,15,C(169,181,202));
            }
            DrawText("ACTUAL SIZE",x+212,y+235,13,C(128,223,215));
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
        DrawText("LEFT / RIGHT: page   |   0: clear ratings   |   ESC: close",42,747,15,C(144,144,164));
        int totals[4]={0};
        for(int i=0;i<N_SKINS;i++)totals[ratings[i]]++;
        DrawText(TextFormat("REJECTED %d   MAYBE / FIX %d   ACCEPTED %d",
                            totals[1],totals[2],totals[3]),760,747,15,C(228,199,140));
        EndMode2D();
        if(clicked>=0){
            ratings[clicked]=ratings[clicked]==clickedValue?0:clickedValue;
            SaveRatings(ratings);
        }
        if(capturePage>=0){
            char shot[512];
            snprintf(shot,sizeof(shot),"%s/Library/player_skins_page_%d.png",
                     GetApplicationDirectory(),page+1);
            rlDrawRenderBatchActive();
            Image frame=LoadImageFromScreen();
            ExportImage(frame,shot);
            UnloadImage(frame);
        }
        EndDrawing();
        if(capturePage>=0)break;
    }
    for(int i=0;i<PAGE_SIZE;i++)for(int bg=0;bg<3;bg++)UnloadRenderTexture(previews[i][bg]);
    CloseWindow();
    return 0;
}
