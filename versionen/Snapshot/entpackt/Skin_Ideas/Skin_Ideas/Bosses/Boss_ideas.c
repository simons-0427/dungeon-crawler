// Boss_ideas.c — interactive picker for the 10 Chaos Warden visual concepts.
// Fully independent of the main game (raylib only).
//
// Build (from this folder, Git Bash / MSYS):
//   export PATH="/c/raylib/w64devkit/bin:$PATH"
//   gcc -O2 -o Boss_ideas.exe Boss_ideas.c -I"/c/raylib/raylib/src" -L"/c/raylib/raylib/src" -lraylib -lopengl32 -lgdi32 -lwinmm
//
// Or Windows cmd / PowerShell (w64devkit on PATH):
//   gcc -O2 -o Boss_ideas.exe Boss_ideas.c -IC:/raylib/raylib/src -LC:/raylib/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm
//
// Run from Skin_Ideas/ so bad_boss_ideas.txt / boss_picks.txt land next to the exe.
//
// Controls:
//   Click preview .............. REJECT (or restore if already rejected)
//   Hover + 1..5 ............... assign to L10 / L20 / L30 / L40 / L50
//   Hover + 0 .................. clear tier assignment
//   Space / S .................. cycle demo state (Stalk/Windup/Dash/Fire/Tired)
//   Mouse wheel / Up/Down ...... scroll
//   Esc ........................ quit

#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

typedef unsigned char uchar;
#define CLAMP255(v) ((v) > 255 ? 255 : ((v) < 0 ? 0 : (v)))
#define NBOSS 11

// Demo combat state shown on every preview (shared cycle).
enum {
    ST_STALK = 0,
    ST_WINDUP,
    ST_DASH,
    ST_FIRE,
    ST_TIRED,
    ST_COUNT
};
static const char *ST_NAME[ST_COUNT] = {
    "STALK", "WINDUP", "DASH", "FIRE", "TIRED"
};
static int demoState = ST_STALK;

// ─── helpers ────────────────────────────────────────────────────────────────
static float Rnd(int i) {
    float x = sinf(i * 127.1f + 311.7f) * 43758.5453f;
    return x - floorf(x);
}

static void Star4(float cx, float cy, float r, Color c) {
    DrawLineEx((Vector2){cx - r, cy}, (Vector2){cx + r, cy}, r * 0.35f, c);
    DrawLineEx((Vector2){cx, cy - r}, (Vector2){cx, cy + r}, r * 0.35f, c);
    DrawCircleV((Vector2){cx, cy}, r * 0.30f, c);
}

static void DrawBrow(float cx, float cy, float R, Color c) {
    DrawLineEx((Vector2){cx - R * 0.50f, cy - R * 0.42f},
               (Vector2){cx - R * 0.10f, cy - R * 0.28f}, R * 0.07f, c);
    DrawLineEx((Vector2){cx + R * 0.10f, cy - R * 0.28f},
               (Vector2){cx + R * 0.50f, cy - R * 0.42f}, R * 0.07f, c);
}

// Shared aura + inv-flash style used by several skins.
static void BossAura(float cx, float cy, float R, Color aura, float t, int st) {
    float pp = 0.5f + 0.5f * sinf(t * 3.0f);
    float ar = R * 1.20f + R * 0.15f * pp;
    if (st == ST_WINDUP) ar = R * 1.35f + R * 0.20f * (0.5f + 0.5f * sinf(t * 12.0f));
    if (st == ST_FIRE)   ar = R * 1.45f + R * 0.18f * pp;
    DrawCircle((int)cx, (int)cy, (int)ar, (Color){aura.r, aura.g, aura.b, (uchar)(26 + 18 * pp)});
}

static Color StateEye(Color base, int st) {
    if (st == ST_DASH)  return (Color){255, 255, 255, 255};
    if (st == ST_TIRED) return (Color){170, 140, 60, 255};
    if (st == ST_FIRE || st == ST_WINDUP) {
        // windup keeps base for most skins; fire overrides below where needed
        if (st == ST_FIRE) return (Color){255, 140, 30, 255};
    }
    return base;
}

