// skinideas.c — interactive picker for alternative designs of the 6
// achievement-locked skins (Chronovore, Deep Cartographer, Midas, Blade
// Legend, Constellation, Ascendant). Fully independent of the rest of the
// project — does not include any of its headers, just raylib.
//
// Build (Git Bash, using the bundled w64devkit toolchain), from this folder:
//   export PATH="/c/raylib/w64devkit/bin:$PATH"
//   gcc -O2 -o skinideas.exe skinideas.c -I"../../raylib/raylib/src" -L"../../raylib/raylib/src" -lraylib -lopengl32 -lgdi32 -lwinmm
// (Or use the absolute raylib path, e.g. -I"/c/raylib/raylib/src" -L"/c/raylib/raylib/src")
//
// Run: ./skinideas.exe   (run from inside Skin_Ideas/ so it finds skin_data.txt)
//
// Names/labels/descriptions for every skin + variant are loaded from
// skin_data.txt at startup (not hardcoded here) — edit that file to rename
// or re-describe an idea without touching this source.
//
// Click any preview to mark it BAD: it's removed from its skin's grid and
// logged to bad_skins.txt (persists across runs). Rejected ideas collect in
// a "REJECTED" section at the bottom of the scroll view — click one there to
// restore it.

#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

typedef unsigned char uchar;

#define NSKIN 6
#define NVAR  8

// ─── Small shared helpers (self-contained, not shared with skins.c) ─────────
static void EllipseRotF(float cx, float cy, float rx, float ry, float ang, Color col) {
    const int SEG = 24;
    Vector2 pts[SEG + 1];
    float ca = cosf(ang), sa = sinf(ang);
    for (int i = 0; i <= SEG; i++) {
        float th = (float)i / SEG * 2.0f * PI;
        float ex = cosf(th) * rx, ey = sinf(th) * ry;
        pts[i] = (Vector2){ cx + ex*ca - ey*sa, cy + ex*sa + ey*ca };
    }
    for (int i = 0; i < SEG; i++)
        DrawTriangle((Vector2){cx, cy}, pts[i + 1], pts[i], col);
}

static void HaloArc(float cx, float cy, float rx, float ry,
                     float a0, float a1, float thick, Color col) {
    const int SEG = 24;
    Vector2 prev = {0};
    for (int i = 0; i <= SEG; i++) {
        float th = a0 + (a1 - a0) * ((float)i / SEG);
        Vector2 p = { cx + cosf(th)*rx, cy + sinf(th)*ry };
        if (i > 0) DrawLineEx(prev, p, thick, col);
        prev = p;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// CHRONOVORE — 8 variants (Rare, tied to Veteran Tier 6)
// ═══════════════════════════════════════════════════════════════════════════
static Color ChronovoreColor(float gt) {
    float p = 0.5f + 0.5f*sinf(gt * 2.0f);
    return (Color){(uchar)(140+(int)(60*p)), (uchar)(100+(int)(50*p)), (uchar)(40+(int)(20*p)), 255};
}
static void Chronovore_V1(float cx, float cy, float R, float t) {
    Color body = ChronovoreColor(t);
    DrawCircleV((Vector2){cx, cy}, R*1.30f, (Color){200,150,60,26});
    for (int k=0;k<12;k++){
        float ang = (float)k/12.0f * 2.0f*PI;
        float ix = cx + cosf(ang)*R*1.18f, iy = cy + sinf(ang)*R*1.18f;
        float ox = cx + cosf(ang)*R*1.30f, oy = cy + sinf(ang)*R*1.30f;
        Color tc = (k%3==0) ? (Color){230,190,110,150} : (Color){180,140,80,80};
        DrawLineEx((Vector2){ix,iy},(Vector2){ox,oy}, R*0.05f, tc);
    }
    DrawCircle((int)cx,(int)cy,(int)R, body);
    DrawCircleV((Vector2){cx,cy}, R*0.62f, (Color){60,42,20,200});
    DrawCircleLines((int)cx,(int)cy,(int)(R*0.62f),(Color){230,190,110,200});
    float hourAng = t*0.6f, minAng = t*2.4f;
    DrawLineEx((Vector2){cx,cy}, (Vector2){cx+cosf(hourAng)*R*0.30f, cy+sinf(hourAng)*R*0.30f}, R*0.09f, (Color){245,220,160,255});
    DrawLineEx((Vector2){cx,cy}, (Vector2){cx+cosf(minAng)*R*0.48f,  cy+sinf(minAng)*R*0.48f},  R*0.06f, (Color){245,220,160,220});
    DrawCircleV((Vector2){cx,cy}, R*0.07f, (Color){255,235,180,255});
}
static void Chronovore_V2(float cx, float cy, float R, float t) {
    Color glass = (Color){60,50,70,180};
    Color sand  = (Color){235,190,110,255};
    DrawCircleV((Vector2){cx, cy}, R*1.25f, (Color){255,210,110,20});
    DrawTriangle((Vector2){cx-R*0.75f,cy-R*0.95f},(Vector2){cx+R*0.75f,cy-R*0.95f},(Vector2){cx,cy}, glass);
    DrawTriangle((Vector2){cx-R*0.75f,cy+R*0.95f},(Vector2){cx+R*0.75f,cy+R*0.95f},(Vector2){cx,cy}, glass);
    float fill = 0.5f+0.5f*sinf(t*0.5f);
    DrawTriangle((Vector2){cx-R*0.55f*fill,cy-R*0.90f},(Vector2){cx+R*0.55f*fill,cy-R*0.90f},(Vector2){cx,cy-R*0.20f}, sand);
    DrawTriangle((Vector2){cx-R*0.55f*(1.0f-fill),cy+R*0.90f},(Vector2){cx+R*0.55f*(1.0f-fill),cy+R*0.90f},(Vector2){cx,cy+R*0.20f}, sand);
    for (int k=0;k<3;k++) {
        float dy = fmodf(t*2.5f + k*0.6f, 1.0f);
        DrawCircleV((Vector2){cx, cy-R*0.5f+dy*R*1.0f}, R*0.04f, sand);
    }
    DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.15f}, R*0.09f, (Color){255,235,180,255});
    DrawCircleV((Vector2){cx+R*0.28f, cy-R*0.15f}, R*0.09f, (Color){255,235,180,255});
}
static void Chronovore_V3(float cx, float cy, float R, float t) {
    Color body = (Color){50,45,70,255};
    DrawCircleV((Vector2){cx, cy}, R*1.30f, (Color){80,220,255,22});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    float p = 0.5f+0.5f*sinf(t*3.0f);
    Color crack = (Color){80,230,255,(uchar)(160+80*p)};
    DrawLineEx((Vector2){cx-R*0.6f,cy-R*0.7f},(Vector2){cx-R*0.1f,cy-R*0.1f}, R*0.05f, crack);
    DrawLineEx((Vector2){cx-R*0.1f,cy-R*0.1f},(Vector2){cx+R*0.4f,cy-R*0.5f}, R*0.05f, crack);
    DrawLineEx((Vector2){cx-R*0.1f,cy-R*0.1f},(Vector2){cx+R*0.1f,cy+R*0.6f}, R*0.05f, crack);
    DrawLineEx((Vector2){cx+R*0.1f,cy+R*0.6f},(Vector2){cx-R*0.5f,cy+R*0.8f}, R*0.04f, crack);
    for (int k=0;k<4;k++) {
        float ang = t*0.7f + k*1.7f;
        float rr  = R*(1.05f+0.10f*sinf(t*1.3f+k));
        Vector2 s = { cx+cosf(ang)*rr, cy+sinf(ang)*rr };
        DrawRectanglePro((Rectangle){s.x,s.y,R*0.14f,R*0.14f}, (Vector2){R*0.07f,R*0.07f}, ang*40.0f, (Color){80,230,255,180});
    }
    EllipseRotF(cx-R*0.30f, cy-R*0.02f, R*0.10f, R*0.14f, 0.0f, crack);
    EllipseRotF(cx+R*0.30f, cy-R*0.02f, R*0.10f, R*0.14f, 0.0f, crack);
}
static void Chronovore_V4(float cx, float cy, float R, float t) {
    Color body = (Color){80,60,35,255};
    DrawCircleV((Vector2){cx, cy}, R*1.25f, (Color){200,160,70,20});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int k=0;k<16;k++) {
        float ang = (2.0f*PI/16)*k;
        DrawCircleV((Vector2){cx+cosf(ang)*R*1.05f, cy+sinf(ang)*R*1.05f}, R*0.045f, (Color){210,180,110,200});
    }
    DrawCircleV((Vector2){cx, cy+R*0.05f}, R*0.62f, (Color){235,222,190,255});
    DrawCircleLines((int)cx,(int)(cy+R*0.05f),(int)(R*0.62f),(Color){110,85,40,255});
    for (int k=0;k<12;k++) {
        float ang = (2.0f*PI/12)*k;
        Vector2 a = { cx+cosf(ang)*R*0.54f, cy+R*0.05f+sinf(ang)*R*0.54f };
        Vector2 b = { cx+cosf(ang)*R*0.46f, cy+R*0.05f+sinf(ang)*R*0.46f };
        DrawLineEx(a,b,R*0.025f,(Color){90,70,30,220});
    }
    float ang = t*1.2f;
    DrawLineEx((Vector2){cx,cy+R*0.05f}, (Vector2){cx+cosf(ang)*R*0.40f, cy+R*0.05f+sinf(ang)*R*0.40f}, R*0.05f, (Color){40,30,15,255});
    float sw = sinf(t*1.0f)*0.3f;
    DrawLineEx((Vector2){cx,cy-R*0.62f},(Vector2){cx+sinf(sw)*R*0.4f, cy-R*1.1f}, R*0.035f, (Color){210,180,110,200});
}
static void Chronovore_V5(float cx, float cy, float R, float t) {
    Color body = (Color){40,25,55,255};
    DrawCircleV((Vector2){cx, cy}, R*1.35f, (Color){130,70,220,26});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int arm=0; arm<3; arm++) {
        float baseAng = t*1.5f + arm*(2.0f*PI/3.0f);
        Vector2 prev = {cx,cy};
        for (int s=1; s<=10; s++) {
            float frac = s/10.0f;
            float ang = baseAng + frac*4.0f;
            float rr = R*0.85f*frac;
            Vector2 p = { cx+cosf(ang)*rr, cy+sinf(ang)*rr };
            DrawLineEx(prev, p, R*0.05f*(1.0f-frac*0.5f), (Color){170,110,255,(uchar)(220*(1.0f-frac))});
            prev = p;
        }
    }
    DrawCircleV((Vector2){cx,cy}, R*0.14f, (Color){230,210,255,255});
}
static void Chronovore_V6(float cx, float cy, float R, float t) {
    Color body = ChronovoreColor(t);
    for (int e=3; e>=1; e--) {
        float off = sinf(t*2.0f - e*0.6f)*R*0.35f;
        DrawCircle((int)(cx+off),(int)cy,(int)R, (Color){body.r,body.g,body.b,(uchar)(60/e)});
    }
    DrawCircle((int)cx,(int)cy,(int)R, body);
    EllipseRotF(cx-R*0.28f, cy-R*0.02f, R*0.10f, R*0.14f, 0.0f, (Color){40,25,10,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.02f, R*0.10f, R*0.14f, 0.0f, (Color){40,25,10,255});
    DrawTriangle((Vector2){cx+R*0.10f,cy+R*0.25f},(Vector2){cx+R*0.10f,cy+R*0.45f},(Vector2){cx-R*0.10f,cy+R*0.35f}, (Color){230,210,150,220});
}
static void Chronovore_V7(float cx, float cy, float R, float t) {
    Color body = (Color){90,65,35,255};
    DrawCircleV((Vector2){cx, cy}, R*1.25f, (Color){200,150,60,18});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    int teeth=14;
    for (int k=0;k<teeth;k++) {
        float ang=(2.0f*PI/teeth)*k;
        DrawLineEx((Vector2){cx+cosf(ang)*R*0.98f,cy+sinf(ang)*R*0.98f},
                   (Vector2){cx+cosf(ang)*R*1.14f,cy+sinf(ang)*R*1.14f}, R*0.07f,(Color){170,130,60,220});
    }
    float swing = sinf(t*1.6f)*0.5f;
    Vector2 pTip = { cx+sinf(swing)*R*0.55f, cy+R*0.75f };
    DrawLineEx((Vector2){cx,cy-R*0.1f}, pTip, R*0.05f, (Color){60,45,20,255});
    DrawCircleV(pTip, R*0.16f, (Color){215,180,90,255});
    DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.35f}, R*0.08f, (Color){40,25,10,255});
    DrawCircleV((Vector2){cx+R*0.28f, cy-R*0.35f}, R*0.08f, (Color){40,25,10,255});
}
static void Chronovore_V8(float cx, float cy, float R, float t) {
    Color body = (Color){55,40,60,255};
    DrawCircleV((Vector2){cx, cy}, R*1.35f, (Color){200,150,255,18});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    int seg=28;
    for (int k=0;k<seg;k++) {
        float frac=(float)k/seg;
        float ang = frac*2.0f*PI + t*0.8f;
        Color c = ColorFromHSV(fmodf(frac*300.0f+ t*20.0f,360.0f), 0.6f, 1.0f);
        DrawCircleV((Vector2){cx+cosf(ang)*R*1.18f, cy+sinf(ang)*R*1.18f}, R*0.045f, c);
    }
    float headAng = t*0.8f;
    Vector2 headP = { cx+cosf(headAng)*R*1.18f, cy+sinf(headAng)*R*1.18f };
    DrawTriangle(headP,
        (Vector2){headP.x-cosf(headAng+0.3f)*R*0.16f, headP.y-sinf(headAng+0.3f)*R*0.16f},
        (Vector2){headP.x-cosf(headAng-0.3f)*R*0.16f, headP.y-sinf(headAng-0.3f)*R*0.16f}, (Color){255,255,255,230});
    DrawCircleV((Vector2){cx-R*0.26f, cy-R*0.02f}, R*0.08f, (Color){230,210,255,240});
    DrawCircleV((Vector2){cx+R*0.26f, cy-R*0.02f}, R*0.08f, (Color){230,210,255,240});
}

