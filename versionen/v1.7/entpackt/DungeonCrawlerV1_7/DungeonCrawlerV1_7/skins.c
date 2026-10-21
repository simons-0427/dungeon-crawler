// skins.c — skin colours plus the rare/epic/skull procedural face & back art.
#include "skins.h"
#include "constants.h"   // CLAMP255
#include "types.h"       // uchar
#include "rlgl.h"        // matrix mirror for directional procedural art
#include <math.h>
#include <stdio.h>

Color GetSkinColor(int id) {
    float gt = GetTime();
    switch (id) {
        case 22: { // Pyro — intense Red->Orange->Yellow
            float p = 0.5f + 0.5f*sinf(gt * 3.5f);
            return (Color){255, (uchar)(10+(int)(200*p*p)), (uchar)(30*(p*p*p)), 255};
        }
        case 23: { // Phantom — Violet->Cyan->White, ghostly pulsing alpha 40-130
            float p  = 0.5f + 0.5f*sinf(gt * 1.8f);
            float p2 = 0.5f + 0.5f*sinf(gt * 3.0f);
            return (Color){(uchar)(120+(int)(135*p)),
                           (uchar)( 60+(int)(195*p)),
                           255,
                           (uchar)( 40+(int)( 90*p2))};
        }
        case 24: { // Cyber — smooth dark Cyan/Pink with more black
            float phase = sinf(gt * 1.5f);
            float pos   = phase > 0.0f ? powf(phase,  1.8f) : 0.0f;
            float neg   = phase < 0.0f ? powf(-phase, 1.8f) : 0.0f;
            return (Color){(uchar)(neg*210 + 6),
                           (uchar)(pos*150 + 5),
                           (uchar)(pos*250*0.6f + neg*90*0.5f + 8),
                           255};
        }
        case 25: { // Chroma — full rainbow loop
            return ColorFromHSV(fmodf(gt * 60.0f, 360.0f), 1.0f, 1.0f);
        }
        case 26: { // Biohazard — intense pulsing toxic green
            float p = 0.5f + 0.5f*sinf(gt * 3.5f);
            float pp = p * p;
            return (Color){(uchar)(5+(int)(25*pp)),
                           (uchar)(140+(int)(115*p)),
                           (uchar)(0+(int)(18*pp)), 255};
        }
        case 27: { // Alien — bright emoji green
            return (Color){138, 200, 66, 255};
        }
        case 28: { // Skull (Skull Ember) — cream bone with warm ember tint
            float p = 0.5f + 0.5f*sinf(gt * 2.5f);
            return (Color){(uchar)(228+(int)(10*p)), (uchar)(210+(int)(8*p)), (uchar)(190+(int)(5*p)), 255};
        }
        case 29: { // Angel — pulses warm gold <-> pure white
            float p = 0.5f + 0.5f*sinf(gt * 1.8f);
            return (Color){255,
                           (uchar)(240+(int)(15*p)),
                           (uchar)(160+(int)(95*p)), 255};
        }
        case 30: { // Chronovore — antique bronze, slow heartbeat-like pulse
            float p = 0.5f + 0.5f*sinf(gt * 2.0f);
            return (Color){(uchar)(140+(int)(60*p)), (uchar)(100+(int)(50*p)), (uchar)(40+(int)(20*p)), 255};
        }
        case 31: { // Deep Cartographer — sun-bleached parchment tan
            float p = 0.5f + 0.5f*sinf(gt * 1.2f);
            return (Color){(uchar)(200+(int)(30*p)), (uchar)(170+(int)(25*p)), (uchar)(110+(int)(20*p)), 255};
        }
        case 32: { // Midas — liquid gold, shimmering
            float p = 0.5f + 0.5f*sinf(gt * 2.5f);
            return (Color){255, (uchar)(190+(int)(45*p)), (uchar)(20+(int)(40*p)), 255};
        }
        case 33: { // Blade Legend — battle-forged crimson steel
            float p = 0.5f + 0.5f*sinf(gt * 3.2f);
            return (Color){(uchar)(120+(int)(100*p)), (uchar)(20+(int)(15*p)), (uchar)(20+(int)(10*p)), 255};
        }
        case 34: { // Constellation — deep indigo night sky
            float p = 0.5f + 0.5f*sinf(gt * 1.0f);
            return (Color){(uchar)(20+(int)(20*p)), (uchar)(15+(int)(15*p)), (uchar)(60+(int)(40*p)), 255};
        }
        case 35: { // Ascendant — radiant white-gold, breathing glow
            float p = 0.5f + 0.5f*sinf(gt * 1.6f);
            return (Color){255, (uchar)(245+(int)(10*p)), (uchar)(210+(int)(45*p)), 255};
        }
        case 36: { // Poptart Cat — pastry pink base for the pixel-art sprite
            float p = 0.5f + 0.5f*sinf(gt * 2.0f);
            return (Color){255, (uchar)(92 + (int)(35*p)), (uchar)(168 + (int)(35*p)), 255};
        }
        case 37: { // Apex Rex — primal leaf-green hide
            float p = 0.5f + 0.5f*sinf(gt * 1.8f);
            return (Color){(uchar)(38 + (int)(18*p)), (uchar)(105 + (int)(35*p)), (uchar)(42 + (int)(18*p)), 255};
        }
        case 38: { // Skullvoid — pulsing violet void
            float p = 0.5f + 0.5f*sinf(gt * 2.4f);
            return (Color){(uchar)(82 + (int)(35*p)), (uchar)(34 + (int)(18*p)), (uchar)(160 + (int)(45*p)), 255};
        }
        case 39: { // Bloodscale Vanguard — ember red with gold trim
            float p = 0.5f + 0.5f*sinf(gt * 2.6f);
            return (Color){(uchar)(145 + (int)(55*p)), (uchar)(24 + (int)(22*p)), (uchar)(32 + (int)(24*p)), 255};
        }
        case 40: { // Stormforged Aegis — cobalt and cyan steel
            float p = 0.5f + 0.5f*sinf(gt * 2.8f);
            return (Color){(uchar)(28 + (int)(24*p)), (uchar)(120 + (int)(70*p)), (uchar)(190 + (int)(55*p)), 255};
        }
        case 41: { // Sunforged Crusader — radiant gold
            float p = 0.5f + 0.5f*sinf(gt * 2.5f);
            return (Color){255, (uchar)(170 + (int)(55*p)), (uchar)(38 + (int)(38*p)), 255};
        }
        case 42: { // Black Hole — near-black violet with a warm lensing pulse
            float p = 0.5f + 0.5f*sinf(gt * 2.0f);
            return (Color){(uchar)(18 + (int)(12*p)),
                           (uchar)(8 + (int)(8*p)),
                           (uchar)(28 + (int)(22*p)), 255};
        }
        case 43: return (Color){241,114,58,255};   // Ember Fox
        case 44: return (Color){176,160,203,255}; // Veil Dancer
        case 45: return (Color){229,213,183,255}; // Ossuary Knight
        case 46: return (Color){153,220,244,255}; // Glass Seraph
        case 47: return (Color){225,168,75,255};  // Gilt Jackal
        case 48: return (Color){236,226,201,255}; // Ember Marrow
        case 49: return (Color){123,217,239,255}; // Glass Echo
        case 50: return (Color){255,125,49,255};  // Cinder Sovereign
        case 51: return (Color){247,197,93,255};  // Gilded Oracle
    }
    static const Color SKIN_TABLE[22] = {
        // Common (0-21)
        {  90,220,120,255 }, { 255, 80, 80,255 }, { 255,160, 40,255 },
        { 255,220, 40,255 }, {  60,200, 60,255 }, {  40,200,220,255 },
        {  60,120,255,255 }, { 160, 60,255,255 }, { 220, 60,200,255 },
        { 255,255,255,255 },
        { 150, 90, 40,255 }, {  30, 28, 35,255 }, { 255,120,195,255 },
        { 155,155,162,255 },
        // Former Uncommon colors (14-21), now Common; Void unchanged
        {  22,170, 42,255 }, { 235, 12, 55,255 }, { 255,118,  0,255 },
        { 255,190, 15,255 }, {  10,180,135,255 }, { 110,205,245,255 },
        {  16, 78,245,255 }, {  10,  5, 20,255 },
    };
    if (id >= 0 && id < 22) return SKIN_TABLE[id];
    return (Color){200,200,200,255};
}

