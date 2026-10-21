#include "raylib.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define CONCEPT_COUNT 12
#define EXPORT_SIZE 1024
#define PATH_BUFFER_SIZE 2048

typedef struct IconConcept {
    const char *key;
    const char *name;
    const char *description;
} IconConcept;

static const IconConcept CONCEPTS[CONCEPT_COUNT] = {
    { "labyrinth_sigil", "Labyrinth-Siegel", "Top-down, klar und nah am Spiel" },
    { "gate_of_depths", "Tor der Tiefe", "Dungeon-Tor mit gruenem Helden" },
    { "arcane_portal", "Arkanes Portal", "Original-Skull im arkanen Portal" },
    { "the_mimic", "Der Mimic", "Gefaehrliche Truhe mit Charakter" },
    { "torch_corridor", "Fackelgang", "Klassischer Abstieg ins Verlies" },
    { "trap_rune", "Fallenrune", "Top-down-Falle als starkes Symbol" },
    { "skull_guardian", "Schaedelwaechter", "Dunkel, markant und bossartig" },
    { "boss_eye", "Bossauge", "Bedrohlich und auf kleinen Icons lesbar" },
    { "inferno_rift", "Inferno-Riss", "Skull im brennenden Dimensionsriss" },
    { "void_nexus", "Void-Nexus", "Riesiger Skull im schwarzen Sternentor" },
    { "soulstorm_portal", "Seelensturm", "Skull in tuerkisem Geisterorkan" },
    { "rune_crown_portal", "Runenkrone", "Koenigliches Portal aus Gold und Violett" }
};

static float MinF(float a, float b)
{
    return (a < b) ? a : b;
}

static Rectangle NRect(Rectangle bounds, float x, float y, float width, float height)
{
    return (Rectangle) {
        bounds.x + bounds.width*x,
        bounds.y + bounds.height*y,
        bounds.width*width,
        bounds.height*height
    };
}

static Vector2 NPoint(Rectangle bounds, float x, float y)
{
    return (Vector2) {
        bounds.x + bounds.width*x,
        bounds.y + bounds.height*y
    };
}

static Rectangle Inset(Rectangle rectangle, float amount)
{
    return (Rectangle) {
        rectangle.x + amount,
        rectangle.y + amount,
        rectangle.width - amount*2.0f,
        rectangle.height - amount*2.0f
    };
}

static void DrawNLine(Rectangle bounds, float x1, float y1, float x2, float y2, float thickness, Color color)
{
    DrawLineEx(NPoint(bounds, x1, y1), NPoint(bounds, x2, y2), bounds.width*thickness, color);
}

static void DrawGlow(Vector2 center, float radius, Color color)
{
    for (int layer = 5; layer >= 1; layer--) {
        float scale = 1.0f + (float)layer*0.18f;
        DrawCircleV(center, radius*scale, ColorAlpha(color, 0.035f*(float)(6 - layer)));
    }
}

static void DrawIconBackground(Rectangle bounds, Color top, Color bottom, Color accent)
{
    DrawRectangleGradientEx(bounds, top, bottom, bottom, top);

    for (int i = 1; i < 7; i++) {
        float inset = bounds.width*(0.045f + (float)i*0.012f);
        DrawRectangleRoundedLinesEx(Inset(bounds, inset), 0.16f, 18,
                                    bounds.width*0.0035f,
                                    ColorAlpha(accent, 0.035f));
    }
}

static void DrawLabyrinthSigil(Rectangle b)
{
    const Color violet = (Color){ 129, 79, 238, 255 };
    const Color cyan = (Color){ 44, 211, 225, 255 };
    const Color emerald = (Color){ 84, 255, 144, 255 };
    const Color gold = (Color){ 255, 195, 63, 255 };
    Vector2 center = NPoint(b, 0.50f, 0.51f);

    DrawIconBackground(b, (Color){ 19, 19, 38, 255 }, (Color){ 35, 17, 57, 255 }, violet);
    DrawGlow(center, b.width*0.24f, violet);

    DrawRing(center, b.width*0.315f, b.width*0.355f, 18, 128, 36, violet);
    DrawRing(center, b.width*0.315f, b.width*0.355f, 154, 286, 36, violet);
    DrawRing(center, b.width*0.315f, b.width*0.355f, 312, 360, 18, violet);
    DrawRing(center, b.width*0.215f, b.width*0.250f, -42, 78, 32, cyan);
    DrawRing(center, b.width*0.215f, b.width*0.250f, 108, 236, 32, cyan);
    DrawRing(center, b.width*0.215f, b.width*0.250f, 267, 330, 20, cyan);

    DrawNLine(b, 0.50f, 0.155f, 0.50f, 0.300f, 0.035f, violet);
    DrawNLine(b, 0.745f, 0.510f, 0.860f, 0.510f, 0.035f, violet);
    DrawNLine(b, 0.140f, 0.510f, 0.285f, 0.510f, 0.035f, cyan);
    DrawNLine(b, 0.500f, 0.745f, 0.500f, 0.875f, 0.035f, cyan);
    DrawNLine(b, 0.500f, 0.295f, 0.610f, 0.405f, 0.032f, violet);
    DrawNLine(b, 0.388f, 0.615f, 0.500f, 0.725f, 0.032f, cyan);

    DrawRectangleRounded(NRect(b, 0.435f, 0.445f, 0.13f, 0.13f), 0.22f, 8, (Color){ 18, 45, 35, 255 });
    DrawRectangleRounded(NRect(b, 0.452f, 0.462f, 0.096f, 0.096f), 0.20f, 8, emerald);
    DrawRectangleRoundedLinesEx(NRect(b, 0.452f, 0.462f, 0.096f, 0.096f), 0.20f, 8, b.width*0.012f, WHITE);

    DrawRectangleRounded(NRect(b, 0.455f, 0.105f, 0.09f, 0.065f), 0.18f, 6, gold);
    DrawRectangleRec(NRect(b, 0.475f, 0.118f, 0.05f, 0.012f), (Color){ 255, 237, 153, 255 });
    DrawCircleV(NPoint(b, 0.835f, 0.510f), b.width*0.039f, (Color){ 255, 64, 85, 255 });
    DrawCircleV(NPoint(b, 0.823f, 0.497f), b.width*0.011f, WHITE);
}

static void DrawGateOfDepths(Rectangle b)
{
    const Color stone = (Color){ 103, 100, 131, 255 };
    const Color stoneLight = (Color){ 149, 143, 177, 255 };
    const Color emerald = (Color){ 77, 255, 144, 255 };
    Vector2 archCenter = NPoint(b, 0.50f, 0.43f);

    DrawIconBackground(b, (Color){ 12, 17, 29, 255 }, (Color){ 23, 16, 42, 255 }, stoneLight);
    DrawGlow(archCenter, b.width*0.27f, (Color){ 92, 50, 210, 255 });

    DrawCircleV(archCenter, b.width*0.335f, stone);
    DrawRectangleRec(NRect(b, 0.165f, 0.42f, 0.67f, 0.43f), stone);
    DrawCircleV(NPoint(b, 0.50f, 0.455f), b.width*0.235f, (Color){ 9, 12, 22, 255 });
    DrawRectangleRec(NRect(b, 0.265f, 0.445f, 0.47f, 0.405f), (Color){ 9, 12, 22, 255 });

    DrawRectangleGradientEx(NRect(b, 0.315f, 0.43f, 0.37f, 0.42f),
                            (Color){ 28, 12, 57, 255 },
                            (Color){ 12, 47, 44, 255 },
                            (Color){ 12, 47, 44, 255 },
                            (Color){ 28, 12, 57, 255 });
    DrawRectangleRoundedLinesEx(NRect(b, 0.315f, 0.43f, 0.37f, 0.42f), 0.08f, 12, b.width*0.018f,
                                ColorAlpha(emerald, 0.44f));

    DrawNLine(b, 0.18f, 0.56f, 0.31f, 0.56f, 0.012f, stoneLight);
    DrawNLine(b, 0.69f, 0.56f, 0.82f, 0.56f, 0.012f, stoneLight);
    DrawNLine(b, 0.18f, 0.71f, 0.31f, 0.71f, 0.012f, stoneLight);
    DrawNLine(b, 0.69f, 0.71f, 0.82f, 0.71f, 0.012f, stoneLight);
    DrawNLine(b, 0.50f, 0.10f, 0.50f, 0.20f, 0.012f, stoneLight);
    DrawNLine(b, 0.285f, 0.22f, 0.355f, 0.30f, 0.012f, stoneLight);
    DrawNLine(b, 0.715f, 0.22f, 0.645f, 0.30f, 0.012f, stoneLight);

    DrawCircleV(NPoint(b, 0.445f, 0.505f), b.width*0.017f, (Color){ 255, 48, 77, 255 });
    DrawCircleV(NPoint(b, 0.555f, 0.505f), b.width*0.017f, (Color){ 255, 48, 77, 255 });
    DrawGlow(NPoint(b, 0.50f, 0.725f), b.width*0.065f, emerald);
    DrawRectangleRounded(NRect(b, 0.445f, 0.67f, 0.11f, 0.11f), 0.20f, 8, emerald);
    DrawRectangleRoundedLinesEx(NRect(b, 0.445f, 0.67f, 0.11f, 0.11f), 0.20f, 8, b.width*0.012f, WHITE);
}