// ═══════════════════════════════════════════════════════════════════════════
// DEEP CARTOGRAPHER — 8 variants (Rare, tied to Explorer Tier 6)
// ═══════════════════════════════════════════════════════════════════════════
static Color CartographerColor(float gt) {
    float p = 0.5f + 0.5f*sinf(gt * 1.2f);
    return (Color){(uchar)(200+(int)(30*p)), (uchar)(170+(int)(25*p)), (uchar)(110+(int)(20*p)), 255};
}
static void Cartographer_V1(float cx, float cy, float R, float t) {
    Color body = CartographerColor(t);
    DrawCircleV((Vector2){cx, cy}, R*1.28f, (Color){210,180,120,24});
    for (int k=0;k<8;k++){
        float ang = (float)k/8.0f * 2.0f*PI + t*0.15f;
        float ox = cx + cosf(ang)*R*1.24f, oy = cy + sinf(ang)*R*1.24f;
        DrawLineEx((Vector2){cx,cy},(Vector2){ox,oy}, R*0.03f, (Color){190,160,100,40});
    }
    DrawCircle((int)cx,(int)cy,(int)R, body);
    DrawCircleV((Vector2){cx,cy}, R*0.60f, (Color){70,54,30,190});
    DrawCircleLines((int)cx,(int)cy,(int)(R*0.60f),(Color){230,200,140,200});
    for (int k=0;k<4;k++){
        float ang = (float)k/4.0f * 2.0f*PI;
        Vector2 tip = { cx+cosf(ang)*R*0.54f, cy+sinf(ang)*R*0.54f };
        DrawTriangle((Vector2){cx,cy},(Vector2){cx+cosf(ang+0.10f)*R*0.14f, cy+sinf(ang+0.10f)*R*0.14f}, tip,
                     (k%2==0)?(Color){225,60,50,220}:(Color){225,205,170,220});
    }
    float needleAng = sinf(t*0.8f)*0.5f;
    DrawLineEx((Vector2){cx-cosf(needleAng-PI/2)*R*0.3f, cy-sinf(needleAng-PI/2)*R*0.3f},
               (Vector2){cx+cosf(needleAng-PI/2)*R*0.5f, cy+sinf(needleAng-PI/2)*R*0.5f}, R*0.07f, (Color){235,60,50,240});
    DrawCircleV((Vector2){cx,cy}, R*0.06f, (Color){250,235,200,255});
}
static void Cartographer_V2(float cx, float cy, float R, float t) {
    Color body = (Color){190,175,110,255};
    DrawCircleV((Vector2){cx, cy}, R*1.24f, (Color){160,200,120,20});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int k=0;k<4;k++) {
        float rr = R*(0.30f + k*0.18f) + sinf(t*0.6f+k)*R*0.02f;
        DrawCircleLines((int)(cx), (int)(cy+R*0.10f), (int)rr, (Color){90,60,20,120});
    }
    DrawTriangle((Vector2){cx-R*0.30f,cy-R*0.05f},(Vector2){cx,cy-R*0.55f},(Vector2){cx+R*0.10f,cy-R*0.05f},(Color){110,80,40,220});
    DrawTriangle((Vector2){cx-R*0.05f,cy-R*0.05f},(Vector2){cx+R*0.22f,cy-R*0.40f},(Vector2){cx+R*0.42f,cy-R*0.05f},(Color){110,80,40,220});
    DrawTriangle((Vector2){cx+R*0.02f,cy-R*0.30f},(Vector2){cx+R*0.22f,cy-R*0.40f},(Vector2){cx+R*0.30f,cy-R*0.20f},(Color){250,245,235,230});
    DrawCircleV((Vector2){cx-R*0.34f, cy+R*0.40f}, R*0.07f, (Color){40,30,10,255});
    DrawCircleV((Vector2){cx+R*0.34f, cy+R*0.40f}, R*0.07f, (Color){40,30,10,255});
}
static void Cartographer_V3(float cx, float cy, float R, float t) {
    Color body = (Color){30,35,70,255};
    DrawCircleV((Vector2){cx, cy}, R*1.30f, (Color){255,215,120,20});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    Color ink = (Color){230,190,110,220};
    Vector2 pts[5] = {
        { cx-R*0.4f, cy-R*0.3f }, { cx-R*0.1f, cy-R*0.5f }, { cx+R*0.25f, cy-R*0.2f },
        { cx+R*0.1f, cy+R*0.2f }, { cx-R*0.2f, cy+R*0.35f }
    };
    for (int i=0;i<4;i++) DrawLineEx(pts[i], pts[i+1], R*0.025f, ink);
    for (int i=0;i<5;i++) DrawCircleV(pts[i], R*0.045f, (Color){255,235,180,(uchar)(200+40*sinf(t*3.0f+i))});
    HaloArc(cx, cy+R*0.05f, R*0.85f, R*0.30f, 0.15f*PI, 0.85f*PI, R*0.05f, ink);
    DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.02f}, R*0.08f, (Color){255,235,180,255});
    DrawCircleV((Vector2){cx+R*0.28f, cy-R*0.02f}, R*0.08f, (Color){255,235,180,255});
}
static void Cartographer_V4(float cx, float cy, float R, float t) {
    Color body = (Color){215,190,140,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int k=0;k<6;k++) {
        float ang=(2.0f*PI/6)*k+0.3f;
        DrawCircleV((Vector2){cx+cosf(ang)*R*0.96f, cy+sinf(ang)*R*0.96f}, R*0.10f, (Color){120,85,40,120});
    }
    Vector2 path[4] = { {cx-R*0.5f,cy+R*0.4f},{cx-R*0.1f,cy+R*0.1f},{cx+R*0.2f,cy-R*0.2f},{cx+R*0.45f,cy-R*0.5f} };
    for (int i=0;i<3;i++) {
        Vector2 a=path[i], b=path[i+1];
        for (int d=0; d<=4; d++) {
            float f=(float)d/4;
            DrawCircleV((Vector2){a.x+(b.x-a.x)*f, a.y+(b.y-a.y)*f}, R*0.025f, (Color){90,60,25,220});
        }
    }
    float p=0.5f+0.5f*sinf(t*3.0f);
    Color xc=(Color){200,30,20,(uchar)(200+40*p)};
    DrawLineEx((Vector2){path[3].x-R*0.08f,path[3].y-R*0.08f},(Vector2){path[3].x+R*0.08f,path[3].y+R*0.08f}, R*0.045f, xc);
    DrawLineEx((Vector2){path[3].x-R*0.08f,path[3].y+R*0.08f},(Vector2){path[3].x+R*0.08f,path[3].y-R*0.08f}, R*0.045f, xc);
    DrawCircleV((Vector2){cx-R*0.30f, cy+R*0.05f}, R*0.06f, (Color){60,40,20,255});
    DrawCircleV((Vector2){cx+R*0.10f, cy+R*0.20f}, R*0.06f, (Color){60,40,20,255});
}
static void Cartographer_V5(float cx, float cy, float R, float t) {
    Color body = (Color){60,110,140,255};
    DrawCircleV((Vector2){cx, cy}, R*1.2f, (Color){80,180,255,18});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int k=0;k<3;k++) {
        float rh = R*fabsf(cosf(t*0.4f + k*1.05f));
        if (rh < R*0.08f) rh = R*0.08f;
        DrawEllipseLines((int)cx,(int)cy,rh,R,(Color){200,230,255,140});
    }
    DrawEllipseLines((int)cx,(int)cy,R,R*0.30f,(Color){200,230,255,110});
    DrawEllipseLines((int)cx,(int)cy,R,R*0.62f,(Color){200,230,255,90});
    DrawLine((int)(cx-R),(int)cy,(int)(cx+R),(int)cy,(Color){200,230,255,170});
    float ang=t*0.8f;
    Vector2 ship={cx+cosf(ang)*R*0.95f, cy+sinf(ang)*R*0.20f};
    DrawTriangle((Vector2){ship.x-R*0.06f,ship.y},(Vector2){ship.x+R*0.06f,ship.y},(Vector2){ship.x,ship.y-R*0.12f}, (Color){255,235,200,255});
    DrawCircleV((Vector2){cx-R*0.26f, cy-R*0.30f}, R*0.06f, (Color){230,245,255,230});
    DrawCircleV((Vector2){cx+R*0.26f, cy-R*0.30f}, R*0.06f, (Color){230,245,255,230});
}
static void Cartographer_V6(float cx, float cy, float R, float t) {
    Color body = (Color){225,210,175,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    Vector2 prev={cx,cy};
    for (int s=1;s<=24;s++) {
        float frac=s/24.0f;
        float ang= frac*10.0f + t*0.5f;
        float rr = R*0.75f*frac;
        Vector2 p={cx+cosf(ang)*rr, cy+sinf(ang)*rr};
        DrawLineEx(prev,p,R*0.03f,(Color){30,25,60,200});
        prev=p;
    }
    for (int k=0;k<6;k++) {
        float ang=k*1.15f;
        DrawCircleV((Vector2){cx+cosf(ang)*R*0.5f, cy+sinf(ang)*R*0.4f}, R*0.03f, (Color){30,25,60,160});
    }
    DrawTriangle(prev, (Vector2){prev.x-R*0.18f,prev.y-R*0.30f}, (Vector2){prev.x+R*0.05f,prev.y-R*0.10f}, (Color){240,235,225,230});
    DrawCircleV((Vector2){cx-R*0.24f, cy-R*0.30f}, R*0.06f, (Color){30,25,60,220});
    DrawCircleV((Vector2){cx+R*0.24f, cy-R*0.30f}, R*0.06f, (Color){30,25,60,220});
}
static void Cartographer_V7(float cx, float cy, float R, float t) {
    Color body = (Color){235,235,240,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int k=-2;k<=2;k+=2)
        DrawRectangle((int)(cx-R), (int)(cy+k*R*0.22f-R*0.10f), (int)(R*2), (int)(R*0.20f), (Color){210,40,30,220});
    float ang = t*2.2f;
    Color beam=(Color){255,240,150,90};
    DrawTriangle((Vector2){cx,cy-R*0.1f},
                 (Vector2){cx+cosf(ang-0.25f)*R*2.0f, cy-R*0.1f+sinf(ang-0.25f)*R*2.0f},
                 (Vector2){cx+cosf(ang+0.25f)*R*2.0f, cy-R*0.1f+sinf(ang+0.25f)*R*2.0f}, beam);
    DrawCircleV((Vector2){cx-R*0.26f, cy-R*0.10f}, R*0.09f, (Color){255,230,120,255});
    DrawCircleV((Vector2){cx+R*0.26f, cy-R*0.10f}, R*0.09f, (Color){255,230,120,255});
}
static void Cartographer_V8(float cx, float cy, float R, float t) {
    Color body = (Color){140,100,60,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int k=0;k<6;k++) {
        float ang=(2.0f*PI/6)*k;
        DrawLineEx((Vector2){cx+cosf(ang)*R*0.3f,cy+sinf(ang)*R*0.3f},
                   (Vector2){cx+cosf(ang)*R*0.9f,cy+sinf(ang)*R*0.9f}, R*0.02f,(Color){90,60,30,180});
    }
    for (int k=0;k<5;k++) {
        float ang = k*(2.0f*PI/5.0f) + t*0.2f;
        Vector2 p = { cx+cosf(ang)*R*0.85f, cy+sinf(ang)*R*0.85f };
        Color pc = ColorFromHSV(fmodf(k*70.0f,360.0f),0.7f,1.0f);
        DrawCircleV(p, R*0.07f, pc);
        DrawCircleV(p, R*0.03f, (Color){255,255,255,200});
    }
    DrawCircleV((Vector2){cx-R*0.26f, cy-R*0.05f}, R*0.07f, (Color){40,25,10,255});
    DrawCircleV((Vector2){cx+R*0.26f, cy-R*0.05f}, R*0.07f, (Color){40,25,10,255});
}

// ═══════════════════════════════════════════════════════════════════════════
// MIDAS — 8 variants (Epic, tied to Coin Hoarder + Rich Tier 6)
// ═══════════════════════════════════════════════════════════════════════════
static Color MidasColor(float gt) {
    float p = 0.5f + 0.5f*sinf(gt * 2.5f);
    return (Color){255, (uchar)(190+(int)(45*p)), (uchar)(20+(int)(40*p)), 255};
}
static void Midas_V1(float cx, float cy, float R, float t) {
    Color body = MidasColor(t);
    DrawCircleV((Vector2){cx, cy}, R*1.38f, (Color){255,210,60,20});
    for (int d=-1; d<=1; d++) {
        float dy = fmodf(t*1.6f + d*1.3f, 2.2f);
        float bx = cx + d*R*0.55f, by = cy + R*0.85f + dy*R*0.55f;
        float sh = 1.0f - dy*0.35f;
        DrawCircleV((Vector2){bx, by}, R*0.11f*sh, (Color){255,215,60,(uchar)(210*sh)});
    }
    DrawCircle((int)cx,(int)cy,(int)R, body);
    EllipseRotF(cx-R*0.32f, cy-R*0.05f, R*0.16f, R*0.20f, 0.0f, (Color){255,240,180,255});
    EllipseRotF(cx+R*0.32f, cy-R*0.05f, R*0.16f, R*0.20f, 0.0f, (Color){255,240,180,255});
    DrawCircleV((Vector2){cx-R*0.32f, cy-R*0.05f}, R*0.07f, (Color){140,90,10,255});
    DrawCircleV((Vector2){cx+R*0.32f, cy-R*0.05f}, R*0.07f, (Color){140,90,10,255});
}
static void Midas_V2(float cx, float cy, float R, float t) {
    Color body = (Color){190,150,60,255};
    DrawCircleV((Vector2){cx, cy}, R*1.25f, (Color){255,200,60,16});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    Color crack = (Color){100,75,25,200};
    DrawLineEx((Vector2){cx-R*0.5f,cy-R*0.6f},(Vector2){cx-R*0.1f,cy-R*0.1f}, R*0.035f, crack);
    DrawLineEx((Vector2){cx+R*0.3f,cy-R*0.5f},(Vector2){cx+R*0.05f,cy+R*0.1f}, R*0.035f, crack);
    DrawLineEx((Vector2){cx-R*0.05f,cy+R*0.1f},(Vector2){cx-R*0.4f,cy+R*0.5f}, R*0.03f, crack);
    float p = 0.5f+0.5f*sinf(t*3.5f);
    Color glow = (Color){255,120,20,(uchar)(200+55*p)};
    DrawCircleV((Vector2){cx-R*0.30f, cy-R*0.02f}, R*0.10f, glow);
    DrawCircleV((Vector2){cx+R*0.30f, cy-R*0.02f}, R*0.10f, glow);
    DrawCircleV((Vector2){cx-R*0.30f, cy-R*0.02f}, R*0.05f, (Color){255,230,200,255});
    DrawCircleV((Vector2){cx+R*0.30f, cy-R*0.02f}, R*0.05f, (Color){255,230,200,255});
}
static void Midas_V3(float cx, float cy, float R, float t) {
    Color body = MidasColor(t);
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int row=-1; row<=1; row++) {
        for (int k=-2; k<=2; k++) {
            float ox = k*R*0.34f, oy = row*R*0.34f + (row!=0? R*0.10f*(k%2):0);
            if (ox*ox+oy*oy > R*R*0.85f) continue;
            DrawCircleV((Vector2){cx+ox, cy+oy}, R*0.16f, (Color){255,215,70,255});
            DrawCircleLines((int)(cx+ox),(int)(cy+oy),(int)(R*0.16f),(Color){160,110,10,180});
        }
    }
    for (int k=0;k<3;k++) {
        float ang = t*1.5f + k*(2.0f*PI/3.0f);
        Vector2 c = { cx+cosf(ang)*R*0.9f, cy-R*1.15f+sinf(ang)*R*0.2f };
        DrawCircleV(c, R*0.13f, (Color){255,225,90,220});
    }
    DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.02f}, R*0.06f, (Color){60,40,5,255});
    DrawCircleV((Vector2){cx+R*0.28f, cy-R*0.02f}, R*0.06f, (Color){60,40,5,255});
}
static void Midas_V4(float cx, float cy, float R, float t) {
    Color body = (Color){230,190,60,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    float p=0.5f+0.5f*sinf(t*2.0f);
    DrawCircleV((Vector2){cx-R*0.2f, cy+R*0.25f}, R*0.5f*(0.7f+0.3f*p), (Color){255,235,150,(uchar)(70+40*p)});
    for (int f=0; f<5; f++) {
        float ang=-0.6f+f*0.3f;
        Vector2 tip={cx-R*0.2f+cosf(ang)*R*0.55f, cy+R*0.25f+sinf(ang)*R*0.55f};
        DrawLineEx((Vector2){cx-R*0.2f,cy+R*0.25f}, tip, R*0.05f, (Color){255,230,120,180});
    }
    EllipseRotF(cx-R*0.30f, cy-R*0.05f, R*0.14f, R*0.18f, 0.0f, (Color){255,230,150,255});
    EllipseRotF(cx+R*0.30f, cy-R*0.05f, R*0.14f, R*0.18f, 0.0f, (Color){255,230,150,255});
    DrawCircleV((Vector2){cx-R*0.30f, cy-R*0.05f}, R*0.06f, (Color){120,80,10,255});
    DrawCircleV((Vector2){cx+R*0.30f, cy-R*0.05f}, R*0.06f, (Color){120,80,10,255});
}
static void Midas_V5(float cx, float cy, float R, float t) {
    Color body = (Color){180,150,60,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int k=0;k<10;k++) {
        float ang=(2.0f*PI/10)*k;
        DrawCircleV((Vector2){cx+cosf(ang)*R*0.88f, cy+sinf(ang)*R*0.88f}, R*0.05f, (Color){110,90,30,220});
    }
    DrawCircleV((Vector2){cx,cy}, R*0.5f, (Color){60,48,15,230});
    DrawCircleLines((int)cx,(int)cy,(int)(R*0.5f),(Color){220,190,90,200});
    float ang=t*1.0f;
    DrawLineEx((Vector2){cx,cy}, (Vector2){cx+cosf(ang)*R*0.4f, cy+sinf(ang)*R*0.4f}, R*0.05f, (Color){230,200,100,255});
    DrawCircleV((Vector2){cx,cy}, R*0.06f, (Color){250,225,140,255});
}
static void Midas_V6(float cx, float cy, float R, float t) {
    Color body = (Color){70,40,20,255};
    DrawCircleV((Vector2){cx, cy+R*1.1f}, R*0.9f, (Color){255,150,20,40});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int k=0;k<6;k++) {
        float ang=(2.0f*PI/6)*k + t*0.6f;
        DrawCircleV((Vector2){cx+cosf(ang)*R*1.1f, cy+R*1.0f+sinf(ang)*R*0.4f}, R*0.045f, (Color){255,180,40,220});
    }
    DrawCircleLines((int)cx,(int)(cy+R*1.0f),(int)(R*1.1f),(Color){255,160,20,90});
    float p2=0.5f+0.5f*sinf(t*4.0f);
    DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.05f}, R*0.09f, (Color){255,180,(uchar)(30+60*p2),255});
    DrawCircleV((Vector2){cx+R*0.28f, cy-R*0.05f}, R*0.09f, (Color){255,180,(uchar)(30+60*p2),255});
}
static void Midas_V7(float cx, float cy, float R, float t) {
    Color body = (Color){235,200,80,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    float bob = sinf(t*1.5f)*R*0.06f;
    float cyC = cy-R*1.4f+bob;
    DrawRectangle((int)(cx-R*0.4f),(int)(cyC),(int)(R*0.8f),(int)(R*0.28f),(Color){255,215,60,255});
    for (int k=-1;k<=1;k++) {
        DrawTriangle((Vector2){cx+k*R*0.27f-R*0.13f,cyC},(Vector2){cx+k*R*0.27f+R*0.13f,cyC},(Vector2){cx+k*R*0.27f,cyC-R*0.24f}, (Color){255,215,60,255});
        DrawCircleV((Vector2){cx+k*R*0.27f,cyC-R*0.24f}, R*0.05f, (Color){220,40,60,255});
    }
    DrawLineEx((Vector2){cx-R*0.5f,cy+R*0.6f},(Vector2){cx+R*0.5f,cy-R*0.2f}, R*0.05f, (Color){200,160,40,240});
    DrawLineEx((Vector2){cx+R*0.5f,cy+R*0.6f},(Vector2){cx-R*0.5f,cy-R*0.2f}, R*0.05f, (Color){200,160,40,240});
    DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.02f}, R*0.06f, (Color){120,90,10,255});
    DrawCircleV((Vector2){cx+R*0.28f, cy-R*0.02f}, R*0.06f, (Color){120,90,10,255});
}
static void Midas_V8(float cx, float cy, float R, float t) {
    Color body = (Color){255,200,60,255};
    float wf = sinf(t*4.0f)*R*0.15f;
    Color fireO=(Color){255,140,20,150}, fireI=(Color){255,220,80,220};
    for (int side=-1; side<=1; side+=2) {
        DrawTriangle((Vector2){cx+side*R*0.5f,cy-R*0.1f},(Vector2){cx+side*R*1.6f,cy-R*0.7f-wf},(Vector2){cx+side*R*1.3f,cy+R*0.5f+wf}, fireO);
        DrawTriangle((Vector2){cx+side*R*0.5f,cy},(Vector2){cx+side*R*1.15f,cy-R*0.35f-wf*0.6f},(Vector2){cx+side*R*1.0f,cy+R*0.3f+wf*0.6f}, fireI);
    }
    for (int k=0;k<5;k++) {
        float ang = t*2.0f+k*1.3f;
        float rr = R*(1.3f+0.2f*sinf(t*3.0f+k));
        DrawCircleV((Vector2){cx+cosf(ang)*rr, cy-R*0.3f+sinf(ang)*rr*0.6f}, R*0.035f, (Color){255,200,80,200});
    }
    DrawCircle((int)cx,(int)cy,(int)R, body);
    EllipseRotF(cx-R*0.30f, cy-R*0.05f, R*0.13f, R*0.17f, 0.0f, (Color){255,240,190,255});
    EllipseRotF(cx+R*0.30f, cy-R*0.05f, R*0.13f, R*0.17f, 0.0f, (Color){255,240,190,255});
}

