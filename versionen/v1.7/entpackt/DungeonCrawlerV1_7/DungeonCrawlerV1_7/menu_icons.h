#ifndef MENU_ICONS_H
#define MENU_ICONS_H

/*
 * Hardcoded menu icons used by the current PC build.
 *
 * These are the variants selected by the former icon picker:
 *   TUTORIAL=0  Book+Torch
 *   STATS=0     Bars
 *   SHOP=2      Shop Sign
 *   ACHIEVEMENTS=0  Medal
 *   CHALLENGES=0    Swords
 *
 * The challenge list also intentionally uses the other five challenge glyphs,
 * so all six challenge variants remain here.  There is no runtime file
 * loading, voting, or remixing anymore.
 */

#include "raylib.h"
#include <math.h>

enum {
    IC_TUT = 0,
    IC_STAT,
    IC_SHOP,
    IC_ACH,
    IC_CHAL
};

static const Color COL_TUT  = {  90, 175, 255, 255 };
static const Color COL_STAT = { 255, 200,  50, 255 };
static const Color COL_SHOP = {  65, 210, 130, 255 };
static const Color COL_ACH  = { 255, 200,  50, 255 };
static const Color COL_CHAL = { 185,  85, 255, 255 };

static Color MenuIconDim(Color c, float f) {
    return (Color){ (unsigned char)(c.r * f), (unsigned char)(c.g * f),
                    (unsigned char)(c.b * f), c.a };
}

static void MenuIconStar(float cx, float cy, float r, Color c) {
    Vector2 pts[10];
    for (int i = 0; i < 10; i++) {
        float a = -PI * 0.5f + (float)i * PI / 5.0f;
        float rr = (i & 1) ? r * 0.42f : r;
        pts[i] = (Vector2){ cx + cosf(a) * rr, cy + sinf(a) * rr };
    }
    Vector2 mid = { cx, cy };
    for (int i = 0; i < 10; i++)
        DrawTriangle(mid, pts[i], pts[(i + 1) % 10], c);
}

static void MenuIconBook(float cx, float cy, float s, Color c, float t) {
    float bob = sinf(t * 2.2f) * s * 0.03f;
    cy += bob;
    Color page = { 220, 230, 255, 255 };
    float bw = s * 1.74f;
    float bh = s * 1.22f;
    float orbR = s * 0.10f;
    float stem = s * 0.05f;
    float totalH = orbR * 2.0f + stem + bh;
    float top = cy - totalH * 0.5f;
    float orbY = top + orbR;
    float bookTop = orbY + orbR + stem;
    DrawRectangleRounded((Rectangle){ cx - bw * 0.5f, bookTop, bw, bh },
                         0.16f, 6, MenuIconDim(c, 0.55f));
    float pw = bw * 0.38f, ph = bh * 0.82f;
    float pageY = bookTop + bh * 0.10f;
    DrawRectangleRounded((Rectangle){ cx - bw * 0.44f, pageY, pw, ph }, 0.12f, 4, page);
    DrawRectangleRounded((Rectangle){ cx + bw * 0.06f, pageY, pw, ph }, 0.12f, 4, page);
    DrawRectangleRounded((Rectangle){ cx - s * 0.08f, bookTop + bh * 0.04f,
                                     s * 0.16f, bh * 0.92f },
                         0.4f, 4, c);
    for (int i = 0; i < 3; i++) {
        float y = pageY + ph * (0.26f + i * 0.22f);
        DrawLineEx((Vector2){ cx - bw * 0.36f, y },
                   (Vector2){ cx - bw * 0.12f, y }, 2.0f,
                   (Color){ 140, 165, 210, 180 });
        DrawLineEx((Vector2){ cx + bw * 0.12f, y },
                   (Vector2){ cx + bw * 0.36f, y }, 2.0f,
                   (Color){ 140, 165, 210, 180 });
    }
    DrawRectangleRounded((Rectangle){
        cx - s * 0.05f, orbY, s * 0.10f,
        (bookTop + bh * 0.20f) - orbY
    }, 0.3f, 3, c);
    DrawCircleV((Vector2){ cx, orbY }, orbR, (Color){ 90, 200, 255, 255 });
    DrawCircleV((Vector2){ cx, orbY - orbR * 0.22f }, orbR * 0.42f,
                (Color){ 210, 245, 255, 255 });
}

