// game_boss.c — boss AI (stalk/windup/dash/tired), arena cannons, damage
// handling and rendering for RK_BOSS rounds.
#include "game_boss.h"
#include "globals.h"
#include "constants.h"
#include "types.h"
#include "game_particles.h"  // Burst, SpawnPar
#include "game_map.h"        // WX, WY
#include "game_save.h"       // UpdateAndSaveStats
#include "game_sfx.h"        // PlaySfx
#include "game_utils.h"      // AddScreenShake
#include <math.h>
#include <stdlib.h>          // abs

// Prime HP per boss number (round 10 -> 3, 20 -> 5, ..., 50+ -> 13 cap).
static const int BOSS_HP[] = { 3, 5, 7, 11, 13 };

// Per-tier tempo + ability unlocks — later bosses get FASTER and MEANER, not
// just fatter. Tier 1 is the intro boss (melee only, baseline constants);
// tier 2 unlocks the fire breath; tier 3 the chained double dash; tiers 4-5
// crank the dials (more adds, longer beam, shorter breathers).
typedef struct {
    float stalkCd;    // seconds per stalk step
    float chargeMin;  // min seconds between charges
    float dashStep;   // seconds per tile during a dash
    int   fireRange;  // beam length in tiles
    int   addWant;    // max adds per summon
    float addIntvl;   // seconds between add summons
    bool  hasFire;    // fire breath unlocked
    bool  doubleDash; // dash chains into a second, re-aimed dash
    int   trampleCap; // max trample traps alive at once (oldest despawns)
} BossTier;

static const BossTier TIERS[5] = {
    { BOSS_STALK_CD, BOSS_CHARGE_MIN, BOSS_DASH_STEP, BOSS_FIRE_RANGE,   2, BOSS_ADD_INTERVAL, false, false, 12 },
    { 0.40f,         6.0f,            0.052f,         BOSS_FIRE_RANGE+2, 2, BOSS_ADD_INTERVAL, true,  false, 16 },
    { 0.37f,         5.5f,            0.048f,         BOSS_FIRE_RANGE+4, 2, 6.5f,              true,  true,  20 },
    { 0.34f,         5.0f,            0.044f,         BOSS_FIRE_RANGE+6, 3, 5.5f,              true,  true,  24 },
    { 0.30f,         4.5f,            0.040f,         BOSS_FIRE_RANGE+8, 3, 4.5f,              true,  true,  28 },
};

// Set once per SpawnBoss; only one boss ever exists at a time.
static const BossTier *tier = &TIERS[0];
static int bossTierIdx = 0;   // 0..4 — visual skin + HUD name for this fight

// Boss picks (from Skin_Ideas/boss_picks.txt):
//   L10 Chaos Warden | L20 Crimson Herald | L30 Inferno Hydra
//   L40 Obsidian Brute | L50 Ashen Wraith
static const char *BOSS_TIER_NAME[5] = {
    "CHAOS WARDEN",
    "CRIMSON HERALD",
    "INFERNO HYDRA",
    "OBSIDIAN BRUTE",
    "ASHEN WRAITH",
};
static const Color BOSS_TIER_ACCENT[5] = {
    {170, 40, 200, 255},   // purple
    {160, 28, 48, 255},    // crimson
    {255, 120, 30, 255},   // ember
    {255, 110, 25, 255},   // lava
    {100, 220, 230, 255},  // soul cyan (readable on dark)
};

BossGuideInfo BossGuideForTier(int index) {
    if(index<0)index=0;
    if(index>4)index=4;
    const BossTier *data=&TIERS[index];
    int hp=BOSS_HP[index];
    int addCap=hp*BOSS_ADDS_PER_HP;
    if(addCap>BOSS_ADD_CAP)addCap=BOSS_ADD_CAP;
    return (BossGuideInfo){BOSS_TIER_NAME[index],(index+1)*10,hp,
                           data->addWant,addCap,data->fireRange,
                           data->stalkCd,data->dashStep,data->chargeMin,
                           data->addIntvl,data->hasFire,data->doubleDash};
}

const char *BossDisplayName(void) {
    if (bossTierIdx < 0 || bossTierIdx > 4) return "CHAOS WARDEN";
    return BOSS_TIER_NAME[bossTierIdx];
}

static Color BossAccent(void) {
    if (bossTierIdx < 0 || bossTierIdx > 4) return CBOSS;
    return BOSS_TIER_ACCENT[bossTierIdx];
}

// Art-facing combat state (collapses fire windup into fire).
typedef enum { ART_STALK=0, ART_WINDUP, ART_DASH, ART_FIRE, ART_TIRED } ArtSt;
static ArtSt BossArtSt(void) {
    switch (boss.state) {
        case BS_WINDUP: return ART_WINDUP;
        case BS_DASH:   return ART_DASH;
        case BS_TIRED:  return ART_TIRED;
        case BS_FIRE_WINDUP:
        case BS_FIRE:   return ART_FIRE;
        default:        return ART_STALK;
    }
}

static float Brnd(int i) {
    float x = sinf(i * 127.1f + 311.7f) * 43758.5453f;
    return x - floorf(x);
}

static Color FlashCol(Color c, bool flash) {
    if (!flash) return c;
    return (Color){245, 235, 255, c.a};
}

// ─── BODY ART (per boss pick; telegraphs stay in DrawBoss) ─────────────────
// R is the body radius (~CELL-4). Coordinates are world pixels at footprint centre.

// L10 Chaos Warden — exact body art from DungeonCrawler_Save, scaled from the
// same R used by the live boss so the tutorial preview stays 1:1 with gameplay.
static void DrawSkin_ChaosWarden(float cx, float cy, float R, float t, ArtSt st, bool flash) {
    float pp = 0.5f + 0.5f * sinf(t * 3.0f);
    bool windup = (st == ART_WINDUP);
    bool fire = (st == ART_FIRE);

    // Aura + body (2x2 tiles -> radius just under one full cell)
    Color aura = windup ? (Color){235, 70, 70, 0}
               : fire ? CFIRE
               : CBOSS;
    float artScale = R / (float)(CELL - 4);
    float windupK = windup ? 1.0f - boss.stT / BOSS_WINDUP_TIME : 0.0f;
    if (windupK < 0.0f) windupK = 0.0f;
    if (windupK > 1.0f) windupK = 1.0f;
    float ar   = windup ? (CELL + 10 + 8 * windupK) * artScale
                : fire ? (CELL + 14 + 6 * pp) * artScale
                       : (CELL + 6 + 4 * pp) * artScale;
    DrawCircle((int)cx, (int)cy, (int)ar, (Color){aura.r, aura.g, aura.b, (uchar)(26 + 18 * pp)});

    Color body = flash ? (Color){245, 235, 255, 255} : CBOSS;
    if (st == ART_TIRED) body = (Color){110, 80, 130, 255};   // drained
    DrawCircle((int)cx, (int)cy, (int)R, body);
    DrawCircle((int)(cx - R * 0.21f), (int)(cy - R * 0.29f), (int)(R * 0.57f),
               (Color){CLAMP255(body.r + 50), CLAMP255(body.g + 50), CLAMP255(body.b + 50), 90});

    // Horns
    Color horn = flash ? (Color){255, 255, 255, 255} : (Color){235, 220, 90, 255};
    DrawTriangle((Vector2){cx - R * 0.64f, cy - R * 0.57f},
                 (Vector2){cx - R * 0.29f, cy - R * 0.71f},
                 (Vector2){cx - R * 0.71f, cy - R * 1.21f}, horn);
    DrawTriangle((Vector2){cx + R * 0.29f, cy - R * 0.71f},
                 (Vector2){cx + R * 0.64f, cy - R * 0.57f},
                 (Vector2){cx + R * 0.71f, cy - R * 1.21f}, horn);

    // Eyes: angry while hunting, white-hot in a dash, fiery while breathing, dull when tired.
    Color eye = (st == ART_DASH) ? (Color){255, 255, 255, 255}
              : (st == ART_TIRED) ? (Color){170, 140, 60, 255}
              : fire ? CFIRE
              : (Color){255, 200, 0, 255};
    float eyeR = fmaxf(2.0f, R * 0.18f);
    DrawCircle((int)(cx - R * 0.29f), (int)(cy - R * 0.14f), (int)eyeR, eye);
    DrawCircle((int)(cx + R * 0.29f), (int)(cy - R * 0.14f), (int)eyeR, eye);
    DrawCircle((int)(cx - R * 0.29f), (int)(cy - R * 0.14f), (int)fmaxf(1.0f, R * 0.07f), (Color){30, 0, 10, 255});
    DrawCircle((int)(cx + R * 0.29f), (int)(cy - R * 0.14f), (int)fmaxf(1.0f, R * 0.07f), (Color){30, 0, 10, 255});
    DrawLineEx((Vector2){cx - R * 0.50f, cy - R * 0.43f},
               (Vector2){cx - R * 0.11f, cy - R * 0.29f},
               fmaxf(1.0f, R * 0.07f), (Color){40, 10, 50, 255});
    DrawLineEx((Vector2){cx + R * 0.11f, cy - R * 0.29f},
               (Vector2){cx + R * 0.50f, cy - R * 0.43f},
               fmaxf(1.0f, R * 0.07f), (Color){40, 10, 50, 255});
}