// ═══════════════════════════════════════════════════════════════════════════
// BLADE LEGEND — 8 variants (Epic, tied to Slayer + Kill Streak Tier 6)
// ═══════════════════════════════════════════════════════════════════════════
static Color BladeLegendColor(float gt) {
    float p = 0.5f + 0.5f*sinf(gt * 3.2f);
    return (Color){(uchar)(120+(int)(100*p)), (uchar)(20+(int)(15*p)), (uchar)(20+(int)(10*p)), 255};
}
static void BladeLegend_V1(float cx, float cy, float R, float t) {
    Color body = BladeLegendColor(t);
    DrawCircleV((Vector2){cx, cy}, R*1.30f, (Color){255,60,40,26});
    Color steel = (Color){70,70,80,220}, edge = (Color){210,210,220,240};
    for (int s = -1; s <= 1; s += 2) {
        float a = (PI/4.0f) * s;
        Vector2 dir = { cosf(a), sinf(a) };
        Vector2 tip  = { cx + dir.x*R*1.35f, cy + dir.y*R*1.35f };
        Vector2 hilt = { cx - dir.x*R*0.55f, cy - dir.y*R*0.55f };
        DrawLineEx(hilt, tip, R*0.20f, steel);
        DrawLineEx(hilt, tip, R*0.08f, edge);
    }
    DrawCircle((int)cx,(int)cy,(int)R, body);
    EllipseRotF(cx-R*0.30f, cy-R*0.02f, R*0.20f, R*0.13f, -0.25f, (Color){20,5,5,255});
    EllipseRotF(cx+R*0.30f, cy-R*0.02f, R*0.20f, R*0.13f,  0.25f, (Color){20,5,5,255});
    DrawCircleV((Vector2){cx-R*0.30f, cy-R*0.02f}, R*0.05f, (Color){255,120,90,255});
    DrawCircleV((Vector2){cx+R*0.30f, cy-R*0.02f}, R*0.05f, (Color){255,120,90,255});
    Color paint = (Color){230,40,30,230};
    DrawLineEx((Vector2){cx-R*0.55f,cy+R*0.28f},(Vector2){cx-R*0.20f,cy+R*0.42f}, R*0.08f, paint);
    DrawLineEx((Vector2){cx+R*0.20f,cy+R*0.42f},(Vector2){cx+R*0.55f,cy+R*0.28f}, R*0.08f, paint);
}
static void BladeLegend_V2(float cx, float cy, float R, float t) {
    Color body = (Color){100,25,25,255};
    float p = 0.5f+0.5f*sinf(t*4.0f);
    DrawCircleV((Vector2){cx, cy}, R*(1.30f+0.12f*p), (Color){255,20,20,30});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    Vector2 hilt = { cx+R*0.6f, cy+R*1.1f };
    Vector2 tip  = { cx-R*0.5f, cy-R*1.3f };
    DrawLineEx(hilt, tip, R*0.36f, (Color){90,90,100,255});
    DrawLineEx(hilt, tip, R*0.14f, (Color){220,220,230,255});
    DrawCircleV(hilt, R*0.14f, (Color){60,40,20,255});
    EllipseRotF(cx-R*0.30f, cy-R*0.02f, R*0.16f, R*0.12f, -0.2f, (Color){10,5,5,255});
    DrawCircleV((Vector2){cx-R*0.30f, cy-R*0.02f}, R*0.06f, (Color){255,60,40,(uchar)(220+30*p)});
    DrawRectanglePro((Rectangle){cx+R*0.30f,cy-R*0.02f,R*0.34f,R*0.20f}, (Vector2){R*0.17f,R*0.10f}, -12.0f, (Color){15,10,10,255});
}
static void BladeLegend_V3(float cx, float cy, float R, float t) {
    Color body = (Color){60,80,120,255};
    DrawCircleV((Vector2){cx, cy}, R*1.28f, (Color){120,200,255,22});
    Color steel = (Color){190,205,220,230};
    for (int s=-1; s<=1; s+=2) {
        float a = (PI/3.0f)*s + sinf(t*1.5f)*0.06f;
        Vector2 dir = { cosf(a), sinf(a) };
        Vector2 tip  = { cx+dir.x*R*1.4f, cy+dir.y*R*1.4f };
        Vector2 hilt = { cx-dir.x*R*0.4f, cy-dir.y*R*0.4f };
        DrawLineEx(hilt, tip, R*0.045f, steel);
        DrawCircleV(hilt, R*0.09f, (Color){40,40,55,255});
    }
    DrawCircle((int)cx,(int)cy,(int)R, body);
    DrawCircleV((Vector2){cx, cy-R*0.02f}, R*0.55f, (Color){20,22,30,210});
    for (int gx=-2; gx<=2; gx++)
        DrawLineEx((Vector2){cx+gx*R*0.14f, cy-R*0.45f},(Vector2){cx+gx*R*0.14f, cy+R*0.40f}, R*0.02f, (Color){150,170,190,140});
    for (int gy=-2; gy<=2; gy++)
        DrawLineEx((Vector2){cx-R*0.45f, cy+gy*R*0.14f},(Vector2){cx+R*0.45f, cy+gy*R*0.14f}, R*0.02f, (Color){150,170,190,140});
    DrawCircleV((Vector2){cx-R*0.26f, cy-R*0.02f}, R*0.045f, (Color){140,220,255,220});
    DrawCircleV((Vector2){cx+R*0.26f, cy-R*0.02f}, R*0.045f, (Color){140,220,255,220});
}
static void BladeLegend_V4(float cx, float cy, float R, float t) {
    Color body = (Color){40,40,45,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    Vector2 hilt={cx-R*0.7f,cy+R*0.6f}, tip={cx+R*1.2f,cy-R*0.9f};
    DrawLineEx(hilt,tip,R*0.10f,(Color){210,210,215,240});
    DrawLineEx(hilt,tip,R*0.04f,(Color){255,255,255,255});
    DrawCircleV(hilt, R*0.10f, (Color){120,20,20,255});
    for (int k=0;k<5;k++) {
        float dy = fmodf(t*1.0f + k*0.7f, 2.2f);
        float bx = cx-R*1.1f+k*R*0.5f + sinf(t+k)*R*0.15f;
        DrawCircleV((Vector2){bx, cy-R*1.2f+dy*R*1.0f}, R*0.045f, (Color){255,170,190,200});
    }
    EllipseRotF(cx-R*0.28f, cy-R*0.02f, R*0.15f, R*0.07f, 0.0f, (Color){15,10,10,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.02f, R*0.15f, R*0.07f, 0.0f, (Color){15,10,10,255});
}
static void BladeLegend_V5(float cx, float cy, float R, float t) {
    Color body = (Color){150,110,70,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int k=-2;k<=2;k++) {
        float ang = -PI/2.0f + k*0.28f;
        float len = R*(1.1f + 0.1f*sinf(t*2.0f+k));
        Vector2 tip={cx+cosf(ang)*len, cy+sinf(ang)*len - R*0.3f};
        Color fc = (k%2==0)?(Color){230,50,40,230}:(Color){250,240,220,230};
        DrawTriangle((Vector2){cx,cy-R*0.9f}, tip, (Vector2){cx+0.15f*R,cy-R*0.9f}, fc);
    }
    EllipseRotF(cx-R*0.30f, cy-R*0.02f, R*0.13f, R*0.16f, 0.0f, (Color){255,255,255,230});
    EllipseRotF(cx+R*0.30f, cy-R*0.02f, R*0.13f, R*0.16f, 0.0f, (Color){255,255,255,230});
    DrawCircleV((Vector2){cx-R*0.30f,cy-R*0.02f}, R*0.05f, (Color){20,15,10,255});
    DrawCircleV((Vector2){cx+R*0.30f,cy-R*0.02f}, R*0.05f, (Color){20,15,10,255});
    DrawLineEx((Vector2){cx-R*0.45f,cy+R*0.3f},(Vector2){cx+R*0.45f,cy+R*0.3f}, R*0.05f, (Color){230,50,40,220});
    DrawLineEx((Vector2){cx+R*0.9f,cy+R*1.1f},(Vector2){cx+R*0.9f,cy-R*1.1f}, R*0.045f, (Color){110,80,40,255});
    DrawTriangle((Vector2){cx+R*0.75f,cy-R*1.0f},(Vector2){cx+R*1.05f,cy-R*1.0f},(Vector2){cx+R*0.9f,cy-R*1.35f},(Color){200,200,210,255});
}
static void BladeLegend_V6(float cx, float cy, float R, float t) {
    Color body = (Color){25,22,28,255};
    float p = 0.5f+0.5f*sinf(t*1.5f);
    DrawCircleV((Vector2){cx, cy}, R*(1.2f+0.08f*p), (Color){150,10,10,26});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    EllipseRotF(cx, cy-R*0.15f, R*0.65f, R*0.55f, 0.0f, (Color){10,8,10,220});
    DrawCircleV((Vector2){cx-R*0.22f, cy-R*0.05f}, R*0.045f, (Color){220,40,30,(uchar)(200+40*p)});
    DrawCircleV((Vector2){cx+R*0.22f, cy-R*0.05f}, R*0.045f, (Color){220,40,30,(uchar)(200+40*p)});
    float ang = t*0.3f;
    Vector2 handleTop = {cx+cosf(ang)*R*0.1f, cy-R*1.5f};
    Vector2 handleBot = {cx, cy+R*0.5f};
    DrawLineEx(handleTop, handleBot, R*0.07f, (Color){70,50,30,255});
    DrawTriangle(handleTop, (Vector2){handleTop.x-R*0.45f,handleTop.y+R*0.35f}, (Vector2){handleTop.x-R*0.1f,handleTop.y+R*0.5f}, (Color){140,140,150,240});
    DrawTriangle(handleTop, (Vector2){handleTop.x+R*0.45f,handleTop.y+R*0.35f}, (Vector2){handleTop.x+R*0.1f,handleTop.y+R*0.5f}, (Color){140,140,150,240});
    for (int k=0;k<3;k++) DrawCircleLines((int)(cx-R*0.4f+k*R*0.15f),(int)(cy+R*0.6f),(int)(R*0.08f),(Color){120,120,130,200});
}
static void BladeLegend_V7(float cx, float cy, float R, float t) {
    Color body = (Color){40,60,90,255};
    DrawCircleV((Vector2){cx, cy}, R*1.3f, (Color){120,200,255,26});
    for (int e=3; e>=1; e--) {
        float ang=(PI/4.0f) + e*0.15f*sinf(t*2.0f);
        Vector2 tip={cx+cosf(ang)*R*1.3f, cy+sinf(ang)*R*1.3f};
        Vector2 hilt={cx-cosf(ang)*R*0.5f, cy-sinf(ang)*R*0.5f};
        DrawLineEx(hilt,tip,R*0.10f,(Color){140,210,255,(uchar)(70/e)});
    }
    DrawCircle((int)cx,(int)cy,(int)R, body);
    EllipseRotF(cx-R*0.28f, cy-R*0.02f, R*0.11f, R*0.15f, 0.0f, (Color){200,235,255,240});
    EllipseRotF(cx+R*0.28f, cy-R*0.02f, R*0.11f, R*0.15f, 0.0f, (Color){200,235,255,240});
    DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.02f}, R*0.04f, (Color){20,40,70,255});
    DrawCircleV((Vector2){cx+R*0.28f, cy-R*0.02f}, R*0.04f, (Color){20,40,70,255});
}
static void BladeLegend_V8(float cx, float cy, float R, float t) {
    Color body = (Color){190,155,90,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    DrawRectangle((int)(cx-R*0.6f),(int)(cy-R*0.55f),(int)(R*1.2f),(int)(R*0.3f), (Color){200,170,60,255});
    for (int k=0;k<7;k++) {
        float xk = cx-R*0.5f+k*R*0.17f;
        float bob = sinf(t*3.0f+k)*R*0.03f;
        DrawRectangle((int)xk,(int)(cy-R*1.0f+bob),(int)(R*0.10f),(int)(R*0.5f),(Color){210,40,30,230});
    }
    DrawLineEx((Vector2){cx-R*0.6f,cy+R*0.9f},(Vector2){cx+R*0.3f,cy+R*0.1f}, R*0.06f, (Color){225,225,230,240});
    DrawLineEx((Vector2){cx+R*0.6f,cy+R*0.9f},(Vector2){cx-R*0.3f,cy+R*0.1f}, R*0.06f, (Color){225,225,230,240});
    EllipseRotF(cx-R*0.28f, cy-R*0.02f, R*0.11f, R*0.14f, 0.0f, (Color){30,20,10,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.02f, R*0.11f, R*0.14f, 0.0f, (Color){30,20,10,255});
}

// ═══════════════════════════════════════════════════════════════════════════
// CONSTELLATION — 8 variants (Legendary, tied to Overachiever Tier 6)
// ═══════════════════════════════════════════════════════════════════════════
static Color ConstellationColor(float gt) {
    float p = 0.5f + 0.5f*sinf(gt * 1.0f);
    return (Color){(uchar)(20+(int)(20*p)), (uchar)(15+(int)(15*p)), (uchar)(60+(int)(40*p)), 255};
}
static void Constellation_V1(float cx, float cy, float R, float t) {
    Color body = ConstellationColor(t);
    DrawCircleV((Vector2){cx, cy}, R*1.45f, (Color){120,100,255,26});
    for (int k=0;k<7;k++) {
        float ang = t*0.5f + k*(PI*2.0f/7.0f);
        float rr  = R*(1.15f + 0.10f*sinf(t*1.3f + k));
        float sx = cx + cosf(ang)*rr, sy = cy + sinf(ang)*rr;
        float tw = 0.5f+0.5f*sinf(t*3.0f + k*1.7f);
        Color c  = (Color){220,220,255,(uchar)(120+120*tw)};
        float sr = R*(0.05f + 0.04f*tw);
        DrawLineEx((Vector2){sx-sr,sy},(Vector2){sx+sr,sy}, R*0.03f, c);
        DrawLineEx((Vector2){sx,sy-sr},(Vector2){sx,sy+sr}, R*0.03f, c);
    }
    DrawCircle((int)cx,(int)cy,(int)R, body);
    Color starCol = (Color){225,225,255,(uchar)(200+40*(0.5f+0.5f*sinf(t*2.0f)))};
    DrawCircleV((Vector2){cx-R*0.30f, cy-R*0.04f}, R*0.09f, starCol);
    DrawCircleV((Vector2){cx+R*0.30f, cy-R*0.04f}, R*0.09f, starCol);
    DrawCircleV((Vector2){cx, cy+R*0.30f}, R*0.06f, starCol);
    DrawLineEx((Vector2){cx-R*0.30f,cy-R*0.04f},(Vector2){cx,cy+R*0.30f}, R*0.025f, (Color){190,190,255,120});
    DrawLineEx((Vector2){cx+R*0.30f,cy-R*0.04f},(Vector2){cx,cy+R*0.30f}, R*0.025f, (Color){190,190,255,120});
}
static void Constellation_V2(float cx, float cy, float R, float t) {
    Color body = ConstellationColor(t*0.6f);
    DrawCircleV((Vector2){cx, cy}, R*1.5f, (Color){140,110,255,20});
    for (int k=0;k<12;k++) {
        float ang = t*0.4f + k*(2.0f*PI/12.0f);
        Vector2 p = { cx+cosf(ang)*R*1.30f, cy+sinf(ang)*R*1.30f };
        float tw = 0.5f+0.5f*sinf(t*2.0f+k);
        DrawCircleV(p, R*0.06f, (Color){225,215,255,(uchar)(140+100*tw)});
    }
    DrawCircleLines((int)cx,(int)cy,(int)(R*1.30f),(Color){160,140,255,50});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.05f}, R*0.08f, (Color){230,225,255,240});
    DrawCircleV((Vector2){cx+R*0.28f, cy-R*0.05f}, R*0.08f, (Color){230,225,255,240});
    DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.05f}, R*0.03f, (Color){80,60,160,255});
    DrawCircleV((Vector2){cx+R*0.28f, cy-R*0.05f}, R*0.03f, (Color){80,60,160,255});
}
static void Constellation_V3(float cx, float cy, float R, float t) {
    Color body = (Color){35,25,55,255};
    Color blobs[3] = { (Color){255,120,200,26}, (Color){120,160,255,26}, (Color){170,100,255,26} };
    for (int k=0;k<3;k++) {
        float ang = t*0.3f + k*2.1f;
        Vector2 c = { cx+cosf(ang)*R*0.5f, cy+sinf(ang)*R*0.4f };
        DrawCircleV(c, R*1.0f, blobs[k]);
    }
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int k=0;k<9;k++) {
        float ang = k*1.9f + t*0.5f;
        float rr  = R*0.15f + (k%4)*R*0.14f;
        Vector2 p = { cx+cosf(ang)*rr, cy+sinf(ang)*rr*0.7f };
        float tw = 0.5f+0.5f*sinf(t*4.0f+k*1.3f);
        DrawCircleV(p, R*0.025f, (Color){255,255,255,(uchar)(150*tw)});
    }
    DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.03f}, R*0.07f, (Color){255,235,255,220});
    DrawCircleV((Vector2){cx+R*0.28f, cy-R*0.03f}, R*0.07f, (Color){255,235,255,220});
}
static void Constellation_V4(float cx, float cy, float R, float t) {
    Color body = (Color){15,12,35,255};
    DrawCircleV((Vector2){cx, cy}, R*1.3f, (Color){120,90,220,20});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int arm=0; arm<2; arm++) {
        float baseAng = t*0.6f + arm*PI;
        for (int s=1; s<=14; s++) {
            float frac = s/14.0f;
            float ang = baseAng + frac*3.2f;
            float rr = R*0.85f*frac;
            Color c = ColorFromHSV(fmodf(220.0f+frac*80.0f,360.0f),0.5f,1.0f);
            DrawCircleV((Vector2){cx+cosf(ang)*rr, cy+sinf(ang)*rr}, R*0.035f*(1.0f-frac*0.4f), c);
        }
    }
    DrawCircleV((Vector2){cx,cy}, R*0.14f, (Color){255,250,230,255});
}
static void Constellation_V5(float cx, float cy, float R, float t) {
    Color body = ConstellationColor(t);
    DrawCircle((int)cx,(int)cy,(int)R, body);
    float frac = fmodf(t*0.6f, 1.0f);
    for (int k=0;k<8;k++) {
        float f2 = frac - k*0.03f; if (f2<0) f2+=1.0f;
        Vector2 p = { cx-R*1.4f+f2*R*2.8f, cy+R*0.9f-f2*R*1.8f };
        DrawCircleV(p, R*0.06f*(1.0f-k*0.10f), (Color){255,255,255,(uchar)(220-k*24)});
    }
    DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.04f}, R*0.07f, (Color){225,225,255,230});
    DrawCircleV((Vector2){cx+R*0.28f, cy-R*0.04f}, R*0.07f, (Color){225,225,255,230});
}
static void Constellation_V6(float cx, float cy, float R, float t) {
    Color body = (Color){20,20,45,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int k=-2;k<=2;k++) DrawLineEx((Vector2){cx-R*0.9f, cy+k*R*0.30f},(Vector2){cx+R*0.9f, cy+k*R*0.30f}, R*0.015f, (Color){140,150,220,90});
    for (int k=-2;k<=2;k++) DrawLineEx((Vector2){cx+k*R*0.30f, cy-R*0.9f},(Vector2){cx+k*R*0.30f, cy+R*0.9f}, R*0.015f, (Color){140,150,220,90});
    for (int k=0;k<6;k++) {
        float ang = k*1.3f;
        Vector2 p = { cx+cosf(ang)*R*0.6f, cy+sinf(ang)*R*0.5f };
        float tw = 0.5f+0.5f*sinf(t*3.0f+k);
        DrawCircleV(p, R*0.04f, (Color){255,255,255,(uchar)(150*tw)});
        DrawLineEx((Vector2){p.x-R*0.07f,p.y},(Vector2){p.x+R*0.07f,p.y}, R*0.012f, (Color){255,255,255,120});
        DrawLineEx((Vector2){p.x,p.y-R*0.07f},(Vector2){p.x,p.y+R*0.07f}, R*0.012f, (Color){255,255,255,120});
    }
    DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.02f}, R*0.06f, (Color){210,210,255,230});
    DrawCircleV((Vector2){cx+R*0.28f, cy-R*0.02f}, R*0.06f, (Color){210,210,255,230});
}
static void Constellation_V7(float cx, float cy, float R, float t) {
    Color body = (Color){40,20,60,255};
    DrawCircleV((Vector2){cx, cy-R*0.2f}, R*1.05f, (Color){160,90,220,30});
    DrawCircle((int)cx,(int)(cy-R*0.2f),(int)(R*0.85f), body);
    for (int k=0;k<5;k++) {
        float baseX = cx - R*0.6f + k*R*0.3f;
        Vector2 prev = { baseX, cy+R*0.3f };
        for (int s=1;s<=8;s++) {
            float f=s/8.0f;
            float wob = sinf(t*2.0f+k+f*4.0f)*R*0.12f;
            Vector2 p = { baseX+wob, cy+R*0.3f+f*R*1.1f };
            Color c = ColorFromHSV(fmodf(280.0f+k*15.0f,360.0f),0.55f,1.0f);
            DrawLineEx(prev,p,R*0.035f*(1.0f-f*0.6f),(Color){c.r,c.g,c.b,(uchar)(180*(1.0f-f))});
            prev=p;
        }
    }
    DrawCircleV((Vector2){cx-R*0.24f, cy-R*0.22f}, R*0.06f, (Color){230,220,255,230});
    DrawCircleV((Vector2){cx+R*0.24f, cy-R*0.22f}, R*0.06f, (Color){230,220,255,230});
}
static void Constellation_V8(float cx, float cy, float R, float t) {
    DrawCircleV((Vector2){cx, cy}, R*1.4f, (Color){255,140,60,16});
    DrawRing((Vector2){cx,cy}, R*1.05f, R*1.30f, 0.0f, 360.0f, 48, (Color){255,150,60,140});
    DrawRing((Vector2){cx,cy}, R*1.10f, R*1.20f, 0.0f, 360.0f, 48, (Color){255,220,140,180});
    for (int k=0;k<3;k++) {
        float a0 = t*0.5f + k*2.1f;
        HaloArc(cx, cy, R*1.15f, R*0.5f, a0, a0+1.2f, R*0.03f, (Color){200,140,255,120});
    }
    DrawCircle((int)cx,(int)cy,(int)(R*0.85f), (Color){5,3,10,255});
    DrawCircleV((Vector2){cx,cy}, R*0.85f*0.3f, (Color){0,0,0,255});
}