// Exact visual language of DungeonCrawlerV1_6 skin ID 28 (shop name "Skull").
// Adapted from skins.c::DrawSkull so the launcher icon uses the real in-game skin.
static void DrawGameSkullEllipse(float cx, float cy, float rx, float ry, float angle, Color color)
{
    const int segments = 24;
    Vector2 points[segments + 1];
    float ca = cosf(angle);
    float sa = sinf(angle);

    for (int i = 0; i <= segments; i++) {
        float theta = (float)i/(float)segments*2.0f*PI;
        float ex = cosf(theta)*rx;
        float ey = sinf(theta)*ry;
        points[i] = (Vector2){ cx + ex*ca - ey*sa, cy + ex*sa + ey*ca };
    }
    for (int i = 0; i < segments; i++) {
        DrawTriangle((Vector2){ cx, cy }, points[i + 1], points[i], color);
    }
}

static void DrawGameSkullShape(float cx, float cy, float radius, Color color)
{
    static const float contour[][2] = {
        { 0.00f, -1.05f }, { 0.42f, -0.99f }, { 0.72f, -0.82f },
        { 0.91f, -0.55f }, { 1.00f, -0.22f }, { 1.01f,  0.10f },
        { 0.90f,  0.34f }, { 0.72f,  0.52f },
        { 0.64f,  0.72f }, { 0.60f,  0.95f }, { 0.50f,  1.12f },
        { 0.30f,  1.21f }, { 0.00f,  1.23f }
    };
    int count = (int)(sizeof(contour)/sizeof(contour[0]));
    Vector2 control[40];
    int total = 0;

    for (int i = 0; i < count; i++) {
        control[total++] = (Vector2){ cx + contour[i][0]*radius, cy + contour[i][1]*radius };
    }
    for (int i = count - 2; i >= 1; i--) {
        control[total++] = (Vector2){ cx - contour[i][0]*radius, cy + contour[i][1]*radius };
    }

    Vector2 center = { cx, cy };
    Vector2 first = { 0 };
    Vector2 previous = { 0 };
    bool started = false;
    const int subdivisions = 8;

    for (int i = 0; i < total; i++) {
        Vector2 p0 = control[(i - 1 + total)%total];
        Vector2 p1 = control[i];
        Vector2 p2 = control[(i + 1)%total];
        Vector2 p3 = control[(i + 2)%total];

        for (int step = 0; step < subdivisions; step++) {
            float u = (float)step/(float)subdivisions;
            float u2 = u*u;
            float u3 = u2*u;
            Vector2 point = {
                0.5f*((2.0f*p1.x) + (-p0.x + p2.x)*u +
                      (2.0f*p0.x - 5.0f*p1.x + 4.0f*p2.x - p3.x)*u2 +
                      (-p0.x + 3.0f*p1.x - 3.0f*p2.x + p3.x)*u3),
                0.5f*((2.0f*p1.y) + (-p0.y + p2.y)*u +
                      (2.0f*p0.y - 5.0f*p1.y + 4.0f*p2.y - p3.y)*u2 +
                      (-p0.y + 3.0f*p1.y - 3.0f*p2.y + p3.y)*u3)
            };

            if (started) {
                DrawTriangle(center, point, previous, color);
                DrawTriangle(center, previous, point, color);
            } else {
                first = point;
                started = true;
            }
            previous = point;
        }
    }

    DrawTriangle(center, first, previous, color);
    DrawTriangle(center, previous, first, color);
}

static float GameSkullRandom(int index)
{
    float value = sinf((float)index*127.1f + 311.7f)*43758.5453f;
    return value - floorf(value);
}

