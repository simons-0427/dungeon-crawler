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
#define NVAR  16   // max variants per skin; unused slots stay NULL in fns[]

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

static void EllipseLinesRot(float cx, float cy, float rx, float ry,
                            float ang, float thick, Color col) {
    const int SEG = 32;
    Vector2 prev = {0};
    float ca = cosf(ang), sa = sinf(ang);
    for (int i = 0; i <= SEG; i++) {
        float th = (float)i / SEG * 2.0f * PI;
        float ex = cosf(th)*rx, ey = sinf(th)*ry;
        Vector2 p = { cx + ex*ca - ey*sa, cy + ex*sa + ey*ca };
        if (i > 0) DrawLineEx(prev, p, thick, col);
        prev = p;
    }
}

// deterministic 0..1 hash — stable per-particle offsets without rand()
static float Rnd(int i) {
    float x = sinf(i*127.1f + 311.7f) * 43758.5453f;
    return x - floorf(x);
}

// 4-point star sparkle
static void Star4(float cx, float cy, float r, Color c) {
    DrawLineEx((Vector2){cx-r,cy},(Vector2){cx+r,cy}, r*0.35f, c);
    DrawLineEx((Vector2){cx,cy-r},(Vector2){cx,cy+r}, r*0.35f, c);
    DrawCircleV((Vector2){cx,cy}, r*0.30f, c);
}