static void DrawSkin_CrimsonHerald(float cx, float cy, float R, float t, ArtSt st, bool flash) {
    Color body = FlashCol((st == ART_TIRED) ? (Color){110, 55, 48, 255} : (Color){160, 28, 48, 255}, flash);
    Color gold = flash ? (Color){255, 255, 255, 255} : (Color){220, 180, 70, 255};
    float pp = 0.5f + 0.5f * sinf(t * 3.0f);
    DrawCircle((int)cx, (int)cy, (int)(R * 1.20f + R * 0.12f * pp),
               (Color){body.r, body.g, body.b, (uchar)(26 + 18 * pp)});
    // filled gold crown ring (rotating highlight)
    float spin = (st == ART_WINDUP) ? t * 4.0f : t * 0.8f;
    float deg = spin * (180.0f / PI);
    DrawRing((Vector2){cx, cy}, R * 1.12f, R * 1.32f, 0.0f, 360.0f, 48, gold);
    DrawRing((Vector2){cx, cy}, R * 1.10f, R * 1.34f, deg, deg + 70.0f, 24,
             (Color){255, 235, 140, 220});
    DrawCircle((int)cx, (int)cy, (int)R, body);
    DrawCircle((int)cx, (int)cy, (int)(R * 0.55f), FlashCol((Color){90, 10, 25, 255}, flash));
    DrawTriangle((Vector2){cx - R * 0.12f, cy - R * 0.90f},
                 (Vector2){cx + R * 0.12f, cy - R * 0.90f},
                 (Vector2){cx, cy - R * 1.45f}, gold);
    Color horn = FlashCol((Color){120, 15, 30, 255}, flash);
    DrawTriangle((Vector2){cx - R * 0.70f, cy - R * 0.45f},
                 (Vector2){cx - R * 0.30f, cy - R * 0.65f},
                 (Vector2){cx - R * 1.05f, cy - R * 1.15f}, horn);
    DrawTriangle((Vector2){cx + R * 0.30f, cy - R * 0.65f},
                 (Vector2){cx + R * 0.70f, cy - R * 0.45f},
                 (Vector2){cx + R * 1.05f, cy - R * 1.15f}, horn);
    Color eye = (st == ART_DASH) ? (Color){255, 255, 255, 255}
              : (st == ART_TIRED) ? (Color){170, 140, 60, 255}
              : (st == ART_FIRE)  ? (Color){255, 160, 50, 255}
                                 : (Color){255, 255, 255, 255};
    DrawRectangle((int)(cx - R * 0.42f), (int)(cy - R * 0.18f), (int)(R * 0.22f), (int)(R * 0.10f), eye);
    DrawRectangle((int)(cx + R * 0.20f), (int)(cy - R * 0.18f), (int)(R * 0.22f), (int)(R * 0.10f), eye);
}

static void DrawSkin_InfernoHydra(float cx, float cy, float R, float t, ArtSt st, bool flash) {
    Color outer = FlashCol((st == ART_TIRED) ? (Color){40, 30, 28, 255} : (Color){220, 45, 12, 255}, flash);
    Color mid   = FlashCol((st == ART_TIRED) ? (Color){90, 50, 30, 255} : (Color){255, 130, 28, 255}, flash);
    Color core  = FlashCol((st == ART_TIRED) ? (Color){60, 40, 35, 255} : (Color){255, 235, 100, 255}, flash);
    float pp = 0.5f + 0.5f * sinf(t * 5.0f);
    if (st != ART_TIRED && !flash) {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle((int)cx, (int)cy, (int)(R * (1.35f + 0.10f * pp)),
                   (Color){255, 80, 10, (uchar)(18 + 12 * pp)});
        EndBlendMode();
    }
    if (st != ART_TIRED) {
        BeginBlendMode(BLEND_ADDITIVE);
        for (int k = 0; k < 10; k++) {
            float a = k * (2.0f * PI / 10.0f) + t * 1.5f;
            float flicker = 0.7f + 0.3f * sinf(t * 9.0f + k);
            float len = (st == ART_DASH) ? R * 1.65f : R * (1.20f + 0.22f * pp);
            DrawCircle((int)(cx + cosf(a) * len * 0.55f),
                       (int)(cy + sinf(a) * len * 0.55f),
                       (int)(R * 0.38f * flicker), (Color){255, 100, 20, 85});
        }
        EndBlendMode();
    }
    DrawCircle((int)cx, (int)cy, (int)(R * 1.05f), outer);
    DrawCircle((int)cx, (int)cy, (int)(R * 0.75f), mid);
    DrawCircle((int)cx, (int)cy, (int)(R * 0.40f), core);
    if (st != ART_TIRED && !flash) {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle((int)cx, (int)cy, (int)(R * (0.22f + 0.04f * pp)),
                   (Color){255, 255, 200, (uchar)(90 + 40 * pp)});
        EndBlendMode();
    }
    if (st != ART_TIRED) {
        for (int k = 0; k < 9; k++) {
            float p = fmodf(t * 0.6f + Brnd(k + 5), 1.0f);
            DrawCircleV((Vector2){cx - R * 0.5f + Brnd(k) * R, cy + R * 0.6f - p * R * 2.0f},
                        R * 0.05f, (Color){255, 180, 60, (uchar)(220 * sinf(p * PI))});
        }
    }
    Color eye = (st == ART_TIRED) ? (Color){80, 50, 30, 255}
              : (st == ART_DASH)  ? (Color){255, 255, 255, 255}
                                 : (Color){255, 245, 140, 255};
    float lock = (st == ART_WINDUP || st == ART_FIRE) ? 0.06f : 0.0f;
    float eyes[3][2] = { {-0.32f + lock, -0.08f}, {0.32f + lock, -0.08f}, {lock, -0.42f} };
    for (int i = 0; i < 3; i++) {
        float ex = cx + eyes[i][0] * R, ey = cy + eyes[i][1] * R;
        if (st != ART_TIRED && !flash) {
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircle((int)ex, (int)ey, (int)(R * 0.20f), (Color){255, 200, 50, 55});
            EndBlendMode();
        }
        DrawCircle((int)ex, (int)ey, (int)(R * (i == 2 ? 0.12f : 0.13f)), eye);
        DrawCircle((int)ex, (int)ey, (int)(R * 0.05f), (Color){40, 10, 0, 255});
    }
    if (st == ART_FIRE) {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle((int)cx, (int)(cy - R * 0.42f), (int)(R * 0.30f), (Color){255, 200, 50, 130});
        EndBlendMode();
    }
    if (st != ART_TIRED) {
        Color horn = flash ? (Color){255, 255, 255, 255} : (Color){255, 90, 20, 230};
        DrawTriangle((Vector2){cx - R * 0.55f, cy - R * 0.75f},
                     (Vector2){cx - R * 0.25f, cy - R * 0.85f},
                     (Vector2){cx - R * 0.50f, cy - R * 1.30f}, horn);
        DrawTriangle((Vector2){cx + R * 0.25f, cy - R * 0.85f},
                     (Vector2){cx + R * 0.55f, cy - R * 0.75f},
                     (Vector2){cx + R * 0.50f, cy - R * 1.30f}, horn);
        if (!flash) {
            DrawCircleV((Vector2){cx - R * 0.50f, cy - R * 1.28f}, R * 0.07f, (Color){255, 240, 160, 220});
            DrawCircleV((Vector2){cx + R * 0.50f, cy - R * 1.28f}, R * 0.07f, (Color){255, 240, 160, 220});
        }
    }
}