static void DrawGameSkull(float cx, float cy, float radius, float time)
{
    Color bone = (Color){ 231, 226, 212, 255 };
    Color boneLow = (Color){ 193, 187, 172, 255 };
    Color boneHigh = (Color){ 247, 244, 235, 255 };
    Color socket = (Color){ 20, 10, 8, 255 };
    Color dark = (Color){ 24, 20, 21, 255 };
    Color ember = (Color){ 255, 110, 30, 255 };
    float bob = sinf(time*2.2f)*radius*0.04f;
    float centerY = cy + bob;
    float breath = 0.5f + 0.5f*sinf(time*1.8f);
    float pulse = 0.5f + 0.5f*sinf(time*4.0f);

    BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleV((Vector2){ cx, centerY }, radius*(1.25f + 0.06f*breath),
                    (Color){ 255, 90, 20, (unsigned char)(14.0f + 12.0f*breath) });
    EndBlendMode();

    DrawGameSkullShape(cx, centerY + radius*0.03f, radius*1.03f, boneLow);
    DrawGameSkullShape(cx, centerY, radius, bone);

    for (int i = 0; i < 4; i++) {
        float scale = 1.0f - (float)i*0.18f;
        DrawGameSkullEllipse(cx - radius*0.10f, centerY - radius*0.46f,
                             radius*0.58f*scale, radius*0.44f*scale, 0.0f,
                             (Color){ boneHigh.r, boneHigh.g, boneHigh.b, 42 });
    }
    DrawGameSkullEllipse(cx, centerY + radius*0.58f, radius*0.52f, radius*0.26f, 0.0f,
                         (Color){ boneLow.r, boneLow.g, boneLow.b, 110 });

    DrawGameSkullEllipse(cx - radius*0.44f, centerY - radius*0.07f,
                         radius*0.48f, radius*0.42f, 0.32f, (Color){ 150, 144, 132, 120 });
    DrawGameSkullEllipse(cx + radius*0.44f, centerY - radius*0.07f,
                         radius*0.48f, radius*0.42f, -0.32f, (Color){ 150, 144, 132, 120 });
    DrawGameSkullEllipse(cx - radius*0.44f, centerY - radius*0.08f,
                         radius*0.42f, radius*0.36f, 0.32f, socket);
    DrawGameSkullEllipse(cx + radius*0.44f, centerY - radius*0.08f,
                         radius*0.42f, radius*0.36f, -0.32f, socket);

    float emberRadius = radius*(0.10f + 0.05f*pulse);
    BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleV((Vector2){ cx - radius*0.40f, centerY - radius*0.04f }, emberRadius,
                    (Color){ ember.r, ember.g, ember.b, (unsigned char)(90.0f + 100.0f*pulse) });
        DrawCircleV((Vector2){ cx + radius*0.40f, centerY - radius*0.04f }, emberRadius,
                    (Color){ ember.r, ember.g, ember.b, (unsigned char)(90.0f + 100.0f*pulse) });
        DrawCircleV((Vector2){ cx - radius*0.40f, centerY - radius*0.04f }, emberRadius*0.45f,
                    (Color){ 255, 220, 120, (unsigned char)(120.0f + 80.0f*pulse) });
        DrawCircleV((Vector2){ cx + radius*0.40f, centerY - radius*0.04f }, emberRadius*0.45f,
                    (Color){ 255, 220, 120, (unsigned char)(120.0f + 80.0f*pulse) });
    EndBlendMode();
    DrawCircleV((Vector2){ cx - radius*0.34f, centerY + radius*0.10f }, radius*0.05f,
                (Color){ 55, 51, 53, 150 });
    DrawCircleV((Vector2){ cx + radius*0.34f, centerY + radius*0.10f }, radius*0.05f,
                (Color){ 55, 51, 53, 150 });

    DrawTriangle((Vector2){ cx, centerY + radius*0.44f },
                 (Vector2){ cx - radius*0.15f, centerY + radius*0.20f },
                 (Vector2){ cx + radius*0.15f, centerY + radius*0.20f }, dark);
    DrawTriangle((Vector2){ cx, centerY + radius*0.44f },
                 (Vector2){ cx + radius*0.15f, centerY + radius*0.20f },
                 (Vector2){ cx - radius*0.15f, centerY + radius*0.20f }, dark);
    DrawCircleV((Vector2){ cx - radius*0.08f, centerY + radius*0.22f }, radius*0.075f, dark);
    DrawCircleV((Vector2){ cx + radius*0.08f, centerY + radius*0.22f }, radius*0.075f, dark);

    float chatter = sinf(time*12.0f)*radius*0.012f;
    float mouthX = cx - radius*0.46f;
    float mouthWidth = radius*0.92f;
    float mouthTop = centerY + radius*0.62f + chatter;
    float mouthBottom = centerY + radius*1.00f + chatter;
    DrawRectangleRounded((Rectangle){ mouthX, mouthTop, mouthWidth, mouthBottom - mouthTop },
                         0.35f, 6, dark);
    const int teeth = 6;
    float toothWidth = mouthWidth/(float)teeth;
    for (int tooth = 0; tooth < teeth; tooth++) {
        float toothX = mouthX + (float)tooth*toothWidth;
        DrawRectangleRounded((Rectangle){ toothX + radius*0.03f, mouthTop + radius*0.03f,
                                          toothWidth - radius*0.06f,
                                          (mouthBottom - mouthTop) - radius*0.06f },
                             0.30f, 4, boneHigh);
    }

    for (int spark = 0; spark < 6; spark++) {
        float progress = fmodf(time*0.7f + GameSkullRandom(spark + 3), 1.0f);
        float side = (spark%2) ? 1.0f : -1.0f;
        DrawCircleV((Vector2){ cx + side*radius*0.40f + (GameSkullRandom(spark) - 0.5f)*radius*0.1f,
                               centerY - radius*0.05f - progress*radius },
                    radius*(0.05f - progress*0.03f),
                    (Color){ 255, 140, 40, (unsigned char)(200.0f*(1.0f - progress)) });
    }
}

static void DrawArcanePortal(Rectangle b)
{
    const Color cyan = (Color){ 48, 230, 255, 255 };
    const Color violet = (Color){ 171, 74, 255, 255 };
    const Color blue = (Color){ 54, 103, 255, 255 };
    Vector2 center = NPoint(b, 0.50f, 0.50f);

    DrawIconBackground(b, (Color){ 8, 15, 35, 255 }, (Color){ 26, 10, 49, 255 }, cyan);
    DrawGlow(center, b.width*0.30f, violet);
    DrawCircleV(center, b.width*0.31f, (Color){ 13, 13, 38, 255 });

    DrawRing(center, b.width*0.305f, b.width*0.345f, 10, 145, 44, cyan);
    DrawRing(center, b.width*0.305f, b.width*0.345f, 180, 325, 44, violet);
    DrawRing(center, b.width*0.235f, b.width*0.265f, -55, 82, 38, blue);
    DrawRing(center, b.width*0.235f, b.width*0.265f, 112, 258, 38, cyan);
    DrawRing(center, b.width*0.170f, b.width*0.200f, 0, 205, 36, violet);
    DrawRing(center, b.width*0.170f, b.width*0.200f, 235, 340, 28, blue);

    for (int i = 0; i < 8; i++) {
        float angle = ((float)i*45.0f + 22.5f)*DEG2RAD;
        float radius = b.width*((i % 2 == 0) ? 0.405f : 0.385f);
        Vector2 p = { center.x + cosf(angle)*radius, center.y + sinf(angle)*radius };
        DrawPoly(p, 4, b.width*0.024f, 45.0f, (i % 2 == 0) ? cyan : violet);
    }

    DrawCircleV(center, b.width*0.158f, (Color){ 10, 10, 24, 225 });
    DrawGameSkull(center.x, center.y - b.width*0.006f, b.width*0.130f, 1.35f);
}

static void DrawRotatedOrbit(Vector2 center, float radiusX, float radiusY,
                             float rotation, float thickness, Color color)
{
    const int segments = 64;
    float cosine = cosf(rotation);
    float sine = sinf(rotation);
    Vector2 previous = { 0 };

    for (int segment = 0; segment <= segments; segment++) {
        float angle = (float)segment/(float)segments*2.0f*PI;
        float x = cosf(angle)*radiusX;
        float y = sinf(angle)*radiusY;
        Vector2 point = {
            center.x + x*cosine - y*sine,
            center.y + x*sine + y*cosine
        };
        if (segment > 0) DrawLineEx(previous, point, thickness, color);
        previous = point;
    }
}

static void DrawInfernoRift(Rectangle b)
{
    const Color crimson = (Color){ 255, 45, 55, 255 };
    const Color orange = (Color){ 255, 112, 24, 255 };
    const Color gold = (Color){ 255, 212, 73, 255 };
    Vector2 center = NPoint(b, 0.50f, 0.51f);

    DrawIconBackground(b, (Color){ 30, 7, 14, 255 }, (Color){ 74, 15, 17, 255 }, orange);
    DrawGlow(center, b.width*0.34f, crimson);
    DrawCircleV(center, b.width*0.355f, (Color){ 37, 7, 13, 255 });

    DrawRing(center, b.width*0.315f, b.width*0.365f, -22, 83, 36, crimson);
    DrawRing(center, b.width*0.315f, b.width*0.365f, 105, 213, 36, orange);
    DrawRing(center, b.width*0.315f, b.width*0.365f, 238, 338, 36, gold);
    DrawRing(center, b.width*0.235f, b.width*0.275f, 8, 137, 36, gold);
    DrawRing(center, b.width*0.235f, b.width*0.275f, 168, 307, 36, crimson);

    for (int i = 0; i < 12; i++) {
        float angle = ((float)i*30.0f - 90.0f)*DEG2RAD;
        float sideAngle = angle + PI*0.5f;
        Vector2 base = {
            center.x + cosf(angle)*b.width*0.365f,
            center.y + sinf(angle)*b.width*0.365f
        };
        Vector2 tip = {
            center.x + cosf(angle)*b.width*((i%3 == 0) ? 0.475f : 0.435f),
            center.y + sinf(angle)*b.width*((i%3 == 0) ? 0.475f : 0.435f)
        };
        Vector2 left = {
            base.x + cosf(sideAngle)*b.width*0.030f,
            base.y + sinf(sideAngle)*b.width*0.030f
        };
        Vector2 right = {
            base.x - cosf(sideAngle)*b.width*0.030f,
            base.y - sinf(sideAngle)*b.width*0.030f
        };
        DrawTriangle(left, tip, right, (i%2 == 0) ? orange : crimson);
    }

    for (int i = 0; i < 10; i++) {
        float angle = ((float)i*36.0f + 15.0f)*DEG2RAD;
        float radius = b.width*((i%2 == 0) ? 0.415f : 0.385f);
        Vector2 mote = { center.x + cosf(angle)*radius, center.y + sinf(angle)*radius };
        DrawCircleV(mote, b.width*((i%3 == 0) ? 0.018f : 0.011f), (i%2 == 0) ? gold : orange);
    }

    DrawGlow(center, b.width*0.20f, orange);
    DrawCircleV(center, b.width*0.205f, (Color){ 15, 7, 12, 245 });
    DrawRing(center, b.width*0.185f, b.width*0.205f, 0, 360, 48, ColorAlpha(gold, 0.65f));
    DrawGameSkull(center.x, center.y - b.width*0.010f, b.width*0.158f, 0.42f);
}

