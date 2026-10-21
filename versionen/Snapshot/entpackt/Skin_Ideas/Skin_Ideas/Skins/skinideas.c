// skinideas.c — flat interactive picker for player skin concepts.
// Fully independent of the main game (raylib only). No achievement
// categories — every design is just an entry in one scrollable list.
//
// Build (from this folder):
//   gcc -O2 -o skinideas.exe skinideas.c -IC:/raylib/raylib/src -LC:/raylib/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm
//
// Run from Skin_Ideas/ so skin_data.txt / bad_skins.txt / skin_picks.txt resolve.
//
// Left-click a preview ........ REJECT (logged to bad_skins.txt)
// Hover + P or right-click .... toggle PICK for the real game (skin_picks.txt)
// Click a rejected chip ....... restore it

#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

typedef unsigned char uchar;

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

// ═══════════════════════════════════════════════════════════════════════════

static Color ChronovoreColor(float gt) {
    float p = 0.5f + 0.5f*sinf(gt * 2.0f);
    return (Color){(uchar)(140+(int)(60*p)), (uchar)(100+(int)(50*p)), (uchar)(40+(int)(20*p)), 255};
}

static void Skin_AntiqueClock(float cx, float cy, float R, float t) {
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

static void Skin_TimeLoop(float cx, float cy, float R, float t) {
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

static void Skin_Blutklinge(float cx, float cy, float R, float t) {
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

static void Skin_BlackHole(float cx, float cy, float R, float t) {
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

// Copy of Black Hole pushed into a dramatic Interstellar-style lensing scene.
static void Skin_BlackHoleInterstellar(float cx, float cy, float R, float t) {
    float pulse = 0.5f + 0.5f * sinf(t * 2.0f);
    float spin = t * 0.55f;

    // Deep violet/orange corona and orbiting debris in the surrounding space.
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cy}, R * 1.82f, (Color){255, 76, 20, 16});
    DrawCircleV((Vector2){cx, cy}, R * 1.48f, (Color){88, 76, 255, 12});
    DrawRing((Vector2){cx, cy}, R * 1.30f, R * 1.57f,
             0.0f, 360.0f, 64, (Color){255, 64, 18, 32});
    DrawRing((Vector2){cx, cy}, R * 1.12f, R * 1.20f,
             0.0f, 360.0f, 64, (Color){106, 144, 255, 28});
    for (int k = 0; k < 18; k++) {
        float a = spin + Rnd(k) * 2.0f * PI;
        float d = R * (1.24f + Rnd(k + 18) * 0.78f);
        float px = cx + cosf(a) * d;
        float py = cy + sinf(a) * d * 0.54f;
        float size = R * (0.018f + Rnd(k + 36) * 0.035f);
        Color particle = (k % 4 == 0)
            ? (Color){155, 198, 255, (uchar)(95 + 55 * pulse)}
            : (Color){255, (uchar)(110 + 80 * Rnd(k + 54)), 36,
                       (uchar)(105 + 70 * pulse)};
        DrawCircleV((Vector2){px, py}, size, particle);
    }
    EndBlendMode();

    // A broad, tilted accretion disk sits behind the event horizon.
    EllipseRotF(cx, cy + R * 0.03f, R * 1.78f, R * 0.38f, -0.10f,
                (Color){255, 54, 14, (uchar)(48 + 24 * pulse)});
    EllipseRotF(cx, cy + R * 0.01f, R * 1.54f, R * 0.24f, -0.10f,
                (Color){255, 164, 42, (uchar)(92 + 32 * pulse)});

    BeginBlendMode(BLEND_ADDITIVE);
    // Hot layered disk contours.
    for (int k = 0; k < 5; k++) {
        float rx = R * (1.00f + 0.15f * k);
        float ry = R * (0.26f + 0.065f * k);
        Color disk = (Color){255, (uchar)(82 + 30 * k), (uchar)(18 + 8 * k),
                             (uchar)(170 - 20 * k + 25 * pulse)};
        EllipseLinesRot(cx, cy - R * 0.04f, rx, ry, -0.10f,
                        R * (0.052f - 0.006f * k), disk);
    }

    // Gravitational lensing bends the same disk far above and below the shadow.
    for (int k = 0; k < 4; k++) {
        float rx = R * (1.04f + 0.17f * k);
        float ry = R * (0.48f + 0.11f * k);
        HaloArc(cx, cy - R * 0.11f, rx, ry, PI + 0.10f, 2.0f * PI - 0.10f,
                R * (0.062f - 0.009f * k),
                (Color){255, (uchar)(168 + 18 * k), 64, (uchar)(185 - 25 * k)});
        HaloArc(cx, cy + R * 0.11f, rx, ry, 0.10f, PI - 0.10f,
                R * (0.068f - 0.009f * k),
                (Color){255, (uchar)(92 + 28 * k), 22, (uchar)(205 - 27 * k)});
    }

    // A second, much taller lensing canopy makes the disk read as 3D.
    for (int k = 0; k < 5; k++) {
        float q = k / 4.0f;
        float rx = R * (1.10f + 0.16f * q);
        float ry = R * (0.78f + 0.18f * q);
        float thick = R * (0.040f - 0.004f * q);
        Color canopy = (k % 2 == 0)
            ? (Color){255, (uchar)(205 - 18 * k), 92, (uchar)(120 - 12 * k)}
            : (Color){108, 176, 255, (uchar)(105 - 10 * k)};
        HaloArc(cx, cy - R * 0.08f, rx, ry, PI + 0.16f, 2.0f * PI - 0.16f,
                thick, canopy);
        HaloArc(cx, cy + R * 0.08f, rx, ry, 0.16f, PI - 0.16f,
                thick, canopy);
    }

    // Broken orbital ribbons sell the extreme curvature instead of a flat ring.
    for (int k = 0; k < 8; k++) {
        float q = Rnd(k + 126);
        float phase = spin * (0.32f + q * 0.35f) + k * 0.78f;
        float rx = R * (1.18f + q * 0.50f);
        float ry = R * (0.34f + q * 0.26f);
        float span = 0.44f + q * 0.28f;
        Color ribbon = (k % 3 == 0)
            ? (Color){155, 196, 255, (uchar)(95 + 55 * pulse)}
            : (Color){255, (uchar)(108 + 80 * q), 26, (uchar)(130 + 55 * pulse)};
        HaloArc(cx, cy, rx, ry, phase, phase + span,
                R * (0.025f + 0.012f * q), ribbon);
        HaloArc(cx, cy, rx, ry, phase + PI, phase + PI + span,
                R * (0.018f + 0.010f * q), ribbon);
    }

    // Fast-moving plasma streaks give the disk a living, violent texture.
    for (int k = 0; k < 10; k++) {
        float q = Rnd(k + 72);
        float a = spin * 0.7f + q * 2.0f * PI;
        float d0 = R * (1.08f + q * 0.28f);
        float d1 = d0 + R * (0.12f + Rnd(k + 90) * 0.18f);
        Vector2 p0 = {cx + cosf(a) * d0, cy + sinf(a) * d0 * 0.24f};
        Vector2 p1 = {cx + cosf(a) * d1, cy + sinf(a) * d1 * 0.24f};
        DrawLineEx(p0, p1, R * (0.018f + 0.018f * Rnd(k + 108)),
                   (Color){255, (uchar)(120 + 85 * q), 34, (uchar)(90 + 80 * pulse)});
    }
    EndBlendMode();

    // The event horizon is almost perfectly black, with a violet outer rim.
    DrawCircleV((Vector2){cx, cy}, R * 1.00f, (Color){22, 10, 28, 230});
    DrawCircleGradient((Vector2){cx, cy}, R * 0.92f,
                       (Color){11, 8, 18, 255}, (Color){0, 0, 2, 255});
    DrawCircleV((Vector2){cx, cy}, R * 0.78f, (Color){0, 0, 1, 255});

    BeginBlendMode(BLEND_ADDITIVE);
    // Photon ring: a razor-thin white-gold edge with a blue gravitational rim.
    EllipseLinesRot(cx, cy, R * 0.93f, R * 0.93f, 0.0f, R * 0.065f,
                    (Color){255, 152, 54, (uchar)(180 + 50 * pulse)});
    EllipseLinesRot(cx, cy, R * 0.89f, R * 0.89f, 0.0f, R * 0.022f,
                    (Color){190, 220, 255, (uchar)(145 + 55 * pulse)});
    HaloArc(cx, cy - R * 0.02f, R * 1.08f, R * 0.66f,
            PI + 0.28f, 2.0f * PI - 0.28f, R * 0.045f,
            (Color){236, 246, 255, (uchar)(155 + 60 * pulse)});

    // Foreground disk crossing the shadow, as in the iconic silhouette.
    HaloArc(cx, cy + R * 0.12f, R * 1.52f, R * 0.23f,
            0.08f, PI - 0.08f, R * 0.12f,
            (Color){255, 92, 18, (uchar)(205 + 45 * pulse)});
    HaloArc(cx, cy + R * 0.12f, R * 1.34f, R * 0.15f,
            0.10f, PI - 0.10f, R * 0.040f,
            (Color){255, 242, 184, (uchar)(220 + 35 * pulse)});
    HaloArc(cx, cy + R * 0.12f, R * 1.43f, R * 0.18f,
            0.12f, PI - 0.12f, R * 0.020f,
            (Color){255, 255, 242, (uchar)(220 + 35 * pulse)});

    // A rotating relativistic hotspot flares along the front edge.
    float hotA = spin * 1.8f;
    float hx = cx + cosf(hotA) * R * 1.34f;
    float hy = cy + sinf(hotA) * R * 0.22f + R * 0.12f;
    for (int k = 4; k >= 1; k--) {
        float trailA = hotA - k * 0.13f;
        float tx = cx + cosf(trailA) * R * (1.34f - k * 0.035f);
        float ty = cy + sinf(trailA) * R * 0.22f + R * 0.12f;
        DrawCircleV((Vector2){tx, ty}, R * (0.025f + 0.012f * (5 - k)),
                    (Color){255, (uchar)(92 + k * 18), 30, (uchar)(75 + 20 * pulse)});
    }
    DrawCircleV((Vector2){hx, hy}, R * 0.17f,
                (Color){255, 70, 12, (uchar)(55 + 45 * pulse)});
    DrawCircleV((Vector2){hx, hy}, R * 0.065f,
                (Color){255, 248, 202, (uchar)(210 + 45 * pulse)});
    Star4(hx, hy, R * 0.065f, (Color){255, 232, 166, 220});
    EndBlendMode();
}

static void Skin_Apotheosis(float cx, float cy, float R, float t) {
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

static void Skin_HaloHierarch(float cx, float cy, float R, float t) {
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

static void Skin_Dawnbringer(float cx, float cy, float R, float t) {
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


// ═══════════════════════════════════════════════════════════════════════════
// NEW IDEAS — dense animation, clear silhouettes, not recolors
// ═══════════════════════════════════════════════════════════════════════════

static void Skin_AbyssalEye(float cx, float cy, float R, float t) {
    // A living void-eye: iris rotates, tentacles writhe, pupil dilates.
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cy}, R*1.45f, (Color){80, 20, 120, 30});
    EndBlendMode();
    for (int k = 0; k < 8; k++) {
        float a = t*0.9f + k*(2.0f*PI/8.0f);
        float wob = sinf(t*2.5f + k)*R*0.18f;
        Vector2 root = { cx + cosf(a)*R*0.85f, cy + sinf(a)*R*0.85f };
        Vector2 mid  = { cx + cosf(a+0.2f)*(R*1.25f+wob), cy + sinf(a+0.2f)*(R*1.25f+wob*0.6f) };
        Vector2 tip  = { cx + cosf(a-0.15f)*(R*1.65f), cy + sinf(a-0.15f)*(R*1.55f) };
        DrawLineEx(root, mid, R*0.14f, (Color){40, 18, 55, 220});
        DrawLineEx(mid, tip, R*0.09f, (Color){60, 25, 80, 200});
        DrawCircleV(tip, R*0.07f, (Color){180, 60, 220, 230});
    }
    DrawCircle((int)cx, (int)cy, (int)R, (Color){18, 10, 28, 255});
    float dil = 0.55f + 0.15f*sinf(t*1.8f);
    DrawCircleV((Vector2){cx, cy}, R*0.72f, (Color){40, 12, 70, 255});
    // iris wedges
    for (int k = 0; k < 12; k++) {
        float a0 = t*0.6f + k*(2.0f*PI/12.0f);
        float a1 = a0 + 0.22f;
        Color ic = ColorFromHSV(fmodf(280.0f + k*8.0f + t*20.0f, 360.0f), 0.7f, 0.95f);
        DrawTriangle((Vector2){cx, cy},
                     (Vector2){cx+cosf(a0)*R*0.62f, cy+sinf(a0)*R*0.62f},
                     (Vector2){cx+cosf(a1)*R*0.62f, cy+sinf(a1)*R*0.62f},
                     (Color){ic.r, ic.g, ic.b, 200});
    }
    DrawCircleV((Vector2){cx, cy}, R*0.28f*dil, (Color){0, 0, 0, 255});
    DrawCircleV((Vector2){cx - R*0.08f, cy - R*0.10f}, R*0.07f, (Color){255, 255, 255, 200});
}

static void Skin_PhoenixPlume(float cx, float cy, float R, float t) {
    // Rising firebird — wing fans beat, embers rain, core is white-hot.
    float beat = 0.5f + 0.5f*sinf(t*3.2f);
    for (int side = -1; side <= 1; side += 2) {
        for (int i = 0; i < 5; i++) {
            float spread = 0.35f + i*0.22f + beat*0.15f;
            float len = R*(1.1f + i*0.18f);
            Vector2 tip = { cx + side*sinf(spread)*len, cy - cosf(spread)*len*0.85f - R*0.1f };
            Color fc = (i%2) ? (Color){255, 90, 20, 210} : (Color){255, 180, 40, 200};
            DrawTriangle((Vector2){cx, cy}, tip,
                         (Vector2){cx + side*sinf(spread-0.12f)*len*0.7f, cy - cosf(spread-0.12f)*len*0.6f},
                         fc);
        }
    }
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cy}, R*1.35f, (Color){255, 100, 20, (uchar)(25+20*beat)});
    EndBlendMode();
    DrawCircle((int)cx, (int)cy, (int)R, (Color){220, 60, 20, 255});
    DrawCircleV((Vector2){cx, cy}, R*0.55f, (Color){255, 200, 80, 255});
    DrawCircleV((Vector2){cx, cy}, R*0.25f, (Color){255, 250, 220, 255});
    for (int k = 0; k < 10; k++) {
        float p = fmodf(t*0.55f + Rnd(k), 1.0f);
        DrawCircleV((Vector2){cx+(Rnd(k+3)-0.5f)*R*2.2f, cy+R*0.8f - p*R*2.6f},
                    R*0.05f, (Color){255, 160, 50, (uchar)(220*sinf(p*PI))});
    }
    EllipseRotF(cx-R*0.28f, cy-R*0.05f, R*0.10f, R*0.06f, 0.0f, (Color){40, 10, 5, 255});
    EllipseRotF(cx+R*0.28f, cy-R*0.05f, R*0.10f, R*0.06f, 0.0f, (Color){40, 10, 5, 255});
}

static void Skin_CrystalPrism(float cx, float cy, float R, float t) {
    // Faceted crystal that splits a white beam into a rainbow fan.
    DrawLineEx((Vector2){cx-R*2.0f, cy-R*1.5f}, (Vector2){cx-R*0.15f, cy-R*0.1f},
               R*0.10f, (Color){255, 255, 255, 150});
    Color rb[6] = {
        {255,80,80,200},{255,160,50,200},{255,230,70,200},
        {80,220,100,200},{80,160,255,200},{160,90,255,200}
    };
    for (int i = 0; i < 6; i++) {
        float ang = 0.25f + i*0.14f + 0.04f*sinf(t*2.0f+i);
        DrawLineEx((Vector2){cx+R*0.15f, cy+R*0.1f},
                   (Vector2){cx+R*0.15f+cosf(ang)*R*2.0f, cy+R*0.1f+sinf(ang)*R*2.0f},
                   R*0.08f, rb[i]);
    }
    // diamond body
    DrawTriangle((Vector2){cx, cy-R}, (Vector2){cx-R*0.85f, cy}, (Vector2){cx+R*0.85f, cy},
                 (Color){220, 240, 255, 240});
    DrawTriangle((Vector2){cx, cy+R}, (Vector2){cx-R*0.85f, cy}, (Vector2){cx+R*0.85f, cy},
                 (Color){160, 200, 240, 230});
    float tw = fabsf(sinf(t*2.8f));
    Star4(cx+R*0.35f, cy-R*0.35f, R*(0.12f+0.12f*tw), (Color){255, 255, 255, 255});
    DrawCircleV((Vector2){cx-R*0.25f, cy-R*0.05f}, R*0.07f, (Color){40, 70, 110, 255});
    DrawCircleV((Vector2){cx+R*0.25f, cy-R*0.05f}, R*0.07f, (Color){40, 70, 110, 255});
}

static void Skin_VoidSerpent(float cx, float cy, float R, float t) {
    // Body is a dark disc; a neon serpent coils around it and bites its tail.
    DrawCircleV((Vector2){cx, cy}, R*1.4f, (Color){80, 20, 120, 20});
    DrawCircle((int)cx, (int)cy, (int)R, (Color){22, 12, 35, 255});
    int segs = 22;
    Vector2 prev = {0};
    for (int i = 0; i <= segs; i++) {
        float u = (float)i / segs;
        float a = u*2.0f*PI*1.6f + t*1.4f;
        float rad = R*(0.75f + 0.55f*u);
        Vector2 p = { cx + cosf(a)*rad, cy + sinf(a)*rad*0.85f };
        Color sc = ColorFromHSV(fmodf(280.0f + u*40.0f + t*30.0f, 360.0f), 0.75f, 1.0f);
        if (i > 0) DrawLineEx(prev, p, R*(0.16f - u*0.06f), (Color){sc.r, sc.g, sc.b, 230});
        prev = p;
    }
    // head
    float ha = 1.6f*2.0f*PI + t*1.4f;
    Vector2 head = { cx + cosf(ha)*R*1.3f, cy + sinf(ha)*R*1.1f };
    DrawCircleV(head, R*0.18f, (Color){200, 100, 255, 255});
    DrawCircleV((Vector2){head.x + cosf(ha)*R*0.08f, head.y + sinf(ha)*R*0.08f},
                R*0.05f, (Color){255, 255, 100, 255});
    EllipseRotF(cx-R*0.26f, cy-R*0.05f, R*0.10f, R*0.06f, 0.0f, (Color){180, 80, 255, 255});
    EllipseRotF(cx+R*0.26f, cy-R*0.05f, R*0.10f, R*0.06f, 0.0f, (Color){180, 80, 255, 255});
}




static void Skin_EclipseWarden(float cx, float cy, float R, float t) {
    // Black disc eclipsing a star — corona flares, diamond-ring flash.
    float phase = fmodf(t*0.25f, 1.0f);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cy}, R*1.55f, (Color){255, 160, 40, 30});
    // corona spikes
    for (int k = 0; k < 16; k++) {
        float a = k*(2.0f*PI/16.0f) + t*0.2f;
        float len = R*(1.25f + 0.35f*sinf(t*3.0f+k));
        DrawLineEx((Vector2){cx+cosf(a)*R*0.95f, cy+sinf(a)*R*0.95f},
                   (Vector2){cx+cosf(a)*len, cy+sinf(a)*len},
                   R*0.07f, (Color){255, 140, 30, 160});
    }
    EndBlendMode();
    // bright star behind offset
    float ox = cosf(t*0.4f)*R*0.15f;
    DrawCircle((int)(cx+ox), (int)cy, (int)(R*0.95f), (Color){255, 220, 120, 255});
    // dark moon
    DrawCircle((int)cx, (int)cy, (int)R, (Color){12, 10, 18, 255});
    // diamond ring flash
    if (phase > 0.85f) {
        float q = (phase-0.85f)/0.15f;
        DrawRing((Vector2){cx, cy}, R*0.95f, R*1.15f, 0, 360, 40,
                 (Color){255, 255, 255, (uchar)(200*sinf(q*PI))});
    }
    EllipseRotF(cx-R*0.28f, cy-R*0.05f, R*0.09f, R*0.05f, 0.0f, (Color){255, 200, 100, 255});
    EllipseRotF(cx+R*0.28f, cy-R*0.05f, R*0.09f, R*0.05f, 0.0f, (Color){255, 200, 100, 255});
}

// ─── NEW: Blade Legend rework + Warrior + Skull ideas ────────────────────────

// ─── SKULL (matches in-game DrawSkull silhouette) + animated variants ─────────

