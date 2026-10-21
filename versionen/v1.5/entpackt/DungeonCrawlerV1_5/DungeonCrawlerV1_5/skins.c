// skins.c — skin colours plus the rare/epic/skull procedural face & back art.
#include "skins.h"
#include "constants.h"   // CLAMP255
#include "types.h"       // uchar
#include <math.h>

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
        case 28: { // Skull — cream white
            return (Color){228, 222, 205, 255};
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
    }
    static const Color SKIN_TABLE[22] = {
        // Common (0-13)
        {  90,220,120,255 }, { 255, 80, 80,255 }, { 255,160, 40,255 },
        { 255,220, 40,255 }, {  60,200, 60,255 }, {  40,200,220,255 },
        {  60,120,255,255 }, { 160, 60,255,255 }, { 220, 60,200,255 },
        { 255,255,255,255 },
        { 150, 90, 40,255 }, {  30, 28, 35,255 }, { 255,120,195,255 },
        { 155,155,162,255 },
        // Uncommon (14-21) — intensified to match their names (Void unchanged)
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
// Fills the skull silhouette (cranium + cheekbones + tapering jaw + chin) from a
// hand-tuned half-contour mirrored about the vertical axis.  Drawn as a triangle
// fan from the centre (both windings, so back-face culling never drops a slice).
static void DrawSkullShape(float cx, float cy, float R, Color col) {
    // half outline (x>=0), top → chin, in units of R
    static const float CONT[][2] = {
        { 0.00f, -1.05f }, { 0.42f, -0.99f }, { 0.72f, -0.82f },
        { 0.91f, -0.55f }, { 1.00f, -0.22f }, { 1.01f,  0.10f },  // temple / cheekbone
        { 0.90f,  0.34f }, { 0.72f,  0.52f },                     // cheek taper
        { 0.64f,  0.72f }, { 0.60f,  0.95f }, { 0.50f,  1.12f },
        { 0.30f,  1.21f }, { 0.00f,  1.23f },                     // chin
    };
    int n = (int)(sizeof(CONT) / sizeof(CONT[0]));
    // closed control polygon: right side top→chin, then mirrored left side
    Vector2 ctrl[40];
    int m = 0;
    for (int i = 0;   i < n;  i++) ctrl[m++] = (Vector2){ cx + CONT[i][0]*R, cy + CONT[i][1]*R };
    for (int i = n-2; i >= 1; i--) ctrl[m++] = (Vector2){ cx - CONT[i][0]*R, cy + CONT[i][1]*R };

    // Catmull-Rom smoothing → glatte geschlossene Silhouette, gefüllt als Fan
    // (jedes Dreieck in beiden Windungen, damit Culling nie eine Scheibe verwirft)
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

// Full skull skin: silhouette + soft shading, big angled eye sockets, a heart
// shaped nasal cavity and a real tooth row.  Replaces the plain body circle.
void DrawSkull(float cx, float cy, float R, float t) {
    (void)t;
    Color bone   = (Color){231, 226, 212, 255};
    Color boneLo = (Color){193, 187, 172, 255};   // shadow tone
    Color boneHi = (Color){247, 244, 235, 255};   // highlight
    Color socket = (Color){ 13, 11, 13, 255 };
    Color dark   = (Color){ 24, 20, 21, 255 };

    // 1) silhouette: shadow rim slightly offset down, then the bright skull on top
    DrawSkullShape(cx, cy + R*0.03f, R*1.03f, boneLo);
    DrawSkullShape(cx, cy, R, bone);

    // 2) soft top sheen (layered translucent ellipses → pseudo gradient)
    for (int i = 0; i < 4; i++) {
        float k = 1.0f - i*0.18f;
        DrawEllipseRotF(cx - R*0.10f, cy - R*0.46f, R*0.58f*k, R*0.44f*k, 0.0f,
                        (Color){boneHi.r, boneHi.g, boneHi.b, 42});
    }
    // 3) faint shadow under the cheekbones / above the jaw
    DrawEllipseRotF(cx, cy + R*0.58f, R*0.52f, R*0.26f, 0.0f, (Color){boneLo.r,boneLo.g,boneLo.b,110});

    // 4) eye sockets — large ovals angled outward-up (mirrored), soft recessed rim
    DrawEllipseRotF(cx - R*0.44f, cy - R*0.07f, R*0.48f, R*0.42f,  0.32f, (Color){150,144,132,120});
    DrawEllipseRotF(cx + R*0.44f, cy - R*0.07f, R*0.48f, R*0.42f, -0.32f, (Color){150,144,132,120});
    DrawEllipseRotF(cx - R*0.44f, cy - R*0.08f, R*0.42f, R*0.36f,  0.32f, socket);
    DrawEllipseRotF(cx + R*0.44f, cy - R*0.08f, R*0.42f, R*0.36f, -0.32f, socket);
    // tiny depth glints low in the sockets
    DrawCircleV((Vector2){cx - R*0.34f, cy + R*0.10f}, R*0.05f, (Color){55,51,53,150});
    DrawCircleV((Vector2){cx + R*0.34f, cy + R*0.10f}, R*0.05f, (Color){55,51,53,150});

    // 5) nasal cavity — inverted-heart (triangle + two lobes)
    DrawTriangle((Vector2){cx,            cy + R*0.44f},
                 (Vector2){cx - R*0.15f,  cy + R*0.20f},
                 (Vector2){cx + R*0.15f,  cy + R*0.20f}, dark);
    DrawTriangle((Vector2){cx,            cy + R*0.44f},
                 (Vector2){cx + R*0.15f,  cy + R*0.20f},
                 (Vector2){cx - R*0.15f,  cy + R*0.20f}, dark);
    DrawCircleV((Vector2){cx - R*0.08f, cy + R*0.22f}, R*0.075f, dark);
    DrawCircleV((Vector2){cx + R*0.08f, cy + R*0.22f}, R*0.075f, dark);

    // 6) teeth — dark mouth backdrop with bright teeth + thin gaps
    float mx = cx - R*0.46f, mouthW = R*0.92f;
    float myT = cy + R*0.62f, myB = cy + R*1.00f;
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
    }
}

// Front facial details for rare skins (22-26), drawn over the body circle.
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
    }
}

// ─── LEGENDARY SKIN FACES (achievement-locked, shop preview + in-game) ────────
// Behind-body effects for legendary skins (34-35): auras, starfields, halo rings.
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