static void DrawSkin_ObsidianBrute(float cx, float cy, float R, float t, ArtSt st, bool flash) {
    Color body = FlashCol((st == ART_TIRED) ? (Color){36, 40, 48, 255} : (Color){18, 14, 26, 255}, flash);
    Color lava = (st == ART_TIRED) ? (Color){70, 85, 100, 200}
               : (st == ART_FIRE)  ? (Color){255, 200, 60, 255}
               : (st == ART_WINDUP)? (Color){255, 90, 20, 255}
                                  : (Color){255, 110, 25, 255};
    if (flash) lava = (Color){245, 235, 255, lava.a};
    float pulse = 0.5f + 0.5f * sinf(t * 3.2f);
    float fury  = (st == ART_WINDUP || st == ART_FIRE) ? 1.35f : 1.0f;
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircle((int)cx, (int)cy, (int)(R * (1.55f + 0.15f * pulse) * fury),
               (Color){255, 80, 10, (uchar)(28 + 20 * pulse)});
    if (st == ART_WINDUP || st == ART_DASH) {
        float ring = fmodf(t * 2.5f, 1.0f);
        DrawCircleLines((int)cx, (int)cy, (int)(R * (1.1f + ring * 0.9f)),
                        (Color){255, 140, 40, (uchar)(160 * (1.0f - ring))});
    }
    EndBlendMode();
    for (int k = 0; k < 7; k++) {
        float a = t * 1.1f + k * (2.0f * PI / 7.0f);
        float dist = R * (1.55f + 0.12f * sinf(t * 2.0f + k));
        float sx = cx + cosf(a) * dist, sy = cy + sinf(a) * dist * 0.75f;
        DrawTriangle((Vector2){sx, sy - R * 0.18f},
                     (Vector2){sx - R * 0.14f, sy + R * 0.12f},
                     (Vector2){sx + R * 0.14f, sy + R * 0.12f},
                     FlashCol((Color){45, 40, 55, 240}, flash));
        DrawCircleV((Vector2){sx, sy}, R * 0.05f, lava);
    }
    DrawCircle((int)(cx - R * 1.05f), (int)(cy + R * 0.20f), (int)(R * 0.72f), body);
    DrawCircle((int)(cx + R * 1.05f), (int)(cy + R * 0.20f), (int)(R * 0.72f), body);
    DrawCircle((int)cx, (int)(cy + R * 0.08f), (int)(R * 1.12f), body);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircle((int)cx, (int)cy, (int)(R * (0.55f + 0.08f * pulse)),
               (Color){lava.r, lava.g, lava.b, (uchar)(90 + 50 * pulse)});
    EndBlendMode();
    DrawCircle((int)cx, (int)cy, (int)(R * 0.95f), (Color){body.r, body.g, body.b, 210});
    for (int k = 0; k < 8; k++) {
        float a0 = k * 0.78f + t * 0.15f;
        float a1 = a0 + 0.55f + 0.2f * sinf(t + k);
        float thick = R * (0.07f + 0.05f * pulse) * fury;
        DrawLineEx((Vector2){cx + cosf(a0) * R * 0.12f, cy + sinf(a0) * R * 0.08f},
                   (Vector2){cx + cosf(a1) * R * 0.95f, cy + sinf(a1) * R * 0.95f}, thick, lava);
        float mid = 0.55f;
        float mx = cx + cosf(a0) * R * 0.12f * (1.0f - mid) + cosf(a1) * R * 0.95f * mid;
        float my = cy + sinf(a0) * R * 0.08f * (1.0f - mid) + sinf(a1) * R * 0.95f * mid;
        DrawLineEx((Vector2){mx, my},
                   (Vector2){mx + cosf(a1 + 0.9f) * R * 0.35f, my + sinf(a1 + 0.9f) * R * 0.35f},
                   thick * 0.6f, lava);
    }
    if (st != ART_TIRED) {
        for (int k = 0; k < 5; k++) {
            float p = fmodf(t * 0.7f + Brnd(k + 11), 1.0f);
            float dx = cx - R * 0.6f + Brnd(k) * R * 1.2f;
            DrawCircleV((Vector2){dx, cy + R * 0.5f + p * R * 0.9f},
                        R * (0.07f - p * 0.04f), (Color){255, 140, 30, (uchar)(220 * (1.0f - p))});
        }
    }
    for (int k = -3; k <= 3; k++) {
        float hx = cx + k * R * 0.22f;
        float hlen = R * (0.55f + 0.35f * (1.0f - fabsf((float)k) / 3.0f) + 0.1f * sinf(t * 3.0f + k));
        DrawTriangle((Vector2){hx - R * 0.10f, cy - R * 0.75f},
                     (Vector2){hx + R * 0.10f, cy - R * 0.75f},
                     (Vector2){hx, cy - R * 0.75f - hlen},
                     (k == 0) ? lava : FlashCol((Color){55, 50, 65, 255}, flash));
    }
    Color eye = (st == ART_TIRED) ? (Color){70, 80, 90, 255} : lava;
    for (int side = -1; side <= 1; side += 2) {
        float ex = cx + side * R * 0.32f, ey = cy - R * 0.08f;
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle((int)ex, (int)ey, (int)(R * 0.28f), (Color){lava.r, lava.g, lava.b, 70});
        EndBlendMode();
        DrawCircle((int)ex, (int)ey, (int)(R * 0.18f), eye);
        DrawCircle((int)ex, (int)ey, (int)(R * 0.07f), (Color){255, 255, 200, 255});
    }
    if (st == ART_FIRE || st == ART_WINDUP || st == ART_DASH) {
        float open = (st == ART_FIRE) ? 0.35f : 0.18f;
        DrawTriangle((Vector2){cx - R * 0.40f, cy + R * 0.20f},
                     (Vector2){cx + R * 0.40f, cy + R * 0.20f},
                     (Vector2){cx, cy + R * (0.55f + open)}, lava);
        DrawTriangle((Vector2){cx - R * 0.25f, cy + R * 0.25f},
                     (Vector2){cx + R * 0.25f, cy + R * 0.25f},
                     (Vector2){cx, cy + R * (0.40f + open)}, (Color){255, 240, 120, 230});
    }
}

static void DrawSkin_AshenWraith(float cx, float cy, float R, float t, ArtSt st, bool flash) {
    float alphaMul = (st == ART_TIRED) ? 0.40f : 1.0f;
    Color core = FlashCol((Color){28, 18, 40, (uchar)(230 * alphaMul)}, flash);
    Color smoke = FlashCol((Color){100, 90, 130, (uchar)(150 * alphaMul)}, flash);
    Color soul = (st == ART_FIRE) ? (Color){255, 150, 60, 255} : (Color){120, 255, 255, 255};
    if (flash) soul = (Color){245, 235, 255, 255};
    if (st == ART_DASH || st == ART_WINDUP) {
        for (int g = 3; g >= 1; g--) {
            float gx = cx - g * R * 0.40f, gy = cy - g * R * 0.08f;
            DrawCircle((int)gx, (int)gy, (int)(R * (0.85f - g * 0.08f)),
                       (Color){smoke.r, smoke.g, smoke.b, (uchar)(55 * alphaMul)});
        }
    }
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
    for (int k = 0; k < 10; k++) {
        float ox = sinf(t * 1.6f + k * 0.9f) * R * 0.28f;
        float oy = cosf(t * 1.3f + k * 0.7f) * R * 0.20f;
        float rr = R * (0.45f + (k % 4) * 0.14f);
        DrawCircle((int)(cx + ox), (int)(cy + oy), (int)rr,
                   (Color){smoke.r, smoke.g, smoke.b, (uchar)((35 + 14 * k) * alphaMul)});
    }
    DrawCircle((int)cx, (int)cy, (int)(R * 0.72f), core);
    DrawCircle((int)cx, (int)(cy - R * 0.05f), (int)(R * 0.42f),
               FlashCol((Color){55, 48, 70, (uchar)(200 * alphaMul)}, flash));
    DrawCircle((int)(cx - R * 0.14f), (int)(cy - R * 0.08f), (int)(R * 0.12f), core);
    DrawCircle((int)(cx + R * 0.14f), (int)(cy - R * 0.08f), (int)(R * 0.12f), core);
    DrawTriangle((Vector2){cx - R * 0.10f, cy + R * 0.12f},
                 (Vector2){cx + R * 0.10f, cy + R * 0.12f},
                 (Vector2){cx, cy + R * 0.28f}, core);
    for (int k = 0; k < 18; k++) {
        float p = fmodf(t * (0.5f + (k % 3) * 0.15f) + Brnd(k), 1.0f);
        float a = Brnd(k + 7) * 2.0f * PI + t * 0.8f;
        float dist = R * (0.4f + p * 1.5f);
        int soulP = (k % 4 == 0);
        DrawCircleV((Vector2){cx + cosf(a) * dist, cy + sinf(a) * dist * 0.75f - p * R * 0.4f},
                    R * (soulP ? 0.06f : 0.035f),
                    soulP ? (Color){soul.r, soul.g, soul.b, (uchar)(200 * sinf(p * PI) * alphaMul)}
                          : (Color){200, 195, 215, (uchar)(170 * sinf(p * PI) * alphaMul)});
    }
    for (int side = -1; side <= 1; side += 2) {
        float bx = cx + side * R * 0.30f, by = cy - R * 0.50f;
        for (int i = 0; i < 7; i++) {
            float u = i / 6.0f;
            float hx = bx + side * (u * R * 0.55f + sinf(t * 2.5f + i) * R * 0.14f);
            float hy = by - u * R * 1.25f;
            DrawCircleV((Vector2){hx, hy}, R * (0.22f - u * 0.14f),
                        FlashCol((Color){130, 120, 160, (uchar)(170 * alphaMul)}, flash));
            if (i == 4)
                DrawCircleV((Vector2){hx - side * R * 0.25f, hy - R * 0.20f},
                            R * 0.10f, FlashCol((Color){130, 120, 160, (uchar)(150 * alphaMul)}, flash));
        }
    }
    Color eye = (st == ART_TIRED) ? (Color){70, 90, 100, 180} : soul;
    for (int side = -1; side <= 1; side += 2) {
        float ex = cx + side * R * 0.22f, ey = cy - R * 0.10f;
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle((int)ex, (int)ey, (int)(R * 0.22f), (Color){eye.r, eye.g, eye.b, 80});
        EndBlendMode();
        DrawCircle((int)ex, (int)ey, (int)(R * 0.13f), eye);
        DrawCircle((int)ex, (int)ey, (int)(R * 0.05f), (Color){255, 255, 255, 240});
        DrawLineEx((Vector2){ex, ey + R * 0.12f},
                   (Vector2){ex + side * R * 0.05f, ey + R * 0.55f},
                   R * 0.04f, (Color){eye.r, eye.g, eye.b, (uchar)(140 * alphaMul)});
    }
}