static void Ideas_SkullShape(float cx, float cy, float R, Color col) {
    static const float CONT[][2] = {
        { 0.00f, -1.05f }, { 0.42f, -0.99f }, { 0.72f, -0.82f },
        { 0.91f, -0.55f }, { 1.00f, -0.22f }, { 1.01f,  0.10f },
        { 0.90f,  0.34f }, { 0.72f,  0.52f },
        { 0.64f,  0.72f }, { 0.60f,  0.95f }, { 0.50f,  1.12f },
        { 0.30f,  1.21f }, { 0.00f,  1.23f },
    };
    int n = (int)(sizeof(CONT) / sizeof(CONT[0]));
    Vector2 ctrl[40];
    int m = 0;
    for (int i = 0; i < n; i++) ctrl[m++] = (Vector2){ cx + CONT[i][0]*R, cy + CONT[i][1]*R };
    for (int i = n-2; i >= 1; i--) ctrl[m++] = (Vector2){ cx - CONT[i][0]*R, cy + CONT[i][1]*R };

    Vector2 c = { cx, cy };
    Vector2 first = {0}, prev = {0};
    int started = 0;
    const int SUB = 8;
    for (int i = 0; i < m; i++) {
        Vector2 p0 = ctrl[(i - 1 + m) % m], p1 = ctrl[i];
        Vector2 p2 = ctrl[(i + 1) % m],     p3 = ctrl[(i + 2) % m];
        for (int s = 0; s < SUB; s++) {
            float u = (float)s / SUB, u2 = u*u, u3 = u2*u;
            Vector2 q = {
                0.5f*((2*p1.x) + (-p0.x+p2.x)*u + (2*p0.x-5*p1.x+4*p2.x-p3.x)*u2 + (-p0.x+3*p1.x-3*p2.x+p3.x)*u3),
                0.5f*((2*p1.y) + (-p0.y+p2.y)*u + (2*p0.y-5*p1.y+4*p2.y-p3.y)*u2 + (-p0.y+3*p1.y-3*p2.y+p3.y)*u3)
            };
            if (started) { DrawTriangle(c, q, prev, col); DrawTriangle(c, prev, q, col); }
            else { first = q; started = 1; }
            prev = q;
        }
    }
    DrawTriangle(c, first, prev, col);
    DrawTriangle(c, prev, first, col);
}

// Base = in-game skull. bob + eyePulse animate without changing silhouette language.
static void Ideas_SkullBase(float cx, float cy, float R, float t,
                            Color bone, Color boneLo, Color boneHi,
                            Color socketFill, Color eyeGlow, float eyePulse) {
    float bob = sinf(t * 2.2f) * R * 0.04f;
    float cyb = cy + bob;
    float breath = 0.5f + 0.5f * sinf(t * 1.8f);

    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cyb}, R * (1.25f + 0.06f * breath),
                (Color){eyeGlow.r, eyeGlow.g, eyeGlow.b, (uchar)(12 + 10 * breath)});
    EndBlendMode();

    Ideas_SkullShape(cx, cyb + R*0.03f, R*1.03f, boneLo);
    Ideas_SkullShape(cx, cyb, R, bone);

    for (int i = 0; i < 4; i++) {
        float k = 1.0f - i*0.18f;
        EllipseRotF(cx - R*0.10f, cyb - R*0.46f, R*0.58f*k, R*0.44f*k, 0.0f,
                        (Color){boneHi.r, boneHi.g, boneHi.b, 42});
    }
    EllipseRotF(cx, cyb + R*0.58f, R*0.52f, R*0.26f, 0.0f,
                    (Color){boneLo.r, boneLo.g, boneLo.b, 110});

    EllipseRotF(cx - R*0.44f, cyb - R*0.07f, R*0.48f, R*0.42f,  0.32f, (Color){150,144,132,120});
    EllipseRotF(cx + R*0.44f, cyb - R*0.07f, R*0.48f, R*0.42f, -0.32f, (Color){150,144,132,120});
    EllipseRotF(cx - R*0.44f, cyb - R*0.08f, R*0.42f, R*0.36f,  0.32f, socketFill);
    EllipseRotF(cx + R*0.44f, cyb - R*0.08f, R*0.42f, R*0.36f, -0.32f, socketFill);

    float er = R * (0.10f + 0.05f * eyePulse);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx - R*0.40f, cyb - R*0.04f}, er,
                (Color){eyeGlow.r, eyeGlow.g, eyeGlow.b, (uchar)(90 + 100 * eyePulse)});
    DrawCircleV((Vector2){cx + R*0.40f, cyb - R*0.04f}, er,
                (Color){eyeGlow.r, eyeGlow.g, eyeGlow.b, (uchar)(90 + 100 * eyePulse)});
    DrawCircleV((Vector2){cx - R*0.40f, cyb - R*0.04f}, er * 0.45f,
                (Color){255, 255, 255, (uchar)(120 + 80 * eyePulse)});
    DrawCircleV((Vector2){cx + R*0.40f, cyb - R*0.04f}, er * 0.45f,
                (Color){255, 255, 255, (uchar)(120 + 80 * eyePulse)});
    EndBlendMode();
    DrawCircleV((Vector2){cx - R*0.34f, cyb + R*0.10f}, R*0.05f, (Color){55,51,53,150});
    DrawCircleV((Vector2){cx + R*0.34f, cyb + R*0.10f}, R*0.05f, (Color){55,51,53,150});

    Color dark = (Color){ 24, 20, 21, 255 };
    DrawTriangle((Vector2){cx,            cyb + R*0.44f},
                 (Vector2){cx - R*0.15f,  cyb + R*0.20f},
                 (Vector2){cx + R*0.15f,  cyb + R*0.20f}, dark);
    DrawTriangle((Vector2){cx,            cyb + R*0.44f},
                 (Vector2){cx + R*0.15f,  cyb + R*0.20f},
                 (Vector2){cx - R*0.15f,  cyb + R*0.20f}, dark);
    DrawCircleV((Vector2){cx - R*0.08f, cyb + R*0.22f}, R*0.075f, dark);
    DrawCircleV((Vector2){cx + R*0.08f, cyb + R*0.22f}, R*0.075f, dark);

    float chatter = sinf(t * 12.0f) * R * 0.012f;
    float mx = cx - R*0.46f, mouthW = R*0.92f;
    float myT = cyb + R*0.62f + chatter, myB = cyb + R*1.00f + chatter;
    DrawRectangleRounded((Rectangle){mx, myT, mouthW, myB - myT}, 0.35f, 6, dark);
    int nt = 6;
    float tw = mouthW / nt;
    for (int k = 0; k < nt; k++) {
        float tx = mx + k*tw;
        DrawRectangleRounded(
            (Rectangle){tx + R*0.03f, myT + R*0.03f, tw - R*0.06f, (myB - myT) - R*0.06f},
            0.30f, 4, boneHi);
    }
}

// Purple-only skull base for the crowned void variant.  This is intentionally
// separate from Ideas_SkullBase: its socket rim, cheek shadow and teeth all use
// amethyst values so no bone/grey surface leaks through the completely lilac
// fantasy requested for this skin.
static void Ideas_PurpleSkullBase(float cx, float cy, float R, float t,
                                  Color violet, Color violetLo, Color violetHi,
                                  Color socketFill, Color eyeGlow, float eyePulse) {
    float bob = sinf(t * 2.2f) * R * 0.04f;
    float cyb = cy + bob;
    float breath = 0.5f + 0.5f * sinf(t * 1.8f);

    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cyb}, R * (1.30f + 0.07f * breath),
                (Color){eyeGlow.r, eyeGlow.g, eyeGlow.b, (uchar)(20 + 16 * breath)});
    EndBlendMode();

    Ideas_SkullShape(cx, cyb + R * 0.03f, R * 1.03f, violetLo);
    Ideas_SkullShape(cx, cyb, R, violet);

    for (int i = 0; i < 4; i++) {
        float k = 1.0f - i * 0.18f;
        EllipseRotF(cx - R * 0.10f, cyb - R * 0.46f,
                    R * 0.58f * k, R * 0.44f * k, 0.0f,
                    (Color){violetHi.r, violetHi.g, violetHi.b, 48});
    }
    EllipseRotF(cx, cyb + R * 0.58f, R * 0.52f, R * 0.26f, 0.0f,
                (Color){violetLo.r, violetLo.g, violetLo.b, 150});

    Color socketRim = (Color){100, 32, 160, 210};
    EllipseRotF(cx - R * 0.44f, cyb - R * 0.07f,
                R * 0.48f, R * 0.42f, 0.32f, socketRim);
    EllipseRotF(cx + R * 0.44f, cyb - R * 0.07f,
                R * 0.48f, R * 0.42f, -0.32f, socketRim);
    EllipseRotF(cx - R * 0.44f, cyb - R * 0.08f,
                R * 0.42f, R * 0.36f, 0.32f, socketFill);
    EllipseRotF(cx + R * 0.44f, cyb - R * 0.08f,
                R * 0.42f, R * 0.36f, -0.32f, socketFill);

    float er = R * (0.11f + 0.06f * eyePulse);
    BeginBlendMode(BLEND_ADDITIVE);
    for (int side = -1; side <= 1; side += 2) {
        float ex = cx + side * R * 0.40f;
        DrawCircleV((Vector2){ex, cyb - R * 0.04f}, er,
                    (Color){eyeGlow.r, eyeGlow.g, eyeGlow.b, (uchar)(110 + 110 * eyePulse)});
        DrawCircleV((Vector2){ex, cyb - R * 0.04f}, er * 0.42f,
                    (Color){255, 220, 255, (uchar)(150 + 80 * eyePulse)});
    }
    EndBlendMode();

    Color mouth = (Color){18, 4, 34, 255};
    DrawTriangle((Vector2){cx, cyb + R * 0.44f},
                 (Vector2){cx - R * 0.15f, cyb + R * 0.20f},
                 (Vector2){cx + R * 0.15f, cyb + R * 0.20f}, mouth);
    DrawTriangle((Vector2){cx, cyb + R * 0.44f},
                 (Vector2){cx + R * 0.15f, cyb + R * 0.20f},
                 (Vector2){cx - R * 0.15f, cyb + R * 0.20f}, mouth);
    float chatter = sinf(t * 12.0f) * R * 0.012f;
    float mx = cx - R * 0.46f, mouthW = R * 0.92f;
    float myT = cyb + R * 0.62f + chatter, myB = cyb + R * 1.00f + chatter;
    DrawRectangleRounded((Rectangle){mx, myT, mouthW, myB - myT}, 0.35f, 6, mouth);
    for (int k = 0; k < 6; k++) {
        float tx = mx + k * (mouthW / 6.0f);
        DrawRectangleRounded(
            (Rectangle){tx + R * 0.03f, myT + R * 0.03f,
                        mouthW / 6.0f - R * 0.06f, (myB - myT) - R * 0.06f},
            0.30f, 4, (Color){220, 175, 255, 255});
    }
}

static void Skin_SkullClassic(float cx, float cy, float R, float t) {
    float pulse = 0.5f + 0.5f * sinf(t * 3.2f);
    Ideas_SkullBase(cx, cy, R, t,
        (Color){231, 226, 212, 255}, (Color){193, 187, 172, 255}, (Color){247, 244, 235, 255},
        (Color){ 13,  11,  13, 255}, (Color){200, 190, 170, 255}, pulse * 0.35f);
}

static void Skin_SkullEmber(float cx, float cy, float R, float t) {
    float pulse = 0.5f + 0.5f * sinf(t * 4.0f);
    Ideas_SkullBase(cx, cy, R, t,
        (Color){231, 226, 212, 255}, (Color){193, 187, 172, 255}, (Color){247, 244, 235, 255},
        (Color){ 20,  10,   8, 255}, (Color){255, 110,  30, 255}, pulse);
    float bob = sinf(t * 2.2f) * R * 0.04f;
    for (int k = 0; k < 6; k++) {
        float p = fmodf(t * 0.7f + Rnd(k + 3), 1.0f);
        float side = (k % 2) ? 1.0f : -1.0f;
        DrawCircleV((Vector2){cx + side * R * 0.40f + (Rnd(k)-0.5f)*R*0.1f,
                              cy + bob - R*0.05f - p * R * 1.0f},
                    R * (0.05f - p * 0.03f),
                    (Color){255, 140, 40, (uchar)(200 * (1.0f - p))});
    }
}

static void Skin_SkullVoid(float cx, float cy, float R, float t) {
    float pulse = 0.5f + 0.5f * sinf(t * 3.6f);
    Ideas_SkullBase(cx, cy, R, t,
        (Color){220, 215, 230, 255}, (Color){170, 165, 185, 255}, (Color){245, 242, 255, 255},
        (Color){  8,   4,  16, 255}, (Color){170,  80, 255, 255}, pulse);
    float bob = sinf(t * 2.2f) * R * 0.04f;
    for (int k = 0; k < 8; k++) {
        float a = t * 1.4f + k * (2.0f * PI / 8.0f);
        float d = R * (1.15f + 0.08f * sinf(t * 2.5f + k));
        DrawCircleV((Vector2){cx + cosf(a) * d, cy + bob + sinf(a) * d * 0.55f},
                    R * 0.04f, (Color){180, 100, 255, 160});
    }
}

static void Skin_SkullCrowned(float cx, float cy, float R, float t) {
    float pulse = 0.5f + 0.5f * sinf(t * 2.8f);
    Ideas_SkullBase(cx, cy, R, t,
        (Color){231, 226, 212, 255}, (Color){193, 187, 172, 255}, (Color){247, 244, 235, 255},
        (Color){ 18,  12,  12, 255}, (Color){255,  70,  50, 255}, pulse * 0.85f);
    float bob = sinf(t * 2.2f) * R * 0.04f;
    float cy0 = cy + bob - R * (1.18f + 0.05f * sinf(t * 2.0f));
    DrawRectangleRounded((Rectangle){cx - R*0.62f, cy0, R*1.24f, R*0.16f}, 0.4f, 6,
                         (Color){210, 165, 40, 245});
    for (int k = -2; k <= 2; k++) {
        float hx = cx + k * R * 0.24f;
        float hlen = R * (0.22f + (k == 0 ? 0.14f : 0.04f) + 0.03f * sinf(t * 3.0f + k));
        DrawTriangle((Vector2){hx - R*0.08f, cy0},
                     (Vector2){hx + R*0.08f, cy0},
                     (Vector2){hx, cy0 - hlen},
                     (k == 0) ? (Color){255, 220, 90, 255} : (Color){200, 150, 35, 255});
    }
}

// ─── SKULLVOID CROWNED — complete amethyst skull with an imperial crown ──────
static void Skin_SkullVoidCrowned(float cx, float cy, float R, float t) {
    float pulse = 0.5f + 0.5f * sinf(t * 3.4f);
    Ideas_PurpleSkullBase(cx, cy, R, t,
        (Color){112, 34, 190, 255}, (Color){54, 14, 106, 255},
        (Color){226, 170, 255, 255}, (Color){18, 3, 42, 255},
        (Color){240, 75, 255, 255}, pulse);

    float bob = sinf(t * 2.2f) * R * 0.04f;
    float crownY = cy + bob - R * (1.22f + 0.04f * sinf(t * 2.0f));
    Color crownDark = (Color){78, 18, 130, 255};
    Color crown = (Color){180, 55, 240, 255};
    Color crownHi = (Color){240, 170, 255, 255};

    BeginBlendMode(BLEND_ADDITIVE);
    DrawRectangleRounded((Rectangle){cx - R * 0.72f, crownY - R * 0.06f,
                                    R * 1.44f, R * 0.34f},
                         0.28f, 6, (Color){205, 55, 255, (uchar)(35 + 35 * pulse)});
    EndBlendMode();
    DrawRectangleRounded((Rectangle){cx - R * 0.64f, crownY,
                                    R * 1.28f, R * 0.22f},
                         0.30f, 6, crownDark);
    DrawRectangleRoundedLines((Rectangle){cx - R * 0.64f, crownY,
                                          R * 1.28f, R * 0.22f},
                              0.30f, 6, crownHi);
    for (int k = -2; k <= 2; k++) {
        float x = cx + k * R * 0.27f;
        float h = R * (0.34f + (k == 0 ? 0.18f : 0.0f))
                  + sinf(t * 2.8f + k) * R * 0.03f;
        DrawTriangle((Vector2){x - R * 0.13f, crownY},
                     (Vector2){x + R * 0.13f, crownY},
                     (Vector2){x, crownY - h}, crown);
        DrawLineEx((Vector2){x, crownY - h + R * 0.05f},
                   (Vector2){x, crownY - R * 0.04f}, R * 0.025f, crownHi);
        DrawCircleV((Vector2){x, crownY - h}, R * 0.075f,
                    k == 0 ? (Color){255, 90, 235, 255} : (Color){120, 220, 255, 235});
    }

    // Orbiting void motes and a slow violet crown halo distinguish this from
    // the existing bone Skull Void and the bone Skull Crowned.
    DrawRing((Vector2){cx, cy + bob - R * 0.72f}, R * 1.12f, R * 1.18f,
             202.0f, 338.0f, 28, (Color){205, 80, 255, 155});
    for (int k = 0; k < 10; k++) {
        float a = t * 1.25f + k * (2.0f * PI / 10.0f);
        float d = R * (1.18f + 0.10f * sinf(t * 2.1f + k));
        DrawCircleV((Vector2){cx + cosf(a) * d,
                              cy + bob - R * 0.15f + sinf(a) * d * 0.58f},
                    R * (0.025f + 0.02f * pulse),
                    (Color){220, 105, 255, (uchar)(130 + 90 * pulse)});
    }
}

static void Skin_SkullCrimson(float cx, float cy, float R, float t) {
    float pulse = 0.5f + 0.5f * sinf(t * 4.5f);
    Ideas_SkullBase(cx, cy, R, t,
        (Color){235, 210, 200, 255}, (Color){190, 160, 150, 255}, (Color){250, 230, 220, 255},
        (Color){ 18,   6,   8, 255}, (Color){255,  35,  45, 255}, pulse);
}

static void Skin_ApexBladelord(float cx, float cy, float R, float t) {
    float pulse = 0.5f + 0.5f * sinf(t * 5.0f);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cy}, R * (1.55f + 0.12f * pulse),
                (Color){255, 50, 20, (uchar)(28 + 22 * pulse)});
    DrawCircleV((Vector2){cx, cy}, R * 1.25f, (Color){255, 120, 30, 40});
    EndBlendMode();
    for (int s = -1; s <= 1; s += 2) {
        float a = s * (0.55f + 0.12f * sinf(t * 2.2f));
        float ca = cosf(a), sa = sinf(a);
        Vector2 tip  = { cx + ca * R * 1.55f, cy + sa * R * 1.55f - R * 0.15f };
        Vector2 hilt = { cx - ca * R * 0.85f, cy - sa * R * 0.85f + R * 0.10f };
        DrawLineEx(hilt, tip, R * 0.26f, (Color){45, 42, 55, 255});
        DrawLineEx(hilt, tip, R * 0.12f, (Color){200, 205, 220, 255});
        BeginBlendMode(BLEND_ADDITIVE);
        DrawLineEx(hilt, tip, R * 0.05f, (Color){255, 90, 40, (uchar)(160 + 60 * pulse)});
        EndBlendMode();
        Vector2 mid = { hilt.x * 0.35f + tip.x * 0.65f, hilt.y * 0.35f + tip.y * 0.65f };
        Vector2 perp = { -sa, ca };
        DrawLineEx((Vector2){mid.x - perp.x * R * 0.28f, mid.y - perp.y * R * 0.28f},
                   (Vector2){mid.x + perp.x * R * 0.28f, mid.y + perp.y * R * 0.28f},
                   R * 0.08f, (Color){255, 180, 60, 240});
        DrawCircleV(tip, R * 0.08f, (Color){255, 220, 120, 230});
    }
    for (int k = 0; k < 5; k++) {
        float a = t * 1.8f + k * (2.0f * PI / 5.0f);
        float d = R * (1.35f + 0.08f * sinf(t * 3.0f + k));
        float bx = cx + cosf(a) * d, by = cy + sinf(a) * d * 0.72f;
        DrawLineEx((Vector2){bx - cosf(a) * R * 0.18f, by - sinf(a) * R * 0.18f},
                   (Vector2){bx + cosf(a) * R * 0.18f, by + sinf(a) * R * 0.18f},
                   R * 0.06f, (Color){255, 100, 40, 200});
    }
    DrawCircle((int)cx, (int)cy, (int)R, (Color){120, 18, 22, 255});
    DrawCircleV((Vector2){cx, cy}, R * 0.62f, (Color){180, 30, 30, 160});
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cy}, R * (0.38f + 0.06f * pulse),
                (Color){255, 90, 30, (uchar)(90 + 50 * pulse)});
    EndBlendMode();
    DrawRectangle((int)(cx - R * 0.42f), (int)(cy - R * 0.12f), (int)(R * 0.28f), (int)(R * 0.10f),
                  (Color){255, 220, 60, 255});
    DrawRectangle((int)(cx + R * 0.14f), (int)(cy - R * 0.12f), (int)(R * 0.28f), (int)(R * 0.10f),
                  (Color){255, 220, 60, 255});
    DrawLineEx((Vector2){cx - R * 0.55f, cy + R * 0.22f}, (Vector2){cx - R * 0.15f, cy + R * 0.38f},
               R * 0.07f, (Color){40, 8, 10, 230});
    DrawLineEx((Vector2){cx + R * 0.15f, cy + R * 0.38f}, (Vector2){cx + R * 0.55f, cy + R * 0.22f},
               R * 0.07f, (Color){40, 8, 10, 230});
}

