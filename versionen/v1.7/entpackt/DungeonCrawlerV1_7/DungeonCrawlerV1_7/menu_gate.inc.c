/* Included from game_ui.c — dungeon-gate PLAY door and menu layout helpers. */

#define DESIGN_W 1280.0f
#define DESIGN_H  720.0f
#define EllipseRot DrawBeamedGearEllipseRot
#define Lighten    MusicGearLighten

typedef struct {
    int sw, sh;
    float s;
} MenuUI;

static MenuUI MakeUI(void) {
    MenuUI u;
    u.sw = GetScreenWidth();
    u.sh = GetScreenHeight();
    float sx = (float)u.sw / DESIGN_W;
    float sy = (float)u.sh / DESIGN_H;
    u.s = (sx < sy) ? sx : sy;
    if (u.s < 0.45f) u.s = 0.45f;
    return u;
}

static int MenuPx(MenuUI u, float v) { return (int)(v * u.s + 0.5f); }
static float MenuPxf(MenuUI u, float v) { return v * u.s; }

static Color WithA(Color c, int a) {
    if (a < 0) a = 0;
    if (a > 255) a = 255;
    return (Color){ c.r, c.g, c.b, (uchar)a };
}

static Color PlayAccent(float t) {
    float cyc = fmodf(t * 0.55f, 1.0f);
    Color cols[5] = { CTRES, CENE, CPORT, CWLK, CPL };
    float pos5 = cyc * 5.0f;
    int idx0 = (int)pos5 % 5;
    Color cA = cols[idx0], cB = cols[(idx0 + 1) % 5];
    float frac = pos5 - (float)(int)pos5;
    return (Color){
        (uchar)(cA.r + (cB.r - cA.r) * frac),
        (uchar)(cA.g + (cB.g - cA.g) * frac),
        (uchar)(cA.b + (cB.b - cA.b) * frac), 255
    };
}

static Vector2 V2(float x, float y) { return (Vector2){ x, y }; }

static Vector2 LerpV(Vector2 a, Vector2 b, float u) {
    return V2(a.x + (b.x - a.x) * u, a.y + (b.y - a.y) * u);
}

static void FillQuad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color col) {
    DrawTriangle(a, b, c, col);
    DrawTriangle(a, c, d, col);
    DrawTriangle(a, c, b, col);
    DrawTriangle(a, d, c, col);
}

static float EaseOutQuart(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    float i = 1.0f - x;
    return 1.0f - i * i * i * i;
}

static float EaseInQuad(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x * x;
}

static float Hash21(float x, float y) {
    float n = sinf(x * 127.1f + y * 311.7f) * 43758.5453f;
    return n - floorf(n);
}

static float DoorOpenK(bool hover) {
    (void)hover;
    if (menuEnter < 0.0f) return 0.0f;
    if (menuEnter > 1.55f) return 0.0f;
    if (menuEnter < 0.12f) return 0.0f;
    return EaseOutQuart((menuEnter - 0.12f) / 0.62f);
}

static float DoorOpenKSide(int side, bool hover) {
    float delay = (side > 0) ? 0.08f : 0.0f;
    if (menuEnter < 0.0f) return DoorOpenK(hover);
    if (menuEnter > 1.55f) return 0.0f;
    float te = menuEnter - delay;
    if (te < 0.12f) return 0.0f;
    return EaseOutQuart((te - 0.12f) / 0.62f);
}

static float DoorRushK(void) {
    if (menuEnter < 0.56f) return 0.0f;
    float k = (menuEnter - 0.56f) / 0.30f;
    if (k > 1.0f) return 1.0f;
    return 0.78f * k + 0.22f * k * k;
}

static float DoorOverlayK(void) {
    if (menuEnter < 0.0f) return 0.0f;
    if (menuEnter < 0.80f) return 0.0f;
    if (menuEnter < 1.00f) return EaseInQuad((menuEnter - 0.80f) / 0.20f);
    return 1.0f;
}

static bool menuWooshSfxPlayed = false;

void MenuPlayBegin(void) {
    if (skip_play_anim) {
        menuEnterFire = true;
        menuEnter = -1.0f;
        return;
    }
    if (menuEnter < 0.0f) {
        menuEnter = 0.0f;
        menuWooshSfxPlayed = false;
        RequestGameplayMusicStartDelay();
        PlaySfx(SFX_TITLE_DOOR_OPEN);
    }
}

bool MenuPlayBusy(void) { return menuEnter >= 0.0f || menuEnterFire; }

bool MenuPlayShouldStart(void) {
    if (!menuEnterFire) return false;
    menuEnterFire = false;
    return true;
}

void UpdateMenuPlayAnim(void) {
    if (menuEnter < 0.0f) return;
    menuEnter += GetFrameTime();
    if (!menuWooshSfxPlayed && menuEnter >= 0.50f) {
        menuWooshSfxPlayed = true;
        PlaySfx(SFX_TITLE_WOOSH);
    }
    if (menuEnter >= 1.22f) {
        menuEnterFire = true;
        menuEnter = -1.0f;
    }
}

static void LayoutTopChips(MenuUI u, Rectangle out[MN_COUNT]) {
    float chip = MenuPxf(u, 76);
    float gap = MenuPxf(u, 10);
    float x0 = MenuPxf(u, 10);
    float y0 = MenuPxf(u, 16);
    out[MN_SETTINGS] = (Rectangle){ x0, y0, chip, chip };
    out[MN_MUSIC]    = (Rectangle){ x0 + (chip + gap) * 1.0f, y0, chip, chip };
    out[MN_TUTORIAL] = (Rectangle){ x0 + (chip + gap) * 2.0f, y0, chip, chip };
    out[MN_FUTURE]   = (Rectangle){ x0 + (chip + gap) * 3.0f, y0, chip, chip };
}