static void DrawBossSkinAt(float cx, float cy, float R, float t,
                           int pick, ArtSt st, bool flash) {
    switch (pick) {
        case 1: DrawSkin_CrimsonHerald(cx, cy, R, t, st, flash); break;
        case 2: DrawSkin_InfernoHydra(cx, cy, R, t, st, flash);  break;
        case 3: DrawSkin_ObsidianBrute(cx, cy, R, t, st, flash); break;
        case 4: DrawSkin_AshenWraith(cx, cy, R, t, st, flash);   break;
        default: DrawSkin_ChaosWarden(cx, cy, R, t, st, flash);  break;
    }
}

static void DrawBossBody(float cx, float cy, float t, bool flash) {
    // Base radius matches the 2x2 footprint. Inferno Hydra + Ashen Wraith are
    // drawn larger for presence only — collision/HP footprint stays 2x2.
    float R = (float)(CELL - 4);
    if (bossTierIdx == 2 || bossTierIdx == 4) R *= 1.42f;
    DrawBossSkinAt(cx, cy, R, t, bossTierIdx, BossArtSt(), flash);
}

void DrawBossPreview(float cx, float cy, float radius, int pick, float t) {
    if (pick < 0) pick = 0;
    if (pick > 4) pick = 4;
    if (radius < 1.0f) radius = 1.0f;
    if (pick == 2 || pick == 4) radius *= 1.42f;
    // ART_STALK keeps the portrait calm while every detail comes from the same
    // skin function that renders the corresponding live boss.
    DrawBossSkinAt(cx, cy, radius, t, pick, ART_STALK, false);
}


// ─── ARENA CANNONS ────────────────────────────────────────────────────────────
// 'C' tiles; standing on one fires a ball at the boss, then the cannon reloads.
typedef struct { int x, y; float cd; } Cannon;
typedef struct { float x, y, vx, vy; bool on; } CBall;

#define MAX_CANNON 2
#define MAX_CBALL  4
static Cannon cans[MAX_CANNON];
static int    ncans;
static CBall  balls[MAX_CBALL];

// ─── TRAMPLE TRAPS ────────────────────────────────────────────────────────────
// FIFO of traps the boss has stomped into the floor. Capped per tier (see
// BossTier.trampleCap) so a long fight can't paper the whole arena in spikes —
// the oldest trap despawns to make room for the newest.
#define MAX_TRAMPLE 32
static int trapQx[MAX_TRAMPLE], trapQy[MAX_TRAMPLE];
static int trapQHead, trapQCount;

// ─── HELPERS ──────────────────────────────────────────────────────────────────
static float BossCX(void) { return (boss.x + 1) * (float)CELL; }  // footprint centre
static float BossCY(void) { return (boss.y + 1) * (float)CELL; }

bool BossCovers(int tx, int ty) {
    if (!boss.on) return false;
    return tx >= boss.x && tx <= boss.x+1 && ty >= boss.y && ty <= boss.y+1;
}

// Chebyshev distance from the player to the nearest footprint tile.
static int BossDistToPlayer(void) {
    int dx = (px < boss.x) ? boss.x - px : (px > boss.x+1) ? px - (boss.x+1) : 0;
    int dy = (py < boss.y) ? boss.y - py : (py > boss.y+1) ? py - (boss.y+1) : 0;
    return (dx > dy) ? dx : dy;
}

bool BossInRange(int rng) {
    if (!boss.on) return false;
    return BossDistToPlayer() <= rng;
}

// Can the 2x2 footprint sit with its top-left tile at (nx,ny)?
static bool CanOccupy(int nx, int ny) {
    if (nx < 1 || ny < 1 || nx+1 > mw-2 || ny+1 > mh-2) return false;
    for (int dy = 0; dy <= 1; dy++)
    for (int dx = 0; dx <= 1; dx++)
        if (map[ny+dy][nx+dx] == '#') return false;
    return true;
}

// Return the tile offset at a given major-axis step along the locked aim ray.
// The minor axis is rounded proportionally, producing an extendable Bresenham-
// style line that reaches the player's exact displacement rather than snapping
// to one of eight compass directions.
static void AimOffsetAt(int step, int *ox, int *oy) {
    int ax = abs(boss.dashdx), ay = abs(boss.dashdy);
    int sx = (boss.dashdx > 0) ? 1 : (boss.dashdx < 0) ? -1 : 0;
    int sy = (boss.dashdy > 0) ? 1 : (boss.dashdy < 0) ? -1 : 0;

    if (ax >= ay && ax > 0) {
        *ox = sx * step;
        *oy = sy * ((step * ay + ax/2) / ax);
    } else if (ay > 0) {
        *ox = sx * ((step * ax + ay/2) / ay);
        *oy = sy * step;
    } else {
        *ox = 0;
        *oy = 0;
    }
}

// Return the single-tile move made at this step of the locked aim ray.
static void AimStepAt(int step, int *sx, int *sy) {
    int ox0, oy0, ox1, oy1;
    AimOffsetAt(step - 1, &ox0, &oy0);
    AimOffsetAt(step,     &ox1, &oy1);
    *sx = ox1 - ox0;
    *sy = oy1 - oy0;
}

static void AimUnit(float *ux, float *uy) {
    float dx = (float)boss.dashdx, dy = (float)boss.dashdy;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 0.001f) { *ux = 0.0f; *uy = 1.0f; return; }
    *ux = dx / len;
    *uy = dy / len;
}

// Get a clear fire-ray tile. Callers walk steps in order and stop on false so
// the first wall blocks every tile behind it.
static bool FirePathTile(int step, int *tx, int *ty) {
    int ox, oy;
    AimOffsetAt(step, &ox, &oy);
    *tx = boss.x + boss.aimx + ox;
    *ty = boss.y + boss.aimy + oy;
    if (*tx < 0 || *ty < 0 || *tx >= mw || *ty >= mh) return false;
    return map[*ty][*tx] != '#';
}

static void RollNextCharge(void) {
    boss.chargeT = tier->chargeMin + (float)GetRandomValue(0, BOSS_CHARGE_VAR);
    boss.chain   = 0;
}

// ─── SPAWN ────────────────────────────────────────────────────────────────────
void SpawnBoss(void) {
    StatsBossStarted();
    int n   = roundn / 10;                       // boss number (1-based)
    int idx = n - 1;
    if (idx < 0) idx = 0;
    if (idx > 4) idx = 4;                        // cap: 13 HP from round 50 on

    boss = (Boss){0};
    boss.x  = mw/2 - 1;
    boss.y  = 2;
    boss.hp = boss.maxhp = BOSS_HP[idx];
    boss.state = BS_STALK;
    boss.on = true;
    tier = &TIERS[idx];
    bossTierIdx = idx;
    RollNextCharge();
    trapQHead = 0; trapQCount = 0;
    PlaySfx(SFX_BOSS_SPAWN);

    // Register the arena cannons placed by GenBossArena. They start
    // unloaded (on cooldown) — the fight has to earn its first shot.
    ncans = 0;
    for (int y = 1; y < mh-1; y++)
    for (int x = 1; x < mw-1; x++)
        if (map[y][x] == 'C' && ncans < MAX_CANNON)
            cans[ncans++] = (Cannon){x, y, CANNON_CD};
    for (int i = 0; i < MAX_CBALL; i++) balls[i].on = false;
}