// Copy of Apex Bladelord with one animated sword carried on each side.
static void Skin_ApexBladelordSides(float cx, float cy, float R, float t) {
    float pulse = 0.5f + 0.5f * sinf(t * 5.0f);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cy}, R * (1.55f + 0.12f * pulse),
                (Color){255, 50, 20, (uchar)(28 + 22 * pulse)});
    DrawCircleV((Vector2){cx, cy}, R * 1.25f, (Color){255, 120, 30, 40});
    EndBlendMode();

    for (int s = -1; s <= 1; s += 2) {
        float side = (float)s;
        float a = 0.55f + 0.12f * sinf(t * 2.2f);
        float ca = cosf(a), sa = sinf(a);
        // Keep both blades on the upper, central back; the body hides their lower roots.
        Vector2 tip  = { cx + side * ca * R * 1.38f, cy - sa * R * 1.38f - R * 0.22f };
        Vector2 hilt = { cx + side * ca * R * 0.52f, cy + R * 0.03f };
        DrawLineEx(hilt, tip, R * 0.26f, (Color){45, 42, 55, 255});
        DrawLineEx(hilt, tip, R * 0.12f, (Color){200, 205, 220, 255});
        BeginBlendMode(BLEND_ADDITIVE);
        DrawLineEx(hilt, tip, R * 0.05f, (Color){255, 90, 40, (uchar)(160 + 60 * pulse)});
        EndBlendMode();

        Vector2 dir = {tip.x - hilt.x, tip.y - hilt.y};
        float invLen = 1.0f / sqrtf(dir.x * dir.x + dir.y * dir.y);
        Vector2 perp = {-dir.y * invLen, dir.x * invLen};
        Vector2 mid = { hilt.x * 0.35f + tip.x * 0.65f,
                        hilt.y * 0.35f + tip.y * 0.65f };
        DrawLineEx((Vector2){mid.x - perp.x * R * 0.28f, mid.y - perp.y * R * 0.28f},
                   (Vector2){mid.x + perp.x * R * 0.28f, mid.y + perp.y * R * 0.28f},
                   R * 0.08f, (Color){255, 180, 60, 240});
        DrawCircleV(tip, R * 0.08f, (Color){255, 220, 120, 230});
    }

    for (int k = 0; k < 5; k++) {
        float a = t * 1.8f + k * (2.0f * PI / 5.0f);
        float d = R * (1.35f + 0.08f * sinf(t * 3.0f + k));
        float bx = cx + cosf(a) * d, by = cy + sinf(a) * d * 0.72f;
        DrawLineEx((Vector2){bx - cosf(a) * R * 0.18f, by - sinf(a) * R * 0.18f},
                   (Vector2){bx + cosf(a) * R * 0.18f, by + sinf(a) * R * 0.18f},
                   R * 0.06f, (Color){255, 100, 40, 200});
    }
    DrawCircle((int)cx, (int)cy, (int)R, (Color){120, 18, 22, 255});
    DrawCircleV((Vector2){cx, cy}, R * 0.62f, (Color){180, 30, 30, 160});
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cy}, R * (0.38f + 0.06f * pulse),
                (Color){255, 90, 30, (uchar)(90 + 50 * pulse)});
    EndBlendMode();
    DrawRectangle((int)(cx - R * 0.42f), (int)(cy - R * 0.12f), (int)(R * 0.28f), (int)(R * 0.10f),
                  (Color){255, 220, 60, 255});
    DrawRectangle((int)(cx + R * 0.14f), (int)(cy - R * 0.12f), (int)(R * 0.28f), (int)(R * 0.10f),
                  (Color){255, 220, 60, 255});
    DrawLineEx((Vector2){cx - R * 0.55f, cy + R * 0.22f}, (Vector2){cx - R * 0.15f, cy + R * 0.38f},
               R * 0.07f, (Color){40, 8, 10, 230});
    DrawLineEx((Vector2){cx + R * 0.15f, cy + R * 0.38f}, (Vector2){cx + R * 0.55f, cy + R * 0.22f},
               R * 0.07f, (Color){40, 8, 10, 230});
}

// ─── SKULL #17 = in-game Skull (Clockwork Seraph art) ─────────────────────────

// ─── WARRIORS (high detail, saturated color) ─────────────────────────────────







// ─── DEMON RAGE — Idea.png cartoon demon (tight proportions) ──────────────────

// Tiny filled heart (cute FX helper)
static void HeartDot(float x, float y, float s, Color c) {
    DrawCircleV((Vector2){x - s * 0.35f, y - s * 0.15f}, s * 0.42f, c);
    DrawCircleV((Vector2){x + s * 0.35f, y - s * 0.15f}, s * 0.42f, c);
    DrawTriangle((Vector2){x - s * 0.72f, y - s * 0.05f},
                 (Vector2){x + s * 0.72f, y - s * 0.05f},
                 (Vector2){x, y + s * 0.85f}, c);
}

// ─── STARFALL UNICORN — super cute free silhouette ───────────────────────────
// Round chubby cheeks, stubby muzzle, HUGE sparkly eyes, candy horn, soft mane.
static void Skin_StarfallUnicorn(float cx, float cy, float R, float t) {
    float bob = sinf(t * 2.0f) * R * 0.05f;
    float cyb = cy + bob;
    float p  = 0.5f + 0.5f * sinf(t * 2.6f);
    float p2 = 0.5f + 0.5f * sinf(t * 3.4f + 0.8f);

    Color cream  = (Color){255, 250, 252, 255};
    Color creamD = (Color){255, 230, 240, 255};
    Color blush  = (Color){255, 130, 175, 130};
    Color white  = (Color){255, 255, 255, 255};

    // Soft pastel cloud aura
    BeginBlendMode(BLEND_ADDITIVE);
    for (int k = 0; k < 7; k++) {
        Color c = ColorFromHSV(fmodf(300.0f + k * 25.0f + t * 20.0f, 360.0f), 0.35f, 1.0f);
        float a = t * 0.5f + k * 0.95f;
        DrawCircleV((Vector2){cx + cosf(a) * R * 0.85f, cyb + sinf(a * 1.2f) * R * 0.55f},
                    R * (0.45f + 0.1f * sinf(t + k)),
                    (Color){c.r, c.g, c.b, (uchar)(16 + 12 * p)});
    }
    EndBlendMode();

    // ── Fluffy cotton-candy mane (soft blobs, not harsh ribbons) ────────────
    for (int i = 0; i < 12; i++) {
        float u = i / 11.0f;
        float sway = sinf(t * 2.2f + i * 0.5f) * R * (0.14f + u * 0.12f);
        float mx = cx - R * 0.15f + sway - u * R * 0.40f;
        float my = cyb - R * 0.35f + u * R * 1.55f;
        Color mc = ColorFromHSV(fmodf(310.0f + i * 10.0f + t * 18.0f, 360.0f), 0.38f, 1.0f);
        float rr = R * (0.36f - u * 0.12f);
        DrawCircleV((Vector2){mx, my}, rr, (Color){mc.r, mc.g, mc.b, (uchar)(235 - i * 8)});
        DrawCircleV((Vector2){mx - R * 0.06f, my - R * 0.06f}, rr * 0.45f,
                    (Color){255, 255, 255, (uchar)(70 + 30 * p)});
    }
    // soft forelock puffs
    for (int i = 0; i < 4; i++) {
        float sway = sinf(t * 2.8f + i) * R * 0.08f;
        float fx = cx - R * 0.05f + (i - 1.5f) * R * 0.16f + sway;
        float fy = cyb - R * 0.55f + fabsf(i - 1.5f) * R * 0.04f;
        Color fc = ColorFromHSV(fmodf(320.0f + i * 18.0f + t * 25.0f, 360.0f), 0.32f, 1.0f);
        DrawCircleV((Vector2){fx, fy}, R * 0.20f, (Color){fc.r, fc.g, fc.b, 240});
    }

    // ── Chubby neck ─────────────────────────────────────────────────────────
    DrawEllipse((int)(cx + R * 0.05f), (int)(cyb + R * 0.65f),
                (int)(R * 0.55f), (int)(R * 0.50f), creamD);
    DrawEllipse((int)(cx + R * 0.10f), (int)(cyb + R * 0.70f),
                (int)(R * 0.38f), (int)(R * 0.32f), cream);

    // ── Round chubby head (super soft, almost ball with cheeks) ─────────────
    DrawEllipse((int)cx, (int)(cyb + R * 0.05f), (int)(R * 0.98f), (int)(R * 1.00f), creamD);
    DrawEllipse((int)cx, (int)cyb, (int)(R * 0.90f), (int)(R * 0.92f), cream);
    // big cheek blobs
    DrawCircleV((Vector2){cx - R * 0.52f, cyb + R * 0.25f}, R * 0.38f, cream);
    DrawCircleV((Vector2){cx + R * 0.48f, cyb + R * 0.28f}, R * 0.34f, cream);
    // glossy highlight
    DrawCircleV((Vector2){cx - R * 0.28f, cyb - R * 0.32f}, R * 0.28f,
                (Color){255, 255, 255, 100});
    DrawCircleV((Vector2){cx - R * 0.34f, cyb - R * 0.38f}, R * 0.10f,
                (Color){255, 255, 255, 200});

    // Heavy blush
    DrawCircleV((Vector2){cx - R * 0.48f, cyb + R * 0.28f}, R * 0.20f, blush);
    DrawCircleV((Vector2){cx + R * 0.42f, cyb + R * 0.30f}, R * 0.17f,
                (Color){255, 130, 175, 110});
    // freckle dots
    DrawCircleV((Vector2){cx - R * 0.55f, cyb + R * 0.22f}, R * 0.025f, (Color){255, 150, 180, 180});
    DrawCircleV((Vector2){cx - R * 0.42f, cyb + R * 0.35f}, R * 0.02f, (Color){255, 150, 180, 160});
    DrawCircleV((Vector2){cx + R * 0.48f, cyb + R * 0.24f}, R * 0.022f, (Color){255, 150, 180, 170});

    // ── Stubby cute muzzle (short, not long horse face) ─────────────────────
    DrawEllipse((int)(cx + R * 0.28f), (int)(cyb + R * 0.38f),
                (int)(R * 0.38f), (int)(R * 0.30f), creamD);
    DrawEllipse((int)(cx + R * 0.32f), (int)(cyb + R * 0.40f),
                (int)(R * 0.28f), (int)(R * 0.22f), (Color){255, 235, 245, 255});
    // tiny nostrils
    DrawEllipse((int)(cx + R * 0.38f), (int)(cyb + R * 0.38f),
                (int)(R * 0.045f), (int)(R * 0.055f), (Color){200, 140, 170, 200});
    DrawEllipse((int)(cx + R * 0.48f), (int)(cyb + R * 0.42f),
                (int)(R * 0.04f), (int)(R * 0.05f), (Color){200, 140, 170, 190});

    // ── Round soft ears ─────────────────────────────────────────────────────
    DrawEllipse((int)(cx - R * 0.55f), (int)(cyb - R * 0.70f),
                (int)(R * 0.22f), (int)(R * 0.32f), cream);
    DrawEllipse((int)(cx - R * 0.55f), (int)(cyb - R * 0.72f),
                (int)(R * 0.12f), (int)(R * 0.20f), (Color){255, 160, 200, 220});
    DrawEllipse((int)(cx + R * 0.45f), (int)(cyb - R * 0.65f),
                (int)(R * 0.18f), (int)(R * 0.26f), cream);
    DrawEllipse((int)(cx + R * 0.45f), (int)(cyb - R * 0.67f),
                (int)(R * 0.09f), (int)(R * 0.15f), (Color){255, 160, 200, 200});

    // ── Candy spiral horn (shorter, pastel, cute) ───────────────────────────
    float hx = cx, hy = cyb - R * 0.75f;
    DrawEllipse((int)hx, (int)(hy + R * 0.06f), (int)(R * 0.18f), (int)(R * 0.10f),
                (Color){255, 210, 230, 255});
    for (int i = 0; i < 7; i++) {
        float u0 = i / 7.0f, u1 = (i + 1) / 7.0f;
        float w0 = R * (0.14f - u0 * 0.10f), w1 = R * (0.14f - u1 * 0.10f);
        float y0 = hy - u0 * R * 0.95f, y1 = hy - u1 * R * 0.95f;
        float spin = t * 1.8f + i * 0.9f;
        float ox0 = sinf(spin) * R * 0.04f * (1.0f - u0);
        float ox1 = sinf(spin + 0.9f) * R * 0.04f * (1.0f - u1);
        Color hc = ColorFromHSV(fmodf(320.0f + i * 25.0f + t * 30.0f, 360.0f), 0.35f, 1.0f);
        if (i % 2 == 0) hc = (Color){255, 240, 160, 255};
        DrawTriangle((Vector2){hx + ox0 - w0, y0}, (Vector2){hx + ox0 + w0, y0},
                     (Vector2){hx + ox1, y1}, hc);
        DrawTriangle((Vector2){hx + ox0 + w0, y0}, (Vector2){hx + ox1 + w1, y1},
                     (Vector2){hx + ox1 - w1, y1}, hc);
    }
    float tipY = hy - R * 0.95f;
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){hx, tipY}, R * (0.14f + 0.04f * p),
                (Color){255, 220, 255, (uchar)(110 + 70 * p)});
    EndBlendMode();
    Star4(hx, tipY, R * 0.16f, (Color){255, 255, 230, 255});
    Star4(hx, tipY, R * 0.08f, (Color){255, 200, 230, 255});

    // ── HUGE kawaii eyes (the cuteness engine) ──────────────────────────────
    // left eye
    DrawEllipse((int)(cx - R * 0.22f), (int)(cyb - R * 0.02f),
                (int)(R * 0.26f), (int)(R * 0.30f), (Color){40, 28, 55, 255});
    DrawEllipse((int)(cx - R * 0.20f), (int)(cyb - R * 0.04f),
                (int)(R * 0.18f), (int)(R * 0.20f), (Color){160, 100, 220, 255});
    DrawEllipse((int)(cx - R * 0.18f), (int)(cyb - R * 0.04f),
                (int)(R * 0.10f), (int)(R * 0.14f), (Color){70, 40, 120, 255});
    DrawEllipse((int)(cx - R * 0.18f), (int)(cyb - R * 0.04f),
                (int)(R * 0.05f), (int)(R * 0.09f), (Color){20, 10, 30, 255});
    DrawCircleV((Vector2){cx - R * 0.12f, cyb - R * 0.12f}, R * 0.07f, white);
    DrawCircleV((Vector2){cx - R * 0.28f, cyb + R * 0.06f}, R * 0.035f, white);
    // right eye (slightly smaller / 3/4)
    DrawEllipse((int)(cx + R * 0.28f), (int)(cyb + R * 0.00f),
                (int)(R * 0.22f), (int)(R * 0.26f), (Color){40, 28, 55, 255});
    DrawEllipse((int)(cx + R * 0.30f), (int)(cyb - R * 0.02f),
                (int)(R * 0.14f), (int)(R * 0.16f), (Color){160, 100, 220, 255});
    DrawEllipse((int)(cx + R * 0.31f), (int)(cyb - R * 0.02f),
                (int)(R * 0.07f), (int)(R * 0.10f), (Color){70, 40, 120, 255});
    DrawEllipse((int)(cx + R * 0.32f), (int)(cyb - R * 0.02f),
                (int)(R * 0.04f), (int)(R * 0.07f), (Color){20, 10, 30, 255});
    DrawCircleV((Vector2){cx + R * 0.36f, cyb - R * 0.08f}, R * 0.055f, white);
    DrawCircleV((Vector2){cx + R * 0.22f, cyb + R * 0.06f}, R * 0.025f, white);

    // soft lash arcs
    DrawLineEx((Vector2){cx - R * 0.42f, cyb - R * 0.18f},
               (Vector2){cx - R * 0.05f, cyb - R * 0.22f}, R * 0.04f, (Color){50, 30, 70, 220});
    DrawLineEx((Vector2){cx + R * 0.10f, cyb - R * 0.16f},
               (Vector2){cx + R * 0.45f, cyb - R * 0.18f}, R * 0.035f, (Color){50, 30, 70, 200});
    // tiny lash tips
    DrawLineEx((Vector2){cx - R * 0.40f, cyb - R * 0.18f},
               (Vector2){cx - R * 0.50f, cyb - R * 0.30f}, R * 0.03f, (Color){45, 25, 65, 220});
    DrawLineEx((Vector2){cx - R * 0.28f, cyb - R * 0.24f},
               (Vector2){cx - R * 0.32f, cyb - R * 0.36f}, R * 0.025f, (Color){45, 25, 65, 200});

    // ── Tiny happy smile + soft wink line ───────────────────────────────────
    {
        Vector2 prev = {0};
        for (int s = 0; s <= 10; s++) {
            float th = 0.35f + 0.9f * (s / 10.0f);
            Vector2 pt = {
                cx + R * 0.05f + cosf(th) * R * 0.18f,
                cyb + R * 0.48f + sinf(th) * R * 0.10f
            };
            if (s > 0) DrawLineEx(prev, pt, R * 0.045f, (Color){240, 120, 160, 230});
            prev = pt;
        }
    }

    // ── Hearts + soft sparkles ──────────────────────────────────────────────
    for (int k = 0; k < 12; k++) {
        float pr = fmodf(t * 0.40f + Rnd(k + 11), 1.0f);
        float a  = Rnd(k) * 2.0f * PI + t * 0.65f;
        float dist = R * (0.70f + pr * 0.90f);
        float sx = cx + cosf(a) * dist * 0.85f;
        float sy = cyb + sinf(a) * dist * 0.70f - pr * R * 0.2f;
        uchar al = (uchar)(200 * (1.0f - pr));
        if (k % 3 == 0)
            HeartDot(sx, sy, R * (0.07f - pr * 0.03f),
                     (Color){255, 140, 190, al});
        else if (k % 3 == 1)
            Star4(sx, sy, R * (0.06f - pr * 0.025f),
                  (Color){255, 230, 255, al});
        else
            DrawCircleV((Vector2){sx, sy}, R * (0.03f - pr * 0.012f),
                        (Color){255, 255, 220, al});
    }
    // cheek heart pop
    if (p2 > 0.55f) {
        float a = (p2 - 0.55f) / 0.45f;
        HeartDot(cx - R * 0.72f + sinf(t * 2.5f) * R * 0.04f,
                 cyb + R * 0.05f - a * R * 0.35f,
                 R * 0.08f, (Color){255, 120, 170, (uchar)(200 * a)});
    }
}