// ─── EPIC SKIN FACES (shared by in-game player + shop preview) ────────────────
// Filled rotated ellipse (used for slanted alien eyes / skull sockets).
static void DrawEllipseRotF(float cx, float cy, float rx, float ry, float ang, Color col) {
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

// Deterministic 0..1 hash used by the Skin_Ideas particle and scale placement.
static float SkinRnd(int i) {
    float x = sinf(i * 127.1f + 311.7f) * 43758.5453f;
    return x - floorf(x);
}

// Thick elliptical arc (used for the perspective halo ring) drawn as a band of
// short segments between angle a0..a1.
static void DrawHaloArc(float cx, float cy, float rx, float ry,
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

static void DrawEllipseLinesRot(float cx, float cy, float rx, float ry,
                                float ang, float thick, Color col) {
    const int SEG = 32;
    Vector2 prev = {0};
    float ca = cosf(ang), sa = sinf(ang);
    for (int i = 0; i <= SEG; i++) {
        float th = (float)i / SEG * 2.0f * PI;
        float ex = cosf(th) * rx, ey = sinf(th) * ry;
        Vector2 p = {cx + ex*ca - ey*sa, cy + ex*sa + ey*ca};
        if (i > 0) DrawLineEx(prev, p, thick, col);
        prev = p;
    }
}

static void DrawStar4(float cx, float cy, float r, Color c) {
    DrawLineEx((Vector2){cx-r, cy}, (Vector2){cx+r, cy}, r*0.35f, c);
    DrawLineEx((Vector2){cx, cy-r}, (Vector2){cx, cy+r}, r*0.35f, c);
    DrawCircleV((Vector2){cx, cy}, r*0.30f, c);
}

// Exact transfer of Skin_Ideas #26: the Interstellar-style Black Hole variant.
void DrawBlackHole(float cx, float cy, float R, float t) {
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
        float a = spin + SkinRnd(k) * 2.0f * PI;
        float d = R * (1.24f + SkinRnd(k + 18) * 0.78f);
        float px = cx + cosf(a) * d;
        float py = cy + sinf(a) * d * 0.54f;
        float size = R * (0.018f + SkinRnd(k + 36) * 0.035f);
        Color particle = (k % 4 == 0)
            ? (Color){155, 198, 255, (uchar)(95 + 55 * pulse)}
            : (Color){255, (uchar)(110 + 80 * SkinRnd(k + 54)), 36,
                       (uchar)(105 + 70 * pulse)};
        DrawCircleV((Vector2){px, py}, size, particle);
    }
    EndBlendMode();

    // A broad, tilted accretion disk sits behind the event horizon.
    DrawEllipseRotF(cx, cy + R * 0.03f, R * 1.78f, R * 0.38f, -0.10f,
                    (Color){255, 54, 14, (uchar)(48 + 24 * pulse)});
    DrawEllipseRotF(cx, cy + R * 0.01f, R * 1.54f, R * 0.24f, -0.10f,
                    (Color){255, 164, 42, (uchar)(92 + 32 * pulse)});

    BeginBlendMode(BLEND_ADDITIVE);
    // Hot layered disk contours.
    for (int k = 0; k < 5; k++) {
        float rx = R * (1.00f + 0.15f * k);
        float ry = R * (0.26f + 0.065f * k);
        Color disk = (Color){255, (uchar)(82 + 30 * k), (uchar)(18 + 8 * k),
                             (uchar)(170 - 20 * k + 25 * pulse)};
        DrawEllipseLinesRot(cx, cy - R * 0.04f, rx, ry, -0.10f,
                            R * (0.052f - 0.006f * k), disk);
    }

    // Gravitational lensing bends the same disk far above and below the shadow.
    for (int k = 0; k < 4; k++) {
        float rx = R * (1.04f + 0.17f * k);
        float ry = R * (0.48f + 0.11f * k);
        DrawHaloArc(cx, cy - R * 0.11f, rx, ry, PI + 0.10f, 2.0f * PI - 0.10f,
                    R * (0.062f - 0.009f * k),
                    (Color){255, (uchar)(168 + 18 * k), 64, (uchar)(185 - 25 * k)});
        DrawHaloArc(cx, cy + R * 0.11f, rx, ry, 0.10f, PI - 0.10f,
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
        DrawHaloArc(cx, cy - R * 0.08f, rx, ry, PI + 0.16f, 2.0f * PI - 0.16f,
                    thick, canopy);
        DrawHaloArc(cx, cy + R * 0.08f, rx, ry, 0.16f, PI - 0.16f,
                    thick, canopy);
    }

    // Broken orbital ribbons sell the extreme curvature instead of a flat ring.
    for (int k = 0; k < 8; k++) {
        float q = SkinRnd(k + 126);
        float phase = spin * (0.32f + q * 0.35f) + k * 0.78f;
        float rx = R * (1.18f + q * 0.50f);
        float ry = R * (0.34f + q * 0.26f);
        float span = 0.44f + q * 0.28f;
        Color ribbon = (k % 3 == 0)
            ? (Color){155, 196, 255, (uchar)(95 + 55 * pulse)}
            : (Color){255, (uchar)(108 + 80 * q), 26, (uchar)(130 + 55 * pulse)};
        DrawHaloArc(cx, cy, rx, ry, phase, phase + span,
                    R * (0.025f + 0.012f * q), ribbon);
        DrawHaloArc(cx, cy, rx, ry, phase + PI, phase + PI + span,
                    R * (0.018f + 0.010f * q), ribbon);
    }

    // Fast-moving plasma streaks give the disk a living, violent texture.
    for (int k = 0; k < 10; k++) {
        float q = SkinRnd(k + 72);
        float a = spin * 0.7f + q * 2.0f * PI;
        float d0 = R * (1.08f + q * 0.28f);
        float d1 = d0 + R * (0.12f + SkinRnd(k + 90) * 0.18f);
        Vector2 p0 = {cx + cosf(a) * d0, cy + sinf(a) * d0 * 0.24f};
        Vector2 p1 = {cx + cosf(a) * d1, cy + sinf(a) * d1 * 0.24f};
        DrawLineEx(p0, p1, R * (0.018f + 0.018f * SkinRnd(k + 108)),
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
    DrawEllipseLinesRot(cx, cy, R * 0.93f, R * 0.93f, 0.0f, R * 0.065f,
                        (Color){255, 152, 54, (uchar)(180 + 50 * pulse)});
    DrawEllipseLinesRot(cx, cy, R * 0.89f, R * 0.89f, 0.0f, R * 0.022f,
                        (Color){190, 220, 255, (uchar)(145 + 55 * pulse)});
    DrawHaloArc(cx, cy - R * 0.02f, R * 1.08f, R * 0.66f,
                PI + 0.28f, 2.0f * PI - 0.28f, R * 0.045f,
                (Color){236, 246, 255, (uchar)(155 + 60 * pulse)});

    // Foreground disk crossing the shadow, as in the iconic silhouette.
    DrawHaloArc(cx, cy + R * 0.12f, R * 1.52f, R * 0.23f,
                0.08f, PI - 0.08f, R * 0.12f,
                (Color){255, 92, 18, (uchar)(205 + 45 * pulse)});
    DrawHaloArc(cx, cy + R * 0.12f, R * 1.34f, R * 0.15f,
                0.10f, PI - 0.10f, R * 0.040f,
                (Color){255, 242, 184, (uchar)(220 + 35 * pulse)});
    DrawHaloArc(cx, cy + R * 0.12f, R * 1.43f, R * 0.18f,
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
    DrawStar4(hx, hy, R * 0.065f, (Color){255, 232, 166, 220});
    EndBlendMode();
}

// Halo ellipse geometry shared by the back (behind head) and front (over brow)
// passes so the two arcs line up into one continuous ring.
static void HaloGeom(float cx, float cy, float R, float t,
                     float *hx, float *hy, float *rx, float *ry) {
    *hx = cx;
    *hy = cy - R*0.95f + sinf(t * 2.2f) * (R * 0.14f);
    *rx = R * 1.06f;
    *ry = R * 0.40f;
}

// Epic-skin elements that belong BEHIND the body circle (drawn before it):
// the angel's wings, outer glow and the rear half of the halo ring.
void DrawEpicBack(int id, float cx, float cy, float R, float t) {
    if (id == 29) {
        DrawCircleV((Vector2){cx, cy}, R*1.35f, (Color){255,200,50,16});

        float wf = sinf(t * 3.0f) * (R * 0.25f);
        Color wo = (Color){255,245,210, 90};   // outer (translucent)
        Color wi = (Color){255,250,230,210};   // inner (brighter)
        // left wing (both windings against culling)
        DrawTriangle((Vector2){cx-R*0.50f,cy-R*0.20f},(Vector2){cx-R*1.45f,cy-R*0.50f-wf},(Vector2){cx-R*1.25f,cy+R*0.45f+wf}, wo);
        DrawTriangle((Vector2){cx-R*0.50f,cy-R*0.20f},(Vector2){cx-R*1.25f,cy+R*0.45f+wf},(Vector2){cx-R*1.45f,cy-R*0.50f-wf}, wo);
        DrawTriangle((Vector2){cx-R*0.50f,cy-R*0.10f},(Vector2){cx-R*1.10f,cy-R*0.25f-wf},(Vector2){cx-R*1.00f,cy+R*0.35f+wf}, wi);
        DrawTriangle((Vector2){cx-R*0.50f,cy-R*0.10f},(Vector2){cx-R*1.00f,cy+R*0.35f+wf},(Vector2){cx-R*1.10f,cy-R*0.25f-wf}, wi);
        // right wing
        DrawTriangle((Vector2){cx+R*0.50f,cy-R*0.20f},(Vector2){cx+R*1.45f,cy-R*0.50f-wf},(Vector2){cx+R*1.25f,cy+R*0.45f+wf}, wo);
        DrawTriangle((Vector2){cx+R*0.50f,cy-R*0.20f},(Vector2){cx+R*1.25f,cy+R*0.45f+wf},(Vector2){cx+R*1.45f,cy-R*0.50f-wf}, wo);
        DrawTriangle((Vector2){cx+R*0.50f,cy-R*0.10f},(Vector2){cx+R*1.10f,cy-R*0.25f-wf},(Vector2){cx+R*1.00f,cy+R*0.35f+wf}, wi);
        DrawTriangle((Vector2){cx+R*0.50f,cy-R*0.10f},(Vector2){cx+R*1.00f,cy+R*0.35f+wf},(Vector2){cx+R*1.10f,cy-R*0.25f-wf}, wi);

        // rear half of the halo (upper arc, θ 180°..360°) — head will be drawn
        // over it so the part directly behind the skull is hidden.
        float hx, hy, rx, ry;  HaloGeom(cx, cy, R, t, &hx, &hy, &rx, &ry);
        DrawHaloArc(hx, hy, rx, ry, PI, 2.0f*PI, R*0.22f, (Color){255,200,50,70});
        DrawHaloArc(hx, hy, rx, ry, PI, 2.0f*PI, R*0.12f, (Color){255,214,72,235});
    }
    else if (id == 32) {
        // Midas (Gilded Sovereign) — floating jewelled crown, trickling gold
        // drips off its base, and slow-orbiting gold motes.
        float bob = sinf(t*1.2f)*R*0.05f;
        float crownY = cy - R*1.20f + bob;
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
    }
    else if (id == 33) {
        // Blade Legend — ember aura + crossed blades behind the body
        DrawCircleV((Vector2){cx, cy}, R*1.30f, (Color){255,60,40,26});
        Color steel = (Color){70,70,80,220};
        Color edge  = (Color){210,210,220,240};
        for (int s = -1; s <= 1; s += 2) {
            float a = (PI/4.0f) * s;
            Vector2 dir  = { cosf(a), sinf(a) };
            Vector2 tip  = { cx + dir.x*R*1.35f, cy + dir.y*R*1.35f };
            Vector2 hilt = { cx - dir.x*R*0.55f, cy - dir.y*R*0.55f };
            DrawLineEx(hilt, tip, R*0.20f, steel);
            DrawLineEx(hilt, tip, R*0.08f, edge);
        }
    }
}

// ─── SKULL SKIN (id 28) ───────────────────────────────────────────────────────
// Skin Ideas #18 "Skull Ember": classic bone silhouette + pulsing ember sockets
// + rising sparks. Shop name remains "Skull".
static void DrawSkullShape(float cx, float cy, float R, Color col) {
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

static float SkullRnd(int i) {
    float x = sinf(i * 127.1f + 311.7f) * 43758.5453f;
    return x - floorf(x);
}

void DrawSkull(float cx, float cy, float R, float t) {
    Color bone   = (Color){231, 226, 212, 255};
    Color boneLo = (Color){193, 187, 172, 255};
    Color boneHi = (Color){247, 244, 235, 255};
    Color socket = (Color){ 20,  10,   8, 255};
    Color dark   = (Color){ 24,  20,  21, 255};
    Color ember  = (Color){255, 110,  30, 255};

    float bob   = sinf(t * 2.2f) * R * 0.04f;
    float cyb   = cy + bob;
    float breath = 0.5f + 0.5f * sinf(t * 1.8f);
    float pulse  = 0.5f + 0.5f * sinf(t * 4.0f);

    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cyb}, R * (1.25f + 0.06f * breath),
                (Color){255, 90, 20, (uchar)(14 + 12 * breath)});
    EndBlendMode();

    DrawSkullShape(cx, cyb + R * 0.03f, R * 1.03f, boneLo);
    DrawSkullShape(cx, cyb, R, bone);

    for (int i = 0; i < 4; i++) {
        float k = 1.0f - i * 0.18f;
        DrawEllipseRotF(cx - R * 0.10f, cyb - R * 0.46f, R * 0.58f * k, R * 0.44f * k, 0.0f,
                        (Color){boneHi.r, boneHi.g, boneHi.b, 42});
    }
    DrawEllipseRotF(cx, cyb + R * 0.58f, R * 0.52f, R * 0.26f, 0.0f,
                    (Color){boneLo.r, boneLo.g, boneLo.b, 110});

    // Eye sockets
    DrawEllipseRotF(cx - R * 0.44f, cyb - R * 0.07f, R * 0.48f, R * 0.42f,  0.32f, (Color){150,144,132,120});
    DrawEllipseRotF(cx + R * 0.44f, cyb - R * 0.07f, R * 0.48f, R * 0.42f, -0.32f, (Color){150,144,132,120});
    DrawEllipseRotF(cx - R * 0.44f, cyb - R * 0.08f, R * 0.42f, R * 0.36f,  0.32f, socket);
    DrawEllipseRotF(cx + R * 0.44f, cyb - R * 0.08f, R * 0.42f, R * 0.36f, -0.32f, socket);

    // Ember glow in sockets
    float er = R * (0.10f + 0.05f * pulse);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx - R * 0.40f, cyb - R * 0.04f}, er,
                (Color){ember.r, ember.g, ember.b, (uchar)(90 + 100 * pulse)});
    DrawCircleV((Vector2){cx + R * 0.40f, cyb - R * 0.04f}, er,
                (Color){ember.r, ember.g, ember.b, (uchar)(90 + 100 * pulse)});
    DrawCircleV((Vector2){cx - R * 0.40f, cyb - R * 0.04f}, er * 0.45f,
                (Color){255, 220, 120, (uchar)(120 + 80 * pulse)});
    DrawCircleV((Vector2){cx + R * 0.40f, cyb - R * 0.04f}, er * 0.45f,
                (Color){255, 220, 120, (uchar)(120 + 80 * pulse)});
    EndBlendMode();
    DrawCircleV((Vector2){cx - R * 0.34f, cyb + R * 0.10f}, R * 0.05f, (Color){55,51,53,150});
    DrawCircleV((Vector2){cx + R * 0.34f, cyb + R * 0.10f}, R * 0.05f, (Color){55,51,53,150});

    // Nose cavity
    DrawTriangle((Vector2){cx, cyb + R * 0.44f},
                 (Vector2){cx - R * 0.15f, cyb + R * 0.20f},
                 (Vector2){cx + R * 0.15f, cyb + R * 0.20f}, dark);
    DrawTriangle((Vector2){cx, cyb + R * 0.44f},
                 (Vector2){cx + R * 0.15f, cyb + R * 0.20f},
                 (Vector2){cx - R * 0.15f, cyb + R * 0.20f}, dark);
    DrawCircleV((Vector2){cx - R * 0.08f, cyb + R * 0.22f}, R * 0.075f, dark);
    DrawCircleV((Vector2){cx + R * 0.08f, cyb + R * 0.22f}, R * 0.075f, dark);

    // Teeth (light chatter)
    float chatter = sinf(t * 12.0f) * R * 0.012f;
    float mx = cx - R * 0.46f, mouthW = R * 0.92f;
    float myT = cyb + R * 0.62f + chatter, myB = cyb + R * 1.00f + chatter;
    DrawRectangleRounded((Rectangle){mx, myT, mouthW, myB - myT}, 0.35f, 6, dark);
    int nt = 6;
    float tw = mouthW / nt;
    for (int k = 0; k < nt; k++) {
        float tx = mx + k * tw;
        DrawRectangleRounded(
            (Rectangle){tx + R * 0.03f, myT + R * 0.03f, tw - R * 0.06f, (myB - myT) - R * 0.06f},
            0.30f, 4, boneHi);
    }

    // Rising ember sparks from sockets
    for (int k = 0; k < 6; k++) {
        float pr = fmodf(t * 0.7f + SkullRnd(k + 3), 1.0f);
        float side = (k % 2) ? 1.0f : -1.0f;
        DrawCircleV((Vector2){cx + side * R * 0.40f + (SkullRnd(k) - 0.5f) * R * 0.1f,
                              cyb - R * 0.05f - pr * R * 1.0f},
                    R * (0.05f - pr * 0.03f),
                    (Color){255, 140, 40, (uchar)(200 * (1.0f - pr))});
    }
}

static void DrawPurpleSkullBase(float cx, float cy, float R, float t,
                                Color violet, Color violetLo, Color violetHi,
                                Color socketFill, Color eyeGlow, float eyePulse) {
    float bob = sinf(t * 2.2f) * R * 0.04f;
    float cyb = cy + bob;
    float breath = 0.5f + 0.5f * sinf(t * 1.8f);

    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cyb}, R * (1.30f + 0.07f * breath),
                (Color){eyeGlow.r, eyeGlow.g, eyeGlow.b, (uchar)(20 + 16 * breath)});
    EndBlendMode();

    DrawSkullShape(cx, cyb + R * 0.03f, R * 1.03f, violetLo);
    DrawSkullShape(cx, cyb, R, violet);

    for (int i = 0; i < 4; i++) {
        float k = 1.0f - i * 0.18f;
        DrawEllipseRotF(cx - R * 0.10f, cyb - R * 0.46f,
                        R * 0.58f * k, R * 0.44f * k, 0.0f,
                        (Color){violetHi.r, violetHi.g, violetHi.b, 48});
    }
    DrawEllipseRotF(cx, cyb + R * 0.58f, R * 0.52f, R * 0.26f, 0.0f,
                    (Color){violetLo.r, violetLo.g, violetLo.b, 150});

    Color socketRim = (Color){100, 32, 160, 210};
    DrawEllipseRotF(cx - R * 0.44f, cyb - R * 0.07f,
                    R * 0.48f, R * 0.42f, 0.32f, socketRim);
    DrawEllipseRotF(cx + R * 0.44f, cyb - R * 0.07f,
                    R * 0.48f, R * 0.42f, -0.32f, socketRim);
    DrawEllipseRotF(cx - R * 0.44f, cyb - R * 0.08f,
                    R * 0.42f, R * 0.36f, 0.32f, socketFill);
    DrawEllipseRotF(cx + R * 0.44f, cyb - R * 0.08f,
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

void DrawSkullvoidCrowned(float cx, float cy, float R, float t) {
    float pulse = 0.5f + 0.5f * sinf(t * 3.4f);
    DrawPurpleSkullBase(cx, cy, R, t,
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

// Exact transfer of Skin_Ideas #27 "Apex Bladelord Splitguard".
void DrawApexBladelord(float cx, float cy, float R, float t) {
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
        Vector2 tip  = {cx + side * ca * R * 1.38f,
                        cy - sa * R * 1.38f - R * 0.22f};
        Vector2 hilt = {cx + side * ca * R * 0.52f, cy + R * 0.03f};
        DrawLineEx(hilt, tip, R * 0.26f, (Color){45, 42, 55, 255});
        DrawLineEx(hilt, tip, R * 0.12f, (Color){200, 205, 220, 255});
        BeginBlendMode(BLEND_ADDITIVE);
        DrawLineEx(hilt, tip, R * 0.05f,
                   (Color){255, 90, 40, (uchar)(160 + 60 * pulse)});
        EndBlendMode();

        Vector2 dir = {tip.x - hilt.x, tip.y - hilt.y};
        float invLen = 1.0f / sqrtf(dir.x * dir.x + dir.y * dir.y);
        Vector2 perp = {-dir.y * invLen, dir.x * invLen};
        Vector2 mid = {hilt.x * 0.35f + tip.x * 0.65f,
                       hilt.y * 0.35f + tip.y * 0.65f};
        DrawLineEx((Vector2){mid.x - perp.x * R * 0.28f,
                             mid.y - perp.y * R * 0.28f},
                   (Vector2){mid.x + perp.x * R * 0.28f,
                             mid.y + perp.y * R * 0.28f},
                   R * 0.08f, (Color){255, 180, 60, 240});
        DrawCircleV(tip, R * 0.08f, (Color){255, 220, 120, 230});
    }

    for (int k = 0; k < 5; k++) {
        float a = t * 1.8f + k * (2.0f * PI / 5.0f);
        float d = R * (1.35f + 0.08f * sinf(t * 3.0f + k));
        float bx = cx + cosf(a) * d, by = cy + sinf(a) * d * 0.72f;
        DrawLineEx((Vector2){bx - cosf(a) * R * 0.18f,
                             by - sinf(a) * R * 0.18f},
                   (Vector2){bx + cosf(a) * R * 0.18f,
                             by + sinf(a) * R * 0.18f},
                   R * 0.06f, (Color){255, 100, 40, 200});
    }
    DrawCircle((int)cx, (int)cy, (int)R, (Color){120, 18, 22, 255});
    DrawCircleV((Vector2){cx, cy}, R * 0.62f, (Color){180, 30, 30, 160});
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){cx, cy}, R * (0.38f + 0.06f * pulse),
                (Color){255, 90, 30, (uchar)(90 + 50 * pulse)});
    EndBlendMode();
    DrawRectangle((int)(cx - R * 0.42f), (int)(cy - R * 0.12f),
                  (int)(R * 0.28f), (int)(R * 0.10f),
                  (Color){255, 220, 60, 255});
    DrawRectangle((int)(cx + R * 0.14f), (int)(cy - R * 0.12f),
                  (int)(R * 0.28f), (int)(R * 0.10f),
                  (Color){255, 220, 60, 255});
    DrawLineEx((Vector2){cx - R * 0.55f, cy + R * 0.22f},
               (Vector2){cx - R * 0.15f, cy + R * 0.38f},
               R * 0.07f, (Color){40, 8, 10, 230});
    DrawLineEx((Vector2){cx + R * 0.15f, cy + R * 0.38f},
               (Vector2){cx + R * 0.55f, cy + R * 0.22f},
               R * 0.07f, (Color){40, 8, 10, 230});
}

static Texture2D poptartCatTexture = {0};
static bool poptartCatTextureLoaded = false;

static void DrawPoptartRainbowExtension(float cx, float cyb, float R, float t, bool flipX) {
    static const Color rainbow[6] = {
        {255,   0,   0, 255},
        {255, 150,   0, 255},
        {255, 235,   0, 255},
        { 35, 220,  55, 255},
        {  0, 170, 245, 255},
        { 90,  60, 235, 255},
    };
    const int bands = 6;
    const int segments = 6;
    const float tailWidth = R * 1.05f;
    const float bandHeight = R * 0.24f;
    const float segmentWidth = tailWidth / segments;
    const float attachX = flipX ? cx + R * 1.60f : cx - R * 1.60f;

    // Small per-segment offsets make the extra tail feel alive while retaining
    // the hard-edged, pixel-art character of the source PNG.
    for (int s = 0; s < segments; s++) {
        float wave = sinf(t * 1.8f + s * 0.85f) * R * 0.045f;
        float x = flipX
            ? attachX + s * segmentWidth
            : attachX - (s + 1) * segmentWidth;
        for (int b = 0; b < bands; b++) {
            float shimmer = 0.5f + 0.5f * sinf(t * 2.2f + s * 0.55f + b * 0.35f);
            Color c = rainbow[b];
            c.a = (uchar)(110 + (int)(55.0f * shimmer));
            DrawRectangle((int)floorf(x),
                          (int)floorf(cyb - R * 0.72f + b * bandHeight + wave),
                          (int)(segmentWidth + 2.0f),
                          (int)(bandHeight + 1.0f), c);
        }
    }
}

static void DrawPoptartCatVariant(float cx, float cy, float R, float t, bool flipX, bool animateRainbow) {
    float cyb = cy + sinf(t * 1.8f) * R * 0.02f;
    if (!poptartCatTextureLoaded) {
        poptartCatTexture = LoadTexture("assets/skins/nyan_cat.png");
        if (poptartCatTexture.id > 0)
            SetTextureFilter(poptartCatTexture, TEXTURE_FILTER_POINT);
        poptartCatTextureLoaded = true;
    }
    if (poptartCatTexture.id > 0) {
        Rectangle source = {192.0f, 113.0f, flipX ? -479.0f : 479.0f, 270.0f};
        float width = R * 3.20f;
        float height = R * 1.80f;
        Rectangle dest = {cx - width * 0.5f, cyb - height * 0.5f, width, height};
        if (animateRainbow)
            DrawPoptartRainbowExtension(cx, cyb, R, t, flipX);
        DrawTexturePro(poptartCatTexture, source, dest, (Vector2){0, 0}, 0.0f, WHITE);
    }
}

void DrawPoptartCat(float cx, float cy, float R, float t) {
    DrawPoptartCatVariant(cx, cy, R, t, false, false);
}

void DrawPoptartCatInGame(float cx, float cy, float R, float t, bool flipX) {
    DrawPoptartCatVariant(cx, cy, R * 1.20f, t, flipX, true);
}

static void DrawApexRexCore(float cx, float cy, float R, float t, bool drawAura) {
    float chomp   = 0.5f + 0.5f * sinf(t * 3.2f);
    float bob     = sinf(t * 1.5f) * R * 0.045f;
    float cyb     = cy + bob;
    float jawOpen = R * (0.06f + 0.16f * chomp);

    Color hideD  = (Color){28, 72, 32, 255};
    Color hide   = (Color){48, 125, 50, 255};
    Color hideM  = (Color){70, 160, 62, 255};
    Color hideHi = (Color){110, 200, 85, 255};
    Color belly  = (Color){210, 225, 110, 255};
    Color gum    = (Color){90, 30, 40, 255};
    Color tooth  = (Color){250, 248, 235, 255};
    Color toothD = (Color){210, 205, 185, 255};

    if (drawAura) {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleV((Vector2){cx + R * 0.2f, cyb}, R * (1.55f + 0.12f * chomp),
                    (Color){255, 90, 15, (uchar)(18 + 20 * chomp)});
        DrawCircleV((Vector2){cx + R * 0.9f, cyb}, R * 0.70f,
                    (Color){255, 140, 30, (uchar)(10 + 18 * chomp)});
        EndBlendMode();
    }

    DrawEllipseRotF(cx - R * 1.05f, cyb + R * 0.55f,
                    R * 0.55f, R * 0.28f, 0.35f, hideD);
    DrawEllipseRotF(cx - R * 1.25f, cyb + R * 0.70f,
                    R * 0.35f, R * 0.18f, 0.55f, hide);
    DrawTriangle((Vector2){cx - R * 1.50f, cyb + R * 0.65f},
                 (Vector2){cx - R * 1.50f, cyb + R * 0.85f},
                 (Vector2){cx - R * 1.85f, cyb + R * 0.90f}, hideD);

    for (int i = 0; i < 9; i++) {
        float u = i / 8.0f;
        float sx = cx - R * 1.00f + u * R * 0.95f;
        float sy = cyb - R * 0.05f - u * R * 0.25f + sinf(u * PI) * R * 0.08f;
        float h  = R * (0.35f + 0.70f * sinf(u * PI) + 0.05f * sinf(t * 3.5f + i));
        float lean = sinf(t * 1.8f + i * 0.4f) * R * 0.05f;
        DrawTriangle((Vector2){sx - R * 0.14f, sy},
                     (Vector2){sx + R * 0.14f, sy},
                     (Vector2){sx + lean, sy - h}, hideD);
        DrawTriangle((Vector2){sx - R * 0.07f, sy - R * 0.02f},
                     (Vector2){sx + R * 0.07f, sy - R * 0.02f},
                     (Vector2){sx + lean * 0.5f, sy - h * 0.88f}, hideHi);
        DrawLineEx((Vector2){sx - R * 0.04f, sy - R * 0.02f},
                   (Vector2){sx + lean * 0.3f, sy - h * 0.9f},
                   R * 0.02f, (Color){160, 230, 120, 180});
    }

    DrawEllipseRotF(cx - R * 0.40f, cyb + R * 0.40f,
                    R * 0.95f, R * 0.80f, 0.20f, hideD);
    DrawEllipseRotF(cx - R * 0.25f, cyb + R * 0.30f,
                    R * 0.80f, R * 0.65f, 0.15f, hide);
    DrawEllipseRotF(cx - R * 0.15f, cyb + R * 0.20f,
                    R * 0.60f, R * 0.48f, 0.10f, hideM);
    DrawEllipseRotF(cx - R * 0.05f, cyb + R * 0.55f,
                    R * 0.48f, R * 0.38f, 0.25f, belly);
    DrawEllipseRotF(cx + R * 0.05f, cyb + R * 0.50f,
                    R * 0.28f, R * 0.22f, 0.2f, (Color){230, 240, 150, 200});
    for (int k = 0; k < 4; k++) {
        float by = cyb + R * 0.40f + k * R * 0.10f;
        DrawLineEx((Vector2){cx - R * 0.30f, by},
                   (Vector2){cx + R * 0.25f, by + R * 0.04f},
                   R * 0.025f, (Color){160, 175, 70, 140});
    }

    DrawEllipseRotF(cx + R * 0.08f, cyb - R * 0.02f,
                    R * 1.00f, R * 0.88f, -0.08f, hide);
    DrawEllipseRotF(cx + R * 0.12f, cyb - R * 0.12f,
                    R * 0.75f, R * 0.58f, -0.06f, hideM);
    DrawEllipseRotF(cx - R * 0.05f, cyb - R * 0.28f,
                    R * 0.40f, R * 0.22f, -0.3f, (Color){130, 210, 100, 120});

    for (int k = 0; k < 10; k++) {
        float sx = cx - R * 0.25f + SkinRnd(k + 40) * R * 0.70f;
        float sy = cyb - R * 0.35f + SkinRnd(k + 50) * R * 0.55f;
        DrawCircleV((Vector2){sx, sy}, R * (0.035f + SkinRnd(k) * 0.025f),
                    (Color){35, 90, 35, 160});
    }

    DrawEllipseRotF(cx + R * 0.85f, cyb + R * 0.02f,
                    R * 0.85f, R * 0.48f, 0.08f, hide);
    DrawEllipseRotF(cx + R * 1.05f, cyb - R * 0.02f,
                    R * 0.55f, R * 0.36f, 0.05f, hideM);
    DrawEllipseRotF(cx + R * 0.75f, cyb - R * 0.18f,
                    R * 0.55f, R * 0.16f, 0.05f, hideHi);
    DrawEllipseRotF(cx + R * 1.25f, cyb + R * 0.00f,
                    R * 0.28f, R * 0.24f, 0.0f, hide);
    DrawEllipseRotF(cx + R * 1.22f, cyb - R * 0.08f,
                    R * 0.09f, R * 0.11f, 0.2f, (Color){12, 22, 10, 255});
    DrawEllipseRotF(cx + R * 1.32f, cyb + R * 0.06f,
                    R * 0.08f, R * 0.10f, 0.15f, (Color){12, 22, 10, 255});
    DrawCircleV((Vector2){cx + R * 1.20f, cyb - R * 0.12f}, R * 0.025f,
                (Color){80, 140, 70, 180});

    DrawEllipseRotF(cx + R * 0.45f, cyb + R * 0.22f,
                    R * 0.35f, R * 0.28f, 0.1f, hideD);
    DrawEllipseRotF(cx + R * 0.18f, cyb - R * 0.38f,
                    R * 0.60f, R * 0.24f, -0.12f, hideD);
    DrawTriangle((Vector2){cx - R * 0.05f, cyb - R * 0.45f},
                 (Vector2){cx + R * 0.15f, cyb - R * 0.50f},
                 (Vector2){cx + R * 0.05f, cyb - R * 0.72f}, hideD);
    DrawTriangle((Vector2){cx + R * 0.25f, cyb - R * 0.42f},
                 (Vector2){cx + R * 0.45f, cyb - R * 0.45f},
                 (Vector2){cx + R * 0.38f, cyb - R * 0.68f}, hideD);
    DrawLineEx((Vector2){cx - R * 0.20f, cyb - R * 0.30f},
               (Vector2){cx + R * 0.60f, cyb - R * 0.16f},
               R * 0.13f, (Color){18, 45, 18, 255});
    DrawLineEx((Vector2){cx - R * 0.15f, cyb - R * 0.34f},
               (Vector2){cx + R * 0.55f, cyb - R * 0.20f},
               R * 0.06f, (Color){12, 30, 12, 255});

    float blinkPhase = fmodf(t, 4.8f);
    float blink = 0.0f;
    if (blinkPhase > 4.64f) {
        float u = (blinkPhase - 4.64f) / 0.16f;
        blink = sinf(u * PI);
    }
    float eyeX = cx + R * 0.24f;
    float eyeY = cyb - R * 0.10f;
    float eyeOuterH = R * (0.20f - blink * 0.08f);
    float irisH = R * (0.14f - blink * 0.04f);
    DrawEllipseRotF(eyeX, eyeY, R * 0.27f, eyeOuterH, 0.0f, (Color){10, 18, 8, 255});
    DrawEllipseRotF(eyeX, eyeY, R * 0.18f, irisH, 0.0f, (Color){255, 225, 35, 255});
    DrawEllipseRotF(eyeX, eyeY, R * 0.12f, R * 0.11f, 0.0f, (Color){255, 160, 20, 255});
    // Keep the vertical pupil large enough to remain readable at in-game scale.
    DrawEllipseRotF(eyeX + R * 0.015f, eyeY, R * 0.055f, R * 0.095f,
                    0.0f, (Color){8, 8, 8, 255});
    DrawCircleV((Vector2){eyeX - R * 0.06f, eyeY - R * 0.07f}, R * 0.035f,
                (Color){255, 255, 255, 230});
    if (blink > 0.02f) {
        DrawLineEx((Vector2){eyeX - R * 0.22f, eyeY - R * 0.11f},
                   (Vector2){eyeX + R * 0.22f, eyeY - R * 0.11f},
                   R * 0.045f, (Color){28, 72, 32, 230});
    }

    float mouthY = cyb + R * 0.30f;
    DrawEllipse((int)(cx + R * 0.60f), (int)(mouthY + jawOpen * 0.35f),
                (int)(R * 0.55f), (int)(R * 0.18f + jawOpen * 0.7f),
                (Color){20, 8, 10, 255});
    DrawEllipse((int)(cx + R * 0.60f), (int)(mouthY - R * 0.02f),
                (int)(R * 0.52f), (int)(R * 0.10f), gum);
    for (int k = 0; k < 7; k++) {
        float u  = k / 6.0f;
        float tx = cx + R * 0.30f + u * R * 0.95f;
        float ty = mouthY + R * 0.02f;
        float th = R * (0.14f + 0.10f * sinf(u * PI) + ((k == 1 || k == 2) ? 0.08f : 0.0f));
        float tw = R * (0.055f + ((k == 1 || k == 2) ? 0.02f : 0.0f));
        DrawTriangle((Vector2){tx - tw, ty},
                     (Vector2){tx + tw, ty},
                     (Vector2){tx + (SkinRnd(k) - 0.5f) * R * 0.02f, ty + th},
                     (k % 2) ? tooth : toothD);
        DrawLineEx((Vector2){tx - tw * 0.3f, ty + R * 0.02f},
                   (Vector2){tx, ty + th * 0.7f},
                   R * 0.015f, (Color){255, 255, 255, 160});
    }

    float jy = cyb + R * 0.48f + jawOpen;
    DrawEllipseRotF(cx + R * 0.60f, jy,
                    R * 0.78f, R * 0.34f, 0.06f, hideD);
    DrawEllipseRotF(cx + R * 0.78f, jy + R * 0.04f,
                    R * 0.52f, R * 0.22f, 0.05f, hide);
    DrawEllipseRotF(cx + R * 0.70f, jy + R * 0.08f,
                    R * 0.30f, R * 0.10f, 0.0f, hideM);
    DrawEllipse((int)(cx + R * 0.58f), (int)(jy - R * 0.10f),
                (int)(R * 0.48f), (int)(R * 0.08f), gum);
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

    if (jawOpen > R * 0.12f) {
        float tong = (jawOpen - R * 0.12f) / (R * 0.10f);
        if (tong > 1.0f) tong = 1.0f;
        float twag = sinf(t * 6.0f) * R * 0.04f;
        DrawEllipse((int)(cx + R * 0.70f + twag), (int)(mouthY + jawOpen * 0.5f),
                    (int)(R * 0.18f * tong), (int)(R * 0.10f + jawOpen * 0.2f),
                    (Color){200, 40, 55, (uchar)(220 * tong)});
    }

    if (chomp > 0.55f) {
        float pr = (chomp - 0.55f) / 0.45f;
        for (int k = 0; k < 7; k++) {
            float px = cx + R * 1.30f + k * R * 0.10f + SkinRnd(k + 20) * R * 0.08f;
            float py = cyb - R * 0.05f + (SkinRnd(k + 30) - 0.5f) * R * 0.25f
                       - pr * R * 0.15f;
            DrawCircleV((Vector2){px, py},
                        R * (0.05f + SkinRnd(k) * 0.04f) * pr,
                        (Color){255, (uchar)(140 + k * 10), 30, (uchar)(160 * pr * (1.0f - k * 0.1f))});
        }
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

void DrawApexRex(float cx, float cy, float R, float t) {
    // Shop preview keeps the original fiery aura.
    DrawApexRexCore(cx, cy, R, t, true);
}

static RenderTexture2D apexRexGameTarget = {0};

void DrawApexRexInGame(float cx, float cy, float R, float t, bool flipX) {
    float gameR = R * 1.20f;
    if (!flipX) {
        DrawApexRexCore(cx, cy, gameR, t, false);
        return;
    }

    // Draw normally into a transparent canvas and flip the finished image.
    // This avoids negative geometry scaling, which reverses triangle winding
    // and can make the procedural Rex disappear under backface culling.
    if (apexRexGameTarget.id <= 0)
        apexRexGameTarget = LoadRenderTexture(128, 128);
    if (apexRexGameTarget.id <= 0) {
        DrawApexRexCore(cx, cy, gameR, t, false);
        return;
    }

    Matrix savedProjection = rlGetMatrixProjection();
    Matrix savedModelview  = rlGetMatrixModelview();
    BeginTextureMode(apexRexGameTarget);
    ClearBackground(BLANK);
    DrawApexRexCore(64.0f, 64.0f, gameR, t, false);
    EndTextureMode();
    rlSetMatrixProjection(savedProjection);
    rlSetMatrixModelview(savedModelview);

    // The negative source width flips only the finished texture quad; the
    // negative height corrects the render-texture's upside-down Y orientation.
    Rectangle source = {32.0f, 32.0f, -64.0f, -64.0f};
    Rectangle dest = {cx - 32.0f, cy - 32.0f, 64.0f, 64.0f};
    DrawTexturePro(apexRexGameTarget.texture, source, dest,
                   (Vector2){0, 0}, 0.0f, WHITE);
}

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
            DrawEllipseRotF(xw, yw, rr, rr * 1.25f, ang, face);
            DrawEllipseRotF(xw, yw + rr * 0.35f, rr * 0.85f, rr * 0.40f, ang, scBase);
            DrawEllipseRotF(xw - rr * 0.15f, yw - rr * 0.35f,
                        rr * 0.40f, rr * 0.28f, ang - 0.2f,
                        (Color){scHi.r, scHi.g, scHi.b, (uchar)(90 + 50 * p2)});
            if ((row + col) % 5 == 0)
                DrawCircleV((Vector2){xw, yw - rr * 0.15f}, rr * 0.18f,
                            (Color){rivet.r, rivet.g, rivet.b, 180});
        }
    }
}

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