// ═══════════════════════════════════════════════════════════════════════════
// ASCENDANT — 8 variants (Legendary, tied to Perfectionist Tier 6)
// ═══════════════════════════════════════════════════════════════════════════
static Color AscendantColor(float gt) {
    float p = 0.5f + 0.5f*sinf(gt * 1.6f);
    return (Color){255, (uchar)(245+(int)(10*p)), (uchar)(210+(int)(45*p)), 255};
}
static void Ascendant_V1(float cx, float cy, float R, float t) {
    Color body = AscendantColor(t);
    DrawCircleV((Vector2){cx, cy}, R*1.55f, (Color){255,240,190,26});
    DrawCircleV((Vector2){cx, cy}, R*1.30f, (Color){255,225,140,30});
    const int SLICES = 10;
    for (int s = 0; s < SLICES; s++) {
        float a0 = PI + (PI * s)     / SLICES;
        float a1 = PI + (PI * (s+1)) / SLICES;
        Color rc = ColorFromHSV(fmodf(t*50.0f + s*36.0f, 360.0f), 0.7f, 1.0f);
        HaloArc(cx, cy-R*0.95f, R*1.30f, R*0.46f, a0, a1, R*0.16f, (Color){rc.r,rc.g,rc.b,150});
    }
    DrawCircle((int)cx,(int)cy,(int)R, body);
    DrawCircleV((Vector2){cx-R*0.32f, cy-R*0.05f}, R*0.15f, (Color){255,250,220,255});
    DrawCircleV((Vector2){cx+R*0.32f, cy-R*0.05f}, R*0.15f, (Color){255,250,220,255});
    DrawCircleV((Vector2){cx-R*0.32f, cy-R*0.05f}, R*0.07f, (Color){255,215,90,255});
    DrawCircleV((Vector2){cx+R*0.32f, cy-R*0.05f}, R*0.07f, (Color){255,215,90,255});
    for (int s = 0; s < SLICES; s++) {
        float a0 = (PI * s)     / SLICES;
        float a1 = (PI * (s+1)) / SLICES;
        Color rc = ColorFromHSV(fmodf(t*50.0f + s*36.0f + 180.0f, 360.0f), 0.7f, 1.0f);
        HaloArc(cx, cy-R*0.95f, R*1.30f, R*0.46f, a0, a1, R*0.10f, (Color){rc.r,rc.g,rc.b,220});
    }
}
static void Ascendant_V2(float cx, float cy, float R, float t) {
    Color body = (Color){255,250,230,255};
    DrawCircleV((Vector2){cx, cy}, R*1.6f, (Color){255,220,120,20});
    int spikes = 16;
    for (int k=0;k<spikes;k++) {
        float ang = (2.0f*PI/spikes)*k + t*0.3f;
        float len = R*(1.1f + 0.30f*(0.5f+0.5f*sinf(t*4.0f+k*0.9f)));
        Vector2 tip = { cx+cosf(ang)*len, cy+sinf(ang)*len };
        Vector2 b1  = { cx+cosf(ang-0.05f)*R*0.9f, cy+sinf(ang-0.05f)*R*0.9f };
        Vector2 b2  = { cx+cosf(ang+0.05f)*R*0.9f, cy+sinf(ang+0.05f)*R*0.9f };
        DrawTriangle(b1, tip, b2, (Color){255,225,120,180});
    }
    DrawCircle((int)cx,(int)cy,(int)R, body);
    DrawCircleV((Vector2){cx-R*0.30f, cy-R*0.04f}, R*0.10f, (Color){255,190,40,255});
    DrawCircleV((Vector2){cx+R*0.30f, cy-R*0.04f}, R*0.10f, (Color){255,190,40,255});
}
static void Ascendant_V3(float cx, float cy, float R, float t) {
    Color body = (Color){255,248,225,255};
    float wf = sinf(t * 2.0f) * (R * 0.20f);
    Color wo = (Color){255,245,210, 80}, wi = (Color){255,250,235,200};
    for (int side=-1; side<=1; side+=2) {
        DrawTriangle((Vector2){cx+side*R*0.5f,cy-R*0.2f},(Vector2){cx+side*R*1.7f,cy-R*0.6f-wf},(Vector2){cx+side*R*1.5f,cy+R*0.6f+wf}, wo);
        DrawTriangle((Vector2){cx+side*R*0.5f,cy-R*0.1f},(Vector2){cx+side*R*1.3f,cy-R*0.3f-wf},(Vector2){cx+side*R*1.15f,cy+R*0.4f+wf}, wi);
    }
    Color rc = ColorFromHSV(fmodf(t*30.0f, 360.0f), 0.35f, 1.0f);
    DrawCircleLines((int)cx,(int)(cy-R*0.95f),(int)(R*0.55f),(Color){rc.r,rc.g,rc.b,200});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    DrawCircleV((Vector2){cx-R*0.30f, cy-R*0.04f}, R*0.09f, (Color){255,215,90,255});
    DrawCircleV((Vector2){cx+R*0.30f, cy-R*0.04f}, R*0.09f, (Color){255,215,90,255});
}
static void Ascendant_V4(float cx, float cy, float R, float t) {
    Color body = (Color){255,250,235,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    float bob = sinf(t*1.4f)*R*0.05f;
    for (int k=0;k<5;k++) {
        float ang = -PI/2.0f + (k-2)*0.32f;
        float rr = R*1.35f;
        Vector2 p = { cx+cosf(ang)*rr, cy+sinf(ang)*rr-R*0.3f+bob };
        float tw = 0.5f+0.5f*sinf(t*3.0f+k);
        DrawCircleV(p, R*0.10f, (Color){255,240,180,(uchar)(200+40*tw)});
        DrawCircleV(p, R*0.05f, (Color){255,255,255,255});
    }
    DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.03f}, R*0.08f, (Color){255,215,90,255});
    DrawCircleV((Vector2){cx+R*0.28f, cy-R*0.03f}, R*0.08f, (Color){255,215,90,255});
}
static void Ascendant_V5(float cx, float cy, float R, float t) {
    DrawCircleV((Vector2){cx, cy}, R*1.3f, (Color){255,255,255,20});
    int facets=10;
    for (int k=0;k<facets;k++) {
        float a0=(2.0f*PI/facets)*k + t*0.2f;
        float a1=(2.0f*PI/facets)*(k+1) + t*0.2f;
        Color c = ColorFromHSV(fmodf(k*36.0f+t*40.0f,360.0f), 0.35f, 1.0f);
        DrawTriangle((Vector2){cx,cy}, (Vector2){cx+cosf(a1)*R,cy+sinf(a1)*R}, (Vector2){cx+cosf(a0)*R,cy+sinf(a0)*R}, c);
    }
    DrawCircleLines((int)cx,(int)cy,(int)R,(Color){255,255,255,200});
    DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.03f}, R*0.08f, (Color){255,255,255,240});
    DrawCircleV((Vector2){cx+R*0.28f, cy-R*0.03f}, R*0.08f, (Color){255,255,255,240});
}
static void Ascendant_V6(float cx, float cy, float R, float t) {
    Color body = (Color){255,248,225,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int k=0;k<8;k++) {
        float dy = fmodf(t*1.2f + k*0.4f, 2.0f);
        float bx = cx + (k%3-1)*R*0.35f + sinf(t*1.5f+k)*R*0.1f;
        float alpha = 220.0f*(1.0f-dy/2.0f);
        DrawCircleV((Vector2){bx, cy-R*0.5f-dy*R*0.8f}, R*0.05f*(1.0f-dy*0.3f), (Color){255,230,150,(uchar)(alpha<0?0:alpha)});
    }
    DrawCircleV((Vector2){cx, cy+R*0.8f}, R*0.5f, (Color){255,210,100,60});
    EllipseRotF(cx-R*0.28f, cy-R*0.04f, R*0.13f, R*0.17f, 0.0f, (Color){255,230,150,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.04f, R*0.13f, R*0.17f, 0.0f, (Color){255,230,150,255});
}
static void Ascendant_V7(float cx, float cy, float R, float t) {
    Color body = (Color){250,245,255,255};
    DrawCircleV((Vector2){cx, cy}, R*1.35f, (Color){255,235,255,18});
    for (int k=0;k<6;k++) {
        float ang=(2.0f*PI/6)*k + t*0.4f;
        Vector2 p={cx+cosf(ang)*R*0.85f, cy+sinf(ang)*R*0.85f};
        DrawCircleLines((int)p.x,(int)p.y,(int)(R*0.55f),(Color){220,180,255,120});
    }
    DrawCircleLines((int)cx,(int)cy,(int)(R*0.55f),(Color){220,180,255,150});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    DrawCircleV((Vector2){cx-R*0.26f, cy-R*0.03f}, R*0.07f, (Color){215,180,255,240});
    DrawCircleV((Vector2){cx+R*0.26f, cy-R*0.03f}, R*0.07f, (Color){215,180,255,240});
}
static void Ascendant_V8(float cx, float cy, float R, float t) {
    Color body = (Color){255,250,235,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int k=0;k<2;k++) {
        float ang = t*1.3f + k*PI;
        Vector2 p = { cx+cosf(ang)*R*1.25f, cy+sinf(ang)*R*0.5f };
        Color c = k==0? (Color){255,210,90,255} : (Color){255,150,90,255};
        DrawCircleV(p, R*0.30f, (Color){c.r,c.g,c.b,50});
        DrawCircleV(p, R*0.18f, c);
        DrawCircleV(p, R*0.09f, (Color){255,250,220,255});
    }
    DrawCircleV((Vector2){cx-R*0.26f, cy-R*0.03f}, R*0.07f, (Color){255,215,90,255});
    DrawCircleV((Vector2){cx+R*0.26f, cy-R*0.03f}, R*0.07f, (Color){255,215,90,255});
}

typedef void (*DrawFn)(float cx, float cy, float R, float t);

static DrawFn fns[NSKIN][NVAR] = {
    { Chronovore_V1,    Chronovore_V2,    Chronovore_V3,    Chronovore_V4,
      Chronovore_V5,    Chronovore_V6,    Chronovore_V7,    Chronovore_V8    },
    { Cartographer_V1,  Cartographer_V2,  Cartographer_V3,  Cartographer_V4,
      Cartographer_V5,  Cartographer_V6,  Cartographer_V7,  Cartographer_V8  },
    { Midas_V1,         Midas_V2,         Midas_V3,         Midas_V4,
      Midas_V5,         Midas_V6,         Midas_V7,         Midas_V8         },
    { BladeLegend_V1,   BladeLegend_V2,   BladeLegend_V3,   BladeLegend_V4,
      BladeLegend_V5,   BladeLegend_V6,   BladeLegend_V7,   BladeLegend_V8   },
    { Constellation_V1, Constellation_V2, Constellation_V3, Constellation_V4,
      Constellation_V5, Constellation_V6, Constellation_V7, Constellation_V8 },
    { Ascendant_V1,     Ascendant_V2,     Ascendant_V3,     Ascendant_V4,
      Ascendant_V5,     Ascendant_V6,     Ascendant_V7,     Ascendant_V8     },
};

// ═══════════════════════════════════════════════════════════════════════════
// DATA-DRIVEN METADATA — loaded from skin_data.txt / bad_skins.txt
// ═══════════════════════════════════════════════════════════════════════════
typedef struct { char name[40]; char hint[80]; } SkinMeta;
typedef struct { char label[8]; char desc[120]; } VariantMeta;

static SkinMeta    skinMeta[NSKIN];
static VariantMeta varMeta[NSKIN][NVAR];
static bool        bad[NSKIN][NVAR];

static void TrimNewline(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = '\0';
}

static void LoadSkinData(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        TraceLog(LOG_WARNING, "skin_data.txt not found next to the exe — names will be blank");
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        TrimNewline(line);
        if (line[0] == '#' || line[0] == '\0') continue;

        if (strncmp(line, "SKIN|", 5) == 0) {
            int idx; char name[40]="", hint[80]="";
            // SKIN|<idx>|<name>|<hint>
            char *p = line + 5;
            char *tok = strtok(p, "|"); if (!tok) continue; idx = atoi(tok);
            tok = strtok(NULL, "|"); if (tok) strncpy(name, tok, sizeof(name)-1);
            tok = strtok(NULL, "|"); if (tok) strncpy(hint, tok, sizeof(hint)-1);
            if (idx >= 0 && idx < NSKIN) {
                strncpy(skinMeta[idx].name, name, sizeof(skinMeta[idx].name)-1);
                strncpy(skinMeta[idx].hint, hint, sizeof(skinMeta[idx].hint)-1);
            }
        } else if (strncmp(line, "VARIANT|", 8) == 0) {
            int si, vi; char label[8]="", desc[120]="";
            char *p = line + 8;
            char *tok = strtok(p, "|"); if (!tok) continue; si = atoi(tok);
            tok = strtok(NULL, "|"); if (!tok) continue; vi = atoi(tok);
            tok = strtok(NULL, "|"); if (tok) strncpy(label, tok, sizeof(label)-1);
            tok = strtok(NULL, "|"); if (tok) strncpy(desc, tok, sizeof(desc)-1);
            if (si >= 0 && si < NSKIN && vi >= 0 && vi < NVAR) {
                strncpy(varMeta[si][vi].label, label, sizeof(varMeta[si][vi].label)-1);
                strncpy(varMeta[si][vi].desc,  desc,  sizeof(varMeta[si][vi].desc)-1);
            }
        }
    }
    fclose(f);
}