static void LayoutTitle(MenuUI u, float chipRight, int *ts, int *tx, int *ty) {
    const char *TT = "DUNGEON CRAWLER";
    int chipY = MenuPx(u, 16);
    int chipH = MenuPx(u, 76);
    int gap = MenuPx(u, 16);
    int verW = MeasureText("v1.6", MenuPx(u, 13)) + MenuPx(u, 44);
    int leftLimit = (int)chipRight + gap;
    int rightLimit = u.sw - verW - gap;
    int size = MenuPx(u, 68);
    int minS = MenuPx(u, 26);
    for (;;) {
        int tw = MeasureText(TT, size);
        int x = u.sw / 2 - tw / 2;
        if (x >= leftLimit && x + tw <= rightLimit) break;
        if (size <= minS) break;
        size--;
    }
    *ts = size;
    *tx = u.sw / 2 - MeasureText(TT, size) / 2;
    *ty = chipY + (chipH - size) / 2;
    if (*ty < chipY) *ty = chipY;
}

static int TitleBottomY(MenuUI u, float chipRight) {
    int ts, tx, ty;
    LayoutTitle(u, chipRight, &ts, &tx, &ty);
    int chipBottom = MenuPx(u, 16) + MenuPx(u, 76);
    int tb = ty + ts + MenuPx(u, 12);
    if (tb < chipBottom + MenuPx(u, 8)) tb = chipBottom + MenuPx(u, 8);
    return tb;
}

typedef struct {
    Vector2 hingeTop, hingeBot, innerTop, innerBot;
    float visW, open;
    int side;
} DoorLeaf;

static DoorLeaf MakeLeaf(Rectangle inner, int side, float open) {
    DoorLeaf L;
    L.side = side;
    L.open = open;
    float ang = open * 1.22f;
    float fs = (open < 0.001f) ? 1.0f : cosf(ang);
    if (fs < 0.08f) fs = 0.08f;
    L.visW = inner.width * 0.5f * fs;
    if (open < 0.001f) L.visW += 2.0f;
    float persp = (open < 0.001f) ? 0.0f : inner.height * 0.032f * sinf(ang);
    float thick = (open < 0.001f) ? 0.0f : fmaxf(L.visW * 0.18f, inner.height * 0.04f) * sinf(ang);
    if (side < 0) {
        L.hingeTop = V2(inner.x, inner.y);
        L.hingeBot = V2(inner.x, inner.y + inner.height);
        L.innerTop = V2(inner.x + L.visW, inner.y + persp);
        L.innerBot = V2(inner.x + L.visW, inner.y + inner.height - persp);
        if (thick > 1.0f) {
            L.innerTop.x += thick * 0.15f;
            L.innerBot.x += thick * 0.15f;
        }
    } else {
        L.hingeTop = V2(inner.x + inner.width, inner.y);
        L.hingeBot = V2(inner.x + inner.width, inner.y + inner.height);
        L.innerTop = V2(inner.x + inner.width - L.visW, inner.y + persp);
        L.innerBot = V2(inner.x + inner.width - L.visW, inner.y + inner.height - persp);
    }
    return L;
}