// ─── DEATH / REWARD ───────────────────────────────────────────────────────────
static void BossDie(void) {
    StatsBossDefeated();
    boss.on = false;
    StatsInc(&global_bosses_killed);
    // Unbeaten Champion: rank 2 = L10 (tier 0) … rank 6 = L50 (tier 4)
    {
        int rank = 2 + bossTierIdx;
        if (rank > 6) rank = 6;
        if (StatsEnabled() && rank > hs_unbeaten) hs_unbeaten = rank;
    }
    SaveStats();

    float cx = BossCX(), cy = BossCY();
    Burst(cx, cy, CBOSS, 48, 6);
    Burst(cx, cy, CCOIN, 32, 5);
    PlaySfx(SFX_BOSS_DEFEAT);
    AddScreenShake(16.0f, 0.40f);

    // Coin shower reward: 10 + 5 per boss number (x4 in Coin Rush).
    int reward = 10 + 5 * (roundn / 10);
    if (coinRush) reward *= 4;
    totcoi += reward; StatsAdd(&global_totcoi, reward);
    StatsCoinEarned(STATS_COIN_BOSS,reward);
    coin_balance += reward; coins_round += reward;

    // Despawn remaining adds (no kill credit — the fight is over).
    for (int i = 0; i < MAX_ENE; i++) {
        if (!enes[i].on) continue;
        enes[i].on = false;
        Burst(WX((int)(enes[i].x+0.5f)), WY((int)(enes[i].y+0.5f)), CENE, 8, 3);
    }
    boss.telen = 0; boss.teleT = 0;
    for (int i = 0; i < MAX_CBALL; i++) balls[i].on = false;

    // The treasure drops near the arena centre (spiral search for floor).
    int gx = mw/2, gy = mh/2;
    for (int r = 0; r < mw+mh; r++) {
        for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++) {
            if (abs(dx) != r && abs(dy) != r) continue;   // ring only
            int tx = gx+dx, ty = gy+dy;
            if (tx < 1 || ty < 1 || tx > mw-2 || ty > mh-2) continue;
            if (map[ty][tx] != '.' || (tx == px && ty == py)) continue;
            map[ty][tx] = 'T';
            Burst(WX(tx), WY(ty), CTRES, 28, 5);
            return;
        }
    }
}

// ─── DAMAGE / KNOCKBACK ───────────────────────────────────────────────────────
static void BossKnockback(int tiles) {
    int ddx = (px <= boss.x) ? 1 : (px >= boss.x+1) ? -1 : 0;
    int ddy = (py <= boss.y) ? 1 : (py >= boss.y+1) ? -1 : 0;
    if (!ddx && !ddy) ddy = -1;
    for (int s = 0; s < tiles; s++) {
        if      (CanOccupy(boss.x+ddx, boss.y+ddy)) { boss.x += ddx; boss.y += ddy; }
        else if (ddx && CanOccupy(boss.x+ddx, boss.y)) boss.x += ddx;
        else if (ddy && CanOccupy(boss.x, boss.y+ddy)) boss.y += ddy;
        else break;
    }
}

static void BossRetreatTeleport(void) {
    float ox = BossCX(), oy = BossCY();
    for (int t = 0; t < 80; t++) {
        int nx = GetRandomValue(1, mw-3), ny = GetRandomValue(1, mh-3);
        int dx = abs(nx+1 - px) > abs(nx - px) ? abs(nx - px) : abs(nx+1 - px);
        int dy = abs(ny+1 - py) > abs(ny - py) ? abs(ny - py) : abs(ny+1 - py);
        int d  = (dx > dy) ? dx : dy;
        if (d < 4 || d > 7) continue;
        if (!CanOccupy(nx, ny)) continue;
        if (px >= nx && px <= nx+1 && py >= ny && py <= ny+1) continue;
        boss.x = nx; boss.y = ny;
        Burst(ox, oy, CBOSS, 20, 5);
        Burst(BossCX(), BossCY(), CBOSS, 20, 5);
        return;
    }
    BossKnockback(3);   // cramped arena fallback
}

bool HitBoss(int dmg, int knockTiles, bool teleport) {
    if (!boss.on || boss.inv > 0) return false;
    boss.hp -= dmg;
    StatsAdd(&global_boss_dmg, dmg);
    boss.inv = BOSS_INV_TIME;
    Burst(BossCX(), BossCY(), CBOSS, 26, 5);
    Burst(BossCX(), BossCY(), (Color){255,255,255,255}, 10, 4);
    PlaySfx(SFX_BOSS_DAMAGE);
    AddScreenShake(7.0f, 0.16f);
    if (boss.hp <= 0) { BossDie(); return true; }
    if (teleport) {
        BossRetreatTeleport();
        boss.state = BS_STALK;      // a sword hit interrupts any charge
        boss.mt = 0;
        RollNextCharge();
    } else if (knockTiles) {
        BossKnockback(knockTiles);
    }
    return true;
}

// ─── PER-FRAME UPDATE ─────────────────────────────────────────────────────────
// Boss touches the player: shield saves (and hurts the boss), otherwise death.
static void BossContact(void) {
    if (pshield || pshield_perk) {
        if (pshield) pshield = false;
        else pshield_perk = false;
        Burst(WX(px), WY(py), CSHD, 24, 5);
        PlaySfx(SFX_SHIELD_TRIGGER);
        HitBoss(1, 2, false);
    } else {
        Burst(WX(px), WY(py), CPD, 32, 5);
        PlaySfx(SFX_DEATH_ENEMY);
        AddScreenShake(14.0f, 0.30f);
        StatsInc(&global_deaths_boss);
        pdead = true; gs = GS_DEAD; sttime = 0;
        UpdateAndSaveStats();
    }
}

// Does the calculated fire ray cover tile (tx,ty)? The ray follows the exact
// player-relative ratio locked during windup and stops at the first wall.
static bool FireCoversTile(int tx, int ty) {
    for (int step = 1; step <= tier->fireRange; step++) {
        int fx, fy;
        if (!FirePathTile(step, &fx, &fy)) break;
        if (fx == tx && fy == ty) return true;
    }
    return false;
}

// Fire breath touches the player: shield saves (no boss damage — it's a
// ranged attack, not a punishable melee opening), otherwise death. A save
// snuffs the breath out early — the beam doesn't move, so without this the
// player would still be standing in it next frame with no shield left,
// dying a split second after being "saved."
static void BossFireHit(void) {
    if (pshield || pshield_perk) {
        if (pshield) pshield = false;
        else pshield_perk = false;
        Burst(WX(px), WY(py), CSHD, 24, 5);
        PlaySfx(SFX_SHIELD_TRIGGER);
        boss.state = BS_TIRED;
        boss.stT   = BOSS_TIRED_TIME;
    } else {
        Burst(WX(px), WY(py), CFIRE, 32, 5);
        PlaySfx(SFX_DEATH_ENEMY);
        AddScreenShake(14.0f, 0.30f);
        StatsInc(&global_deaths_boss);
        pdead = true; gs = GS_DEAD; sttime = 0;
        UpdateAndSaveStats();
    }
}

// Armed spike plates '*' under the footprint hurt the boss (unless i-framed).
// Un-armed plates '^' are harmless until the player steps on them (TryMove).
static void CheckPlates(void) {
    for (int dy = 0; dy <= 1; dy++)
    for (int dx = 0; dx <= 1; dx++) {
        int tx = boss.x+dx, ty = boss.y+dy;
        if (map[ty][tx] != '*') continue;
        if (boss.inv > 0) continue;         // plate survives, triggers later
        map[ty][tx] = '.';
        Burst(WX(tx), WY(ty), CPLATE, 22, 5);
        HitBoss(1, 0, false);
        return;
    }
}

// Summon adds at the arena edge after a particle telegraph. The simultaneous
// cap is always based on full boss HP, so damaging the boss cannot lower it.
static void UpdAdds(float dt) {
    if (!eneon) return;                     // adds respect the enemy toggle

    int addCap = boss.maxhp * BOSS_ADDS_PER_HP;
    if (addCap > BOSS_ADD_CAP) addCap = BOSS_ADD_CAP;
    if (addCap > MAX_ENE) addCap = MAX_ENE;
    int alive = 0;
    for (int e = 0; e < MAX_ENE; e++) if (enes[e].on) alive++;
    if (alive >= addCap) {
        boss.addT = 0;
        boss.teleT = 0;
        boss.telen = 0;
        return;
    }

    if (boss.telen > 0) {                   // telegraph running
        boss.teleT -= dt;
        for (int i = 0; i < boss.telen; i++) {
            SpawnPar(WX(boss.telex[i]) + GetRandomValue(-8,8),
                     WY(boss.teley[i]) + GetRandomValue(-4,10),
                     CENE, 0, -40.0f - GetRandomValue(0,30), 0.4f, 2.5f);
        }
        if (boss.teleT <= 0) {
            for (int i = 0; i < boss.telen && alive < addCap; i++) {
                for (int e = 0; e < MAX_ENE; e++) {
                    if (enes[e].on) continue;
                    enes[e] = (Ene){(float)boss.telex[i], (float)boss.teley[i],
                                    0, 0.65f + GetRandomValue(0,35)*0.01f, true};
                    alive++;
                    Burst(WX(boss.telex[i]), WY(boss.teley[i]), CENE, 16, 4);
                    break;
                }
            }
            boss.telen = 0;
        }
        return;
    }

    boss.addT += dt;
    if (boss.addT < tier->addIntvl) return;
    boss.addT = 0;

    int want = 1 + GetRandomValue(0, tier->addWant - 1);
    if (want > addCap-alive) want = addCap-alive;
    for (int t = 0; t < 120 && boss.telen < want; t++) {
        // Floor ring just inside the arena walls, away from the player.
        int tx, ty;
        if (GetRandomValue(0,1)) { tx = GetRandomValue(1, mw-2); ty = GetRandomValue(0,1) ? 1 : mh-2; }
        else                     { ty = GetRandomValue(1, mh-2); tx = GetRandomValue(0,1) ? 1 : mw-2; }
        if (IsHiddenRoomTile(tx,ty) || map[ty][tx] != '.' || BossCovers(tx, ty)) continue;
        if (abs(tx-px) + abs(ty-py) <= 5) continue;
        boss.telex[boss.telen] = tx;
        boss.teley[boss.telen] = ty;
        boss.telen++;
    }
    if (boss.telen > 0) { boss.teleT = BOSS_ADD_TELE; PlaySfx(SFX_BOSS_ADDS); }
}