// ─── APEX REX — free silhouette T-rex predator ────────────────────────────────
// Long snout, heavy brow, chomping jaw, dorsal sail-spines, scale texture,
// tiny comic arms, belly, roaring heat — deliberately NOT a circle.
static void Skin_ApexRex(float cx, float cy, float R, float t) {
    float chomp   = 0.5f + 0.5f * sinf(t * 3.2f);
    float bob     = sinf(t * 1.5f) * R * 0.045f;
    float cyb     = cy + bob;
    float jawOpen = R * (0.06f + 0.16f * chomp);
    float snarl   = 0.5f + 0.5f * sinf(t * 2.1f);

    Color hideD  = (Color){28, 72, 32, 255};
    Color hide   = (Color){48, 125, 50, 255};
    Color hideM  = (Color){70, 160, 62, 255};
    Color hideHi = (Color){110, 200, 85, 255};
    Color belly  = (Color){210, 225, 110, 255};
    Color gum    = (Color){90, 30, 40, 255};
    Color tooth  = (Color){250, 248, 235, 255};
    Color toothD = (Color){210, 205, 185, 255};

    // Ambient primal heat / dust
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx + R * 0.2f, cyb}, R * (1.55f + 0.12f * chomp),
                (Color){255, 90, 15, (uchar)(18 + 20 * chomp)});
    DrawCircleV((Vector2){cx + R * 0.9f, cyb}, R * 0.70f,
                (Color){255, 140, 30, (uchar)(10 + 18 * chomp)});
    EndBlendMode();

    // ── Tail stump (left, breaks circle) ────────────────────────────────────
    EllipseRotF(cx - R * 1.05f, cyb + R * 0.55f,
                R * 0.55f, R * 0.28f, 0.35f, hideD);
    EllipseRotF(cx - R * 1.25f, cyb + R * 0.70f,
                R * 0.35f, R * 0.18f, 0.55f, hide);
    // tail tip spike
    DrawTriangle((Vector2){cx - R * 1.50f, cyb + R * 0.65f},
                 (Vector2){cx - R * 1.50f, cyb + R * 0.85f},
                 (Vector2){cx - R * 1.85f, cyb + R * 0.90f}, hideD);

    // ── Dorsal sail / spines (back ridge — big silhouette break) ────────────
    for (int i = 0; i < 9; i++) {
        float u = i / 8.0f;
        float sx = cx - R * 1.00f + u * R * 0.95f;
        float sy = cyb - R * 0.05f - u * R * 0.25f + sinf(u * PI) * R * 0.08f;
        float h  = R * (0.35f + 0.70f * sinf(u * PI) + 0.05f * sinf(t * 3.5f + i));
        float lean = sinf(t * 1.8f + i * 0.4f) * R * 0.05f;
        // dark base plate
        DrawTriangle((Vector2){sx - R * 0.14f, sy},
                     (Vector2){sx + R * 0.14f, sy},
                     (Vector2){sx + lean, sy - h}, hideD);
        // bright face of spine
        DrawTriangle((Vector2){sx - R * 0.07f, sy - R * 0.02f},
                     (Vector2){sx + R * 0.07f, sy - R * 0.02f},
                     (Vector2){sx + lean * 0.5f, sy - h * 0.88f}, hideHi);
        // thin rim highlight
        DrawLineEx((Vector2){sx - R * 0.04f, sy - R * 0.02f},
                   (Vector2){sx + lean * 0.3f, sy - h * 0.9f},
                   R * 0.02f, (Color){160, 230, 120, 180});
    }

    // ── Thick body / neck mass ──────────────────────────────────────────────
    EllipseRotF(cx - R * 0.40f, cyb + R * 0.40f,
                R * 0.95f, R * 0.80f, 0.20f, hideD);
    EllipseRotF(cx - R * 0.25f, cyb + R * 0.30f,
                R * 0.80f, R * 0.65f, 0.15f, hide);
    EllipseRotF(cx - R * 0.15f, cyb + R * 0.20f,
                R * 0.60f, R * 0.48f, 0.10f, hideM);
    // belly plate
    EllipseRotF(cx - R * 0.05f, cyb + R * 0.55f,
                R * 0.48f, R * 0.38f, 0.25f, belly);
    EllipseRotF(cx + R * 0.05f, cyb + R * 0.50f,
                R * 0.28f, R * 0.22f, 0.2f, (Color){230, 240, 150, 200});
    // belly scale lines
    for (int k = 0; k < 4; k++) {
        float by = cyb + R * 0.40f + k * R * 0.10f;
        DrawLineEx((Vector2){cx - R * 0.30f, by},
                   (Vector2){cx + R * 0.25f, by + R * 0.04f},
                   R * 0.025f, (Color){160, 175, 70, 140});
    }

    // ── Cranium (heavy oval skull) ──────────────────────────────────────────
    EllipseRotF(cx + R * 0.08f, cyb - R * 0.02f,
                R * 1.00f, R * 0.88f, -0.08f, hide);
    EllipseRotF(cx + R * 0.12f, cyb - R * 0.12f,
                R * 0.75f, R * 0.58f, -0.06f, hideM);
    // skull highlight
    EllipseRotF(cx - R * 0.05f, cyb - R * 0.28f,
                R * 0.40f, R * 0.22f, -0.3f, (Color){130, 210, 100, 120});

    // Scale texture dots on cranium
    for (int k = 0; k < 10; k++) {
        float sx = cx - R * 0.25f + Rnd(k + 40) * R * 0.70f;
        float sy = cyb - R * 0.35f + Rnd(k + 50) * R * 0.55f;
        DrawCircleV((Vector2){sx, sy}, R * (0.035f + Rnd(k) * 0.025f),
                    (Color){35, 90, 35, 160});
    }

    // ── Long snout (the non-round hero shape) ───────────────────────────────
    EllipseRotF(cx + R * 0.85f, cyb + R * 0.02f,
                R * 0.85f, R * 0.48f, 0.08f, hide);
    EllipseRotF(cx + R * 1.05f, cyb - R * 0.02f,
                R * 0.55f, R * 0.36f, 0.05f, hideM);
    // upper nose ridge (lighter)
    EllipseRotF(cx + R * 0.75f, cyb - R * 0.18f,
                R * 0.55f, R * 0.16f, 0.05f, hideHi);
    // snout tip
    EllipseRotF(cx + R * 1.25f, cyb + R * 0.00f,
                R * 0.28f, R * 0.24f, 0.0f, hide);
    // nostrils (deep pits)
    EllipseRotF(cx + R * 1.22f, cyb - R * 0.08f,
                R * 0.09f, R * 0.11f, 0.2f, (Color){12, 22, 10, 255});
    EllipseRotF(cx + R * 1.32f, cyb + R * 0.06f,
                R * 0.08f, R * 0.10f, 0.15f, (Color){12, 22, 10, 255});
    // wet nostril shine
    DrawCircleV((Vector2){cx + R * 1.20f, cyb - R * 0.12f}, R * 0.025f,
                (Color){80, 140, 70, 180});

    // Side jaw muscle bulge
    EllipseRotF(cx + R * 0.45f, cyb + R * 0.22f,
                R * 0.35f, R * 0.28f, 0.1f, hideD);
    // ── Heavy brow ridge + angry scowl ──────────────────────────────────────
    EllipseRotF(cx + R * 0.18f, cyb - R * 0.38f,
                R * 0.60f, R * 0.24f, -0.12f, hideD);
    // bony brow spikes
    DrawTriangle((Vector2){cx - R * 0.05f, cyb - R * 0.45f},
                 (Vector2){cx + R * 0.15f, cyb - R * 0.50f},
                 (Vector2){cx + R * 0.05f, cyb - R * 0.72f}, hideD);
    DrawTriangle((Vector2){cx + R * 0.25f, cyb - R * 0.42f},
                 (Vector2){cx + R * 0.45f, cyb - R * 0.45f},
                 (Vector2){cx + R * 0.38f, cyb - R * 0.68f}, hideD);
    // scowl line
    DrawLineEx((Vector2){cx - R * 0.20f, cyb - R * 0.30f},
               (Vector2){cx + R * 0.60f, cyb - R * 0.16f},
               R * 0.13f, (Color){18, 45, 18, 255});
    DrawLineEx((Vector2){cx - R * 0.15f, cyb - R * 0.34f},
               (Vector2){cx + R * 0.55f, cyb - R * 0.20f},
               R * 0.06f, (Color){12, 30, 12, 255});

    // ── Predatory eye ───────────────────────────────────────────────────────
    // socket
    DrawEllipse((int)(cx + R * 0.22f), (int)(cyb - R * 0.10f),
                (int)(R * 0.26f), (int)(R * 0.20f), (Color){10, 18, 8, 255});
    // yellow sclera
    DrawEllipse((int)(cx + R * 0.24f), (int)(cyb - R * 0.10f),
                (int)(R * 0.17f), (int)(R * 0.14f), (Color){255, 225, 35, 255});
    // orange rim
    DrawEllipse((int)(cx + R * 0.24f), (int)(cyb - R * 0.10f),
                (int)(R * 0.12f), (int)(R * 0.11f), (Color){255, 160, 20, 255});
    // vertical slit pupil (widens slightly with snarl)
    float slitW = R * (0.045f + 0.02f * (1.0f - snarl));
    DrawEllipse((int)(cx + R * 0.26f), (int)(cyb - R * 0.10f),
                (int)slitW, (int)(R * 0.11f), (Color){8, 8, 8, 255});
    // catchlight
    DrawCircleV((Vector2){cx + R * 0.18f, cyb - R * 0.16f}, R * 0.035f,
                (Color){255, 255, 255, 230});

    // ── Mouth cavity + gums ─────────────────────────────────────────────────
    float mouthY = cyb + R * 0.30f;
    DrawEllipse((int)(cx + R * 0.60f), (int)(mouthY + jawOpen * 0.35f),
                (int)(R * 0.55f), (int)(R * 0.18f + jawOpen * 0.7f),
                (Color){20, 8, 10, 255});
    // upper gum line
    DrawEllipse((int)(cx + R * 0.60f), (int)(mouthY - R * 0.02f),
                (int)(R * 0.52f), (int)(R * 0.10f), gum);

    // Upper teeth (uneven, longer front fangs)
    for (int k = 0; k < 7; k++) {
        float u  = k / 6.0f;
        float tx = cx + R * 0.30f + u * R * 0.95f;
        float ty = mouthY + R * 0.02f;
        float th = R * (0.14f + 0.10f * sinf(u * PI) + ((k == 1 || k == 2) ? 0.08f : 0.0f));
        float tw = R * (0.055f + ((k == 1 || k == 2) ? 0.02f : 0.0f));
        DrawTriangle((Vector2){tx - tw, ty},
                     (Vector2){tx + tw, ty},
                     (Vector2){tx + (Rnd(k) - 0.5f) * R * 0.02f, ty + th},
                     (k % 2) ? tooth : toothD);
        // enamel highlight
        DrawLineEx((Vector2){tx - tw * 0.3f, ty + R * 0.02f},
                   (Vector2){tx, ty + th * 0.7f},
                   R * 0.015f, (Color){255, 255, 255, 160});
    }

    // ── Lower jaw (animated chomp) ──────────────────────────────────────────
    float jy = cyb + R * 0.48f + jawOpen;
    EllipseRotF(cx + R * 0.60f, jy,
                R * 0.78f, R * 0.34f, 0.06f, hideD);
    EllipseRotF(cx + R * 0.78f, jy + R * 0.04f,
                R * 0.52f, R * 0.22f, 0.05f, hide);
    // chin highlight
    EllipseRotF(cx + R * 0.70f, jy + R * 0.08f,
                R * 0.30f, R * 0.10f, 0.0f, hideM);
    // lower gum
    DrawEllipse((int)(cx + R * 0.58f), (int)(jy - R * 0.10f),
                (int)(R * 0.48f), (int)(R * 0.08f), gum);
    // lower teeth (point up)
    for (int k = 0; k < 6; k++) {
        float u  = k / 5.0f;
        float tx = cx + R * 0.28f + u * R * 0.90f;
        float ty = jy - R * 0.08f;
        float th = R * (0.12f + 0.06f * sinf(u * PI));
        float tw = R * 0.05f;
        DrawTriangle((Vector2){tx - tw, ty},
                     (Vector2){tx + tw, ty},
                     (Vector2){tx, ty - th},
                     (k % 2) ? tooth : toothD);
    }

    // Tongue flick when mouth wide
    if (jawOpen > R * 0.12f) {
        float tong = (jawOpen - R * 0.12f) / (R * 0.10f);
        if (tong > 1.0f) tong = 1.0f;
        float twag = sinf(t * 6.0f) * R * 0.04f;
        DrawEllipse((int)(cx + R * 0.70f + twag), (int)(mouthY + jawOpen * 0.5f),
                    (int)(R * 0.18f * tong), (int)(R * 0.10f + jawOpen * 0.2f),
                    (Color){200, 40, 55, (uchar)(220 * tong)});
    }

    // ── Breath / roar vapor when chomping ───────────────────────────────────
    if (chomp > 0.55f) {
        float pr = (chomp - 0.55f) / 0.45f;
        for (int k = 0; k < 7; k++) {
            float px = cx + R * 1.30f + k * R * 0.10f + Rnd(k + 20) * R * 0.08f;
            float py = cyb - R * 0.05f + (Rnd(k + 30) - 0.5f) * R * 0.25f
                       - pr * R * 0.15f;
            DrawCircleV((Vector2){px, py},
                        R * (0.05f + Rnd(k) * 0.04f) * pr,
                        (Color){255, (uchar)(140 + k * 10), 30, (uchar)(160 * pr * (1.0f - k * 0.1f))});
        }
        // heat shimmer lines
        BeginBlendMode(BLEND_ADDITIVE);
        for (int k = 0; k < 3; k++) {
            float a = -0.3f + k * 0.3f + sinf(t * 8.0f + k) * 0.1f;
            DrawLineEx((Vector2){cx + R * 1.35f, cyb},
                       (Vector2){cx + R * 1.35f + cosf(a) * R * 0.55f * pr,
                                 cyb + sinf(a) * R * 0.25f},
                       R * 0.04f, (Color){255, 180, 40, (uchar)(50 * pr)});
        }
        EndBlendMode();
    }
}

// ─── BLOODSCALE VANGUARD — dark-red mini-scale plate warrior ──────────────────
// Hero read: entire harness tiled with tiny overlapping dark-crimson scales.
// Free silhouette: tall crest helm, spined pauldrons, kite shield, longblade.

// ─── CROWNCREST REX — tall upright T-rex (vertical, not long) ────────────────
// Stacked silhouette: body low, long neck UP, head high, short snout, crown spines.

// ─── NIGHTWHISPER ROGUE — medieval cutpurse, leather + steel ─────────────────
// Same craft level as Bloodscale, but hood, studded leather, dual daggers,
// earthy forest/charcoal palette — classic medieval rogue, not fantasy plate.

// Shared mini-scale mail (Bloodscale #31 body craft) — used by warrior redesigns.
static void DrawMiniScaleMail(float cx, float cyb, float R, float p, float p2,
                              Color scBase, Color scMid, Color scLit, Color scHi, Color scHot,
                              Color rivet) {
    for (int row = 0; row < 14; row++) {
        float u = row / 13.0f;
        float yw = cyb - R * 0.78f + u * R * 1.70f;
        float halfW = R * (0.78f + 0.18f * sinf(u * PI) - fabsf(u - 0.55f) * 0.15f);
        int cols = 9 + (row % 2);
        for (int col = 0; col < cols; col++) {
            float v = (cols <= 1) ? 0.5f : col / (float)(cols - 1);
            float stagger = (row % 2) ? (0.5f / cols) : 0.0f;
            float xw = cx - halfW + (v + stagger) * halfW * 2.0f;
            float dx = (xw - cx) / (R * 0.95f);
            float dy = (yw - cyb) / (R * 1.02f);
            if (dx * dx + dy * dy > 1.05f) continue;
            float rr = R * (0.065f + 0.012f * sinf(u * 6.0f + col));
            float ang = (v - 0.5f) * 0.35f;
            int hash = (row * 17 + col * 31) & 7;
            Color face = scLit;
            if (hash == 0) face = scMid;
            if (hash == 1) face = scHi;
            if (hash == 2 && p > 0.65f) face = scHot;
            EllipseRotF(xw, yw, rr, rr * 1.25f, ang, face);
            EllipseRotF(xw, yw + rr * 0.35f, rr * 0.85f, rr * 0.40f, ang, scBase);
            EllipseRotF(xw - rr * 0.15f, yw - rr * 0.35f,
                        rr * 0.40f, rr * 0.28f, ang - 0.2f,
                        (Color){scHi.r, scHi.g, scHi.b, (uchar)(90 + 50 * p2)});
            if ((row + col) % 5 == 0)
                DrawCircleV((Vector2){xw, yw - rr * 0.15f}, rr * 0.18f,
                            (Color){rivet.r, rivet.g, rivet.b, 180});
        }
    }
}

// ─── STORMFORGED AEGIS — Stormplate in Bloodscale craft ──────────────────────
// Cobalt mini-plate mail, tower shield, cyan-edge longsword, crimson plume helm.

// ─── SUNFORGED CRUSADER — Solar Paladin in Bloodscale craft ──────────────────
// Golden mini-scales, warhammer, sun-crest helm, holy cross, ray aura.

// ─── LEGATE PRAETORIAN — Crimson Centurion in Bloodscale craft ────────────────
// Bronze mini-lorica, scutum, gladius, tall red crest galea.
static void Skin_LegatePraetorian(float cx, float cy, float R, float t) {
    float p  = 0.5f + 0.5f * sinf(t * 3.0f);
    float p2 = 0.5f + 0.5f * sinf(t * 3.8f + 0.5f);
    float bob = sinf(t * 1.5f) * R * 0.03f;
    float cyb = cy + bob;

    Color scBase = (Color){90, 55, 20, 255};
    Color scMid  = (Color){160, 110, 40, 255};
    Color scLit  = (Color){210, 160, 55, 255};
    Color scHi   = (Color){245, 205, 100, 255};
    Color scHot  = (Color){255, 230, 150, 255};
    Color gold   = (Color){230, 180, 50, 255};
    Color goldHi = (Color){255, 230, 120, 255};
    Color crimson= (Color){200, 25, 30, 255};
    Color voidB  = (Color){20, 10, 8, 255};
    Color steel  = (Color){150, 155, 170, 255};
    Color steelH = (Color){230, 235, 245, 255};

    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cyb}, R * (1.45f + 0.08f * p),
                (Color){255, 50, 30, (uchar)(18 + 14 * p)});
    EndBlendMode();

    // Crimson cape
    for (int i = 0; i < 7; i++) {
        float u = i / 6.0f;
        float sway = sinf(t * 2.3f + i * 0.55f) * R * (0.12f + u * 0.15f);
        EllipseRotF(cx + sway * 0.5f, cyb + R * 0.25f + u * R * 1.05f,
                    R * (0.72f - u * 0.22f), R * 0.30f, 0.12f,
                    (Color){(uchar)(180 - i * 8), 10, 20, (uchar)(230 - i * 16)});
    }

    // Scutum (left) — vivid red + gold
    {
        float sx = cx - R * 1.40f, sy = cyb + R * 0.05f;
        DrawRectangleRounded((Rectangle){sx - R * 0.40f, sy - R * 1.00f, R * 0.82f, R * 2.05f},
                             0.10f, 6, (Color){80, 12, 12, 255});
        DrawRectangleRounded((Rectangle){sx - R * 0.32f, sy - R * 0.88f, R * 0.66f, R * 1.82f},
                             0.10f, 6, crimson);
        DrawRectangleRounded((Rectangle){sx - R * 0.26f, sy - R * 0.78f, R * 0.54f, R * 1.60f},
                             0.10f, 6, (Color){255, 55, 45, 230});
        // mini bronze studs on shield
        for (int row = 0; row < 8; row++)
            for (int col = 0; col < 3; col++) {
                float xw = sx - R * 0.16f + col * R * 0.16f;
                float yw = sy - R * 0.65f + row * R * 0.20f;
                DrawCircleV((Vector2){xw, yw}, R * 0.03f, gold);
            }
        DrawLineEx((Vector2){sx, sy - R * 0.70f}, (Vector2){sx, sy + R * 0.75f}, R * 0.07f, gold);
        DrawCircleV((Vector2){sx, sy}, R * 0.16f, gold);
        DrawCircleV((Vector2){sx, sy}, R * 0.08f, goldHi);
    }

    // Gladius (right)
    {
        Vector2 hilt = { cx + R * 0.52f, cyb + R * 0.52f };
        Vector2 tip  = { cx + R * 1.25f, cyb - R * 1.20f };
        DrawLineEx(hilt, tip, R * 0.16f, voidB);
        DrawLineEx(hilt, tip, R * 0.10f, steel);
        DrawLineEx(hilt, tip, R * 0.04f, steelH);
        DrawLineEx((Vector2){cx + R * 0.80f, cyb + R * 0.05f},
                   (Vector2){cx + R * 1.10f, cyb + R * 0.25f}, R * 0.10f, scBase);
        DrawLineEx((Vector2){cx + R * 0.80f, cyb + R * 0.05f},
                   (Vector2){cx + R * 1.10f, cyb + R * 0.25f}, R * 0.04f, gold);
        DrawCircleV(hilt, R * 0.10f, gold);
        DrawCircleV(hilt, R * 0.05f, goldHi);
    }

    DrawEllipse((int)cx, (int)(cyb + R * 0.08f), (int)(R * 1.02f), (int)(R * 1.10f), scBase);
    DrawEllipse((int)cx, (int)cyb, (int)(R * 0.92f), (int)(R * 0.98f), scMid);
    DrawMiniScaleMail(cx, cyb, R, p, p2, scBase, scMid, scLit, scHi, scHot, gold);

    // Horizontal lorica band accents over scales
    for (int i = -2; i <= 2; i++) {
        DrawRectangleRounded((Rectangle){cx - R * 0.72f, cyb + i * R * 0.22f - R * 0.05f,
                                         R * 1.44f, R * 0.08f}, 0.4f, 4,
                             (Color){scHi.r, scHi.g, scHi.b, 120});
    }

    // Bronze pauldrons
    for (int s = -1; s <= 1; s += 2) {
        float px = cx + s * R * 1.00f, py = cyb;
        DrawEllipse((int)px, (int)py, (int)(R * 0.46f), (int)(R * 0.36f), scBase);
        DrawEllipse((int)px, (int)(py - R * 0.04f), (int)(R * 0.38f), (int)(R * 0.28f), scLit);
        for (int k = 0; k < 4; k++)
            EllipseRotF(px + s * (k - 1.5f) * R * 0.08f, py - R * 0.02f,
                        R * 0.12f, R * 0.14f, s * 0.15f, scHi);
        HaloArc(px, py, R * 0.40f, R * 0.30f, -2.1f, 0.4f, R * 0.035f, gold);
    }

    DrawRectangleRounded((Rectangle){cx - R * 0.68f, cyb + R * 0.55f, R * 1.36f, R * 0.14f}, 0.4f, 4, gold);
    for (int k = -3; k <= 3; k++) {
        float sway = sinf(t * 2.6f + k * 0.5f) * R * 0.03f;
        EllipseRotF(cx + k * R * 0.17f + sway, cyb + R * 0.78f, R * 0.11f, R * 0.15f, 0, scLit);
    }

    // Galea helm + tall red crest
    DrawEllipse((int)cx, (int)(cyb - R * 0.82f), (int)(R * 0.70f), (int)(R * 0.55f), scBase);
    DrawEllipse((int)cx, (int)(cyb - R * 0.88f), (int)(R * 0.58f), (int)(R * 0.44f), scLit);
    for (int row = 0; row < 4; row++)
        for (int col = 0; col < 5; col++) {
            float hx = cx - R * 0.38f + col * R * 0.19f + ((row % 2) ? R * 0.05f : 0);
            float hy = cyb - R * 1.05f + row * R * 0.12f;
            float dx = (hx - cx) / (R * 0.52f), dy = (hy - (cyb - R * 0.86f)) / (R * 0.40f);
            if (dx * dx + dy * dy > 1.0f) continue;
            DrawEllipse((int)hx, (int)hy, (int)(R * 0.05f), (int)(R * 0.05f), scHi);
        }
    // cheek guards
    DrawEllipse((int)(cx - R * 0.48f), (int)(cyb - R * 0.55f), (int)(R * 0.16f), (int)(R * 0.28f), scMid);
    DrawEllipse((int)(cx + R * 0.48f), (int)(cyb - R * 0.55f), (int)(R * 0.16f), (int)(R * 0.28f), scMid);
    DrawRectangleRounded((Rectangle){cx - R * 0.38f, cyb - R * 0.72f, R * 0.76f, R * 0.13f}, 0.3f, 4, voidB);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawRectangleRounded((Rectangle){cx - R * 0.32f, cyb - R * 0.68f, R * 0.64f, R * 0.06f},
                         0.3f, 4, (Color){255, 80, 40, (uchar)(100 + 50 * p)});
    EndBlendMode();
    // vivid crest
    for (int i = 0; i < 11; i++) {
        float sway = sinf(t * 5.0f + i * 0.35f) * R * 0.07f;
        DrawLineEx((Vector2){cx + sway * 0.2f, cyb - R * 0.95f},
                   (Vector2){cx + sway, cyb - R * (1.25f + i * 0.07f)},
                   R * 0.075f, (Color){255, 25, 35, (uchar)(245 - i * 12)});
    }
    DrawRectangleRounded((Rectangle){cx - R * 0.12f, cyb - R * 1.05f, R * 0.24f, R * 0.12f}, 0.3f, 4, gold);

    // chest medallion
    DrawCircleV((Vector2){cx, cyb + R * 0.08f}, R * 0.16f, gold);
    DrawCircleV((Vector2){cx, cyb + R * 0.08f}, R * 0.10f, crimson);
    DrawCircleV((Vector2){cx, cyb + R * 0.08f}, R * 0.04f, goldHi);

    for (int k = 0; k < 8; k++) {
        float pr = fmodf(t * 0.38f + Rnd(k + 55), 1.0f);
        DrawCircleV((Vector2){cx + (Rnd(k + 56) - 0.5f) * R * 1.4f, cyb + R * 0.5f - pr * R * 1.5f},
                    R * 0.03f, (Color){255, 80, 40, (uchar)(130 * (1.0f - pr))});
    }
}