static void DrawVoidNexus(Rectangle b)
{
    const Color violet = (Color){ 154, 57, 255, 255 };
    const Color magenta = (Color){ 238, 64, 255, 255 };
    const Color blue = (Color){ 56, 105, 255, 255 };
    Vector2 center = NPoint(b, 0.50f, 0.51f);

    DrawIconBackground(b, (Color){ 5, 8, 23, 255 }, (Color){ 22, 8, 42, 255 }, violet);
    DrawGlow(center, b.width*0.34f, violet);
    DrawCircleV(center, b.width*0.38f, (Color){ 12, 7, 30, 255 });
    DrawRing(center, b.width*0.332f, b.width*0.365f, 18, 151, 42, violet);
    DrawRing(center, b.width*0.332f, b.width*0.365f, 183, 316, 42, blue);
    DrawRing(center, b.width*0.275f, b.width*0.296f, -40, 98, 36, magenta);
    DrawRing(center, b.width*0.275f, b.width*0.296f, 133, 282, 36, violet);

    DrawRotatedOrbit(center, b.width*0.40f, b.width*0.145f, 0.34f,
                     b.width*0.014f, ColorAlpha(magenta, 0.82f));
    DrawRotatedOrbit(center, b.width*0.39f, b.width*0.125f, -0.48f,
                     b.width*0.011f, ColorAlpha(blue, 0.80f));

    for (int i = 0; i < 12; i++) {
        float angle = ((float)i*30.0f + 10.0f)*DEG2RAD;
        float radius = b.width*((i%3 == 0) ? 0.435f : 0.395f);
        Vector2 star = { center.x + cosf(angle)*radius, center.y + sinf(angle)*radius };
        DrawPoly(star, 4, b.width*((i%3 == 0) ? 0.022f : 0.013f), 45.0f,
                 (i%2 == 0) ? magenta : blue);
    }

    DrawCircleV(center, b.width*0.215f, (Color){ 3, 4, 13, 250 });
    DrawRing(center, b.width*0.198f, b.width*0.215f, 0, 360, 48, ColorAlpha(violet, 0.72f));
    DrawGameSkull(center.x, center.y - b.width*0.010f, b.width*0.166f, 1.02f);
}

static void DrawSoulstormPortal(Rectangle b)
{
    const Color cyan = (Color){ 54, 242, 239, 255 };
    const Color teal = (Color){ 20, 190, 166, 255 };
    const Color blue = (Color){ 65, 121, 255, 255 };
    Vector2 center = NPoint(b, 0.50f, 0.51f);

    DrawIconBackground(b, (Color){ 5, 20, 29, 255 }, (Color){ 10, 38, 49, 255 }, cyan);
    DrawGlow(center, b.width*0.34f, teal);
    DrawCircleV(center, b.width*0.37f, (Color){ 4, 24, 31, 255 });

    DrawRing(center, b.width*0.320f, b.width*0.365f, -32, 76, 36, cyan);
    DrawRing(center, b.width*0.320f, b.width*0.365f, 103, 213, 36, teal);
    DrawRing(center, b.width*0.320f, b.width*0.365f, 242, 338, 36, blue);
    DrawRing(center, b.width*0.242f, b.width*0.278f, 12, 142, 36, blue);
    DrawRing(center, b.width*0.242f, b.width*0.278f, 170, 312, 36, cyan);

    const float bolts[][8] = {
        { 0.17f,0.28f, 0.25f,0.33f, 0.21f,0.42f, 0.31f,0.45f },
        { 0.83f,0.25f, 0.75f,0.33f, 0.79f,0.41f, 0.69f,0.46f },
        { 0.17f,0.74f, 0.26f,0.68f, 0.23f,0.59f, 0.32f,0.56f },
        { 0.83f,0.76f, 0.75f,0.67f, 0.79f,0.59f, 0.68f,0.55f }
    };
    for (int i = 0; i < 4; i++) {
        DrawNLine(b, bolts[i][0], bolts[i][1], bolts[i][2], bolts[i][3], 0.017f, cyan);
        DrawNLine(b, bolts[i][2], bolts[i][3], bolts[i][4], bolts[i][5], 0.017f, cyan);
        DrawNLine(b, bolts[i][4], bolts[i][5], bolts[i][6], bolts[i][7], 0.017f, blue);
    }

    for (int i = 0; i < 8; i++) {
        float angle = ((float)i*45.0f + 22.0f)*DEG2RAD;
        float radius = b.width*0.415f;
        Vector2 soul = { center.x + cosf(angle)*radius, center.y + sinf(angle)*radius };
        DrawGlow(soul, b.width*0.020f, cyan);
        DrawCircleV(soul, b.width*0.016f, (i%2 == 0) ? cyan : teal);
    }

    DrawCircleV(center, b.width*0.205f, (Color){ 3, 15, 22, 247 });
    DrawRing(center, b.width*0.188f, b.width*0.205f, 0, 360, 48, ColorAlpha(cyan, 0.70f));
    DrawGameSkull(center.x, center.y - b.width*0.010f, b.width*0.158f, 1.78f);
}

static void DrawRuneCrownPortal(Rectangle b)
{
    const Color gold = (Color){ 255, 205, 68, 255 };
    const Color goldLight = (Color){ 255, 239, 154, 255 };
    const Color violet = (Color){ 144, 66, 239, 255 };
    const Color amethyst = (Color){ 211, 75, 255, 255 };
    Vector2 center = NPoint(b, 0.50f, 0.52f);

    DrawIconBackground(b, (Color){ 21, 13, 37, 255 }, (Color){ 48, 23, 65, 255 }, gold);
    DrawGlow(center, b.width*0.34f, violet);
    DrawCircleV(center, b.width*0.375f, (Color){ 26, 13, 43, 255 });
    DrawPolyLinesEx(center, 8, b.width*0.365f, 22.5f, b.width*0.032f, gold);
    DrawPolyLinesEx(center, 8, b.width*0.300f, 22.5f, b.width*0.020f, violet);

    for (int i = 0; i < 12; i++) {
        float angle = ((float)i*30.0f - 90.0f)*DEG2RAD;
        float radius = b.width*0.425f;
        Vector2 rune = { center.x + cosf(angle)*radius, center.y + sinf(angle)*radius };
        DrawPoly(rune, (i%3 == 0) ? 3 : 4, b.width*((i%3 == 0) ? 0.035f : 0.026f),
                 (float)i*15.0f, (i%2 == 0) ? gold : amethyst);
        DrawCircleV(rune, b.width*0.008f, goldLight);
    }

    DrawRing(center, b.width*0.235f, b.width*0.268f, -20, 112, 36, gold);
    DrawRing(center, b.width*0.235f, b.width*0.268f, 143, 278, 36, violet);
    DrawCircleV(center, b.width*0.212f, (Color){ 13, 8, 23, 248 });

    DrawRectangleRounded(NRect(b, 0.405f, 0.292f, 0.19f, 0.060f), 0.18f, 8, gold);
    DrawTriangle(NPoint(b, 0.405f, 0.310f), NPoint(b, 0.425f, 0.215f), NPoint(b, 0.475f, 0.310f), gold);
    DrawTriangle(NPoint(b, 0.455f, 0.310f), NPoint(b, 0.500f, 0.185f), NPoint(b, 0.545f, 0.310f), goldLight);
    DrawTriangle(NPoint(b, 0.525f, 0.310f), NPoint(b, 0.575f, 0.215f), NPoint(b, 0.595f, 0.310f), gold);
    DrawCircleV(NPoint(b, 0.50f, 0.275f), b.width*0.018f, amethyst);

    DrawGameSkull(center.x, center.y - b.width*0.004f, b.width*0.164f, 2.24f);
}