// Cannons fire when the player stands on them; balls fly straight at the boss.
static void UpdCannons(float dt) {
    for (int i = 0; i < ncans; i++) {
        if (cans[i].cd > 0) cans[i].cd -= dt;
        if (!boss.on || cans[i].cd > 0) continue;
        if (px != cans[i].x || py != cans[i].y) continue;

        for (int b = 0; b < MAX_CBALL; b++) {
            if (balls[b].on) continue;
            float sx = WX(cans[i].x), sy = WY(cans[i].y);
            float dx = BossCX() - sx, dy = BossCY() - sy;
            float len = sqrtf(dx*dx + dy*dy);
            if (len < 1.0f) len = 1.0f;
            balls[b] = (CBall){sx, sy, dx/len*CANNON_BALL_SPD, dy/len*CANNON_BALL_SPD, true};
            cans[i].cd = CANNON_CD;
            Burst(sx, sy, CCANN, 14, 4);
            PlaySfx(SFX_BOMB);
            break;
        }
    }

    for (int b = 0; b < MAX_CBALL; b++) {
        if (!balls[b].on) continue;
        if (!boss.on) { balls[b].on = false; continue; }
        // Straight-line shot — no homing. That's the thrill: read where the
        // boss is heading and time the trigger, don't just stand and win.
        balls[b].x += balls[b].vx * dt;
        balls[b].y += balls[b].vy * dt;
        SpawnPar(balls[b].x, balls[b].y, (Color){255,210,120,255}, 0, 0, 0.18f, 2.0f);

        float ddx = balls[b].x - BossCX(), ddy = balls[b].y - BossCY();
        if (ddx*ddx + ddy*ddy < (float)(CELL*CELL)) {
            balls[b].on = false;
            Burst(balls[b].x, balls[b].y, CCANN, 18, 5);
            // Count only successful hits (i-frames make HitBoss return false).
            if (HitBoss(1, 0, false))
                StatsInc(&global_cannon_hits);
            continue;
        }
        if (balls[b].x < 0 || balls[b].y < 0 ||
            balls[b].x > mw*CELL || balls[b].y > mh*CELL)
            balls[b].on = false;
    }
}

// Stomp a trample trap into (tx,ty), evicting the oldest one first if the
// tier's cap is already full.
static void PlaceTrampleTrap(int tx, int ty) {
    if (trapQCount >= tier->trampleCap) {
        int ox2 = trapQx[trapQHead], oy2 = trapQy[trapQHead];
        if (map[oy2][ox2] == '+') map[oy2][ox2] = '.';
        trapQHead = (trapQHead + 1) % MAX_TRAMPLE;
        trapQCount--;
    }
    map[ty][tx] = '+';
    Burst(WX(tx), WY(ty), CTRAP, 6, 2);
    int tail = (trapQHead + trapQCount) % MAX_TRAMPLE;
    trapQx[tail] = tx; trapQy[tail] = ty;
    trapQCount++;
}

// One slow stalking step toward the player (greedy, like normal enemies).
static void StalkStep(void) {
    int ddx = (px > boss.x+1) ? 1 : (px < boss.x) ? -1 : 0;
    int ddy = (py > boss.y+1) ? 1 : (py < boss.y) ? -1 : 0;
    bool yFirst = (GetRandomValue(0,3) == 0);
    int ox = boss.x, oy = boss.y;
    bool moved = false;

    if (!yFirst && ddx && CanOccupy(boss.x+ddx, boss.y)) { boss.x += ddx; moved = true; }
    if (!moved  && ddy && CanOccupy(boss.x, boss.y+ddy)) { boss.y += ddy; moved = true; }
    if (!moved  && yFirst && ddx && CanOccupy(boss.x+ddx, boss.y)) { boss.x += ddx; moved = true; }
    if (!moved) return;

    // Trample: every few steps a trap is stomped into a vacated tile.
    boss.steps++;
    if (boss.steps >= BOSS_TRAMPLE_STEPS) {
        boss.steps = 0;
        int vx[2], vy[2];
        if (boss.x != ox) { int col = (boss.x > ox) ? ox : ox+1;
                            vx[0]=col; vy[0]=oy; vx[1]=col; vy[1]=oy+1; }
        else              { int row = (boss.y > oy) ? oy : oy+1;
                            vx[0]=ox; vy[0]=row; vx[1]=ox+1; vy[1]=row; }
        int pick = GetRandomValue(0,1);
        for (int i = 0; i < 2; i++) {
            int tx = vx[(pick+i)%2], ty = vy[(pick+i)%2];
            if (map[ty][tx] == '.' && !(tx == px && ty == py)) {
                PlaceTrampleTrap(tx, ty);
                break;
            }
        }
    }
    CheckPlates();
}

// Calculate and lock the precise player-relative attack vector. The origin is
// the nearest tile of the boss's 2x2 footprint, so following the complete vector
// reaches the player's tile exactly. No fixed angle or ratio is selected.
static void LockPlayerAim(void) {
    int anchorx = (px < boss.x) ? boss.x : (px > boss.x+1) ? boss.x+1 : px;
    int anchory = (py < boss.y) ? boss.y : (py > boss.y+1) ? boss.y+1 : py;
    boss.aimx = anchorx - boss.x;
    boss.aimy = anchory - boss.y;
    boss.dashdx = px - anchorx;
    boss.dashdy = py - anchory;
    boss.dashn  = 0;

    // Contact normally resolves before an attack can be aimed from inside the
    // footprint. Keep a safe deterministic fallback for that edge case.
    if (!boss.dashdx && !boss.dashdy) {
        boss.aimy = 1;
        boss.dashdy = 1;
    }
}

// A dash just ended (wall crash, or the player contact was survived). Tier 3+
// chains ONE re-aimed second dash per charge before finally tiring out.
static void EndDash(void) {
    if (tier->doubleDash && boss.chain == 0) {
        boss.chain = 1;
        LockPlayerAim();
        boss.state = BS_WINDUP;
        boss.stT   = 0.6f;              // short re-aim, not a full windup
        PlaySfx(SFX_BOSS_AIM);
    } else {
        boss.state = BS_TIRED;
        boss.stT   = BOSS_TIRED_TIME;
    }
}