static Vector2 LeafUV(DoorLeaf L, float u, float v) {
    Vector2 a = LerpV(L.hingeTop, L.innerTop, u);
    Vector2 b = LerpV(L.hingeBot, L.innerBot, u);
    return LerpV(a, b, v);
}

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
            float uu = (float)s / SUB, u2 = uu*uu, u3 = u2*uu;
            Vector2 q = {
                0.5f*((2*p1.x) + (-p0.x+p2.x)*uu + (2*p0.x-5*p1.x+4*p2.x-p3.x)*u2 + (-p0.x+3*p1.x-3*p2.x+p3.x)*u3),
                0.5f*((2*p1.y) + (-p0.y+p2.y)*uu + (2*p0.y-5*p1.y+4*p2.y-p3.y)*u2 + (-p0.y+3*p1.y-3*p2.y+p3.y)*u3)
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

static void DrawFlameTongue(float bx, float by, float h, float w, float t, float seed) {
    float flick = 0.78f + 0.22f * sinf(t * 12.7f + seed);
    float sway  = sinf(t * 7.4f + seed * 1.8f) * w * 0.55f;
    float H = h * flick;
    Vector2 tip = V2(bx + sway, by - H);
    DrawTriangle(V2(bx - w, by), V2(bx + w, by), tip, (Color){ 255, 70, 12, 190 });
    DrawTriangle(V2(bx + w, by), V2(bx - w, by), tip, (Color){ 255, 70, 12, 190 });
    EllipseRot(bx, by - H * 0.08f, w * 0.95f, H * 0.22f, 0.0f, (Color){ 255, 80, 16, 150 });
    Vector2 midTip = V2(bx + sway * 0.72f, by - H * 0.82f);
    DrawTriangle(V2(bx - w * 0.62f, by), V2(bx + w * 0.62f, by), midTip, (Color){ 255, 150, 30, 210 });
    DrawTriangle(V2(bx + w * 0.62f, by), V2(bx - w * 0.62f, by), midTip, (Color){ 255, 150, 30, 210 });
    Vector2 coreTip = V2(bx + sway * 0.40f, by - H * 0.56f);
    DrawTriangle(V2(bx - w * 0.30f, by + H * 0.02f), V2(bx + w * 0.30f, by + H * 0.02f),
                 coreTip, (Color){ 255, 230, 120, 220 });
    DrawTriangle(V2(bx + w * 0.30f, by + H * 0.02f), V2(bx - w * 0.30f, by + H * 0.02f),
                 coreTip, (Color){ 255, 230, 120, 220 });
    DrawCircleV(V2(bx + sway * 0.15f, by - H * 0.12f), w * 0.28f, (Color){ 255, 250, 220, 150 });
}

static void DrawGateSkull(float cx, float cy, float R, float t) {
    Color bone   = (Color){ 231, 226, 212, 255 };
    Color boneLo = (Color){ 193, 187, 172, 255 };
    Color boneHi = (Color){ 247, 244, 235, 255 };
    Color socket = (Color){  20,  10,   8, 255 };
    Color dark   = (Color){  24,  20,  21, 255 };
    Color ember  = (Color){ 255, 110,  30, 255 };

    float bob    = sinf(t * 2.2f) * R * 0.04f;
    float cyb    = cy + bob;
    float pulse  = 0.5f + 0.5f * sinf(t * 4.0f);

    DrawSkullShape(cx, cyb + R * 0.03f, R * 1.03f, boneLo);
    DrawSkullShape(cx, cyb, R, bone);

    for (int i = 0; i < 4; i++) {
        float k = 1.0f - i * 0.18f;
        EllipseRot(cx - R * 0.10f, cyb - R * 0.46f, R * 0.58f * k, R * 0.44f * k, 0.0f,
                   (Color){ boneHi.r, boneHi.g, boneHi.b, 42 });
    }
    EllipseRot(cx, cyb + R * 0.58f, R * 0.52f, R * 0.26f, 0.0f,
               (Color){ boneLo.r, boneLo.g, boneLo.b, 110 });

    EllipseRot(cx - R * 0.44f, cyb - R * 0.07f, R * 0.48f, R * 0.42f,  0.32f, (Color){ 150, 144, 132, 120 });
    EllipseRot(cx + R * 0.44f, cyb - R * 0.07f, R * 0.48f, R * 0.42f, -0.32f, (Color){ 150, 144, 132, 120 });
    EllipseRot(cx - R * 0.44f, cyb - R * 0.08f, R * 0.42f, R * 0.36f,  0.32f, socket);
    EllipseRot(cx + R * 0.44f, cyb - R * 0.08f, R * 0.42f, R * 0.36f, -0.32f, socket);

    float er = R * (0.10f + 0.05f * pulse);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV((Vector2){ cx - R * 0.40f, cyb - R * 0.04f }, er * 2.2f,
                (Color){ ember.r, ember.g, ember.b, (uchar)(40 + 40 * pulse) });
    DrawCircleV((Vector2){ cx + R * 0.40f, cyb - R * 0.04f }, er * 2.2f,
                (Color){ ember.r, ember.g, ember.b, (uchar)(40 + 40 * pulse) });
    DrawCircleV((Vector2){ cx - R * 0.40f, cyb - R * 0.04f }, er,
                (Color){ ember.r, ember.g, ember.b, (uchar)(90 + 100 * pulse) });
    DrawCircleV((Vector2){ cx + R * 0.40f, cyb - R * 0.04f }, er,
                (Color){ ember.r, ember.g, ember.b, (uchar)(90 + 100 * pulse) });
    DrawCircleV((Vector2){ cx - R * 0.40f, cyb - R * 0.04f }, er * 0.45f,
                (Color){ 255, 220, 120, (uchar)(120 + 80 * pulse) });
    DrawCircleV((Vector2){ cx + R * 0.40f, cyb - R * 0.04f }, er * 0.45f,
                (Color){ 255, 220, 120, (uchar)(120 + 80 * pulse) });
    DrawFlameTongue(cx - R * 0.40f, cyb - R * 0.02f, R * (0.62f + 0.14f * pulse), R * 0.14f, t, 4.2f);
    DrawFlameTongue(cx + R * 0.40f, cyb - R * 0.02f, R * (0.62f + 0.14f * pulse), R * 0.14f, t, 5.1f);
    DrawFlameTongue(cx - R * 0.40f, cyb + R * 0.04f, R * 0.40f, R * 0.08f, t, 6.8f);
    DrawFlameTongue(cx + R * 0.40f, cyb + R * 0.04f, R * 0.40f, R * 0.08f, t, 7.6f);
    EndBlendMode();
    DrawCircleV((Vector2){ cx - R * 0.34f, cyb + R * 0.10f }, R * 0.05f, (Color){ 55, 51, 53, 150 });
    DrawCircleV((Vector2){ cx + R * 0.34f, cyb + R * 0.10f }, R * 0.05f, (Color){ 55, 51, 53, 150 });

    DrawTriangle((Vector2){ cx, cyb + R * 0.44f },
                 (Vector2){ cx - R * 0.15f, cyb + R * 0.20f },
                 (Vector2){ cx + R * 0.15f, cyb + R * 0.20f }, dark);
    DrawTriangle((Vector2){ cx, cyb + R * 0.44f },
                 (Vector2){ cx + R * 0.15f, cyb + R * 0.20f },
                 (Vector2){ cx - R * 0.15f, cyb + R * 0.20f }, dark);
    DrawCircleV((Vector2){ cx - R * 0.08f, cyb + R * 0.22f }, R * 0.075f, dark);
    DrawCircleV((Vector2){ cx + R * 0.08f, cyb + R * 0.22f }, R * 0.075f, dark);

    float chatter = sinf(t * 12.0f) * R * 0.012f;
    float mx = cx - R * 0.46f, mouthW = R * 0.92f;
    float myT = cyb + R * 0.62f + chatter, myB = cyb + R * 1.00f + chatter;
    DrawRectangleRounded((Rectangle){ mx, myT, mouthW, myB - myT }, 0.35f, 6, dark);
    int nt = 6;
    float tw = mouthW / nt;
    for (int k = 0; k < nt; k++) {
        float tx = mx + k * tw;
        DrawRectangleRounded(
            (Rectangle){ tx + R * 0.03f, myT + R * 0.03f, tw - R * 0.06f, (myB - myT) - R * 0.06f },
            0.30f, 4, boneHi);
    }

    for (int k = 0; k < 6; k++) {
        float pr = fmodf(t * 0.7f + SkullRnd(k + 3), 1.0f);
        float side = (k % 2) ? 1.0f : -1.0f;
        DrawCircleV((Vector2){ cx + side * R * 0.40f + (SkullRnd(k) - 0.5f) * R * 0.1f,
                              cyb - R * 0.05f - pr * R * 1.0f },
                    R * (0.05f - pr * 0.03f),
                    (Color){ 255, 140, 40, (uchar)(200 * (1.0f - pr)) });
    }
}

static void DrawRecessedPanel(DoorLeaf L, float u0, float v0, float u1, float v1, Color accent, float pulse) {
    Vector2 a = LeafUV(L, u0, v0), b = LeafUV(L, u0, v1);
    Vector2 c = LeafUV(L, u1, v1), d = LeafUV(L, u1, v0);
    FillQuad(a, b, c, d, (Color){ 8, 6, 14, 255 });
    Vector2 a2 = LeafUV(L, u0 + 0.04f, v0 + 0.035f);
    Vector2 b2 = LeafUV(L, u0 + 0.04f, v1 - 0.035f);
    Vector2 c2 = LeafUV(L, u1 - 0.04f, v1 - 0.035f);
    Vector2 d2 = LeafUV(L, u1 - 0.04f, v0 + 0.035f);
    FillQuad(a2, b2, c2, d2, (Color){ 20, 15, 32, 255 });
    DrawLineEx(a2, d2, 1.8f, (Color){ 48, 40, 64, 160 });
    DrawLineEx(a2, b2, 1.8f, (Color){ 6, 4, 10, 200 });
    DrawLineEx(d2, c2, 1.4f, (Color){ 6, 4, 10, 140 });
    DrawLineEx(a2, d2, 1.0f, WithA(accent, (int)(18 + 22 * pulse)));
}

static void DrawStrapHinge(DoorLeaf L, float v, Color accent, float pulse) {
    Vector2 a = LeafUV(L, 0.00f, v - 0.032f);
    Vector2 b = LeafUV(L, 0.00f, v + 0.032f);
    Vector2 c = LeafUV(L, 0.58f, v + 0.024f);
    Vector2 d = LeafUV(L, 0.58f, v - 0.024f);
    Color metal = { 42, 38, 58, 255 };
    FillQuad(a, b, c, d, metal);
    DrawLineEx(a, d, 1.7f, Lighten(metal, 0.22f));
    DrawLineEx(b, c, 1.5f, (Color){ 16, 12, 24, 200 });
    Vector2 tip = LeafUV(L, 0.68f, v);
    DrawTriangle(c, d, tip, metal);
    DrawLineEx(d, tip, 1.4f, Lighten(metal, 0.18f));
    for (int i = 0; i < 3; i++) {
        Vector2 p = LeafUV(L, 0.12f + i * 0.16f, v);
        DrawCircleV(p, 3.6f, (Color){ 64, 58, 82, 255 });
        DrawCircleV(p, 3.6f, WithA(accent, (int)(10 + 14 * pulse)));
        DrawCircleV(p, 1.5f, (Color){ 14, 10, 20, 255 });
        DrawCircleV(V2(p.x - 0.8f, p.y - 0.8f), 0.7f, (Color){ 120, 112, 140, 140 });
    }
    Vector2 barrel = LeafUV(L, 0.015f, v);
    DrawCircleV(barrel, 5.4f, (Color){ 28, 24, 40, 255 });
    DrawCircleV(barrel, 3.2f, metal);
    DrawCircleV(barrel, 1.4f, (Color){ 10, 8, 16, 255 });
}

static void StrokeEllipse(Vector2 c, float rx, float ry, float ang, float thick, Color col) {
    const int SEG = 40;
    float ca = cosf(ang), sa = sinf(ang);
    Vector2 prev = { 0 };
    for (int i = 0; i <= SEG; i++) {
        float th = (float)i / (float)SEG * 2.0f * PI;
        float ex = cosf(th) * rx, ey = sinf(th) * ry;
        Vector2 p = { c.x + ex * ca - ey * sa, c.y + ex * sa + ey * ca };
        if (i > 0) DrawLineEx(prev, p, thick, col);
        prev = p;
    }
}

static void DrawRingKnocker(DoorLeaf L, float t, Color accent) {
    float u = 0.88f;
    float v = 0.76f;
    Vector2 plate = LeafUV(L, u, v);
    float h = L.innerBot.y - L.hingeTop.y;
    float squash = (L.open < 0.001f) ? 1.0f : fmaxf(0.16f, cosf(L.open * 1.22f));
    float plateR = h * 0.036f;
    float ringR  = h * 0.058f;
    float thick  = fmaxf(3.4f, h * 0.015f);

    Color iron   = { 92, 86, 112, 255 };
    Color ironHi = { 158, 150, 176, 255 };
    Color ironLo = { 22, 18, 32, 255 };
    Color plateC = { 48, 44, 64, 255 };

    Vector2 a = LeafUV(L, u - 0.075f, v - 0.048f);
    Vector2 b = LeafUV(L, u - 0.075f, v + 0.048f);
    Vector2 c = LeafUV(L, u + 0.075f, v + 0.048f);
    Vector2 d = LeafUV(L, u + 0.075f, v - 0.048f);
    FillQuad(a, b, c, d, plateC);
    DrawLineEx(a, d, 2.2f, Lighten(plateC, 0.30f));
    DrawLineEx(b, c, 2.2f, ironLo);
    DrawLineEx(a, b, 1.6f, Lighten(plateC, 0.12f));
    DrawLineEx(d, c, 1.6f, ironLo);

    float ru[4] = { -0.052f, -0.052f, 0.052f, 0.052f };
    float rv[4] = { -0.032f,  0.032f, -0.032f, 0.032f };
    for (int i = 0; i < 4; i++) {
        Vector2 p = LeafUV(L, u + ru[i], v + rv[i]);
        DrawCircleV(p, 3.4f, iron);
        DrawCircleV(p, 1.4f, ironLo);
        DrawCircleV(V2(p.x - 0.8f, p.y - 0.8f), 0.8f, ironHi);
    }

    DrawCircleV(plate, plateR, ironLo);
    DrawCircleV(plate, plateR * 0.82f, iron);
    DrawCircleV(V2(plate.x - plateR * 0.22f, plate.y - plateR * 0.22f), plateR * 0.30f,
                (Color){ ironHi.r, ironHi.g, ironHi.b, 100 });
    DrawCircleV(plate, plateR * 0.20f, ironLo);

    Vector2 staple = V2(plate.x, plate.y - plateR * 0.12f);
    float swing = sinf(t * 1.7f + (float)L.side * 1.4f) * 0.08f;
    if (menuEnter >= 0.12f && L.open > 0.01f && L.open < 0.95f)
        swing += sinf(menuEnter * 20.0f + (float)L.side) * 0.28f * (1.0f - L.open);

    float hang = ringR * 0.90f;
    Vector2 rc = V2(staple.x + sinf(swing) * hang * squash,
                    staple.y + cosf(swing) * hang);

    StrokeEllipse(V2(rc.x + 1.4f, rc.y + 1.8f), ringR * squash, ringR, swing, thick + 1.6f, ironLo);
    StrokeEllipse(rc, ringR * squash, ringR, swing, thick, iron);
    StrokeEllipse(rc, (ringR - thick * 0.38f) * squash, ringR - thick * 0.38f, swing,
                  thick * 0.42f, ironLo);
    StrokeEllipse(V2(rc.x - thick * 0.20f, rc.y - thick * 0.22f),
                  (ringR - thick * 0.18f) * squash, ringR - thick * 0.18f, swing,
                  thick * 0.30f, (Color){ ironHi.r, ironHi.g, ironHi.b, 170 });

    DrawRectangleRounded((Rectangle){
        staple.x - thick * 0.75f, staple.y - thick * 1.05f,
        thick * 1.50f, thick * 2.10f
    }, 0.35f, 4, iron);
    DrawCircleV(staple, thick * 0.58f, ironHi);
    DrawCircleV(staple, thick * 0.24f, ironLo);

    Vector2 strike = V2(plate.x, plate.y + plateR * 0.92f);
    DrawCircleV(strike, thick * 0.55f, iron);
    DrawCircleV(strike, thick * 0.22f, ironLo);
    DrawCircleV(plate, 2.0f, WithA(accent, 55));
}

static void DrawVaultLeaf(DoorLeaf L, float t, Color accent, bool hover) {
    float pulse = 0.45f + 0.55f * sinf(t * 1.55f + (float)L.side);
    FillQuad(L.hingeTop, L.hingeBot, L.innerBot, L.innerTop, (Color){ 16, 12, 26, 255 });
    FillQuad(LeafUV(L, 0.05f, 0.03f), LeafUV(L, 0.05f, 0.97f),
             LeafUV(L, 0.95f, 0.97f), LeafUV(L, 0.95f, 0.03f), (Color){ 20, 15, 32, 255 });
    DrawRecessedPanel(L, 0.18f, 0.07f, 0.90f, 0.31f, accent, pulse);
    DrawRecessedPanel(L, 0.18f, 0.36f, 0.90f, 0.64f, accent, pulse);
    DrawRecessedPanel(L, 0.18f, 0.69f, 0.90f, 0.93f, accent, pulse);
    DrawStrapHinge(L, 0.17f, accent, pulse);
    DrawStrapHinge(L, 0.50f, accent, pulse);
    DrawStrapHinge(L, 0.83f, accent, pulse);
    if (L.open > 0.04f) {
        float thick = fmaxf(L.visW * 0.16f, 4.0f) * sinf(L.open * 1.22f);
        if (thick > 1.0f) {
            Vector2 e0 = L.innerTop, e1 = L.innerBot;
            Vector2 e2 = V2(e1.x + (float)L.side * thick, e1.y);
            Vector2 e3 = V2(e0.x + (float)L.side * thick, e0.y);
            FillQuad(e0, e1, e2, e3, (Color){ 8, 6, 14, 255 });
            DrawLineEx(e0, e1, 2.4f, WithA(accent, (int)(50 + 90 * L.open)));
        }
    }
    DrawLineEx(L.hingeTop, L.innerTop, 1.8f, WithA(accent, hover ? 70 : 36));
    DrawLineEx(L.hingeBot, L.innerBot, 1.8f, WithA(accent, hover ? 50 : 24));
    DrawLineEx(L.hingeTop, L.hingeBot, 5.5f, (Color){ 8, 6, 14, 255 });
}

static void DrawVaultFrame(Rectangle r, Rectangle inner, Color accent, bool hover, float t) {
    float gp = 0.5f + 0.5f * sinf(t * 1.55f);
    Color fill = hover ? (Color){ 20, 16, 32, 245 } : (Color){ 14, 11, 22, 235 };
    int leftW  = (int)(inner.x - r.x);
    int rightX = (int)(inner.x + inner.width);
    int rightW = (int)(r.x + r.width - inner.x - inner.width);
    int topH   = (int)(inner.y - r.y);
    int botY   = (int)(inner.y + inner.height);
    int botH   = (int)(r.y + r.height - inner.y - inner.height);
    if (leftW < 1) leftW = 1;
    if (rightW < 1) rightW = 1;
    if (topH < 1) topH = 1;
    if (botH < 1) botH = 1;
    DrawRectangle((int)r.x, (int)r.y, leftW, (int)r.height, fill);
    DrawRectangle(rightX, (int)r.y, rightW, (int)r.height, fill);
    DrawRectangle((int)r.x, (int)r.y, (int)r.width, topH, fill);
    DrawRectangle((int)r.x, botY, (int)r.width, botH, fill);
    DrawRectangleRoundedLines(r, 0.04f, 8, hover ? WithA(accent, 200) : WithA(accent, (int)(90 + 45 * gp)));
    Rectangle inset = { r.x + 4, r.y + 4, r.width - 8, r.height - 8 };
    DrawRectangleRoundedLines(inset, 0.04f, 8, WithA(accent, hover ? 50 : 20));
    DrawCornerAccents(r, WithA(accent, hover ? 170 : 85), 18);
    DrawRectangle((int)inner.x - 5, botY, (int)inner.width + 10, botH > 8 ? 8 : botH, (Color){ 8, 6, 14, 255 });
    DrawLine((int)inner.x, (int)inner.y, rightX, (int)inner.y, WithA(accent, 55));
    DrawLine((int)inner.x, (int)inner.y, (int)inner.x, botY, (Color){ 8, 6, 14, 170 });
    DrawLine(rightX, (int)inner.y, rightX, botY, (Color){ 8, 6, 14, 170 });
}

static void DrawPlayInscription(Rectangle inner, float open, bool hover, float t) {
    float fade = 1.0f - fminf(1.0f, open * 2.2f);
    if (fade < 0.02f) return;
    Color acPlay = PlayAccent(t);
    MenuUI u = MakeUI();
    int playFS = MenuPx(u, 52);
    if (playFS > (int)(inner.width * 0.20f)) playFS = (int)(inner.width * 0.20f);
    if (playFS < 20) playFS = 20;
    float triH = playFS * 0.46f;
    float triW = triH * 1.65f;
    int playTW = MeasureText("PLAY", playFS);
    float gap = MenuPxf(u, 14);
    int glow = 6;
    float unitW = triW + gap + (float)playTW + (float)glow;
    float unitX = inner.x + inner.width * 0.5f - unitW * 0.5f;
    float unitCY = inner.y + inner.height * 0.30f;
    float triCX = unitX + triH * 0.65f;
    Vector2 tp0 = { triCX - triH * 0.65f, unitCY - triH };
    Vector2 tp1 = { triCX - triH * 0.65f, unitCY + triH };
    Vector2 tp2 = { triCX + triH, unitCY };
    int plTx = (int)(unitX + triW + gap);
    int plTy = (int)(unitCY - playFS / 2);
    float gp2 = 0.5f + 0.5f * sinf(t * 3.2f);
    for (int g = 6; g >= 1; g--) {
        uchar al = hover ? (uchar)((14 * g + (int)(6 * g * gp2)) * fade)
                         : (uchar)((8 * g + (int)(3 * g * gp2)) * fade);
        DrawText("PLAY", plTx - g, plTy - g, playFS, WithA(acPlay, al));
        DrawText("PLAY", plTx + g, plTy + g, playFS, WithA(acPlay, al));
    }
    DrawText("PLAY", plTx + 2, plTy + 3, playFS, (Color){ 0, 0, 0, (uchar)(140 * fade) });
    Color hi = hover ? (Color){ 255, 255, 255, (uchar)(255 * fade) }
                     : WithA(acPlay, (int)(235 * fade));
    DrawTriangle(tp0, tp1, tp2, hi);
    DrawText("PLAY", plTx, plTy, playFS, hi);
    int ulY = (int)(unitCY + triH + MenuPxf(u, 10));
    int ulMid = (int)(inner.x + inner.width * 0.5f);
    int ulHalf = (int)(unitW * 0.5f) + MenuPx(u, 6);
    DrawLine(ulMid - ulHalf, ulY, ulMid + ulHalf, ulY,
             WithA(acPlay, hover ? (int)(180 * fade) : (int)((70 + 50 * gp2) * fade)));
}

static void DrawHallTorch(float x, float y, float sc, int wall, float t, int seed, float dim) {
    if (dim < 0.012f) return;
    if (sc < 2.2f) {
        DrawCircleV(V2(x, y - sc * 0.55f), fmaxf(0.7f, sc * 0.50f),
                    (Color){ 255, 120, 32, (uchar)(210 * dim) });
        DrawCircleV(V2(x, y - sc * 0.72f), fmaxf(0.4f, sc * 0.22f),
                    (Color){ 255, 220, 110, (uchar)(170 * dim) });
        return;
    }
    float dir = (float)wall;
    Color wood   = { 92, 58, 28, 255 };
    Color woodLo = { 58, 34, 16, 255 };
    Color woodHi = { 130, 82, 40, 255 };
    Color iron   = { 72, 64, 84, 255 };
    Color ironLo = { 28, 22, 38, 255 };
    float plateX = x - dir * sc * 0.08f;
    DrawRectangleRounded((Rectangle){
        plateX - sc * 0.28f, y - sc * 0.22f, sc * 0.56f, sc * 0.70f
    }, 0.12f, 3, ironLo);
    DrawRectangleRoundedLines((Rectangle){
        plateX - sc * 0.28f, y - sc * 0.22f, sc * 0.56f, sc * 0.70f
    }, 0.12f, 3, iron);
    DrawCircleV(V2(plateX - sc * 0.16f, y - sc * 0.08f), sc * 0.07f, iron);
    DrawCircleV(V2(plateX + sc * 0.16f, y - sc * 0.08f), sc * 0.07f, iron);
    DrawCircleV(V2(plateX - sc * 0.16f, y + sc * 0.32f), sc * 0.07f, iron);
    DrawCircleV(V2(plateX + sc * 0.16f, y + sc * 0.32f), sc * 0.07f, iron);

    float sx = x + dir * sc * 0.18f;
    float top = y - sc * 1.05f;
    float bot = y + sc * 1.15f;
    float thick = fmaxf(2.4f, sc * 0.22f);
    DrawLineEx(V2(sx, bot), V2(sx, top), thick + 1.6f, woodLo);
    DrawLineEx(V2(sx, bot), V2(sx, top), thick, wood);
    DrawLineEx(V2(sx - thick * 0.22f, bot), V2(sx - thick * 0.22f, top),
               1.4f, woodHi);

    DrawLineEx(V2(plateX, y + sc * 0.18f), V2(sx, y + sc * 0.18f),
               fmaxf(2.0f, sc * 0.14f), iron);
    DrawCircleV(V2(sx, y + sc * 0.18f), sc * 0.20f, iron);
    DrawCircleV(V2(sx, y + sc * 0.18f), sc * 0.10f, ironLo);

    DrawRectangleRounded((Rectangle){
        sx - sc * 0.22f, top - sc * 0.06f, sc * 0.44f, sc * 0.28f
    }, 0.25f, 3, (Color){ 48, 28, 12, 255 });
    DrawLineEx(V2(sx - sc * 0.18f, top + sc * 0.04f),
               V2(sx + sc * 0.18f, top + sc * 0.04f), 1.5f, woodLo);
    DrawLineEx(V2(sx - sc * 0.16f, top + sc * 0.14f),
               V2(sx + sc * 0.16f, top + sc * 0.14f), 1.5f, woodLo);

    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleV(V2(sx, top - sc * 0.22f), sc * 0.72f,
                (Color){ 255, 70, 12, (uchar)(14 * dim) });
    EndBlendMode();
    DrawFlameTongue(sx, top, sc * 1.85f * dim, sc * 0.48f * (0.45f + 0.55f * dim), t, (float)seed);
    if (dim > 0.28f) {
        DrawFlameTongue(sx - sc * 0.14f, top + sc * 0.06f, sc * 1.25f * dim, sc * 0.28f, t, (float)seed + 2.4f);
        DrawFlameTongue(sx + sc * 0.16f, top + sc * 0.04f, sc * 1.15f * dim, sc * 0.24f, t, (float)seed + 5.1f);
    }
    if (dim > 0.55f)
        DrawFlameTongue(sx, top - sc * 0.08f, sc * 0.70f, sc * 0.16f, t, (float)seed + 8.2f);
    int sparks = dim > 0.45f ? 3 : (dim > 0.2f ? 1 : 0);
    for (int k = 0; k < sparks; k++) {
        float pr = fmodf(t * (0.7f + k * 0.15f) + Hash21((float)(seed + k), 3.3f), 1.0f);
        DrawCircleV(V2(sx + (Hash21((float)(seed + k), 1.1f) - 0.5f) * sc * 0.4f,
                       top - pr * sc * 2.1f),
                    sc * (0.09f - pr * 0.06f),
                    (Color){ 255, 150, 35, (uchar)(200 * (1.0f - pr) * dim) });
    }
}

static void DrawTorchHall(Rectangle inner, float rush, float t) {
    float vpX = inner.x + inner.width * 0.5f;
    float vpY = inner.y + inner.height * 0.32f;
    float yNF = inner.y + inner.height;
    float yNC = inner.y;
    float halfN = inner.width * 0.5f;
    float cam = rush * 0.70f;

    Color colCeil  = { 14, 11, 20, 255 };
    Color colFloor = { 28, 20, 22, 255 };
    Color colWallA = { 46, 36, 58, 255 };
    Color colWallB = { 34, 26, 44, 255 };
    Color colJoin  = { 72, 58, 88, 255 };
    Color colMort  = { 18, 12, 26, 255 };

    FillQuad(V2(inner.x, yNC), V2(inner.x + inner.width, yNC),
             V2(vpX, vpY), V2(vpX, vpY), colCeil);
    FillQuad(V2(inner.x, yNF), V2(inner.x + inner.width, yNF),
             V2(vpX, vpY), V2(vpX, vpY), colFloor);
    FillQuad(V2(inner.x, yNC), V2(inner.x, yNF),
             V2(vpX, vpY), V2(vpX, vpY), colWallA);
    FillQuad(V2(inner.x + inner.width, yNC), V2(inner.x + inner.width, yNF),
             V2(vpX, vpY), V2(vpX, vpY), colWallA);

    const int NS = 16;
    const int NR = 5;
    for (int i = 0; i < NS; i++) {
        float u0 = ((float)i / (float)NS - cam) / (1.05f - cam);
        float u1 = ((float)(i + 1) / (float)NS - cam) / (1.05f - cam);
        if (u1 < 0.0f || u0 > 0.97f) continue;
        if (u0 < 0.0f) u0 = 0.0f;
        if (u1 > 0.97f) u1 = 0.97f;
        float z0 = 0.28f + u0 * 2.10f, z1 = 0.28f + u1 * 2.10f;
        float x0 = halfN * (0.28f / z0), x1 = halfN * (0.28f / z1);
        float yf0 = vpY + (yNF - vpY) * (x0 / halfN);
        float yc0 = vpY + (yNC - vpY) * (x0 / halfN);
        float yf1 = vpY + (yNF - vpY) * (x1 / halfN);
        float yc1 = vpY + (yNC - vpY) * (x1 / halfN);
        DrawLineEx(V2(vpX - x0, yf0), V2(vpX + x0, yf0), 1.5f, (Color){ 48, 34, 40, 120 });
        for (int row = 0; row < NR; row++) {
            float a0 = (float)row / (float)NR;
            float a1 = (float)(row + 1) / (float)NR;
            Color brick = ((i + row) & 1) ? colWallA : colWallB;
            Vector2 l0 = V2(vpX - x0, yf0 + (yc0 - yf0) * a0);
            Vector2 l1 = V2(vpX - x0, yf0 + (yc0 - yf0) * a1);
            Vector2 l2 = V2(vpX - x1, yf1 + (yc1 - yf1) * a1);
            Vector2 l3 = V2(vpX - x1, yf1 + (yc1 - yf1) * a0);
            FillQuad(l0, l1, l2, l3, brick);
            DrawLineEx(l0, l3, 1.2f, colMort);
            DrawLineEx(l1, l2, 1.2f, colMort);
            Vector2 r0 = V2(vpX + x0, yf0 + (yc0 - yf0) * a0);
            Vector2 r1 = V2(vpX + x0, yf0 + (yc0 - yf0) * a1);
            Vector2 r2 = V2(vpX + x1, yf1 + (yc1 - yf1) * a1);
            Vector2 r3 = V2(vpX + x1, yf1 + (yc1 - yf1) * a0);
            FillQuad(r0, r1, r2, r3, brick);
            DrawLineEx(r0, r3, 1.2f, colMort);
            DrawLineEx(r1, r2, 1.2f, colMort);
        }
        DrawLineEx(V2(vpX - x0, yf0), V2(vpX - x0, yc0), 2.4f, colJoin);
        DrawLineEx(V2(vpX + x0, yf0), V2(vpX + x0, yc0), 2.4f, colJoin);
    }

    DrawLineEx(V2(inner.x, yNF), V2(vpX, vpY), 3.2f, colJoin);
    DrawLineEx(V2(inner.x + inner.width, yNF), V2(vpX, vpY), 3.2f, colJoin);
    DrawLineEx(V2(inner.x, yNC), V2(vpX, vpY), 2.2f, (Color){ 22, 16, 30, 200 });
    DrawLineEx(V2(inner.x + inner.width, yNC), V2(vpX, vpY), 2.2f, (Color){ 22, 16, 30, 200 });
    DrawCircleV(V2(vpX, vpY), 2.2f, (Color){ 4, 2, 8, 255 });

    const int NT = 14;
    for (int i = NT - 1; i >= 0; i--) {
        float uWorld = 0.038f + (float)i * (1.008f / (float)(NT - 1));
        float u = (uWorld - cam) / (1.05f - cam);
        if (u < 0.006f || u > 0.9992f) continue;
        float z = 0.28f + u * 2.10f;
        float xu = halfN * (0.28f / z);
        float yf = vpY + (yNF - vpY) * (xu / halfN);
        float yc = vpY + (yNC - vpY) * (xu / halfN);
        float yy = yf + (yc - yf) * 0.40f;
        float near = 1.0f - u;
        float sc = inner.height * (0.100f * near * near + 0.0055f);
        float torchDim = near * near;
        if (torchDim < 0.025f) torchDim = 0.025f;
        float glowDim = near * near * near * near;
        for (int wall = -1; wall <= 1; wall += 2) {
            float xx = vpX + (float)wall * xu * 0.98f;
            if (u < 0.34f && glowDim > 0.05f) {
                BeginBlendMode(BLEND_ADDITIVE);
                DrawEllipse((int)(vpX + (float)wall * xu * 0.38f), (int)yf,
                            (int)(xu * 0.16f), (int)(sc * 0.38f),
                            (Color){ 255, 70, 12, (uchar)(16 * glowDim) });
                EndBlendMode();
            }
            if (u < 0.55f && glowDim > 0.03f) {
                BeginBlendMode(BLEND_ADDITIVE);
                DrawCircleV(V2(xx, yy - sc * 0.8f), sc * 0.85f,
                            (Color){ 255, 55, 10, (uchar)(12 * glowDim) });
                EndBlendMode();
            }
            DrawHallTorch(xx, yy, sc, wall, t, i * 2 + (wall > 0 ? 1 : 0) + 11, torchDim);
        }
    }
}

static void DrawDungeonGate(Rectangle r, float t, bool hover) {
    MenuUI u = MakeUI();
    Color ac = PlayAccent(t);
    if (menuEnter >= 0.0f && menuEnter < 0.16f) {
        float k = 1.0f - menuEnter / 0.16f;
        r.x += sinf(menuEnter * 78.0f) * MenuPxf(u, 1.8f) * k;
        r.y += cosf(menuEnter * 54.0f) * MenuPxf(u, 1.1f) * k;
    }
    float wall = fmaxf(MenuPxf(u, 12), r.width * 0.045f);
    Rectangle inner = {
        r.x + wall, r.y + wall,
        r.width - wall * 2.0f, r.height - wall * 1.65f
    };
    if (inner.width < 40.0f || inner.height < 80.0f) return;

    float openL = DoorOpenKSide(-1, hover);
    float openR = DoorOpenKSide(1, hover);
    float open = DoorOpenK(hover);
    float rush = DoorRushK();

    DrawRectangleRec(inner, (Color){ 4, 2, 8, 255 });
    BeginScissorMode((int)inner.x, (int)inner.y, (int)inner.width, (int)inner.height);
    DrawTorchHall(inner, rush, t);
    EndScissorMode();

    DoorLeaf L = MakeLeaf(inner, -1, openL);
    DoorLeaf Rgt = MakeLeaf(inner, 1, openR);
    DrawVaultLeaf(L, t, ac, hover);
    DrawVaultLeaf(Rgt, t, ac, hover);

    {
        float scx = inner.x + inner.width * 0.5f;
        float scy = inner.y + inner.height * 0.56f;
        float sR = fminf(inner.width, inner.height) * 0.100f;
        BeginScissorMode((int)inner.x, (int)inner.y, (int)L.visW, (int)inner.height);
        DrawGateSkull(scx, scy, sR, t);
        EndScissorMode();
        BeginScissorMode((int)(inner.x + inner.width - Rgt.visW), (int)inner.y,
                         (int)Rgt.visW, (int)inner.height);
        DrawGateSkull(scx, scy, sR, t);
        EndScissorMode();
        DrawRingKnocker(L, t, ac);
        DrawRingKnocker(Rgt, t, ac);
    }

    if (open < 0.14f) {
        float sx = inner.x + inner.width * 0.5f;
        float glow = (hover ? 0.9f : 0.35f) * (1.0f - open / 0.14f);
        glow *= 0.5f + 0.5f * sinf(t * 2.1f);
        DrawLineEx(V2(sx, inner.y + 8), V2(sx, inner.y + inner.height - 8),
                   hover ? 2.4f : 1.4f, WithA(ac, (int)(40 + 100 * glow)));
    }

    if (menuEnter >= 0.12f && menuEnter < 0.42f) {
        float p = (menuEnter - 0.12f) / 0.30f;
        float sx = inner.x + inner.width * 0.5f;
        for (int i = 0; i < 8; i++) {
            float n = Hash21((float)i, 4.2f);
            float vy = -20.0f - n * 40.0f;
            float vx = (n - 0.5f) * 70.0f;
            float px = sx + vx * p;
            float py = inner.y + inner.height * 0.52f + vy * p;
            DrawCircle((int)px, (int)py, 1.4f + (1.0f - p) * 1.6f,
                       WithA(ac, (int)((1.0f - p) * 110)));
        }
    }

    DrawVaultFrame(r, inner, ac, hover, t);
    DrawPlayInscription(inner, open, hover, t);
    {
        float ov = DoorOverlayK();
        if (ov > 0.01f)
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                          (Color){ 0, 0, 0, (uchar)(255.0f * ov) });
    }
}

static void DrawTutorialChip(Rectangle r, float t, bool hov) {
    Color ic  = hov ? (Color){ 210, 230, 255, 255 } : (Color){ 120, 185, 255, 220 };
    Color bgc = hov ? (Color){ 20, 28, 52, 230 } : (Color){ 12, 16, 32, 190 };
    DrawRectangleRounded(r, 0.18f, 8, bgc);
    DrawRectangleRoundedLines(r, 0.18f, 8,
        hov ? (Color){ 140, 190, 255, 210 } : (Color){ 70, 110, 170, 150 });
    DrawMenuGlyph(IC_TUT, SelectedMenuIconVariant(IC_TUT),
                  r.x + r.width * 0.5f, r.y + r.height * 0.50f,
                  r.width * 0.40f, ic, t);
}

#undef EllipseRot
#undef Lighten