static void MenuIconBars(float cx, float cy, float s, Color c, float t) {
    float h[3] = { 0.42f, 0.68f, 0.95f };
    float pulse = 0.5f + 0.5f * sinf(t * 2.4f);
    float base = cy + s * 0.55f;
    DrawLineEx((Vector2){ cx - s * 0.70f, base },
               (Vector2){ cx + s * 0.70f, base }, s * 0.08f,
               MenuIconDim(c, 0.7f));
    for (int i = 0; i < 3; i++) {
        float bh = s * h[i] * (0.82f + 0.18f * pulse);
        float x = cx - s * 0.42f + i * s * 0.42f;
        DrawRectangleRounded((Rectangle){ x - s * 0.14f, base - bh,
                                         s * 0.28f, bh },
                             0.18f, 4, c);
    }
    DrawCircleV((Vector2){ cx + s * 0.42f,
                           base - s * 0.95f * (0.82f + 0.18f * pulse) - s * 0.16f },
                s * 0.16f, c);
}

static void MenuIconShopSign(float cx, float cy, float s, Color c, float t) {
    float sway = sinf(t * 1.7f) * 0.08f;
    DrawLineEx((Vector2){ cx, cy - s * 0.95f },
               (Vector2){ cx, cy + s * 0.85f }, s * 0.12f,
               (Color){ 90, 60, 30, 255 });
    float x = cx + sway * s;
    DrawRectangleRounded((Rectangle){ x - s * 0.72f, cy - s * 0.55f,
                                     s * 1.44f, s * 0.70f },
                         0.12f, 4, c);
    DrawCircleV((Vector2){ x, cy - s * 0.20f }, s * 0.18f, COL_STAT);
}

static void MenuIconMedal(float cx, float cy, float s, Color c, float t) {
    float spin = sinf(t * 1.8f) * 0.04f;
    DrawTriangle((Vector2){ cx - s * 0.18f, cy + s * 0.10f },
                 (Vector2){ cx - s * 0.42f, cy + s * 0.78f },
                 (Vector2){ cx - s * 0.02f, cy + s * 0.52f },
                 MenuIconDim(c, 0.75f));
    DrawTriangle((Vector2){ cx + s * 0.18f, cy + s * 0.10f },
                 (Vector2){ cx + s * 0.02f, cy + s * 0.52f },
                 (Vector2){ cx + s * 0.42f, cy + s * 0.78f },
                 MenuIconDim(c, 0.75f));
    DrawCircleV((Vector2){ cx + spin * s, cy - s * 0.08f }, s * 0.58f, c);
    DrawCircleV((Vector2){ cx + spin * s, cy - s * 0.08f }, s * 0.44f,
                MenuIconDim(c, 0.72f));
    MenuIconStar(cx + spin * s, cy - s * 0.08f, s * 0.30f,
                 (Color){ 255, 245, 200, 255 });
}

static void MenuIconSwords(float cx, float cy, float s, Color c, float t) {
    float pulse = 0.5f + 0.5f * sinf(t * 2.6f);
    Color hi = {
        (unsigned char)fminf(255.0f, c.r + 40.0f),
        (unsigned char)fminf(255.0f, c.g + 30.0f),
        (unsigned char)fminf(255.0f, c.b + 40.0f), 255
    };
    for (int k = 0; k < 2; k++) {
        float ang = (k == 0 ? -38.0f : 38.0f);
        DrawRectanglePro((Rectangle){ cx, cy, s * 1.55f, s * 0.22f },
                         (Vector2){ s * 0.78f, s * 0.11f }, ang,
                         k ? hi : c);
        DrawRectanglePro((Rectangle){ cx, cy, s * 0.42f, s * 0.38f },
                         (Vector2){ s * 0.08f, s * 0.19f }, ang,
                         MenuIconDim(c, 0.65f));
        DrawRectanglePro((Rectangle){ cx, cy, s * 0.22f, s * 0.22f },
                         (Vector2){ s * -0.18f, s * 0.11f }, ang, hi);
    }
    DrawCircleV((Vector2){ cx, cy }, s * 0.12f * (0.8f + 0.4f * pulse),
                (Color){ 255, 230, 255, 220 });
}

static void MenuIconTarget(float cx, float cy, float s, Color c, float t) {
    float pulse = 0.5f + 0.5f * sinf(t * 2.8f);
    DrawCircleV((Vector2){ cx, cy }, s * 0.82f, c);
    DrawCircleV((Vector2){ cx, cy }, s * 0.58f, (Color){ 16, 12, 28, 255 });
    DrawCircleV((Vector2){ cx, cy }, s * 0.36f, c);
    DrawCircleV((Vector2){ cx, cy }, s * 0.16f * (0.8f + 0.3f * pulse),
                (Color){ 16, 12, 28, 255 });
}