void DrawBloodscaleVanguard(float cx, float cy, float R, float t) {
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
        DrawEllipseRotF(mx, my,
                    R * (0.55f - u * 0.18f), R * (0.30f - u * 0.06f),
                    0.4f + u * 0.35f + sinf(t + i) * 0.08f,
                    (Color){(uchar)(40 + i * 2), 4, 10, (uchar)(230 - i * 16)});
        // subtle gold thread on top folds
        if (i < 3)
            DrawEllipseRotF(mx - R * 0.05f, my - R * 0.04f,
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

            DrawEllipseRotF(xw, yw, rr, rr * 1.25f, ang, face);
            // underside shadow of scale
            DrawEllipseRotF(xw, yw + rr * 0.35f, rr * 0.85f, rr * 0.40f, ang, scBase);
            // top glint
            DrawEllipseRotF(xw - rr * 0.15f, yw - rr * 0.35f,
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
            DrawEllipseRotF(ox, oy, R * 0.14f, R * 0.18f, s * 0.25f + (k - 2.5f) * 0.1f, scLit);
            DrawEllipseRotF(ox - s * R * 0.03f, oy - R * 0.05f, R * 0.06f, R * 0.05f,
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
        DrawHaloArc(px, py, R * 0.42f, R * 0.32f, -2.2f, 0.4f, R * 0.04f, gold);
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
        DrawEllipseRotF(tx + sway, ty, R * 0.12f, R * 0.18f, 0.05f * k, scLit);
        DrawEllipseRotF(tx + sway, ty + R * 0.12f, R * 0.10f, R * 0.14f, 0.05f * k, scMid);
        DrawCircleV((Vector2){tx + sway, ty - R * 0.06f}, R * 0.025f, gold);
    }

    // â”€â”€ Great helm â€” tall free silhouette, scale-faced â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    // neck gorget scales
    for (int k = 0; k < 5; k++) {
        float a = -0.6f + k * 0.3f;
        float gx = cx + sinf(a) * R * 0.45f;
        float gy = cyb - R * 0.55f + cosf(a) * R * 0.08f;
        DrawEllipseRotF(gx, gy, R * 0.12f, R * 0.10f, a, scLit);
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
        DrawEllipseRotF(ex, ey, R * 0.06f, R * 0.08f, a, scHot);
    }
    DrawCircleV((Vector2){cx, cyb + R * 0.05f}, R * 0.04f, goldHi);

    // â”€â”€ Ember motes rising off the armor â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    for (int k = 0; k < 10; k++) {
        float pr = fmodf(t * 0.35f + SkinRnd(k + 60), 1.0f);
        float sx = cx + (SkinRnd(k + 70) - 0.5f) * R * 1.6f;
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

void DrawStormforgedAegis(float cx, float cy, float R, float t) {
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
        DrawEllipseRotF(cx - R * 0.40f + sway, cyb + R * 0.25f + u * R * 1.0f,
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
            DrawEllipseRotF(ox, py - R * 0.02f, R * 0.12f, R * 0.16f, s * 0.2f, scHi);
        }
        DrawTriangle((Vector2){px - R * 0.10f, py - R * 0.15f},
                     (Vector2){px + R * 0.10f, py - R * 0.15f},
                     (Vector2){px + s * R * 0.05f, py - R * 0.55f}, scHi);
        DrawHaloArc(px, py, R * 0.42f, R * 0.32f, -2.2f, 0.4f, R * 0.035f, gold);
    }

    // Belt + tassets
    DrawRectangleRounded((Rectangle){cx - R * 0.70f, cyb + R * 0.55f, R * 1.40f, R * 0.14f}, 0.4f, 4, gold);
    for (int k = -3; k <= 3; k++) {
        float sway = sinf(t * 2.4f + k * 0.6f) * R * 0.035f;
        DrawEllipseRotF(cx + k * R * 0.18f + sway, cyb + R * 0.78f,
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
        float pr = fmodf(t * 0.4f + SkinRnd(k + 20), 1.0f);
        DrawCircleV((Vector2){cx + (SkinRnd(k) - 0.5f) * R * 1.5f, cyb + R * 0.5f - pr * R * 1.6f},
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

void DrawSunforgedCrusader(float cx, float cy, float R, float t) {
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
        DrawEllipseRotF(cx + sway, cyb + R * 0.30f + u * R * 0.95f,
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
        DrawHaloArc(px, py, R * 0.42f, R * 0.30f, -2.0f, 0.5f, R * 0.04f, white);
    }

    DrawRectangleRounded((Rectangle){cx - R * 0.68f, cyb + R * 0.55f, R * 1.36f, R * 0.14f}, 0.4f, 4, gold);
    for (int k = -3; k <= 3; k++) {
        float sway = sinf(t * 2.2f + k) * R * 0.03f;
        DrawEllipseRotF(cx + k * R * 0.17f + sway, cyb + R * 0.78f, R * 0.11f, R * 0.15f, 0, scLit);
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
        float pr = fmodf(t * 0.35f + SkinRnd(k + 40), 1.0f);
        DrawCircleV((Vector2){cx + (SkinRnd(k + 41) - 0.5f) * R * 1.5f, cyb + R * 0.4f - pr * R * 1.7f},
                    R * (0.035f - pr * 0.015f),
                    (Color){255, 220, 80, (uchar)(160 * (1.0f - pr))});
    }
}

// Draws the facial details for epic skins (27-29) on top of a body circle of
// radius R centred at (cx,cy).  All offsets scale with R so the same code works
// for the in-game player (R≈12) and the larger shop preview (R≈18).
void DrawEpicFace(int id, float cx, float cy, float R, float t) {
    if (id == 27) {
        // ── Alien (emoji): light-green head, big slanted black eyes,
        //    two antennae with bulbs, small smile ──
        float ab = sinf(t * 2.0f) * (R * 0.16f);     // antenna bob
        Color stalk = (Color){ 78, 150, 44, 240 };
        Color bulb  = (Color){ 150, 220, 80, 255 };
        DrawLineEx((Vector2){cx - R*0.34f, cy - R*0.72f},
                   (Vector2){cx - R*0.64f, cy - R*1.16f - ab}, R*0.11f, stalk);
        DrawCircleV((Vector2){cx - R*0.64f, cy - R*1.16f - ab}, R*0.18f, bulb);
        DrawLineEx((Vector2){cx + R*0.34f, cy - R*0.72f},
                   (Vector2){cx + R*0.64f, cy - R*1.16f + ab}, R*0.11f, stalk);
        DrawCircleV((Vector2){cx + R*0.64f, cy - R*1.16f + ab}, R*0.18f, bulb);

        // soft top-left sheen
        DrawCircleV((Vector2){cx - R*0.30f, cy - R*0.40f}, R*0.30f, (Color){185,235,125,90});

        // big slanted almond eyes (black), mirrored
        DrawEllipseRotF(cx - R*0.40f, cy - R*0.02f, R*0.42f, R*0.25f,  0.52f, (Color){10,14,10,255});
        DrawEllipseRotF(cx + R*0.40f, cy - R*0.02f, R*0.42f, R*0.25f, -0.52f, (Color){10,14,10,255});
        // subtle glints
        DrawCircleV((Vector2){cx - R*0.52f, cy - R*0.12f}, R*0.07f, (Color){255,255,255,190});
        DrawCircleV((Vector2){cx + R*0.30f, cy - R*0.12f}, R*0.07f, (Color){255,255,255,190});

        // small smile
        Color mouth = (Color){ 42, 92, 28, 235 };
        float mcx = cx, mcy = cy + R*0.30f, mr = R*0.18f;
        Vector2 prev = {0};
        for (int s = 0; s <= 8; s++) {
            float th = PI/3.0f + (PI/3.0f) * (s / 8.0f);   // 60°..120° (smile arc)
            Vector2 p = { mcx + cosf(th)*mr, mcy + sinf(th)*mr };
            if (s > 0) DrawLineEx(prev, p, R*0.09f, mouth);
            prev = p;
        }
    }
    else if (id == 29) {
        // ── Angel: golden eyes + front half of the halo ring ──
        // (wings, glow and the rear halo arc are drawn in DrawEpicBack)
        DrawCircleV((Vector2){cx-R*0.33f, cy-R*0.06f}, R*0.16f, (Color){255,210,80,255});
        DrawCircleV((Vector2){cx+R*0.33f, cy-R*0.06f}, R*0.16f, (Color){255,210,80,255});

        // front half of the halo (lower arc, θ 0°..180°) — sits over the brow,
        // above the eyes so it never covers them.
        float hx, hy, rx, ry;  HaloGeom(cx, cy, R, t, &hx, &hy, &rx, &ry);
        DrawHaloArc(hx, hy, rx, ry, 0.0f, PI, R*0.22f, (Color){255,200,50,70});
        DrawHaloArc(hx, hy, rx, ry, 0.0f, PI, R*0.12f, (Color){255,214,72,235});
    }
    else if (id == 32) {
        // Midas (Gilded Sovereign) — soft molten-gold eyes
        DrawEllipseRotF(cx-R*0.30f, cy-R*0.05f, R*0.13f, R*0.17f, 0.0f, (Color){255,240,190,255});
        DrawEllipseRotF(cx+R*0.30f, cy-R*0.05f, R*0.13f, R*0.17f, 0.0f, (Color){255,240,190,255});
    }
    else if (id == 33) {
        // Blade Legend — fierce ember eyes + war-paint stripes
        DrawEllipseRotF(cx-R*0.30f, cy-R*0.02f, R*0.20f, R*0.13f, -0.25f, (Color){20,5,5,255});
        DrawEllipseRotF(cx+R*0.30f, cy-R*0.02f, R*0.20f, R*0.13f,  0.25f, (Color){20,5,5,255});
        DrawCircleV((Vector2){cx-R*0.30f, cy-R*0.02f}, R*0.05f, (Color){255,120,90,255});
        DrawCircleV((Vector2){cx+R*0.30f, cy-R*0.02f}, R*0.05f, (Color){255,120,90,255});
        Color paint = (Color){230,40,30,230};
        DrawLineEx((Vector2){cx-R*0.55f,cy+R*0.28f},(Vector2){cx-R*0.20f,cy+R*0.42f}, R*0.08f, paint);
        DrawLineEx((Vector2){cx+R*0.20f,cy+R*0.42f},(Vector2){cx+R*0.55f,cy+R*0.28f}, R*0.08f, paint);
    }
}

// ─── RARE SKIN FACES (shop preview) ──────────────────────────────────────────
// Behind-body effects for rare skins (22-26): auras, flames, tails, halos.
void DrawRareBack(int id, float cx, float cy, float R, float t) {
    switch (id) {
        case 22: { // Pyro — pulsing ember aura + rising flame tongues
            float p = 0.5f + 0.5f*sinf(t*6.0f);
            DrawCircleV((Vector2){cx, cy}, R*(1.32f+0.10f*p), (Color){255,70,0,30});
            DrawCircleV((Vector2){cx, cy}, R*(1.12f+0.08f*p), (Color){255,150,0,42});
            for (int f=-1; f<=1; f++) {
                float fl = sinf(t*7.0f + f*1.7f);
                float bx = cx + f*R*0.55f, by = cy - R*0.70f;
                float tipx = bx + fl*R*0.18f;
                float tipy = by - R*(0.85f + 0.32f*(0.5f+0.5f*fl));
                DrawTriangle((Vector2){bx-R*0.30f, by},(Vector2){tipx, tipy},(Vector2){bx+R*0.30f, by},(Color){255,90,0,180});
                DrawTriangle((Vector2){bx-R*0.30f, by},(Vector2){bx+R*0.30f, by},(Vector2){tipx, tipy},(Color){255,90,0,180});
                DrawTriangle((Vector2){bx-R*0.16f, by},(Vector2){tipx, tipy+R*0.20f},(Vector2){bx+R*0.16f, by},(Color){255,215,70,225});
                DrawTriangle((Vector2){bx-R*0.16f, by},(Vector2){bx+R*0.16f, by},(Vector2){tipx, tipy+R*0.20f},(Color){255,215,70,225});
            }
            break;
        }
        case 23: { // Phantom — classic floating sheet-ghost silhouette + aura
            DrawCircleV((Vector2){cx, cy}, R*1.34f, (Color){150,90,255,26});
            Color gb = (Color){206,190,255,175};
            // full lavender silhouette (head + lower body) so the whole ghost
            // reads uniformly; the translucent body circle only tints it.
            DrawCircleV((Vector2){cx, cy}, R, gb);
            DrawRectangleRounded((Rectangle){cx-R*0.96f, cy-R*0.05f, R*1.92f, R*1.0f}, 0.45f, 8, gb);
            // three softly bobbing scalloped bumps along the hem
            for (int k=-1;k<=1;k++) {
                float bob = sinf(t*3.0f + k*1.1f) * R*0.10f;
                DrawCircleV((Vector2){cx + k*R*0.63f, cy + R*0.92f + bob}, R*0.34f, gb);
            }
            break;
        }
        case 24: { // Cyber — neon scan rings + back glow
            float p = 0.5f + 0.5f*sinf(t*4.0f);
            DrawCircleV((Vector2){cx, cy}, R*1.30f, (Color){0,220,255,26});
            DrawCircleLines((int)cx,(int)cy,(int)(R*(1.15f+0.12f*p)),(Color){0,240,255,120});
            DrawCircleLines((int)cx,(int)cy,(int)(R*1.34f),(Color){255,40,160,70});
            break;
        }
        case 25: { // Chroma — soft white aura + orbiting prismatic sparkles
            DrawCircleV((Vector2){cx, cy}, R*1.30f, (Color){255,255,255,22});
            for (int k=0;k<5;k++){
                float ang = t*0.8f + k*(PI*2.0f/5.0f);
                float sxp = cx + cosf(ang)*R*1.24f;
                float syp = cy + sinf(ang)*R*1.24f;
                float tw  = 0.5f+0.5f*sinf(t*4.0f + k*1.3f);
                Color c   = ColorFromHSV(fmodf(k*72.0f + t*120.0f,360.0f),1.0f,1.0f);
                float sr  = R*(0.15f + 0.10f*tw);
                Color cc  = (Color){c.r,c.g,c.b,(uchar)(110+120*tw)};
                DrawLineEx((Vector2){sxp-sr,syp},(Vector2){sxp+sr,syp}, R*0.05f, cc);
                DrawLineEx((Vector2){sxp,syp-sr},(Vector2){sxp,syp+sr}, R*0.05f, cc);
            }
            break;
        }
        case 26: { // Biohazard — toxic pulsing aura + bubbling drips
            float p = 0.5f + 0.5f*sinf(t*5.0f);
            DrawCircleV((Vector2){cx, cy}, R*(1.26f+0.10f*p), (Color){80,255,0,30});
            for (int d=-1; d<=1; d++) {
                float dy = fmodf(t*3.0f + d*2.1f, 2.0f);
                float bx = cx + d*R*0.5f;
                float by = cy + R*0.9f + dy*R*0.5f;
                DrawCircleV((Vector2){bx, by}, R*0.12f*(1.0f-dy*0.4f),
                            (Color){120,255,30,(uchar)(160*(1.0f-dy*0.5f))});
            }
            break;
        }
        case 30: { // Chronovore — amber aura + gear-tooth clock rim
            DrawCircleV((Vector2){cx, cy}, R*1.25f, (Color){200,150,60,18});
            int teeth = 14;
            for (int k=0;k<teeth;k++){
                float ang = (2.0f*PI/teeth)*k;
                DrawLineEx((Vector2){cx+cosf(ang)*R*0.98f,cy+sinf(ang)*R*0.98f},
                           (Vector2){cx+cosf(ang)*R*1.14f,cy+sinf(ang)*R*1.14f}, R*0.07f, (Color){170,130,60,220});
            }
            break;
        }
        case 31: { // Deep Cartographer — brass compass rim with cardinal ticks
            DrawCircleV((Vector2){cx, cy}, R*1.28f, (Color){210,180,120,24});
            DrawRing((Vector2){cx,cy}, R*1.12f, R*1.22f, 0, 360, 48, (Color){150,110,60,150});
            for (int k=0;k<16;k++){
                float ang = (float)k/16.0f * 2.0f*PI;
                bool major = (k%4==0);
                float innerR = major ? R*0.98f : R*1.05f;
                float outerR = R*1.18f;
                Color tc = major ? (Color){235,90,60,220} : (Color){190,160,100,140};
                DrawLineEx((Vector2){cx+cosf(ang)*innerR,cy+sinf(ang)*innerR},
                           (Vector2){cx+cosf(ang)*outerR,cy+sinf(ang)*outerR}, major?R*0.05f:R*0.025f, tc);
            }
            break;
        }
        case 33: { // Blade Legend — Skin Ideas #27, twin blades high on the back
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
                Vector2 tip  = {cx + side * ca * R * 1.38f,
                                cy - sa * R * 1.38f - R * 0.22f};
                Vector2 hilt = {cx + side * ca * R * 0.52f, cy + R * 0.03f};
                DrawLineEx(hilt, tip, R * 0.26f, (Color){45, 42, 55, 255});
                DrawLineEx(hilt, tip, R * 0.12f, (Color){200, 205, 220, 255});
                BeginBlendMode(BLEND_ADDITIVE);
                DrawLineEx(hilt, tip, R * 0.05f,
                           (Color){255, 90, 40, (uchar)(160 + 60 * pulse)});
                EndBlendMode();
                Vector2 dir = {tip.x - hilt.x, tip.y - hilt.y};
                float invLen = 1.0f / sqrtf(dir.x * dir.x + dir.y * dir.y);
                Vector2 perp = {-dir.y * invLen, dir.x * invLen};
                Vector2 mid = {hilt.x * 0.35f + tip.x * 0.65f,
                               hilt.y * 0.35f + tip.y * 0.65f};
                DrawLineEx((Vector2){mid.x - perp.x * R * 0.28f,
                                     mid.y - perp.y * R * 0.28f},
                           (Vector2){mid.x + perp.x * R * 0.28f,
                                     mid.y + perp.y * R * 0.28f},
                           R * 0.08f, (Color){255, 180, 60, 240});
            }
            break;
        }
        case 34:
        case 35:
            // Their art remains the former legendary art; only shop rarity changes.
            DrawLegendaryBack(id, cx, cy, R, t);
            break;
    }
}

// Front facial details for rare skins (22-26, 28, 30-35), drawn over the body circle.
void DrawRareFace(int id, float cx, float cy, float R, float t) {
    switch (id) {
        case 22: { // Pyro — fierce angry eyes + furrowed molten brows
            DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.50f}, R*0.30f, (Color){255,230,120,90});
            DrawEllipseRotF(cx-R*0.34f, cy-R*0.02f, R*0.26f, R*0.16f, -0.40f, (Color){35,10,0,255});
            DrawEllipseRotF(cx+R*0.34f, cy-R*0.02f, R*0.26f, R*0.16f,  0.40f, (Color){35,10,0,255});
            DrawCircleV((Vector2){cx-R*0.34f, cy}, R*0.07f, (Color){255,240,180,255});
            DrawCircleV((Vector2){cx+R*0.34f, cy}, R*0.07f, (Color){255,240,180,255});
            DrawLineEx((Vector2){cx-R*0.56f, cy-R*0.32f},(Vector2){cx-R*0.14f, cy-R*0.16f}, R*0.11f, (Color){70,18,0,255});
            DrawLineEx((Vector2){cx+R*0.56f, cy-R*0.32f},(Vector2){cx+R*0.14f, cy-R*0.16f}, R*0.11f, (Color){70,18,0,255});
            break;
        }
        case 23: { // Phantom — big oval ghost eyes + round wailing mouth
            float p = 0.5f+0.5f*sinf(t*2.5f);
            DrawEllipseRotF(cx-R*0.32f, cy-R*0.10f, R*0.16f, R*0.23f, 0.0f, (Color){48,16,92,255});
            DrawEllipseRotF(cx+R*0.32f, cy-R*0.10f, R*0.16f, R*0.23f, 0.0f, (Color){48,16,92,255});
            DrawCircleV((Vector2){cx-R*0.29f, cy-R*0.17f}, R*0.05f, (Color){240,230,255,(uchar)(170+70*p)});
            DrawCircleV((Vector2){cx+R*0.35f, cy-R*0.17f}, R*0.05f, (Color){240,230,255,(uchar)(170+70*p)});
            DrawEllipseRotF(cx, cy+R*0.42f, R*0.12f, R*0.18f, 0.0f, (Color){48,16,92,255});
            break;
        }
        case 24: { // Cyber — glowing visor band + sliding scan dot + antenna
            float p = 0.5f+0.5f*sinf(t*5.0f);
            DrawRectangleRounded((Rectangle){cx-R*0.62f, cy-R*0.20f, R*1.24f, R*0.42f}, 0.6f, 6, (Color){8,18,28,255});
            DrawRectangleRounded((Rectangle){cx-R*0.58f, cy-R*0.11f, R*1.16f, R*0.17f}, 0.9f, 6, (Color){0,240,255,(uchar)(180+60*p)});
            float sx = cx - R*0.50f + (0.5f+0.5f*sinf(t*4.0f))*R*1.0f;
            DrawCircleV((Vector2){sx, cy-R*0.02f}, R*0.07f, (Color){255,255,255,255});
            DrawLineEx((Vector2){cx+R*0.66f, cy-R*0.52f},(Vector2){cx+R*0.95f, cy-R*0.98f}, R*0.08f, (Color){255,40,160,255});
            DrawCircleV((Vector2){cx+R*0.95f, cy-R*0.98f}, R*0.10f, (Color){255,60,180,255});
            break;
        }
        case 25: { // Chroma — glossy eyes + big twinkling star-shine
            DrawCircleV((Vector2){cx-R*0.30f, cy-R*0.05f}, R*0.15f, (Color){255,255,255,245});
            DrawCircleV((Vector2){cx+R*0.30f, cy-R*0.05f}, R*0.15f, (Color){255,255,255,245});
            DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.03f}, R*0.07f, (Color){30,30,55,255});
            DrawCircleV((Vector2){cx+R*0.32f, cy-R*0.03f}, R*0.07f, (Color){30,30,55,255});
            DrawCircleV((Vector2){cx-R*0.31f, cy-R*0.07f}, R*0.03f, (Color){255,255,255,255});
            DrawCircleV((Vector2){cx+R*0.29f, cy-R*0.07f}, R*0.03f, (Color){255,255,255,255});
            float p  = 0.5f+0.5f*sinf(t*4.5f);
            Color sh = (Color){255,255,255,(uchar)(160+90*p)};
            float sr = R*(0.24f+0.10f*p);
            float scx = cx+R*0.42f, scy = cy-R*0.44f;
            DrawLineEx((Vector2){scx-sr,scy},(Vector2){scx+sr,scy}, R*0.05f, sh);
            DrawLineEx((Vector2){scx,scy-sr},(Vector2){scx,scy+sr}, R*0.05f, sh);
            DrawLineEx((Vector2){scx-sr*0.6f,scy-sr*0.6f},(Vector2){scx+sr*0.6f,scy+sr*0.6f}, R*0.035f, sh);
            DrawLineEx((Vector2){scx-sr*0.6f,scy+sr*0.6f},(Vector2){scx+sr*0.6f,scy-sr*0.6f}, R*0.035f, sh);
            break;
        }
        case 26: { // Biohazard — glowing trefoil symbol
            Color sym  = (Color){20,42,0,255};
            Color glow = (Color){185,255,45,255};
            for (int k=0;k<3;k++){
                float ang = -PI/2.0f + k*(PI*2.0f/3.0f);
                float bx = cx + cosf(ang)*R*0.42f;
                float by = cy + sinf(ang)*R*0.42f + R*0.05f;
                DrawCircleV((Vector2){bx,by}, R*0.27f, sym);
                DrawCircleV((Vector2){bx,by}, R*0.15f, glow);
            }
            DrawCircleV((Vector2){cx, cy+R*0.05f}, R*0.17f, sym);
            DrawCircleV((Vector2){cx, cy+R*0.05f}, R*0.08f, (Color){10,22,0,255});
            break;
        }
        case 30: { // Chronovore — swinging pendulum + dark clockwork eyes
            float swing = sinf(t*1.6f)*0.5f;
            Vector2 pTip = { cx+sinf(swing)*R*0.55f, cy+R*0.75f };
            DrawLineEx((Vector2){cx,cy-R*0.1f}, pTip, R*0.05f, (Color){60,45,20,255});
            DrawCircleV(pTip, R*0.16f, (Color){215,180,90,255});
            DrawCircleV((Vector2){cx-R*0.28f, cy-R*0.35f}, R*0.08f, (Color){40,25,10,255});
            DrawCircleV((Vector2){cx+R*0.28f, cy-R*0.35f}, R*0.08f, (Color){40,25,10,255});
            break;
        }
        case 31: { // Deep Cartographer — full compass-rose face with eyes + swinging needle
            DrawCircleV((Vector2){cx,cy}, R*0.66f, (Color){70,54,30,200});
            DrawCircleLines((int)cx,(int)cy,(int)(R*0.66f),(Color){230,200,140,220});
            DrawCircleLines((int)cx,(int)cy,(int)(R*0.50f),(Color){200,170,110,150});
            for (int k=0;k<8;k++){
                float ang = (float)k/8.0f * 2.0f*PI - PI/2.0f;
                bool cardinal = (k%2==0);
                float len = cardinal ? R*0.60f : R*0.40f;
                Vector2 tip   = { cx+cosf(ang)*len, cy+sinf(ang)*len };
                Vector2 baseL = { cx+cosf(ang-0.12f)*R*0.10f, cy+sinf(ang-0.12f)*R*0.10f };
                Vector2 baseR = { cx+cosf(ang+0.12f)*R*0.10f, cy+sinf(ang+0.12f)*R*0.10f };
                Color col = (k==0) ? (Color){225,60,50,230} : (Color){225,205,170,210};
                DrawTriangle(baseL, tip, baseR, col);
            }
            float needleAng = sinf(t*0.8f)*0.5f;
            Vector2 nTip  = { cx+cosf(needleAng-PI/2)*R*0.5f, cy+sinf(needleAng-PI/2)*R*0.5f };
            Vector2 nTail = { cx-cosf(needleAng-PI/2)*R*0.3f, cy-sinf(needleAng-PI/2)*R*0.3f };
            DrawLineEx(nTail, nTip, R*0.06f, (Color){235,60,50,240});
            DrawCircleV((Vector2){cx,cy}, R*0.07f, (Color){250,235,200,255});
            DrawCircleV((Vector2){cx-R*0.24f, cy-R*0.42f}, R*0.075f, (Color){40,28,14,255});
            DrawCircleV((Vector2){cx+R*0.24f, cy-R*0.42f}, R*0.075f, (Color){40,28,14,255});
            DrawCircleV((Vector2){cx-R*0.22f, cy-R*0.44f}, R*0.025f, (Color){255,245,220,255});
            DrawCircleV((Vector2){cx+R*0.26f, cy-R*0.44f}, R*0.025f, (Color){255,245,220,255});
            break;
        }
        case 33: { // Blade Legend — the exact Skin Ideas #27 face treatment
            DrawEllipseRotF(cx-R*0.30f, cy-R*0.02f, R*0.20f, R*0.13f, -0.25f, (Color){20,5,5,255});
            DrawEllipseRotF(cx+R*0.30f, cy-R*0.02f, R*0.20f, R*0.13f,  0.25f, (Color){20,5,5,255});
            DrawCircleV((Vector2){cx-R*0.30f, cy-R*0.02f}, R*0.05f, (Color){255,120,90,255});
            DrawCircleV((Vector2){cx+R*0.30f, cy-R*0.02f}, R*0.05f, (Color){255,120,90,255});
            Color paint = (Color){230,40,30,230};
            DrawLineEx((Vector2){cx-R*0.55f,cy+R*0.28f},(Vector2){cx-R*0.20f,cy+R*0.42f}, R*0.08f, paint);
            DrawLineEx((Vector2){cx+R*0.20f,cy+R*0.42f},(Vector2){cx+R*0.55f,cy+R*0.28f}, R*0.08f, paint);
            break;
        }
        case 34:
        case 35:
            // Their art remains the former legendary art; only shop rarity changes.
            DrawLegendaryFace(id, cx, cy, R, t);
            break;
    }
}

// ─── CONSTELLATION / ASCENDANT ART (shared by Rare shop + in-game) ────────────
// These two skins moved to Rare, but their original layered art is preserved.
void DrawLegendaryBack(int id, float cx, float cy, float R, float t) {
    if (id == 34) {
        // Constellation — deep indigo aura + a slow orbiting field of tiny stars
        DrawCircleV((Vector2){cx, cy}, R*1.45f, (Color){120,100,255,26});
        DrawCircleV((Vector2){cx, cy}, R*1.20f, (Color){80,60,200,20});
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
    }
    else if (id == 35) {
        // Ascendant — wide radiant glow + rear half of a rotating rainbow halo
        DrawCircleV((Vector2){cx, cy}, R*1.55f, (Color){255,240,190,26});
        DrawCircleV((Vector2){cx, cy}, R*1.30f, (Color){255,225,140,30});
        float hx, hy, rx, ry;  HaloGeom(cx, cy, R*1.15f, t, &hx, &hy, &rx, &ry);
        const int SLICES = 10;
        for (int s = 0; s < SLICES; s++) {
            float a0 = PI + (PI * s)     / SLICES;
            float a1 = PI + (PI * (s+1)) / SLICES;
            Color rc = ColorFromHSV(fmodf(t*50.0f + s*36.0f, 360.0f), 0.7f, 1.0f);
            DrawHaloArc(hx, hy, rx, ry, a0, a1, R*0.16f, (Color){rc.r,rc.g,rc.b,150});
        }
    }
}

// Front facial details for legendary skins (34-35), drawn over the body circle.
void DrawLegendaryFace(int id, float cx, float cy, float R, float t) {
    if (id == 34) {
        // Constellation — glowing star-shaped eyes linked by faint constellation lines
        Color starCol = (Color){225,225,255,(uchar)(200+40*(0.5f+0.5f*sinf(t*2.0f)))};
        DrawCircleV((Vector2){cx-R*0.30f, cy-R*0.04f}, R*0.09f, starCol);
        DrawCircleV((Vector2){cx+R*0.30f, cy-R*0.04f}, R*0.09f, starCol);
        DrawCircleV((Vector2){cx, cy+R*0.30f}, R*0.06f, starCol);
        DrawLineEx((Vector2){cx-R*0.30f,cy-R*0.04f},(Vector2){cx,cy+R*0.30f}, R*0.025f, (Color){190,190,255,120});
        DrawLineEx((Vector2){cx+R*0.30f,cy-R*0.04f},(Vector2){cx,cy+R*0.30f}, R*0.025f, (Color){190,190,255,120});
        for (int k=0;k<5;k++) {
            float ang = k*1.3f + t*0.6f;
            float sx = cx + cosf(ang)*R*0.30f, sy = cy + sinf(ang)*R*0.25f;
            float tw = 0.5f+0.5f*sinf(t*4.0f + k*2.1f);
            DrawCircleV((Vector2){sx,sy}, R*0.025f, (Color){255,255,255,(uchar)(120*tw)});
        }
    }
    else if (id == 35) {
        // Ascendant — pure white-gold eyes + front half of the rainbow halo
        DrawCircleV((Vector2){cx-R*0.32f, cy-R*0.05f}, R*0.15f, (Color){255,250,220,255});
        DrawCircleV((Vector2){cx+R*0.32f, cy-R*0.05f}, R*0.15f, (Color){255,250,220,255});
        DrawCircleV((Vector2){cx-R*0.32f, cy-R*0.05f}, R*0.07f, (Color){255,215,90,255});
        DrawCircleV((Vector2){cx+R*0.32f, cy-R*0.05f}, R*0.07f, (Color){255,215,90,255});
        float hx, hy, rx, ry;  HaloGeom(cx, cy, R*1.15f, t, &hx, &hy, &rx, &ry);
        const int SLICES = 10;
        for (int s = 0; s < SLICES; s++) {
            float a0 = (PI * s)     / SLICES;
            float a1 = (PI * (s+1)) / SLICES;
            Color rc = ColorFromHSV(fmodf(t*50.0f + s*36.0f + 180.0f, 360.0f), 0.7f, 1.0f);
            DrawHaloArc(hx, hy, rx, ry, a0, a1, R*0.10f, (Color){rc.r,rc.g,rc.b,220});
        }
    }
}

// Constellation keeps its original composition regardless of shop category.
void DrawConstellation(float cx, float cy, float R, float t) {
    Color body = GetSkinColor(34);
    DrawLegendaryBack(34, cx, cy, R, t);
    DrawCircle((int)cx, (int)cy, (int)R, body);
    DrawCircle((int)cx - (int)(R / 6.0f), (int)cy - (int)(R / 4.0f), (int)(R / 2.0f),
               (Color){CLAMP255(body.r + 55), CLAMP255(body.g + 55), CLAMP255(body.b + 55), 90});
    DrawLegendaryFace(34, cx, cy, R, t);
}

// The six accepted design ideas use the same 32x32 compositions as
// Design_Ideas/player_skin_ideas.c. Scale the artwork for game and shop alike.
#define IDEA_C(r,g,b) ((Color){r,g,b,255})
#define IDEA_A(r,g,b,a) ((Color){r,g,b,a})
#define IDEA_B(x,y,w,h,c) DrawRectangle(x,y,w,h,c)
#define IDEA_O(x,y,r,c) DrawCircle(x,y,r,c)
#define IDEA_L(x,y,u,v,w,c) DrawLineEx((Vector2){x,y},(Vector2){u,v},w,c)

static void IdeaTriangle(float x1, float y1, float x2, float y2,
                         float x3, float y3, Color color) {
    float cross = (x2-x1)*(y3-y1) - (y2-y1)*(x3-x1);
    if (cross > 0) DrawTriangle((Vector2){x1,y1}, (Vector2){x3,y3}, (Vector2){x2,y2}, color);
    else DrawTriangle((Vector2){x1,y1}, (Vector2){x2,y2}, (Vector2){x3,y3}, color);
}

#define IDEA_T(x1,y1,x2,y2,x3,y3,c) IdeaTriangle(x1,y1,x2,y2,x3,y3,c)

static void IdeaDiamond(float x, float y, float r, Color color) {
    IDEA_T(x,y-r,x+r,y,x,y+r,color);
    IDEA_T(x,y-r,x,y+r,x-r,y,color);
}

static void IdeaBase(Color rim, Color face) {
    IDEA_O(16,17,13,rim);
    IDEA_O(16,16,10,face);
}

static void IdeaStar(int x, int y, int r, Color color) {
    IDEA_L(x-r,y,x+r,y,1,color);
    IDEA_L(x,y-r,x,y+r,1,color);
    IDEA_O(x,y,1,color);
}

void DrawAcceptedSkin(int id, float cx, float cy, float R, float t) {
    if (id < 43 || id > 48) return;
    float scale = R / 12.0f;
    float p = 0.5f + 0.5f*sinf(t*2.5f);
    rlPushMatrix();
    rlTranslatef(cx - 16.0f*scale, cy - 16.0f*scale, 0.0f);
    rlScalef(scale, scale, 1.0f);
    switch (id) {
        case 43: // Ember Fox
            IDEA_T(8,10,7,1,15,8,IDEA_C(232,91,57));
            IDEA_T(24,10,25,1,17,8,IDEA_C(255,155,69));
            IDEA_T(10,26,2,21,4,8,IDEA_C(255,113,62));
            IDEA_T(22,27,31,18,27,6,IDEA_C(255,191,76));
            IdeaBase(IDEA_C(170,72,55), IDEA_C(241,114,58));
            IDEA_T(10,19,22,19,16,27,IDEA_C(255,226,158));
            IDEA_O(11,14,2,IDEA_C(35,20,31));
            IDEA_O(21,14,2,IDEA_C(35,20,31));
            IdeaDiamond(16,20,2,IDEA_C(40,24,30));
            IDEA_O(29,6,1,IDEA_C(255,235,177));
            break;
        case 44: // Veil Dancer
            IDEA_T(9,8,2,29,17,24,IDEA_C(101,78,154));
            IDEA_T(23,8,30,29,15,24,IDEA_C(151,119,207));
            IDEA_O(16,14,12,IDEA_A(127,99,197,58));
            IDEA_T(9,9,23,9,21,24,IDEA_C(235,220,229));
            IDEA_T(9,9,21,24,11,24,IDEA_C(176,160,203));
            IDEA_L(8,7,23,7,2,IDEA_C(226,193,247));
            IDEA_O(12,15,2,IDEA_C(34,27,60));
            IDEA_O(20,15,2,IDEA_C(34,27,60));
            IDEA_L(16,19,19,22,1,IDEA_C(101,77,145));
            break;
        case 45: // Ossuary Knight
            IDEA_T(1,16,10,9,10,25,IDEA_C(121,117,125));
            IDEA_T(31,16,22,9,22,25,IDEA_C(189,179,162));
            IdeaBase(IDEA_C(112,108,112),IDEA_C(40,39,52));
            IDEA_T(7,9,16,4,25,9,IDEA_C(238,224,194));
            IDEA_B(8,9,16,13,IDEA_C(229,213,183));
            IDEA_B(10,14,5,3,IDEA_C(19,20,36));
            IDEA_B(18,14,5,3,IDEA_C(19,20,36));
            IDEA_B(12,21,8,5,IDEA_C(204,188,165));
            for (int i=0; i<3; i++) IDEA_B(13+i*2,22,1,3,IDEA_C(78,64,74));
            break;
        case 46: // Glass Seraph
            IDEA_T(12,15,2,3,7,24,IDEA_C(153,220,244));
            IDEA_T(20,15,30,3,25,24,IDEA_C(191,245,251));
            IDEA_T(10,18,3,26,5,10,IDEA_C(76,180,212));
            IDEA_T(22,18,29,26,27,10,IDEA_C(104,218,226));
            IdeaDiamond(16,16,11,IDEA_C(238,239,220));
            IdeaDiamond(16,16,8,IDEA_C(97,194,220));
            IdeaDiamond(16,16,5,IDEA_C(249,252,238));
            IDEA_B(11,15,4,2,IDEA_C(207,166,95));
            IDEA_B(18,15,4,2,IDEA_C(207,166,95));
            IdeaStar(16,2,3,IDEA_C(255,223,147));
            break;
        case 47: // Gilt Jackal
            IDEA_T(7,11,8,0,15,9,IDEA_C(236,186,91));
            IDEA_T(25,11,24,0,17,9,IDEA_C(255,217,116));
            IdeaBase(IDEA_C(119,75,54),IDEA_C(225,168,75));
            IDEA_T(6,9,16,5,16,27,IDEA_C(251,207,117));
            IDEA_T(26,9,16,5,16,27,IDEA_C(194,125,59));
            IDEA_T(11,19,21,19,16,29,IDEA_C(56,39,48));
            IDEA_B(9,14,5,2,IDEA_C(28,29,39));
            IDEA_B(18,14,5,2,IDEA_C(28,29,39));
            IdeaDiamond(16,21,2,IDEA_C(255,238,169));
            break;
        case 48: // Ember Marrow
            IDEA_O(16,16,15,IDEA_A(255,106,39,(unsigned char)(24+20*p)));
            IDEA_T(4,13,7,4,13,12,IDEA_C(223,214,189));
            IDEA_T(28,13,25,4,19,12,IDEA_C(244,231,198));
            IDEA_T(7,9,11,2,14,10,IDEA_C(255,220,128));
            IDEA_T(25,9,21,2,18,10,IDEA_C(255,220,128));
            IdeaBase(IDEA_C(139,131,117),IDEA_C(236,226,201));
            IDEA_T(6,12,16,8,16,25,IDEA_C(250,243,220));
            IDEA_T(26,12,16,8,16,25,IDEA_C(211,199,175));
            IDEA_B(8,13,7,6,IDEA_C(43,34,42));
            IDEA_B(17,13,7,6,IDEA_C(43,34,42));
            IDEA_O(11,16,2,IDEA_C(255,109,41));
            IDEA_O(21,16,2,IDEA_C(255,109,41));
            IDEA_O(11,16,1,IDEA_C(255,235,119));
            IDEA_O(21,16,1,IDEA_C(255,235,119));
            IDEA_T(13,19,19,19,16,23,IDEA_C(73,53,57));
            for (int i=0; i<4; i++) IDEA_B(13+i*2,22,1,3,IDEA_C(247,236,211));
            IDEA_L(8,12,11,14,1,IDEA_C(255,247,213));
            IDEA_L(24,12,21,14,1,IDEA_C(255,247,213));
            IDEA_L(15,9,14,12,1,IDEA_C(191,67,48));
            IDEA_L(14,12,16,14,1,IDEA_C(255,135,53));
            IDEA_L(22,20,25,23,1,IDEA_C(168,59,51));
            IdeaDiamond(16,10,2,IDEA_C(255,178,62));
            for (int i=0; i<4; i++) {
                float a=t*1.4f+i*PI/2.0f;
                IdeaStar((int)(16+cosf(a)*14),(int)(14+sinf(a)*12),1,IDEA_C(255,171,75));
            }
            break;
    }
    rlPopMatrix();
}

#undef IDEA_C
#undef IDEA_A
#undef IDEA_B
#undef IDEA_O
#undef IDEA_L
#undef IDEA_T

// Exact transparent 32x32 exports of the selected Player Skin Fixes.
void DrawSelectedFixSkin(int id, float cx, float cy, float R) {
    static const char *const paths[3] = {
        "assets/skins/split_reflection.png",
        "assets/skins/cinder_sovereign.png",
        "assets/skins/filigree_crown.png"
    };
    static Texture2D textures[3];
    static bool loaded[3];
    int index = id - 49;
    if (index < 0 || index >= 3) return;
    if (!loaded[index]) {
        const char *path = paths[index];
        char fullPath[512];
        if (!FileExists(path)) {
            snprintf(fullPath, sizeof(fullPath), "%s%s", GetApplicationDirectory(), path);
            path = fullPath;
        }
        textures[index] = LoadTexture(path);
        if (textures[index].id > 0)
            SetTextureFilter(textures[index], TEXTURE_FILTER_POINT);
        loaded[index] = true;
    }
    if (textures[index].id > 0) {
        float side = 32.0f * R / 12.0f * 1.12f;
        DrawTexturePro(textures[index],
                       (Rectangle){0,0,(float)textures[index].width,(float)textures[index].height},
                       (Rectangle){cx-side*0.5f,cy-side*0.5f,side,side},
                       (Vector2){0,0},0.0f,WHITE);
    } else {
        DrawCircleV((Vector2){cx,cy},R,GetSkinColor(id));
    }
}