// ─── ONI-IRON RONIN — Shadow Ronin in Bloodscale craft ───────────────────────
// Dark lacquered mini-scales, kabuto, cyan katana trail, indigo sode.

// Back-mounted twin swords: only the wrapped grips and pommels emerge above
// the shoulders; the blades stay completely hidden behind the torso.
static void DrawBackTwinGrips(float cx, float cyb, float R,
                              Color outline, Color grip, Color gripHi, Color accent) {
    Vector2 bases[2] = {
        {cx - R * 0.36f, cyb - R * 0.22f},
        {cx + R * 0.36f, cyb - R * 0.22f}
    };
    Vector2 pommels[2] = {
        {cx - R * 0.98f, cyb - R * 1.03f},
        {cx + R * 0.98f, cyb - R * 1.03f}
    };

    for (int sword = 0; sword < 2; sword++) {
        Vector2 base = bases[sword];
        Vector2 pommel = pommels[sword];
        Vector2 delta = {pommel.x - base.x, pommel.y - base.y};
        float len = sqrtf(delta.x * delta.x + delta.y * delta.y);
        Vector2 dir = {delta.x / len, delta.y / len};
        Vector2 normal = {-dir.y, dir.x};

        DrawLineEx(base, pommel, R * 0.18f, outline);
        DrawLineEx(base, pommel, R * 0.10f, grip);
        DrawLineEx((Vector2){base.x + dir.x * len * 0.16f,
                            base.y + dir.y * len * 0.16f},
                   (Vector2){pommel.x - dir.x * len * 0.10f,
                            pommel.y - dir.y * len * 0.10f},
                   R * 0.035f, gripHi);

        // Wrapped grip bands.
        for (int band = 1; band <= 3; band++) {
            float u = 0.28f + band * 0.16f;
            Vector2 center = {base.x + dir.x * len * u,
                              base.y + dir.y * len * u};
            DrawLineEx((Vector2){center.x - normal.x * R * 0.11f,
                                 center.y - normal.y * R * 0.11f},
                       (Vector2){center.x + normal.x * R * 0.11f,
                                 center.y + normal.y * R * 0.11f},
                       R * 0.040f, gripHi);
        }

        // Crossguard and pommel are mostly hidden behind the shoulders.
        DrawLineEx((Vector2){base.x - normal.x * R * 0.18f,
                             base.y - normal.y * R * 0.18f},
                   (Vector2){base.x + normal.x * R * 0.18f,
                             base.y + normal.y * R * 0.18f},
                   R * 0.075f, outline);
        DrawLineEx((Vector2){base.x - normal.x * R * 0.14f,
                             base.y - normal.y * R * 0.14f},
                   (Vector2){base.x + normal.x * R * 0.14f,
                             base.y + normal.y * R * 0.14f},
                   R * 0.035f, accent);
        DrawCircleV(pommel, R * 0.12f, outline);
        DrawCircleV(pommel, R * 0.065f, accent);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// NEW VARIANT: Bloodscale Vanguard with the shield rendered in front of the hand.
static void Skin_BloodscaleFrontguard(float cx, float cy, float R, float t) {
    float p  = 0.5f + 0.5f * sinf(t * 2.6f);
    float p2 = 0.5f + 0.5f * sinf(t * 3.4f + 1.2f);
    float bob = sinf(t * 1.4f) * R * 0.03f;
    float cyb = cy + bob;

    // Palette â€” mini dark-red scales + gold trim + cold steel
    Color scBase = (Color){55, 8, 14, 255};     // deepest scale well
    Color scMid  = (Color){95, 14, 22, 255};     // main dark red
    Color scLit  = (Color){140, 28, 38, 255};    // scale face
    Color scHi   = (Color){190, 50, 55, 255};    // rim glint
    Color scHot  = (Color){220, 70, 60, 255};    // living pulse edge
    Color gold   = (Color){220, 175, 55, 255};
    Color goldHi = (Color){255, 230, 140, 255};
    Color voidB  = (Color){12, 6, 10, 255};
    Color shieldBack = (Color){34, 5, 10, 255};
    Color shieldFace = (Color){112, 20, 30, 255};

    // Soft blood-ember aura
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cyb}, R * (1.55f + 0.08f * p),
                (Color){180, 20, 30, (uchar)(22 + 18 * p)});
    DrawCircleV((Vector2){cx, cyb - R * 0.2f}, R * 1.1f,
                (Color){255, 40, 50, (uchar)(10 + 12 * p2)});
    EndBlendMode();
    DrawBackTwinGrips(cx, cyb, R, voidB, scBase, scHi, scHot);

    // â”€â”€ Heavy half-cape (left rear) â€” free form break â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    for (int i = 0; i < 8; i++) {
        float u = i / 7.0f;
        float sway = sinf(t * 2.0f + i * 0.55f) * R * (0.12f + u * 0.18f);
        float mx = cx - R * 0.55f + sway - u * R * 0.15f;
        float my = cyb + R * 0.15f + u * R * 1.15f;
        EllipseRotF(mx, my,
                    R * (0.55f - u * 0.18f), R * (0.30f - u * 0.06f),
                    0.4f + u * 0.35f + sinf(t + i) * 0.08f,
                    (Color){(uchar)(40 + i * 2), 4, 10, (uchar)(230 - i * 16)});
        // subtle gold thread on top folds
        if (i < 3)
            EllipseRotF(mx - R * 0.05f, my - R * 0.04f,
                        R * 0.18f, R * 0.06f, 0.5f,
                        (Color){180, 130, 40, (uchar)(60 - i * 15)});
    }



    // â”€â”€ Body under-shell (silhouette base for scales) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    DrawEllipse((int)cx, (int)(cyb + R * 0.08f), (int)(R * 1.02f), (int)(R * 1.10f), scBase);
    DrawEllipse((int)cx, (int)cyb, (int)(R * 0.92f), (int)(R * 0.98f), scMid);

    // â”€â”€ FULL mini-scale mail (the hero detail) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    // Staggered rows of tiny teardrop/oval scales covering torso + shoulders.
    for (int row = 0; row < 14; row++) {
        float u = row / 13.0f;
        // torso band â€” wider mid, taper top/bottom slightly
        float yw = cyb - R * 0.78f + u * R * 1.70f;
        float halfW = R * (0.78f + 0.18f * sinf(u * PI) - fabsf(u - 0.55f) * 0.15f);
        int cols = 9 + (row % 2);
        for (int col = 0; col < cols; col++) {
            float v = (cols <= 1) ? 0.5f : col / (float)(cols - 1);
            float stagger = (row % 2) ? (0.5f / cols) : 0.0f;
            float xw = cx - halfW + (v + stagger) * halfW * 2.0f;
            // keep roughly in body oval
            float dx = (xw - cx) / (R * 0.95f);
            float dy = (yw - cyb) / (R * 1.02f);
            if (dx * dx + dy * dy > 1.05f) continue;

            float rr = R * (0.065f + 0.012f * sinf(u * 6.0f + col));
            // slight scale tilt
            float ang = (v - 0.5f) * 0.35f;
            // color variation + living pulse on a few scales
            int hash = (row * 17 + col * 31) & 7;
            Color face = scLit;
            if (hash == 0) face = scMid;
            if (hash == 1) face = scHi;
            if (hash == 2 && p > 0.65f) face = scHot;

            EllipseRotF(xw, yw, rr, rr * 1.25f, ang, face);
            // underside shadow of scale
            EllipseRotF(xw, yw + rr * 0.35f, rr * 0.85f, rr * 0.40f, ang, scBase);
            // top glint
            EllipseRotF(xw - rr * 0.15f, yw - rr * 0.35f,
                        rr * 0.40f, rr * 0.28f, ang - 0.2f,
                        (Color){scHi.r, scHi.g, scHi.b, (uchar)(90 + 50 * p2)});
            // micro gold pin on every 5th scale (rivet)
            if ((row + col) % 5 == 0)
                DrawCircleV((Vector2){xw, yw - rr * 0.15f}, rr * 0.18f,
                            (Color){gold.r, gold.g, gold.b, 180});
        }
    }

    // â”€â”€ Spined pauldrons (big free-form scales) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    for (int s = -1; s <= 1; s += 2) {
        float px = cx + s * R * 1.05f;
        float py = cyb - R * 0.05f;
        // pauldron dome
        DrawEllipse((int)px, (int)py, (int)(R * 0.48f), (int)(R * 0.38f), scBase);
        DrawEllipse((int)px, (int)(py - R * 0.05f), (int)(R * 0.40f), (int)(R * 0.30f), scMid);
        // layered large scales on pauldron
        for (int k = 0; k < 6; k++) {
            float ox = px + s * (k - 2.5f) * R * 0.08f;
            float oy = py - R * 0.05f + fabsf((float)k - 2.5f) * R * 0.04f;
            EllipseRotF(ox, oy, R * 0.14f, R * 0.18f, s * 0.25f + (k - 2.5f) * 0.1f, scLit);
            EllipseRotF(ox - s * R * 0.03f, oy - R * 0.05f, R * 0.06f, R * 0.05f,
                        s * 0.2f, (Color){scHi.r, scHi.g, scHi.b, 160});
        }
        // upward spine spikes from pauldron
        for (int k = 0; k < 3; k++) {
            float ox = px + s * (k - 1) * R * 0.12f;
            float h  = R * (0.35f + 0.12f * (k == 1 ? 1.0f : 0.0f) + 0.04f * sinf(t * 3.0f + k));
            DrawTriangle((Vector2){ox - R * 0.07f, py - R * 0.15f},
                         (Vector2){ox + R * 0.07f, py - R * 0.15f},
                         (Vector2){ox + s * R * 0.04f, py - R * 0.15f - h}, scBase);
            DrawTriangle((Vector2){ox - R * 0.035f, py - R * 0.18f},
                         (Vector2){ox + R * 0.035f, py - R * 0.18f},
                         (Vector2){ox + s * R * 0.02f, py - R * 0.18f - h * 0.85f}, scHi);
        }
        // gold pauldron rim
        HaloArc(px, py, R * 0.42f, R * 0.32f, -2.2f, 0.4f, R * 0.04f, gold);
    }

    // â”€â”€ Belt + tassets (scale skirt) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    DrawRectangleRounded((Rectangle){cx - R * 0.70f, cyb + R * 0.55f, R * 1.40f, R * 0.16f},
                         0.4f, 4, gold);
    DrawRectangleRounded((Rectangle){cx - R * 0.65f, cyb + R * 0.58f, R * 1.30f, R * 0.10f},
                         0.4f, 4, goldHi);
    // hanging scale tassets
    for (int k = -3; k <= 3; k++) {
        float tx = cx + k * R * 0.18f;
        float ty = cyb + R * 0.78f;
        float sway = sinf(t * 2.5f + k * 0.7f) * R * 0.04f;
        EllipseRotF(tx + sway, ty, R * 0.12f, R * 0.18f, 0.05f * k, scLit);
        EllipseRotF(tx + sway, ty + R * 0.12f, R * 0.10f, R * 0.14f, 0.05f * k, scMid);
        DrawCircleV((Vector2){tx + sway, ty - R * 0.06f}, R * 0.025f, gold);
    }

    // â”€â”€ Great helm â€” tall free silhouette, scale-faced â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    // neck gorget scales
    for (int k = 0; k < 5; k++) {
        float a = -0.6f + k * 0.3f;
        float gx = cx + sinf(a) * R * 0.45f;
        float gy = cyb - R * 0.55f + cosf(a) * R * 0.08f;
        EllipseRotF(gx, gy, R * 0.12f, R * 0.10f, a, scLit);
    }
    // helm bowl
    DrawEllipse((int)cx, (int)(cyb - R * 0.85f), (int)(R * 0.72f), (int)(R * 0.62f), scBase);
    DrawEllipse((int)cx, (int)(cyb - R * 0.90f), (int)(R * 0.62f), (int)(R * 0.52f), scMid);
    // mini scales on helm
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 6; col++) {
            float u = row / 4.0f, v = col / 5.0f;
            float hx = cx - R * 0.45f + v * R * 0.90f + ((row % 2) ? R * 0.06f : 0.0f);
            float hy = cyb - R * 1.15f + u * R * 0.55f;
            float dx = (hx - cx) / (R * 0.58f);
            float dy = (hy - (cyb - R * 0.90f)) / (R * 0.48f);
            if (dx * dx + dy * dy > 1.0f) continue;
            DrawEllipse((int)hx, (int)hy, (int)(R * 0.055f), (int)(R * 0.065f), scLit);
            DrawEllipse((int)(hx - R * 0.01f), (int)(hy - R * 0.02f),
                        (int)(R * 0.022f), (int)(R * 0.018f),
                        (Color){scHi.r, scHi.g, scHi.b, 150});
        }
    }
    // visor slit
    DrawRectangleRounded((Rectangle){cx - R * 0.42f, cyb - R * 0.78f, R * 0.84f, R * 0.16f},
                         0.4f, 4, voidB);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawRectangleRounded((Rectangle){cx - R * 0.36f, cyb - R * 0.74f, R * 0.72f, R * 0.08f},
                         0.4f, 4, (Color){255, 50, 40, (uchar)(160 + 70 * p)});
    // eye glints in visor
    DrawCircleV((Vector2){cx - R * 0.16f, cyb - R * 0.70f}, R * 0.04f,
                (Color){255, 120, 80, (uchar)(180 + 50 * p)});
    DrawCircleV((Vector2){cx + R * 0.16f, cyb - R * 0.70f}, R * 0.04f,
                (Color){255, 120, 80, (uchar)(180 + 50 * p)});
    EndBlendMode();
    // gold brow band
    DrawRectangleRounded((Rectangle){cx - R * 0.55f, cyb - R * 1.05f, R * 1.10f, R * 0.10f},
                         0.3f, 4, gold);
    DrawRectangleRounded((Rectangle){cx - R * 0.50f, cyb - R * 1.03f, R * 1.00f, R * 0.05f},
                         0.3f, 4, goldHi);

    // Tall crest / plume of rigid scale-fins (not soft feather)
    for (int i = 0; i < 7; i++) {
        float sway = sinf(t * 3.5f + i * 0.45f) * R * 0.06f;
        float hx = cx + sway + (i - 3) * R * 0.04f;
        float baseY = cyb - R * 1.20f;
        float h = R * (0.55f + 0.25f * sinf(i * 0.9f) + 0.05f * sinf(t * 4.0f + i));
        DrawTriangle((Vector2){hx - R * 0.07f, baseY},
                     (Vector2){hx + R * 0.07f, baseY},
                     (Vector2){hx, baseY - h}, scBase);
        DrawTriangle((Vector2){hx - R * 0.035f, baseY - R * 0.02f},
                     (Vector2){hx + R * 0.035f, baseY - R * 0.02f},
                     (Vector2){hx, baseY - h * 0.9f},
                     (i % 2) ? scHi : scHot);
    }
    // crest gold spine
    DrawLineEx((Vector2){cx, cyb - R * 1.18f},
               (Vector2){cx + sinf(t * 2.0f) * R * 0.03f, cyb - R * 1.70f},
               R * 0.035f, gold);

    // â”€â”€ Chest emblem â€” larger scale medallion â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    DrawCircleV((Vector2){cx, cyb + R * 0.05f}, R * 0.22f, gold);
    DrawCircleV((Vector2){cx, cyb + R * 0.05f}, R * 0.17f, scBase);
    // diamond of 4 big scales
    for (int k = 0; k < 4; k++) {
        float a = -PI * 0.5f + k * (PI * 0.5f) + t * 0.15f;
        float ex = cx + cosf(a) * R * 0.07f;
        float ey = cyb + R * 0.05f + sinf(a) * R * 0.07f;
        EllipseRotF(ex, ey, R * 0.06f, R * 0.08f, a, scHot);
    }
    DrawCircleV((Vector2){cx, cyb + R * 0.05f}, R * 0.04f, goldHi);

    // â”€â”€ Ember motes rising off the armor â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    for (int k = 0; k < 10; k++) {
        float pr = fmodf(t * 0.35f + Rnd(k + 60), 1.0f);
        float sx = cx + (Rnd(k + 70) - 0.5f) * R * 1.6f;
        float sy = cyb + R * 0.6f - pr * R * 1.8f;
        uchar al = (uchar)(160 * (1.0f - pr));
        if (k % 3 == 0)
            DrawCircleV((Vector2){sx, sy}, R * (0.04f - pr * 0.02f),
                        (Color){255, 80, 40, al});
        else
            DrawCircleV((Vector2){sx, sy}, R * (0.025f - pr * 0.01f),
                        (Color){200, 40, 30, al});
    }

    // â”€â”€ Kite shield (left) â€” scale-faced with boss â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        float sx = cx - R * 1.35f, sy = cyb + R * 0.10f;
        // shield body silhouette
        DrawTriangle((Vector2){sx - R * 0.38f, sy - R * 0.95f},
                     (Vector2){sx + R * 0.42f, sy - R * 0.95f},
                     (Vector2){sx + R * 0.02f, sy + R * 1.05f}, shieldBack);
        DrawTriangle((Vector2){sx - R * 0.30f, sy - R * 0.85f},
                     (Vector2){sx + R * 0.34f, sy - R * 0.85f},
                     (Vector2){sx + R * 0.02f, sy + R * 0.88f}, shieldFace);
        // mini scales on shield face (staggered rows)
        for (int row = 0; row < 11; row++) {
            float u = row / 10.0f;
            float yw = sy - R * 0.75f + u * R * 1.50f;
            float halfW = R * (0.28f - u * 0.18f);
            int cols = 4 + (row % 2);
            for (int col = 0; col < cols; col++) {
                float v = (cols <= 1) ? 0.5f : col / (float)(cols - 1);
                float xw = sx - halfW + v * halfW * 2.0f + ((row % 2) ? R * 0.03f : 0.0f);
                float rr = R * (0.055f - u * 0.015f);
                Color sc = ((row + col) % 3 == 0) ? scLit : scMid;
                DrawEllipse((int)xw, (int)yw, (int)rr, (int)(rr * 1.15f), sc);
                DrawEllipse((int)xw, (int)(yw - rr * 0.25f), (int)(rr * 0.45f), (int)(rr * 0.35f),
                            (Color){scHi.r, scHi.g, scHi.b, 255});
            }
        }
        // gold rim + boss
        DrawLineEx((Vector2){sx - R * 0.30f, sy - R * 0.85f},
                   (Vector2){sx + R * 0.02f, sy + R * 0.88f}, R * 0.045f, gold);
        DrawLineEx((Vector2){sx + R * 0.34f, sy - R * 0.85f},
                   (Vector2){sx + R * 0.02f, sy + R * 0.88f}, R * 0.045f, gold);
        DrawLineEx((Vector2){sx - R * 0.30f, sy - R * 0.85f},
                   (Vector2){sx + R * 0.34f, sy - R * 0.85f}, R * 0.05f, gold);
        DrawCircleV((Vector2){sx + R * 0.02f, sy - R * 0.05f}, R * 0.14f, gold);
        DrawCircleV((Vector2){sx + R * 0.02f, sy - R * 0.05f}, R * 0.07f, goldHi);
        DrawCircleV((Vector2){sx + R * 0.00f, sy - R * 0.09f}, R * 0.03f, (Color){255, 255, 220, 220});
    }


}