void UpdBoss(float dt) {
    if (roomKind != RK_BOSS) return;

    UpdCannons(dt);
    if (!boss.on) return;

    if (boss.inv > 0) boss.inv -= dt;
    UpdAdds(dt);
    CheckPlates();
    if (!boss.on || gs != GS_PLAY) return;   // plates/balls may have ended it

    switch (boss.state) {

    case BS_STALK:
        boss.chargeT -= dt;
        if (boss.chargeT <= 0) {
            boss.mt = 0;
            // Far away: breathe fire (tier 2+ only — the intro boss dashes
            // from any range). Close: melee dash. Both calculate and lock the
            // player's exact current displacement, with unrestricted slopes.
            // Both attacks share the same "aim" telegraph cue; the attack's
            // own sound lands when it actually fires.
            if (tier->hasFire && BossDistToPlayer() >= BOSS_FIRE_MIN_DIST) {
                LockPlayerAim();
                boss.state = BS_FIRE_WINDUP;
                boss.stT   = BOSS_FIRE_WINDUP_TIME;
            } else {
                LockPlayerAim();
                boss.state = BS_WINDUP;
                boss.stT   = BOSS_WINDUP_TIME;
            }
            PlaySfx(SFX_BOSS_AIM);
            break;
        }
        boss.mt += dt;
        if (boss.mt >= tier->stalkCd) {
            boss.mt = 0;
            StalkStep();
            if (!boss.on) return;
            if (BossCovers(px, py)) {
                BossContact();           // stalked onto the player's tile
                if (gs != GS_PLAY) return;
            }
        }
        break;

    case BS_WINDUP:
        boss.stT -= dt;
        SpawnPar(BossCX() + GetRandomValue(-24,24),
                 BossCY() + GetRandomValue(-24,24),
                 (Color){235,70,70,255}, 0, -30, 0.3f, 2.5f);
        if (boss.stT <= 0) {
            boss.state = BS_DASH;
            boss.mt = 0;
            boss.dashn = 0;
            PlaySfx(SFX_BOSS_CHARGE);
            AddScreenShake(6.0f, 0.18f);
        }
        break;

    case BS_FIRE_WINDUP: {
        boss.stT -= dt;
        float ux, uy;
        AimUnit(&ux, &uy);
        float mx = BossCX() + ux * (float)CELL;
        float my = BossCY() + uy * (float)CELL;
        SpawnPar(mx + GetRandomValue(-6,6), my + GetRandomValue(-6,6),
                 CFIRE, -ux*10.0f, -uy*10.0f, 0.35f, 3.0f);
        if (boss.stT <= 0) { boss.state = BS_FIRE; boss.stT = BOSS_FIRE_DURATION; PlaySfx(SFX_BOSS_FIRE); }
        break;
    }

    case BS_FIRE: {
        boss.stT -= dt;
        float ux, uy;
        AimUnit(&ux, &uy);
        for (int i = 0; i < 5; i++) {
            int fx, fy;
            int step = GetRandomValue(1, tier->fireRange);
            if (!FirePathTile(step, &fx, &fy)) continue;
            float side = (float)GetRandomValue(-35,35);
            float speed = (float)GetRandomValue(25,70);
            SpawnPar(WX(fx) + GetRandomValue(-10,10),
                     WY(fy) + GetRandomValue(-10,10),
                     (Color){255, (uchar)GetRandomValue(125,230),
                             (uchar)GetRandomValue(15,70), 255},
                     ux*speed - uy*side, uy*speed + ux*side,
                     GetRandomValue(18,36)*0.01f, GetRandomValue(35,60)*0.1f);
        }
        if (FireCoversTile(px, py)) {
            BossFireHit();
            if (gs != GS_PLAY) return;
        }
        if (boss.stT <= 0) { boss.state = BS_TIRED; boss.stT = BOSS_TIRED_TIME; }
        break;
    }

    case BS_DASH:
        boss.mt += dt;
        while (boss.mt >= tier->dashStep && boss.state == BS_DASH) {
            boss.mt -= tier->dashStep;
            int stepx, stepy;
            AimStepAt(boss.dashn + 1, &stepx, &stepy);
            if (!CanOccupy(boss.x + stepx, boss.y + stepy)) {
                Burst(BossCX() + stepx*CELL, BossCY() + stepy*CELL,
                      (Color){200,200,220,255}, 20, 5);   // wall impact
                EndDash();
                break;
            }
            boss.x += stepx;
            boss.y += stepy;
            boss.dashn++;
            SpawnPar(BossCX(), BossCY(), CBOSS, 0, 0, 0.25f, 4.0f);
            CheckPlates();
            if (!boss.on) return;
            if (BossCovers(px, py)) {
                BossContact();               // hit the player mid-dash
                if (gs != GS_PLAY) return;
                if (!boss.on) return;        // shield hit may have killed it
                EndDash();
                break;
            }
        }
        break;

    case BS_TIRED:
        boss.stT -= dt;
        if (boss.stT <= 0) {
            boss.state = BS_STALK;
            boss.mt = 0;
            RollNextCharge();
        }
        break;
    }
    // Note: no generic trailing contact check here. Every state that can
    // make the boss newly overlap the player (STALK, DASH) already checks
    // right after it moves; a blanket re-check at the end used to fire a
    // second, now-unshielded hit in the SAME frame whenever a shield-saved
    // knockback failed to fully clear the overlap (e.g. against a wall) —
    // the shield would flash but the player died anyway.
}

// ─── DRAWING ──────────────────────────────────────────────────────────────────
static void DrawCannons(float t) {
    for (int i = 0; i < ncans; i++) {
        float cx = WX(cans[i].x), cy = WY(cans[i].y);
        bool ready = (cans[i].cd <= 0);

        // Always-on halo so the cannon reads from across the arena even
        // mid-reload — with a 30s cooldown, each one is a landmark to plan
        // around, not something you stumble onto.
        float hp2 = 0.5f + 0.5f*sinf(t*1.6f + i*2.0f);
        DrawCircle((int)cx, (int)cy, (int)(22 + 4*hp2),
                   (Color){CCANN.r, CCANN.g, CCANN.b, (uchar)(20 + 14*hp2)});

        // Barrel aims at the boss (points up once the fight is over).
        float ang = boss.on ? atan2f(BossCY()-cy, BossCX()-cx) : -1.5708f;
        Vector2 tip = { cx + cosf(ang)*17.0f, cy + sinf(ang)*17.0f };
        DrawLineEx((Vector2){cx, cy}, tip, 8.0f, (Color){70,62,58,255});
        DrawLineEx((Vector2){cx, cy}, tip, 4.0f, (Color){130,112,96,255});

        DrawCircle((int)cx, (int)cy, 12, (Color){58,52,50,255});
        DrawCircleLines((int)cx, (int)cy, 12, CCANN);

        if (ready) {
            float pp = 0.5f + 0.5f*sinf(t*4.0f + i*2.0f);
            DrawCircle((int)tip.x, (int)tip.y, (int)(7 + 2*pp),
                       (Color){CCANN.r, CCANN.g, CCANN.b, (uchar)(90 + 70*pp)});
            DrawCircle((int)tip.x, (int)tip.y, 3,
                       (Color){255, 245, 185, (uchar)(190 + 65*pp)});
            DrawCircle((int)cx, (int)cy, 6, CCANN);
            DrawCircleLines((int)cx, (int)cy, (int)(15 + 3*pp),
                            (Color){CCANN.r, CCANN.g, CCANN.b, (uchar)(180+70*pp)});
            DrawCircleLines((int)cx, (int)cy, (int)(20 + 4*pp),
                            (Color){CCANN.r, CCANN.g, CCANN.b, (uchar)(70+40*pp)});
            const char *rdy = "READY";
            DrawText(rdy, (int)(cx - MeasureText(rdy,10)/2), (int)(cy-32), 10,
                     (Color){CCANN.r, CCANN.g, CCANN.b, (uchar)(160+80*pp)});
        } else {
            // Reload progress ring — long cooldown, so this stays visible
            // and legible for most of the fight.
            float prog = 1.0f - cans[i].cd / CANNON_CD;
            DrawRing((Vector2){cx, cy}, 13, 17, -90, -90 + 360*prog, 32,
                     (Color){CCANN.r, CCANN.g, CCANN.b, 210});
            DrawCircle((int)cx, (int)cy, 6, (Color){110,95,72,255});
            DrawText(TextFormat("%.0fs", cans[i].cd),
                     (int)(cx - MeasureText(TextFormat("%.0fs", cans[i].cd),10)/2),
                     (int)(cy-32), 10, (Color){CCANN.r, CCANN.g, CCANN.b, 190});
        }
    }

    for (int b = 0; b < MAX_CBALL; b++) {
        if (!balls[b].on) continue;
        DrawCircle((int)balls[b].x, (int)balls[b].y, 7,
                   (Color){CCANN.r, CCANN.g, CCANN.b, 90});
        DrawCircle((int)balls[b].x, (int)balls[b].y, 5, (Color){40,36,34,255});
        DrawCircle((int)balls[b].x-1, (int)balls[b].y-2, 2, (Color){140,130,120,255});
    }
}