static void LoadBadSkins(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return; // no rejections yet — that's fine
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        int si, vi;
        if (sscanf(line, "%d %d", &si, &vi) == 2)
            if (si >= 0 && si < NSKIN && vi >= 0 && vi < NVAR) bad[si][vi] = true;
    }
    fclose(f);
}

// Rewrites the whole file from the current in-memory bad[] state every time
// a selection changes — simple and always consistent with what's on screen.
static void SaveBadSkins(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "# bad_skins.txt — rejected skin-idea picks (auto-generated by skinideas.exe)\n");
    fprintf(f, "# skin_index variant_index  skin_name  variant_label\n");
    for (int i = 0; i < NSKIN; i++)
        for (int j = 0; j < NVAR; j++)
            if (bad[i][j])
                fprintf(f, "%d %d  %s %s\n", i, j, skinMeta[i].name, varMeta[i][j].label);
    fclose(f);
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN — scrollable grid; click a preview to reject it, click a rejected
// chip at the bottom to restore it.
// ═══════════════════════════════════════════════════════════════════════════
int main(void) {
    LoadSkinData("skin_data.txt");
    LoadBadSkins("bad_skins.txt");

    const int sw = 1500, sh = 860;
    InitWindow(sw, sh, "Skin Design Picker — click a preview to reject it");
    SetTargetFPS(60);

    int leftMargin = 40;
    int gridLeft   = 210;
    int gridW      = sw - gridLeft - 30;
    int cols       = 4;
    int colW       = gridW / cols;
    int bandH      = 250;
    int headerH    = 56;
    float R        = 30.0f;

    int scroll = 0;
    char hoverDesc[128] = "";
    Vector2 hoverPos = {0,0};

    while (!WindowShouldClose()) {
        float t = (float)GetTime();
        Vector2 mp = GetMousePosition();
        bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        hoverDesc[0] = '\0';

        // ── count rejected + compute dynamic content height ────────────────
        int rejCount = 0;
        for (int i=0;i<NSKIN;i++) for (int j=0;j<NVAR;j++) if (bad[i][j]) rejCount++;
        int rejCols = 8;
        int rejRows = (rejCount + rejCols - 1) / rejCols; if (rejRows < 1) rejRows = 1;
        int rejBandH = 70 + rejRows*70 + 20;
        int totalH = 6*bandH + rejBandH + headerH + 20;
        int maxScroll = totalH - sh;
        if (maxScroll < 0) maxScroll = 0;

        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) scroll -= (int)(wheel * 50.0f);
        if (IsKeyDown(KEY_DOWN)) scroll += 8;
        if (IsKeyDown(KEY_UP))   scroll -= 8;
        if (scroll < 0) scroll = 0;
        if (scroll > maxScroll) scroll = maxScroll;

        BeginDrawing();
        ClearBackground((Color){14,12,20,255});

        DrawText("SKIN DESIGN PICKER -- click a preview to REJECT it (logged to bad_skins.txt); click a rejected chip below to restore it",
                  leftMargin, 10, 15, (Color){230,225,240,255});
        DrawText(TextFormat("%d / %d rejected", rejCount, NSKIN*NVAR), leftMargin, 30, 13, (Color){170,120,120,220});

        BeginScissorMode(0, headerH, sw, sh-headerH);

        // ── 6 skin bands, bad variants removed + remaining reflowed ────────
        for (int row = 0; row < NSKIN; row++) {
            int bandTop = headerH + row*bandH - scroll;
            if (bandTop + bandH < headerH || bandTop > sh) continue;

            DrawText(TextFormat("%s  (%s)", skinMeta[row].name, skinMeta[row].hint),
                      leftMargin, bandTop + 14, 16, (Color){205,195,225,225});
            DrawLine(0, bandTop+bandH-4, sw, bandTop+bandH-4, (Color){40,36,55,255});

            int visList[NVAR], visCount = 0;
            for (int v = 0; v < NVAR; v++) if (!bad[row][v]) visList[visCount++] = v;

            for (int pos = 0; pos < visCount; pos++) {
                int variant = visList[pos];
                int col = pos % cols, sub = pos / cols;
                int cx = gridLeft + col*colW + colW/2;
                int ry = bandTop + 66 + sub*100;

                Rectangle hitBox = { (float)(cx - R*1.6f), (float)(ry - R*1.6f), R*3.2f, R*3.2f };
                bool hov = CheckCollisionPointRec(mp, hitBox) && bandTop >= headerH-bandH;

                DrawCircleV((Vector2){(float)cx,(float)ry}, R*1.6f, hov ? (Color){255,80,80,20} : (Color){255,255,255,7});
                fns[row][variant](cx, ry, R, t);

                const char *lbl = varMeta[row][variant].label;
                int tw = MeasureText(lbl, 12);
                DrawText(lbl, cx - tw/2, ry + (int)(R*1.75f), 12, (Color){160,155,180,200});

                if (hov) {
                    DrawCircleLines(cx, ry, (int)(R*1.6f), (Color){255,90,90,220});
                    strncpy(hoverDesc, varMeta[row][variant].desc, sizeof(hoverDesc)-1);
                    hoverPos = (Vector2){(float)cx, (float)(ry - R*1.9f)};
                    if (clicked) { bad[row][variant] = true; SaveBadSkins("bad_skins.txt"); }
                }
            }
        }

        // ── rejected section — click a chip to restore ─────────────────────
        {
            int bandTop = headerH + 6*bandH - scroll;
            if (!(bandTop + rejBandH < headerH || bandTop > sh)) {
                DrawText("REJECTED  (click to restore)", leftMargin, bandTop + 14, 16, (Color){225,140,140,225});
                DrawLine(0, bandTop+rejBandH-2, sw, bandTop+rejBandH-2, (Color){40,36,55,255});

                if (rejCount == 0) {
                    DrawText("(none yet)", leftMargin, bandTop + 46, 14, (Color){110,105,130,200});
                } else {
                    int idx = 0;
                    for (int i = 0; i < NSKIN; i++) {
                        for (int j = 0; j < NVAR; j++) {
                            if (!bad[i][j]) continue;
                            int rc = idx % rejCols, rr = idx / rejCols;
                            int cx = gridLeft + rc*colW + colW/2;
                            int ry = bandTop + 60 + rr*70;
                            float rR = 20.0f;

                            Rectangle hitBox = { (float)(cx - rR*1.6f), (float)(ry - rR*1.6f), rR*3.2f, rR*3.2f };
                            bool hov = CheckCollisionPointRec(mp, hitBox);

                            DrawCircleV((Vector2){(float)cx,(float)ry}, rR*1.6f, hov ? (Color){120,255,120,25} : (Color){255,255,255,6});
                            fns[i][j](cx, ry, rR, t);
                            DrawLineEx((Vector2){cx-rR*1.1f,ry-rR*1.1f},(Vector2){cx+rR*1.1f,ry+rR*1.1f}, 3.0f, (Color){255,60,60,200});

                            char nm[48]; snprintf(nm, sizeof(nm), "%.10s %s", skinMeta[i].name, varMeta[i][j].label);
                            int tw = MeasureText(nm, 11);
                            DrawText(nm, cx - tw/2, ry + (int)(rR*1.7f), 11, (Color){170,165,190,200});

                            if (hov) {
                                DrawCircleLines(cx, ry, (int)(rR*1.6f), (Color){120,255,120,220});
                                strncpy(hoverDesc, varMeta[i][j].desc, sizeof(hoverDesc)-1);
                                hoverPos = (Vector2){(float)cx, (float)(ry - rR*1.9f)};
                                if (clicked) { bad[i][j] = false; SaveBadSkins("bad_skins.txt"); }
                            }
                            idx++;
                        }
                    }
                }
            }
        }

        EndScissorMode();

        if (maxScroll > 0) {
            int sbX = sw-10, sbY = headerH, sbH = sh-headerH;
            int sbTH = (int)((float)sbH / totalH * sbH);
            if (sbTH < 20) sbTH = 20;
            int sbTY = sbY + (int)((float)scroll / maxScroll * (sbH - sbTH));
            DrawRectangle(sbX, sbY, 4, sbH, (Color){255,255,255,25});
            DrawRectangle(sbX, sbTY, 4, sbTH, (Color){255,255,255,110});
        }

        if (hoverDesc[0] != '\0') {
            int tw = MeasureText(hoverDesc, 13);
            int bx = (int)hoverPos.x - tw/2 - 8, by = (int)hoverPos.y - 22;
            if (bx < 4) bx = 4;
            if (bx + tw + 16 > sw) bx = sw - tw - 16;
            DrawRectangle(bx, by, tw+16, 22, (Color){10,8,18,235});
            DrawRectangleLines(bx, by, tw+16, 22, (Color){255,255,255,60});
            DrawText(hoverDesc, bx+8, by+4, 13, (Color){235,230,245,255});
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