static void MenuIconFlag(float cx, float cy, float s, Color c, float t) {
    float wave = sinf(t * 2.5f) * s * 0.08f;
    DrawLineEx((Vector2){ cx - s * 0.45f, cy - s * 0.85f },
               (Vector2){ cx - s * 0.45f, cy + s * 0.85f }, s * 0.12f,
               (Color){ 200, 200, 220, 255 });
    DrawTriangle((Vector2){ cx - s * 0.38f, cy - s * 0.82f },
                 (Vector2){ cx + s * 0.70f + wave, cy - s * 0.40f },
                 (Vector2){ cx - s * 0.38f, cy + s * 0.05f }, c);
}

static void MenuIconHourglass(float cx, float cy, float s, Color c, float t) {
    float sand = 0.5f + 0.5f * sinf(t * 1.4f);
    DrawTriangle((Vector2){ cx - s * 0.55f, cy - s * 0.75f },
                 (Vector2){ cx + s * 0.55f, cy - s * 0.75f },
                 (Vector2){ cx, cy - s * 0.02f }, c);
    DrawTriangle((Vector2){ cx - s * 0.55f, cy + s * 0.75f },
                 (Vector2){ cx + s * 0.55f, cy + s * 0.75f },
                 (Vector2){ cx, cy + s * 0.02f }, c);
    DrawCircleV((Vector2){ cx, cy - s * 0.38f }, s * 0.16f * sand,
                (Color){ 255, 230, 140, 255 });
    DrawEllipse((int)cx, (int)(cy + s * 0.48f),
                (int)(s * 0.28f * (1.1f - sand)), (int)(s * 0.10f),
                (Color){ 255, 230, 140, 255 });
}

static void MenuIconFist(float cx, float cy, float s, Color c, float t) {
    float punch = sinf(t * 3.0f) * s * 0.05f;
    cx += punch;
    DrawRectangleRounded((Rectangle){ cx - s * 0.55f, cy - s * 0.35f,
                                     s * 1.20f, s * 0.80f },
                         0.35f, 6, c);
    for (int i = 0; i < 4; i++) {
        float x = cx - s * 0.35f + i * s * 0.24f;
        DrawRectangleRounded((Rectangle){ x, cy - s * 0.62f,
                                         s * 0.20f, s * 0.38f },
                             0.4f, 4, c);
    }
}

static void MenuIconPeak(float cx, float cy, float s, Color c, float t) {
    float snow = 0.5f + 0.5f * sinf(t * 1.5f);
    DrawTriangle((Vector2){ cx - s * 0.95f, cy + s * 0.70f },
                 (Vector2){ cx + s * 0.95f, cy + s * 0.70f },
                 (Vector2){ cx, cy - s * 0.85f }, c);
    DrawTriangle((Vector2){ cx - s * 0.22f, cy - s * 0.35f },
                 (Vector2){ cx + s * 0.22f, cy - s * 0.35f },
                 (Vector2){ cx, cy - s * 0.85f },
                 (Color){ 245, 245, 255, (unsigned char)(180 + 50 * snow) });
}

/* Fixed selection from the former icon picker. */
static int SelectedMenuIconVariant(int cat) {
    return cat == IC_SHOP ? 2 : 0;
}

/* Draw only the icon variants that are used by the live game. */
static void DrawMenuGlyph(int cat, int var, float cx, float cy, float s,
                          Color c, float t) {
    if (var < 0) var = 0;
    if (cat == IC_TUT) {
        MenuIconBook(cx, cy, s, c, t);
        return;
    }
    if (cat == IC_STAT) {
        MenuIconBars(cx, cy, s, c, t);
        return;
    }
    if (cat == IC_SHOP) {
        MenuIconShopSign(cx, cy, s, c, t);
        return;
    }
    if (cat == IC_ACH) {
        MenuIconMedal(cx, cy, s, c, t);
        return;
    }
    if (cat != IC_CHAL) return;

    switch (var % 6) {
        case 0: MenuIconSwords(cx, cy, s, c, t); break;
        case 1: MenuIconTarget(cx, cy, s, c, t); break;
        case 2: MenuIconFlag(cx, cy, s, c, t); break;
        case 3: MenuIconHourglass(cx, cy, s, c, t); break;
        case 4: MenuIconFist(cx, cy, s, c, t); break;
        default: MenuIconPeak(cx, cy, s, c, t); break;
    }
}

#endif /* MENU_ICONS_H */