static void DrawMimic(Rectangle b)
{
    const Color gold = (Color){ 242, 170, 43, 255 };
    const Color goldLight = (Color){ 255, 221, 105, 255 };
    const Color wood = (Color){ 111, 53, 34, 255 };
    const Color red = (Color){ 255, 51, 72, 255 };

    DrawIconBackground(b, (Color){ 31, 13, 24, 255 }, (Color){ 58, 18, 25, 255 }, gold);
    DrawGlow(NPoint(b, 0.50f, 0.53f), b.width*0.30f, red);

    DrawRectangleRounded(NRect(b, 0.16f, 0.27f, 0.68f, 0.28f), 0.20f, 18, gold);
    DrawRectangleRounded(NRect(b, 0.205f, 0.32f, 0.59f, 0.20f), 0.16f, 14, wood);
    DrawRectangleRoundedLinesEx(NRect(b, 0.16f, 0.27f, 0.68f, 0.28f), 0.20f, 18, b.width*0.022f, goldLight);
    DrawRectangleRounded(NRect(b, 0.14f, 0.49f, 0.72f, 0.34f), 0.10f, 12, wood);
    DrawRectangleRoundedLinesEx(NRect(b, 0.14f, 0.49f, 0.72f, 0.34f), 0.10f, 12, b.width*0.025f, gold);

    DrawRectangleRec(NRect(b, 0.14f, 0.49f, 0.72f, 0.115f), (Color){ 22, 9, 14, 255 });
    DrawCircleV(NPoint(b, 0.355f, 0.545f), b.width*0.046f, red);
    DrawCircleV(NPoint(b, 0.645f, 0.545f), b.width*0.046f, red);
    DrawCircleV(NPoint(b, 0.342f, 0.530f), b.width*0.013f, WHITE);
    DrawCircleV(NPoint(b, 0.632f, 0.530f), b.width*0.013f, WHITE);

    for (int i = 0; i < 6; i++) {
        float x = 0.22f + (float)i*0.105f;
        DrawTriangle(NPoint(b, x, 0.605f), NPoint(b, x + 0.052f, 0.695f), NPoint(b, x + 0.10f, 0.605f), (Color){ 255, 244, 208, 255 });
    }
    DrawRectangleRounded(NRect(b, 0.435f, 0.665f, 0.13f, 0.115f), 0.16f, 8, gold);
    DrawRectangleRounded(NRect(b, 0.475f, 0.695f, 0.05f, 0.055f), 0.20f, 6, (Color){ 68, 31, 29, 255 });
}

static void DrawTorchCorridor(Rectangle b)
{
    const Color orange = (Color){ 255, 134, 42, 255 };
    const Color amber = (Color){ 255, 216, 83, 255 };
    const Color emerald = (Color){ 80, 255, 139, 255 };
    Vector2 vanish = NPoint(b, 0.50f, 0.43f);

    DrawIconBackground(b, (Color){ 18, 15, 18, 255 }, (Color){ 55, 29, 23, 255 }, orange);
    DrawTriangle(NPoint(b, 0.06f, 0.10f), NPoint(b, 0.06f, 0.90f), NPoint(b, 0.39f, 0.63f), (Color){ 72, 52, 48, 255 });
    DrawTriangle(NPoint(b, 0.94f, 0.10f), NPoint(b, 0.61f, 0.63f), NPoint(b, 0.94f, 0.90f), (Color){ 72, 52, 48, 255 });
    DrawTriangle(NPoint(b, 0.06f, 0.90f), NPoint(b, 0.94f, 0.90f), vanish, (Color){ 35, 31, 35, 255 });
    DrawTriangle(NPoint(b, 0.06f, 0.10f), vanish, NPoint(b, 0.94f, 0.10f), (Color){ 28, 24, 31, 255 });

    DrawNLine(b, 0.06f, 0.10f, 0.39f, 0.63f, 0.010f, (Color){ 137, 95, 72, 255 });
    DrawNLine(b, 0.94f, 0.10f, 0.61f, 0.63f, 0.010f, (Color){ 137, 95, 72, 255 });
    DrawNLine(b, 0.06f, 0.90f, 0.39f, 0.63f, 0.010f, (Color){ 137, 95, 72, 255 });
    DrawNLine(b, 0.94f, 0.90f, 0.61f, 0.63f, 0.010f, (Color){ 137, 95, 72, 255 });

    DrawRectangleGradientEx(NRect(b, 0.39f, 0.34f, 0.22f, 0.31f),
                            (Color){ 21, 11, 32, 255 }, (Color){ 17, 74, 53, 255 },
                            (Color){ 17, 74, 53, 255 }, (Color){ 21, 11, 32, 255 });
    DrawRectangleRoundedLinesEx(NRect(b, 0.39f, 0.34f, 0.22f, 0.31f), 0.12f, 10, b.width*0.018f, (Color){ 125, 95, 120, 255 });

    DrawNLine(b, 0.24f, 0.48f, 0.24f, 0.69f, 0.022f, (Color){ 117, 76, 48, 255 });
    DrawNLine(b, 0.76f, 0.48f, 0.76f, 0.69f, 0.022f, (Color){ 117, 76, 48, 255 });
    DrawGlow(NPoint(b, 0.24f, 0.43f), b.width*0.06f, orange);
    DrawGlow(NPoint(b, 0.76f, 0.43f), b.width*0.06f, orange);
    DrawTriangle(NPoint(b, 0.24f, 0.31f), NPoint(b, 0.18f, 0.47f), NPoint(b, 0.30f, 0.47f), orange);
    DrawCircleV(NPoint(b, 0.24f, 0.43f), b.width*0.055f, amber);
    DrawTriangle(NPoint(b, 0.76f, 0.31f), NPoint(b, 0.70f, 0.47f), NPoint(b, 0.82f, 0.47f), orange);
    DrawCircleV(NPoint(b, 0.76f, 0.43f), b.width*0.055f, amber);

    DrawGlow(NPoint(b, 0.50f, 0.56f), b.width*0.055f, emerald);
    DrawRectangleRounded(NRect(b, 0.455f, 0.515f, 0.09f, 0.09f), 0.18f, 7, emerald);
    DrawRectangleRoundedLinesEx(NRect(b, 0.455f, 0.515f, 0.09f, 0.09f), 0.18f, 7, b.width*0.010f, WHITE);
}

static void DrawTrapRune(Rectangle b)
{
    const Color red = (Color){ 255, 61, 83, 255 };
    const Color orange = (Color){ 255, 145, 44, 255 };
    const Color emerald = (Color){ 75, 255, 139, 255 };
    Vector2 center = NPoint(b, 0.50f, 0.50f);

    DrawIconBackground(b, (Color){ 11, 27, 32, 255 }, (Color){ 18, 15, 37, 255 }, red);
    DrawGlow(center, b.width*0.28f, red);

    for (int i = 0; i < 8; i++) {
        float angle = ((float)i*45.0f - 90.0f)*DEG2RAD;
        float side = angle + 90.0f*DEG2RAD;
        Vector2 base = { center.x + cosf(angle)*b.width*0.29f, center.y + sinf(angle)*b.width*0.29f };
        Vector2 tip = { center.x + cosf(angle)*b.width*0.43f, center.y + sinf(angle)*b.width*0.43f };
        Vector2 left = { base.x + cosf(side)*b.width*0.045f, base.y + sinf(side)*b.width*0.045f };
        Vector2 right = { base.x - cosf(side)*b.width*0.045f, base.y - sinf(side)*b.width*0.045f };
        DrawTriangle(left, tip, right, (i % 2 == 0) ? red : orange);
    }

    DrawPoly(center, 4, b.width*0.285f, 45.0f, (Color){ 79, 21, 42, 255 });
    DrawPolyLinesEx(center, 4, b.width*0.285f, 45.0f, b.width*0.026f, red);
    DrawRing(center, b.width*0.145f, b.width*0.178f, 0, 360, 40, orange);
    DrawNLine(b, 0.39f, 0.39f, 0.61f, 0.61f, 0.022f, red);
    DrawNLine(b, 0.61f, 0.39f, 0.39f, 0.61f, 0.022f, red);

    DrawGlow(center, b.width*0.075f, emerald);
    DrawRectangleRounded(NRect(b, 0.445f, 0.445f, 0.11f, 0.11f), 0.18f, 8, emerald);
    DrawRectangleRoundedLinesEx(NRect(b, 0.445f, 0.445f, 0.11f, 0.11f), 0.18f, 8, b.width*0.012f, WHITE);
}