// NEW VARIANT: Stormforged Aegis with the shield rendered in front of the hand.
static void Skin_StormfrontAegis(float cx, float cy, float R, float t) {
    float p  = 0.5f + 0.5f * sinf(t * 2.8f);
    float p2 = 0.5f + 0.5f * sinf(t * 3.5f + 1.0f);
    float bob = sinf(t * 1.5f) * R * 0.03f;
    float cyb = cy + bob;

    Color scBase = (Color){25, 40, 75, 255};
    Color scMid  = (Color){45, 75, 140, 255};
    Color scLit  = (Color){70, 115, 200, 255};
    Color scHi   = (Color){120, 170, 240, 255};
    Color scHot  = (Color){100, 220, 255, 255};
    Color gold   = (Color){230, 185, 50, 255};
    Color goldHi = (Color){255, 235, 140, 255};
    Color voidB  = (Color){8, 12, 22, 255};

    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cyb}, R * (1.50f + 0.08f * p),
                (Color){40, 100, 255, (uchar)(22 + 18 * p)});
    EndBlendMode();
    DrawBackTwinGrips(cx, cyb, R, voidB, scBase, scHi, scHot);

    // Soft rear banner
    for (int i = 0; i < 6; i++) {
        float u = i / 5.0f;
        float sway = sinf(t * 2.2f + i * 0.5f) * R * (0.10f + u * 0.14f);
        EllipseRotF(cx - R * 0.40f + sway, cyb + R * 0.25f + u * R * 1.0f,
                    R * (0.50f - u * 0.15f), R * 0.28f, 0.35f + u * 0.2f,
                    (Color){20, 35, 80, (uchar)(220 - i * 20)});
    }



    DrawEllipse((int)cx, (int)(cyb + R * 0.08f), (int)(R * 1.02f), (int)(R * 1.10f), scBase);
    DrawEllipse((int)cx, (int)cyb, (int)(R * 0.92f), (int)(R * 0.98f), scMid);
    DrawMiniScaleMail(cx, cyb, R, p, p2, scBase, scMid, scLit, scHi, scHot, gold);

    // Pauldrons â€” steel plates + spikes
    for (int s = -1; s <= 1; s += 2) {
        float px = cx + s * R * 1.05f, py = cyb - R * 0.05f;
        DrawEllipse((int)px, (int)py, (int)(R * 0.48f), (int)(R * 0.38f), scBase);
        DrawEllipse((int)px, (int)(py - R * 0.05f), (int)(R * 0.40f), (int)(R * 0.30f), scLit);
        for (int k = 0; k < 5; k++) {
            float ox = px + s * (k - 2.0f) * R * 0.09f;
            EllipseRotF(ox, py - R * 0.02f, R * 0.12f, R * 0.16f, s * 0.2f, scHi);
        }
        DrawTriangle((Vector2){px - R * 0.10f, py - R * 0.15f},
                     (Vector2){px + R * 0.10f, py - R * 0.15f},
                     (Vector2){px + s * R * 0.05f, py - R * 0.55f}, scHi);
        HaloArc(px, py, R * 0.42f, R * 0.32f, -2.2f, 0.4f, R * 0.035f, gold);
    }

    // Belt + tassets
    DrawRectangleRounded((Rectangle){cx - R * 0.70f, cyb + R * 0.55f, R * 1.40f, R * 0.14f}, 0.4f, 4, gold);
    for (int k = -3; k <= 3; k++) {
        float sway = sinf(t * 2.4f + k * 0.6f) * R * 0.035f;
        EllipseRotF(cx + k * R * 0.18f + sway, cyb + R * 0.78f,
                    R * 0.11f, R * 0.16f, 0.05f * k, scLit);
    }

    // Great helm + cyan visor + crimson plume
    DrawEllipse((int)cx, (int)(cyb - R * 0.85f), (int)(R * 0.72f), (int)(R * 0.62f), scBase);
    DrawEllipse((int)cx, (int)(cyb - R * 0.90f), (int)(R * 0.60f), (int)(R * 0.50f), scMid);
    for (int row = 0; row < 4; row++)
        for (int col = 0; col < 5; col++) {
            float hx = cx - R * 0.40f + col * R * 0.20f + ((row % 2) ? R * 0.05f : 0);
            float hy = cyb - R * 1.10f + row * R * 0.14f;
            float dx = (hx - cx) / (R * 0.55f), dy = (hy - (cyb - R * 0.90f)) / (R * 0.45f);
            if (dx * dx + dy * dy > 1.0f) continue;
            DrawEllipse((int)hx, (int)hy, (int)(R * 0.05f), (int)(R * 0.055f), scLit);
        }
    DrawRectangleRounded((Rectangle){cx - R * 0.42f, cyb - R * 0.78f, R * 0.84f, R * 0.16f}, 0.4f, 4, voidB);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawRectangleRounded((Rectangle){cx - R * 0.36f, cyb - R * 0.74f, R * 0.72f, R * 0.08f},
                         0.4f, 4, (Color){80, 220, 255, (uchar)(170 + 60 * p)});
    DrawCircleV((Vector2){cx - R * 0.16f, cyb - R * 0.70f}, R * 0.04f,
                (Color){150, 240, 255, (uchar)(180 + 50 * p)});
    DrawCircleV((Vector2){cx + R * 0.16f, cyb - R * 0.70f}, R * 0.04f,
                (Color){150, 240, 255, (uchar)(180 + 50 * p)});
    EndBlendMode();
    DrawRectangleRounded((Rectangle){cx - R * 0.55f, cyb - R * 1.05f, R * 1.10f, R * 0.10f}, 0.3f, 4, gold);
    // crimson plume
    for (int i = 0; i < 9; i++) {
        float sway = sinf(t * 4.0f + i * 0.4f) * R * 0.08f;
        DrawCircleV((Vector2){cx + sway, cyb - R * (1.22f + i * 0.09f)},
                    R * (0.11f - i * 0.007f),
                    (Color){255, 30, 50, (uchar)(240 - i * 16)});
    }

    // chest boss
    DrawCircleV((Vector2){cx, cyb + R * 0.05f}, R * 0.18f, gold);
    DrawCircleV((Vector2){cx, cyb + R * 0.05f}, R * 0.12f, scBase);
    DrawCircleV((Vector2){cx, cyb + R * 0.05f}, R * 0.05f, goldHi);

    // electric motes
    for (int k = 0; k < 8; k++) {
        float pr = fmodf(t * 0.4f + Rnd(k + 20), 1.0f);
        DrawCircleV((Vector2){cx + (Rnd(k) - 0.5f) * R * 1.5f, cyb + R * 0.5f - pr * R * 1.6f},
                    R * 0.03f, (Color){100, 200, 255, (uchar)(140 * (1.0f - pr))});
    }

    // Tower shield (left)
    {
        float sx = cx - R * 1.40f, sy = cyb + R * 0.05f;
        DrawRectangleRounded((Rectangle){sx - R * 0.38f, sy - R * 1.00f, R * 0.78f, R * 2.05f},
                             0.12f, 8, scBase);
        DrawRectangleRounded((Rectangle){sx - R * 0.30f, sy - R * 0.90f, R * 0.62f, R * 1.85f},
                             0.12f, 8, scMid);
        for (int row = 0; row < 12; row++) {
            float u = row / 11.0f;
            float yw = sy - R * 0.80f + u * R * 1.60f;
            int cols = 4 + (row % 2);
            for (int col = 0; col < cols; col++) {
                float v = col / (float)(cols - 1 + 0.001f);
                float xw = sx - R * 0.22f + v * R * 0.46f + ((row % 2) ? R * 0.04f : 0);
                DrawEllipse((int)xw, (int)yw, (int)(R * 0.055f), (int)(R * 0.065f), scLit);
                DrawEllipse((int)xw, (int)(yw - R * 0.015f), (int)(R * 0.025f), (int)(R * 0.02f),
                            (Color){scHi.r, scHi.g, scHi.b, 140});
            }
        }
        DrawLineEx((Vector2){sx, sy - R * 0.75f}, (Vector2){sx, sy + R * 0.80f}, R * 0.07f, gold);
        DrawCircleV((Vector2){sx, sy}, R * 0.16f, gold);
        DrawCircleV((Vector2){sx, sy}, R * 0.08f, goldHi);
        for (int i = 0; i < 4; i++) {
            float yy = sy - R * 0.55f + i * R * 0.40f;
            DrawCircleV((Vector2){sx - R * 0.18f, yy}, R * 0.035f, gold);
            DrawCircleV((Vector2){sx + R * 0.18f, yy}, R * 0.035f, gold);
        }
    }


}

// NEW VARIANT: Sunforged Crusader with twin swords carried on the back.
static void Skin_SunforgedTwinblades(float cx, float cy, float R, float t) {
    float p  = 0.5f + 0.5f * sinf(t * 2.5f);
    float p2 = 0.5f + 0.5f * sinf(t * 3.2f + 0.7f);
    float bob = sinf(t * 1.4f) * R * 0.03f;
    float cyb = cy + bob;

    Color scBase = (Color){120, 75, 15, 255};
    Color scMid  = (Color){190, 130, 25, 255};
    Color scLit  = (Color){240, 185, 45, 255};
    Color scHi   = (Color){255, 230, 120, 255};
    Color scHot  = (Color){255, 250, 200, 255};
    Color gold   = (Color){255, 210, 60, 255};
    Color goldHi = (Color){255, 245, 180, 255};
    Color white  = (Color){255, 255, 240, 255};
    Color voidB  = (Color){40, 25, 5, 255};

    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cyb}, R * (1.55f + 0.10f * p),
                (Color){255, 180, 30, (uchar)(28 + 20 * p)});
    for (int k = 0; k < 12; k++) {
        float a = k * (2.0f * PI / 12.0f) + t * 0.3f;
        float len = R * (1.30f + 0.20f * sinf(t * 2.8f + k));
        DrawLineEx((Vector2){cx + cosf(a) * R * 0.95f, cyb + sinf(a) * R * 0.95f},
                   (Vector2){cx + cosf(a) * len, cyb + sinf(a) * len},
                   R * 0.05f, (Color){255, 210, 60, 90});
    }
    EndBlendMode();

    // Soft white-gold cape
    for (int i = 0; i < 6; i++) {
        float u = i / 5.0f;
        float sway = sinf(t * 1.8f + i * 0.5f) * R * (0.10f + u * 0.12f);
        EllipseRotF(cx + sway, cyb + R * 0.30f + u * R * 0.95f,
                    R * (0.70f - u * 0.22f), R * 0.28f, 0.1f,
                    (Color){255, (uchar)(200 - i * 8), (uchar)(80 - i * 5), (uchar)(200 - i * 18)});
    }


    // Only the two gold-wrapped grips emerge above the shoulders; blades stay hidden.
    DrawBackTwinGrips(cx, cyb, R, voidB, scBase, scHi, gold);

    DrawEllipse((int)cx, (int)(cyb + R * 0.08f), (int)(R * 1.02f), (int)(R * 1.10f), scBase);
    DrawEllipse((int)cx, (int)cyb, (int)(R * 0.92f), (int)(R * 0.98f), scMid);
    DrawMiniScaleMail(cx, cyb, R, p, p2, scBase, scMid, scLit, scHi, scHot, white);

    // Holy cross over chest
    DrawLineEx((Vector2){cx, cyb - R * 0.35f}, (Vector2){cx, cyb + R * 0.40f}, R * 0.08f, white);
    DrawLineEx((Vector2){cx - R * 0.28f, cyb}, (Vector2){cx + R * 0.28f, cyb}, R * 0.07f, white);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawLineEx((Vector2){cx, cyb - R * 0.30f}, (Vector2){cx, cyb + R * 0.35f}, R * 0.03f,
               (Color){255, 255, 200, (uchar)(120 + 60 * p)});
    EndBlendMode();

    // Gold pauldrons with sun spikes
    for (int s = -1; s <= 1; s += 2) {
        float px = cx + s * R * 1.02f, py = cyb;
        DrawEllipse((int)px, (int)py, (int)(R * 0.50f), (int)(R * 0.38f), scBase);
        DrawEllipse((int)px, (int)(py - R * 0.05f), (int)(R * 0.40f), (int)(R * 0.28f), scLit);
        for (int k = 0; k < 4; k++) {
            float a = -1.0f + k * 0.4f;
            DrawTriangle((Vector2){px - R * 0.08f, py - R * 0.10f},
                         (Vector2){px + R * 0.08f, py - R * 0.05f},
                         (Vector2){px + s * cosf(a) * R * 0.15f + s * R * 0.25f,
                                   py - R * 0.15f - sinf(a + 1.0f) * R * 0.40f},
                         scHi);
        }
        HaloArc(px, py, R * 0.42f, R * 0.30f, -2.0f, 0.5f, R * 0.04f, white);
    }

    DrawRectangleRounded((Rectangle){cx - R * 0.68f, cyb + R * 0.55f, R * 1.36f, R * 0.14f}, 0.4f, 4, gold);
    for (int k = -3; k <= 3; k++) {
        float sway = sinf(t * 2.2f + k) * R * 0.03f;
        EllipseRotF(cx + k * R * 0.17f + sway, cyb + R * 0.78f, R * 0.11f, R * 0.15f, 0, scLit);
    }

    // Helm + sun crest
    DrawEllipse((int)cx, (int)(cyb - R * 0.82f), (int)(R * 0.68f), (int)(R * 0.58f), scBase);
    DrawEllipse((int)cx, (int)(cyb - R * 0.88f), (int)(R * 0.56f), (int)(R * 0.46f), scLit);
    for (int row = 0; row < 4; row++)
        for (int col = 0; col < 5; col++) {
            float hx = cx - R * 0.38f + col * R * 0.19f + ((row % 2) ? R * 0.05f : 0);
            float hy = cyb - R * 1.08f + row * R * 0.13f;
            float dx = (hx - cx) / (R * 0.52f), dy = (hy - (cyb - R * 0.88f)) / (R * 0.42f);
            if (dx * dx + dy * dy > 1.0f) continue;
            DrawEllipse((int)hx, (int)hy, (int)(R * 0.05f), (int)(R * 0.055f), scHi);
        }
    DrawRectangleRounded((Rectangle){cx - R * 0.40f, cyb - R * 0.72f, R * 0.80f, R * 0.14f}, 0.4f, 4, voidB);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawRectangleRounded((Rectangle){cx - R * 0.34f, cyb - R * 0.68f, R * 0.68f, R * 0.07f},
                         0.4f, 4, (Color){255, 245, 120, (uchar)(180 + 50 * p)});
    EndBlendMode();
    // sun disc crest
    float scx = cx, scy = cyb - R * 1.25f;
    DrawCircleV((Vector2){scx, scy}, R * 0.22f, gold);
    DrawCircleV((Vector2){scx, scy}, R * 0.14f, goldHi);
    for (int k = 0; k < 10; k++) {
        float a = k * (2.0f * PI / 10.0f) + t * 0.5f;
        DrawLineEx((Vector2){scx + cosf(a) * R * 0.18f, scy + sinf(a) * R * 0.18f},
                   (Vector2){scx + cosf(a) * R * 0.42f, scy + sinf(a) * R * 0.42f},
                   R * 0.04f, gold);
    }
    DrawCircleV((Vector2){scx, scy}, R * 0.06f, white);

    // golden motes
    for (int k = 0; k < 10; k++) {
        float pr = fmodf(t * 0.35f + Rnd(k + 40), 1.0f);
        DrawCircleV((Vector2){cx + (Rnd(k + 41) - 0.5f) * R * 1.5f, cyb + R * 0.4f - pr * R * 1.7f},
                    R * (0.035f - pr * 0.015f),
                    (Color){255, 220, 80, (uchar)(160 * (1.0f - pr))});
    }
}

// NEW SKIN: the supplied PNG itself, cropped to its opaque sprite bounds.
// Point filtering preserves the original hard pixel edges at every picker size.
static Texture2D poptartCatTexture = {0};
static bool poptartCatTextureLoaded = false;

static void Skin_PoptartCat(float cx, float cy, float R, float t) {
    float cyb = cy + sinf(t * 1.8f) * R * 0.02f;
    if (!poptartCatTextureLoaded) {
        poptartCatTexture = LoadTexture("Nyan-Cat-PNG-Transparent.png");
        if (poptartCatTexture.id > 0) SetTextureFilter(poptartCatTexture, TEXTURE_FILTER_POINT);
        poptartCatTextureLoaded = true;
    }

    if (poptartCatTexture.id > 0) {
        // Opaque bounds measured from the supplied 900x520 PNG.
        Rectangle source = {192.0f, 113.0f, 479.0f, 270.0f};
        float width = R * 4.35f;
        float height = R * 2.45f;
        Rectangle dest = {cx - width * 0.5f, cyb - height * 0.5f, width, height};
        DrawTexturePro(poptartCatTexture, source, dest, (Vector2){0, 0}, 0.0f, WHITE);
    }
}
// ADDITIONAL KNIGHT / WARRIOR CONCEPTS
// These silhouettes intentionally use different roles: immovable tower-shield
// tank, airborne lancer, oversized hammer colossus, ice-axe vanguard and void
// greatsword regent.  They are not recolours of the earlier plate/ronin set.
#if 0 /* retired after visual review; new warrior concepts follow below */

static void DrawConceptBlade(Vector2 hilt, Vector2 tip, float width,
                             Color dark, Color edge, Color energy) {
    DrawLineEx(hilt, tip, width, dark);
    DrawLineEx(hilt, tip, width * 0.46f, edge);
    if (energy.a > 0) {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawLineEx(hilt, tip, width * 0.14f, energy);
        EndBlendMode();
    }
}

static void Skin_ThornwallBastion(float cx, float cy, float R, float t) {
    float p = 0.5f + 0.5f * sinf(t * 2.0f);
    float bob = sinf(t * 1.6f) * R * 0.025f;
    float cyb = cy + bob;
    Color moss = (Color){30, 74, 66, 255};
    Color plate = (Color){52, 124, 105, 255};
    Color plateHi = (Color){115, 220, 168, 255};
    Color dark = (Color){12, 28, 34, 255};
    Color gold = (Color){240, 190, 70, 255};

    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cyb}, R * (1.46f + p * 0.08f),
                (Color){40, 220, 150, (uchar)(18 + 14 * p)});
    EndBlendMode();

    // Massive tower shield — the main silhouette break.
    DrawRectangleRounded((Rectangle){cx - R * 1.12f, cyb - R * 0.70f,
                                    R * 0.70f, R * 1.46f},
                         0.15f, 8, dark);
    DrawRectangleRounded((Rectangle){cx - R * 1.04f, cyb - R * 0.62f,
                                    R * 0.56f, R * 1.25f},
                         0.13f, 8, moss);
    DrawTriangle((Vector2){cx - R * 1.04f, cyb - R * 0.61f},
                 (Vector2){cx - R * 0.48f, cyb - R * 0.61f},
                 (Vector2){cx - R * 0.76f, cyb - R * 0.95f}, plate);
    DrawRectangleRoundedLines((Rectangle){cx - R * 1.04f, cyb - R * 0.62f,
                                          R * 0.56f, R * 1.25f},
                              0.13f, 8, plateHi);
    DrawLineEx((Vector2){cx - R * 0.90f, cyb - R * 0.51f},
               (Vector2){cx - R * 0.90f, cyb + R * 0.52f}, R * 0.045f,
               (Color){150, 255, 200, 155});
    DrawLineEx((Vector2){cx - R * 0.66f, cyb - R * 0.51f},
               (Vector2){cx - R * 0.66f, cyb + R * 0.52f}, R * 0.035f,
               (Color){150, 255, 200, 110});
    DrawCircleV((Vector2){cx - R * 0.76f, cyb + R * 0.02f}, R * 0.22f, dark);
    DrawCircleV((Vector2){cx - R * 0.76f, cyb + R * 0.02f}, R * 0.13f, gold);
    DrawTriangle((Vector2){cx - R * 0.76f, cyb - R * 0.14f},
                 (Vector2){cx - R * 0.67f, cyb + R * 0.04f},
                 (Vector2){cx - R * 0.85f, cyb + R * 0.04f}, plateHi);
    DrawTriangle((Vector2){cx - R * 0.76f, cyb + R * 0.20f},
                 (Vector2){cx - R * 0.67f, cyb + R * 0.01f},
                 (Vector2){cx - R * 0.85f, cyb + R * 0.01f}, plateHi);

    // Short flanged mace on the opposite side.
    DrawLineEx((Vector2){cx + R * 0.66f, cyb + R * 0.52f},
               (Vector2){cx + R * 1.12f, cyb - R * 0.53f}, R * 0.10f, dark);
    DrawLineEx((Vector2){cx + R * 0.68f, cyb + R * 0.48f},
               (Vector2){cx + R * 1.10f, cyb - R * 0.50f}, R * 0.035f, gold);
    DrawCircleV((Vector2){cx + R * 1.12f, cyb - R * 0.58f}, R * 0.20f, dark);
    for (int k = 0; k < 4; k++) {
        float a = t * 0.4f + k * PI * 0.5f;
        DrawLineEx((Vector2){cx + R * 1.12f + cosf(a) * R * 0.14f,
                            cyb - R * 0.58f + sinf(a) * R * 0.14f},
                   (Vector2){cx + R * 1.12f + cosf(a) * R * 0.28f,
                            cyb - R * 0.58f + sinf(a) * R * 0.28f},
                   R * 0.055f, gold);
    }

    // Broad square body and layered pauldrons.
    DrawRectangleRounded((Rectangle){cx - R * 0.58f, cyb - R * 0.22f,
                                    R * 1.22f, R * 1.18f},
                         0.24f, 8, dark);
    DrawRectangleRounded((Rectangle){cx - R * 0.49f, cyb - R * 0.15f,
                                    R * 1.04f, R * 0.99f},
                         0.20f, 8, plate);
    DrawRectangleRounded((Rectangle){cx - R * 0.27f, cyb - R * 0.05f,
                                    R * 0.47f, R * 0.66f},
                         0.18f, 6, moss);
    DrawLineEx((Vector2){cx - R * 0.20f, cyb + R * 0.02f},
               (Vector2){cx + R * 0.16f, cyb + R * 0.49f}, R * 0.035f, plateHi);
    DrawLineEx((Vector2){cx + R * 0.16f, cyb + R * 0.02f},
               (Vector2){cx - R * 0.20f, cyb + R * 0.49f}, R * 0.035f, plateHi);
    DrawCircleV((Vector2){cx - R * 0.03f, cyb + R * 0.25f}, R * 0.08f, gold);
    DrawCircleV((Vector2){cx - R * 0.69f, cyb - R * 0.12f}, R * 0.30f, moss);
    DrawCircleV((Vector2){cx + R * 0.69f, cyb - R * 0.12f}, R * 0.30f, moss);
    DrawCircleLines((int)(cx - R * 0.69f), (int)(cyb - R * 0.12f), R * 0.27f, plateHi);
    DrawCircleLines((int)(cx + R * 0.69f), (int)(cyb - R * 0.12f), R * 0.27f, plateHi);

    // Helm with a narrow emerald visor.
    DrawRectangleRounded((Rectangle){cx - R * 0.45f, cyb - R * 0.82f,
                                    R * 0.88f, R * 0.66f},
                         0.30f, 8, dark);
    DrawRectangleRounded((Rectangle){cx - R * 0.36f, cyb - R * 0.75f,
                                    R * 0.70f, R * 0.48f},
                         0.25f, 8, plate);
    DrawRectangleRounded((Rectangle){cx - R * 0.37f, cyb - R * 0.50f,
                                    R * 0.74f, R * 0.11f},
                         0.70f, 5, (Color){110, 255, 205, (uchar)(170 + 70 * p)});
    for (int k = -2; k <= 2; k++)
        DrawLineEx((Vector2){cx + k * R * 0.11f, cyb - R * 0.48f},
                   (Vector2){cx + k * R * 0.11f, cyb - R * 0.40f}, R * 0.018f,
                   (Color){10, 45, 45, 230});
    DrawTriangle((Vector2){cx - R * 0.25f, cyb - R * 0.78f},
                 (Vector2){cx, cyb - R * 1.08f},
                 (Vector2){cx + R * 0.22f, cyb - R * 0.78f}, plateHi);
    for (int k = 0; k < 5; k++) {
        float pr = fmodf(t * 0.36f + Rnd(k + 140), 1.0f);
        DrawCircleV((Vector2){cx - R * 1.18f + Rnd(k + 150) * R * 1.95f,
                              cyb + R * 0.95f - pr * R * 1.8f},
                    R * 0.025f, (Color){140, 255, 190, (uchar)(140 * (1.0f - pr))});
    }
}