void DrawBoss(float t) {
    if (roomKind != RK_BOSS) return;

    DrawCannons(t);
    if (!boss.on) return;

    float cx = BossCX(), cy = BossCY();
    bool flash = !photosensitivity && (boss.inv > 0) && (fmodf(t*10.0f, 2.0f) < 1.0f);

    // Windup: a clean airborne red line follows the calculated charge path.
    // It communicates direction without painting the boss footprint on tiles.
    if (boss.state == BS_WINDUP) {
        float wp = photosensitivity ? 0.45f : (0.5f + 0.5f*sinf(t*12.0f));
        Vector2 start = { BossCX(), BossCY() };
        Vector2 end = start;
        int clearSteps = 0;
        for (int step = 1; step < mw+mh; step++) {
            int ox, oy;
            AimOffsetAt(step, &ox, &oy);
            int tx = boss.x + ox, ty = boss.y + oy;
            if (!CanOccupy(tx, ty)) break;
            end = (Vector2){ (tx+1)*(float)CELL, (ty+1)*(float)CELL };
            clearSteps = step;
        }
        if (clearSteps > 0) {
            BeginBlendMode(BLEND_ADDITIVE);
            DrawLineEx(start, end, CELL*(1.20f + 0.10f*wp),
                       (Color){210,5,20,(uchar)(32 + 25*wp)});
            DrawLineEx(start, end, CELL*(0.56f + 0.10f*wp),
                       (Color){255,25,35,(uchar)(175 + 65*wp)});
            EndBlendMode();
        }
        cx += sinf(t*45.0f) * 2.5f;

        const char *warn = "!!";
        DrawText(warn, (int)(cx - MeasureText(warn, 20)/2), (int)(cy - CELL - 24),
                 20, (Color){255,220,60,255});
    }

    // Dash: motion streaks behind the body.
    if (boss.state == BS_DASH) {
        int stepx, stepy;
        AimStepAt((boss.dashn > 0) ? boss.dashn : 1, &stepx, &stepy);
        float trailR = (float)(CELL - 4);
        if (bossTierIdx == 2 || bossTierIdx == 4) trailR *= 1.42f;
        for (int s = 1; s <= 3; s++)
            DrawCircle((int)(cx - stepx*CELL*0.45f*s),
                       (int)(cy - stepy*CELL*0.45f*s),
                       (int)(trailR - s*5), (Color){BossAccent().r, BossAccent().g, BossAccent().b, (uchar)(90 - s*25)});
    }

    // Fire windup: a wider chain of airborne fire bubbles. Nothing is painted
    // onto the floor tiles; the exact ray still drives hit detection.
    if (boss.state == BS_FIRE_WINDUP) {
        float wp = 0.5f + 0.5f*sinf(t*14.0f);
        Vector2 start = { BossCX(), BossCY() };
        Vector2 end = start;
        int clearSteps = 0;
        for (int step = 1; step <= tier->fireRange; step++) {
            int tx, ty;
            if (!FirePathTile(step, &tx, &ty)) break;
            end = (Vector2){ WX(tx), WY(ty) };
            clearSteps = step;
        }
        if (clearSteps > 0) {
            float dx = end.x-start.x, dy = end.y-start.y;
            float len = sqrtf(dx*dx + dy*dy);
            float nx = (len > 0.1f) ? -dy/len : 0.0f;
            float ny = (len > 0.1f) ?  dx/len : 0.0f;
            int bubbles = clearSteps*2 + 1;
            if (!photosensitivity) BeginBlendMode(BLEND_ADDITIVE);
            for (int i = 0; i < bubbles; i++) {
                float q = (bubbles > 1) ? i/(float)(bubbles-1) : 0.0f;
                float phase = photosensitivity ? 0.4f : (0.5f + 0.5f*sinf(t*9.0f + i*1.35f));
                float wobble = photosensitivity ? 0.0f : (sinf(t*5.0f + i*1.8f) * 3.5f);
                float bx = start.x + dx*q + nx*wobble;
                float by = start.y + dy*q + ny*wobble;
                float radius = 6.0f + 3.0f*phase + 2.0f*wp;
                DrawCircle((int)bx, (int)by, (int)(radius+4.0f),
                           (Color){255,65,5,(uchar)(30 + 30*phase)});
                DrawCircle((int)bx, (int)by, (int)radius,
                           (Color){255,140,20,(uchar)(100 + 55*phase)});
                DrawCircleLines((int)bx, (int)by, radius,
                                (Color){255,225,105,(uchar)(175 + 65*phase)});
                DrawCircle((int)(bx-radius*0.25f), (int)(by-radius*0.25f),
                           (int)(2.0f + phase), (Color){255,255,220,220});
            }
            if (!photosensitivity) EndBlendMode();
        }
        if (!photosensitivity) cx += sinf(t*50.0f) * 2.0f;

        const char *warn = "~FIRE~";
        DrawText(warn, (int)(cx - MeasureText(warn, 18)/2), (int)(cy - CELL - 24),
                 18, (Color){255,150,40,255});
    }

    // Fire breath: layered additive glow, a white-hot core, and animated flame
    // lobes make the freely angled beam read as fire rather than a flat laser.
    if (boss.state == BS_FIRE) {
        Vector2 start = { BossCX(), BossCY() };
        Vector2 end = start;
        int clearSteps = 0;
        for (int step = 1; step <= tier->fireRange; step++) {
            int tx, ty;
            if (!FirePathTile(step, &tx, &ty)) break;
            end = (Vector2){ WX(tx), WY(ty) };
            clearSteps = step;
        }
        if (clearSteps > 0) {
            float pulse = photosensitivity ? 0.35f : (0.5f + 0.5f*sinf(t*22.0f));
            float dx = end.x-start.x, dy = end.y-start.y;
            float len = sqrtf(dx*dx + dy*dy);
            float nx = (len > 0.1f) ? -dy/len : 0.0f;
            float ny = (len > 0.1f) ?  dx/len : 0.0f;

            if (!photosensitivity) BeginBlendMode(BLEND_ADDITIVE);
            DrawLineEx(start, end, 62.0f + 10.0f*pulse,
                       (Color){255,45,5,(uchar)(24 + 18*pulse)});
            DrawLineEx(start, end, 46.0f + 8.0f*pulse,
                       (Color){255,80,8,(uchar)(65 + 35*pulse)});
            DrawLineEx(start, end, 33.0f + 6.0f*pulse,
                       (Color){255,145,20,(uchar)(135 + 45*pulse)});

            for (int i = 0; i < 14; i++) {
                float q = fmodf(t*1.9f + i/14.0f, 1.0f);
                float flicker = photosensitivity ? 0.35f
                    : (0.5f + 0.5f*sinf(t*(13.0f + (i%3)*2.0f) + i*1.8f));
                float side = photosensitivity ? 0.0f
                    : (sinf(t*11.0f + i*2.1f) * (5.0f + 6.0f*flicker));
                float fx = start.x + dx*q + nx*side;
                float fy = start.y + dy*q + ny*side;
                float radius = 11.0f + 8.0f*flicker - 3.0f*q;
                Color flame = (i%3 == 0)
                            ? (Color){255,75,8,105}
                            : (Color){255,165,25,135};
                DrawCircle((int)fx, (int)fy, (int)radius, flame);
            }

            DrawLineEx(start, end, 18.0f + 5.0f*pulse,
                       (Color){255,220,75,225});
            DrawLineEx(start, end, 8.0f + 2.5f*pulse,
                       (Color){255,255,220,245});
            DrawCircle((int)start.x, (int)start.y, (int)(27 + 6*pulse),
                       (Color){255,150,30,180});
            DrawCircle((int)start.x, (int)start.y, (int)(14 + 4*pulse),
                       (Color){255,250,180,240});
            if (!photosensitivity) EndBlendMode();
        }
    }

    // Skin body art for this fight's boss pick (tier 1..5).
    DrawBossBody(cx, cy, t, flash);
}

void DrawBossBar(void) {
    if (roomKind != RK_BOSS || !boss.on) return;

    int sw = GetScreenWidth();
    int bw = 380, bh = 20;
    int bx = sw/2 - bw/2, by = 108;   // below the 76px HUD bar + name line

    Color ac = BossAccent();
    const char *name = TextFormat("%s  -  %d / %d HP",
                                  BossDisplayName(), boss.hp, boss.maxhp);
    DrawText(name, sw/2 - MeasureText(name, 16)/2, by - 22, 16,
             (Color){ac.r, CLAMP255(ac.g + 80), CLAMP255(ac.b + 40), 235});

    DrawRectangleRounded((Rectangle){(float)(bx-3), (float)(by-3), (float)(bw+6), (float)(bh+6)},
                         0.4f, 6, (Color){14, 8, 22, 220});
    Color rim = (boss.inv > 0) ? (Color){255,255,255,220}
              : (boss.state == BS_WINDUP) ? (Color){235,70,70,235}
              : (boss.state == BS_FIRE_WINDUP || boss.state == BS_FIRE) ? CFIRE
              : (Color){BossAccent().r, BossAccent().g, BossAccent().b, 220};
    DrawRectangleRoundedLines((Rectangle){(float)(bx-3), (float)(by-3), (float)(bw+6), (float)(bh+6)},
                              0.4f, 6, rim);

    // One segment per HP point (maxhp is prime, so segments stay readable).
    float segW = (float)(bw - (boss.maxhp-1)*2) / boss.maxhp;
    for (int i = 0; i < boss.maxhp; i++) {
        float sx = bx + i * (segW + 2);
        Color c = (i < boss.hp) ? (Color){210, 55, 90, 255} : (Color){40, 28, 52, 255};
        DrawRectangleRec((Rectangle){sx, (float)by, segW, (float)bh}, c);
        if (i < boss.hp)
            DrawRectangleRec((Rectangle){sx, (float)by, segW, 5.0f},
                             (Color){255, 130, 160, 200});
    }
}