// katana drawn hilt->tip, ang in radians (0 = straight up), used behind bodies
static void KatanaBehind(float R, float bx, float by, float ang, float len) {
    Vector2 dir = {  sinf(ang), -cosf(ang) };
    Vector2 per = {  cosf(ang),  sinf(ang) };
    Vector2 tip = { bx + dir.x*len, by + dir.y*len };
    DrawLineEx((Vector2){bx - dir.x*R*0.35f, by - dir.y*R*0.35f}, (Vector2){bx,by}, R*0.11f, (Color){58,47,34,255});
    DrawLineEx((Vector2){bx - per.x*R*0.16f, by - per.y*R*0.16f},
               (Vector2){bx + per.x*R*0.16f, by + per.y*R*0.16f}, R*0.09f, (Color){200,164,74,255});
    DrawLineEx((Vector2){bx,by}, tip, R*0.14f, (Color){150,150,165,235});
    DrawLineEx((Vector2){bx + per.x*R*0.03f, by + per.y*R*0.03f},
               (Vector2){tip.x + per.x*R*0.02f, tip.y + per.y*R*0.02f}, R*0.05f, (Color){235,235,245,255});
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
    Color body = (Color){90,60,30,255};
    DrawCircleV((Vector2){cx, cy}, R*1.25f, (Color){200,150,60,20});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    DrawTriangle((Vector2){cx-R*0.55f,cy+R*0.75f},(Vector2){cx+R*0.55f,cy+R*0.75f},(Vector2){cx,cy+R*0.05f}, (Color){150,110,45,255});
    for (int k=-3;k<=3;k++) {
        float ang = -PI/2.0f + k*0.22f;
        DrawLineEx((Vector2){cx+cosf(ang)*R*0.62f, cy+R*0.05f+sinf(ang)*R*0.62f},
                   (Vector2){cx+cosf(ang)*R*0.72f, cy+R*0.05f+sinf(ang)*R*0.72f}, R*0.03f, (Color){230,200,140,200});
    }
    float swing = sinf(t*2.2f)*0.6f;
    Vector2 tip = { cx+sinf(swing)*R*0.55f, cy-R*0.55f };
    DrawLineEx((Vector2){cx,cy+R*0.05f}, tip, R*0.045f, (Color){40,30,15,255});
    DrawCircleV(tip, R*0.10f, (Color){215,180,90,255});
    EllipseRotF(cx-R*0.28f, cy-R*0.05f, R*0.09f, R*0.13f, 0.0f, (Color){40,25,10,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.05f, R*0.09f, R*0.13f, 0.0f, (Color){40,25,10,255});
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
    Color body = (Color){225,215,195,255};
    DrawCircleV((Vector2){cx, cy}, R*1.2f, (Color){200,170,120,18});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int k=-2;k<=2;k++) DrawCircleLines((int)(cx+k*R*0.28f), (int)(cy-R*0.95f), (int)(R*0.06f), (Color){120,100,70,220});
    float peel = 0.5f+0.5f*sinf(t*1.5f);
    DrawTriangle((Vector2){cx+R*0.55f,cy-R*0.55f},(Vector2){cx+R*0.55f-peel*R*0.35f,cy-R*0.55f},(Vector2){cx+R*0.55f,cy-R*0.55f+peel*R*0.35f}, (Color){245,240,225,255});
    DrawText("31", (int)(cx-R*0.22f), (int)(cy-R*0.15f), (int)(R*0.5f), (Color){90,70,40,220});
    DrawLine((int)(cx-R*0.5f),(int)(cy+R*0.35f),(int)(cx+R*0.5f),(int)(cy+R*0.35f),(Color){120,100,70,180});
    EllipseRotF(cx-R*0.28f, cy-R*0.02f, R*0.09f, R*0.12f, 0.0f, (Color){70,55,35,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.02f, R*0.09f, R*0.12f, 0.0f, (Color){70,55,35,255});
}
static void Chronovore_V6(float cx, float cy, float R, float t) {
    Color body = (Color){190,170,120,255};
    DrawCircleV((Vector2){cx, cy}, R*1.2f, (Color){230,200,120,20});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    DrawCircleLines((int)cx,(int)cy,(int)(R*0.75f),(Color){120,95,55,160});
    DrawCircleLines((int)cx,(int)cy,(int)(R*0.50f),(Color){120,95,55,160});
    float ang = t*0.5f;
    Vector2 shadowTip = { cx+cosf(ang)*R*0.7f, cy+sinf(ang)*R*0.35f };
    DrawTriangle((Vector2){cx,cy},(Vector2){cx+R*0.06f,cy-R*0.05f}, shadowTip, (Color){50,38,20,180});
    DrawLineEx((Vector2){cx,cy},(Vector2){cx,cy-R*0.55f}, R*0.06f, (Color){70,55,30,255});
    EllipseRotF(cx-R*0.28f, cy-R*0.02f, R*0.09f, R*0.12f, 0.0f, (Color){60,45,25,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.02f, R*0.09f, R*0.12f, 0.0f, (Color){60,45,25,255});
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
    Color body = (Color){70,55,40,255};
    DrawCircleV((Vector2){cx, cy}, R*1.2f, (Color){210,180,120,18});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    HaloArc(cx, cy+R*0.15f, R*0.85f, R*0.85f, PI*1.05f, PI*1.75f, R*0.06f, (Color){200,160,80,220});
    for (int k=0;k<=6;k++) {
        float ang = PI*1.05f + (PI*0.7f)*(k/6.0f);
        DrawLineEx((Vector2){cx+cosf(ang)*R*0.78f, cy+R*0.15f+sinf(ang)*R*0.78f},
                   (Vector2){cx+cosf(ang)*R*0.92f, cy+R*0.15f+sinf(ang)*R*0.92f}, R*0.02f, (Color){230,200,140,200});
    }
    float ang2 = PI*1.4f + sinf(t*0.8f)*0.15f;
    DrawLineEx((Vector2){cx,cy+R*0.15f},(Vector2){cx+cosf(ang2)*R*0.9f, cy+R*0.15f+sinf(ang2)*R*0.9f}, R*0.035f, (Color){235,215,160,240});
    DrawCircleV((Vector2){cx+cosf(ang2)*R*0.9f, cy+R*0.15f+sinf(ang2)*R*0.9f}, R*0.05f, (Color){255,240,190,255});
    EllipseRotF(cx-R*0.26f, cy-R*0.15f, R*0.09f, R*0.12f, 0.0f, (Color){225,200,150,240});
    EllipseRotF(cx+R*0.26f, cy-R*0.15f, R*0.09f, R*0.12f, 0.0f, (Color){225,200,150,240});
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
    Color body = (Color){40,80,95,255};
    DrawCircleV((Vector2){cx, cy}, R*1.2f, (Color){80,190,200,18});
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int k=0;k<4;k++) {
        float yy = cy-R*0.5f+k*R*0.33f;
        Vector2 prev = { cx-R*0.9f, yy };
        for (int s=1;s<=12;s++) {
            float f = (float)s/12.0f;
            float wob = sinf(f*12.0f + t*2.0f + k)*R*0.05f;
            Vector2 p = { cx-R*0.9f+f*R*1.8f, yy+wob };
            DrawLineEx(prev,p, R*0.02f, (Color){160,225,230,160});
            prev = p;
        }
    }
    DrawCircleV((Vector2){cx, cy+R*0.65f}, R*0.14f, (Color){230,235,240,230});
    float phase = 0.5f+0.5f*sinf(t*0.6f);
    DrawCircleV((Vector2){cx+R*0.14f*(1.0f-2.0f*phase), cy+R*0.65f}, R*0.14f, (Color){40,80,95,255});
    EllipseRotF(cx-R*0.26f, cy-R*0.10f, R*0.09f, R*0.12f, 0.0f, (Color){225,240,240,230});
    EllipseRotF(cx+R*0.26f, cy-R*0.10f, R*0.09f, R*0.12f, 0.0f, (Color){225,240,240,230});
}
static void Cartographer_V7(float cx, float cy, float R, float t) {
    Color body = (Color){95,65,40,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int k=-2;k<=2;k++)
        DrawLineEx((Vector2){cx+k*R*0.28f, cy-R*0.9f},(Vector2){cx+k*R*0.28f+sinf(t*0.3f+k)*R*0.05f, cy+R*0.9f}, R*0.015f, (Color){70,45,25,140});
    Color blaze = (Color){225,215,190,235};
    DrawTriangle((Vector2){cx-R*0.35f,cy-R*0.15f},(Vector2){cx,cy+R*0.15f},(Vector2){cx-R*0.05f,cy-R*0.15f}, blaze);
    DrawTriangle((Vector2){cx+R*0.05f,cy-R*0.15f},(Vector2){cx,cy+R*0.15f},(Vector2){cx+R*0.35f,cy-R*0.15f}, blaze);
    float sw = sinf(t*1.2f)*0.15f;
    DrawLineEx((Vector2){cx,cy-R*0.95f},(Vector2){cx+sw*R,cy-R*0.60f}, R*0.03f, (Color){60,40,20,255});
    DrawTriangle((Vector2){cx+sw*R-R*0.18f,cy-R*0.60f},(Vector2){cx+sw*R+R*0.18f,cy-R*0.60f},(Vector2){cx+sw*R+R*0.30f,cy-R*0.50f}, (Color){210,60,40,230});
    EllipseRotF(cx-R*0.26f, cy+R*0.35f, R*0.08f, R*0.11f, 0.0f, (Color){45,30,15,255});
    EllipseRotF(cx+R*0.26f, cy+R*0.35f, R*0.08f, R*0.11f, 0.0f, (Color){45,30,15,255});
}
static void Cartographer_V8(float cx, float cy, float R, float t) {
    Color body = (Color){150,150,155,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int k=0;k<3;k++) {
        float ry = cy - R*0.55f - k*R*0.35f;
        float rr = R*(0.55f - k*0.14f);
        DrawEllipse((int)cx,(int)ry,(int)rr,(int)(rr*0.55f),(Color){120,120,128,255});
        DrawEllipseLines((int)cx,(int)ry,(int)rr,(int)(rr*0.55f),(Color){80,80,88,220});
    }
    float p = 0.5f+0.5f*sinf(t*1.5f);
    DrawTriangle((Vector2){cx-R*0.3f,cy+R*0.55f},(Vector2){cx+R*0.3f,cy+R*0.55f},(Vector2){cx,cy+R*0.85f}, (Color){220,60,(uchar)(30+40*p),230});
    EllipseRotF(cx-R*0.26f, cy+R*0.10f, R*0.09f, R*0.12f, 0.0f, (Color){60,60,68,255});
    EllipseRotF(cx+R*0.26f, cy+R*0.10f, R*0.09f, R*0.12f, 0.0f, (Color){60,60,68,255});
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
    // Gilded Sovereign — V8's phoenix-warm body/embers minus the flame wings,
    // V1's falling-drop trickle-down effect (now dripping off the crown), plus
    // a far more elaborate five-point jewelled crown than V7's simple one.
    Color body = MidasColor(t);
    float bob = sinf(t*1.2f)*R*0.05f;
    float crownY = cy - R*1.55f + bob;
    DrawRectangle((int)(cx-R*0.55f), (int)crownY, (int)(R*1.10f), (int)(R*0.32f), (Color){255,215,60,255});
    DrawRectangleLines((int)(cx-R*0.55f), (int)crownY, (int)(R*1.10f), (int)(R*0.32f), (Color){160,110,10,220});
    for (int k=-2;k<=2;k++) {
        float px = cx + k*R*0.22f;
        float spike = (k==0)? R*0.12f : 0.0f;
        Color gem = (k==0) ? (Color){220,40,60,255} : ((k%2==0)?(Color){60,140,255,255}:(Color){60,220,120,255});
        DrawTriangle((Vector2){px-R*0.11f,crownY},(Vector2){px+R*0.11f,crownY},(Vector2){px,crownY-R*0.30f-spike}, (Color){255,225,90,255});
        DrawCircleV((Vector2){px,crownY-R*0.30f-spike}, R*0.06f, gem);
    }
    for (int d=-2; d<=2; d++) {
        float dy = fmodf(t*1.4f + d*0.9f, 2.4f);
        float bx = cx + d*R*0.22f, by = crownY + R*0.35f + dy*R*0.65f;
        float sh = 1.0f - dy*0.30f; if (sh<0) sh=0;
        DrawCircleV((Vector2){bx, by}, R*0.075f*sh, (Color){255,215,60,(uchar)(220*sh)});
    }
    for (int k=0;k<5;k++) {
        float ang = t*2.0f+k*1.3f;
        float rr = R*(1.25f+0.18f*sinf(t*3.0f+k));
        DrawCircleV((Vector2){cx+cosf(ang)*rr, cy-R*0.2f+sinf(ang)*rr*0.55f}, R*0.03f, (Color){255,200,80,190});
    }
    DrawCircle((int)cx,(int)cy,(int)R, body);
    EllipseRotF(cx-R*0.30f, cy-R*0.05f, R*0.13f, R*0.17f, 0.0f, (Color){255,240,190,255});
    EllipseRotF(cx+R*0.30f, cy-R*0.05f, R*0.13f, R*0.17f, 0.0f, (Color){255,240,190,255});
}
static void Midas_V4(float cx, float cy, float R, float t) {
    Color body = (Color){225,180,50,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    for (int k=0;k<6;k++) {
        float f = fmodf(t*0.9f + k*0.18f, 1.0f);
        float ang = -PI/2.0f + (k-2.5f)*0.28f;
        float px = cx + cosf(ang)*R*0.5f*f;
        float py = cy - R*1.3f + f*R*2.0f;
        float spin = t*6.0f + k;
        float ew = R*0.14f*fabsf(cosf(spin));
        DrawEllipse((int)px,(int)py,(int)(ew<2?2:ew),(int)(R*0.14f),(Color){255,225,90,230});
    }
    DrawEllipse((int)cx,(int)(cy+R*0.85f),(int)(R*0.6f),(int)(R*0.18f),(Color){255,215,70,120});
    EllipseRotF(cx-R*0.28f, cy-R*0.05f, R*0.10f, R*0.14f, 0.0f, (Color){120,85,10,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.05f, R*0.10f, R*0.14f, 0.0f, (Color){120,85,10,255});
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
    Color body = (Color){200,160,55,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    float tilt = sinf(t*0.9f)*0.18f;
    float beamY = cy-R*1.2f;
    DrawLineEx((Vector2){cx,cy-R*0.8f},(Vector2){cx,beamY}, R*0.05f, (Color){110,80,20,255});
    Vector2 lp = { cx-R*0.55f, beamY-sinf(tilt)*R*0.3f };
    Vector2 rp = { cx+R*0.55f, beamY+sinf(tilt)*R*0.3f };
    DrawLineEx((Vector2){cx,beamY}, lp, R*0.035f, (Color){110,80,20,255});
    DrawLineEx((Vector2){cx,beamY}, rp, R*0.035f, (Color){110,80,20,255});
    for (int k=0;k<3;k++) DrawCircleV((Vector2){lp.x-R*0.08f+k*R*0.08f, lp.y+R*0.12f}, R*0.09f, (Color){255,225,90,220});
    DrawCircleV(rp, R*0.09f, (Color){255,215,70,255});
    DrawCircleV(rp, R*0.05f, (Color){255,245,200,255});
    EllipseRotF(cx-R*0.28f, cy-R*0.02f, R*0.10f, R*0.13f, 0.0f, (Color){110,80,10,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.02f, R*0.10f, R*0.13f, 0.0f, (Color){110,80,10,255});
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
// BLADE LEGEND — 16 variants (Epic, tied to Slayer + Kill Streak Tier 6)
// V4/V5/V7/V8 (Frostbite/Kopfgeldjäger/Klingensturm/Warbanner) were rejected
// and removed; V1/V2/V3/V6 survive unchanged, 12 new ideas fill the rest.
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
    // Blutklinge — #8B0000 body with a darker outer rim, crossed blades sitting
    // in the background (drawn before the body), glowing #FFD700 eye slits.
    DrawCircleV((Vector2){cx, cy}, R*1.35f, (Color){35,0,0,255});
    Color steel = (Color){60,60,65,230};
    for (int s=-1; s<=1; s+=2) {
        float a = (PI/4.0f)*s;
        Vector2 dir = { cosf(a), sinf(a) };
        Vector2 tip  = { cx+dir.x*R*1.30f, cy+dir.y*R*1.30f };
        Vector2 hilt = { cx-dir.x*R*1.30f, cy-dir.y*R*1.30f };
        DrawLineEx(hilt, tip, R*0.09f, steel);
    }
    Color body = (Color){139,0,0,255};
    DrawCircle((int)cx,(int)cy,(int)R, body);
    DrawCircleV((Vector2){cx,cy}, R*0.55f, (Color){175,10,10,140});
    EllipseRotF(cx-R*0.28f, cy-R*0.02f, R*0.15f, R*0.035f, -0.15f, (Color){255,215,0,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.02f, R*0.15f, R*0.035f,  0.15f, (Color){255,215,0,255});
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
static void BladeLegend_Oni(float cx, float cy, float R, float t) {
    // Oni Shogun — gold demon horns, crossed katana behind, drifting embers
    DrawCircleV((Vector2){cx, cy}, R*1.35f, (Color){255,60,40,(uchar)(30+10*sinf(t*3.0f))});
    KatanaBehind(R, cx-R*0.6f, cy+R*0.5f, -0.75f, R*1.6f);
    KatanaBehind(R, cx+R*0.6f, cy+R*0.5f,  0.75f, R*1.6f);
    for (int k=0;k<7;k++) {
        float p = fmodf(t*0.35f+Rnd(k), 1.0f);
        float ex = cx + (Rnd(k+9)-0.5f)*R*2.6f, ey = cy+R*1.6f - p*R*3.4f;
        DrawCircleV((Vector2){ex,ey}, R*0.05f, (Color){255,179,71,(uchar)(200*(1.0f-p))});
    }
    DrawCircle((int)cx,(int)cy,(int)R, (Color){232,67,58,255});
    DrawTriangle((Vector2){cx-R*0.55f,cy-R*0.75f},(Vector2){cx-R*0.15f,cy-R*0.85f},(Vector2){cx-R*0.40f,cy-R*1.55f}, (Color){242,193,78,255});
    DrawTriangle((Vector2){cx+R*0.55f,cy-R*0.75f},(Vector2){cx+R*0.15f,cy-R*0.85f},(Vector2){cx+R*0.40f,cy-R*1.55f}, (Color){242,193,78,255});
    EllipseRotF(cx-R*0.30f, cy-R*0.28f, R*0.17f, R*0.10f, 0.0f, (Color){255,248,208,255});
    EllipseRotF(cx+R*0.30f, cy-R*0.28f, R*0.17f, R*0.10f, 0.0f, (Color){255,248,208,255});
    DrawRectangleRounded((Rectangle){cx-R*0.26f, cy+R*0.26f, R*0.52f, R*0.16f}, 0.5f, 6, (Color){255,255,255,255});
    for (int k=-2;k<=2;k++) DrawLineEx((Vector2){cx+k*R*0.10f, cy+R*0.26f}, (Vector2){cx+k*R*0.10f, cy+R*0.42f}, R*0.02f, (Color){94,15,18,255});
}
static void BladeLegend_Storm(float cx, float cy, float R, float t) {
    // Storm Slasher — steel body, jagged lightning-blade behind, crackling rim arcs
    float fl = fmodf(t, 2.2f);
    bool flash = fl < 0.12f;
    DrawCircleV((Vector2){cx, cy}, R*1.3f, (Color){120,200,255,(uchar)(flash?90:35)});
    Vector2 top  = { cx+R*0.9f,  cy-R*1.3f };
    Vector2 mid1 = { cx+R*0.55f, cy-R*0.2f };
    Vector2 mid2 = { cx+R*0.85f, cy-R*0.05f };
    Vector2 tip  = { cx+R*0.6f,  cy+R*1.15f };
    Color bolt = flash ? (Color){255,255,255,255} : (Color){174,226,255,235};
    DrawTriangle(top, mid1, mid2, bolt);
    DrawTriangle(mid1, tip, mid2, bolt);
    if (flash) {
        for (int k=0;k<3;k++) {
            float ang = Rnd(k+1)*2.0f*PI + t*4.0f;
            DrawLineEx((Vector2){cx+cosf(ang)*R, cy+sinf(ang)*R},
                       (Vector2){cx+cosf(ang)*R*1.35f, cy+sinf(ang)*R*1.35f}, R*0.045f, (Color){220,245,255,230});
        }
    }
    DrawCircle((int)cx,(int)cy,(int)R, (Color){92,110,133,255});
    DrawCircleLines((int)cx,(int)cy,(int)(R-1), (Color){160,220,255,140});
    EllipseRotF(cx-R*0.28f, cy-R*0.06f, R*0.15f, flash?R*0.11f:R*0.06f, 0.0f, (Color){207,238,255,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.06f, R*0.15f, flash?R*0.11f:R*0.06f, 0.0f, (Color){207,238,255,255});
}
static void BladeLegend_VoidReaper(float cx, float cy, float R, float t) {
    // Void Reaper — violet void-rift behind, inward-drawn soul motes, dark scythe
    EllipseRotF(cx, cy, R*1.85f, R*0.75f, 0.25f*sinf(t*0.7f), (Color){26,8,48,255});
    EllipseLinesRot(cx, cy, R*1.85f, R*0.75f, 0.25f*sinf(t*0.7f), R*0.045f, (Color){160,92,255,220});
    for (int k=0;k<9;k++) {
        float p = fmodf(t*0.5f+Rnd(k), 1.0f);
        float ang = Rnd(k+40)*2.0f*PI + t*0.6f;
        float d = R*2.0f*(1.0f-p);
        DrawCircleV((Vector2){cx+cosf(ang)*d, cy+sinf(ang)*d*0.45f}, R*0.05f, (Color){201,160,255,(uchar)(220*p)});
    }
    Vector2 handleTop = { cx+R*0.85f, cy-R*1.35f };
    DrawLineEx((Vector2){cx+R*0.65f,cy+R*0.9f}, handleTop, R*0.09f, (Color){61,43,85,255});
    DrawTriangle(handleTop, (Vector2){handleTop.x-R*0.75f,handleTop.y+R*0.10f}, (Vector2){handleTop.x-R*0.15f,handleTop.y+R*0.55f}, (Color){217,194,255,235});
    DrawCircle((int)cx,(int)cy,(int)R, (Color){36,24,51,255});
    DrawCircleLines((int)cx,(int)cy,(int)R,(Color){160,92,255,200});
    EllipseRotF(cx-R*0.28f, cy-R*0.02f, R*0.11f, R*0.035f, -0.1f, (Color){201,160,255,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.02f, R*0.11f, R*0.035f,  0.1f, (Color){201,160,255,255});
}
static void BladeLegend_Dragonfang(float cx, float cy, float R, float t) {
    // Dragonfang — scale armor, curved fang-wings, drifting breath mist, fang eyes
    for (int k=0;k<4;k++) {
        float p = fmodf(t*0.3f+k*0.25f, 1.0f);
        DrawCircleV((Vector2){cx+R*0.5f+p*R*0.9f, cy+R*0.4f-p*R*0.35f}, R*(0.10f+p*0.25f), (Color){159,184,173,(uchar)(90*(1.0f-p))});
    }
    DrawTriangle((Vector2){cx-R*0.85f,cy+R*0.45f},(Vector2){cx-R*1.55f,cy-R*0.25f},(Vector2){cx-R*1.1f,cy-R*1.1f}, (Color){244,239,226,255});
    DrawTriangle((Vector2){cx+R*0.85f,cy+R*0.45f},(Vector2){cx+R*1.55f,cy-R*0.25f},(Vector2){cx+R*1.1f,cy-R*1.1f}, (Color){244,239,226,255});
    DrawCircle((int)cx,(int)cy,(int)R, (Color){47,158,120,255});
    for (int ro=0; ro<3; ro++)
        for (int k=-2;k<=2;k++)
            HaloArc(cx+k*R*0.36f+(ro%2?R*0.18f:0.0f), cy+R*0.1f+ro*R*0.30f, R*0.18f, R*0.18f, 0.15f*PI, 0.85f*PI, R*0.02f, (Color){10,50,38,160});
    DrawCircleV((Vector2){cx-R*0.30f, cy-R*0.28f}, R*0.10f, (Color){255,210,74,255});
    DrawCircleV((Vector2){cx+R*0.30f, cy-R*0.28f}, R*0.10f, (Color){255,210,74,255});
    EllipseRotF(cx-R*0.30f, cy-R*0.28f, R*0.045f, R*0.09f, 0.0f, (Color){20,20,10,255});
    EllipseRotF(cx+R*0.30f, cy-R*0.28f, R*0.045f, R*0.09f, 0.0f, (Color){20,20,10,255});
}
static void BladeLegend_Gladiator(float cx, float cy, float R, float t) {
    // Gladiator Prime — crested helm arc, round shield, crimson/gold body, gladius
    HaloArc(cx, cy-R*1.0f, R*0.55f, R*0.30f, PI, 2.0f*PI, R*0.16f, (Color){200,60,40,255});
    DrawCircleV((Vector2){cx-R*1.15f, cy+R*0.25f}, R*0.55f, (Color){122,90,48,255});
    DrawCircleLines((int)(cx-R*1.15f),(int)(cy+R*0.25f),(int)(R*0.55f),(Color){232,194,106,255});
    DrawCircleV((Vector2){cx-R*1.15f, cy+R*0.25f}, R*0.14f, (Color){232,194,106,255});
    DrawLineEx((Vector2){cx+R*0.5f,cy+R*1.0f}, (Vector2){cx+R*1.35f,cy-R*1.1f}, R*0.13f, (Color){217,212,200,255});
    float sh = fmodf(t*0.5f, 1.0f);
    if (sh < 0.2f) {
        float q = sh/0.2f;
        DrawLineEx((Vector2){cx+R*0.7f-q*R*0.5f,cy+R*0.5f-q*R*3.0f}, (Vector2){cx+R*0.95f-q*R*0.5f,cy+R*0.7f-q*R*3.0f}, R*0.05f, (Color){255,255,255,(uchar)(255*(1.0f-q))});
    }
    DrawCircle((int)cx,(int)cy,(int)R, (Color){217,164,65,255});
    for (int k=-3;k<=3;k++) {
        float a = -PI/4.0f + k*0.16f;
        DrawCircleV((Vector2){cx+cosf(a)*R*1.05f, cy-R*0.05f+sinf(a)*R*1.05f}, R*0.10f, (Color){195,50,50,255});
    }
    EllipseRotF(cx-R*0.28f, cy-R*0.24f, R*0.10f, R*0.06f, 0.0f, (Color){255,216,118,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.24f, R*0.10f, R*0.06f, 0.0f, (Color){255,216,118,255});
}
static void BladeLegend_Assassin(float cx, float cy, float R, float t) {
    // Shadow Assassin — smoke wisps, blinking cyan visor, occasional dash streak
    for (int k=0;k<6;k++) {
        float p = fmodf(t*0.25f+Rnd(k), 1.0f);
        DrawCircleV((Vector2){cx-R*0.3f-p*R*1.3f, cy+R*0.2f+sinf(t*2.0f+k)*R*0.25f}, R*(0.35f+p*0.3f), (Color){58,53,80,(uchar)(60*(1.0f-p))});
    }
    DrawCircleV((Vector2){cx, cy}, R, (Color){38,35,58,235});
    DrawCircleV((Vector2){cx+sinf(t*1.7f)*R*0.14f, cy}, R*0.94f, (Color){55,51,77,110});
    float dash = fmodf(t*0.45f, 1.0f);
    if (dash < 0.13f) {
        float q = dash/0.13f;
        DrawLineEx((Vector2){cx-R*1.6f+q*R*3.2f, cy+R*1.3f-q*R*2.6f}, (Vector2){cx-R*0.9f+q*R*3.2f, cy+R*0.8f-q*R*2.6f}, R*0.06f, (Color){255,255,255,(uchar)(220*sinf(q*PI))});
    }
    DrawRectangleRounded((Rectangle){cx-R*0.34f, cy-R*0.11f, R*0.68f, R*0.10f}, 1.0f, 6, (Color){159,243,255,240});
}
static void BladeLegend_Tempest(float cx, float cy, float R, float t) {
    // Crimson Tempest — six blades orbiting as a deadly swirl, front/back depth
    DrawCircleV((Vector2){cx, cy}, R*1.3f, (Color){255,70,60,33});
    int n = 6;
    for (int k=0;k<n;k++) {
        float ang = t*4.0f + k*(2.0f*PI/n);
        if (sinf(ang) > 0.0f) continue; // behind
        float bx = cx+cosf(ang)*R*1.65f, by = cy+sinf(ang)*R*0.55f;
        DrawTriangle((Vector2){bx,by-R*0.30f},(Vector2){bx+R*0.09f,by+R*0.17f},(Vector2){bx-R*0.09f,by+R*0.17f}, (Color){255,217,200,235});
    }
    DrawCircle((int)cx,(int)cy,(int)R, (Color){224,72,60,255});
    EllipseRotF(cx-R*0.28f, cy-R*0.06f, R*0.15f, R*0.075f, 0.0f, (Color){255,224,176,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.06f, R*0.15f, R*0.075f, 0.0f, (Color){255,224,176,255});
    for (int k=0;k<n;k++) {
        float ang = t*4.0f + k*(2.0f*PI/n);
        if (sinf(ang) <= 0.0f) continue; // front
        float bx = cx+cosf(ang)*R*1.65f, by = cy+sinf(ang)*R*0.55f;
        DrawTriangle((Vector2){bx,by-R*0.30f},(Vector2){bx+R*0.09f,by+R*0.17f},(Vector2){bx-R*0.09f,by+R*0.17f}, (Color){255,217,200,235});
    }
}
static void BladeLegend_SoulHarvester(float cx, float cy, float R, float t) {
    // Soul Harvester — rising soul-wisps, glowing rune notches, green eyes
    for (int k=0;k<6;k++) {
        float p = fmodf(t*0.4f+Rnd(k), 1.0f);
        float wx = cx + (Rnd(k+7)-0.5f)*R*2.3f + sinf(t*3.0f+k*2.0f)*R*0.2f;
        float wy = cy+R*1.5f - p*R*3.4f;
        EllipseRotF(wx, wy, R*0.08f, R*0.2f, sinf(t*3.0f+k)*0.3f, (Color){109,255,158,(uchar)(180*(1.0f-p))});
    }
    DrawCircle((int)cx,(int)cy,(int)R, (Color){61,74,63,255});
    float gx[3] = { cx-R*0.47f, cx+R*0.07f, cx-R*0.20f };
    float gy[3] = { cy+R*0.13f, cy+R*0.27f, cy+R*0.53f };
    for (int g=0; g<3; g++) {
        float gl = 0.55f+0.45f*sinf(t*2.0f+g*2.0f);
        Color rc = (Color){142,255,180,(uchar)(255*gl)};
        for (int k=0;k<4;k++) DrawLineEx((Vector2){gx[g]+k*R*0.10f, gy[g]-R*0.10f},(Vector2){gx[g]+k*R*0.10f, gy[g]+R*0.10f}, R*0.025f, rc);
    }
    EllipseRotF(cx-R*0.30f, cy-R*0.24f, R*0.11f, R*0.06f, 0.0f, (Color){180,255,206,255});
    EllipseRotF(cx+R*0.30f, cy-R*0.24f, R*0.11f, R*0.06f, 0.0f, (Color){180,255,206,255});
}
static void BladeLegend_Obsidian(float cx, float cy, float R, float t) {
    // Obsidian Duelist — glass-black body, pulsing lava cracks, elegant rapier
    DrawLineEx((Vector2){cx+R*1.05f,cy+R*0.55f},(Vector2){cx+R*1.05f,cy-R*1.35f}, R*0.06f, (Color){232,226,216,255});
    DrawCircleLines((int)(cx+R*1.05f),(int)(cy+R*0.48f),(int)(R*0.17f),(Color){200,164,74,255});
    DrawCircle((int)cx,(int)cy,(int)R, (Color){44,40,51,255});
    DrawCircleV((Vector2){cx-R*0.36f, cy-R*0.42f}, R*0.22f, (Color){255,255,255,60});
    float gl = 0.5f+0.5f*sinf(t*2.4f);
    Color crack = (Color){255,(uchar)(110+70*gl),30,(uchar)(140+100*gl)};
    Vector2 seg1[4] = {
        { cx-R*0.26f, cy-R*0.86f },
        { cx-R*0.13f, cy-R*0.40f },
        { cx-R*0.40f, cy-R*0.07f },
        { cx-R*0.23f, cy+R*0.33f }
    };
    for (int i=0;i<3;i++) DrawLineEx(seg1[i], seg1[i+1], R*0.03f, crack);
    DrawLineEx((Vector2){cx+R*0.33f,cy-R*0.66f},(Vector2){cx+R*0.2f,cy-R*0.2f}, R*0.03f, crack);
    DrawLineEx((Vector2){cx+R*0.2f,cy-R*0.2f},(Vector2){cx+R*0.46f,cy+R*0.13f}, R*0.03f, crack);
    EllipseRotF(cx-R*0.28f, cy-R*0.2f, R*0.09f, R*0.12f, 0.0f, (Color){255,176,102,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.2f, R*0.09f, R*0.12f, 0.0f, (Color){255,176,102,255});
}
static void BladeLegend_ThousandCuts(float cx, float cy, float R, float t) {
    // Thousand Cuts — scar-covered veteran, each scar flashes gold in turn
    DrawCircle((int)cx,(int)cy,(int)R, (Color){138,109,112,255});
    float scars[10][3] = {
        {-14,-14,0.5f},{6,-18,-0.4f},{-4,-4,0.9f},{12,-2,0.3f},{-18,2,-0.7f},
        {2,10,0.6f},{14,12,-0.5f},{-10,18,0.2f},{-2,-24,-0.2f},{8,20,0.8f}
    };
    for (int i=0;i<10;i++) {
        float ph = fmodf(t*0.5f - i*0.1f, 1.0f);
        float hot = (ph < 0.22f) ? sinf(ph/0.22f*PI) : 0.0f;
        Vector2 c = { cx+scars[i][0]*R/30.0f, cy+scars[i][1]*R/30.0f };
        Vector2 d = { cosf(scars[i][2]), sinf(scars[i][2]) };
        Color col = hot>0.0f ? (Color){255,233,168,255} : (Color){255,190,110,115};
        DrawLineEx((Vector2){c.x-d.x*R*0.20f, c.y-d.y*R*0.20f}, (Vector2){c.x+d.x*R*0.20f, c.y+d.y*R*0.20f}, R*(0.025f+hot*0.02f), col);
    }
    EllipseRotF(cx-R*0.28f, cy-R*0.06f, R*0.15f, R*0.075f, 0.0f, (Color){255,216,118,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.06f, R*0.15f, R*0.075f, 0.0f, (Color){255,216,118,255});
}
static void BladeLegend_WarGod(float cx, float cy, float R, float t) {
    // War God — burning flame crown, crossed spears, ember rain, crimson body
    for (int k=0;k<10;k++) {
        float p = fmodf(t*0.3f+Rnd(k), 1.0f);
        float ex = cx + (Rnd(k+3)-0.5f)*R*4.0f;
        float ey = cy - R*2.2f + p*R*4.6f;
        Color c = (k%3) ? (Color){122,115,110,(uchar)(90*(1.0f-p*0.4f))} : (Color){255,171,94,(uchar)(140*(1.0f-p*0.4f))};
        DrawCircleV((Vector2){ex,ey}, (k%3)?R*0.04f:R*0.06f, c);
    }
    Vector2 spearTip1 = { cx-R*0.5f, cy-R*2.05f }, spearTip2 = { cx+R*0.5f, cy-R*2.05f };
    DrawLineEx((Vector2){cx-R*0.47f, cy+R*0.65f}, spearTip1, R*0.10f, (Color){110,88,68,255});
    DrawLineEx((Vector2){cx+R*0.47f, cy+R*0.65f}, spearTip2, R*0.10f, (Color){110,88,68,255});
    DrawTriangle(spearTip1, (Vector2){spearTip1.x-R*0.18f,spearTip1.y+R*0.5f}, (Vector2){spearTip1.x+R*0.18f,spearTip1.y+R*0.5f}, (Color){216,210,196,255});
    DrawTriangle(spearTip2, (Vector2){spearTip2.x-R*0.18f,spearTip2.y+R*0.5f}, (Vector2){spearTip2.x+R*0.18f,spearTip2.y+R*0.5f}, (Color){216,210,196,255});
    DrawCircleV((Vector2){cx, cy-R*1.13f}, R*0.5f, (Color){255,140,40,90});
    DrawCircle((int)cx,(int)cy,(int)R, (Color){194,58,46,255});
    for (int k=-2;k<=2;k++) {
        float fx = cx+k*R*0.30f;
        float fh = R*(0.33f+0.17f*sinf(t*7.0f+k*2.1f));
        DrawTriangle((Vector2){fx-R*0.12f,cy-R*0.86f}, (Vector2){fx,cy-R*1.0f-fh}, (Vector2){fx+R*0.12f,cy-R*0.86f}, (Color){255,179,71,255});
    }
    EllipseRotF(cx-R*0.30f, cy-R*0.04f, R*0.15f, R*0.08f, 0.0f, (Color){255,242,216,255});
    EllipseRotF(cx+R*0.30f, cy-R*0.04f, R*0.15f, R*0.08f, 0.0f, (Color){255,242,216,255});
}
static void BladeLegend_Neon(float cx, float cy, float R, float t) {
    // Neon Slayer — cyber-katana glow, glitch flicker, scanline overlay
    DrawRectangleRounded((Rectangle){cx-R*1.35f, cy-R*1.35f, R*2.7f, R*2.7f}, 0.35f, 8, (Color){14,11,23,235});
    DrawLineEx((Vector2){cx+R*0.6f,cy+R*0.9f}, (Vector2){cx+R*0.6f,cy-R*1.5f}, R*0.10f, (Color){191,253,255,255});
    DrawLineEx((Vector2){cx+R*0.45f,cy+R*0.9f}, (Vector2){cx+R*0.45f,cy-R*1.35f}, R*0.035f, (Color){255,63,216,255});
    bool glitch = fmodf(t*1.3f, 3.0f) < 0.10f;
    float off = glitch ? (Rnd((int)(t*30.0f))-0.5f)*R*0.3f : 0.0f;
    DrawCircle((int)(cx+off),(int)cy,(int)R, (Color){34,29,51,255});
    DrawCircleLines((int)(cx+off),(int)cy,(int)R,(Color){255,63,216,220});
    EllipseRotF(cx+off-R*0.28f, cy-R*0.06f, R*0.14f, R*0.045f, 0.0f, (Color){191,253,255,255});
    EllipseRotF(cx+off+R*0.28f, cy-R*0.06f, R*0.14f, R*0.045f, 0.0f, (Color){191,253,255,255});
    if (glitch) {
        DrawRectangle((int)(cx-R*1.3f), (int)(cy+R*0.2f), (int)(R*2.6f), 2, (Color){63,245,255,180});
        DrawRectangle((int)(cx-R*0.2f), (int)(cy-R*0.6f), (int)(R*2.6f), 2, (Color){255,63,216,180});
    }
    for (float y = cy-R*1.3f; y < cy+R*1.3f; y += R*0.16f)
        DrawLine((int)(cx-R*1.3f), (int)y, (int)(cx+R*1.3f), (int)y, (Color){159,255,255,20});
}

// ═══════════════════════════════════════════════════════════════════════════
// CONSTELLATION — 10 variants (Legendary, tied to Overachiever Tier 6)
// V1-V6 were rejected and removed; V7/V8 survive unchanged, 8 new ideas
// fill the rest.
// ═══════════════════════════════════════════════════════════════════════════
static void Constellation_Nebula(float cx, float cy, float R, float t) {
    // Nebula Genesis — swirling colour nebula, a new star igniting at the core
    DrawCircleV((Vector2){cx, cy}, R*1.4f, (Color){120,90,255,36});
    DrawCircle((int)cx,(int)cy,(int)R, (Color){42,34,88,255});
    Color neb[3] = { (Color){255,80,180,70}, (Color){80,160,255,70}, (Color){160,80,255,70} };
    for (int k=0;k<6;k++) {
        float ang = t*0.4f + k*(2.0f*PI/6.0f);
        float d = R*(0.4f+0.2f*sinf(t*0.7f+k));
        DrawCircleV((Vector2){cx+cosf(ang)*d, cy+sinf(ang)*d}, R*0.36f, neb[k%3]);
    }
    float pl = 0.6f+0.4f*sinf(t*2.2f);
    Star4(cx, cy-R*0.05f, R*(0.25f+0.1f*pl), (Color){255,255,255,255});
    EllipseRotF(cx-R*0.3f, cy+R*0.32f, R*0.10f, R*0.05f, 0.0f, (Color){207,216,255,220});
    EllipseRotF(cx+R*0.3f, cy+R*0.32f, R*0.10f, R*0.05f, 0.0f, (Color){207,216,255,220});
}
static void Constellation_Supernova(float cx, float cy, float R, float t) {
    // Supernova Core — pulsing heart, expanding shockwave rings
    DrawCircle((int)cx,(int)cy,(int)R, (Color){58,42,102,255});
    for (int i=0;i<3;i++) {
        float p = fmodf(t*0.45f+i/3.0f, 1.0f);
        DrawCircleLines((int)cx,(int)cy,(int)(R*0.3f+p*R*1.75f), (Color){255,176,102,(uchar)(200*(1.0f-p))});
    }
    float pl = 0.5f+0.5f*sinf(t*3.0f);
    DrawCircleV((Vector2){cx,cy}, R*(0.35f+0.15f*pl), (Color){255,160,80,(uchar)(140*pl+60)});
    DrawCircleV((Vector2){cx,cy+R*0.03f}, R*(0.18f+0.08f*pl), (Color){255,242,221,255});
    EllipseRotF(cx-R*0.30f, cy-R*0.4f, R*0.09f, R*0.045f, 0.0f, (Color){255,226,192,255});
    EllipseRotF(cx+R*0.30f, cy-R*0.4f, R*0.09f, R*0.045f, 0.0f, (Color){255,226,192,255});
}
static void Constellation_AstralDragon(float cx, float cy, float R, float t) {
    // Astral Dragon — a serpent of starlight coils around the body
    Vector2 pts[9]; bool front[9];
    for (int i=0;i<9;i++) {
        float u = i/8.0f;
        float ang = t*0.7f + u*2.0f*PI*1.15f;
        float rr = R*(1.35f + 0.2f*sinf(t+u*9.0f));
        pts[i] = (Vector2){ cx+cosf(ang)*rr, cy+sinf(ang)*rr*0.55f-u*R*0.35f+R*0.25f };
        front[i] = sinf(ang) <= 0.0f;
    }
    for (int pass=0; pass<2; pass++) {
        bool wantFront = (pass==1);
        Vector2 prev = {0,0}; bool have = false;
        for (int i=0;i<9;i++) {
            if (front[i] != wantFront) { have=false; continue; }
            if (have) DrawLineEx(prev, pts[i], R*0.03f, (Color){150,190,255,110});
            prev = pts[i]; have = true;
            float rr = (i==8) ? R*0.16f : R*0.09f;
            Star4(pts[i].x, pts[i].y, rr, i==8 ? (Color){255,255,255,255} : (Color){207,224,255,230});
        }
        if (pass==0) DrawCircle((int)cx,(int)cy,(int)R, (Color){44,36,102,255});
    }
    EllipseRotF(cx-R*0.28f, cy-R*0.06f, R*0.10f, R*0.05f, 0.0f, (Color){207,224,255,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.06f, R*0.10f, R*0.05f, 0.0f, (Color){207,224,255,255});
}
static void Constellation_Comet(float cx, float cy, float R, float t) {
    // Comet Rider — a comet with a long icy tail on a tight orbit
    float ang = t*1.6f;
    float ex = cx+cosf(ang)*R*1.75f, ey = cy+sinf(ang)*R*0.8f-R*0.2f;
    bool front = sinf(ang) <= 0.0f;
    if (!front) {
        for (int i=1;i<=10;i++) {
            float aa = ang - i*0.11f;
            float tx = cx+cosf(aa)*R*1.75f, ty = cy+sinf(aa)*R*0.8f-R*0.2f;
            DrawCircleV((Vector2){tx,ty}, R*0.14f*(1.0f-(float)i/12.0f), (Color){127,184,255,(uchar)(150*(1.0f-(float)i/10.0f))});
        }
        DrawCircleV((Vector2){ex,ey}, R*0.15f, (Color){255,255,255,255});
    }
    DrawCircle((int)cx,(int)cy,(int)R, (Color){40,48,110,255});
    for (int k=0;k<5;k++) {
        float tw = 0.4f+0.6f*fabsf(sinf(t*2.0f+k*1.7f));
        Star4(cx+(Rnd(k+2)-0.5f)*R*1.5f, cy+(Rnd(k+11)-0.5f)*R*1.5f, R*0.07f*tw, (Color){223,234,255,(uchar)(255*tw)});
    }
    EllipseRotF(cx-R*0.28f, cy-R*0.06f, R*0.09f, R*0.045f, 0.0f, (Color){223,234,255,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.06f, R*0.09f, R*0.045f, 0.0f, (Color){223,234,255,255});
    if (front) {
        for (int i=1;i<=10;i++) {
            float aa = ang - i*0.11f;
            float tx = cx+cosf(aa)*R*1.75f, ty = cy+sinf(aa)*R*0.8f-R*0.2f;
            DrawCircleV((Vector2){tx,ty}, R*0.14f*(1.0f-(float)i/12.0f), (Color){127,184,255,(uchar)(150*(1.0f-(float)i/10.0f))});
        }
        DrawCircleV((Vector2){ex,ey}, R*0.15f, (Color){255,255,255,255});
    }
}
static void Constellation_Orrery(float cx, float cy, float R, float t) {
    // Celestial Orrery — brass orbital rings with circling planets
    EllipseLinesRot(cx, cy, R*1.75f, R*0.55f, 0.25f, R*0.035f, (Color){220,180,110,180});
    EllipseLinesRot(cx, cy, R*1.75f, R*0.73f, -0.35f, R*0.035f, (Color){220,180,110,180});
    for (int pass=0; pass<2; pass++) {
        bool wantFront = (pass==1);
        float a1 = t*1.1f, c1=cosf(0.25f), s1=sinf(0.25f);
        float px1=cosf(a1)*R*1.75f, py1=sinf(a1)*R*0.55f;
        bool f1 = sinf(a1) <= 0.0f;
        if (f1==wantFront) DrawCircleV((Vector2){cx+px1*c1-py1*s1, cy+px1*s1+py1*c1}, R*0.11f, (Color){255,176,102,255});
        float a2 = -t*0.7f, c2=cosf(-0.35f), s2=sinf(-0.35f);
        float px2=cosf(a2)*R*1.75f, py2=sinf(a2)*R*0.73f;
        bool f2 = sinf(a2) <= 0.0f;
        if (f2==wantFront) DrawCircleV((Vector2){cx+px2*c2-py2*s2, cy+px2*s2+py2*c2}, R*0.09f, (Color){143,216,255,255});
        if (pass==0) {
            DrawCircle((int)cx,(int)cy,(int)R, (Color){74,58,122,255});
            DrawCircleLines((int)cx,(int)cy,(int)(R*0.86f),(Color){232,194,106,220});
        }
    }
    DrawCircleV((Vector2){cx,cy-R*0.06f}, R*0.17f, (Color){255,216,118,255});
    EllipseRotF(cx-R*0.30f, cy+R*0.32f, R*0.09f, R*0.045f, 0.0f, (Color){255,233,184,255});
    EllipseRotF(cx+R*0.30f, cy+R*0.32f, R*0.09f, R*0.045f, 0.0f, (Color){255,233,184,255});
}
static void Constellation_GalaxyVeil(float cx, float cy, float R, float t) {
    // Galaxy Veil — a flowing cloak of galactic dust and starlight
    float w1 = R*0.25f*sinf(t*1.2f);
    Vector2 lp[4] = {
        { cx-R*0.2f, cy-R*1.1f }, { cx-R*2.2f+w1, cy-R*0.6f },
        { cx-R*2.2f-w1, cy+R*1.6f }, { cx-R*0.5f, cy+R*1.9f }
    };
    DrawTriangle(lp[0], lp[1], lp[2], (Color){120,80,255,60});
    DrawTriangle(lp[0], lp[2], lp[3], (Color){255,90,200,55});
    Vector2 rp[4] = {
        { cx+R*0.2f, cy-R*1.1f }, { cx+R*2.2f-w1, cy-R*0.6f },
        { cx+R*2.2f+w1, cy+R*1.6f }, { cx+R*0.5f, cy+R*1.9f }
    };
    DrawTriangle(rp[0], rp[1], rp[2], (Color){60,150,255,60});
    DrawTriangle(rp[0], rp[2], rp[3], (Color){255,90,200,55});
    for (int k=0;k<10;k++) {
        float p = Rnd(k);
        float sx = cx+(k%2?1:-1)*(R*1.1f+p*R*0.9f), sy = cy-R+p*R*2.8f;
        float tw = 0.3f+0.7f*fabsf(sinf(t*2.5f+k));
        DrawCircleV((Vector2){sx+sinf(t+k)*R*0.2f, sy}, R*0.045f, (Color){238,230,255,(uchar)(255*tw)});
    }
    DrawCircle((int)cx,(int)cy,(int)R, (Color){51,42,110,255});
    EllipseRotF(cx-R*0.30f, cy-R*0.22f, R*0.15f, R*0.075f, 0.0f, (Color){242,238,255,255});
    EllipseRotF(cx+R*0.30f, cy-R*0.22f, R*0.15f, R*0.075f, 0.0f, (Color){242,238,255,255});
}
static void Constellation_Eclipse(float cx, float cy, float R, float t) {
    // Eclipse — dark disc, blazing corona, orbiting diamond-ring flash
    DrawCircleV((Vector2){cx, cy}, R*1.75f, (Color){255,200,110,52});
    for (int k=0;k<10;k++) {
        float ang = t*0.5f + k*(2.0f*PI/10.0f);
        float len = R*(1.0f+0.35f*sinf(t*2.0f+k*2.6f));
        DrawTriangle((Vector2){cx+cosf(ang)*R*0.98f, cy+sinf(ang)*R*0.98f-R*0.10f},
                     (Vector2){cx+cosf(ang)*(R+len), cy+sinf(ang)*(R+len)-R*0.10f},
                     (Vector2){cx+cosf(ang+0.06f)*R*0.98f, cy+sinf(ang+0.06f)*R*0.98f-R*0.10f},
                     (Color){255,200,110,130});
    }
    DrawCircleLines((int)cx,(int)(cy-R*0.10f),(int)(R*1.02f),(Color){255,232,184,255});
    DrawCircle((int)cx,(int)(cy-R*0.10f),(int)R, (Color){22,17,25,255});
    float da = t*0.9f;
    float dx = cx+cosf(da)*R*1.02f, dy = cy-R*0.10f+sinf(da)*R*1.02f;
    float pl = 0.6f+0.4f*sinf(t*5.0f);
    Star4(dx, dy, R*(0.18f+0.1f*pl), (Color){255,255,255,255});
    EllipseRotF(cx-R*0.28f, cy-R*0.16f, R*0.09f, R*0.045f, 0.0f, (Color){255,224,158,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.16f, R*0.09f, R*0.045f, 0.0f, (Color){255,224,158,255});
}
static void Constellation_StarForge(float cx, float cy, float R, float t) {
    // Star Forge — hammer strikes forge fresh stars, showering sparks
    DrawRectangle((int)(cx-R*0.8f), (int)(cy+R*1.15f), (int)(R*1.6f), (int)(R*0.35f), (Color){74,67,86,255});
    float hit = fmodf(t*0.6f, 1.0f);
    bool strike = hit < 0.1f;
    float ha = strike ? 0.9f : (-0.6f - 0.5f*sinf(hit*2.0f*PI));
    Vector2 hb = { cx+R*1.0f, cy+R*0.6f };
    Vector2 ht = { hb.x + sinf(ha)*R*0.8f, hb.y - cosf(ha)*R*0.8f };
    DrawLineEx(hb, ht, R*0.10f, (Color){110,88,68,255});
    DrawRectanglePro((Rectangle){ht.x,ht.y,R*0.5f,R*0.3f}, (Vector2){R*0.25f,R*0.15f}, ha*(180.0f/PI), (Color){184,178,196,255});
    if (strike) DrawCircleV((Vector2){cx,cy+R*1.0f}, R*0.85f, (Color){255,230,150,110});
    for (int k=0;k<8;k++) {
        float p = fmodf(t*0.6f+Rnd(k)*0.9f, 1.0f);
        float sx = cx+(Rnd(k+4)-0.5f)*R*2.0f;
        float sy = cy+R*1.0f-p*R*2.7f+8.0f*p*p*R*0.33f;
        Color c = p<0.4f ? (Color){255,216,118,255} : (Color){207,224,255,255};
        Star4(sx, sy, p<0.4f?R*0.06f:R*0.09f, (Color){c.r,c.g,c.b,(uchar)(255*(1.0f-p))});
    }
    DrawCircle((int)cx,(int)cy,(int)R, (Color){90,74,134,255});
    EllipseRotF(cx-R*0.28f, cy-R*0.06f, R*0.10f, R*0.05f, 0.0f, (Color){255,233,184,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.06f, R*0.10f, R*0.05f, 0.0f, (Color){255,233,184,255});
    DrawCircleV((Vector2){cx, cy+R*0.33f}, R*0.06f+(strike?R*0.06f:0.0f), (Color){255,216,118,230});
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
// ASCENDANT — 9 variants (Legendary, tied to Perfectionist Tier 6)
// V2-V8 were rejected and removed; V1 survives unchanged, 8 new ideas fill
// the rest.
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
static void Ascendant_Seraph(float cx, float cy, float R, float t) {
    // Seraph — six slow-beating wings of light, golden halo above
    for (int side=-1; side<=1; side+=2) {
        for (int i=0;i<3;i++) {
            float base = -0.15f - i*0.5f;
            float fl = 0.12f*sinf(t*2.2f+i);
            float ang = side*(base+fl);
            Vector2 root = { cx+side*R*0.65f, cy-R*0.13f+i*R*0.26f };
            Vector2 tip  = { root.x + side*cosf(ang)*R*1.7f, root.y - sinf(ang)*R*1.0f };
            Vector2 mid  = { root.x + side*cosf(ang)*R*1.0f, root.y - sinf(ang)*R*0.55f - R*0.35f };
            DrawTriangle(root, mid, tip, (Color){255,244,214,(uchar)(230-i*40)});
        }
    }
    DrawCircle((int)cx,(int)cy,(int)R, (Color){255,246,224,255});
    EllipseRotF(cx-R*0.30f, cy-R*0.06f, R*0.10f, R*0.05f, 0.0f, (Color){138,100,32,255});
    EllipseRotF(cx+R*0.30f, cy-R*0.06f, R*0.10f, R*0.05f, 0.0f, (Color){138,100,32,255});
    HaloArc(cx, cy-R*1.35f, R*0.57f, R*0.17f, 0.0f, 2.0f*PI, R*0.05f, (Color){255,233,184,235});
}
static void Ascendant_Apotheosis(float cx, float cy, float R, float t) {
    // Apotheosis — the body dissolves upward into rising golden light
    DrawRectangleGradientV((int)(cx-R*0.9f), (int)(cy-R*2.5f), (int)(R*1.8f), (int)(R*2.5f),
                            (Color){255,235,180,140}, (Color){255,235,180,0});
    DrawCircle((int)cx,(int)cy,(int)R, (Color){255,237,196,255});
    for (int k=0;k<12;k++) {
        float p = fmodf(t*0.4f+Rnd(k), 1.0f);
        float px = cx+(Rnd(k+8)-0.5f)*R*1.6f*(1.0f-p*0.4f), py = cy-R*0.35f-p*R*2.1f;
        DrawCircleV((Vector2){px,py}, R*(0.04f+Rnd(k+3)*0.05f), (Color){255,233,184,(uchar)(255*(1.0f-p))});
    }
    float gl = 0.55f+0.2f*sinf(t*3.0f);
    DrawCircleV((Vector2){cx, cy-R*0.55f}, R*0.62f*gl, (Color){255,248,230,120});
    EllipseRotF(cx-R*0.28f, cy-R*0.04f, R*0.10f, R*0.05f, 0.0f, (Color){138,100,32,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.04f, R*0.10f, R*0.05f, 0.0f, (Color){138,100,32,255});
}
static void Ascendant_Prism(float cx, float cy, float R, float t) {
    // Prism Avatar — crystalline body splitting white light into a rainbow fan
    DrawLineEx((Vector2){cx-R*2.2f,cy-R*1.7f}, (Vector2){cx-R*0.2f,cy-R*0.2f}, R*0.12f, (Color){255,255,255,160});
    Color rb[6] = {
        (Color){255,90,90,200},(Color){255,176,74,200},(Color){255,233,90,200},
        (Color){90,222,122,200},(Color){90,184,255,200},(Color){160,106,255,200}
    };
    for (int i=0;i<6;i++) {
        float ang = 0.35f+i*0.11f+0.03f*sinf(t*2.0f+i);
        DrawLineEx((Vector2){cx+R*0.2f,cy+R*0.2f}, (Vector2){cx+R*0.2f+cosf(ang)*R*2.1f, cy+R*0.2f+sinf(ang)*R*2.1f}, R*0.09f, rb[i]);
    }
    DrawCircle((int)cx,(int)cy,(int)R, (Color){244,250,255,255});
    EllipseLinesRot(cx, cy, R*0.9f, R*0.9f, 0.0f, R*0.02f, (Color){255,255,255,140});
    float tw = fabsf(sinf(t*2.6f));
    Star4(cx-R*0.4f, cy-R*0.47f, R*(0.1f+0.13f*tw), (Color){255,255,255,255});
    EllipseRotF(cx-R*0.28f, cy+R*0.02f, R*0.10f, R*0.05f, 0.0f, (Color){74,106,148,255});
    EllipseRotF(cx+R*0.28f, cy+R*0.02f, R*0.10f, R*0.05f, 0.0f, (Color){74,106,148,255});
}
static void Ascendant_Hierarch(float cx, float cy, float R, float t) {
    // Halo Hierarch — three halos rotate on independent gyroscopic axes
    float rys[3] = {20.0f,26.0f,14.0f};
    float sps[3] = {0.9f,0.7f,1.15f};
    float phs[3] = {0.0f,2.0f,4.0f};
    for (int pass=0; pass<2; pass++) {
        bool wantFront = (pass==1);
        for (int i=0;i<3;i++) {
            float ry = (rys[i]/30.0f*R) * sinf(t*sps[i]+phs[i]);
            bool frontHalo = (ry <= 0.0f);
            if (frontHalo != wantFront) continue;
            EllipseLinesRot(cx, cy, R*1.47f, fabsf(ry), phs[i]*0.4f, R*0.07f, (Color){255,224,140,230});
        }
        if (pass==0) DrawCircle((int)cx,(int)cy,(int)R, (Color){255,244,218,255});
    }
    EllipseRotF(cx-R*0.28f, cy-R*0.04f, R*0.10f, R*0.05f, 0.0f, (Color){138,100,32,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.04f, R*0.10f, R*0.05f, 0.0f, (Color){138,100,32,255});
}
static void Ascendant_Dawn(float cx, float cy, float R, float t) {
    // Dawnbringer — sunrise body with a slowly rotating ray crown
    for (int k=0;k<12;k++) {
        float ang = t*0.25f + k*(2.0f*PI/12.0f);
        Vector2 a = { cx+cosf(ang)*R*0.98f, cy+sinf(ang)*R*0.98f };
        Vector2 b = { cx+cosf(ang)*R*1.75f, cy+sinf(ang)*R*1.75f };
        DrawLineEx(a, b, R*0.08f, (Color){255,180,100,110});
    }
    DrawLine((int)(cx-R*1.8f), (int)(cy+R*0.55f), (int)(cx+R*1.8f), (int)(cy+R*0.55f), (Color){255,150,90,120});
    DrawCircleGradient((Vector2){cx,cy}, R, (Color){255,222,138,255}, (Color){200,80,106,255});
    for (int k=0;k<6;k++) {
        float p = fmodf(t*0.25f+Rnd(k), 1.0f);
        DrawCircleV((Vector2){cx+(Rnd(k+6)-0.5f)*R*3.0f, cy+R*1.0f-p*R*2.3f}, R*0.05f, (Color){255,233,184,(uchar)(220*sinf(p*PI))});
    }
    EllipseRotF(cx-R*0.28f, cy-R*0.06f, R*0.10f, R*0.05f, 0.0f, (Color){122,46,48,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.06f, R*0.10f, R*0.05f, 0.0f, (Color){122,46,48,255});
    float fa = t*0.7f;
    Star4(cx+cosf(fa)*R*0.7f, cy-R*0.27f+sinf(fa)*R*0.4f, R*0.1f, (Color){255,255,255,(uchar)(200*(0.6f+0.4f*sinf(t*3.0f)))});
}
static void Ascendant_Lotus(float cx, float cy, float R, float t) {
    // Transcendence — meditating above a blooming lotus of light
    float bob = R*0.14f*sinf(t*1.5f);
    for (int i=-3;i<=3;i++) {
        float pl = 0.7f+0.3f*sinf(t*2.0f+i);
        float ang = i*0.3f;
        Vector2 base = { cx+sinf(ang)*R*0.15f, cy+R*1.3f };
        Vector2 tip  = { base.x+sinf(ang)*R*0.9f, base.y-R*0.85f };
        DrawTriangle((Vector2){base.x-R*0.16f,base.y}, tip, (Vector2){base.x+R*0.16f,base.y}, (Color){255,222,138,(uchar)(200*pl)});
    }
    DrawCircleV((Vector2){cx, cy+R*1.3f}, R*1.1f, (Color){255,220,150,60});
    DrawCircle((int)cx,(int)(cy-R*0.2f+bob),(int)R, (Color){255,242,212,255});
    for (int k=0;k<5;k++) {
        float p = fmodf(t*0.3f+Rnd(k), 1.0f);
        DrawCircleV((Vector2){cx+(Rnd(k+9)-0.5f)*R*2.0f, cy+R*0.6f-p*R*2.1f+bob*0.3f}, R*0.045f, (Color){255,233,184,(uchar)(200*sinf(p*PI))});
    }
    EllipseRotF(cx-R*0.28f, cy-R*0.24f+bob, R*0.10f, R*0.05f, 0.0f, (Color){138,100,32,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.24f+bob, R*0.10f, R*0.05f, 0.0f, (Color){138,100,32,255});
}
static void Ascendant_Judgement(float cx, float cy, float R, float t) {
    // Divine Judgement — a column of light descends, ascending golden runes
    DrawTriangle((Vector2){cx-R*0.47f, cy-R*2.6f}, (Vector2){cx+R*0.47f, cy-R*2.6f}, (Vector2){cx+R*1.0f, cy+R*1.0f}, (Color){255,240,200,70});
    DrawTriangle((Vector2){cx-R*0.47f, cy-R*2.6f}, (Vector2){cx+R*1.0f, cy+R*1.0f}, (Vector2){cx-R*1.0f, cy+R*1.0f}, (Color){255,240,200,70});
    DrawRectangle((int)(cx-R*0.10f), (int)(cy-R*2.6f), (int)(R*0.20f), (int)(R*3.6f), (Color){255,255,255,(uchar)(90+40*sinf(t*4.0f))});
    for (int k=0;k<5;k++) {
        float p = fmodf(t*0.28f+Rnd(k), 1.0f);
        float sx = cx-R*0.55f+Rnd(k+2)*R*1.1f, sy = cy+R*0.7f-p*R*2.8f;
        Star4(sx, sy, R*0.075f, (Color){255,233,184,(uchar)(230*sinf(p*PI))});
    }
    DrawCircle((int)cx,(int)cy,(int)R, (Color){255,246,226,255});
    EllipseRotF(cx-R*0.28f, cy-R*0.10f, R*0.10f, R*0.05f, 0.0f, (Color){138,100,32,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.10f, R*0.10f, R*0.05f, 0.0f, (Color){138,100,32,255});
}
static void Ascendant_Flawless(float cx, float cy, float R, float t) {
    // The Flawless — a completion ring that seals itself with a flash of light
    float ph = fmodf(t*0.3f, 1.0f);
    bool done = ph > 0.85f;
    float prog = ph/0.85f; if (prog > 1.0f) prog = 1.0f;
    DrawCircleV((Vector2){cx,cy}, R*1.35f, (Color){255,235,190,(uchar)(done?100:30)});
    DrawCircle((int)cx,(int)cy,(int)R, (Color){255,255,255,255});
    DrawCircleLines((int)cx,(int)cy,(int)(R*1.23f),(Color){200,160,80,90});
    HaloArc(cx, cy, R*1.23f, R*1.23f, -PI/2.0f, -PI/2.0f + prog*2.0f*PI, R*0.06f, (Color){232,184,74,255});
    if (done) {
        float q = (ph-0.85f)/0.15f;
        Star4(cx+R*0.62f, cy-R*0.62f, R*0.3f*sinf(q*PI), (Color){255,255,255,255});
    }
    EllipseLinesRot(cx, cy, R*0.4f, R, 0.0f, R*0.025f, (Color){200,160,80,150});
    EllipseRotF(cx-R*0.28f, cy-R*0.06f, R*0.10f, R*0.05f, 0.0f, (Color){138,112,64,255});
    EllipseRotF(cx+R*0.28f, cy-R*0.06f, R*0.10f, R*0.05f, 0.0f, (Color){138,112,64,255});
}

typedef void (*DrawFn)(float cx, float cy, float R, float t);

static DrawFn fns[NSKIN][NVAR] = {
    { Chronovore_V1,    Chronovore_V2,    Chronovore_V3,    Chronovore_V4,
      Chronovore_V5,    Chronovore_V6,    Chronovore_V7,    Chronovore_V8    },
    { Cartographer_V1,  Cartographer_V2,  Cartographer_V3,  Cartographer_V4,
      Cartographer_V5,  Cartographer_V6,  Cartographer_V7,  Cartographer_V8  },
    { Midas_V1,         Midas_V2,         Midas_V3,         Midas_V4,
      Midas_V5,         Midas_V6,         Midas_V7,         Midas_V8         },
    { BladeLegend_V1,          BladeLegend_V2,           BladeLegend_V3,
      BladeLegend_V6,          BladeLegend_Oni,          BladeLegend_Storm,
      BladeLegend_VoidReaper,  BladeLegend_Dragonfang,   BladeLegend_Gladiator,
      BladeLegend_Assassin,    BladeLegend_Tempest,      BladeLegend_SoulHarvester,
      BladeLegend_Obsidian,    BladeLegend_ThousandCuts, BladeLegend_WarGod,
      BladeLegend_Neon },
    { Constellation_V7,        Constellation_V8,         Constellation_Nebula,
      Constellation_Supernova, Constellation_AstralDragon, Constellation_Comet,
      Constellation_Orrery,    Constellation_GalaxyVeil, Constellation_Eclipse,
      Constellation_StarForge },
    { Ascendant_V1,     Ascendant_Seraph,   Ascendant_Apotheosis, Ascendant_Prism,
      Ascendant_Hierarch, Ascendant_Dawn,   Ascendant_Lotus,      Ascendant_Judgement,
      Ascendant_Flawless },
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

    // total variants actually wired up in fns[][] — used for the header count
    // and to keep rows with fewer than NVAR entries from indexing past NULL.
    int totalVariants = 0;
    for (int i=0;i<NSKIN;i++) for (int j=0;j<NVAR;j++) if (fns[i][j]) totalVariants++;

    const int sw = 1500, sh = 860;
    InitWindow(sw, sh, "Skin Design Picker — click a preview to reject it");
    SetTargetFPS(60);

    int leftMargin = 40;
    int gridLeft   = 210;
    int gridW      = sw - gridLeft - 30;
    int cols       = 4;
    int colW       = gridW / cols;
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

        // ── lay out each skin's band from its own visible-variant count ────
        // (variant counts differ per skin now, so band height can't be fixed)
        int visList[NSKIN][NVAR], visCount[NSKIN];
        int rowTop[NSKIN], rowBandH[NSKIN];
        int cum = headerH;
        for (int row = 0; row < NSKIN; row++) {
            int vc = 0;
            for (int v = 0; v < NVAR; v++) if (fns[row][v] && !bad[row][v]) visList[row][vc++] = v;
            visCount[row] = vc;
            int rows = (vc + cols - 1) / cols; if (rows < 1) rows = 1;
            rowBandH[row] = 66 + rows*100 + 20;
            rowTop[row] = cum;
            cum += rowBandH[row];
        }

        // ── count rejected + compute dynamic content height ────────────────
        int rejCount = 0;
        for (int i=0;i<NSKIN;i++) for (int j=0;j<NVAR;j++) if (fns[i][j] && bad[i][j]) rejCount++;
        int rejCols = 8;
        int rejRows = (rejCount + rejCols - 1) / rejCols; if (rejRows < 1) rejRows = 1;
        int rejBandH = 70 + rejRows*70 + 20;
        int rejBandTop = cum;
        int totalH = cum + rejBandH + 20;
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
        DrawText(TextFormat("%d / %d rejected", rejCount, totalVariants), leftMargin, 30, 13, (Color){170,120,120,220});

        BeginScissorMode(0, headerH, sw, sh-headerH);

        // ── 6 skin bands, bad/missing variants removed + remaining reflowed ──
        for (int row = 0; row < NSKIN; row++) {
            int bandH   = rowBandH[row];
            int bandTop = rowTop[row] - scroll;
            if (bandTop + bandH < headerH || bandTop > sh) continue;

            DrawText(TextFormat("%s  (%s)", skinMeta[row].name, skinMeta[row].hint),
                      leftMargin, bandTop + 14, 16, (Color){205,195,225,225});
            DrawLine(0, bandTop+bandH-4, sw, bandTop+bandH-4, (Color){40,36,55,255});

            for (int pos = 0; pos < visCount[row]; pos++) {
                int variant = visList[row][pos];
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
            int bandTop = rejBandTop - scroll;
            if (!(bandTop + rejBandH < headerH || bandTop > sh)) {
                DrawText("REJECTED  (click to restore)", leftMargin, bandTop + 14, 16, (Color){225,140,140,225});
                DrawLine(0, bandTop+rejBandH-2, sw, bandTop+rejBandH-2, (Color){40,36,55,255});

                if (rejCount == 0) {
                    DrawText("(none yet)", leftMargin, bandTop + 46, 14, (Color){110,105,130,200});
                } else {
                    int idx = 0;
                    for (int i = 0; i < NSKIN; i++) {
                        for (int j = 0; j < NVAR; j++) {
                            if (!fns[i][j] || !bad[i][j]) continue;
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