static void Skin_AstralLancer(float cx, float cy, float R, float t) {
    float p = 0.5f + 0.5f * sinf(t * 2.6f);
    float bob = sinf(t * 1.7f) * R * 0.03f;
    float cyb = cy + bob;
    Color navy = (Color){16, 24, 74, 255};
    Color blue = (Color){38, 76, 160, 255};
    Color cyan = (Color){100, 220, 255, 255};
    Color star = (Color){255, 230, 120, 255};
    Color steel = (Color){148, 180, 230, 255};

    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cyb}, R * (1.44f + 0.10f * p),
                (Color){75, 110, 255, (uchar)(20 + 20 * p)});
    EndBlendMode();

    // Long lance and comet pennant behind the body.
    Vector2 lanceBase = {cx - R * 1.18f, cyb + R * 1.18f};
    Vector2 lanceTip  = {cx + R * 1.28f, cyb - R * 1.30f};
    DrawLineEx(lanceBase, lanceTip, R * 0.14f, navy);
    DrawLineEx(lanceBase, lanceTip, R * 0.045f, steel);
    DrawTriangle((Vector2){lanceTip.x, lanceTip.y - R * 0.23f},
                 (Vector2){lanceTip.x + R * 0.13f, lanceTip.y + R * 0.10f},
                 (Vector2){lanceTip.x - R * 0.13f, lanceTip.y + R * 0.10f}, cyan);
    for (int k = 0; k < 4; k++) {
        float u = k / 3.0f;
        float x = cx - R * 0.76f + u * R * 0.80f;
        float y = cyb - R * 0.98f + u * R * 0.28f;
        DrawTriangle((Vector2){x, y}, (Vector2){x - R * 0.18f, y + R * 0.17f},
                     (Vector2){x - R * 0.08f, y + R * 0.30f},
                     (Color){80, 150, 255, (uchar)(190 - k * 25)});
    }

    // Starry cloak, round buckler and articulated plate body.
    DrawTriangle((Vector2){cx - R * 0.42f, cyb - R * 0.50f},
                 (Vector2){cx - R * 1.02f, cyb + R * 0.96f},
                 (Vector2){cx + R * 0.12f, cyb + R * 0.74f}, navy);
    for (int k = 0; k < 8; k++) {
        float a = t * 0.2f + Rnd(k + 170) * PI * 2.0f;
        float d = R * (0.48f + Rnd(k + 180) * 0.60f);
        Star4(cx - R * 0.34f + cosf(a) * d * 0.65f,
              cyb + R * 0.16f + sinf(a) * d * 0.75f,
              R * 0.045f, (Color){140, 200, 255, 190});
    }
    DrawCircleV((Vector2){cx - R * 0.80f, cyb + R * 0.12f}, R * 0.34f, navy);
    DrawCircleLines((int)(cx - R * 0.80f), (int)(cyb + R * 0.12f), R * 0.30f, cyan);
    Star4(cx - R * 0.80f, cyb + R * 0.12f, R * 0.16f, star);

    EllipseRotF(cx + R * 0.03f, cyb + R * 0.34f, R * 0.60f, R * 0.78f, 0.04f, navy);
    EllipseRotF(cx + R * 0.02f, cyb + R * 0.26f, R * 0.48f, R * 0.63f, 0.02f, blue);
    DrawRectangleRounded((Rectangle){cx - R * 0.30f, cyb - R * 0.10f,
                                    R * 0.55f, R * 0.60f},
                         0.18f, 6, (Color){30, 58, 128, 255});
    DrawLineEx((Vector2){cx - R * 0.24f, cyb + R * 0.05f},
               (Vector2){cx + R * 0.22f, cyb + R * 0.38f}, R * 0.035f, cyan);
    DrawLineEx((Vector2){cx + R * 0.22f, cyb + R * 0.05f},
               (Vector2){cx - R * 0.24f, cyb + R * 0.38f}, R * 0.035f, cyan);

    // Plumed helm and luminous cross visor.
    DrawRectangleRounded((Rectangle){cx - R * 0.42f, cyb - R * 0.90f,
                                    R * 0.82f, R * 0.68f},
                         0.30f, 8, navy);
    DrawRectangleRounded((Rectangle){cx - R * 0.33f, cyb - R * 0.82f,
                                    R * 0.64f, R * 0.50f},
                         0.26f, 8, blue);
    DrawRectangleRounded((Rectangle){cx - R * 0.36f, cyb - R * 0.58f,
                                    R * 0.72f, R * 0.12f},
                         0.8f, 5, (Color){80, 190, 255, (uchar)(180 + 70 * p)});
    DrawLineEx((Vector2){cx, cyb - R * 0.58f},
               (Vector2){cx, cyb - R * 0.46f}, R * 0.035f, star);
    DrawTriangle((Vector2){cx - R * 0.18f, cyb - R * 0.82f},
                 (Vector2){cx + R * 0.06f, cyb - R * 1.28f},
                 (Vector2){cx + R * 0.30f, cyb - R * 0.82f}, steel);
    BeginBlendMode(BLEND_ADDITIVE);
    for (int k = 0; k < 6; k++) {
        float a = t * 0.8f + k * PI / 3.0f;
        float d = R * (1.12f + 0.12f * sinf(t * 2.5f + k));
        DrawCircleV((Vector2){cx + cosf(a) * d, cyb + sinf(a) * d * 0.72f},
                    R * 0.035f, (Color){120, 190, 255, (uchar)(100 + 100 * p)});
    }
    EndBlendMode();
}

static void Skin_CinderColossus(float cx, float cy, float R, float t) {
    float p = 0.5f + 0.5f * sinf(t * 4.0f);
    float bob = sinf(t * 1.2f) * R * 0.025f;
    float cyb = cy + bob;
    Color coal = (Color){24, 18, 24, 255};
    Color iron = (Color){66, 54, 64, 255};
    Color ember = (Color){226, 70, 24, 255};
    Color hot = (Color){255, 210, 68, 255};
    Color soot = (Color){10, 8, 14, 255};

    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cyb + R * 0.10f}, R * (1.48f + p * 0.12f),
                (Color){255, 70, 10, (uchar)(22 + 20 * p)});
    EndBlendMode();

    // Enormous double-headed hammer behind the square silhouette.
    DrawLineEx((Vector2){cx + R * 0.40f, cyb + R * 1.16f},
               (Vector2){cx + R * 1.16f, cyb - R * 0.82f}, R * 0.16f, soot);
    DrawLineEx((Vector2){cx + R * 0.40f, cyb + R * 1.16f},
               (Vector2){cx + R * 1.16f, cyb - R * 0.82f}, R * 0.065f, hot);
    DrawRectangleRounded((Rectangle){cx + R * 0.82f, cyb - R * 1.06f,
                                    R * 0.64f, R * 0.48f},
                         0.14f, 6, soot);
    DrawRectangleRoundedLines((Rectangle){cx + R * 0.82f, cyb - R * 1.06f,
                                          R * 0.64f, R * 0.48f},
                              0.14f, 6, hot);
    DrawTriangle((Vector2){cx + R * 0.90f, cyb - R * 0.60f},
                 (Vector2){cx + R * 0.99f, cyb - R * 0.48f},
                 (Vector2){cx + R * 1.10f, cyb - R * 0.60f}, ember);
    DrawTriangle((Vector2){cx + R * 1.18f, cyb - R * 1.06f},
                 (Vector2){cx + R * 1.30f, cyb - R * 1.18f},
                 (Vector2){cx + R * 1.42f, cyb - R * 1.06f}, ember);

    // Bulky shoulders and a squared furnace chest.
    DrawCircleV((Vector2){cx - R * 0.70f, cyb - R * 0.22f}, R * 0.43f, coal);
    DrawCircleV((Vector2){cx + R * 0.70f, cyb - R * 0.22f}, R * 0.43f, coal);
    DrawCircleLines((int)(cx - R * 0.70f), (int)(cyb - R * 0.22f), R * 0.35f, ember);
    DrawCircleLines((int)(cx + R * 0.70f), (int)(cyb - R * 0.22f), R * 0.35f, ember);
    DrawRectangleRounded((Rectangle){cx - R * 0.62f, cyb - R * 0.36f,
                                    R * 1.26f, R * 1.24f},
                         0.18f, 8, soot);
    DrawRectangleRounded((Rectangle){cx - R * 0.50f, cyb - R * 0.27f,
                                    R * 1.00f, R * 1.02f},
                         0.12f, 8, iron);
    DrawRectangleRounded((Rectangle){cx - R * 0.24f, cyb - R * 0.08f,
                                    R * 0.48f, R * 0.66f},
                         0.10f, 5, coal);
    // Magma seams across the armor.
    DrawLineEx((Vector2){cx - R * 0.20f, cyb - R * 0.03f},
               (Vector2){cx - R * 0.05f, cyb + R * 0.22f}, R * 0.045f, ember);
    DrawLineEx((Vector2){cx - R * 0.05f, cyb + R * 0.22f},
               (Vector2){cx + R * 0.20f, cyb + R * 0.05f}, R * 0.045f, hot);
    DrawLineEx((Vector2){cx + R * 0.05f, cyb + R * 0.22f},
               (Vector2){cx - R * 0.12f, cyb + R * 0.54f}, R * 0.035f, ember);
    DrawCircleV((Vector2){cx + R * 0.04f, cyb + R * 0.22f}, R * 0.07f,
                (Color){255, 225, 100, (uchar)(170 + 85 * p)});

    // Furnace helm, horned crest and molten eye slits.
    DrawRectangleRounded((Rectangle){cx - R * 0.46f, cyb - R * 0.94f,
                                    R * 0.92f, R * 0.72f},
                         0.20f, 8, coal);
    DrawRectangleRounded((Rectangle){cx - R * 0.36f, cyb - R * 0.84f,
                                    R * 0.72f, R * 0.53f},
                         0.16f, 8, iron);
    DrawTriangle((Vector2){cx - R * 0.30f, cyb - R * 0.79f},
                 (Vector2){cx - R * 0.55f, cyb - R * 1.20f},
                 (Vector2){cx - R * 0.05f, cyb - R * 0.89f}, ember);
    DrawTriangle((Vector2){cx + R * 0.05f, cyb - R * 0.89f},
                 (Vector2){cx + R * 0.55f, cyb - R * 1.20f},
                 (Vector2){cx + R * 0.30f, cyb - R * 0.79f}, ember);
    DrawLineEx((Vector2){cx - R * 0.31f, cyb - R * 0.55f},
               (Vector2){cx - R * 0.06f, cyb - R * 0.49f}, R * 0.075f, hot);
    DrawLineEx((Vector2){cx + R * 0.06f, cyb - R * 0.49f},
               (Vector2){cx + R * 0.31f, cyb - R * 0.55f}, R * 0.075f, hot);
    DrawRectangleRounded((Rectangle){cx - R * 0.25f, cyb - R * 0.30f,
                                    R * 0.50f, R * 0.10f},
                         0.5f, 5, (Color){255, 120, 30, 220});
    for (int k = 0; k < 7; k++) {
        float pr = fmodf(t * 0.55f + Rnd(k + 200), 1.0f);
        DrawCircleV((Vector2){cx + (Rnd(k + 201) - 0.5f) * R * 1.8f,
                              cyb + R * 0.95f - pr * R * 2.0f},
                    R * (0.02f + 0.03f * (1.0f - pr)),
                    (Color){255, 170, 50, (uchar)(180 * (1.0f - pr))});
    }
}

static void Skin_FrostveilValkyrie(float cx, float cy, float R, float t) {
    float p = 0.5f + 0.5f * sinf(t * 2.4f);
    float bob = sinf(t * 1.8f) * R * 0.035f;
    float cyb = cy + bob;
    Color ice = (Color){125, 220, 245, 255};
    Color iceHi = (Color){225, 255, 255, 255};
    Color blue = (Color){34, 85, 150, 255};
    Color steel = (Color){130, 165, 205, 255};
    Color dark = (Color){18, 32, 68, 255};

    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cyb}, R * (1.42f + 0.08f * p),
                (Color){80, 220, 255, (uchar)(20 + 16 * p)});
    EndBlendMode();

    // Split frost cloak / wing silhouette.
    DrawTriangle((Vector2){cx - R * 0.32f, cyb - R * 0.45f},
                 (Vector2){cx - R * 1.30f, cyb - R * 0.15f},
                 (Vector2){cx - R * 0.86f, cyb + R * 1.10f}, dark);
    DrawTriangle((Vector2){cx + R * 0.32f, cyb - R * 0.45f},
                 (Vector2){cx + R * 1.30f, cyb - R * 0.15f},
                 (Vector2){cx + R * 0.86f, cyb + R * 1.10f}, dark);
    for (int side = -1; side <= 1; side += 2) {
        for (int k = 0; k < 3; k++) {
            float x = cx + side * R * (0.60f + k * 0.22f);
            float y = cyb + R * (0.18f + k * 0.24f);
            DrawLineEx((Vector2){cx + side * R * 0.34f, cyb - R * 0.24f},
                       (Vector2){x, y}, R * 0.035f, ice);
        }
    }

    // Twin ice axes behind the shoulders.
    for (int side = -1; side <= 1; side += 2) {
        Vector2 h = {cx + side * R * 0.62f, cyb + R * 0.72f};
        Vector2 t0 = {cx + side * R * 1.12f, cyb - R * 0.52f};
        DrawLineEx(h, t0, R * 0.12f, dark);
        DrawLineEx(h, t0, R * 0.045f, iceHi);
        DrawTriangle((Vector2){t0.x, t0.y - R * 0.30f},
                     (Vector2){t0.x + side * R * 0.24f, t0.y + R * 0.04f},
                     (Vector2){t0.x - side * R * 0.18f, t0.y + R * 0.12f}, ice);
        DrawLineEx((Vector2){t0.x + side * R * 0.08f, t0.y - R * 0.16f},
                   (Vector2){t0.x + side * R * 0.12f, t0.y + R * 0.02f},
                   R * 0.025f, (Color){255, 255, 255, 220});
    }

    // Slim layered armor and an icy waist sash.
    EllipseRotF(cx, cyb + R * 0.30f, R * 0.56f, R * 0.78f, 0.0f, dark);
    EllipseRotF(cx, cyb + R * 0.20f, R * 0.44f, R * 0.62f, 0.0f, blue);
    DrawRectangleRounded((Rectangle){cx - R * 0.25f, cyb - R * 0.04f,
                                    R * 0.50f, R * 0.57f},
                         0.16f, 6, (Color){70, 135, 190, 255});
    DrawLineEx((Vector2){cx - R * 0.20f, cyb + R * 0.05f},
               (Vector2){cx + R * 0.23f, cyb + R * 0.38f}, R * 0.035f, iceHi);
    DrawLineEx((Vector2){cx + R * 0.20f, cyb + R * 0.05f},
               (Vector2){cx - R * 0.23f, cyb + R * 0.38f}, R * 0.035f, iceHi);
    DrawRectangleRounded((Rectangle){cx - R * 0.62f, cyb + R * 0.48f,
                                    R * 1.24f, R * 0.15f},
                         0.35f, 6, (Color){100, 230, 255, 210});
    DrawCircleV((Vector2){cx, cyb + R * 0.54f}, R * 0.08f, iceHi);

    // Frostveil helm with a long central crest.
    DrawRectangleRounded((Rectangle){cx - R * 0.39f, cyb - R * 0.87f,
                                    R * 0.78f, R * 0.68f},
                         0.38f, 8, dark);
    DrawRectangleRounded((Rectangle){cx - R * 0.30f, cyb - R * 0.79f,
                                    R * 0.60f, R * 0.50f},
                         0.30f, 8, steel);
    DrawRectangleRounded((Rectangle){cx - R * 0.33f, cyb - R * 0.57f,
                                    R * 0.66f, R * 0.11f},
                         0.75f, 5, (Color){30, 65, 120, 255});
    DrawLineEx((Vector2){cx - R * 0.22f, cyb - R * 0.51f},
               (Vector2){cx - R * 0.04f, cyb - R * 0.51f}, R * 0.025f, iceHi);
    DrawLineEx((Vector2){cx + R * 0.04f, cyb - R * 0.51f},
               (Vector2){cx + R * 0.22f, cyb - R * 0.51f}, R * 0.025f, iceHi);
    DrawTriangle((Vector2){cx - R * 0.18f, cyb - R * 0.80f},
                 (Vector2){cx, cyb - R * 1.35f},
                 (Vector2){cx + R * 0.18f, cyb - R * 0.80f}, ice);
    for (int k = 0; k < 9; k++) {
        float pr = fmodf(t * 0.32f + Rnd(k + 230), 1.0f);
        DrawCircleV((Vector2){cx + (Rnd(k + 231) - 0.5f) * R * 2.4f,
                              cyb + R * 1.10f - pr * R * 2.4f},
                    R * 0.025f, (Color){180, 250, 255, (uchar)(170 * (1.0f - pr))});
    }
}

static void Skin_VoidbladeRegent(float cx, float cy, float R, float t) {
    float p = 0.5f + 0.5f * sinf(t * 3.0f);
    float bob = sinf(t * 1.4f) * R * 0.025f;
    float cyb = cy + bob;
    Color voidD = (Color){9, 7, 24, 255};
    Color voidM = (Color){31, 18, 75, 255};
    Color violet = (Color){145, 55, 255, 255};
    Color violetHi = (Color){225, 155, 255, 255};
    Color silver = (Color){170, 170, 220, 255};

    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cyb}, R * (1.48f + 0.10f * p),
                (Color){115, 30, 255, (uchar)(20 + 24 * p)});
    EndBlendMode();

    // Floating split cape and a huge violet greatsword.
    DrawTriangle((Vector2){cx - R * 0.30f, cyb - R * 0.45f},
                 (Vector2){cx - R * 1.06f, cyb + R * 1.05f},
                 (Vector2){cx - R * 0.08f, cyb + R * 0.78f}, voidD);
    DrawTriangle((Vector2){cx + R * 0.30f, cyb - R * 0.45f},
                 (Vector2){cx + R * 1.06f, cyb + R * 1.05f},
                 (Vector2){cx + R * 0.08f, cyb + R * 0.78f}, voidD);
    Vector2 hilt = {cx + R * 0.42f, cyb + R * 0.90f};
    Vector2 tip  = {cx + R * 1.22f, cyb - R * 1.26f};
    DrawConceptBlade(hilt, tip, R * 0.20f, voidD, voidM, violet);
    DrawTriangle((Vector2){tip.x, tip.y - R * 0.24f},
                 (Vector2){tip.x + R * 0.16f, tip.y + R * 0.12f},
                 (Vector2){tip.x - R * 0.16f, tip.y + R * 0.12f}, violetHi);
    DrawLineEx((Vector2){cx + R * 0.18f, cyb + R * 0.52f},
               (Vector2){cx + R * 0.69f, cyb + R * 0.72f}, R * 0.07f, silver);
    DrawLineEx((Vector2){cx + R * 0.43f, cyb + R * 0.63f},
               (Vector2){cx + R * 0.45f, cyb + R * 0.90f}, R * 0.06f, violet);

    // High-collar armor: narrow, elegant silhouette distinct from a paladin.
    EllipseRotF(cx, cyb + R * 0.32f, R * 0.52f, R * 0.76f, 0.0f, voidD);
    EllipseRotF(cx, cyb + R * 0.20f, R * 0.40f, R * 0.60f, 0.0f, voidM);
    DrawTriangle((Vector2){cx - R * 0.34f, cyb - R * 0.32f},
                 (Vector2){cx, cyb + R * 0.36f},
                 (Vector2){cx + R * 0.34f, cyb - R * 0.32f}, voidD);
    DrawLineEx((Vector2){cx - R * 0.20f, cyb + R * 0.02f},
               (Vector2){cx, cyb + R * 0.28f}, R * 0.035f, violet);
    DrawLineEx((Vector2){cx, cyb + R * 0.28f},
               (Vector2){cx + R * 0.20f, cyb + R * 0.02f}, R * 0.035f, violet);
    DrawCircleV((Vector2){cx, cyb + R * 0.28f}, R * 0.065f, violetHi);
    DrawCircleV((Vector2){cx - R * 0.58f, cyb - R * 0.20f}, R * 0.27f, voidM);
    DrawCircleV((Vector2){cx + R * 0.58f, cyb - R * 0.20f}, R * 0.27f, voidM);
    DrawCircleLines((int)(cx - R * 0.58f), (int)(cyb - R * 0.20f), R * 0.25f, violet);
    DrawCircleLines((int)(cx + R * 0.58f), (int)(cyb - R * 0.20f), R * 0.25f, violet);

    // Closed crown-helm, single visor slit and a purple crest.
    DrawRectangleRounded((Rectangle){cx - R * 0.40f, cyb - R * 0.92f,
                                    R * 0.80f, R * 0.70f},
                         0.30f, 8, voidD);
    DrawRectangleRounded((Rectangle){cx - R * 0.30f, cyb - R * 0.82f,
                                    R * 0.60f, R * 0.51f},
                         0.24f, 8, voidM);
    DrawRectangleRounded((Rectangle){cx - R * 0.35f, cyb - R * 0.56f,
                                    R * 0.70f, R * 0.13f},
                         0.80f, 5, (Color){115, 30, 220, (uchar)(180 + 70 * p)});
    DrawLineEx((Vector2){cx - R * 0.22f, cyb - R * 0.50f},
               (Vector2){cx + R * 0.22f, cyb - R * 0.50f}, R * 0.028f, violetHi);
    DrawTriangle((Vector2){cx - R * 0.22f, cyb - R * 0.82f},
                 (Vector2){cx - R * 0.06f, cyb - R * 1.28f},
                 (Vector2){cx + R * 0.02f, cyb - R * 0.82f}, violet);
    DrawTriangle((Vector2){cx - R * 0.02f, cyb - R * 0.82f},
                 (Vector2){cx + R * 0.14f, cyb - R * 1.16f},
                 (Vector2){cx + R * 0.23f, cyb - R * 0.82f}, violet);

    for (int k = 0; k < 9; k++) {
        float a = t * 0.9f + k * 0.7f;
        float d = R * (1.12f + 0.16f * sinf(t * 1.9f + k));
        DrawCircleV((Vector2){cx + cosf(a) * d,
                              cyb + sinf(a) * d * 0.70f},
                    R * (0.025f + 0.02f * p),
                    (Color){190, 80, 255, (uchar)(120 + 100 * p)});
    }
}