static void DrawSkullGuardian(Rectangle b)
{
    const Color bone = (Color){ 224, 222, 207, 255 };
    const Color boneShadow = (Color){ 151, 150, 153, 255 };
    const Color violet = (Color){ 143, 67, 235, 255 };
    const Color red = (Color){ 255, 49, 75, 255 };
    const Color emerald = (Color){ 78, 255, 142, 255 };

    DrawIconBackground(b, (Color){ 15, 13, 29, 255 }, (Color){ 42, 17, 54, 255 }, violet);
    DrawGlow(NPoint(b, 0.50f, 0.47f), b.width*0.31f, violet);

    DrawCircleV(NPoint(b, 0.50f, 0.40f), b.width*0.295f, bone);
    DrawRectangleRounded(NRect(b, 0.265f, 0.38f, 0.47f, 0.36f), 0.18f, 14, bone);
    DrawRectangleRounded(NRect(b, 0.34f, 0.64f, 0.32f, 0.18f), 0.12f, 10, boneShadow);

    DrawTriangle(NPoint(b, 0.31f, 0.38f), NPoint(b, 0.46f, 0.43f), NPoint(b, 0.35f, 0.57f), (Color){ 28, 17, 38, 255 });
    DrawTriangle(NPoint(b, 0.69f, 0.38f), NPoint(b, 0.65f, 0.57f), NPoint(b, 0.54f, 0.43f), (Color){ 28, 17, 38, 255 });
    DrawCircleV(NPoint(b, 0.385f, 0.475f), b.width*0.032f, red);
    DrawCircleV(NPoint(b, 0.615f, 0.475f), b.width*0.032f, red);
    DrawTriangle(NPoint(b, 0.50f, 0.49f), NPoint(b, 0.44f, 0.60f), NPoint(b, 0.56f, 0.60f), (Color){ 49, 37, 48, 255 });

    DrawRectangleRounded(NRect(b, 0.365f, 0.655f, 0.27f, 0.135f), 0.10f, 8, (Color){ 23, 21, 31, 255 });
    for (int i = 1; i < 4; i++) {
        DrawNLine(b, 0.365f + (float)i*0.0675f, 0.66f, 0.365f + (float)i*0.0675f, 0.785f, 0.009f, bone);
    }
    DrawGlow(NPoint(b, 0.50f, 0.72f), b.width*0.055f, emerald);
    DrawRectangleRounded(NRect(b, 0.458f, 0.678f, 0.084f, 0.084f), 0.18f, 7, emerald);
}

static void DrawBossEye(Rectangle b)
{
    const Color orange = (Color){ 255, 126, 33, 255 };
    const Color gold = (Color){ 255, 207, 64, 255 };
    const Color red = (Color){ 245, 45, 62, 255 };
    const Color emerald = (Color){ 75, 255, 139, 255 };
    Vector2 center = NPoint(b, 0.50f, 0.50f);

    DrawIconBackground(b, (Color){ 21, 13, 17, 255 }, (Color){ 52, 19, 20, 255 }, orange);
    for (int i = 1; i < 5; i++) {
        float y = 0.16f + (float)i*0.15f;
        DrawNLine(b, 0.08f, y, 0.92f, y - 0.055f, 0.010f, (Color){ 108, 49, 43, 110 });
    }
    DrawGlow(center, b.width*0.31f, orange);
    DrawEllipse((int)center.x, (int)center.y, b.width*0.39f, b.width*0.245f, (Color){ 91, 28, 27, 255 });
    DrawEllipse((int)center.x, (int)center.y, b.width*0.345f, b.width*0.195f, orange);
    DrawEllipse((int)center.x, (int)center.y, b.width*0.265f, b.width*0.145f, gold);
    DrawCircleV(center, b.width*0.138f, red);
    DrawEllipse((int)center.x, (int)center.y, b.width*0.050f, b.width*0.145f, (Color){ 20, 10, 15, 255 });
    DrawCircleV(NPoint(b, 0.44f, 0.44f), b.width*0.026f, ColorAlpha(WHITE, 0.80f));

    DrawGlow(NPoint(b, 0.70f, 0.68f), b.width*0.045f, emerald);
    DrawRectangleRounded(NRect(b, 0.665f, 0.645f, 0.07f, 0.07f), 0.16f, 6, emerald);
    DrawRectangleRoundedLinesEx(NRect(b, 0.665f, 0.645f, 0.07f, 0.07f), 0.16f, 6, b.width*0.008f, WHITE);
}

static void DrawIconArt(int concept, Rectangle bounds)
{
    switch (concept) {
        case 0: DrawLabyrinthSigil(bounds); break;
        case 1: DrawGateOfDepths(bounds); break;
        case 2: DrawArcanePortal(bounds); break;
        case 3: DrawMimic(bounds); break;
        case 4: DrawTorchCorridor(bounds); break;
        case 5: DrawTrapRune(bounds); break;
        case 6: DrawSkullGuardian(bounds); break;
        case 7: DrawBossEye(bounds); break;
        case 8: DrawInfernoRift(bounds); break;
        case 9: DrawVoidNexus(bounds); break;
        case 10: DrawSoulstormPortal(bounds); break;
        case 11: DrawRuneCrownPortal(bounds); break;
        default: DrawRectangleRec(bounds, BLACK); break;
    }

    DrawRectangleRoundedLinesEx(Inset(bounds, bounds.width*0.025f), 0.14f, 18,
                                bounds.width*0.010f, ColorAlpha(WHITE, 0.16f));
}

static void DrawCenteredText(const char *text, float centerX, float y, int fontSize, Color color)
{
    int width = MeasureText(text, fontSize);
    DrawText(text, (int)(centerX - (float)width*0.5f), (int)y, fontSize, color);
}

static Rectangle GetCardRectangle(int index, int screenWidth, int screenHeight)
{
    const int columns = 4;
    const int rows = (CONCEPT_COUNT + columns - 1)/columns;
    const float margin = 28.0f;
    const float gap = 15.0f;
    const float top = 104.0f;
    const float bottom = 132.0f;
    float width = ((float)screenWidth - margin*2.0f - gap*3.0f)/(float)columns;
    float height = ((float)screenHeight - top - bottom - gap*(float)(rows - 1))/(float)rows;
    int column = index % columns;
    int row = index / columns;

    return (Rectangle) {
        margin + (float)column*(width + gap),
        top + (float)row*(height + gap),
        width,
        height
    };
}

static Rectangle GetSaveButtonRectangle(int screenWidth, int screenHeight)
{
    return (Rectangle) { (float)screenWidth - 330.0f, (float)screenHeight - 91.0f, 302.0f, 58.0f };
}