// Mini aim line under the preview when windup/dash/fire.
static void DrawAimHint(float cx, float cy, float R, int st, float t, Color col) {
    if (st != ST_WINDUP && st != ST_DASH && st != ST_FIRE) return;
    float wp = 0.5f + 0.5f * sinf(t * 14.0f);
    Vector2 a = {cx, cy};
    Vector2 b = {cx + R * 2.4f, cy + R * 1.1f};
    if (st == ST_WINDUP) {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawLineEx(a, b, R * 0.22f, (Color){255, 25, 35, (uchar)(140 + 60 * wp)});
        EndBlendMode();
    } else if (st == ST_DASH) {
        for (int s = 1; s <= 3; s++)
            DrawCircle((int)(cx - s * R * 0.35f), (int)(cy - s * R * 0.15f),
                       (int)(R * (0.85f - s * 0.15f)),
                       (Color){col.r, col.g, col.b, (uchar)(90 - s * 22)});
    } else {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawLineEx(a, b, R * 0.55f, (Color){255, 80, 8, 70});
        DrawLineEx(a, b, R * 0.28f, (Color){255, 180, 40, 160});
        DrawLineEx(a, b, R * 0.12f, (Color){255, 255, 220, 220});
        EndBlendMode();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 0 — Chaos Warden (current in-game look)
// ═══════════════════════════════════════════════════════════════════════════
static void Boss_ChaosWarden(float cx, float cy, float R, float t, int st) {
    Color CBOSS = (Color){170, 40, 200, 255};
    Color body  = CBOSS;
    if (st == ST_TIRED) body = (Color){110, 80, 130, 255};
    DrawAimHint(cx, cy, R, st, t, CBOSS);
    BossAura(cx, cy, R, (st == ST_FIRE) ? (Color){255, 140, 30, 255} : CBOSS, t, st);
    if (st == ST_WINDUP) cx += sinf(t * 45.0f) * R * 0.06f;

    DrawCircle((int)cx, (int)cy, (int)R, body);
    DrawCircle((int)(cx - R * 0.21f), (int)(cy - R * 0.28f), (int)(R * 0.50f),
               (Color){CLAMP255(body.r + 50), CLAMP255(body.g + 50), CLAMP255(body.b + 50), 90});

    Color horn = (Color){235, 220, 90, 255};
    DrawTriangle((Vector2){cx - R * 0.64f, cy - R * 0.57f},
                 (Vector2){cx - R * 0.28f, cy - R * 0.71f},
                 (Vector2){cx - R * 0.71f, cy - R * 1.21f}, horn);
    DrawTriangle((Vector2){cx + R * 0.28f, cy - R * 0.71f},
                 (Vector2){cx + R * 0.64f, cy - R * 0.57f},
                 (Vector2){cx + R * 0.71f, cy - R * 1.21f}, horn);

    Color eye = StateEye((Color){255, 200, 0, 255}, st);
    if (st == ST_FIRE) eye = (Color){255, 140, 30, 255};
    DrawCircle((int)(cx - R * 0.28f), (int)(cy - R * 0.14f), (int)(R * 0.18f), eye);
    DrawCircle((int)(cx + R * 0.28f), (int)(cy - R * 0.14f), (int)(R * 0.18f), eye);
    DrawCircle((int)(cx - R * 0.28f), (int)(cy - R * 0.14f), (int)(R * 0.07f), (Color){30, 0, 10, 255});
    DrawCircle((int)(cx + R * 0.28f), (int)(cy - R * 0.14f), (int)(R * 0.07f), (Color){30, 0, 10, 255});
    DrawBrow(cx, cy, R, (Color){40, 10, 50, 255});
}

// ═══════════════════════════════════════════════════════════════════════════
// 1 — Obsidian Brute  (L30+ rework: magma titan)
// ═══════════════════════════════════════════════════════════════════════════
static void Boss_ObsidianBrute(float cx, float cy, float R, float t, int st) {
    Color body = (st == ST_TIRED) ? (Color){36, 40, 48, 255} : (Color){18, 14, 26, 255};
    Color lava = (st == ST_TIRED) ? (Color){70, 85, 100, 200}
               : (st == ST_FIRE)  ? (Color){255, 200, 60, 255}
               : (st == ST_WINDUP)? (Color){255, 90, 20, 255}
                                  : (Color){255, 110, 25, 255};
    float pulse = 0.5f + 0.5f * sinf(t * 3.2f);
    float fury  = (st == ST_WINDUP || st == ST_FIRE) ? 1.35f : 1.0f;
    DrawAimHint(cx, cy, R, st, t, lava);

    // heat halo + shock rings
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircle((int)cx, (int)cy, (int)(R * (1.55f + 0.15f * pulse) * fury),
               (Color){255, 80, 10, (uchar)(28 + 20 * pulse)});
    if (st == ST_WINDUP || st == ST_DASH) {
        float ring = fmodf(t * 2.5f, 1.0f);
        DrawCircleLines((int)cx, (int)cy, (int)(R * (1.1f + ring * 0.9f)),
                        (Color){255, 140, 40, (uchar)(160 * (1.0f - ring))});
    }
    EndBlendMode();

    // orbiting rock shards
    for (int k = 0; k < 7; k++) {
        float a = t * 1.1f + k * (2.0f * PI / 7.0f);
        float dist = R * (1.55f + 0.12f * sinf(t * 2.0f + k));
        float sx = cx + cosf(a) * dist, sy = cy + sinf(a) * dist * 0.75f;
        DrawTriangle((Vector2){sx, sy - R * 0.18f},
                     (Vector2){sx - R * 0.14f, sy + R * 0.12f},
                     (Vector2){sx + R * 0.14f, sy + R * 0.12f},
                     (Color){45, 40, 55, 240});
        DrawCircleV((Vector2){sx, sy}, R * 0.05f, lava);
    }

    // massive shoulders + bulk body
    DrawCircle((int)(cx - R * 1.05f), (int)(cy + R * 0.20f), (int)(R * 0.72f), body);
    DrawCircle((int)(cx + R * 1.05f), (int)(cy + R * 0.20f), (int)(R * 0.72f), body);
    DrawCircle((int)cx, (int)(cy + R * 0.08f), (int)(R * 1.12f), body);

    // molten core under cracked shell
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircle((int)cx, (int)cy, (int)(R * (0.55f + 0.08f * pulse)),
               (Color){lava.r, lava.g, lava.b, (uchar)(90 + 50 * pulse)});
    EndBlendMode();
    DrawCircle((int)cx, (int)cy, (int)(R * 0.95f), (Color){body.r, body.g, body.b, 210});

    // thick lava fissures (branching)
    for (int k = 0; k < 8; k++) {
        float a0 = k * 0.78f + t * 0.15f;
        float a1 = a0 + 0.55f + 0.2f * sinf(t + k);
        float thick = R * (0.07f + 0.05f * pulse) * fury;
        DrawLineEx((Vector2){cx + cosf(a0) * R * 0.12f, cy + sinf(a0) * R * 0.08f},
                   (Vector2){cx + cosf(a1) * R * 0.95f, cy + sinf(a1) * R * 0.95f},
                   thick, lava);
        // side branch
        float mid = 0.55f;
        float mx = cx + cosf(a0) * R * 0.12f * (1.0f - mid) + cosf(a1) * R * 0.95f * mid;
        float my = cy + sinf(a0) * R * 0.08f * (1.0f - mid) + sinf(a1) * R * 0.95f * mid;
        DrawLineEx((Vector2){mx, my},
                   (Vector2){mx + cosf(a1 + 0.9f) * R * 0.35f, my + sinf(a1 + 0.9f) * R * 0.35f},
                   thick * 0.6f, lava);
    }

    // magma drips
    if (st != ST_TIRED) {
        for (int k = 0; k < 5; k++) {
            float p = fmodf(t * 0.7f + Rnd(k + 11), 1.0f);
            float dx = cx - R * 0.6f + Rnd(k) * R * 1.2f;
            DrawCircleV((Vector2){dx, cy + R * 0.5f + p * R * 0.9f},
                        R * (0.07f - p * 0.04f), (Color){255, 140, 30, (uchar)(220 * (1.0f - p))});
        }
    }

    // crown of jagged basalt spikes
    for (int k = -3; k <= 3; k++) {
        float hx = cx + k * R * 0.22f;
        float hlen = R * (0.55f + 0.35f * (1.0f - fabsf((float)k) / 3.0f) + 0.1f * sinf(t * 3.0f + k));
        DrawTriangle((Vector2){hx - R * 0.10f, cy - R * 0.75f},
                     (Vector2){hx + R * 0.10f, cy - R * 0.75f},
                     (Vector2){hx, cy - R * 0.75f - hlen},
                     (k == 0) ? lava : (Color){55, 50, 65, 255});
    }

    // glowing crater eyes
    Color eye = (st == ST_TIRED) ? (Color){70, 80, 90, 255} : lava;
    for (int side = -1; side <= 1; side += 2) {
        float ex = cx + side * R * 0.32f, ey = cy - R * 0.08f;
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle((int)ex, (int)ey, (int)(R * 0.28f), (Color){lava.r, lava.g, lava.b, 70});
        EndBlendMode();
        DrawCircle((int)ex, (int)ey, (int)(R * 0.18f), eye);
        DrawCircle((int)ex, (int)ey, (int)(R * 0.07f), (Color){255, 255, 200, 255});
    }

    // roaring maw when aggressive
    if (st == ST_FIRE || st == ST_WINDUP || st == ST_DASH) {
        float open = (st == ST_FIRE) ? 0.35f : 0.18f;
        DrawTriangle((Vector2){cx - R * 0.40f, cy + R * 0.20f},
                     (Vector2){cx + R * 0.40f, cy + R * 0.20f},
                     (Vector2){cx, cy + R * (0.55f + open)}, lava);
        DrawTriangle((Vector2){cx - R * 0.25f, cy + R * 0.25f},
                     (Vector2){cx + R * 0.25f, cy + R * 0.25f},
                     (Vector2){cx, cy + R * (0.40f + open)}, (Color){255, 240, 120, 230});
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 2 — Crimson Herald
// ═══════════════════════════════════════════════════════════════════════════
static void Boss_CrimsonHerald(float cx, float cy, float R, float t, int st) {
    Color body = (st == ST_TIRED) ? (Color){110, 55, 48, 255} : (Color){160, 28, 48, 255};
    Color gold = (Color){220, 180, 70, 255};
    DrawAimHint(cx, cy, R, st, t, body);
    BossAura(cx, cy, R, body, t, st);

    // filled gold crown ring (rotating highlight)
    float spin = (st == ST_WINDUP) ? t * 4.0f : t * 0.8f;
    float deg = spin * (180.0f / PI);
    DrawRing((Vector2){cx, cy}, R * 1.12f, R * 1.32f, 0.0f, 360.0f, 48, gold);
    // brighter chasing arc so the ring still reads as spinning
    DrawRing((Vector2){cx, cy}, R * 1.10f, R * 1.34f, deg, deg + 70.0f, 24,
             (Color){255, 235, 140, 220});

    DrawCircle((int)cx, (int)cy, (int)R, body);
    DrawCircle((int)cx, (int)cy, (int)(R * 0.55f), (Color){90, 10, 25, 255});

    // helm crest
    DrawTriangle((Vector2){cx - R * 0.12f, cy - R * 0.90f},
                 (Vector2){cx + R * 0.12f, cy - R * 0.90f},
                 (Vector2){cx, cy - R * 1.45f}, gold);

    // swept horns
    DrawTriangle((Vector2){cx - R * 0.70f, cy - R * 0.45f},
                 (Vector2){cx - R * 0.30f, cy - R * 0.65f},
                 (Vector2){cx - R * 1.05f, cy - R * 1.15f}, (Color){120, 15, 30, 255});
    DrawTriangle((Vector2){cx + R * 0.30f, cy - R * 0.65f},
                 (Vector2){cx + R * 0.70f, cy - R * 0.45f},
                 (Vector2){cx + R * 1.05f, cy - R * 1.15f}, (Color){120, 15, 30, 255});

    Color eye = StateEye((Color){255, 255, 255, 255}, st);
    if (st == ST_FIRE) eye = (Color){255, 160, 50, 255};
    DrawRectangle((int)(cx - R * 0.42f), (int)(cy - R * 0.18f), (int)(R * 0.22f), (int)(R * 0.10f), eye);
    DrawRectangle((int)(cx + R * 0.20f), (int)(cy - R * 0.18f), (int)(R * 0.22f), (int)(R * 0.10f), eye);
}

// ═══════════════════════════════════════════════════════════════════════════
// 3 — Ashen Wraith  (L30+ rework: death cyclone / multi-ghost)
// ═══════════════════════════════════════════════════════════════════════════
static void Boss_AshenWraith(float cx, float cy, float R, float t, int st) {
    float alphaMul = (st == ST_TIRED) ? 0.40f : 1.0f;
    Color core = (Color){28, 18, 40, (uchar)(230 * alphaMul)};
    Color smoke = (Color){100, 90, 130, (uchar)(150 * alphaMul)};
    Color soul = (st == ST_FIRE) ? (Color){255, 150, 60, 255}
               : (Color){120, 255, 255, 255};
    DrawAimHint(cx, cy, R, st, t, soul);

    // dash: stacked ghost afterimages
    if (st == ST_DASH || st == ST_WINDUP) {
        for (int g = 3; g >= 1; g--) {
            float gx = cx - g * R * 0.55f, gy = cy - g * R * 0.12f;
            DrawCircle((int)gx, (int)gy, (int)(R * (0.85f - g * 0.08f)),
                       (Color){smoke.r, smoke.g, smoke.b, (uchar)(55 * alphaMul)});
            DrawCircle((int)(gx - R * 0.18f), (int)(gy - R * 0.05f), (int)(R * 0.08f),
                       (Color){soul.r, soul.g, soul.b, (uchar)(100 * alphaMul)});
            DrawCircle((int)(gx + R * 0.18f), (int)(gy - R * 0.05f), (int)(R * 0.08f),
                       (Color){soul.r, soul.g, soul.b, (uchar)(100 * alphaMul)});
        }
    }

    // outer soul vortex rings
    BeginBlendMode(BLEND_ADDITIVE);
    for (int ring = 0; ring < 3; ring++) {
        float spin = t * (1.4f + ring * 0.6f) * ((ring % 2) ? -1.0f : 1.0f);
        float rr = R * (1.25f + ring * 0.28f);
        for (int k = 0; k < 14; k++) {
            float a = spin + k * (2.0f * PI / 14.0f);
            DrawCircleV((Vector2){cx + cosf(a) * rr, cy + sinf(a) * rr * 0.70f},
                        R * 0.06f, (Color){soul.r, soul.g, soul.b, (uchar)(40 + 20 * ring)});
        }
    }
    EndBlendMode();

    // dense multi-layer cloud body
    for (int k = 0; k < 10; k++) {
        float ox = sinf(t * 1.6f + k * 0.9f) * R * 0.28f;
        float oy = cosf(t * 1.3f + k * 0.7f) * R * 0.20f;
        float rr = R * (0.45f + (k % 4) * 0.14f);
        DrawCircle((int)(cx + ox), (int)(cy + oy), (int)rr,
                   (Color){smoke.r, smoke.g, smoke.b, (uchar)((35 + 14 * k) * alphaMul)});
    }
    DrawCircle((int)cx, (int)cy, (int)(R * 0.72f), core);

    // skull face silhouette in the core
    DrawCircle((int)cx, (int)(cy - R * 0.05f), (int)(R * 0.42f),
               (Color){55, 48, 70, (uchar)(200 * alphaMul)});
    DrawCircle((int)(cx - R * 0.14f), (int)(cy - R * 0.08f), (int)(R * 0.12f), core);
    DrawCircle((int)(cx + R * 0.14f), (int)(cy - R * 0.08f), (int)(R * 0.12f), core);
    DrawTriangle((Vector2){cx - R * 0.10f, cy + R * 0.12f},
                 (Vector2){cx + R * 0.10f, cy + R * 0.12f},
                 (Vector2){cx, cy + R * 0.28f}, core);

    // ash storm + soul wisps
    for (int k = 0; k < 18; k++) {
        float p = fmodf(t * (0.5f + (k % 3) * 0.15f) + Rnd(k), 1.0f);
        float a = Rnd(k + 7) * 2.0f * PI + t * 0.8f;
        float dist = R * (0.4f + p * 1.5f);
        bool soulP = (k % 4 == 0);
        DrawCircleV((Vector2){cx + cosf(a) * dist, cy + sinf(a) * dist * 0.75f - p * R * 0.4f},
                    R * (soulP ? 0.06f : 0.035f),
                    soulP ? (Color){soul.r, soul.g, soul.b, (uchar)(200 * sinf(p * PI) * alphaMul)}
                          : (Color){200, 195, 215, (uchar)(170 * sinf(p * PI) * alphaMul)});
    }

    // towering smoke horns that fork
    for (int side = -1; side <= 1; side += 2) {
        float bx = cx + side * R * 0.30f, by = cy - R * 0.50f;
        for (int i = 0; i < 7; i++) {
            float u = i / 6.0f;
            float hx = bx + side * (u * R * 0.55f + sinf(t * 2.5f + i) * R * 0.14f);
            float hy = by - u * R * 1.25f;
            DrawCircleV((Vector2){hx, hy}, R * (0.22f - u * 0.14f),
                        (Color){130, 120, 160, (uchar)(170 * alphaMul)});
            if (i == 4) {
                // fork tine
                DrawCircleV((Vector2){hx - side * R * 0.25f, hy - R * 0.20f},
                            R * 0.10f, (Color){130, 120, 160, (uchar)(150 * alphaMul)});
            }
        }
    }

    // piercing cyan eyes with glow
    Color eye = (st == ST_TIRED) ? (Color){70, 90, 100, 180} : soul;
    for (int side = -1; side <= 1; side += 2) {
        float ex = cx + side * R * 0.22f, ey = cy - R * 0.10f;
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle((int)ex, (int)ey, (int)(R * 0.22f), (Color){eye.r, eye.g, eye.b, 80});
        EndBlendMode();
        DrawCircle((int)ex, (int)ey, (int)(R * 0.13f), eye);
        DrawCircle((int)ex, (int)ey, (int)(R * 0.05f), (Color){255, 255, 255, 240});
        // long tear trails
        DrawLineEx((Vector2){ex, ey + R * 0.12f},
                   (Vector2){ex + side * R * 0.05f, ey + R * 0.55f},
                   R * 0.04f, (Color){eye.r, eye.g, eye.b, (uchar)(140 * alphaMul)});
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 4 — Void Maw  (L30+ rework: black-hole apex predator)
// ═══════════════════════════════════════════════════════════════════════════
static void Boss_VoidMaw(float cx, float cy, float R, float t, int st) {
    Color body = (Color){6, 4, 14, 255};
    Color ring = (st == ST_TIRED) ? (Color){90, 40, 120, 160}
               : (st == ST_FIRE)  ? (Color){255, 180, 255, 255}
                                  : (Color){230, 40, 255, 255};
    Color ring2 = (Color){40, 180, 255, 200};
    float spin = (st == ST_WINDUP) ? t * 8.0f : (st == ST_DASH) ? t * 5.0f : t * 2.0f;
    float stretch = (st == ST_DASH) ? 1.55f : 1.0f;
    float squash  = (st == ST_DASH) ? 0.70f : 1.0f;
    DrawAimHint(cx, cy, R, st, t, ring);

    // gravitational lensing glow (outer dark rim)
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircle((int)cx, (int)cy, (int)(R * 1.85f), (Color){80, 0, 120, 35});
    DrawCircle((int)cx, (int)cy, (int)(R * 1.55f), (Color){ring.r, ring.g, ring.b, 28});

    // triple accretion discs (counter-rotating)
    for (int disc = 0; disc < 3; disc++) {
        float dir = (disc % 2) ? -1.0f : 1.0f;
        float rr  = R * (1.15f + disc * 0.28f);
        Color rc  = (disc == 1) ? ring2 : ring;
        int segs = 28 + disc * 4;
        for (int k = 0; k < segs; k++) {
            float a = dir * spin * (1.0f + disc * 0.3f) + k * (2.0f * PI / segs);
            float px = cx + cosf(a) * rr * stretch;
            float py = cy + sinf(a) * rr * 0.55f * squash;
            float sz = R * (0.05f + 0.04f * (k % 3 == 0));
            DrawCircleV((Vector2){px, py}, sz,
                        (Color){rc.r, rc.g, rc.b, (uchar)(70 + 30 * (k % 3))});
        }
    }
    // photon-ring flash
    float flash = 0.5f + 0.5f * sinf(t * 6.0f);
    DrawCircleLines((int)cx, (int)cy, (int)(R * 1.05f),
                    (Color){255, 200, 255, (uchar)(80 + 100 * flash)});
    EndBlendMode();

    // event horizon body
    DrawCircle((int)cx, (int)cy, (int)(R * 1.05f), body);
    DrawCircle((int)cx, (int)cy, (int)(R * 0.70f), (Color){0, 0, 0, 255});

    // ring of fangs around the whole disc
    float jawOpen = (st == ST_FIRE) ? 0.22f : (st == ST_WINDUP) ? 0.12f : 0.04f;
    for (int k = 0; k < 12; k++) {
        float a = -PI * 0.15f + k * (PI * 1.3f / 11.0f) + PI * 0.35f;
        float tx = cx + cosf(a) * R * (0.85f + jawOpen);
        float ty = cy + sinf(a) * R * (0.85f + jawOpen);
        float nx = cosf(a), ny = sinf(a);
        DrawTriangle((Vector2){tx - ny * R * 0.10f, ty + nx * R * 0.10f},
                     (Vector2){tx + ny * R * 0.10f, ty - nx * R * 0.10f},
                     (Vector2){tx + nx * R * (0.28f + jawOpen), ty + ny * R * (0.28f + jawOpen)},
                     (Color){220, 210, 240, 240});
    }

    // vertical void-eye (blazing)
    Color eye = (st == ST_TIRED) ? (Color){90, 30, 110, 200}
              : (st == ST_FIRE)  ? (Color){255, 220, 255, 255}
                                 : (Color){255, 40, 200, 255};
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircle((int)cx, (int)(cy - R * 0.05f), (int)(R * 0.35f),
               (Color){eye.r, eye.g, eye.b, 70});
    EndBlendMode();
    DrawRectangle((int)(cx - R * 0.08f), (int)(cy - R * 0.55f),
                  (int)(R * 0.16f), (int)(R * 0.75f), eye);
    DrawCircle((int)cx, (int)(cy - R * 0.08f), (int)(R * 0.14f), (Color){255, 255, 255, 245});
    DrawCircle((int)cx, (int)(cy - R * 0.08f), (int)(R * 0.06f), (Color){20, 0, 30, 255});

    // matter streams spiraling inward
    for (int k = 0; k < 14; k++) {
        float p = fmodf(t * 0.9f + Rnd(k + 20), 1.0f);
        float a = Rnd(k + 40) * 2.0f * PI + t * 2.5f * (1.0f - p);
        float dist = R * (1.9f - p * 1.55f);
        Color pc = (k % 2) ? ring : ring2;
        DrawCircleV((Vector2){cx + cosf(a) * dist * stretch, cy + sinf(a) * dist * 0.55f * squash},
                    R * (0.06f * (1.0f - p * 0.5f)),
                    (Color){pc.r, pc.g, pc.b, (uchar)(220 * (1.0f - p * 0.3f))});
    }

    // reality tears (windup/fire)
    if (st == ST_WINDUP || st == ST_FIRE) {
        for (int k = 0; k < 4; k++) {
            float a = t * 3.0f + k * 1.4f;
            DrawLineEx((Vector2){cx + cosf(a) * R * 0.3f, cy + sinf(a) * R * 0.3f},
                       (Vector2){cx + cosf(a) * R * 1.7f, cy + sinf(a) * R * 1.1f},
                       R * 0.03f, (Color){255, 100, 255, 120});
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 5 — Gilded Tyrant  (L30+ rework: oppressive god-king)
// ═══════════════════════════════════════════════════════════════════════════
static void Boss_GildedTyrant(float cx, float cy, float R, float t, int st) {
    Color body = (st == ST_TIRED) ? (Color){80, 45, 100, 255} : (Color){130, 20, 170, 255};
    Color gold = (st == ST_TIRED) ? (Color){130, 105, 45, 255} : (Color){255, 210, 70, 255};
    Color goldHot = (Color){255, 245, 160, 255};
    Color gem  = (st == ST_WINDUP) ? (Color){255, 50, 70, 255}
               : (st == ST_FIRE)   ? (Color){255, 240, 80, 255}
               : (st == ST_TIRED)  ? (Color){50, 90, 110, 180}
                                   : (Color){30, 255, 220, 255};
    float pulse = 0.5f + 0.5f * sinf(t * 3.0f);
    DrawAimHint(cx, cy, R, st, t, gold);

    // radiant halo spokes
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircle((int)cx, (int)cy, (int)(R * 1.70f), (Color){gold.r, gold.g, gold.b, (uchar)(22 + 14 * pulse)});
    for (int k = 0; k < 12; k++) {
        float a = k * (2.0f * PI / 12.0f) + t * 0.35f;
        float len = R * (1.35f + 0.25f * pulse);
        DrawLineEx((Vector2){cx + cosf(a) * R * 1.05f, cy + sinf(a) * R * 1.05f},
                   (Vector2){cx + cosf(a) * len, cy + sinf(a) * len},
                   R * 0.06f, (Color){gold.r, gold.g, gold.b, 90});
    }
    EndBlendMode();

    // dual floating scepters
    for (int side = -1; side <= 1; side += 2) {
        float sx = cx + side * R * 1.45f;
        float sy = cy + sinf(t * 2.0f + side) * R * 0.12f;
        float ang = side * 0.35f + sinf(t * 1.5f) * 0.1f;
        Vector2 tip = {sx + sinf(ang) * R * 0.9f, sy - cosf(ang) * R * 0.9f};
        Vector2 base = {sx - sinf(ang) * R * 0.35f, sy + cosf(ang) * R * 0.35f};
        DrawLineEx(base, tip, R * 0.10f, gold);
        DrawCircleV(tip, R * 0.16f, gem);
        DrawCircleV(tip, R * 0.07f, goldHot);
        // cross-guard
        DrawLineEx((Vector2){sx - cosf(ang) * R * 0.22f, sy - sinf(ang) * R * 0.22f},
                   (Vector2){sx + cosf(ang) * R * 0.22f, sy + sinf(ang) * R * 0.22f},
                   R * 0.07f, gold);
    }

    // towering multi-spike crown
    float crownY = cy - R * 1.15f + ((st == ST_TIRED) ? R * 0.18f : sinf(t * 2.2f) * R * 0.05f);
    float tilt = (st == ST_TIRED) ? 0.4f : (st == ST_FIRE) ? 0.15f : 0.0f;
    DrawRectangle((int)(cx - R * 0.75f + tilt * R * 0.1f), (int)crownY,
                  (int)(R * 1.50f), (int)(R * 0.16f), gold);
    for (int k = -3; k <= 3; k++) {
        float hx = cx + k * R * 0.22f + tilt * R * 0.15f;
        float hlen = R * (0.40f + 0.45f * (1.0f - fabsf((float)k) / 3.5f));
        if (k == 0) hlen *= 1.35f;
        DrawTriangle((Vector2){hx - R * 0.09f, crownY},
                     (Vector2){hx + R * 0.09f, crownY},
                     (Vector2){hx, crownY - hlen},
                     (k == 0) ? goldHot : gold);
        if (k == 0) DrawCircleV((Vector2){hx, crownY - hlen + R * 0.08f}, R * 0.10f, gem);
    }
    // hanging jewels from crown
    for (int k = -2; k <= 2; k++) {
        if (k == 0) continue;
        float jx = cx + k * R * 0.28f;
        float jy = crownY + R * 0.20f + fabsf(sinf(t * 3.0f + k)) * R * 0.08f;
        DrawLineEx((Vector2){jx, crownY + R * 0.14f}, (Vector2){jx, jy}, R * 0.03f, gold);
        DrawCircleV((Vector2){jx, jy}, R * 0.07f, (k % 2) ? gem : (Color){255, 60, 100, 255});
    }

    DrawCircle((int)cx, (int)cy, (int)(R * 1.05f), body);

    // ornate gold plate segments + rivets
    for (int k = 0; k < 8; k++) {
        float a0 = k * (2.0f * PI / 8.0f) + t * 0.08f;
        float a1 = a0 + 0.45f;
        DrawLineEx((Vector2){cx + cosf(a0) * R * 1.00f, cy + sinf(a0) * R * 1.00f},
                   (Vector2){cx + cosf(a1) * R * 1.00f, cy + sinf(a1) * R * 1.00f},
                   R * 0.14f, gold);
        DrawCircleV((Vector2){cx + cosf(a0) * R * 1.00f, cy + sinf(a0) * R * 1.00f},
                    R * 0.06f, goldHot);
    }

    // five royal horns
    for (int k = -2; k <= 2; k++) {
        float hx = cx + k * R * 0.28f;
        float hlen = R * (0.70f + 0.25f * (1.0f - fabsf((float)k) / 2.0f));
        DrawTriangle((Vector2){hx - R * 0.12f, cy - R * 0.55f},
                     (Vector2){hx + R * 0.12f, cy - R * 0.55f},
                     (Vector2){hx + (k > 0 ? 1 : k < 0 ? -1 : 0) * R * 0.15f, cy - R * 0.55f - hlen},
                     gold);
    }

    // dripping molten gold
    if (st != ST_TIRED) {
        for (int k = 0; k < 6; k++) {
            float p = fmodf(t * 0.55f + Rnd(k + 30), 1.0f);
            float dx = cx - R * 0.7f + Rnd(k + 2) * R * 1.4f;
            DrawCircleV((Vector2){dx, cy + R * 0.6f + p * R * 0.85f},
                        R * (0.08f - p * 0.04f),
                        (Color){gold.r, gold.g, gold.b, (uchar)(230 * (1.0f - p))});
        }
    }

    // massive centre gem (beam origin)
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircle((int)cx, (int)(cy + R * 0.12f), (int)(R * (0.38f + 0.08f * pulse)),
               (Color){gem.r, gem.g, gem.b, 90});
    EndBlendMode();
    DrawCircle((int)cx, (int)(cy + R * 0.12f), (int)(R * 0.28f), gem);
    DrawCircle((int)(cx - R * 0.06f), (int)(cy + R * 0.06f), (int)(R * 0.10f),
               (Color){255, 255, 255, 200});

    Color eye = StateEye((Color){40, 255, 180, 255}, st);
    if (st == ST_FIRE) eye = (Color){255, 230, 80, 255};
    for (int side = -1; side <= 1; side += 2) {
        float ex = cx + side * R * 0.32f, ey = cy - R * 0.18f;
        DrawCircle((int)ex, (int)ey, (int)(R * 0.16f), eye);
        DrawCircle((int)ex, (int)ey, (int)(R * 0.07f), (Color){10, 20, 15, 255});
        // gold lids
        DrawLineEx((Vector2){ex - R * 0.16f, ey - R * 0.12f},
                   (Vector2){ex + R * 0.16f, ey - R * 0.06f}, R * 0.05f, gold);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Inferno Hydra Crown — hydra silhouette, slightly hotter than the old core
// ═══════════════════════════════════════════════════════════════════════════
static void Boss_EmberHydra_InfernoCrown(float cx, float cy, float R, float t, int st) {
    // hydra palette, nudged hotter
    Color outer = (st == ST_TIRED) ? (Color){40, 30, 28, 255} : (Color){220, 45, 12, 255};
    Color mid   = (st == ST_TIRED) ? (Color){90, 50, 30, 255} : (Color){255, 130, 28, 255};
    Color core  = (st == ST_TIRED) ? (Color){60, 40, 35, 255} : (Color){255, 235, 100, 255};
    float pp = 0.5f + 0.5f * sinf(t * 5.0f);
    DrawAimHint(cx, cy, R, st, t, mid);

    // soft outer heat (only extra vs #6)
    if (st != ST_TIRED) {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle((int)cx, (int)cy, (int)(R * (1.35f + 0.10f * pp)),
                   (Color){255, 80, 10, (uchar)(18 + 12 * pp)});
        EndBlendMode();
    }

    // flame lobes — same layout as #6, one more lobe + slightly larger
    if (st != ST_TIRED) {
        BeginBlendMode(BLEND_ADDITIVE);
        for (int k = 0; k < 10; k++) {
            float a = k * (2.0f * PI / 10.0f) + t * 1.5f;
            float flicker = 0.7f + 0.3f * sinf(t * 9.0f + k);
            float len = (st == ST_DASH) ? R * 1.65f : R * (1.20f + 0.22f * pp);
            DrawCircle((int)(cx + cosf(a) * len * 0.55f),
                       (int)(cy + sinf(a) * len * 0.55f),
                       (int)(R * 0.38f * flicker), (Color){255, 100, 20, 85});
        }
        EndBlendMode();
    }

    DrawCircle((int)cx, (int)cy, (int)(R * 1.05f), outer);
    DrawCircle((int)cx, (int)cy, (int)(R * 0.75f), mid);
    DrawCircle((int)cx, (int)cy, (int)(R * 0.40f), core);
    // brighter core glint
    if (st != ST_TIRED) {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle((int)cx, (int)cy, (int)(R * (0.22f + 0.04f * pp)),
                   (Color){255, 255, 200, (uchar)(90 + 40 * pp)});
        EndBlendMode();
    }

    // rising embers — same as #6, a couple more
    if (st != ST_TIRED) {
        for (int k = 0; k < 9; k++) {
            float p = fmodf(t * 0.6f + Rnd(k + 5), 1.0f);
            DrawCircleV((Vector2){cx - R * 0.5f + Rnd(k) * R, cy + R * 0.6f - p * R * 2.0f},
                        R * 0.05f, (Color){255, 180, 60, (uchar)(220 * sinf(p * PI))});
        }
    }

    // three eyes — same positions, soft glow rings
    Color eye = (st == ST_TIRED) ? (Color){80, 50, 30, 255}
              : (st == ST_DASH)  ? (Color){255, 255, 255, 255}
                                 : (Color){255, 245, 140, 255};
    float lock = (st == ST_WINDUP || st == ST_FIRE) ? 0.06f : 0.0f;
    float eyes[3][2] = {
        { -0.32f + lock, -0.08f },
        {  0.32f + lock, -0.08f },
        {  0.00f + lock, -0.42f },
    };
    for (int i = 0; i < 3; i++) {
        float ex = cx + eyes[i][0] * R, ey = cy + eyes[i][1] * R;
        if (st != ST_TIRED) {
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircle((int)ex, (int)ey, (int)(R * 0.20f), (Color){255, 200, 50, 55});
            EndBlendMode();
        }
        DrawCircle((int)ex, (int)ey, (int)(R * (i == 2 ? 0.12f : 0.13f)), eye);
        DrawCircle((int)ex, (int)ey, (int)(R * 0.05f), (Color){40, 10, 0, 255});
    }

    if (st == ST_FIRE) {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle((int)cx, (int)(cy - R * 0.42f), (int)(R * 0.30f), (Color){255, 200, 50, 130});
        EndBlendMode();
    }

    // flame-tongue horns — same shape, white-hot tips
    if (st != ST_TIRED) {
        DrawTriangle((Vector2){cx - R * 0.55f, cy - R * 0.75f},
                     (Vector2){cx - R * 0.25f, cy - R * 0.85f},
                     (Vector2){cx - R * 0.50f, cy - R * 1.30f}, (Color){255, 90, 20, 230});
        DrawTriangle((Vector2){cx + R * 0.25f, cy - R * 0.85f},
                     (Vector2){cx + R * 0.55f, cy - R * 0.75f},
                     (Vector2){cx + R * 0.50f, cy - R * 1.30f}, (Color){255, 90, 20, 230});
        // tip highlight
        DrawCircleV((Vector2){cx - R * 0.50f, cy - R * 1.28f}, R * 0.07f, (Color){255, 240, 160, 220});
        DrawCircleV((Vector2){cx + R * 0.50f, cy - R * 1.28f}, R * 0.07f, (Color){255, 240, 160, 220});
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 11 — Ember Hydra V3: Solar Cataclysm
// A living sun — corona flares, nova shock rings, eclipse core, ray spikes.
// ═══════════════════════════════════════════════════════════════════════════
static void Boss_EmberHydra_SolarCataclysm(float cx, float cy, float R, float t, int st) {
    float pulse = 0.5f + 0.5f * sinf(t * 4.0f);
    float fury  = (st == ST_FIRE) ? 1.4f : (st == ST_WINDUP || st == ST_DASH) ? 1.2f : 1.0f;
    Color corona = (st == ST_TIRED) ? (Color){100, 60, 40, 160} : (Color){255, 140, 20, 255};
    Color core   = (st == ST_TIRED) ? (Color){120, 80, 50, 255} : (Color){255, 250, 210, 255};
    DrawAimHint(cx, cy, R, st, t, corona);

    // multi-layer corona
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircle((int)cx, (int)cy, (int)(R * 1.90f * fury),
               (Color){255, 80, 0, (uchar)(18 + 14 * pulse)});
    DrawCircle((int)cx, (int)cy, (int)(R * 1.45f * fury),
               (Color){255, 160, 20, (uchar)(30 + 20 * pulse)});

    // solar flare tongues (12 rotating spikes)
    int nFlare = 12;
    for (int k = 0; k < nFlare; k++) {
        float a = t * 0.55f + k * (2.0f * PI / nFlare);
        float flick = 0.75f + 0.25f * sinf(t * 7.0f + k * 1.3f);
        float len = R * (1.35f + 0.55f * flick) * fury;
        if (st == ST_DASH) len *= 1.25f;
        Vector2 tip = {cx + cosf(a) * len, cy + sinf(a) * len};
        Vector2 l   = {cx + cosf(a - 0.12f) * R * 0.85f, cy + sinf(a - 0.12f) * R * 0.85f};
        Vector2 r   = {cx + cosf(a + 0.12f) * R * 0.85f, cy + sinf(a + 0.12f) * R * 0.85f};
        DrawTriangle(l, r, tip, (Color){255, 120, 15, (uchar)(110 + 40 * flick)});
        DrawLineEx((Vector2){cx + cosf(a) * R * 0.9f, cy + sinf(a) * R * 0.9f}, tip,
                   R * 0.06f, (Color){255, 230, 100, (uchar)(160 * flick)});
    }

    // nova rings (expanding)
    if (st != ST_TIRED) {
        for (int r = 0; r < 3; r++) {
            float ring = fmodf(t * 1.2f + r * 0.33f, 1.0f);
            DrawCircleLines((int)cx, (int)cy, (int)(R * (0.9f + ring * 1.3f) * fury),
                            (Color){255, 220, 80, (uchar)(140 * (1.0f - ring))});
        }
    }
    EndBlendMode();

    // sun body gradient
    DrawCircle((int)cx, (int)cy, (int)(R * 1.10f),
               (st == ST_TIRED) ? (Color){90, 55, 35, 255} : (Color){255, 90, 10, 255});
    DrawCircle((int)cx, (int)cy, (int)(R * 0.78f),
               (st == ST_TIRED) ? (Color){140, 90, 40, 255} : (Color){255, 170, 30, 255});
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircle((int)cx, (int)cy, (int)(R * (0.50f + 0.08f * pulse)),
               (Color){core.r, core.g, core.b, (uchar)(180 + 50 * pulse)});
    EndBlendMode();
    DrawCircle((int)cx, (int)cy, (int)(R * 0.32f), core);

    // three eclipse "eyes" as dark spots with fire rims (hydra read)
    for (int e = 0; e < 3; e++) {
        float a = -PI * 0.55f + e * 0.55f + ((st == ST_WINDUP) ? sinf(t * 10.0f) * 0.05f : 0.0f);
        float ex = cx + cosf(a) * R * 0.38f;
        float ey = cy + sinf(a) * R * 0.28f - R * 0.05f;
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle((int)ex, (int)ey, (int)(R * 0.20f), (Color){255, 200, 50, 90});
        EndBlendMode();
        DrawCircle((int)ex, (int)ey, (int)(R * 0.13f), (Color){20, 8, 5, 255});
        Color pupil = (st == ST_DASH) ? (Color){255, 255, 255, 255}
                    : (st == ST_FIRE) ? (Color){255, 80, 10, 255}
                    : (st == ST_TIRED)? (Color){80, 50, 30, 200}
                                      : (Color){255, 220, 60, 255};
        DrawCircle((int)ex, (int)ey, (int)(R * 0.06f), pupil);
    }

    // plasma motes
    if (st != ST_TIRED) {
        for (int k = 0; k < 14; k++) {
            float p = fmodf(t * 0.55f + Rnd(k + 30), 1.0f);
            float a = Rnd(k + 40) * 2.0f * PI + t * 0.8f;
            float dist = R * (0.6f + p * 1.5f) * fury;
            DrawCircleV((Vector2){cx + cosf(a) * dist, cy + sinf(a) * dist},
                        R * 0.045f, (Color){255, 240, 160, (uchar)(200 * sinf(p * PI))});
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Bone Warden  (L30+ rework: ossuary death lord)
// ═══════════════════════════════════════════════════════════════════════════
static void Boss_BoneWarden(float cx, float cy, float R, float t, int st) {
    Color bone = (st == ST_TIRED) ? (Color){140, 135, 130, 255} : (Color){240, 232, 215, 255};
    Color boneDark = (Color){160, 145, 130, 255};
    Color hollow = (Color){25, 10, 40, 255};
    Color soul = (st == ST_FIRE) ? (Color){220, 100, 255, 255}
               : (st == ST_TIRED) ? (Color){55, 35, 75, 160}
                                 : (Color){180, 50, 255, 255};
    float pulse = 0.5f + 0.5f * sinf(t * 3.5f);
    DrawAimHint(cx, cy, R, st, t, soul);

    // soul storm aura
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircle((int)cx, (int)cy, (int)(R * 1.65f), (Color){soul.r, soul.g, soul.b, (uchar)(20 + 18 * pulse)});
    for (int k = 0; k < 10; k++) {
        float p = fmodf(t * 0.55f + Rnd(k + 50), 1.0f);
        float a = Rnd(k + 60) * 2.0f * PI + t;
        DrawCircleV((Vector2){cx + cosf(a) * R * (0.9f + p * 0.9f),
                              cy + sinf(a) * R * (0.9f + p * 0.9f) - p * R * 0.5f},
                    R * 0.05f, (Color){soul.r, soul.g, soul.b, (uchar)(180 * sinf(p * PI))});
    }
    EndBlendMode();

    // orbiting bone storm (ribs, shards, vertebrae)
    for (int k = 0; k < 10; k++) {
        float a = t * 1.3f + k * (2.0f * PI / 10.0f);
        float dist = R * (1.40f + 0.15f * sinf(t * 2.0f + k));
        float bx = cx + cosf(a) * dist, by = cy + sinf(a) * dist * 0.80f;
        if (k % 3 == 0) {
            DrawLineEx((Vector2){bx - R * 0.18f, by}, (Vector2){bx + R * 0.18f, by}, R * 0.07f, bone);
            DrawLineEx((Vector2){bx, by - R * 0.10f}, (Vector2){bx, by + R * 0.10f}, R * 0.05f, bone);
        } else if (k % 3 == 1) {
            DrawTriangle((Vector2){bx, by - R * 0.16f},
                         (Vector2){bx - R * 0.12f, by + R * 0.10f},
                         (Vector2){bx + R * 0.12f, by + R * 0.10f}, bone);
        } else {
            DrawCircleV((Vector2){bx, by}, R * 0.09f, bone);
            DrawCircleV((Vector2){bx, by}, R * 0.04f, hollow);
        }
    }

    // open ribcage body
    DrawCircle((int)cx, (int)cy, (int)(R * 1.05f), boneDark);
    // soul fire inside open chest
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircle((int)cx, (int)(cy + R * 0.10f), (int)(R * (0.45f + 0.10f * pulse)),
               (Color){soul.r, soul.g, soul.b, (uchar)(100 + 50 * pulse)});
    EndBlendMode();
    DrawCircle((int)cx, (int)(cy + R * 0.10f), (int)(R * 0.32f), hollow);

    // curved ribs framing the soul
    for (int k = 0; k < 6; k++) {
        float y = cy - R * 0.35f + k * R * 0.18f;
        float w = R * (0.55f + (k < 3 ? k : 5 - k) * 0.08f);
        DrawLineEx((Vector2){cx - w, y}, (Vector2){cx - R * 0.20f, y + R * 0.05f}, R * 0.07f, bone);
        DrawLineEx((Vector2){cx + w, y}, (Vector2){cx + R * 0.20f, y + R * 0.05f}, R * 0.07f, bone);
    }

    // giant skull head on top of body
    float skullY = cy - R * 0.55f;
    DrawCircle((int)cx, (int)skullY, (int)(R * 0.62f), bone);
    // nasal cavity
    DrawTriangle((Vector2){cx - R * 0.10f, skullY + R * 0.05f},
                 (Vector2){cx + R * 0.10f, skullY + R * 0.05f},
                 (Vector2){cx, skullY + R * 0.28f}, hollow);
    // teeth row
    for (int k = -3; k <= 3; k++) {
        float tx = cx + k * R * 0.12f;
        DrawRectangle((int)(tx - R * 0.04f), (int)(skullY + R * 0.32f),
                      (int)(R * 0.08f), (int)(R * 0.12f), bone);
    }
    float jaw = (st == ST_WINDUP || st == ST_FIRE || st == ST_DASH) ? R * 0.18f : R * 0.04f;
    DrawRectangle((int)(cx - R * 0.40f), (int)(skullY + R * 0.42f + jaw),
                  (int)(R * 0.80f), (int)(R * 0.12f), boneDark);
    for (int k = -3; k <= 3; k++) {
        float tx = cx + k * R * 0.12f;
        DrawRectangle((int)(tx - R * 0.04f), (int)(skullY + R * 0.42f + jaw),
                      (int)(R * 0.08f), (int)(R * 0.10f), bone);
    }

    // massive branching antlers with hanging bones
    for (int side = -1; side <= 1; side += 2) {
        Vector2 a0 = {cx + side * R * 0.40f, skullY - R * 0.35f};
        Vector2 a1 = {cx + side * R * 0.85f, skullY - R * 1.05f};
        Vector2 a2 = {cx + side * R * 1.25f, skullY - R * 1.35f};
        Vector2 b1 = {cx + side * R * 0.55f, skullY - R * 1.20f};
        DrawLineEx(a0, a1, R * 0.10f, bone);
        DrawLineEx(a1, a2, R * 0.08f, bone);
        DrawLineEx(a1, b1, R * 0.07f, bone);
        // hanging bone charms
        for (int h = 0; h < 3; h++) {
            float u = 0.3f + h * 0.25f;
            float hx = a0.x + (a1.x - a0.x) * u;
            float hy = a0.y + (a1.y - a0.y) * u;
            float swing = sinf(t * 2.5f + h + side) * R * 0.06f;
            DrawLineEx((Vector2){hx, hy}, (Vector2){hx + swing, hy + R * 0.35f}, R * 0.03f, boneDark);
            DrawCircleV((Vector2){hx + swing, hy + R * 0.38f}, R * 0.07f, bone);
        }
    }

    // sockets + blazing soul pupils
    float sink = (st == ST_TIRED) ? R * 0.10f : 0.0f;
    for (int side = -1; side <= 1; side += 2) {
        float ex = cx + side * R * 0.28f, ey = skullY - R * 0.08f;
        DrawCircle((int)ex, (int)ey, (int)(R * 0.20f), hollow);
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle((int)ex, (int)(ey + sink), (int)(R * 0.16f),
                   (Color){soul.r, soul.g, soul.b, 100});
        EndBlendMode();
        DrawCircle((int)ex, (int)(ey + sink), (int)(R * 0.09f), soul);
        DrawCircle((int)ex, (int)(ey + sink), (int)(R * 0.04f), (Color){255, 255, 255, 240});
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 8 — Storm Jailer
// ═══════════════════════════════════════════════════════════════════════════
static void Boss_StormJailer(float cx, float cy, float R, float t, int st) {
    Color body = (st == ST_TIRED) ? (Color){55, 60, 70, 255} : (Color){50, 70, 110, 255};
    Color plate = (Color){35, 48, 75, 255};
    Color arc = (st == ST_TIRED) ? (Color){80, 90, 100, 150}
              : (Color){80, 220, 255, 255};
    DrawAimHint(cx, cy, R, st, t, arc);

    // orbiting shackles
    for (int s = 0; s < 2; s++) {
        float side = (s == 0) ? -1.0f : 1.0f;
        float ang = t * 1.4f + s * PI;
        float droop = (st == ST_TIRED) ? R * 0.35f : 0.0f;
        float snap = (st == ST_WINDUP) ? R * 0.25f : 0.0f;
        float sx = cx + side * (R * 1.35f - snap) + cosf(ang) * R * 0.08f;
        float sy = cy + sinf(ang) * R * 0.15f + droop;
        DrawCircleLines((int)sx, (int)sy, (int)(R * 0.28f), (Color){160, 165, 180, 230});
        DrawCircleLines((int)sx, (int)sy, (int)(R * 0.20f), (Color){120, 125, 140, 200});
        // chain link toward body
        DrawLineEx((Vector2){sx, sy}, (Vector2){cx + side * R * 0.7f, cy},
                   R * 0.05f, (Color){140, 145, 160, 200});
    }

    if (st == ST_DASH) {
        // lightning smear
        BeginBlendMode(BLEND_ADDITIVE);
        for (int s = 1; s <= 4; s++)
            DrawCircle((int)(cx - s * R * 0.40f), (int)(cy - s * R * 0.10f),
                       (int)(R * (0.9f - s * 0.12f)),
                       (Color){100, 220, 255, (uchar)(100 - s * 18)});
        EndBlendMode();
    }

    DrawCircle((int)cx, (int)cy, (int)R, body);
    // armour plates
    DrawCircle((int)(cx - R * 0.35f), (int)(cy + R * 0.10f), (int)(R * 0.40f), plate);
    DrawCircle((int)(cx + R * 0.35f), (int)(cy + R * 0.10f), (int)(R * 0.40f), plate);

    // crackle lines
    if (st != ST_TIRED) {
        BeginBlendMode(BLEND_ADDITIVE);
        for (int k = 0; k < 5; k++) {
            float a0 = Rnd(k + (int)(t * 8)) * 2.0f * PI;
            float a1 = a0 + 0.8f + Rnd(k + 3) * 0.5f;
            DrawLineEx((Vector2){cx + cosf(a0) * R * 0.3f, cy + sinf(a0) * R * 0.3f},
                       (Vector2){cx + cosf(a1) * R * 0.95f, cy + sinf(a1) * R * 0.95f},
                       R * 0.04f, (Color){arc.r, arc.g, arc.b, 180});
        }
        EndBlendMode();
    }

    // stub copper horns
    DrawTriangle((Vector2){cx - R * 0.50f, cy - R * 0.70f},
                 (Vector2){cx - R * 0.22f, cy - R * 0.75f},
                 (Vector2){cx - R * 0.42f, cy - R * 1.10f}, (Color){180, 110, 50, 255});
    DrawTriangle((Vector2){cx + R * 0.22f, cy - R * 0.75f},
                 (Vector2){cx + R * 0.50f, cy - R * 0.70f},
                 (Vector2){cx + R * 0.42f, cy - R * 1.10f}, (Color){180, 110, 50, 255});

    // cyan visor slits
    Color visor = (st == ST_TIRED) ? (Color){80, 90, 100, 200} : arc;
    DrawRectangle((int)(cx - R * 0.48f), (int)(cy - R * 0.18f), (int)(R * 0.32f), (int)(R * 0.10f), visor);
    DrawRectangle((int)(cx + R * 0.16f), (int)(cy - R * 0.18f), (int)(R * 0.32f), (int)(R * 0.10f), visor);
}

// ═══════════════════════════════════════════════════════════════════════════
// 9 — Primeval Chaos  (L30+ rework: reality-breaking final form)
// ═══════════════════════════════════════════════════════════════════════════
static void Boss_PrimevalChaos(float cx, float cy, float R, float t, int st) {
    Color body = (st == ST_TIRED) ? (Color){85, 45, 115, 255} : (Color){175, 25, 210, 255};
    float pulse = 0.5f + 0.5f * sinf(t * 4.0f);
    float glitch = (st == ST_TIRED) ? 0.15f : 1.0f;
    DrawAimHint(cx, cy, R, st, t, body);

    // reality shatter shards around body
    for (int k = 0; k < 8; k++) {
        float a = t * 0.7f + k * (2.0f * PI / 8.0f);
        float dist = R * (1.55f + 0.25f * sinf(t * 3.0f + k));
        float sx = cx + cosf(a) * dist, sy = cy + sinf(a) * dist;
        Color sc = ColorFromHSV(fmodf(t * 40.0f + k * 45.0f, 360.0f), 0.85f, 1.0f);
        sc.a = (uchar)(140 * glitch);
        DrawTriangle((Vector2){sx, sy - R * 0.20f},
                     (Vector2){sx - R * 0.16f, sy + R * 0.14f},
                     (Vector2){sx + R * 0.16f, sy + R * 0.14f}, sc);
    }

    // heavy chromatic split (always on, stronger in combat)
    float split = R * (0.10f + 0.10f * glitch) * ((st == ST_WINDUP || st == ST_DASH) ? 1.8f : 1.0f);
    DrawCircle((int)(cx - split), (int)cy, (int)(R * 1.08f), (Color){255, 30, 90, (uchar)(75 * glitch)});
    DrawCircle((int)(cx + split), (int)cy, (int)(R * 1.08f), (Color){30, 230, 255, (uchar)(75 * glitch)});
    DrawCircle((int)cx, (int)(cy - split * 0.5f), (int)(R * 1.00f), (Color){200, 40, 255, (uchar)(40 * glitch)});

    // triple ghost bodies on dash
    if (st == ST_DASH || st == ST_WINDUP) {
        for (int g = 3; g >= 1; g--) {
            float gx = cx - g * R * 0.70f;
            float gy = cy - g * R * 0.18f;
            Color gc = ColorFromHSV(fmodf(g * 90.0f + t * 50.0f, 360.0f), 0.7f, 1.0f);
            gc.a = (uchar)(55 - g * 8);
            DrawCircle((int)gx, (int)gy, (int)(R * (0.95f - g * 0.08f)), gc);
            DrawCircle((int)(gx - R * 0.22f), (int)(gy - R * 0.10f), (int)(R * 0.10f),
                       (Color){255, 255, 255, (uchar)(80 - g * 15)});
            DrawCircle((int)(gx + R * 0.22f), (int)(gy - R * 0.10f), (int)(R * 0.10f),
                       (Color){255, 255, 255, (uchar)(80 - g * 15)});
        }
    }

    // rainbow glitch ring (thick, spinning, broken)
    for (int k = 0; k < 18; k++) {
        float a0 = k * (2.0f * PI / 18.0f) + t * 1.2f;
        float a1 = a0 + 0.28f;
        // intentional gaps for glitch feel
        if ((k + (int)(t * 6.0f)) % 5 == 0) continue;
        Color seg = ColorFromHSV(fmodf(k * 20.0f + t * 80.0f, 360.0f), 0.9f, 1.0f);
        if (st == ST_TIRED) { seg.r /= 2; seg.g /= 2; seg.b /= 2; seg.a = 120; }
        else seg.a = 200;
        float jitter = ((k + (int)(t * 12)) % 3 == 0) ? R * 0.12f * glitch : 0.0f;
        DrawLineEx((Vector2){cx + cosf(a0) * (R * 1.35f + jitter), cy + sinf(a0) * (R * 1.35f + jitter)},
                   (Vector2){cx + cosf(a1) * (R * 1.35f - jitter), cy + sinf(a1) * (R * 1.35f - jitter)},
                   R * 0.16f, seg);
    }

    // scanline / tear bars across body
    if (st != ST_TIRED) {
        for (int k = 0; k < 4; k++) {
            float y = cy - R * 0.7f + fmodf(t * 40.0f + k * 28.0f, R * 1.6f);
            DrawRectangle((int)(cx - R * 1.1f), (int)y, (int)(R * 2.2f), (int)(R * 0.06f),
                          (Color){255, 255, 255, (uchar)(40 + 30 * ((k + (int)(t * 8)) % 2))});
        }
    }

    BossAura(cx, cy, R, body, t, st);
    DrawCircle((int)cx, (int)cy, (int)(R * 1.08f), body);
    // unstable inner core
    Color core = ColorFromHSV(fmodf(t * 90.0f, 360.0f), 0.6f, 1.0f);
    core.a = 160;
    DrawCircle((int)cx, (int)cy, (int)(R * (0.45f + 0.08f * pulse)), core);
    DrawCircle((int)(cx - R * 0.22f), (int)(cy - R * 0.28f), (int)(R * 0.48f),
               (Color){CLAMP255(body.r + 55), CLAMP255(body.g + 40), CLAMP255(body.b + 55), 100});

    // six horns — gold + chaos-colored rear spikes
    Color horn = (Color){255, 230, 70, 255};
    DrawTriangle((Vector2){cx - R * 0.70f, cy - R * 0.50f},
                 (Vector2){cx - R * 0.32f, cy - R * 0.72f},
                 (Vector2){cx - R * 0.95f, cy - R * 1.40f}, horn);
    DrawTriangle((Vector2){cx + R * 0.32f, cy - R * 0.72f},
                 (Vector2){cx + R * 0.70f, cy - R * 0.50f},
                 (Vector2){cx + R * 0.95f, cy - R * 1.40f}, horn);
    DrawTriangle((Vector2){cx - R * 0.35f, cy - R * 0.78f},
                 (Vector2){cx - R * 0.12f, cy - R * 0.88f},
                 (Vector2){cx - R * 0.45f, cy - R * 1.30f}, (Color){255, 40, 180, 230});
    DrawTriangle((Vector2){cx + R * 0.12f, cy - R * 0.88f},
                 (Vector2){cx + R * 0.35f, cy - R * 0.78f},
                 (Vector2){cx + R * 0.45f, cy - R * 1.30f}, (Color){40, 230, 255, 230});
    DrawTriangle((Vector2){cx - R * 0.10f, cy - R * 0.90f},
                 (Vector2){cx + R * 0.10f, cy - R * 0.90f},
                 (Vector2){cx, cy - R * 1.55f}, ColorFromHSV(fmodf(t * 60.0f, 360.0f), 0.8f, 1.0f));
    // tiny rear spike pair
    DrawTriangle((Vector2){cx - R * 0.55f, cy - R * 0.20f},
                 (Vector2){cx - R * 0.40f, cy - R * 0.30f},
                 (Vector2){cx - R * 0.75f, cy - R * 0.55f}, (Color){255, 80, 255, 180});
    DrawTriangle((Vector2){cx + R * 0.40f, cy - R * 0.30f},
                 (Vector2){cx + R * 0.55f, cy - R * 0.20f},
                 (Vector2){cx + R * 0.75f, cy - R * 0.55f}, (Color){80, 255, 255, 180});

    // orbiting chaos runes / stars
    if (st != ST_TIRED) {
        for (int k = 0; k < 10; k++) {
            float a = t * 1.8f + k * (2.0f * PI / 10.0f);
            Color sc = ColorFromHSV(fmodf(k * 36.0f + t * 70.0f, 360.0f), 0.7f, 1.0f);
            sc.a = 210;
            Star4(cx + cosf(a) * R * 1.15f, cy + sinf(a) * R * 1.15f, R * 0.09f, sc);
        }
    }

    // fire: multicolour maw beam origin
    if (st == ST_FIRE) {
        BeginBlendMode(BLEND_ADDITIVE);
        for (int k = 0; k < 5; k++) {
            Color fc = ColorFromHSV(fmodf(t * 100.0f + k * 50.0f, 360.0f), 0.9f, 1.0f);
            fc.a = 100;
            DrawCircle((int)cx, (int)(cy + R * 0.15f), (int)(R * (0.35f + k * 0.08f)), fc);
        }
        EndBlendMode();
    }

    // eyes: white crosses + occasional glitch offset
    Color eye = (st == ST_TIRED) ? (Color){130, 110, 145, 255}
              : (st == ST_FIRE)  ? ColorFromHSV(fmodf(t * 120.0f, 360.0f), 0.8f, 1.0f)
                                 : (Color){255, 255, 255, 255};
    float eyeJ = ((int)(t * 9.0f) % 7 == 0 && st != ST_TIRED) ? R * 0.08f : 0.0f;
    for (int side = -1; side <= 1; side += 2) {
        float ex = cx + side * R * 0.30f + eyeJ * side, ey = cy - R * 0.14f;
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle((int)ex, (int)ey, (int)(R * 0.26f), (Color){eye.r, eye.g, eye.b, 70});
        EndBlendMode();
        DrawCircle((int)ex, (int)ey, (int)(R * 0.18f), eye);
        DrawLineEx((Vector2){ex - R * 0.12f, ey}, (Vector2){ex + R * 0.12f, ey},
                   R * 0.05f, (Color){10, 0, 20, 255});
        DrawLineEx((Vector2){ex, ey - R * 0.12f}, (Vector2){ex, ey + R * 0.12f},
                   R * 0.05f, (Color){10, 0, 20, 255});
    }
    DrawBrow(cx, cy, R, (Color){30, 5, 45, 255});
}

// ─── table ──────────────────────────────────────────────────────────────────
typedef void (*BossDrawFn)(float cx, float cy, float R, float t, int st);

typedef struct {
    const char *name;
    const char *tag;       // short tagline
    const char *desc;      // hover blurb
    const char *tierHint;  // suggested slot
    BossDrawFn  draw;
} BossIdea;

static BossIdea ideas[NBOSS] = {
    { "Chaos Warden",    "current baseline",
      "Purple disc, gold horns, yellow eyes — the shipped look.",
      "hint L10", Boss_ChaosWarden },
    { "Obsidian Brute",  "magma titan  L30+",
      "Massive basalt bulk, orbiting shards, branching lava fissures, crater eyes, roar maw.",
      "hint L30-50", Boss_ObsidianBrute },
    { "Crimson Herald",  "blood knight",
      "Crimson disc, gold crown ring, swept horns, slit eyes.",
      "hint L20", Boss_CrimsonHerald },
    { "Ashen Wraith",    "death cyclone  L30+",
      "Soul vortex, ghost afterimages, skull core, forked smoke horns, tear trails.",
      "hint L30-50", Boss_AshenWraith },
    { "Void Maw",        "black-hole apex  L30+",
      "Triple accretion discs, fang ring, event horizon, matter streams, reality tears.",
      "hint L40-50", Boss_VoidMaw },
    { "Gilded Tyrant",   "god-king  L30+",
      "Halo spokes, dual scepters, tower crown, molten gold drips, five horns, beam gem.",
      "hint L30-50", Boss_GildedTyrant },
    { "Bone Warden",     "ossuary death lord  L30+",
      "Open ribcage + soul fire, giant skull jaw, bone storm, antlers with hanging charms.",
      "hint L30-50", Boss_BoneWarden },
    { "Storm Jailer",    "arc + shackles",
      "Steel-blue armour, cyan crackle, orbiting iron shackles.",
      "hint L30", Boss_StormJailer },
    { "Primeval Chaos",  "reality break  L30+",
      "RGB split, rainbow glitch ring, multi-ghost dash, scan tears, six horns, chaos core.",
      "hint L40-50", Boss_PrimevalChaos },
    { "Inferno Hydra Crown", "Ember Hydra V2",
      "Like original hydra, hotter: denser flame lobes, core glint, eye glow rings, white-hot horn tips.",
      "hint L20-30", Boss_EmberHydra_InfernoCrown },
    { "Solar Cataclysm",     "Ember Hydra V3",
      "Living sun: rotating solar flares, nova shock rings, three eclipse eyes, plasma motes.",
      "hint L20-40", Boss_EmberHydra_SolarCataclysm },
};

// ─── persistence ────────────────────────────────────────────────────────────
static bool bad[NBOSS];
static int  tierOf[NBOSS];   // 0 = unassigned, 1..5 = L10..L50

static void LoadBad(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\0') continue;
        int id;
        if (sscanf(line, "%d", &id) == 1 && id >= 0 && id < NBOSS) bad[id] = true;
    }
    fclose(f);
}

static void SaveBad(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "# bad_boss_ideas.txt — rejected boss skins (auto by Boss_ideas.exe)\n");
    fprintf(f, "# id  name\n");
    for (int i = 0; i < NBOSS; i++)
        if (bad[i]) fprintf(f, "%d  %s\n", i, ideas[i].name);
    fclose(f);
}

static void LoadPicks(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\0') continue;
        int id, tier;
        if (sscanf(line, "%d %d", &id, &tier) == 2 &&
            id >= 0 && id < NBOSS && tier >= 0 && tier <= 5)
            tierOf[id] = tier;
    }
    fclose(f);
}

static void SavePicks(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "# boss_picks.txt — tier assignments (auto by Boss_ideas.exe)\n");
    fprintf(f, "# id tier(1=L10..5=L50)  name\n");
    static const char *LN[6] = { "-", "L10", "L20", "L30", "L40", "L50" };
    for (int i = 0; i < NBOSS; i++)
        if (tierOf[i] > 0)
            fprintf(f, "%d %d  %s  %s\n", i, tierOf[i], LN[tierOf[i]], ideas[i].name);
    fclose(f);
}

// Assign boss `id` to tier `t` (1..5). Clears any other boss already on that tier.
static void AssignTier(int id, int t) {
    if (id < 0 || id >= NBOSS) return;
    if (t < 0) t = 0;
    if (t > 5) t = 5;
    if (t > 0) {
        for (int i = 0; i < NBOSS; i++)
            if (tierOf[i] == t) tierOf[i] = 0;
    }
    tierOf[id] = t;
    if (t > 0) bad[id] = false;   // assigned implies kept
    SavePicks("boss_picks.txt");
    SaveBad("bad_boss_ideas.txt");
}

// ═══════════════════════════════════════════════════════════════════════════
int main(void) {
    for (int i = 0; i < NBOSS; i++) { bad[i] = false; tierOf[i] = 0; }
    LoadBad("bad_boss_ideas.txt");
    LoadPicks("boss_picks.txt");

    const int sw = 1280, sh = 820;
    InitWindow(sw, sh, "Boss Ideas Picker — Chaos Warden skins");
    SetTargetFPS(60);

    const int headerH = 72;
    const int leftM   = 28;
    const int cols    = 5;
    const int cellW   = (sw - leftM * 2) / cols;
    const int cellH   = 210;
    const float R     = 36.0f;

    int scroll = 0;
    char hoverDesc[160] = "";
    Vector2 hoverPos = {0};
    int hoverId = -1;

    static const char *TIER_LBL[6] = { "", "L10", "L20", "L30", "L40", "L50" };
    static const Color TIER_COL[6] = {
        {0,0,0,0},
        {120, 200, 120, 255},
        {100, 180, 255, 255},
        {255, 200, 80, 255},
        {255, 140, 80, 255},
        {220, 100, 255, 255},
    };

    while (!WindowShouldClose()) {
        float t = (float)GetTime();
        Vector2 mp = GetMousePosition();
        bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        hoverDesc[0] = '\0';
        hoverId = -1;

        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_S))
            demoState = (demoState + 1) % ST_COUNT;

        // visible (non-rejected) list
        int vis[NBOSS], nvis = 0;
        for (int i = 0; i < NBOSS; i++) if (!bad[i]) vis[nvis++] = i;

        int visRows = (nvis + cols - 1) / cols;
        if (visRows < 1) visRows = 1;
        int gridH = 40 + visRows * cellH;

        // rejected band
        int nrej = 0;
        for (int i = 0; i < NBOSS; i++) if (bad[i]) nrej++;
        int rejCols = 8;
        int rejRows = (nrej + rejCols - 1) / rejCols;
        if (rejRows < 1) rejRows = 1;
        int rejH = 60 + rejRows * 100;
        int rejTop = headerH + gridH + 10;
        // content height from top of scrollable area
        int contentH = gridH + 10 + rejH + 30;
        int maxScroll = contentH - (sh - headerH);
        if (maxScroll < 0) maxScroll = 0;

        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) scroll -= (int)(wheel * 48.0f);
        if (IsKeyDown(KEY_DOWN)) scroll += 8;
        if (IsKeyDown(KEY_UP))   scroll -= 8;
        if (scroll < 0) scroll = 0;
        if (scroll > maxScroll) scroll = maxScroll;

        // number keys assign hovered boss (handled after hover detection below —
        // we re-scan hover in the layout pass)

        BeginDrawing();
        ClearBackground((Color){12, 10, 18, 255});

        DrawText("BOSS IDEAS PICKER  —  click = REJECT/restore   |   hover+1..5 = assign L10..L50   |   hover+0 = clear   |   Space = cycle state",
                 leftM, 10, 14, (Color){230, 220, 245, 255});
        DrawText(TextFormat("Demo state: [%s]   |   %d kept / %d rejected   |   picks saved to boss_picks.txt",
                            ST_NAME[demoState], nvis, nrej),
                 leftM, 32, 13, (Color){160, 150, 180, 220});

        // tier summary strip
        int sx = leftM;
        for (int tier = 1; tier <= 5; tier++) {
            int who = -1;
            for (int i = 0; i < NBOSS; i++) if (tierOf[i] == tier) { who = i; break; }
            const char *nm = (who >= 0) ? ideas[who].name : "(empty)";
            const char *line = TextFormat("%s: %s", TIER_LBL[tier], nm);
            DrawText(line, sx, 52, 13, TIER_COL[tier]);
            sx += MeasureText(line, 13) + 28;
        }

        BeginScissorMode(0, headerH, sw, sh - headerH);

        // ── active grid ────────────────────────────────────────────────────
        int gridTop = headerH - scroll;
        DrawText("CANDIDATES", leftM, gridTop + 8, 15, (Color){200, 190, 220, 220});

        for (int pos = 0; pos < nvis; pos++) {
            int id = vis[pos];
            int col = pos % cols;
            int row = pos / cols;
            int cx = leftM + col * cellW + cellW / 2;
            int cy = gridTop + 70 + row * cellH + 50;

            Rectangle hit = {(float)(cx - R * 1.7f), (float)(cy - R * 1.7f),
                             R * 3.4f, R * 3.4f};
            bool hov = CheckCollisionPointRec(mp, hit) && mp.y >= headerH;

            DrawCircleV((Vector2){(float)cx, (float)cy}, R * 1.75f,
                        hov ? (Color){255, 80, 80, 22} : (Color){255, 255, 255, 8});
            ideas[id].draw((float)cx, (float)cy, R, t, demoState);

            // name + tag
            int tw = MeasureText(ideas[id].name, 14);
            DrawText(ideas[id].name, cx - tw / 2, cy + (int)(R * 1.85f), 14,
                     (Color){230, 220, 245, 240});
            int tw2 = MeasureText(ideas[id].tag, 12);
            DrawText(ideas[id].tag, cx - tw2 / 2, cy + (int)(R * 1.85f) + 18, 12,
                     (Color){150, 140, 170, 200});
            int tw3 = MeasureText(ideas[id].tierHint, 11);
            DrawText(ideas[id].tierHint, cx - tw3 / 2, cy + (int)(R * 1.85f) + 34, 11,
                     (Color){120, 110, 140, 180});

            // tier badge
            if (tierOf[id] > 0) {
                const char *lb = TIER_LBL[tierOf[id]];
                int bw = MeasureText(lb, 14) + 12;
                int bx = cx - bw / 2, by = cy - (int)(R * 1.95f);
                DrawRectangleRounded((Rectangle){(float)bx, (float)by, (float)bw, 20},
                                     0.4f, 4, (Color){20, 16, 28, 230});
                DrawRectangleRoundedLinesEx((Rectangle){(float)bx, (float)by, (float)bw, 20},
                                            0.4f, 4, 1.5f, TIER_COL[tierOf[id]]);
                DrawText(lb, bx + 6, by + 3, 14, TIER_COL[tierOf[id]]);
            }

            // id chip
            DrawText(TextFormat("#%d", id), cx - (int)(R * 1.6f), cy - (int)(R * 1.6f),
                     12, (Color){130, 120, 150, 180});

            if (hov) {
                DrawCircleLines(cx, cy, (int)(R * 1.75f), (Color){255, 90, 90, 220});
                strncpy(hoverDesc, ideas[id].desc, sizeof(hoverDesc) - 1);
                hoverPos = (Vector2){(float)cx, (float)(cy - R * 2.1f)};
                hoverId = id;
                if (clicked) {
                    bad[id] = true;
                    tierOf[id] = 0;
                    SaveBad("bad_boss_ideas.txt");
                    SavePicks("boss_picks.txt");
                }
            }
        }

        // ── rejected ───────────────────────────────────────────────────────
        int bandTop = rejTop - scroll;
        DrawText("REJECTED  (click to restore)", leftM, bandTop + 8, 15,
                 (Color){225, 140, 140, 225});
        DrawLine(0, bandTop + rejH - 4, sw, bandTop + rejH - 4, (Color){40, 36, 55, 255});

        if (nrej == 0) {
            DrawText("(none yet)", leftM, bandTop + 40, 13, (Color){110, 105, 130, 200});
        } else {
            int idx = 0;
            float rR = 22.0f;
            int rejCellW = (sw - leftM * 2) / rejCols;
            for (int i = 0; i < NBOSS; i++) {
                if (!bad[i]) continue;
                int rc = idx % rejCols, rr = idx / rejCols;
                int cx = leftM + rc * rejCellW + rejCellW / 2;
                int cy = bandTop + 55 + rr * 95;

                Rectangle hit = {(float)(cx - rR * 1.6f), (float)(cy - rR * 1.6f),
                                 rR * 3.2f, rR * 3.2f};
                bool hov = CheckCollisionPointRec(mp, hit) && mp.y >= headerH;

                DrawCircleV((Vector2){(float)cx, (float)cy}, rR * 1.6f,
                            hov ? (Color){120, 255, 120, 28} : (Color){255, 255, 255, 6});
                ideas[i].draw((float)cx, (float)cy, rR, t, demoState);
                DrawLineEx((Vector2){cx - rR * 1.2f, cy - rR * 1.2f},
                           (Vector2){cx + rR * 1.2f, cy + rR * 1.2f},
                           3.0f, (Color){255, 60, 60, 200});

                int tw = MeasureText(ideas[i].name, 11);
                DrawText(ideas[i].name, cx - tw / 2, cy + (int)(rR * 1.75f), 11,
                         (Color){170, 165, 190, 200});

                if (hov) {
                    DrawCircleLines(cx, cy, (int)(rR * 1.6f), (Color){120, 255, 120, 220});
                    strncpy(hoverDesc, ideas[i].desc, sizeof(hoverDesc) - 1);
                    hoverPos = (Vector2){(float)cx, (float)(cy - rR * 2.0f)};
                    hoverId = i;
                    if (clicked) {
                        bad[i] = false;
                        SaveBad("bad_boss_ideas.txt");
                    }
                }
                idx++;
            }
        }

        EndScissorMode();

        // keyboard assignment for hovered
        if (hoverId >= 0 && !bad[hoverId]) {
            if (IsKeyPressed(KEY_ONE)   || IsKeyPressed(KEY_KP_1)) AssignTier(hoverId, 1);
            if (IsKeyPressed(KEY_TWO)   || IsKeyPressed(KEY_KP_2)) AssignTier(hoverId, 2);
            if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) AssignTier(hoverId, 3);
            if (IsKeyPressed(KEY_FOUR)  || IsKeyPressed(KEY_KP_4)) AssignTier(hoverId, 4);
            if (IsKeyPressed(KEY_FIVE)  || IsKeyPressed(KEY_KP_5)) AssignTier(hoverId, 5);
            if (IsKeyPressed(KEY_ZERO)  || IsKeyPressed(KEY_KP_0)) AssignTier(hoverId, 0);
        }

        // scrollbar
        if (maxScroll > 0) {
            int sbX = sw - 10, sbY = headerH, sbH = sh - headerH;
            int sbTH = (int)((float)sbH / contentH * sbH);
            if (sbTH < 20) sbTH = 20;
            int sbTY = sbY + (int)((float)scroll / maxScroll * (sbH - sbTH));
            DrawRectangle(sbX, sbY, 4, sbH, (Color){255, 255, 255, 25});
            DrawRectangle(sbX, sbTY, 4, sbTH, (Color){255, 255, 255, 110});
        }

        // hover tooltip
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

    CloseWindow();
    return 0;
}