#endif

// ═══════════════════════════════════════════════════════════════════════════
// NEW WARRIOR CONCEPTS — clean silhouettes, readable weapons, restrained FX.


#define NSKIN 28


typedef void (*DrawFn)(float cx, float cy, float R, float t);

static DrawFn fns[NSKIN] = {
    Skin_AntiqueClock,          // 0
    Skin_TimeLoop,              // 1
    Skin_Blutklinge,            // 2
    Skin_BlackHole,             // 3
    Skin_Apotheosis,            // 4
    Skin_HaloHierarch,          // 5
    Skin_Dawnbringer,           // 6
    Skin_AbyssalEye,            // 7
    Skin_PhoenixPlume,          // 8
    Skin_CrystalPrism,          // 9
    Skin_VoidSerpent,           // 10
    Skin_EclipseWarden,         // 11
    Skin_ApexBladelord,         // 12
    Skin_SkullClassic,           // 13
    Skin_SkullEmber,             // 14
    Skin_SkullVoid,              // 15
    Skin_SkullCrowned,           // 16
    Skin_SkullCrimson,           // 17
    Skin_StarfallUnicorn,        // 18
    Skin_ApexRex,                // 19
    Skin_LegatePraetorian,       // 20
    Skin_SkullVoidCrowned,       // 21
    Skin_PoptartCat,             // 22
    Skin_BloodscaleFrontguard,   // 23
    Skin_StormfrontAegis,        // 24
    Skin_SunforgedTwinblades,    // 25
    Skin_BlackHoleInterstellar,  // 26
    Skin_ApexBladelordSides,     // 27
};

typedef struct { char name[48]; char desc[140]; } SkinMeta;
static SkinMeta skinMeta[NSKIN];
static bool bad[NSKIN];
static bool pick[NSKIN];   // marked for inclusion in the real game

static void TrimNewline(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = '\0';
}

static void LoadSkinData(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        TraceLog(LOG_WARNING, "skin_data.txt not found — names will be blank");
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        TrimNewline(line);
        if (line[0] == '#' || line[0] == '\0') continue;
        if (strncmp(line, "SKIN|", 5) != 0) continue;
        int idx; char name[48]="", desc[140]="";
        char *p = line + 5;
        char *tok = strtok(p, "|"); if (!tok) continue; idx = atoi(tok);
        tok = strtok(NULL, "|"); if (tok) strncpy(name, tok, sizeof(name)-1);
        tok = strtok(NULL, "|"); if (tok) strncpy(desc, tok, sizeof(desc)-1);
        if (idx >= 0 && idx < NSKIN) {
            strncpy(skinMeta[idx].name, name, sizeof(skinMeta[idx].name)-1);
            strncpy(skinMeta[idx].desc, desc, sizeof(skinMeta[idx].desc)-1);
        }
    }
    fclose(f);
}

static void LoadBadSkins(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\0') continue;
        int id;
        if (sscanf(line, "%d", &id) == 1 && id >= 0 && id < NSKIN) bad[id] = true;
    }
    fclose(f);
}

static void SaveBadSkins(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "# bad_skins.txt — rejected skin ideas (auto by skinideas.exe)\n");
    fprintf(f, "# id  name\n");
    for (int i = 0; i < NSKIN; i++)
        if (bad[i]) fprintf(f, "%d  %s\n", i, skinMeta[i].name);
    fclose(f);
}

static void LoadPicks(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\0') continue;
        int id;
        if (sscanf(line, "%d", &id) == 1 && id >= 0 && id < NSKIN) pick[id] = true;
    }
    fclose(f);
}

static void SavePicks(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "# skin_picks.txt — skins marked for the real game (auto by skinideas.exe)\n");
    fprintf(f, "# id  name\n");
    for (int i = 0; i < NSKIN; i++)
        if (pick[i]) fprintf(f, "%d  %s\n", i, skinMeta[i].name);
    fclose(f);
}

static void TogglePick(int id) {
    if (id < 0 || id >= NSKIN || bad[id]) return;
    pick[id] = !pick[id];
    SavePicks("skin_picks.txt");
}

// Canonical names — always applied after load so skin_data.txt can't scramble IDs.
static const char *const BUILTIN_NAME[NSKIN] = {
    "Antique Clock", "Time Loop", "Blutklinge", "Black Hole Core", "Apotheosis",
    "Halo Hierarch", "Dawnbringer", "Abyssal Eye", "Phoenix Plume", "Crystal Prism",
    "Void Serpent", "Eclipse Warden", "Apex Bladelord", "Bone Classic", "Skull Ember",
    "Skull Void", "Skull Crowned", "Skull Crimson", "Starfall Unicorn", "Apex Rex",
    "Legate Praetorian", "Skullvoid Crowned", "Poptart Cat", "Bloodscale Vanguard",
    "Stormforged Aegis", "Sunforged Crusader",
    "Black Hole Interstellar", "Apex Bladelord Splitguard"
};
static const char *const BUILTIN_DESC[NSKIN] = {
    "Bronze dial, rotating hands, tick marks",
    "Ouroboros arrow ring in rainbow",
    "Crimson body, crossed blades, gold eyes",
    "Dark disc with glowing accretion ring",
    "Body dissolves into rising golden light",
    "Three gyroscopic gold halos",
    "Sunrise body with rotating ray crown",
    "Void iris, tentacles, dilating pupil",
    "Fire wings, white-hot core, embers",
    "Splits a white beam into a rainbow fan",
    "Neon serpent coils a dark core",
    "Dark moon eclipses a star",
    "Blade Legend rework: twin greatswords",
    "Bone skull silhouette (classic game art)",
    "Bone skull, ember sockets, rising sparks",
    "Bone skull, violet void sockets",
    "Bone skull + floating gold crown",
    "Bone skull, crimson sockets",
    "Super-cute chubby unicorn: huge eyes, candy horn, hearts, soft mane",
    "T-rex silhouette: long snout, chomp jaw, dorsal spines, tiny arms",
    "Centurion-inspired bronze lorica scales, scutum, gladius, red crest",
    "Completely violet Skullvoid, amethyst sockets, imperial crown and void motes",
    "Pixel-art copy of Nyan-Cat-PNG-Transparent.png with pastry, frosting, cat and rainbow",
    "Bloodscale variant with an opaque front shield and two red-glowing sword grips on the back",
    "Stormforged variant with an opaque front shield and two cyan-glowing sword grips on the back",
    "Sunforged variant with two gold sword grips on the back instead of visible blades",
    "Black Hole copy with a thin photon ring, lensed amber accretion disk and deep event horizon",
    "Apex Bladelord copy with one glowing greatsword carried on each side"
};

int main(void) {
    for (int i = 0; i < NSKIN; i++) { bad[i] = false; pick[i] = false; }
    LoadSkinData("skin_data.txt");
    LoadBadSkins("bad_skins.txt");
    LoadPicks("skin_picks.txt");

    // Always force correct names (prevents scrambled skin_data.txt mappings).
    for (int i = 0; i < NSKIN; i++) {
        strncpy(skinMeta[i].name, BUILTIN_NAME[i], sizeof(skinMeta[i].name) - 1);
        strncpy(skinMeta[i].desc, BUILTIN_DESC[i], sizeof(skinMeta[i].desc) - 1);
        skinMeta[i].name[sizeof(skinMeta[i].name) - 1] = '\0';
        skinMeta[i].desc[sizeof(skinMeta[i].desc) - 1] = '\0';
    }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    int sw = 1280, sh = 800;
    InitWindow(sw, sh, "Skin Ideas — reject + pick for game");
    SetTargetFPS(60);

    const int headerH = 78;
    const int leftM = 28;
    const int cols = 5;
    const int cellW = (sw - leftM * 2) / cols;
    const int cellH = 165;
    const float R = 30.0f;
    int scroll = 0;
    char hoverDesc[160] = "";
    Vector2 hoverPos = {0};
    int hoverId = -1;

    while (!WindowShouldClose()) {
        float t = (float)GetTime();
        Vector2 mp = GetMousePosition();
        bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        bool rclick  = IsMouseButtonPressed(MOUSE_RIGHT_BUTTON);
        hoverDesc[0] = '\0';
        hoverId = -1;

        int vis[NSKIN], nvis = 0;
        for (int i = 0; i < NSKIN; i++) if (!bad[i]) vis[nvis++] = i;
        int nrej = 0, npick = 0;
        for (int i = 0; i < NSKIN; i++) {
            if (bad[i]) nrej++;
            if (pick[i] && !bad[i]) npick++;
        }

        int visRows = (nvis + cols - 1) / cols; if (visRows < 1) visRows = 1;
        int gridH = 36 + visRows * cellH;

        int rejCols = 8;
        int rejRows = (nrej + rejCols - 1) / rejCols; if (rejRows < 1) rejRows = 1;
        int rejH = 56 + rejRows * 90;

        // picked strip height
        int pickCols = 10;
        int pickRows = (npick + pickCols - 1) / pickCols; if (pickRows < 1) pickRows = 1;
        int pickH = 70 + pickRows * 72;

        int rejTop = headerH + gridH + 8;
        int pickTop = rejTop + rejH + 10;
        int contentH = gridH + 8 + rejH + 10 + pickH + 24;
        int maxScroll = contentH - (sh - headerH);
        if (maxScroll < 0) maxScroll = 0;

        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) scroll -= (int)(wheel * 48.0f);
        if (IsKeyDown(KEY_DOWN)) scroll += 8;
        if (IsKeyDown(KEY_UP))   scroll -= 8;
        if (scroll < 0) scroll = 0;
        if (scroll > maxScroll) scroll = maxScroll;

        BeginDrawing();
        ClearBackground((Color){14, 12, 20, 255});

        DrawText("SKIN IDEAS  |  L-click = REJECT  |  hover+P / right-click = PICK for game  |  click rejected to restore",
                 leftM, 10, 15, (Color){230, 225, 240, 255});
        DrawText(TextFormat("%d candidates  /  %d rejected  /  %d PICKED for game   (skin_picks.txt + bad_skins.txt)",
                            nvis, nrej, npick),
                 leftM, 32, 13, (Color){160, 150, 180, 220});

        // compact pick name strip
        int sx = leftM;
        DrawText("PICKS:", sx, 54, 13, (Color){100, 220, 140, 230});
        sx += 55;
        if (npick == 0) {
            DrawText("(none — hover a skin and press P)", sx, 54, 13, (Color){110, 105, 130, 200});
        } else {
            for (int i = 0; i < NSKIN; i++) {
                if (!pick[i] || bad[i]) continue;
                const char *nm = skinMeta[i].name;
                int tw = MeasureText(nm, 12);
                if (sx + tw + 16 > sw - 20) break;
                DrawRectangleRounded((Rectangle){(float)sx, 52, (float)(tw+12), 18},
                                     0.4f, 4, (Color){20, 40, 28, 230});
                DrawText(nm, sx + 6, 54, 12, (Color){140, 255, 180, 240});
                sx += tw + 18;
            }
        }

        BeginScissorMode(0, headerH, sw, sh - headerH);

        int gridTop = headerH - scroll;
        DrawText("CANDIDATES", leftM, gridTop + 6, 14, (Color){200, 190, 220, 220});

        for (int pos = 0; pos < nvis; pos++) {
            int id = vis[pos];
            int col = pos % cols, row = pos / cols;
            int cx = leftM + col * cellW + cellW / 2;
            int cy = gridTop + 60 + row * cellH + 42;

            Rectangle hit = {(float)(cx - R * 1.7f), (float)(cy - R * 1.7f), R * 3.4f, R * 3.4f};
            bool hov = CheckCollisionPointRec(mp, hit) && mp.y >= headerH;

            DrawCircleV((Vector2){(float)cx, (float)cy}, R * 1.7f,
                        hov ? (Color){255, 80, 80, 22}
                            : pick[id] ? (Color){80, 255, 120, 16}
                                       : (Color){255, 255, 255, 8});
            fns[id]((float)cx, (float)cy, R, t);

            int tw = MeasureText(skinMeta[id].name, 13);
            DrawText(skinMeta[id].name, cx - tw / 2, cy + (int)(R * 1.85f), 13,
                     (Color){220, 215, 235, 235});
            DrawText(TextFormat("#%d", id), cx - (int)(R * 1.55f), cy - (int)(R * 1.55f),
                     11, (Color){130, 120, 150, 180});

            if (pick[id]) {
                const char *lb = "IN GAME";
                int bw = MeasureText(lb, 12) + 10;
                int bx = cx - bw / 2, by = cy - (int)(R * 2.05f);
                DrawRectangleRounded((Rectangle){(float)bx, (float)by, (float)bw, 18},
                                     0.4f, 4, (Color){16, 36, 24, 235});
                DrawRectangleRoundedLinesEx((Rectangle){(float)bx, (float)by, (float)bw, 18},
                                            0.4f, 4, 1.5f, (Color){100, 230, 140, 240});
                DrawText(lb, bx + 5, by + 2, 12, (Color){140, 255, 180, 250});
            }

            if (hov) {
                DrawCircleLines(cx, cy, (int)(R * 1.7f),
                                pick[id] ? (Color){100, 255, 150, 220} : (Color){255, 90, 90, 220});
                strncpy(hoverDesc, skinMeta[id].desc, sizeof(hoverDesc) - 1);
                hoverPos = (Vector2){(float)cx, (float)(cy - R * 2.2f)};
                hoverId = id;
                if (clicked) {
                    bad[id] = true;
                    pick[id] = false;
                    SaveBadSkins("bad_skins.txt");
                    SavePicks("skin_picks.txt");
                }
                if (rclick) TogglePick(id);
            }
        }

        // rejected
        int bandTop = rejTop - scroll;
        DrawText("REJECTED  (click to restore)", leftM, bandTop + 6, 14, (Color){225, 140, 140, 225});
        DrawLine(0, bandTop + rejH - 4, sw, bandTop + rejH - 4, (Color){40, 36, 55, 255});

        if (nrej == 0) {
            DrawText("(none yet)", leftM, bandTop + 36, 13, (Color){110, 105, 130, 200});
        } else {
            int idx = 0;
            float rR = 20.0f;
            int rejCellW = (sw - leftM * 2) / rejCols;
            for (int i = 0; i < NSKIN; i++) {
                if (!bad[i]) continue;
                int rc = idx % rejCols, rr = idx / rejCols;
                int cx = leftM + rc * rejCellW + rejCellW / 2;
                int cy = bandTop + 48 + rr * 85;

                Rectangle hit = {(float)(cx - rR * 1.6f), (float)(cy - rR * 1.6f), rR * 3.2f, rR * 3.2f};
                bool hov = CheckCollisionPointRec(mp, hit) && mp.y >= headerH;

                DrawCircleV((Vector2){(float)cx, (float)cy}, rR * 1.6f,
                            hov ? (Color){120, 255, 120, 28} : (Color){255, 255, 255, 6});
                fns[i]((float)cx, (float)cy, rR, t);
                DrawLineEx((Vector2){cx - rR * 1.15f, cy - rR * 1.15f},
                           (Vector2){cx + rR * 1.15f, cy + rR * 1.15f},
                           2.5f, (Color){255, 60, 60, 200});
                int tw = MeasureText(skinMeta[i].name, 11);
                DrawText(skinMeta[i].name, cx - tw / 2, cy + (int)(rR * 1.7f), 11,
                         (Color){170, 165, 190, 200});
                if (hov) {
                    DrawCircleLines(cx, cy, (int)(rR * 1.6f), (Color){120, 255, 120, 220});
                    strncpy(hoverDesc, skinMeta[i].desc, sizeof(hoverDesc) - 1);
                    hoverPos = (Vector2){(float)cx, (float)(cy - rR * 1.95f)};
                    if (clicked) { bad[i] = false; SaveBadSkins("bad_skins.txt"); }
                }
                idx++;
            }
        }

        // PICKED FOR GAME gallery
        int pTop = pickTop - scroll;
        DrawText("PICKED FOR GAME  (right-click / P on a candidate to toggle; listed in skin_picks.txt)",
                 leftM, pTop + 6, 14, (Color){140, 230, 170, 230});
        DrawLine(0, pTop + pickH - 4, sw, pTop + pickH - 4, (Color){40, 50, 45, 255});
        if (npick == 0) {
            DrawText("(none yet — these are the skins you want wired into dungeon_crawler)",
                     leftM, pTop + 40, 13, (Color){110, 130, 115, 200});
        } else {
            int idx = 0;
            float pR = 22.0f;
            int pCellW = (sw - leftM * 2) / pickCols;
            for (int i = 0; i < NSKIN; i++) {
                if (!pick[i] || bad[i]) continue;
                int pc = idx % pickCols, pr = idx / pickCols;
                int cx = leftM + pc * pCellW + pCellW / 2;
                int cy = pTop + 48 + pr * 70;
                Rectangle hit = {(float)(cx - pR * 1.5f), (float)(cy - pR * 1.5f), pR * 3.0f, pR * 3.0f};
                bool hov = CheckCollisionPointRec(mp, hit) && mp.y >= headerH;
                DrawCircleV((Vector2){(float)cx, (float)cy}, pR * 1.5f,
                            hov ? (Color){100, 255, 140, 25} : (Color){80, 255, 120, 10});
                fns[i]((float)cx, (float)cy, pR, t);
                int tw = MeasureText(skinMeta[i].name, 11);
                DrawText(skinMeta[i].name, cx - tw / 2, cy + (int)(pR * 1.65f), 11,
                         (Color){160, 230, 180, 220});
                if (hov) {
                    DrawCircleLines(cx, cy, (int)(pR * 1.5f), (Color){120, 255, 160, 220});
                    strncpy(hoverDesc, skinMeta[i].desc, sizeof(hoverDesc) - 1);
                    hoverPos = (Vector2){(float)cx, (float)(cy - pR * 1.9f)};
                    hoverId = i;
                    if (clicked || rclick) TogglePick(i);
                }
                idx++;
            }
        }

        EndScissorMode();

        if (hoverId >= 0 && !bad[hoverId]) {
            if (IsKeyPressed(KEY_P)) TogglePick(hoverId);
        }

        if (maxScroll > 0) {
            int sbX = sw - 10, sbY = headerH, sbH = sh - headerH;
            int sbTH = (int)((float)sbH / contentH * sbH);
            if (sbTH < 20) sbTH = 20;
            int sbTY = sbY + (int)((float)scroll / maxScroll * (sbH - sbTH));
            DrawRectangle(sbX, sbY, 4, sbH, (Color){255, 255, 255, 25});
            DrawRectangle(sbX, sbTY, 4, sbTH, (Color){255, 255, 255, 110});
        }

        if (hoverDesc[0]) {
            int tw = MeasureText(hoverDesc, 13);
            int bx = (int)hoverPos.x - tw / 2 - 8, by = (int)hoverPos.y - 22;
            if (bx < 4) bx = 4;
            if (bx + tw + 16 > sw) bx = sw - tw - 16;
            if (by < headerH) by = headerH + 4;
            DrawRectangle(bx, by, tw + 16, 22, (Color){10, 8, 18, 235});
            DrawRectangleLines(bx, by, tw + 16, 22, (Color){255, 255, 255, 60});
            DrawText(hoverDesc, bx + 8, by + 4, 13, (Color){235, 230, 245, 255});
        }

        EndDrawing();
    }

    if (poptartCatTextureLoaded && poptartCatTexture.id > 0) UnloadTexture(poptartCatTexture);
    CloseWindow();
    return 0;
}