static void DrawConceptCard(int index, Rectangle card, bool hovered, bool selected, bool saved)
{
    Color fill = hovered ? (Color){ 34, 37, 58, 255 } : (Color){ 25, 28, 45, 255 };
    Color outline = selected ? (Color){ 89, 255, 150, 255 } : (Color){ 74, 79, 108, 255 };
    float previewSize = MinF(card.width - 24.0f, card.height - 82.0f);
    Rectangle preview = {
        card.x + (card.width - previewSize)*0.5f,
        card.y + 11.0f,
        previewSize,
        previewSize
    };

    DrawRectangleRounded(card, 0.055f, 14, fill);
    DrawRectangleRoundedLinesEx(card, 0.055f, 14, selected ? 4.0f : 2.0f, outline);
    DrawIconArt(index, preview);

    DrawCircleV((Vector2){ card.x + 28.0f, card.y + 28.0f }, 18.0f,
                selected ? (Color){ 89, 255, 150, 255 } : (Color){ 13, 15, 26, 230 });
    DrawCenteredText(TextFormat("%d", index + 1), card.x + 28.0f, card.y + 18.0f, 20,
                     selected ? (Color){ 10, 31, 22, 255 } : RAYWHITE);

    if (saved) {
        Rectangle badge = { card.x + card.width - 116.0f, card.y + 12.0f, 104.0f, 29.0f };
        DrawRectangleRounded(badge, 0.35f, 10, (Color){ 89, 255, 150, 235 });
        DrawCenteredText("GESPEICHERT", badge.x + badge.width*0.5f, badge.y + 7.0f, 14, (Color){ 10, 31, 22, 255 });
    }

    float labelY = card.y + card.height - 61.0f;
    DrawCenteredText(CONCEPTS[index].name, card.x + card.width*0.5f, labelY, 20, RAYWHITE);
    DrawCenteredText(CONCEPTS[index].description, card.x + card.width*0.5f, labelY + 29.0f, 14,
                     (Color){ 174, 180, 202, 255 });
}

static void DrawSelectorUi(int screenWidth, int screenHeight, int selected, int savedSelection,
                           int hovered, const char *status)
{
    DrawRectangleGradientEx((Rectangle){ 0, 0, (float)screenWidth, (float)screenHeight },
                            (Color){ 10, 12, 22, 255 }, (Color){ 18, 16, 31, 255 },
                            (Color){ 27, 15, 38, 255 }, (Color){ 13, 19, 31, 255 });

    for (int x = 0; x < screenWidth; x += 80) {
        DrawLine(x, 0, x + 260, screenHeight, (Color){ 107, 74, 160, 12 });
    }

    DrawText("DUNGEON CRAWLER - APP-ICON AUSWAHL", 28, 23, 30, RAYWHITE);
    DrawText("1. Vorschlag anklicken   2. Auswahl speichern   3. Codex Bescheid geben", 30, 66, 18,
             (Color){ 178, 184, 207, 255 });

    for (int i = 0; i < CONCEPT_COUNT; i++) {
        DrawConceptCard(i, GetCardRectangle(i, screenWidth, screenHeight), i == hovered,
                        i == selected, i == savedSelection);
    }

    Rectangle footer = { 0, (float)screenHeight - 115.0f, (float)screenWidth, 115.0f };
    DrawRectangleRec(footer, (Color){ 11, 13, 23, 247 });
    DrawLine(0, (int)footer.y, screenWidth, (int)footer.y, (Color){ 77, 82, 111, 255 });

    if (selected >= 0) {
        DrawText(TextFormat("Markiert: %d - %s", selected + 1, CONCEPTS[selected].name), 30,
                 screenHeight - 92, 23, (Color){ 89, 255, 150, 255 });
    } else {
        DrawText("Noch kein Vorschlag markiert.", 30, screenHeight - 92, 23,
                 (Color){ 255, 205, 91, 255 });
    }
    DrawText(status, 30, screenHeight - 55, 16, (Color){ 172, 178, 201, 255 });
    DrawText("Tasten 1-9, 0, Q, W waehlen | ENTER speichert | ESC schliesst", 30, screenHeight - 29, 14,
             (Color){ 116, 123, 150, 255 });

    Rectangle saveButton = GetSaveButtonRectangle(screenWidth, screenHeight);
    bool saveHover = CheckCollisionPointRec(GetMousePosition(), saveButton);
    Color buttonColor = (selected < 0) ? (Color){ 60, 63, 76, 255 }
                                      : (saveHover ? (Color){ 112, 255, 166, 255 } : (Color){ 75, 225, 133, 255 });
    DrawRectangleRounded(saveButton, 0.16f, 12, buttonColor);
    DrawRectangleRoundedLinesEx(saveButton, 0.16f, 12, 2.0f,
                                (selected < 0) ? (Color){ 92, 96, 111, 255 } : (Color){ 181, 255, 210, 255 });
    DrawCenteredText("AUSWAHL SPEICHERN", saveButton.x + saveButton.width*0.5f, saveButton.y + 17.0f, 21,
                     (selected < 0) ? (Color){ 145, 148, 158, 255 } : (Color){ 8, 32, 20, 255 });
}

static void BuildApplicationPath(char *output, size_t outputSize, const char *filename)
{
    const char *directory = GetApplicationDirectory();
    size_t length = strlen(directory);
    bool hasSeparator = (length > 0 && (directory[length - 1] == '/' || directory[length - 1] == '\\'));
    snprintf(output, outputSize, "%s%s%s", directory, hasSeparator ? "" : "\\", filename);
}

static bool ExportSelectedIcon(int selected)
{
    char pngPath[PATH_BUFFER_SIZE];
    BuildApplicationPath(pngPath, sizeof(pngPath), "selected_icon.png");

    RenderTexture2D render = LoadRenderTexture(EXPORT_SIZE, EXPORT_SIZE);
    if (render.id == 0) return false;

    BeginTextureMode(render);
        ClearBackground(BLANK);
        DrawIconArt(selected, (Rectangle){ 0, 0, EXPORT_SIZE, EXPORT_SIZE });
    EndTextureMode();

    Image image = LoadImageFromTexture(render.texture);
    ImageFlipVertical(&image);
    bool exported = ExportImage(image, pngPath);
    UnloadImage(image);
    UnloadRenderTexture(render);
    return exported;
}

static bool SaveSelection(int selected)
{
    char jsonPath[PATH_BUFFER_SIZE];
    char selectedAt[64] = "unknown";
    time_t now = time(NULL);
    struct tm localTime;

    if (selected < 0 || selected >= CONCEPT_COUNT) return false;
    if (!ExportSelectedIcon(selected)) return false;

    if (localtime_s(&localTime, &now) == 0) {
        strftime(selectedAt, sizeof(selectedAt), "%Y-%m-%dT%H:%M:%S", &localTime);
    }

    BuildApplicationPath(jsonPath, sizeof(jsonPath), "icon_selection.json");
    FILE *file = fopen(jsonPath, "wb");
    if (file == NULL) return false;

    fprintf(file,
            "{\n"
            "  \"schema_version\": 1,\n"
            "  \"selected_id\": %d,\n"
            "  \"selected_number\": %d,\n"
            "  \"key\": \"%s\",\n"
            "  \"name\": \"%s\",\n"
            "  \"selected_at\": \"%s\",\n"
            "  \"source_png\": \"selected_icon.png\",\n"
            "  \"source_size\": 1024,\n"
            "  \"status\": \"ready_for_codex_integration\",\n"
            "  \"android_icon_changed\": false\n"
            "}\n",
            selected, selected + 1, CONCEPTS[selected].key, CONCEPTS[selected].name, selectedAt);

    bool okay = (fclose(file) == 0);
    return okay;
}

static int LoadSelection(void)
{
    char jsonPath[PATH_BUFFER_SIZE];
    char buffer[4096];
    int selected = -1;
    BuildApplicationPath(jsonPath, sizeof(jsonPath), "icon_selection.json");

    FILE *file = fopen(jsonPath, "rb");
    if (file == NULL) return -1;
    size_t bytesRead = fread(buffer, 1, sizeof(buffer) - 1, file);
    fclose(file);
    buffer[bytesRead] = '\0';

    char *position = strstr(buffer, "\"selected_id\"");
    if (position != NULL && sscanf(position, "\"selected_id\": %d", &selected) == 1 &&
        selected >= 0 && selected < CONCEPT_COUNT) {
        return selected;
    }
    return -1;
}

static bool MarkSelectionIntegrated(void)
{
    char jsonPath[PATH_BUFFER_SIZE];
    char buffer[4096];
    char selectedAt[32] = "unknown";
    int selected = LoadSelection();
    if (selected < 0 || selected >= CONCEPT_COUNT) return false;

    BuildApplicationPath(jsonPath, sizeof(jsonPath), "icon_selection.json");
    FILE *source = fopen(jsonPath, "rb");
    if (source != NULL) {
        size_t bytesRead = fread(buffer, 1, sizeof(buffer) - 1, source);
        fclose(source);
        buffer[bytesRead] = '\0';

        const char *marker = "\"selected_at\": \"";
        char *position = strstr(buffer, marker);
        if (position != NULL) {
            position += strlen(marker);
            // Keep the portable YYYY-MM-DDTHH:MM:SS portion. Older Windows CRT
            // versions expanded %z to a localized, non-UTF-8 timezone name.
            size_t length = 0;
            while (length < 19 && position[length] != '\0' && position[length] != '\"') length++;
            if (length == 19) {
                memcpy(selectedAt, position, length);
                selectedAt[length] = '\0';
            }
        }
    }

    FILE *file = fopen(jsonPath, "wb");
    if (file == NULL) return false;
    fprintf(file,
            "{\n"
            "  \"schema_version\": 1,\n"
            "  \"selected_id\": %d,\n"
            "  \"selected_number\": %d,\n"
            "  \"key\": \"%s\",\n"
            "  \"name\": \"%s\",\n"
            "  \"selected_at\": \"%s\",\n"
            "  \"source_png\": \"selected_icon.png\",\n"
            "  \"source_size\": 1024,\n"
            "  \"status\": \"integrated_into_debug_apk\",\n"
            "  \"android_icon_changed\": true,\n"
            "  \"integrated_apk\": \"../Handyversion/DungeonCrawler-Android-v1.5.0.apk\"\n"
            "}\n",
            selected, selected + 1, CONCEPTS[selected].key, CONCEPTS[selected].name, selectedAt);
    return fclose(file) == 0;
}

static bool RunRenderSelfTest(void)
{
    RenderTexture2D render = LoadRenderTexture(256, 256);
    if (render.id == 0) return false;

    bool okay = true;
    for (int i = 0; i < CONCEPT_COUNT; i++) {
        BeginTextureMode(render);
            ClearBackground(BLANK);
            DrawIconArt(i, (Rectangle){ 0, 0, 256, 256 });
        EndTextureMode();
        Image image = LoadImageFromTexture(render.texture);
        if (image.data == NULL || image.width != 256 || image.height != 256) okay = false;
        UnloadImage(image);
    }

    UnloadRenderTexture(render);
    return okay;
}

static bool ExportOverviewSheet(void)
{
    const int width = 1600;
    const int height = 1080;
    char outputPath[PATH_BUFFER_SIZE];
    RenderTexture2D render = LoadRenderTexture(width, height);
    if (render.id == 0) return false;

    BeginTextureMode(render);
        ClearBackground(BLANK);
        DrawSelectorUi(width, height, -1, -1, -1, "Vorschau - noch keine Auswahl gespeichert.");
    EndTextureMode();

    Image image = LoadImageFromTexture(render.texture);
    ImageFlipVertical(&image);
    BuildApplicationPath(outputPath, sizeof(outputPath), "Icon-Vorschlaege.png");
    bool okay = ExportImage(image, outputPath);
    UnloadImage(image);
    UnloadRenderTexture(render);
    return okay;
}

int main(int argc, char **argv)
{
    bool selfTest = (argc > 1 && strcmp(argv[1], "--self-test") == 0);
    bool exportSheet = (argc > 1 && strcmp(argv[1], "--export-sheet") == 0);
    bool saveSelfTest = (argc > 1 && strcmp(argv[1], "--save-self-test") == 0);
    bool refreshSelection = (argc > 1 && strcmp(argv[1], "--refresh-selection") == 0);
    bool markIntegrated = (argc > 1 && strcmp(argv[1], "--mark-integrated") == 0);

    SetTraceLogLevel(LOG_WARNING);
    if (selfTest || exportSheet || saveSelfTest || refreshSelection || markIntegrated) SetConfigFlags(FLAG_WINDOW_HIDDEN);
    else SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);

    InitWindow(selfTest || exportSheet || saveSelfTest || refreshSelection || markIntegrated ? 320 : 1600,
               selfTest || exportSheet || saveSelfTest || refreshSelection || markIntegrated ? 240 : 1000,
               "Dungeon Crawler - App-Icon Auswahl");

    if (!IsWindowReady()) return 1;

    if (selfTest) {
        bool okay = RunRenderSelfTest();
        CloseWindow();
        return okay ? 0 : 1;
    }

    if (exportSheet) {
        bool okay = ExportOverviewSheet();
        CloseWindow();
        return okay ? 0 : 1;
    }

    if (saveSelfTest) {
        int testSelection = CONCEPT_COUNT - 1;
        bool okay = SaveSelection(testSelection) && LoadSelection() == testSelection;
        CloseWindow();
        return okay ? 0 : 1;
    }

    if (refreshSelection) {
        int currentSelection = LoadSelection();
        bool okay = currentSelection >= 0 && ExportSelectedIcon(currentSelection);
        CloseWindow();
        return okay ? 0 : 1;
    }

    if (markIntegrated) {
        bool okay = MarkSelectionIntegrated();
        CloseWindow();
        return okay ? 0 : 1;
    }

    SetWindowMinSize(1120, 760);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    int selected = LoadSelection();
    int savedSelection = selected;
    char status[256];
    if (selected >= 0) {
        snprintf(status, sizeof(status), "Gespeicherte Auswahl geladen: %s.", CONCEPTS[selected].name);
    } else {
        snprintf(status, sizeof(status), "Die APK bleibt unveraendert, bis du einen Entwurf gespeichert hast.");
    }

    const int numberKeys[CONCEPT_COUNT] = {
        KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX,
        KEY_SEVEN, KEY_EIGHT, KEY_NINE, KEY_ZERO, KEY_Q, KEY_W
    };

    while (!WindowShouldClose()) {
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        Vector2 mouse = GetMousePosition();
        int hovered = -1;

        for (int i = 0; i < CONCEPT_COUNT; i++) {
            if (CheckCollisionPointRec(mouse, GetCardRectangle(i, screenWidth, screenHeight))) hovered = i;
            if (IsKeyPressed(numberKeys[i])) {
                selected = i;
                snprintf(status, sizeof(status), "Markiert: %s. Zum Bestaetigen jetzt speichern.", CONCEPTS[i].name);
            }
        }

        if (hovered >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selected = hovered;
            snprintf(status, sizeof(status), "Markiert: %s. Zum Bestaetigen jetzt speichern.", CONCEPTS[selected].name);
        }

        Rectangle saveButton = GetSaveButtonRectangle(screenWidth, screenHeight);
        bool saveRequested = (selected >= 0 && IsKeyPressed(KEY_ENTER)) ||
                             (selected >= 0 && CheckCollisionPointRec(mouse, saveButton) &&
                              IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
        if (saveRequested) {
            if (SaveSelection(selected)) {
                savedSelection = selected;
                snprintf(status, sizeof(status), "Gespeichert: icon_selection.json + selected_icon.png. Jetzt Codex Bescheid geben.");
            } else {
                snprintf(status, sizeof(status), "Speichern fehlgeschlagen. Bitte Ordnerrechte pruefen oder erneut versuchen.");
            }
        }

        if (IsKeyPressed(KEY_ESCAPE)) break;

        BeginDrawing();
            ClearBackground(BLACK);
            DrawSelectorUi(screenWidth, screenHeight, selected, savedSelection, hovered, status);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
