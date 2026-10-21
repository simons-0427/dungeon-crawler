// game_ui.c — HUD, pause menu, the full main menu with all sub-screens
// (tutorial / stats / shop / achievements / future-updates / settings) and the
// death / next-round overlay.
#include "game_ui.h"
#include "globals.h"
#include "constants.h"
#include "types.h"
#include "game_items.h"      // IK_NAME, IK_COL
#include "skins.h"           // GetSkinColor, DrawRare*/DrawEpic*/DrawSkull
#include "game_save.h"       // SaveStats
#include "game_shop.h"       // SaveShop
#include "game_utils.h"      // DrawCustomStar(Lines), PointInRect
#include "game_settings.h"   // SaveSettings (called on settings change)
#include "game_audio.h"      // GetNowPlayingName / GetNowPlayingAlpha
#include "game_boss.h"       // BossDisplayName (boss-room banner)
#include <math.h>
#include <string.h>          // strchr, strncpy
#include <stdio.h>           // snprintf

// ─── ACHIEVEMENT COIN REWARDS (indexed by tier 0-5) ─────────────────────────
static const int ACH_REWARDS[6] = { 50, 100, 250, 500, 1000, 3000 };

// ─── UPGRADE COSTS (indexed by current level; used by the SHOP upgrades tab) ──
static const int SPD_COST[] = {  200,  250,  350, 500, 600 };
static const int SHD_COST[] = {  250,  400,  600, 750, 1000};
static const int WLK_COST[] = {  500,  750,  1000, 1250, 1500};
static const int SWD_COST[] = {  500,  1000, 1500 };
static const int BMB_COST[] = {  250,  500, 750, 1000, 1250};
static const int GLD_COST[] = {  300,  450,  650, 900, 1200};

// Shop tab selector (0=skins, 1=upgrades, 2=perks). Local to the menu;
// menuSub / statScroll / upgScroll are shared with main.c via globals.h.
static int shopTab = 0;

// Internal forward declarations (helpers defined further down in this file).
static void DrawPauseButton(void);
static void DrawMenuBG(float t, int sw, int sh);
static void DrawCornerAccents(Rectangle r, Color c, int len);
static bool DrawNavCard(Rectangle r, const char *label, Color accent, float t, float phase);
static Rectangle DrawSubPanel(const char *title, Color accent, float t, int sw, int sh);
// Music menu icon — 1:1 Icon-Ideas "Beamed Gear" (chunky beamed notes + gear lavender).
static void DrawMenuMusicBeamedGear(Rectangle bounds, float t, bool hover);
static void CheckAchievementSkinUnlocks(void);

// ═══════════════════════════════════════════════════════════════════════════
//  The remainder of this file is sliced verbatim from the original monolith:
//  pause/HUD layout, HUD, pause menu, menu-background helpers, and the large
//  DrawMenu / DrawOverlay routines.
// ═══════════════════════════════════════════════════════════════════════════

// ── pause / HUD layout ──────────────────────────────────────────────────────
void GetPauseMenuLayout(Rectangle *panel, Rectangle *resumeBtn, Rectangle *menuBtn) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int pw = 460, ph = 220;
    int px2 = sw / 2 - pw / 2;
    int py2 = sh / 2 - ph / 2;

    if (panel)     *panel     = (Rectangle){ (float)px2, (float)py2, (float)pw, (float)ph };
    if (resumeBtn) *resumeBtn = (Rectangle){ (float)(px2 + pw/2 - 120), (float)(py2 + 110), 240.0f, 36.0f };
    if (menuBtn)   *menuBtn   = (Rectangle){ (float)(px2 + pw/2 - 120), (float)(py2 + 156), 240.0f, 36.0f };
}

Rectangle GetPauseButtonRect(void) {
    int sw = GetScreenWidth();
    return (Rectangle){ (float)(sw - 14 - 44), 15.0f, 44.0f, 44.0f };
}

// ─── DRAW HUD ─────────────────────────────────────────────────────────────────
static void DrawPauseButton(void);
void DrawHUD(float t) {    
    (void)t;
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, 76, (Color){8,6,14,(uchar)(235*hud_opacity)});
    DrawRectangle(0, 75, sw, 2, (Color){55,50,72,(uchar)(200*hud_opacity)});

    DrawText(TextFormat("Round  %d", roundn), 16, 8, 26, (Color){210,205,230,255});

    DrawCircle(213, 21, 9, CCOIN);
    DrawCircle(210, 18, 4, (Color){255,245,160,200});
    DrawText(TextFormat("%d", totcoi), 228, 8, 26, CCOIN);

    DrawRectangle(295, 10, 20, 15, CTRES);
    DrawRectangle(295, 10, 20, 4, (Color){255,240,180,200});
    DrawText(TextFormat("%d", tottrs), 323, 10, 20, (Color){200,180,80,255});

    DrawCircle(392, 21, 9, CENE);
    DrawText(TextFormat("%d", kills_game), 407, 8, 26, CENE);

    {
        const char *rl = NULL; Color rlc = (Color){220,80,80,160};
        if      (roomKind == RK_BOSS && boss.on) { rl = "BOSS FIGHT";        rlc = (Color){CBOSS.r,CBOSS.g,CBOSS.b,220}; }
        else if (roomKind == RK_TRAP)            { rl = "TRAP ROOM";         rlc = (Color){235,150,60,220}; }
        else if (roomKind == RK_DARK)            { rl = "DARK ROOM";         rlc = (Color){150,120,230,220}; }
        else if (roomKind == RK_LAVA)            { rl = "THE FLOOR IS LAVA"; rlc = (Color){CFIRE.r,CFIRE.g,CFIRE.b,230}; }
        else if (roomKind == RK_COIN_RUSH)       { rl = "COIN RUSH";         rlc = (Color){255,210,40,230}; }
        else if (eneon)                          { rl = "ENEMIES ON"; }
        else                                     { rl = "ENEMIES OFF";       rlc = (Color){80,80,100,130}; }
        DrawText(rl, sw/2 - MeasureText(rl,16)/2, 10, 16, rlc);
    }

    DrawLine(14, 38, 520, 38, (Color){45,40,62,180});
    DrawText("HIGHSCORES:", 16, 42, 14, (Color){100,95,130,180});

    bool newRound = (roundn > hs_round);
    Color rcol = newRound ? (Color){255,210,60,255} : (Color){160,155,185,200};
    DrawText(TextFormat("Round %d", newRound ? roundn : hs_round),
             16, 56, 14, rcol);

    bool newCoi = (totcoi > hs_coi);
    Color ccol = newCoi ? (Color){255,215,0,255} : (Color){160,155,185,200};
    DrawCircle(108, 61, 5, newCoi ? CCOIN : (Color){130,110,0,180});
    DrawText(TextFormat("%d", newCoi ? totcoi : hs_coi), 118, 56, 14, ccol);

    bool newTrs = (tottrs > hs_trs);
    Color tcol = newTrs ? (Color){255,200,50,255} : (Color){160,155,185,200};
    DrawRectangle(176, 57, 10, 8, newTrs ? CTRES : (Color){130,100,0,160});
    DrawText(TextFormat("%d", newTrs ? tottrs : hs_trs), 190, 56, 14, tcol);

    bool newKills = (kills_game > hs_kills);
    Color kcol = newKills ? CENE : (Color){160,155,185,200};
    DrawCircle(250, 61, 5, newKills ? CENE : (Color){130,55,55,180});
    DrawText(TextFormat("%d", newKills ? kills_game : hs_kills), 260, 56, 14, kcol);

    DrawCircle(328, 62, 5, CCOIN);
    DrawCircle(326, 60, 2, (Color){255,245,160,200});
    DrawText(TextFormat("Balance: %d", coin_balance), 338, 56, 13, (Color){255,215,0,200});

    // ── Item inventory slots ──────────────────────────────────────────────────
    int eff_slots = INV_SZ + perk_slots;
    int sx = sw - 14 - 64 - eff_slots * 68;
    DrawText("ITEMS", sx-70, 20, 15, (Color){130,125,160,190});
    for (int i = 0; i < eff_slots; i++) {
        int bx = sx + i*68, by = 9;
        bool has = (i < pinvn);
        bool isGld = has && pinv[i] == IK_GLD;

        if (isGld) {
            // Pulsing gold glow that spills past the slot — no metal sheen.
            float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 4.0f + (float)i * 0.7f);
            // 0 → dim, 1 → bright
            float p = pulse;

            // Outer glow (extends past the inventory bar)
            for (int s = 6; s >= 1; s--) {
                float pad = 2.0f + (float)s * 1.4f + p * 1.2f;
                int a = (int)((8 + s * 7) * (0.45f + 0.55f * p));
                if (a > 255) a = 255;
                DrawRectangleRounded(
                    (Rectangle){(float)bx - pad, (float)by - pad,
                                62.0f + pad * 2.0f, 56.0f + pad * 2.0f},
                    0.36f, 10, (Color){255, 190, 40, (uchar)a});
            }

            // Slot body — flat warm gold, clean
            DrawRectangleRounded((Rectangle){(float)bx, (float)by, 62, 56},
                                 0.3f, 8, (Color){40, 30, 12, 240});
            DrawRectangleRounded((Rectangle){(float)(bx + 2), (float)(by + 2), 58, 52},
                                 0.28f, 8,
                                 (Color){70, 52, 14, (uchar)(90 + 50 * p)});

            // Pulsing border: outer (past slot) + main edge
            DrawRectangleRoundedLines(
                (Rectangle){(float)(bx - 3), (float)(by - 3), 68, 62},
                0.32f, 8, (Color){255, 195, 45, (uchar)(50 + 110 * p)});
            DrawRectangleRoundedLines(
                (Rectangle){(float)bx, (float)by, 62, 56},
                0.3f, 8, (Color){255, 220, 80, (uchar)(140 + 100 * p)});

            DrawText(IK_NAME[pinv[i]], bx + 6, by + 5, 14,
                     (Color){255, (uchar)(210 + 40 * p), 70, 255});
            DrawText(TextFormat("[%d]", i + 1), bx + 6, by + 36, 12,
                     (Color){200, 160, 60, (uchar)(180 + 50 * p)});
        } else {
            DrawRectangleRounded((Rectangle){(float)bx, (float)by, 62, 56}, 0.3f, 8,
                                 has ? (Color){35, 30, 52, 210} : (Color){20, 18, 32, 120});
            Color bc = has ? IK_COL[pinv[i]] : (Color){50, 46, 68, 140};
            DrawRectangleRoundedLines((Rectangle){(float)bx, (float)by, 62, 56}, 0.3f, 8, bc);
            if (has) {
                DrawText(IK_NAME[pinv[i]], bx + 6, by + 5, 14, IK_COL[pinv[i]]);
                DrawText(TextFormat("[%d]", i + 1), bx + 6, by + 36, 12,
                         (Color){130, 125, 155, 200});
            } else {
                DrawText(TextFormat("[%d]", i + 1), bx + 20, by + 22, 13,
                         (Color){65, 60, 85, 140});
            }
        }
    }

    // ── Active effects (bottom-left) ─────────────────────────────────────────
    int efx = 14, efy = sh - 32;
    if (pspd > 0) {
        DrawText(TextFormat("SPEED  %.1fs", pspd), efx, efy, 18,
                 (Color){80,220,80,255}); efx += 155;
    }
    if (pshield) {
        if (pshield_dur >= 1e8f)
            DrawText("SHIELD", efx, efy, 18, (Color){80,150,255,255});
        else
            DrawText(TextFormat("SHIELD  %.1fs", pshield_dur), efx, efy, 18, (Color){80,150,255,255});
        efx += 165;
    }
    if (pshield_perk) {
        DrawText("PERK SHIELD", efx, efy, 18, (Color){100,180,255,200}); efx += 155;
    }
    if (pwlk > 0) {
        DrawText(TextFormat("GHOSTWALK  %.1fs", pwlk), efx, efy, 18, CWLK);
        efx += 205;
    }
    if (pgold > 0) {
        DrawText(TextFormat("GOLD TOUCH  %.1fs", pgold), efx, efy, 18, IK_COL[IK_GLD]);
        efx += 210;
    }

    DrawPauseButton();

    const char *help = "WASD/Arrows:Move   1-5:Use Item   F11:Fullscreen";
    DrawText(help, sw - MeasureText(help,14) - 14,
             sh-22, 14, (Color){80,75,100,160});

    // ── Round-start banner: announce special room types ───────────────────────
    if (bannerT > 0 && gs == GS_PLAY && roomKind != RK_NORMAL) {
        const char *bn = "", *sub = ""; Color bc = (Color){235,150,60,255};
        if (roomKind == RK_BOSS) {
            bn = "BOSS FIGHT";
            sub = TextFormat("Defeat %s!", BossDisplayName());
            bc = CBOSS;
        } else if (roomKind == RK_TRAP) {
            bn = "TRAP ROOM"; sub = "Time the arrow sweeps - mind the lava!";
            bc = (Color){235,150,60,255};
        } else if (roomKind == RK_DARK) {
            bn = "DARK ROOM"; sub = "Follow the glow!";
            bc = (Color){170,140,255,255};
        } else if (roomKind == RK_LAVA) {
            bn = "THE FLOOR IS LAVA"; sub = "Run north - don't stop!";
            bc = CFIRE;
        } else if (roomKind == RK_COIN_RUSH) {
            bn = "COIN RUSH";
            sub = "Hard rooms. Double coin piles & value. Stack gold!";
            bc = (Color){255,210,40,255};
        }
        float a = (bannerT > 0.7f) ? 1.0f : bannerT / 0.7f;
        int fs = 46, bw2 = MeasureText(bn, fs);
        int bxx = sw/2 - bw2/2, byy = sh/3 - 20;
        DrawText(bn, bxx+3, byy+3, fs, (Color){0,0,0,(uchar)(150*a)});
        DrawText(bn, bxx,   byy,   fs, (Color){bc.r,bc.g,bc.b,(uchar)(255*a)});
        int sw3 = MeasureText(sub, 20);
        DrawText(sub, sw/2-sw3/2+1, byy+57, 20, (Color){0,0,0,(uchar)(120*a)});
        DrawText(sub, sw/2-sw3/2,   byy+56, 20, (Color){225,220,240,(uchar)(220*a)});
    }

    // ── Now Playing banner — below HUD bar, fades in then out ─────────────────
    {
        float npa = GetNowPlayingAlpha();
        if (npa > 0.01f && !audio_mute) {
            const char *npn = GetNowPlayingName();
            if (npn) {
                uchar npal = (uchar)(npa * 210.0f);
                DrawText("NOW PLAYING:", 14, 82, 11, (Color){160,140,215,npal});
                DrawText(npn,            14, 95, 17, (Color){220,210,255,npal});
            }
        }
    }
}
void DrawPauseMenu(void) {
    Rectangle panel, resumeBtn, menuBtn;
    GetPauseMenuLayout(&panel, &resumeBtn, &menuBtn);

    DrawRectangleRounded(panel, 0.08f, 8, (Color){10, 8, 18, 240});
    DrawRectangleRoundedLines(panel, 0.08f, 8, CW);

    const char *TT = "PAUSED";
    int ttw = MeasureText(TT, 36);
    DrawText(TT, (int)(panel.x + panel.width/2 - ttw/2), (int)(panel.y + 20),
             36, (Color){215, 210, 235, 255});
    
    const char *ST = "ESC or the pause icon resumes the game";
    int stw = MeasureText(ST, 16);
    DrawText(ST, (int)(panel.x + panel.width/2 - stw/2), (int)(panel.y + 68),
             16, (Color){160, 155, 185, 255});

    DrawLine((int)panel.x + 16, (int)panel.y + 50, (int)panel.x + (int)panel.width - 16,
             (int)panel.y + 50, (Color){65, 58, 88, 180});

    bool hoverResume = PointInRect(GetMousePosition(), resumeBtn);
    bool hoverMenu   = PointInRect(GetMousePosition(), menuBtn);

    Color rb = hoverResume ? (Color){35, 65, 42, 255} : (Color){20, 40, 25, 240};
    Color mb = hoverMenu   ? (Color){75, 35, 32, 255} : (Color){45, 20, 18, 240};
    Color rlc = hoverResume ? (Color){80, 220, 80, 255} : (Color){40, 150, 50, 255};
    Color mlc = hoverMenu   ? (Color){220, 65, 65, 255} : (Color){150, 40, 40, 255};

    DrawRectangleRounded(resumeBtn, 0.25f, 8, rb);
    DrawRectangleRoundedLines(resumeBtn, 0.25f, 8, rlc);
    DrawRectangleRounded(menuBtn,   0.25f, 8, mb);
    DrawRectangleRoundedLines(menuBtn,   0.25f, 8, mlc);

    const char *resumeTxt = "RESUME";
    const char *menuTxt = "MENU";
    DrawText(resumeTxt,
             (int)(resumeBtn.x + resumeBtn.width/2 - MeasureText(resumeTxt, 20)/2),
             (int)(resumeBtn.y + 8), 20, (Color){230, 235, 240, 255});
    DrawText(menuTxt,
             (int)(menuBtn.x + menuBtn.width/2 - MeasureText(menuTxt, 20)/2),
             (int)(menuBtn.y + 8), 20, (Color){230, 235, 240, 255});
}

static void DrawPauseButton(void) {
    Rectangle r = GetPauseButtonRect();
    bool hover = PointInRect(GetMousePosition(), r);

   DrawRectangleRounded(r, 0.25f, 8,
                         hover ? (Color){40, 36, 58, 240} : (Color){24, 20, 34, 220});
    DrawRectangleRoundedLines(r, 0.25f, 8,
                              hover ? (Color){180, 170, 220, 255} : (Color){110, 100, 140, 210});

    int barW = 6;
    int barH = 20;
    int barY = (int)(r.y + 12);
    int barX1 = (int)(r.x + 12);
    int barX2 = (int)(r.x + 26);
    DrawRectangle(barX1, barY, barW, barH, (Color){220, 215, 235, 255});
    DrawRectangle(barX2, barY, barW, barH, (Color){220, 215, 235, 255});
}

// ─── DRAW MENU ────────────────────────────────────────────────────────────────
static void DrawMenuBG(float t, int sw, int sh) {
    for (int y = 0; y <= sh/CELL; y++)
    for (int x = 0; x <= sw/CELL; x++) {
        float f = 0.22f + 0.14f*sinf(x*0.41f + y*0.53f + t*0.35f)
                        + 0.08f*sinf(x*0.17f - y*0.29f + t*0.22f);
        DrawRectangle(x*CELL, y*CELL, CELL-1, CELL-1,
                      (Color){(uchar)(12*f),(uchar)(9*f),(uchar)(20*f),255});
    }
    for (int i = 7; i >= 1; i--)
        DrawCircle(sw/2, sh/2, (float)(i*160),
                   (Color){6,4,14,(uchar)(4+i*6)});
}

static void DrawCornerAccents(Rectangle r, Color c, int len) {
    int x1=(int)r.x, y1=(int)r.y;
    int x2=(int)(r.x+r.width-1), y2=(int)(r.y+r.height-1);
    DrawLine(x1,y1,   x1+len,y1,   c); DrawLine(x1,y1, x1,y1+len,   c);
    DrawLine(x2,y1,   x2-len,y1,   c); DrawLine(x2,y1, x2,y1+len,   c);
    DrawLine(x1,y2,   x1+len,y2,   c); DrawLine(x1,y2, x1,y2-len,   c);
    DrawLine(x2,y2,   x2-len,y2,   c); DrawLine(x2,y2, x2,y2-len,   c);
}

// ── Music icon: 1:1 from Skin_Ideas/Icon-Ideas "Beamed Gear" ────────────────
// Colors match Settings gear (160,148,210 / hover 230,220,255).
// Geometry + bob/sway animation match IconIdeas.c DrawBeamedNote case 1.
static Color MusicGearLighten(Color c, float amount)
{
    return (Color){
        (unsigned char)fminf(255.0f, c.r + (255 - c.r) * amount),
        (unsigned char)fminf(255.0f, c.g + (255 - c.g) * amount),
        (unsigned char)fminf(255.0f, c.b + (255 - c.b) * amount),
        c.a
    };
}

static void DrawBeamedGearEllipseRot(float cx, float cy, float rx, float ry, float ang, Color col)
{
    const int SEG = 28;
    Vector2 pts[SEG + 1];
    float ca = cosf(ang), sa = sinf(ang);
    for (int i = 0; i <= SEG; i++) {
        float th = (float)i / (float)SEG * 2.0f * PI;
        float ex = cosf(th) * rx, ey = sinf(th) * ry;
        pts[i] = (Vector2){ cx + ex * ca - ey * sa, cy + ex * sa + ey * ca };
    }
    for (int i = 0; i < SEG; i++)
        DrawTriangle((Vector2){ cx, cy }, pts[i + 1], pts[i], col);
}

// Exact DrawBeamedNote from IconIdeas.c
static void DrawBeamedGearNote(float cx, float cy, float s, Color fill,
                               float bob, float sway, float headBobL, float headBobR)
{
    float gap = s * 1.05f;
    float hxL = cx - gap * 0.50f + sinf(sway) * s * 0.04f;
    float hxR = cx + gap * 0.50f + sinf(sway) * s * 0.04f;
    float hyL = cy + bob + s * 0.32f + headBobL;
    float hyR = cy + bob + s * 0.32f + headBobR;
    const float headAng = -0.48f + sway * 0.4f;

    for (int side = 0; side < 2; side++) {
        float hx = (side == 0) ? hxL : hxR;
        float hy = (side == 0) ? hyL : hyR;
        DrawBeamedGearEllipseRot(hx, hy, s * 0.46f, s * 0.34f, headAng, fill);
        DrawBeamedGearEllipseRot(hx - s * 0.07f, hy - s * 0.04f, s * 0.15f, s * 0.09f, headAng,
                                 MusicGearLighten(fill, 0.28f));
    }

    float stemL = hxL + s * 0.30f;
    float stemR = hxR + s * 0.30f;
    float stemW = s * 0.18f;
    float stemTopL = hyL - s * 1.22f;
    float stemTopR = hyR - s * 1.22f;
    DrawLineEx((Vector2){ stemL, hyL }, (Vector2){ stemL, stemTopL }, stemW, fill);
    DrawLineEx((Vector2){ stemR, hyR }, (Vector2){ stemR, stemTopR }, stemW, fill);

    float beamThick0 = s * 0.22f;
    float beamThick1 = s * 0.16f;
    DrawLineEx((Vector2){ stemL - s * 0.02f, stemTopL + s * 0.02f },
               (Vector2){ stemR + s * 0.02f, stemTopR + s * 0.02f }, beamThick0, fill);
    DrawLineEx((Vector2){ stemL - s * 0.02f, stemTopL + s * 0.34f },
               (Vector2){ stemR + s * 0.02f, stemTopR + s * 0.34f }, beamThick1, fill);
}

static void DrawMenuMusicBeamedGear(Rectangle bounds, float t, bool hover)
{
    // Plate matches Settings button (game_ui gear icon colors)
    Color bgc = hover ? (Color){ 30, 24, 50, 230 } : (Color){ 16, 12, 28, 190 };
    Color border = hover ? (Color){ 190, 170, 255, 210 } : (Color){ 95, 85, 140, 150 };
    DrawRectangleRounded(bounds, 0.18f, 8, bgc);
    DrawRectangleRoundedLines(bounds, 0.18f, 8, border);

    // Glyph colors: Gear lavender / Gear Hover (IconIdeas COL_GEAR / COL_GEAR_HI)
    Color fill = hover ? (Color){ 230, 220, 255, 255 } : (Color){ 160, 148, 210, 255 };

    float cx = bounds.x + bounds.width * 0.50f;
    float cy = bounds.y + bounds.height * 0.50f;
    float s  = bounds.width * 0.34f; // IconIdeas scale

    // Beamed Gear animation (case 1): bob * 0.6, full sway
    float bob  = sinf(t * 2.0f) * s * 0.04f;
    float sway = sinf(t * 1.3f) * 0.08f;
    DrawBeamedGearNote(cx, cy, s, fill, bob * 0.6f, sway, 0.0f, 0.0f);
}

static bool DrawNavCard(Rectangle r, const char *label, Color accent, float t, float phase) {
    Vector2 mp  = GetMousePosition();
    bool hover  = CheckCollisionPointRec(mp, r);
    float gp    = 0.5f + 0.5f*sinf(t*1.6f + phase);

    int fs  = 32;
    int lw  = MeasureText(label, fs);
    int lx  = (int)(r.x + r.width/2 - lw/2);
    int ly  = (int)(r.y + r.height/2 - fs/2);

    {
        float bf = 0.5f + 0.4f*gp;
        Color fill = hover
            ? (Color){(uchar)(20 + accent.r/10),
                      (uchar)(16 + accent.g/10),
                      (uchar)(32 + accent.b/10), 210}
            : (Color){14, 11, 22, 180};
        DrawRectangleRounded(r, 0.10f, 8, fill);
        Color border = hover
            ? (Color){accent.r, accent.g, accent.b, 200}
            : (Color){(uchar)(accent.r*bf), (uchar)(accent.g*bf), (uchar)(accent.b*bf), 130};
        DrawRectangleRoundedLines(r, 0.10f, 8, border);
        Rectangle inner = {r.x+3, r.y+3, r.width-6, r.height-6};
        uchar inal = hover ? 40u : (uchar)(15 + (int)(12*gp));
        DrawRectangleRoundedLines(inner, 0.10f, 8,
                                  (Color){accent.r, accent.g, accent.b, inal});
    }

    for (int g = 5; g >= 1; g--) {
        uchar al = hover ? (uchar)(6*g + (int)(6*g*gp))
                         : (uchar)(4*g + (int)(3*g*gp));
        Color gc = {accent.r, accent.g, accent.b, al};
        DrawText(label, lx - g, ly - g, fs, gc);
        DrawText(label, lx + g, ly + g, fs, gc);
    }

    DrawText(label, lx+2, ly+3, fs, (Color){0,0,0,160});

    Color mc = hover
        ? (Color){255,255,255,255}
        : (Color){(uchar)CLAMP255(accent.r+60),
                  (uchar)CLAMP255(accent.g+60),
                  (uchar)CLAMP255(accent.b+60), 235};
    DrawText(label, lx, ly, fs, mc);

    float ulf = 0.5f + 0.5f*sinf(t*2.0f + phase);
    int ulx1  = lx - 6;
    int ulx2  = lx + lw + 6;
    int uly   = ly + fs + 4;
    uchar ulAlpha = hover ? (uchar)180 : (uchar)(60 + (int)(40*ulf));
    DrawLine(ulx1,    uly,   ulx2,    uly,
             (Color){accent.r, accent.g, accent.b, ulAlpha});
    DrawLine(ulx1+10, uly+5, ulx2-10, uly+5,
             (Color){accent.r, accent.g, accent.b, 35});

    if (hover) {
        int ax = lx + lw + 16;
        int ay = ly + fs/2 - 10;
        DrawText(">", ax+1, ay+1, 20, (Color){0,0,0,100});
        DrawText(">", ax,   ay,   20, (Color){(uchar)CLAMP255(accent.r+60),
                                              (uchar)CLAMP255(accent.g+60),
                                              (uchar)CLAMP255(accent.b+60), 220});
    }

    return hover;
}

static Rectangle DrawSubPanel(const char *title, Color accent, float t, int sw, int sh) {
    int margin2 = 32;
    int pw = sw - margin2*2, ph = sh - margin2*2;
    int px2 = margin2, py2 = margin2;
    Rectangle r = { (float)px2, (float)py2, (float)pw, (float)ph };

    float gp = 0.5f + 0.5f*sinf(t*1.5f);
    DrawRectangleRounded(r, 0.04f, 8, (Color){10,8,18,252});
    Color bdr = {(uchar)(accent.r*(0.5f+0.5f*gp)),
                 (uchar)(accent.g*(0.5f+0.5f*gp)),
                 (uchar)(accent.b*(0.5f+0.5f*gp)), 200};
    DrawRectangleRoundedLines(r, 0.04f, 8, bdr);

    DrawRectangleRounded((Rectangle){r.x+1,r.y+1,r.width-2,54},
                         0.04f,8,(Color){accent.r/7,accent.g/7,accent.b/7,240});
    DrawLine(px2+16,(int)(r.y+56),px2+pw-16,(int)(r.y+56),
             (Color){accent.r,accent.g,accent.b,70});
    int ttw = MeasureText(title, 26);
    DrawText(title, px2+pw/2-ttw/2+1, py2+15+1, 26, (Color){0,0,0,140});
    DrawText(title, px2+pw/2-ttw/2,   py2+15,   26,
             (Color){accent.r,accent.g,accent.b,255});
    DrawCornerAccents(r, (Color){accent.r,accent.g,accent.b,140}, 20);

    // ── Red BACK button (bottom-left of panel) ────────────────────────────────
    int bkW = 130, bkH = 40;
    int bkX = px2 + 20, bkY = py2 + ph - bkH - 16;
    Rectangle backR = { (float)bkX, (float)bkY, (float)bkW, (float)bkH };

    Vector2 mp = GetMousePosition();
    bool bhov = CheckCollisionPointRec(mp, backR);
    Color bkAccent = (Color){220, 55, 55, 255};

    if (bhov) {
        for (int g=4;g>=1;g--)
            DrawRectangleRounded(
                (Rectangle){backR.x-g*3,backR.y-g*2,backR.width+g*6,backR.height+g*4},
                0.4f,8,(Color){bkAccent.r,bkAccent.g,bkAccent.b,(uchar)(12*g)});
    }
    DrawRectangleRounded((Rectangle){backR.x+3,backR.y+4,backR.width,backR.height},
                         0.4f,8,(Color){0,0,0,90});
    DrawRectangleRounded(backR, 0.4f, 8,
                         bhov?(Color){70,12,12,240}:(Color){30,8,8,230});
    DrawRectangleRoundedLines(backR, 0.4f, 8,
                              bhov?bkAccent:(Color){150,35,35,200});
    DrawCornerAccents(backR,(Color){bkAccent.r,bkAccent.g,bkAccent.b,bhov?200u:100u},8);

    const char *bkLabel = "<  BACK";
    int bklw = MeasureText(bkLabel, 17);
    DrawText(bkLabel, bkX+bkW/2-bklw/2+1, bkY+bkH/2-9+1, 17, (Color){0,0,0,100});
    DrawText(bkLabel, bkX+bkW/2-bklw/2,   bkY+bkH/2-9,   17,
             bhov?(Color){255,180,180,255}:(Color){230,100,100,230});

    return backR;
}

void GetMenuIconRects(Rectangle *settingsR, Rectangle *musicR, Rectangle *futureR) {
    if (settingsR) *settingsR = (Rectangle){ 10.0f, 16.0f, 76.0f, 76.0f };
    if (musicR)    *musicR    = (Rectangle){ 96.0f, 16.0f, 76.0f, 76.0f };
    if (futureR)   *futureR   = (Rectangle){182.0f, 16.0f, 76.0f, 76.0f };
}

// ─── SETTINGS UI HELPERS (private to the settings sub-screen) ────────────────

// Horizontal draggable slider. Updates *val while LMB is held. Returns true if changed.
static bool SettingsSlider(int x, int y, int w, int h,
                            float *val, float vmin, float vmax, Color accent) {
    float frac = (*val - vmin) / (vmax - vmin);
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    int kx = x + 2 + (int)(frac * (w - 8));

    DrawRectangleRounded((Rectangle){(float)x,(float)y,(float)w,(float)h},
                          0.5f, 6, (Color){20,16,32,200});
    DrawRectangleRoundedLines((Rectangle){(float)x,(float)y,(float)w,(float)h},
                               0.5f, 6, (Color){accent.r,accent.g,accent.b,70});
    if (kx - x > 4)
        DrawRectangleRounded((Rectangle){(float)(x+2),(float)(y+2),(float)(kx-x-2),(float)(h-4)},
                              0.5f, 6, (Color){accent.r,accent.g,accent.b,180});
    DrawCircle(kx, y+h/2, (float)(h/2+2), (Color){accent.r,accent.g,accent.b,230});
    DrawCircle(kx, y+h/2, (float)(h/2-1), (Color){230,220,255,170});

    Vector2 mp = GetMousePosition();
    Rectangle hr = {(float)(x-4),(float)(y-4),(float)(w+8),(float)(h+8)};
    if (CheckCollisionPointRec(mp, hr) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float nv = vmin + ((mp.x - (float)x) / (float)w) * (vmax - vmin);
        if (nv < vmin) nv = vmin;
        if (nv > vmax) nv = vmax;
        *val = nv;
        return true;
    }
    return false;
}

// ON/OFF pill toggle. Clicks toggle *flag. Returns true if changed this frame.
static bool SettingsToggle(int x, int y, int w, int h, bool *flag, Color accent) {
    Rectangle r = {(float)x,(float)y,(float)w,(float)h};
    bool changed = false;
    if (CheckCollisionPointRec(GetMousePosition(), r) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        { *flag = !*flag; changed = true; }
    Color bg  = *flag ? (Color){accent.r/6, accent.g/6, accent.b/6, 220}
                      : (Color){18,14,28,200};
    Color bdr = *flag ? (Color){accent.r,accent.g,accent.b,200}
                      : (Color){55,48,82,140};
    DrawRectangleRounded(r, 0.35f, 8, bg);
    DrawRectangleRoundedLines(r, 0.35f, 8, bdr);
    const char *lbl = *flag ? "ON" : "OFF";
    int lw = MeasureText(lbl, 16);
    Color tc = *flag ? (Color){accent.r,accent.g,accent.b,255} : (Color){70,62,100,200};
    DrawText(lbl, x + w/2 - lw/2, y + h/2 - 8, 16, tc);
    return changed;
}

// ─── SOUNDTRACK SHOP ART ─────────────────────────────────────────────────────
// The music screen deliberately uses the visual language of the skin shop:
// every track gets its own collectible-looking artwork and a clear equip card.
static Color MusicTrackAccent(int track) {
    if (track == 0) return (Color){75,175,255,255};   // Dungeon Crawler
    if (track == 1) return (Color){255,105,70,255};   // Dangerzone
    if (track == 2) return (Color){255,205,75,255};   // Treasure Hunter
    if (track == 3) return (Color){205,105,255,255};  // Neon Void
    if (track == 4) return (Color){105,190,255,255};  // Safe Haven
    if (track == 5) return (Color){255,205,75,255};   // Insert Coin
    if (track == 6) return (Color){255,115,55,255};   // Pixel Panic
    if (track == 7) return (Color){255,215,90,255};   // Overachievers Overtune
    if (track == 8) return (Color){255,80,220,255};   // Neon Pulse
    if (track == -1) return (Color){155,165,190,255}; // No Music
    return (Color){115,235,220,255};                  // special-card fallback
}

static void DrawMusicArtwork(Rectangle r, int track, float t, bool active) {
    Color ac = MusicTrackAccent(track);
    int x = (int)r.x, y = (int)r.y, w = (int)r.width, h = (int)r.height;
    float pulse = 0.5f + 0.5f*sinf(t*2.2f + (float)(track + 2));

    DrawRectangleRounded(r, 0.12f, 8, (Color){8,9,20,255});

    if (track == 0) {
        // Dungeon Crawler: a torch-lit stone entrance with a blue portal.
        for (int row = 0; row < 4; row++) {
            int by = y + 8 + row * 14;
            int offset = (row % 2) ? 12 : 0;
            for (int col = -1; col < 7; col++) {
                int bx = x + offset + col * 31;
                Color stone = (Color){18 + (uchar)(row * 3),
                                      22 + (uchar)(row * 3),
                                      35 + (uchar)(row * 5), 255};
                DrawRectangle(bx, by, 29, 12, stone);
                DrawLine(bx+2, by+11, bx+27, by+11,
                         (Color){55,62,82,150});
            }
        }

        int portalX = x + w/2;
        int portalY = y + h/2 + 4;
        DrawRectangle(x+26, y+17, w-52, h-30, (Color){5,8,18,245});
        DrawRectangleLinesEx((Rectangle){(float)(x+26),(float)(y+17),
                                        (float)(w-52),(float)(h-30)},
                             3.0f, (Color){66,73,97,230});
        DrawCircle(portalX, portalY, (int)(26 + 3*pulse), (Color){45,145,255,28});
        DrawCircle(portalX, portalY, 19, (Color){19,63,112,245});
        DrawCircleLines(portalX, portalY, 20 + 2*pulse,
                        (Color){95,205,255,230});
        DrawCircleLines(portalX, portalY, 13,
                        (Color){155,235,255,125});
        DrawCircle(portalX, portalY, 5, (Color){210,250,255,230});

        // Small rune-like rays make the portal feel active without adding text.
        for (int i = 0; i < 4; i++) {
            float a = t * 0.7f + i * (PI * 0.5f);
            Vector2 p0 = {portalX + cosf(a)*24.0f, portalY + sinf(a)*24.0f};
            Vector2 p1 = {portalX + cosf(a)*31.0f, portalY + sinf(a)*31.0f};
            DrawLineEx(p0, p1, 2.0f, (Color){115,220,255,170});
        }

        // Warm torches frame the cold portal.
        for (int side = 0; side < 2; side++) {
            int tx = side == 0 ? x+20 : x+w-20;
            int ty = y+h-27;
            DrawCircle(tx, ty-13, (int)(17 + 3*pulse), (Color){255,125,45,22});
            DrawLineEx((Vector2){(float)tx,(float)ty},
                       (Vector2){(float)tx,(float)(ty-18)}, 3.0f,
                       (Color){116,76,48,255});
            DrawPoly((Vector2){(float)tx,(float)(ty-23)}, 5,
                     8.0f + 2.0f*pulse, 0.0f, (Color){255,157,53,235});
            DrawCircle(tx, ty-23, 3.0f, (Color){255,235,145,255});
        }
        DrawLineEx((Vector2){(float)(x+10),(float)(y+h-9)},
                   (Vector2){(float)(x+w-10),(float)(y+h-9)}, 2.0f,
                   (Color){76,91,120,190});
    } else if (track == 1) {
        // Hot diagonal energy and a warning shard for Dangerzone.
        DrawCircle(x+w/2, y+h/2-2, (int)(38+5*pulse), (Color){255,70,45,24});
        for (int i = -2; i < 8; i++) {
            int lx = x - 30 + i*36;
            DrawLineEx((Vector2){(float)lx,(float)(y+h-8)},
                       (Vector2){(float)(lx+72),(float)(y+8)}, 2.0f,
                       (Color){255,95,55,(uchar)(45 + (i == 3 ? 90 : 0))});
        }
        DrawPoly((Vector2){(float)(x+w/2),(float)(y+h/2-2)}, 6,
                 31.0f + 4.0f*pulse, 30.0f, (Color){255,105,70,220});
        DrawPoly((Vector2){(float)(x+w/2),(float)(y+h/2-2)}, 6,
                 19.0f + 3.0f*pulse, 30.0f, (Color){57,13,25,255});
        DrawLineEx((Vector2){(float)(x+28),(float)(y+h-27)},
                   (Vector2){(float)(x+57),(float)(y+h-43)}, 3.0f,
                   (Color){255,235,150,220});
        DrawLineEx((Vector2){(float)(x+57),(float)(y+h-43)},
                   (Vector2){(float)(x+81),(float)(y+h-34)}, 3.0f,
                   (Color){255,235,150,220});
    } else if (track == 2) {
        // Gold rings and a treasure diamond in a midnight vault.
        DrawCircle(x+w/2, y+h/2-2, 41, (Color){255,195,50,20});
        DrawCircleLines(x+w/2, y+h/2-2, 36, (Color){255,205,75,145});
        DrawCircleLines(x+w/2, y+h/2-2, 27, (Color){255,225,130,90});
        DrawPoly((Vector2){(float)(x+w/2),(float)(y+h/2-5)}, 4,
                 24.0f + 4.0f*pulse, 45.0f, (Color){255,213,82,235});
        DrawPoly((Vector2){(float)(x+w/2),(float)(y+h/2-5)}, 4,
                 13.0f, 45.0f, (Color){111,57,26,255});
        DrawCircle(x+24, y+28, 4, (Color){255,232,135,170});
        DrawCircle(x+w-24, y+h-42, 3, (Color){255,196,55,180});
        DrawLineEx((Vector2){(float)(x+12),(float)(y+h-23)},
                   (Vector2){(float)(x+w-12),(float)(y+h-23)}, 2.0f,
                   (Color){255,205,75,120});
    } else if (track == 3) {
        // A neon void: an animated portal over a perspective grid.
        for (int i = 0; i < 5; i++) {
            int yy = y+h-18-i*13;
            DrawLineEx((Vector2){(float)(x+8),(float)yy},
                       (Vector2){(float)(x+w-8),(float)yy}, 1.0f,
                       (Color){95,215,255,(uchar)(55+i*15)});
        }
        for (int i = -3; i <= 3; i++) {
            DrawLineEx((Vector2){(float)(x+w/2),(float)(y+h-9)},
                       (Vector2){(float)(x+w/2+i*40),(float)(y+h-56)}, 1.0f,
                       (Color){205,105,255,95});
        }
        for (int i = 3; i >= 1; i--)
            DrawCircleLines(x+w/2, y+h/2-4, 13.0f*i + 4.0f*pulse,
                            (Color){(uchar)(95+35*i),(uchar)(205-20*i),255,(uchar)(45+35*i)});
        DrawCircle(x+w/2, y+h/2-4, 7, (Color){215,125,255,220});
        DrawCircle(x+w/2, y+h/2-4, 3, (Color){225,250,255,255});
    } else if (track == 4) {
        // Safe Haven: a cold moon above a defensive skyline.
        int mx = x + w - 42, my = y + 33;
        DrawCircle(mx, my, 30.0f, (Color){95,180,255,24});
        DrawCircle(mx, my, 20.0f, (Color){195,235,255,235});
        DrawCircle(mx + 7, my - 5, 20.0f, (Color){10,18,42,255});
        const int stars[7][2] = {{18,19},{47,37},{78,17},{112,29},{145,14},{178,43},{210,20}};
        for (int i = 0; i < 7; i++)
            DrawCircle(x + 10 + stars[i][0] % (w-20),
                       y + 8 + stars[i][1] % (h-46),
                       (i % 3 == 0) ? 2.0f : 1.0f,
                       (Color){190,235,255,(uchar)(120 + 65*pulse)});
        int by = y + h - 28;
        DrawRectangle(x+8, by, w-16, 22, (Color){18,35,70,245});
        for (int i = 0; i < 4; i++) {
            int tx = x + 18 + i*(w-46)/4;
            DrawRectangle(tx, by-17-(i%2)*5, 22, 17+(i%2)*5, (Color){25,52,94,255});
            DrawRectangle(tx+6, by-26-(i%2)*5, 10, 9, (Color){42,80,125,240});
            DrawRectangle(tx+8, by-10, 6, 7, (Color){110,205,255,150});
        }
        DrawLineEx((Vector2){(float)(x+8),(float)(by-1)},
                   (Vector2){(float)(x+w-8),(float)(by-1)}, 2.0f,
                   (Color){105,190,255,190});
    } else if (track == 5) {
        // Insert Coin: a molten core with sparks rising through the dark.
        DrawCircle(x+w/2, y+h/2+7, (int)(38+5*pulse), (Color){255,65,25,26});
        DrawPoly((Vector2){(float)(x+w/2),(float)(y+h/2+4)}, 6,
                 31.0f + 4.0f*pulse, 30.0f, (Color){255,105,45,220});
        DrawPoly((Vector2){(float)(x+w/2),(float)(y+h/2+4)}, 6,
                 18.0f + 3.0f*pulse, 30.0f, (Color){105,28,23,255});
        for (int i = 0; i < 11; i++) {
            float phase = t * (1.1f + (i % 3)*0.18f) + i*1.7f;
            int ex = x + 14 + (i*37) % (w-28);
            int ey = y + h - 19 - (int)(fmodf(phase*27.0f + i*11.0f, (float)(h-28)));
            int len = 4 + (i % 4)*2;
            DrawLineEx((Vector2){(float)ex,(float)ey},
                       (Vector2){(float)(ex+2),(float)(ey-len)}, 2.0f,
                       (Color){255,190,75,(uchar)(110 + (i%3)*40)});
        }
        DrawLineEx((Vector2){(float)(x+12),(float)(y+h-20)},
                   (Vector2){(float)(x+w-12),(float)(y+h-20)}, 2.0f,
                   (Color){255,115,55,165});
    } else if (track == 6) {
        // Pixel Panic: a forged core caught inside a cyan lightning front.
        int cx = x+w/2, cy = y+h/2-3;
        DrawCircle(cx, cy, (int)(40+4*pulse), (Color){45,210,235,22});
        DrawCircleLines(cx, cy, 34.0f + 4.0f*pulse, (Color){80,220,235,170});
        DrawCircleLines(cx, cy, 22.0f, (Color){170,250,255,100});
        DrawPoly((Vector2){(float)cx,(float)cy}, 6, 22.0f, 30.0f,
                 (Color){90,210,235,210});
        DrawPoly((Vector2){(float)cx,(float)cy}, 6, 11.0f, 30.0f,
                 (Color){17,50,70,255});
        DrawLineEx((Vector2){(float)(x+20),(float)(y+12)},
                   (Vector2){(float)(x+55),(float)(y+31)}, 2.0f,
                   (Color){210,255,255,220});
        DrawLineEx((Vector2){(float)(x+55),(float)(y+31)},
                   (Vector2){(float)(x+44),(float)(y+49)}, 2.0f,
                   (Color){120,235,255,220});
        DrawLineEx((Vector2){(float)(x+w-17),(float)(y+h-14)},
                   (Vector2){(float)(x+w-54),(float)(y+h-34)}, 2.0f,
                   (Color){210,255,255,190});
        DrawLineEx((Vector2){(float)(x+w-54),(float)(y+h-34)},
                   (Vector2){(float)(x+w-43),(float)(y+h-51)}, 2.0f,
                   (Color){120,235,255,220});
    } else if (track == 7) {
        // Overachievers Overtune: a gold crown and a rising achievement star.
        int cx = x+w/2, cy = y+h/2+2;
        DrawCircle(cx, cy, (int)(39+5*pulse), (Color){255,205,75,22});
        DrawCircleLines(cx, cy, 31.0f + 3.0f*pulse, (Color){255,215,90,160});
        DrawPoly((Vector2){(float)cx,(float)(y+18)}, 5, 14.0f, 0.0f,
                 (Color){255,235,145,230});
        Vector2 crown[6] = {
            {(float)(cx-31),(float)(cy+13)}, {(float)(cx-23),(float)(cy-14)},
            {(float)(cx-6),(float)(cy+1)}, {(float)cx,(float)(cy-22)},
            {(float)(cx+9),(float)(cy+1)}, {(float)(cx+29),(float)(cy-14)}
        };
        for (int i = 0; i < 5; i++)
            DrawLineEx(crown[i], crown[i+1], 3.0f, (Color){255,215,90,235});
        DrawLineEx(crown[0], crown[5], 3.0f, (Color){255,215,90,235});
        DrawLineEx((Vector2){(float)(cx-31),(float)(cy+13)},
                   (Vector2){(float)(cx+29),(float)(cy+13)}, 5.0f,
                   (Color){255,185,50,230});
        DrawCircle(cx-23, cy-14, 3.0f, (Color){255,250,205,255});
        DrawCircle(cx, cy-22, 3.0f, (Color){255,250,205,255});
        DrawCircle(cx+9, cy+1, 3.0f, (Color){255,250,205,255});
    } else if (track == 8) {
        // Neon Pulse: a magenta/cyan heartbeat travelling through a dark grid.
        int cx = x+w/2, cy = y+h/2-2;
        for (int i = 0; i < 5; i++) {
            int yy = y + h - 18 - i*13;
            DrawLineEx((Vector2){(float)(x+8),(float)yy},
                       (Vector2){(float)(x+w-8),(float)yy}, 1.0f,
                       (Color){75,190,235,(uchar)(40+i*14)});
        }
        for (int i = -3; i <= 3; i++) {
            DrawLineEx((Vector2){(float)cx,(float)(y+h-8)},
                       (Vector2){(float)(cx+i*40),(float)(y+h-55)}, 1.0f,
                       (Color){255,80,220,90});
        }
        DrawCircle(cx, cy, (int)(35+5*pulse), (Color){255,55,205,24});
        DrawCircleLines(cx, cy, 29.0f + 4.0f*pulse,
                        (Color){255,80,220,185});
        DrawCircleLines(cx, cy, 19.0f + 2.0f*pulse,
                        (Color){70,225,255,170});
        DrawPoly((Vector2){(float)cx,(float)cy}, 8,
                 15.0f + 3.0f*pulse, 22.5f,
                 (Color){255,105,225,220});
        DrawCircle(cx, cy, 5, (Color){235,255,255,245});
        DrawLineEx((Vector2){(float)(x+20),(float)(cy+2)},
                   (Vector2){(float)(x+52),(float)(cy-9)}, 2.0f,
                   (Color){95,240,255,220});
        DrawLineEx((Vector2){(float)(x+52),(float)(cy-9)},
                   (Vector2){(float)(x+70),(float)(cy+7)}, 2.0f,
                   (Color){255,95,220,220});
        DrawLineEx((Vector2){(float)(x+w-20),(float)(cy+2)},
                   (Vector2){(float)(x+w-52),(float)(cy-9)}, 2.0f,
                   (Color){95,240,255,190});
    } else if (track == -1) {
        // Silent mode: a quiet signal crossed out, rather than another track.
        DrawCircleLines(x+w/2, y+h/2-4, 27, (Color){155,165,190,90});
        DrawLineEx((Vector2){(float)(x+27),(float)(y+25)},
                   (Vector2){(float)(x+w-27),(float)(y+h-29)}, 4.0f,
                   (Color){205,215,235,210});
        DrawLineEx((Vector2){(float)(x+w-27),(float)(y+25)},
                   (Vector2){(float)(x+27),(float)(y+h-29)}, 1.0f,
                   (Color){100,115,145,90});
    } else {
        // Special-card fallback: a clean animated signal/orbit emblem.
        DrawCircle(x+w/2, y+h/2-4, 36, (Color){90,230,220,18});
        DrawCircleLines(x+w/2, y+h/2-4, 31 + 3*pulse, (Color){115,235,220,155});
        DrawCircleLines(x+w/2, y+h/2-4, 19, (Color){90,170,255,115});
        DrawLineEx((Vector2){(float)(x+22),(float)(y+27)},
                   (Vector2){(float)(x+w-22),(float)(y+h-28)}, 3.0f,
                   (Color){115,235,220,180});
        DrawLineEx((Vector2){(float)(x+w-22),(float)(y+27)},
                   (Vector2){(float)(x+22),(float)(y+h-28)}, 2.0f,
                   (Color){90,170,255,130});
        DrawCircle(x+w/2, y+h/2-4, 5, (Color){225,255,250,230});
    }

    // Small moving equalizer bars make the artwork unmistakably musical.
    int barW = (w - 28) / 9;
    for (int i = 0; i < 9; i++) {
        float wave = 0.5f + 0.5f*sinf(t*3.0f + i*0.72f + track*0.4f);
        int bh = 5 + (int)(10.0f*wave);
        DrawRectangle(x+14+i*barW, y+h-13-bh, barW-3, bh,
                      (Color){ac.r, ac.g, ac.b, (uchar)(active ? 220 : 145)});
    }
    DrawRectangleLinesEx(r, 1.0f, (Color){ac.r, ac.g, ac.b, active ? 170 : 75});
}

static bool DrawSoundtrackCard(Rectangle r, int track,
                               bool selected, bool unlocked, MusicAccess access,
                               int price, float t) {
    Vector2 mp = GetMousePosition();
    bool hover = CheckCollisionPointRec(mp, r);
    Color ac = MusicTrackAccent(track);
    bool locked = track >= 0 && !unlocked;

    if (selected || (hover && unlocked)) {
        for (int g = 4; g >= 1; g--) {
            DrawRectangleRounded(
                (Rectangle){r.x-g*2.0f, r.y-g*2.0f,
                            r.width+g*4.0f, r.height+g*4.0f},
                0.14f, 8, (Color){ac.r, ac.g, ac.b,
                                  (uchar)((selected ? 5 : 3) * g)});
        }
    }

    Color fill = selected ? (Color){18,24,42,250}
                          : hover ? (Color){18,18,35,245}
                                  : (Color){11,10,23,235};
    DrawRectangleRounded(r, 0.10f, 8, fill);
    DrawRectangleRoundedLines(r, 0.10f, 8,
                              (Color){ac.r, ac.g, ac.b,
                                      (uchar)(selected ? 230 : (hover ? 185 : 95))});
    if (selected) {
        Rectangle inner = {r.x+4, r.y+4, r.width-8, r.height-8};
        DrawRectangleRoundedLines(inner, 0.09f, 8,
                                  (Color){ac.r, ac.g, ac.b, 55});
    }

    int pad = 12;
    Rectangle art = {r.x+(float)pad, r.y+(float)pad,
                     r.width-(float)(pad*2), r.height*0.45f};
    DrawMusicArtwork(art, track, t, selected || (hover && unlocked));
    if (locked) {
        DrawRectangleRounded(art, 0.12f, 8, (Color){5,6,15,165});
        const char *lockText = "LOCKED";
        int lockW = MeasureText(lockText, 14);
        DrawText(lockText, (int)(art.x+art.width/2-lockW/2),
                 (int)(art.y+art.height/2-7), 14,
                 (Color){225,215,235,220});
    }

    const char *title = (track >= 0) ? GetMusicTrackName(track) : "NO MUSIC";
    int titleFs = 18;
    while (titleFs > 12 && MeasureText(title, titleFs) > (int)r.width - pad*2)
        titleFs--;
    DrawText(title, (int)r.x+pad, (int)(art.y+art.height+12), titleFs,
             (Color){235,232,248,245});
    const char *detail = track < 0 ? "SILENT SLOT"
                        : unlocked ? "ORIGINAL SCORE"
                        : access == MUSIC_ACCESS_PURCHASE
                            ? TextFormat("%d COINS", price)
                            : GetMusicTrackAccessLabel(track);
    DrawText(detail,
             (int)r.x+pad, (int)(art.y+art.height+35), 10,
              locked ? (Color){205,145,170,210} : (Color){145,140,175,185});

    Rectangle equip = {r.x+(float)pad, r.y+r.height-42.0f,
                       r.width-(float)(pad*2), 28.0f};
    bool equipHover = CheckCollisionPointRec(mp, equip);
    Color equipFill = selected ? (Color){ac.r/5, ac.g/5, ac.b/5, 240}
                               : equipHover ? (Color){ac.r/7, ac.g/7, ac.b/7, 235}
                                            : (Color){12,12,25,230};
    DrawRectangleRounded(equip, 0.30f, 6, equipFill);
    DrawRectangleRoundedLines(equip, 0.30f, 6,
                              (Color){ac.r, ac.g, ac.b,
                                      (uchar)(selected ? 220 : 130)});
    const char *buttonText;
    if (selected) buttonText = "SELECTED";
    else if (locked && access == MUSIC_ACCESS_PURCHASE)
        buttonText = coin_balance >= price ? TextFormat("BUY  %d", price) : "NEED COINS";
    else if (locked) buttonText = "LOCKED";
    else buttonText = "SELECT";
    int bw = MeasureText(buttonText, 12);
    DrawText(buttonText, (int)(equip.x+equip.width/2-bw/2), (int)equip.y+8,
             12, selected ? (Color){235,255,252,255}
                           : locked ? (Color){225,170,190,225}
                                    : (Color){ac.r, ac.g, ac.b, 235});
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// Returns the number of achievements currently at their maximum tier. This
// mirrors the 30 rows used by the Perfectionist achievement in ACHIEVEMENTS.
static int CountMaxedAchievementsForPerfectionist(void) {
    int _skins_b = 0;
    for (int _i = 1; _i < SKIN_COUNT; _i++) if (skin_owned[_i]) _skins_b++;
    int _upg_b  = spd_level + shd_level + wlk_level + swd_level + bmb_level + gld_level;
    int _perk_b = perk_slots + perk_magnet + perk_shield2;
    struct { int v; int t[6]; int max_t; } _a[] = {
        { global_games_played,    { 25,  50,  100,  250,  500,  1000 }, 6 },
        { global_rounds,          { 100, 250,  800, 2000, 4000, 10000 }, 6 },
        { global_totcoi,          { 250,1000, 5000,12000,40000,100000 }, 6 },
        { global_totkills,        { 75, 160, 350, 1000, 2500, 5000 }, 6 },
        { hs_trs,                 { 10,  20,   25,   30,   40,    50 }, 6 },
        { hs_coi,                 { 20,  50,   75,  100,  150,   250 }, 6 },
        { hs_kills,               {  5,  10,   20,   35,   50,    75 }, 6 },
        { global_tottrs,          { 100, 300,  800, 1500, 4000, 10000 }, 6 },
        { global_kills_sword,     { 25, 100,  300,  750, 999999, 999999 }, 4 },
        { global_kills_shield,    { 15,  60,  200,  500, 999999, 999999 }, 4 },
        { global_portals,         { 15,  50,  150,  400, 999999, 999999 }, 4 },
        { global_ghostwalk_walls, { 10,  40,  120,  300, 999999, 999999 }, 4 },
        { global_inv_full,        { 10,  35,  100,  250, 999999, 999999 }, 4 },
        { global_use_spd,         { 20,  75,  200,  500, 999999, 999999 }, 4 },
        { global_use_shd,         { 20,  75,  200,  500, 999999, 999999 }, 4 },
        { global_use_bmb,         { 20,  75,  200,  500, 999999, 999999 }, 4 },
        { global_use_wlk,         { 20,  75,  200,  500, 999999, 999999 }, 4 },
        { global_use_swd,         { 20,  75,  200,  500, 999999, 999999 }, 4 },
        { _skins_b,               {  1,   5,   15,   25, 999999, 999999 }, 4 },
        { _upg_b,                 {  2,   6,   12,   20, 999999, 999999 }, 4 },
        { _perk_b,                {  1,   2,    4,    6, 999999, 999999 }, 4 },
        { global_bosses_killed,   {  6,  18,   48,   90,  180,   300 }, 6 },
        { hs_round,               {  5,  10,   20,   30,   40,    50 }, 6 },
        { hs_coi_round,           { 38,  75,  125,  200, 999999, 999999 }, 4 },
        { global_use_gld,         {  5,  20,   60,  150, 999999, 999999 }, 4 },
        { global_rooms_trap,      {  3,  10,   30,    80, 999999, 999999 }, 4 },
        { global_rooms_dark,      {  3,  10,   30,    80, 999999, 999999 }, 4 },
        { global_rooms_lava,      {  3,  10,   30,    80, 999999, 999999 }, 4 },
        { global_rooms_coinrush,  {  2,   8,   25,    60, 999999, 999999 }, 4 },
        { global_cannon_hits,     {  5,  20,   50,   120, 999999, 999999 }, 4 },
    };

    int _maxed = 0;
    int _count = (int)(sizeof(_a) / sizeof(_a[0]));
    for (int _i = 0; _i < _count; _i++) {
        int _tier = 0;
        for (int _j = 0; _j < 6; _j++) {
            if (_a[_i].v >= _a[_i].t[_j]) _tier = _j + 1; else break;
        }
        if (_tier == _a[_i].max_t) _maxed++;
    }
    return _maxed;
}

// Overachiever Tier 6 is based on the total number of earned tiers across
// the 30 base achievements. Keep this table aligned with ACHIEVEMENTS above.
static int CountEarnedAchievementTiersForOverachiever(void) {
    int _skins_b = 0;
    for (int _i = 1; _i < SKIN_COUNT; _i++) if (skin_owned[_i]) _skins_b++;
    int _upg_b  = spd_level + shd_level + wlk_level + swd_level + bmb_level + gld_level;
    int _perk_b = perk_slots + perk_magnet + perk_shield2;
    struct { int v; int t[6]; int max_t; } _a[] = {
        { global_games_played,     { 25,  50,  100,  250,  500,  1000 }, 6 },
        { global_rounds,           { 100, 250,  800, 2000, 4000, 10000 }, 6 },
        { global_totcoi,           { 250,1000, 5000,12000,40000,100000 }, 6 },
        { global_totkills,         { 75, 160, 350, 1000, 2500, 5000 }, 6 },
        { hs_trs,                  { 10,  20,   25,   30,   40,    50 }, 6 },
        { hs_coi,                  { 20,  50,   75,  100,  150,   250 }, 6 },
        { hs_kills,                {  5,  10,   20,   35,   50,    75 }, 6 },
        { global_tottrs,            { 100, 300,  800, 1500, 4000, 10000 }, 6 },
        { global_kills_sword,      { 25, 100,  300,  750, 999999, 999999 }, 4 },
        { global_kills_shield,     { 15,  60,  200,  500, 999999, 999999 }, 4 },
        { global_portals,          { 15,  50,  150,  400, 999999, 999999 }, 4 },
        { global_ghostwalk_walls,  { 10,  40,  120,  300, 999999, 999999 }, 4 },
        { global_inv_full,         { 10,  35,  100,  250, 999999, 999999 }, 4 },
        { global_use_spd,          { 20,  75,  200,  500, 999999, 999999 }, 4 },
        { global_use_shd,          { 20,  75,  200,  500, 999999, 999999 }, 4 },
        { global_use_bmb,          { 20,  75,  200,  500, 999999, 999999 }, 4 },
        { global_use_wlk,          { 20,  75,  200,  500, 999999, 999999 }, 4 },
        { global_use_swd,          { 20,  75,  200,  500, 999999, 999999 }, 4 },
        { _skins_b,                {  1,   5,   15,   25, 999999, 999999 }, 4 },
        { _upg_b,                 {  2,   6,   12,   20, 999999, 999999 }, 4 },
        { _perk_b,                {  1,   2,    4,    6, 999999, 999999 }, 4 },
        { global_bosses_killed,    {  6,  18,   48,   90,  180,    300 }, 6 },
        { hs_round,                {  5,  10,   20,   30,   40,     50 }, 6 },
        { hs_coi_round,            { 38,  75,  125,  200, 999999, 999999 }, 4 },
        { global_use_gld,          {  5,  20,   60,  150, 999999, 999999 }, 4 },
        { global_rooms_trap,       {  3,  10,   30,    80, 999999, 999999 }, 4 },
        { global_rooms_dark,       {  3,  10,   30,    80, 999999, 999999 }, 4 },
        { global_rooms_lava,       {  3,  10,   30,    80, 999999, 999999 }, 4 },
        { global_rooms_coinrush,   {  2,   8,   25,    60, 999999, 999999 }, 4 },
        { global_cannon_hits,      {  5,  20,   50,   120, 999999, 999999 }, 4 },
    };

    int _earned = 0;
    int _count = (int)(sizeof(_a) / sizeof(_a[0]));
    for (int _i = 0; _i < _count; _i++) {
        int _tier = 0;
        for (int _j = 0; _j < 6; _j++) {
            if (_a[_i].v >= _a[_i].t[_j]) _tier = _j + 1; else break;
        }
        if (_tier > _a[_i].max_t) _tier = _a[_i].max_t;
        _earned += _tier;
    }
    return _earned;
}

// Auto-unlocks the achievement-locked skins once the underlying achievement(s)
// reach Tier 6. Checked once per frame while any menu is open.
static void CheckAchievementSkinUnlocks(void) {
    bool changed = false;
    if (!music_owned[MUSIC_OVERACHIEVER_TRACK] &&
        CountEarnedAchievementTiersForOverachiever() >= 140) {
        music_owned[MUSIC_OVERACHIEVER_TRACK] = 1;
        changed = true;
    }
    if (!skin_owned[30] && global_games_played >= 1000)               { skin_owned[30] = 1; changed = true; }
    if (!skin_owned[31] && global_rounds       >= 10000)              { skin_owned[31] = 1; changed = true; }
    if (!skin_owned[32] && global_totcoi >= 100000 && hs_coi >= 250)  { skin_owned[32] = 1; changed = true; }
    if (!skin_owned[33] && global_totkills >= 5000)                    { skin_owned[33] = 1; changed = true; }
    if (!skin_owned[41] && CountMaxedAchievementsForPerfectionist() >= 30) {
        skin_owned[41] = 1;
        changed = true;
    }
    if (changed) SaveShop();
}

void DrawMenu(float t) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    CheckAchievementSkinUnlocks();
    DrawMenuBG(t, sw, sh);

    // ── TUTORIAL ─────────────────────────────────────────────────────────────
    if (menuSub == 1) {
        Color ac = (Color){100,190,255,255};
        DrawSubPanel("TUTORIAL", ac, t, sw, sh);
        int margin2=32, pw=sw-margin2*2;
        int px2=margin2, py2=margin2;

        int col1x = px2+30, col2x = px2+pw/2+20;
        int topy  = py2+72;

        // ── Left column: SYMBOLS ─────────────────────────────────────────────
        DrawText("SYMBOLS", col1x, topy, 17, (Color){ac.r,ac.g,ac.b,210});
        DrawLine(col1x, topy+22, col1x+260, topy+22, (Color){ac.r,ac.g,ac.b,55});

        struct { const char* desc; Color col; } leg[] = {
            { "Treasure  Reach to advance", CTRES },
            { "Portal   Skip to next round", CPORT },
            { "Trap     Instant death!",     (Color){195,80,80,255} },
            { "Trapdoor Looks like floor!",  (Color){60,55,75,255} },
            { "Coin     Collect for score",  CCOIN },
            { "Item     Use with 1 / 2 / 3", IK_COL[IK_SPD] },
            { "Enemy    They will chase you", CENE },
            { "Boss     Kill it to advance!", CBOSS },
        };
        const int legN = (int)(sizeof(leg)/sizeof(leg[0]));
        for (int i=0; i<legN; i++) {
            int ry2 = topy+32+i*40;
            Color rowbg=(i%2==0)?(Color){20,16,34,130}:(Color){16,13,28,70};
            DrawRectangleRounded((Rectangle){(float)col1x-4,(float)ry2-2,290,36},0.2f,6,rowbg);

            int wx = col1x, wy = ry2 + 4;
            int cs = 28;
            DrawRectangle(wx, wy, cs, cs, CB);
            DrawRectangle(wx+1, wy+1, cs-2, cs-2, CF);

            if (i == 0) {
                float p = 0.5f + 0.5f*sinf(t*3.0f);
                Color tc = CTRES; tc.a = (uchar)(200+55*p);
                DrawRectangle(wx+4,  wy+6,  cs-8, cs-10, tc);
                DrawRectangle(wx+4,  wy+6,  cs-8, 3, (Color){255,240,180,230});
                DrawCircle(wx+cs/2, wy+cs/2+1, 2, (Color){200,150,30,255});
                DrawCircle(wx+cs/2, wy+cs/2, (int)(8+3*p), (Color){255,200,50,(int)(18+20*p)});
            } else if (i == 1) {
                float p = 0.5f + 0.5f*sinf(t*2.0f);
                DrawCircle(wx+cs/2, wy+cs/2, (int)(9+3*p), (Color){80,160,255,(int)(65+50*p)});
                DrawCircle(wx+cs/2, wy+cs/2, (int)(5+2*p), (Color){160,210,255,(int)(140+60*p)});
                DrawCircleLines(wx+cs/2, wy+cs/2, (int)(9+3*p), (Color){120,190,255,200});
            } else if (i == 2) {
                DrawRectangle(wx+2, wy+2, cs-4, cs-4, (Color){20,15,25,255});
                DrawRectangleLines(wx+2, wy+2, cs-4, cs-4, (Color){60,50,70,255});
                for (int gi=0; gi<3; gi++) {
                    for (int gj=0; gj<3; gj++) {
                        int spx = wx + 6 + gi*7;
                        int spy = wy + 6 + gj*7;
                        DrawCircle(spx, spy, 2, (Color){190,190,210,255});
                        DrawRectangle(spx-1, spy-2, 2, 2, CTRAP);
                    }
                }
            } else if (i == 3) {
                // Subtle hatch (matches in-game trapdoor 'h')
                DrawRectangle(wx+1, wy+1, cs-2, cs-2, (Color){20, 19, 32, 255});
                DrawLine(wx+4, wy+cs/2, wx+cs-4, wy+cs/2, (Color){14, 12, 22, 110});
                DrawPixel(wx+3, wy+3, (Color){42, 38, 56, 160});
                DrawPixel(wx+cs-4, wy+3, (Color){42, 38, 56, 160});
                DrawPixel(wx+3, wy+cs-4, (Color){42, 38, 56, 140});
                DrawPixel(wx+cs-4, wy+cs-4, (Color){42, 38, 56, 140});
                DrawRectangleLines(wx+2, wy+2, cs-4, cs-4, (Color){16, 14, 26, 70});
            } else if (i == 4) {
                float p = sinf(t*4.0f);
                int r = (int)(5 + p*1.2f);
                DrawCircle(wx+cs/2+1, wy+cs/2+2, r, (Color){160,120,0,80});
                DrawCircle(wx+cs/2,   wy+cs/2,   r, CCOIN);
                DrawCircle(wx+cs/2-1, wy+cs/2-1, (int)(r*0.4f)+1, (Color){255,245,160,200});
            } else if (i == 5) {
                Color ic = IK_COL[IK_SPD];
                float p = 0.5f + 0.5f*sinf(t*3.0f);
                DrawTriangle(
                    (Vector2){(float)(wx+cs/2), (float)(wy+3)},
                    (Vector2){(float)(wx+cs-3),  (float)(wy+cs/2)},
                    (Vector2){(float)(wx+cs/2), (float)(wy+cs-3)}, ic);
                DrawTriangle(
                    (Vector2){(float)(wx+cs/2), (float)(wy+3)},
                    (Vector2){(float)(wx+cs/2), (float)(wy+cs-3)},
                    (Vector2){(float)(wx+3),     (float)(wy+cs/2)}, ic);
                Color gc = ic; gc.a = (uchar)(55*p);
                DrawCircle(wx+cs/2, wy+cs/2, (int)(10+2*p), gc);
            } else if (i == 6) {
                DrawCircle(wx+cs/2, wy+cs/2, cs/2-3, CENE);
                DrawCircle(wx+cs/2-2, wy+cs/2-2, cs/4-1, (Color){240,100,100,110});
                DrawCircle(wx+cs/2-3, wy+cs/2-2, 2, (Color){255,200,0,255});
                DrawCircle(wx+cs/2+3, wy+cs/2-2, 2, (Color){255,200,0,255});
                DrawCircle(wx+cs/2-3, wy+cs/2-2, 1, (Color){20,0,0,255});
                DrawCircle(wx+cs/2+3, wy+cs/2-2, 1, (Color){20,0,0,255});
            } else if (i == 7) {
                DrawCircle(wx+cs/2, wy+cs/2+2, cs/2-3, CBOSS);
                DrawTriangle((Vector2){(float)(wx+cs/2-9), (float)(wy+8)},
                             (Vector2){(float)(wx+cs/2-3), (float)(wy+6)},
                             (Vector2){(float)(wx+cs/2-10),(float)(wy+1)},
                             (Color){235,220,90,255});
                DrawTriangle((Vector2){(float)(wx+cs/2+3), (float)(wy+6)},
                             (Vector2){(float)(wx+cs/2+9), (float)(wy+8)},
                             (Vector2){(float)(wx+cs/2+10),(float)(wy+1)},
                             (Color){235,220,90,255});
                DrawCircle(wx+cs/2-3, wy+cs/2, 2, (Color){255,200,0,255});
                DrawCircle(wx+cs/2+3, wy+cs/2, 2, (Color){255,200,0,255});
            }

            DrawText(leg[i].desc, col1x+34, ry2+6, 14, (Color){170,165,195,220});
        }

        // ── Right column: ITEMS ───────────────────────────────────────────────
        DrawText("ITEMS", col2x, topy, 17, (Color){ac.r,ac.g,ac.b,210});
        DrawLine(col2x, topy+22, col2x+320, topy+22, (Color){ac.r,ac.g,ac.b,55});

        const char *ileg[] = { "SPD","SHD","BMB","WLK","SWD","GLD" };
        const char *idesc[] = {
            "2x movement speed",
            "Block one trap or enemy hit",
            "Destroy all nearby traps",
            "Walk through walls",
            "Instantly kill nearby enemies",
            "Touching enemies and traps turns them to coins"
        };
        Color iicol[] = {IK_COL[IK_SPD],IK_COL[IK_SHD],IK_COL[IK_BMB],IK_COL[IK_WLK],IK_COL[IK_SWD],IK_COL[IK_GLD]};
        for (int i=0; i<6; i++) {
            int ry2 = topy+32+i*50;
            Color rowbg2=(i%2==0)?(Color){20,16,34,130}:(Color){16,13,28,70};
            DrawRectangleRounded((Rectangle){(float)col2x-4,(float)ry2-2,340,40},0.2f,6,rowbg2);

            int cs2 = 32;
            int iwx = col2x, iwy = ry2 + 4;
            DrawRectangle(iwx, iwy, cs2, cs2, CB);
            DrawRectangle(iwx+1, iwy+1, cs2-2, cs2-2, CF);
            Color ic = iicol[i];
            float p2 = 0.5f + 0.5f*sinf(t*3.0f + (float)i*0.8f);
            DrawTriangle(
                (Vector2){(float)(iwx+cs2/2), (float)(iwy+3)},
                (Vector2){(float)(iwx+cs2-3), (float)(iwy+cs2/2)},
                (Vector2){(float)(iwx+cs2/2), (float)(iwy+cs2-3)}, ic);
            DrawTriangle(
                (Vector2){(float)(iwx+cs2/2), (float)(iwy+3)},
                (Vector2){(float)(iwx+cs2/2), (float)(iwy+cs2-3)},
                (Vector2){(float)(iwx+3),     (float)(iwy+cs2/2)}, ic);
            Color gc2 = ic; gc2.a = (uchar)(55*p2);
            DrawCircle(iwx+cs2/2, iwy+cs2/2, (int)(12+3*p2), gc2);

            DrawText(idesc[i], col2x+40, ry2+5,  12, (Color){160,155,185,215});
            DrawText(ileg[i],  col2x+40, ry2+19, 13, ic);
        }

        // ── Controls table ────────────────────────────────────────────────────
        {
            int ctrlX2 = col1x;
            int ctrlY2 = topy + 32 + legN*40 + 8;
            DrawText("CONTROLS", ctrlX2, ctrlY2, 15, (Color){ac.r,ac.g,ac.b,210});
            DrawLine(ctrlX2, ctrlY2+19, ctrlX2+290, ctrlY2+19, (Color){ac.r,ac.g,ac.b,55});
            struct { const char *key; const char *desc; } ctrl[] = {
                { "WASD / Arrows", "Move player" },
                { "1 / 2 / 3",    "Use item" },
                { "E  (menu)",     "Enemies OFF: practice, no stats" },
                { "ENTER",         "Start / next round" },
                { "ESC",           "Pause / back" },
                { "F11",           "Fullscreen" },
            };
            for (int i = 0; i < 6; i++) {
                int ry3 = ctrlY2 + 26 + i * 28;
                Color rowbg2 = (i%2==0) ? (Color){20,16,34,110} : (Color){14,11,24,60};
                DrawRectangleRounded((Rectangle){(float)(ctrlX2-4),(float)(ry3-2),296,22},
                                     0.15f, 5, rowbg2);
                DrawRectangleRounded((Rectangle){(float)(ctrlX2-4),(float)(ry3-2),3,22},
                                     1.0f, 4, (Color){ac.r,ac.g,ac.b,90});
                DrawText(ctrl[i].key,  ctrlX2+6,   ry3+2, 13, (Color){ac.r,ac.g,ac.b,210});
                DrawText(ctrl[i].desc, ctrlX2+160, ry3+2, 13, (Color){155,150,185,200});
            }
        }

        // ── Explanation box (bottom right) ───────────────────────────────────
        {
            int bw = (pw/2) - 30;
            int bh = 152;
            int bx = col2x - 4;
            int by = py2 + (sh-margin2*2) - bh - 8;

            DrawRectangleRounded((Rectangle){(float)bx,(float)by,(float)bw,(float)bh},
                                 0.12f, 8, (Color){14,11,22,190});
            DrawRectangleRoundedLines((Rectangle){(float)bx,(float)by,(float)bw,(float)bh},
                                     0.12f, 8, (Color){ac.r,ac.g,ac.b,80});
            DrawRectangleRoundedLines(
                (Rectangle){(float)(bx+3),(float)(by+3),(float)(bw-6),(float)(bh-6)},
                0.12f, 8, (Color){ac.r,ac.g,ac.b,25});
            DrawRectangleRounded((Rectangle){(float)bx,(float)(by+8),4,(float)(bh-16)},
                                 1.0f, 4, (Color){ac.r,ac.g,ac.b,140});

            DrawText("HOW TO PLAY", bx+12, by+10, 15, (Color){ac.r,ac.g,ac.b,220});
            DrawLine(bx+12, by+28, bx+bw-12, by+28, (Color){ac.r,ac.g,ac.b,40});

            const char *lines[] = {
                "Navigate the dungeon and find the Treasure to advance.",
                "Avoid Traps! One touch and you're dead.",
                "Collect Coins and Items as you explore each floor.",
                "Enemies chase you. Use items wisely to survive.",
                "Each round the map grows larger and deadlier.",
                "Every 10th round a Boss guards the treasure.",
                "From round 6: trap, dark & lava rooms!",
            };
            for (int li=0; li<7; li++)
                DrawText(lines[li], bx+14, by+34+li*17, 13, (Color){155,150,185,210});
        }
        return;
    }

    // ── STATS ─────────────────────────────────────────────────────────────────
    if (menuSub == 2) {
        Color ac = (Color){255,205,55,255};
        DrawSubPanel("STATS", ac, t, sw, sh);
        int margin2=32, pw=sw-margin2*2, ph=sh-margin2*2;
        int px2=margin2, py2=margin2;

        int zoneTop    = py2 + 64;
        int zoneBottom = py2 + ph - 70;
        int availH     = zoneBottom - zoneTop;
        
        float avg_round = (global_games_played > 0) ? ((float)global_rounds / global_games_played) : 0.0f;
        int _reward_sum = 0;
        for (int _i = 0; _i < 32; _i++)
            for (int _j = 0; _j < 6; _j++)
                if (ach_claimed[_i][_j]) _reward_sum += ACH_REWARDS[_j];
        float avg_coi   = (global_games_played > 0) ? ((float)(global_totcoi - _reward_sum) / global_games_played) : 0.0f;
        float avg_trs   = (global_games_played > 0) ? ((float)global_tottrs  / global_games_played) : 0.0f;
        float avg_kills = (global_games_played > 0) ? ((float)global_totkills / global_games_played) : 0.0f;

        // Row/header heights
        int dataH = availH / 7;
        if (dataH < 52) dataH = 52;
        int hdrH  = 28;

        // Computed shop totals
        int stat_skins_bought = 0;
        for (int _i = 1; _i < SKIN_COUNT; _i++) if (skin_owned[_i]) stat_skins_bought++;
        int stat_upgrades_bought = spd_level + shd_level + wlk_level + swd_level + bmb_level;
        int stat_perks_bought    = perk_slots + perk_magnet + perk_shield2;

        // Achievement totals — same base set + max_t logic as Perfectionist.
        int stat_ach_completed = 0, stat_ach_maxed = 0;
        {
            int _sk = 0;
            for (int _i = 1; _i < SKIN_COUNT; _i++) if (skin_owned[_i]) _sk++;
            int _up = spd_level + shd_level + wlk_level + swd_level + bmb_level + gld_level;
            int _pk = perk_slots + perk_magnet + perk_shield2;
            // max_t: 6 for 5-star, 4 for 3-star — thresholds = ACHIEVEMENTS UI
            struct { int v; int t[6]; int max_t; } _a[] = {
                { global_games_played,     { 25,  50,  100,   250,   500,  1000  }, 6 },
                { global_rounds,           { 100, 250,  800,  2000,  4000, 10000  }, 6 },
                { global_totcoi,           { 250,1000, 5000, 12000, 40000,100000  }, 6 },
                { global_totkills,         {  75, 160,  350,  1000,  2500,  5000  }, 6 },
                { hs_trs,                  {  10,  20,   25,    30,    40,    50   }, 6 },
                { hs_coi,                  {  20,  50,   75,   100,   150,   250   }, 6 },
                { hs_kills,                {   5,  10,   20,    35,    50,    75   }, 6 },
                { global_tottrs,           { 100, 300,  800,  1500,  4000, 10000  }, 6 },
                { global_kills_sword,      {  25, 100,  300,   750, 999999, 999999 }, 4 },
                { global_kills_shield,     {  15,  60,  200,   500, 999999, 999999 }, 4 },
                { global_portals,          {  15,  50,  150,   400, 999999, 999999 }, 4 },
                { global_ghostwalk_walls,  {  10,  40,  120,   300, 999999, 999999 }, 4 },
                { global_inv_full,         {  10,  35,  100,   250, 999999, 999999 }, 4 },
                { global_use_spd,          {  20,  75,  200,   500, 999999, 999999 }, 4 },
                { global_use_shd,          {  20,  75,  200,   500, 999999, 999999 }, 4 },
                { global_use_bmb,          {  20,  75,  200,   500, 999999, 999999 }, 4 },
                { global_use_wlk,          {  20,  75,  200,   500, 999999, 999999 }, 4 },
                { global_use_swd,          {  20,  75,  200,   500, 999999, 999999 }, 4 },
                { _sk,                     {   1,   5,   15,    25, 999999, 999999 }, 4 },
                { _up,                     {   2,   6,   12,    20, 999999, 999999 }, 4 },
                { _pk,                     {   1,   2,    4,     6, 999999, 999999 }, 4 },
                { global_bosses_killed,    {   6,  18,   48,    90,   180,   300  }, 6 },
                { hs_round,                {   5,  10,   20,    30,    40,    50  }, 6 },
                { hs_coi_round,            {  38,  75,  125,   200, 999999, 999999 }, 4 },
                { global_use_gld,          {   5,  20,   60,   150, 999999, 999999 }, 4 },
                { global_rooms_trap,       {   3,  10,   30,    80, 999999, 999999 }, 4 },
                { global_rooms_dark,       {   3,  10,   30,    80, 999999, 999999 }, 4 },
                { global_rooms_lava,       {   3,  10,   30,    80, 999999, 999999 }, 4 },
                { global_rooms_coinrush,   {   2,   8,   25,    60, 999999, 999999 }, 4 },
                { global_cannon_hits,      {   5,  20,   50,   120, 999999, 999999 }, 4 },
            };
            for (int _i = 0; _i < 30; _i++) {
                int _tier = 0;
                for (int _j = 0; _j < 6; _j++) {
                    if (_a[_i].v >= _a[_i].t[_j]) _tier = _j + 1; else break;
                }
                stat_ach_completed += _tier;
                if (_tier == _a[_i].max_t) stat_ach_maxed++;
            }
        }

        // Section accent colours
        #define GRP_GEN   (Color){170,160,220,255}
        #define GRP_COIN  (Color){220,180,  0,255}
        #define GRP_TRES  (Color){255,160, 50,255}
        #define GRP_KILL  (Color){210, 60, 60,255}
        #define GRP_DETH  (Color){240,100, 40,255}
        #define GRP_EXPL  (Color){ 50,185,210,255}
        #define GRP_ITEM  (Color){ 80,170,210,255}
        #define GRP_AVG   (Color){150,135,200,255}
        #define GRP_SHOP  (Color){ 70,215,140,255}
        #define GRP_ACH   (Color){ 55,205,255,255}
        // value text colours
        #define VC_GEN    (Color){210,200,245,255}
        #define VC_COIN   (Color){255,215,  0,255}
        #define VC_COIN2  (Color){255,200, 40,255}
        #define VC_TRES   CTRES
        #define VC_KILL   (Color){230, 80, 80,255}
        #define VC_SWD    (Color){255,200,  0,255}
        #define VC_SHD    (Color){ 90,160,255,255}
        #define VC_DETH   (Color){255,120, 60,255}
        #define VC_EXPL   (Color){ 90,210,255,255}
        #define VC_MISC   (Color){155,205,155,255}
        #define VC_SPD    (Color){ 80,220, 80,255}
        #define VC_BMB    (Color){255,140, 30,255}
        #define VC_WLK    (Color){200,100,255,255}
        #define VC_AVG    (Color){200,185,255,255}

        // Each entry: label, val, is_avg, is_hdr, bar colour (section), value colour
        #define GRP_BOSS  (Color){170, 40,200,255}
        #define GRP_ROOM  (Color){235,150, 60,255}
        #define STAT_COUNT 58
        struct {
            const char *label; float val; bool is_avg; bool is_hdr;
            Color bar; Color vcol;
        } stats[STAT_COUNT] = {
            // ── GENERAL ──────────────────────────────────────────────────────
            { "GAMES",                0,                           false, true,  GRP_GEN,  GRP_GEN  },
            { "Total Games Played",     (float)global_games_played,  false, false, GRP_GEN,  VC_GEN   },
            { "Total Rounds Played",    (float)global_rounds,        false, false, GRP_GEN,  VC_GEN   },
            { "Best Round Reached",     (float)hs_round,             false, false, GRP_GEN,  VC_GEN   },
            // ── COINS ─────────────────────────────────────────────────────────
            { "COINS",                  0,                           false, true,  GRP_COIN, GRP_COIN },
            { "Coin Balance",           (float)coin_balance,         false, false, GRP_COIN, VC_COIN  },
            { "Total Coins Collected",  (float)global_totcoi,        false, false, GRP_COIN, VC_COIN2 },
            { "Most Coins in One Game", (float)hs_coi,               false, false, GRP_COIN, VC_COIN  },
            { "Most Coins in One Round",(float)hs_coi_round,         false, false, GRP_COIN, VC_COIN  },
            // ── TREASURES ─────────────────────────────────────────────────────
            { "TREASURES",              0,                           false, true,  GRP_TRES, GRP_TRES },
            { "Total Treasures",        (float)global_tottrs,        false, false, GRP_TRES, GRP_TRES  },
            { "Most Treasures/Game",    (float)hs_trs,               false, false, GRP_TRES, GRP_TRES  },
            // ── KILLS ─────────────────────────────────────────────────────────
            { "KILLS",                  0,                           false, true,  GRP_KILL, GRP_KILL },
            { "Total Enemy Kills",      (float)global_totkills,      false, false, GRP_KILL, GRP_KILL  },
            { "Most Kills in One Game",(float)hs_kills,             false, false, GRP_KILL, GRP_KILL  },
            { "Kills by Sword",         (float)global_kills_sword,   false, false, GRP_KILL, GRP_KILL   },
            { "Kills by Shield",        (float)global_kills_shield,  false, false, GRP_KILL, GRP_KILL   },
            // ── BOSSES ────────────────────────────────────────────────────────
            { "BOSSES",                 0,                           false, true,  GRP_BOSS, GRP_BOSS },
            { "Bosses Defeated",        (float)global_bosses_killed, false, false, GRP_BOSS, GRP_BOSS },
            { "Boss Damage Dealt",      (float)global_boss_dmg,      false, false, GRP_BOSS, GRP_BOSS },
            { "Cannon Hits on Bosses",  (float)global_cannon_hits,   false, false, GRP_BOSS, GRP_BOSS },
            // ── CHALLENGE ROOMS ───────────────────────────────────────────────
            { "CHALLENGE ROOMS",        0,                           false, true,  GRP_ROOM, GRP_ROOM },
            { "Trap Rooms Cleared",     (float)global_rooms_trap,    false, false, GRP_ROOM, GRP_ROOM },
            { "Dark Rooms Cleared",     (float)global_rooms_dark,    false, false, GRP_ROOM, GRP_ROOM },
            { "Lava Rooms Cleared",     (float)global_rooms_lava,    false, false, GRP_ROOM, GRP_ROOM },
            { "Coin Rush Rooms Cleared",(float)global_rooms_coinrush,false, false, GRP_ROOM, GRP_ROOM },
            // ── DEATHS ────────────────────────────────────────────────────────
            { "DEATHS",                 0,                           false, true,  GRP_DETH, GRP_DETH },
            { "Total Deaths",           (float)global_games_played,  false, false, GRP_DETH, GRP_DETH  },
            { "Deaths by Enemy",        (float)global_deaths_enemy,  false, false, GRP_DETH, GRP_DETH  },
            { "Deaths by Boss",         (float)global_deaths_boss,   false, false, GRP_DETH, GRP_DETH  },
            { "Deaths by Trap",         (float)global_deaths_trap,   false, false, GRP_DETH, GRP_DETH  },
            { "Deaths by Trapdoor",     (float)global_deaths_trapdoor,false, false, GRP_DETH, GRP_DETH  },
            { "Deaths by Lava",         (float)global_deaths_lava,   false, false, GRP_DETH, GRP_DETH  },
            { "Deaths by Quitting",     (float)(global_games_played - global_deaths_enemy - global_deaths_boss - global_deaths_trap - global_deaths_trapdoor - global_deaths_lava), false, false, GRP_DETH, GRP_DETH  },
            // ── GAMEPLAY ───────────────────────────────────────────────────
            { "GAMEPLAY",            0,                           false, true,  GRP_EXPL, GRP_EXPL },
            { "Portals Entered",        (float)global_portals,        false, false, GRP_EXPL, GRP_EXPL  },
            { "Ghost Walk Passes",      (float)global_ghostwalk_walls,false, false, GRP_EXPL, GRP_EXPL   },
            { "Inventory Full Misses",  (float)global_inv_full,      false, false, GRP_EXPL, GRP_EXPL  },
            // ── ITEMS USED ────────────────────────────────────────────────────
            { "ITEMS USED",             0,                           false, true,  GRP_ITEM, GRP_ITEM },
            { "Speed Items Used",       (float)global_use_spd,       false, false, GRP_ITEM, GRP_ITEM   },
            { "Shield Items Used",      (float)global_use_shd,       false, false, GRP_ITEM, GRP_ITEM   },
            { "Bomb Items Used",        (float)global_use_bmb,       false, false, GRP_ITEM, GRP_ITEM   },
            { "Ghost Walk Items Used",  (float)global_use_wlk,       false, false, GRP_ITEM, GRP_ITEM   },
            { "Sword Items Used",       (float)global_use_swd,       false, false, GRP_ITEM, GRP_ITEM   },
            { "Gold Touch Items Used",  (float)global_use_gld,       false, false, GRP_ITEM, GRP_ITEM   },
            // ── SHOP ──────────────────────────────────────────────────────────
            { "SHOP",                        0,                           false, true,  GRP_SHOP, GRP_SHOP },
            { "Skins Bought",                (float)stat_skins_bought,    false, false, GRP_SHOP, GRP_SHOP },
            { "Upgrades Bought",             (float)stat_upgrades_bought, false, false, GRP_SHOP, GRP_SHOP },
            { "Perks Bought",                (float)stat_perks_bought,    false, false, GRP_SHOP, GRP_SHOP },
            // ── ACHIEVEMENTS ──────────────────────────────────────────────────
            { "ACHIEVEMENTS",                0,                           false, true,  GRP_ACH,  GRP_ACH  },
            { "Achievements Completed",      (float)stat_ach_completed,   false, false, GRP_ACH,  GRP_ACH  },
            { "Achievements Maxed", (float)stat_ach_maxed,       false, false, GRP_ACH,  GRP_ACH  },
            // ── AVERAGES ──────────────────────────────────────────────────────
            { "AVERAGES",               0,                           false, true,  GRP_AVG,  GRP_AVG  },
            { "Avg. Rounds per Game",   avg_round,                   true,  false, GRP_AVG,  VC_AVG   },
            { "Avg. Coins per Game",    avg_coi,                     true,  false, GRP_AVG,  VC_AVG   },
            { "Avg. Treasures per Game",avg_trs,                     true,  false, GRP_AVG,  VC_AVG   },
            { "Avg. Kills per Game",    avg_kills,                   true,  false, GRP_AVG,  VC_AVG   },
        };

        // 12 headers + 46 data rows
        int totalH = 12 * hdrH + 46 * dataH;
        int maxScroll = totalH - availH;
        if (maxScroll < 0) maxScroll = 0;
        float wheel = GetMouseWheelMoveV().y;
        if (wheel != 0.0f) statScroll -= (int)(wheel * 40.0f);
        if (IsKeyDown(KEY_UP))   statScroll -= 7;
        if (IsKeyDown(KEY_DOWN)) statScroll += 7;
        if (statScroll < 0) statScroll = 0;
        if (statScroll > maxScroll) statScroll = maxScroll;

        int cx = px2 + 40;
        int dataIdx = 0; // for alternating row bg

        BeginScissorMode(px2, zoneTop, pw, availH);
        int cy2 = zoneTop - statScroll;
        for (int i = 0; i < STAT_COUNT; i++) {
            bool hdr = stats[i].is_hdr;
            int entH = hdr ? hdrH : dataH;
            int ry2  = cy2;
            cy2 += entH;
            if (ry2 + entH < zoneTop || ry2 > zoneBottom) { if (!hdr) dataIdx++; continue; }

            if (hdr) {
                // Section header bar
                Color hc = stats[i].bar;
                DrawRectangle(cx-8, ry2, pw-56, hdrH,
                              (Color){hc.r,hc.g,hc.b,28});
                DrawRectangle(cx-8, ry2, pw-56, 2,
                              (Color){hc.r,hc.g,hc.b,180});
                DrawRectangle(cx-8, ry2+hdrH-2, pw-56, 1,
                              (Color){hc.r,hc.g,hc.b,60});
                DrawText(stats[i].label, cx+6, ry2+7, 13,
                         (Color){hc.r,hc.g,hc.b,220});
            } else {
                Color rowbg = (dataIdx % 2 == 0) ? (Color){20,16,34,140}
                                                  : (Color){14,11,24,70};
                DrawRectangleRounded(
                    (Rectangle){(float)(cx-8),(float)(ry2-2),(float)(pw-64),(float)(dataH-4)},
                    0.12f, 6, rowbg);
                DrawRectangleRounded(
                    (Rectangle){(float)(cx-8),(float)(ry2-2),4.0f,(float)(dataH-4)},
                    1.0f, 4, stats[i].bar);
                DrawText(stats[i].label, cx+6, ry2+2, 16, (Color){140,135,170,210});
                DrawText(stats[i].is_avg ? TextFormat("%.2f", stats[i].val)
                                         : TextFormat("%d", (int)stats[i].val),
                         cx+6, ry2+22, 24, stats[i].vcol);
                dataIdx++;
            }
        }
        EndScissorMode();

        if (maxScroll > 0) {
            int sbX  = px2 + pw - 10;
            int sbY  = zoneTop;
            int sbH  = availH;
            int sbTH = (int)((float)availH / totalH * sbH);
            if (sbTH < 20) sbTH = 20;
            int sbTY = sbY + (int)((float)statScroll / maxScroll * (sbH - sbTH));
            DrawRectangleRounded((Rectangle){(float)sbX,(float)sbY,4.0f,(float)sbH},
                                 1.0f,4,(Color){ac.r,ac.g,ac.b,30});
            DrawRectangleRounded((Rectangle){(float)sbX,(float)sbTY,4.0f,(float)sbTH},
                                 1.0f,4,(Color){ac.r,ac.g,ac.b,140});
        }
        return;
    }

    // ── SHOP ──────────────────────────────────────────────────────────────────
    if (menuSub == 3) {
        Color ac = (Color){70,215,140,255};
        DrawSubPanel("SHOP", ac, t, sw, sh);
        int margin2=32, pw=sw-margin2*2, ph=sh-margin2*2;
        int px2=margin2, py2=margin2;
        

        // ── coin balance header ───────────────────────────────────────────────
        int balY = py2 + 64;
        DrawCircle(px2+pw-80, balY+10, 9, CCOIN);
        DrawCircle(px2+pw-83, balY+7,  4, (Color){255,245,160,200});
        int balw = MeasureText(TextFormat("%d", coin_balance), 20);
        DrawText(TextFormat("%d", coin_balance), px2+pw-65, balY+1, 20, CCOIN);
        (void)balw;

        // ── tab bar ───────────────────────────────────────────────────────────
        int tabY  = balY + 34;
        int tabW  = (pw - 60) / 3;
        const char *tabLabels[] = { "SKINS", "UPGRADES", "PERKS" };
        Color tabAc[] = {
            (Color){ 90,220,120,255},
            (Color){255,200, 50,255},
            (Color){185, 90,255,255}
        };
        for (int i=0;i<3;i++) {
            int tx = px2+20 + i*(tabW+10);
            bool sel = (shopTab==i);
            float gp = 0.5f+0.5f*sinf(t*1.8f+(float)i*1.1f);
            if (sel) {
                for (int g=4;g>=1;g--)
                    DrawRectangleRounded((Rectangle){(float)(tx-g*2),(float)(tabY-g),(float)(tabW+g*4),(float)(34+g*2)},
                                        0.25f,6,(Color){tabAc[i].r,tabAc[i].g,tabAc[i].b,(uchar)(10*g)});
            }
            DrawRectangleRounded((Rectangle){(float)tx,(float)tabY,(float)tabW,34},
                                 0.25f,6, sel?(Color){20,16,34,220}:(Color){14,11,22,160});
            Color bord = sel ? tabAc[i] : (Color){tabAc[i].r,tabAc[i].g,tabAc[i].b,(uchar)(60+(int)(40*gp))};
            DrawRectangleRoundedLines((Rectangle){(float)tx,(float)tabY,(float)tabW,34},0.25f,6,bord);
            int tw=MeasureText(tabLabels[i],15);
            DrawText(tabLabels[i]+1,tx+tabW/2-tw/2+2,tabY+9+1,15,(Color){0,0,0,80});
            DrawText(tabLabels[i],  tx+tabW/2-tw/2,  tabY+9,  15,
                     sel?(Color){255,255,255,255}:(Color){tabAc[i].r,tabAc[i].g,tabAc[i].b,180});
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mp2=GetMousePosition();
                Rectangle tr={(float)tx,(float)tabY,(float)tabW,34};
                if (CheckCollisionPointRec(mp2,tr)) { shopTab=i; upgScroll=0; }
            }
        }

        int cZoneTop  = tabY + 44;
        int cZoneX    = px2 + 12;
        int cZoneW    = pw - 24;

        // ── TAB 0: SKINS ─────────────────────────────────────────────────────
        if (shopTab == 0) {
            static const char* SKIN_NAMES[SKIN_COUNT] = {
                // Common (0-13)
                "Green","Red","Orange","Yellow","Lime",
                "Cyan","Blue","Violet","Magenta","White",
                "Brown","Black","Pink","Grey",
                // Uncommon (14-21, 34)
                "Forest","Crimson","Amber","Gold","Viridian",
                "Glacial","Sapphire","Void",
                // Rare (22-26)
                "Pyro","Phantom","Cyber","Chroma","Biohazard",
                // Epic (27-29)
                "Alien","Skull","Angel",
                // Achievement-locked Rare (30-31)
                "Chronovore","Cartographer",
                // Achievement-locked Epic (32)
                "Midas",
                // Achievement-locked Rare (33)
                "Blade Legend",
                // Uncommon reclassification (34)
                "Constellation",
                // Rare (35)
                "Ascendant",
                // Legendary (36-37)
                "Poptart Cat","Apex Rex",
                // Epic (38-40)
                "Skullvoid","Bloodguard","Stormshield",
                // Legendary (41)
                "Sunblade",
                // Epic (42)
                "Black Hole"
            };
            static const int SKIN_PRICES[SKIN_COUNT] = {
                // Common (0-13)
                100, 100, 100, 100, 100,
                100, 100, 100, 100, 100,
                100, 100, 100, 100,
                // Uncommon (14-21)
                200, 200, 200, 200, 200,
                200, 200, 200,
                // Rare (22-26)
                600, 600, 750, 750, 600,
                // Epic (27)
                1000,
                // Rare Skull (28)
                600,
                // Epic (29)
                1000,
                // Achievement-locked Rare (30-31)
                0, 0,
                // Achievement-locked Epic (32)
                0,
                // Achievement-locked Rare (33)
                0,
                // Uncommon Constellation (34)
                400,
                // Rare (35)
                750,
                // Legendary (36-37)
                2000, 2000,
                // Epic (38-40)
                1000, 1000, 1000,
                // Achievement-locked Legendary (41)
                0,
                // Epic (42), same price as Angel
                1000,
            };
            // Unlock hint shown on hover for achievement-locked skins (index = id-30).
            static const char* ACH_SKIN_HINT[4] = {
                "Unlocks at Tier 6: Veteran",
                "Unlocks at Tier 6: Explorer",
                "Unlocks at Tier 6: Coin Hoarder & Rich",
                "Unlocks at Tier 6: Slayer",
            };
            static const char* SUNFORGED_UNLOCK_HINT =
                "Unlocks at Tier 6: Perfectionist";

            int cellSz  = 110, cellGap = 16;
            int perRow  = 5;
            int rowOff  = (cZoneW - (perRow * cellSz + (perRow-1) * cellGap)) / 2;

            int catHdrH  = 28; 
            int catGap   = 8;

            int skinRowH     = cellSz + cellGap + 28 + 10;
            int commonRows   = 3;
            int uncommonRows = 2;   // 14-21, 34 (9 skins)
            int rareRows     = 2;   // 22-26, 28, 30-31, 33, 35 (10 skins)
            int epicRows     = 2;   // 27, 29, 32, 38-40, 42 (7 skins)
            int legendaryRows= 1;   // 36-37, 41 (3 skins)
            int totalSkinH   = catHdrH + catGap + commonRows * skinRowH + 20
                             + catHdrH + catGap + uncommonRows * skinRowH + 20
                             + catHdrH + catGap + rareRows * skinRowH + 20
                             + catHdrH + catGap + epicRows * skinRowH + 20
                             + catHdrH + catGap + legendaryRows * skinRowH + 16;

            int skinZoneTop = cZoneTop;
            int skinZoneBot = py2 + ph - 70;
            int availH_skin = skinZoneBot - skinZoneTop;
            static int skinScroll = 0;

            int maxScrollSkin = totalSkinH - availH_skin;
            if (maxScrollSkin < 0) maxScrollSkin = 0;

            float wheelS = GetMouseWheelMoveV().y;
            if (wheelS != 0.0f) { skinScroll -= (int)(wheelS * 40.0f); }
            if (IsKeyDown(KEY_UP))   skinScroll -= 7;
            if (IsKeyDown(KEY_DOWN)) skinScroll += 7;
            if (skinScroll < 0) skinScroll = 0;
            if (skinScroll > maxScrollSkin) skinScroll = maxScrollSkin;

            BeginScissorMode(cZoneX, skinZoneTop, cZoneW, availH_skin);

            int curY = skinZoneTop - skinScroll + 8;

            // ── COMMON skins ─────────────────────────────────────────────────
            {
                float lgp = 0.5f + 0.5f*sinf(t*1.5f);
                Color comAc = ac; 
                int lbY = curY;
                DrawRectangleRounded((Rectangle){(float)(cZoneX+rowOff-8),(float)(lbY-2),(float)(perRow*cellSz+(perRow-1)*cellGap+16),(float)(catHdrH)},
                                     0.3f,6,(Color){comAc.r/8,comAc.g/8,comAc.b/8,200});
                DrawRectangleRoundedLines((Rectangle){(float)(cZoneX+rowOff-8),(float)(lbY-2),(float)(perRow*cellSz+(perRow-1)*cellGap+16),(float)(catHdrH)},
                                          0.3f,6,(Color){comAc.r,comAc.g,comAc.b,(uchar)(60+(int)(40*lgp))});
                const char *comLbl = "COMMON";
                DrawText(comLbl, cZoneX+rowOff, lbY+4, 17, (Color){comAc.r,comAc.g,comAc.b,230});
                curY += catHdrH + catGap;

                for (int i=0; i<14; i++) {
                    int col=i%perRow, row=i/perRow;
                    int cx2 = cZoneX + rowOff + col*(cellSz+cellGap);
                    int cy2 = curY + row*skinRowH;

                    bool owned = skin_owned[i];
                    bool equip = (skin_equipped==i);
                    float gp   = 0.5f+0.5f*sinf(t*2.0f+(float)i*0.7f);
                    Color sc2  = GetSkinColor(i);

                    if (equip) {
                        for (int g=5;g>=1;g--)
                            DrawRectangleRounded(
                                (Rectangle){(float)(cx2-g*3),(float)(cy2-g*3),(float)(cellSz+g*6),(float)(cellSz+g*6)},
                                0.22f,8,(Color){sc2.r,sc2.g,sc2.b,(uchar)(8*g)});
                    }
                    DrawRectangleRounded((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                         0.18f,8,(Color){16,13,26,200});
                    Color bord2 = owned
                        ? (Color){sc2.r,sc2.g,sc2.b,equip?255u:(uchar)(80+(int)(60*gp))}
                        : (Color){55,50,75,120};
                    DrawRectangleRoundedLines((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                             0.18f,8,bord2);

                    int pcx=cx2+cellSz/2, pcy=cy2+cellSz/2-8;
                    DrawCircle(pcx,pcy,18,sc2);
                    DrawCircle(pcx-3,pcy-4,9,(Color){CLAMP255(sc2.r+55),CLAMP255(sc2.g+55),CLAMP255(sc2.b+55),120});
                    DrawCircle(pcx-5,pcy-3,3,(Color){10,10,22,255});
                    DrawCircle(pcx+5,pcy-3,3,(Color){10,10,22,255});

                    int nw2=MeasureText(SKIN_NAMES[i],13);
                    DrawText(SKIN_NAMES[i],cx2+cellSz/2-nw2/2,cy2+cellSz-22,13,
                             owned?(Color){200,195,225,220}:(Color){80,75,110,180});

                    int btnY=cy2+cellSz+6, btnH=26;
                    Rectangle btnR={(float)cx2,(float)btnY,(float)cellSz,(float)btnH};
                    Vector2 mp2=GetMousePosition();
                    bool bhov=CheckCollisionPointRec(mp2,btnR);

                    if (equip) {
                        DrawRectangleRounded(btnR,0.3f,6,(Color){20,50,30,220});
                        DrawRectangleRoundedLines(btnR,0.3f,6,ac);
                        int ew=MeasureText("EQUIPPED",12);
                        DrawText("EQUIPPED",cx2+cellSz/2-ew/2,btnY+7,12,(Color){70,215,140,255});
                    } else if (owned) {
                        Color bf2=(Color){bhov?35:20,bhov?28:14,bhov?52:32,230};
                        DrawRectangleRounded(btnR,0.3f,6,bf2);
                        DrawRectangleRoundedLines(btnR,0.3f,6,(Color){sc2.r,sc2.g,sc2.b,bhov?200u:100u});
                        int ew=MeasureText("EQUIP",12);
                        DrawText("EQUIP",cx2+cellSz/2-ew/2,btnY+7,12,
                                 (Color){CLAMP255(sc2.r+50),CLAMP255(sc2.g+50),CLAMP255(sc2.b+50),220});
                        if (bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { skin_equipped=i; SaveShop(); }
                    } else {
                        int price = SKIN_PRICES[i];
                        bool canBuy=(coin_balance>=price);
                        Color bf2=canBuy?(bhov?(Color){30,50,25,230}:(Color){20,35,18,200}):(Color){20,18,28,160};
                        DrawRectangleRounded(btnR,0.3f,6,bf2);
                        DrawRectangleRoundedLines(btnR,0.3f,6,canBuy?(Color){70,215,140,bhov?220u:100u}:(Color){50,46,68,80});
                        DrawCircle(cx2+cellSz/2-26,btnY+13,5,canBuy?CCOIN:(Color){80,70,40,180});
                        char priceStr[16]; snprintf(priceStr, sizeof(priceStr), "%d", price);
                        int pw2=MeasureText(priceStr,12);
                        DrawText(priceStr,cx2+cellSz/2-16,btnY+7,12,canBuy?(Color){255,215,0,220}:(Color){100,90,60,160});
                        (void)pw2;
                        if (canBuy && bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            coin_balance-=price; skin_owned[i]=1; skin_equipped=i;
                            SaveStats(); SaveShop();
                        }
                    }
                }
                curY += commonRows * skinRowH + 20;
            }

            // ── UNCOMMON skins ───────────────────────────────────────────────────
            {
                float lgp = 0.5f + 0.5f*sinf(t*1.5f + 0.8f);
                Color uncommonAc = (Color){255,180,0,255}; 
                int lbY = curY;
                DrawRectangleRounded((Rectangle){(float)(cZoneX+rowOff-8),(float)(lbY-2),(float)(perRow*cellSz+(perRow-1)*cellGap+16),(float)(catHdrH)},
                                     0.3f,6,(Color){uncommonAc.r/8,uncommonAc.g/8,uncommonAc.b/8,200});
                DrawRectangleRoundedLines((Rectangle){(float)(cZoneX+rowOff-8),(float)(lbY-2),(float)(perRow*cellSz+(perRow-1)*cellGap+16),(float)(catHdrH)},
                                          0.3f,6,(Color){uncommonAc.r,uncommonAc.g,uncommonAc.b,(uchar)(60+(int)(40*lgp))});
                const char *uncommonLbl = "UNCOMMON";
                DrawText(uncommonLbl, cZoneX+rowOff, lbY+4, 17, (Color){uncommonAc.r,uncommonAc.g,uncommonAc.b,230});
                curY += catHdrH + catGap;

                static const int UNCOMMON_IDS[9] = { 14, 15, 16, 17, 18, 19, 20, 21, 34 };
                for (int ri=0; ri<9; ri++) {
                    int i = UNCOMMON_IDS[ri];
                    int col=ri%perRow, row=ri/perRow;
                    int cx2 = cZoneX + rowOff + col*(cellSz+cellGap);
                    int cy2 = curY + row*skinRowH;

                    bool owned = skin_owned[i];
                    bool equip = (skin_equipped==i);
                    float gp   = 0.5f+0.5f*sinf(t*2.0f+(float)i*0.7f);
                    Color sc2  = GetSkinColor(i);
                    Color uncommonAcSkin = (Color){255,180,0,255};

                    // uncommon glow effect
                    if (owned) {
                        for (int g=4;g>=1;g--)
                            DrawRectangleRounded(
                                (Rectangle){(float)(cx2-g*2),(float)(cy2-g*2),(float)(cellSz+g*4),(float)(cellSz+g*4)},
                                0.22f,8,(Color){uncommonAcSkin.r,uncommonAcSkin.g,uncommonAcSkin.b,(uchar)(4*g+(int)(3*g*gp))});
                    }
                    if (equip) {
                        for (int g=5;g>=1;g--)
                            DrawRectangleRounded(
                                (Rectangle){(float)(cx2-g*3),(float)(cy2-g*3),(float)(cellSz+g*6),(float)(cellSz+g*6)},
                                0.22f,8,(Color){sc2.r,sc2.g,sc2.b,(uchar)(8*g)});
                    }
                    DrawRectangleRounded((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                         0.18f,8,(Color){20,14,30,220});
                    Color bord2 = owned
                        ? (Color){sc2.r,sc2.g,sc2.b,equip?255u:(uchar)(100+(int)(80*gp))}
                        : (Color){80,60,40,120};
                    DrawRectangleRoundedLines((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                             0.18f,8,bord2);
                    // Uncommon inner border sparkle
                    if (owned) {
                        DrawRectangleRoundedLines((Rectangle){(float)(cx2+3),(float)(cy2+3),(float)(cellSz-6),(float)(cellSz-6)},
                                                  0.18f,8,(Color){uncommonAcSkin.r,uncommonAcSkin.g,uncommonAcSkin.b,(uchar)(30+(int)(20*gp))});
                    }
                   
                     int pcx=cx2+cellSz/2, pcy=cy2+cellSz/2-8;
                     if (i == 34) {
                         DrawConstellation((float)pcx,(float)pcy,18.0f,t);
                     } else {
                         DrawCircle(pcx,pcy,18,sc2);
                         DrawCircle(pcx-3,pcy-4,9,(Color){CLAMP255(sc2.r+55),CLAMP255(sc2.g+55),CLAMP255(sc2.b+55),120});
                         DrawCircle(pcx-5,pcy-3,3,(Color){10,10,22,255});
                         DrawCircle(pcx+5,pcy-3,3,(Color){10,10,22,255});
                     }

                    int nw2=MeasureText(SKIN_NAMES[i],13);
                    DrawText(SKIN_NAMES[i],cx2+cellSz/2-nw2/2,cy2+cellSz-22,13,
                             owned?(Color){220,210,240,230}:(Color){90,80,110,180});

                    int btnY=cy2+cellSz+6, btnH=26;
                    Rectangle btnR={(float)cx2,(float)btnY,(float)cellSz,(float)btnH};
                    Vector2 mp2=GetMousePosition();
                    bool bhov=CheckCollisionPointRec(mp2,btnR);

                    if (equip) {
                        DrawRectangleRounded(btnR,0.3f,6,(Color){40,30,10,220});
                        DrawRectangleRoundedLines(btnR,0.3f,6,(Color){uncommonAcSkin.r,uncommonAcSkin.g,0,255});
                        int ew=MeasureText("EQUIPPED",12);
                        DrawText("EQUIPPED",cx2+cellSz/2-ew/2,btnY+7,12,(Color){70,215,140,255});
                    } else if (owned) {
                        Color bf2=(Color){bhov?50:30,bhov?40:22,bhov?18:10,230};
                        DrawRectangleRounded(btnR,0.3f,6,bf2);
                        DrawRectangleRoundedLines(btnR,0.3f,6,(Color){sc2.r,sc2.g,sc2.b,bhov?200u:100u});
                        int ew=MeasureText("EQUIP",12);
                        DrawText("EQUIP",cx2+cellSz/2-ew/2,btnY+7,12,
                                 (Color){CLAMP255(sc2.r+50),CLAMP255(sc2.g+50),CLAMP255(sc2.b+50),220});
                        if (bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { skin_equipped=i; SaveShop(); }
                    } else {
                        int price = SKIN_PRICES[i];
                        bool canBuy=(coin_balance>=price);
                        Color bf2=canBuy?(bhov?(Color){50,42,10,230}:(Color){35,28,8,200}):(Color){20,18,28,160};
                        DrawRectangleRounded(btnR,0.3f,6,bf2);
                        DrawRectangleRoundedLines(btnR,0.3f,6,canBuy?(Color){uncommonAcSkin.r,uncommonAcSkin.g,0,bhov?220u:100u}:(Color){60,50,30,80});
                        DrawCircle(cx2+cellSz/2-26,btnY+13,5,canBuy?CCOIN:(Color){80,70,40,180});
                        char priceStr[16]; snprintf(priceStr, sizeof(priceStr), "%d", price);
                        DrawText(priceStr,cx2+cellSz/2-16,btnY+7,12,canBuy?(Color){255,215,0,230}:(Color){100,90,60,160});
                        if (canBuy && bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            coin_balance-=price; skin_owned[i]=1; skin_equipped=i;
                            SaveStats(); SaveShop();
                        }
                    }
                }
                curY += uncommonRows * skinRowH + 20;
            }

            // ── RARE skins ───────────────────────────────────────────────────────
            {
                float lgp = 0.5f + 0.5f*sinf(t*1.5f + 1.6f);
                Color rareAc = (Color){255, 80, 80, 255};
                int lbY = curY;
                DrawRectangleRounded((Rectangle){(float)(cZoneX+rowOff-8),(float)(lbY-2),(float)(perRow*cellSz+(perRow-1)*cellGap+16),(float)(catHdrH)},
                                     0.3f,6,(Color){rareAc.r/8,rareAc.g/8,rareAc.b/8,200});
                DrawRectangleRoundedLines((Rectangle){(float)(cZoneX+rowOff-8),(float)(lbY-2),(float)(perRow*cellSz+(perRow-1)*cellGap+16),(float)(catHdrH)},
                                          0.3f,6,(Color){rareAc.r,rareAc.g,rareAc.b,(uchar)(60+(int)(40*lgp))});
                const char *rareLbl = "RARE";
                DrawText(rareLbl, cZoneX+rowOff, lbY+4, 17, (Color){rareAc.r,rareAc.g,rareAc.b,230});
                curY += catHdrH + catGap;

                static const int RARE_IDS[10] = { 22, 23, 24, 25, 26, 28, 30, 31, 33, 35 };
                for (int ri=0; ri<10; ri++) {
                    int i = RARE_IDS[ri];
                    bool isAchSkin = (i == 30 || i == 31 || i == 33);
                    int col=ri%perRow, row=ri/perRow;
                    int cx2 = cZoneX + rowOff + col*(cellSz+cellGap);
                    int cy2 = curY + row*skinRowH;

                    bool owned = skin_owned[i];
                    bool equip = (skin_equipped==i);
                    float gp   = 0.5f+0.5f*sinf(t*2.0f+(float)i*0.7f);
                    Color sc2  = GetSkinColor(i);

                    // rare glow effect (stronger than uncommon, slightly toned down)
                    if (owned) {
                        for (int g=5;g>=1;g--)
                            DrawRectangleRounded(
                                (Rectangle){(float)(cx2-g*3),(float)(cy2-g*3),(float)(cellSz+g*6),(float)(cellSz+g*6)},
                                0.22f,8,(Color){rareAc.r,rareAc.g,rareAc.b,(uchar)(4*g+(int)(3*g*gp))});
                    }
                    if (equip) {
                        for (int g=6;g>=1;g--)
                            DrawRectangleRounded(
                                (Rectangle){(float)(cx2-g*3),(float)(cy2-g*3),(float)(cellSz+g*6),(float)(cellSz+g*6)},
                                0.22f,8,(Color){sc2.r,sc2.g,sc2.b,(uchar)(10*g)});
                    }
                    DrawRectangleRounded((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                         0.18f,8,(Color){28,10,10,230});
                    // Border: animated color preview even when unowned
                    Color bord2 = owned
                        ? (Color){sc2.r,sc2.g,sc2.b,equip?255u:(uchar)(120+(int)(100*gp))}
                        : (Color){sc2.r,sc2.g,sc2.b,55};
                    DrawRectangleRoundedLines((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                             0.18f,8,bord2);
                    // Inner double border (owned only)
                    if (owned) {
                        DrawRectangleRoundedLines((Rectangle){(float)(cx2+3),(float)(cy2+3),(float)(cellSz-6),(float)(cellSz-6)},
                                                  0.18f,8,(Color){rareAc.r,rareAc.g,rareAc.b,(uchar)(50+(int)(35*gp))});
                        DrawRectangleRoundedLines((Rectangle){(float)(cx2+6),(float)(cy2+6),(float)(cellSz-12),(float)(cellSz-12)},
                                                  0.18f,8,(Color){rareAc.r,rareAc.g,rareAc.b,(uchar)(20+(int)(15*gp))});
                    }

                    int pcx=cx2+cellSz/2, pcy=cy2+cellSz/2-8;
                    if (i == 28) {
                        DrawRareBack(i, (float)pcx, (float)pcy, 18.0f, t);
                        DrawSkull((float)pcx, (float)pcy, 18.0f, t);
                    } else if (i == 33) {
                        DrawApexBladelord((float)pcx, (float)pcy, 18.0f, t);
                    } else {
                        DrawRareBack(i, (float)pcx, (float)pcy, 18.0f, t);
                        DrawCircle(pcx,pcy,18,sc2);
                        DrawCircle(pcx-3,pcy-4,9,(Color){CLAMP255(sc2.r+55),CLAMP255(sc2.g+55),CLAMP255(sc2.b+55),90});
                        DrawRareFace(i, (float)pcx, (float)pcy, 18.0f, t);
                    }

                    int nw2=MeasureText(SKIN_NAMES[i],13);
                    DrawText(SKIN_NAMES[i],cx2+cellSz/2-nw2/2,cy2+cellSz-22,13,
                             owned?(Color){235,200,200,240}:(Color){155,140,175,190});

                    int btnY=cy2+cellSz+6, btnH=26;
                    Rectangle btnR={(float)cx2,(float)btnY,(float)cellSz,(float)btnH};
                    Vector2 mp2=GetMousePosition();
                    bool bhov=CheckCollisionPointRec(mp2,btnR);

                    if (equip) {
                        DrawRectangleRounded(btnR,0.3f,6,(Color){50,10,10,220});
                        DrawRectangleRoundedLines(btnR,0.3f,6,rareAc);
                        int ew=MeasureText("EQUIPPED",12);
                        DrawText("EQUIPPED",cx2+cellSz/2-ew/2,btnY+7,12,(Color){70,215,140,255});
                    } else if (owned) {
                        Color bf2=(Color){bhov?60:35,bhov?15:10,bhov?15:10,230};
                        DrawRectangleRounded(btnR,0.3f,6,bf2);
                        DrawRectangleRoundedLines(btnR,0.3f,6,(Color){sc2.r,sc2.g,sc2.b,bhov?220u:120u});
                        int ew=MeasureText("EQUIP",12);
                        DrawText("EQUIP",cx2+cellSz/2-ew/2,btnY+7,12,
                                 (Color){CLAMP255(sc2.r+50),CLAMP255(sc2.g+50),CLAMP255(sc2.b+50),220});
                        if (bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { skin_equipped=i; SaveShop(); }
                    } else if (isAchSkin) {
                        DrawRectangleRounded(btnR,0.3f,6,(Color){20,18,28,180});
                        DrawRectangleRoundedLines(btnR,0.3f,6,(Color){110,100,90,120});
                        int lw=MeasureText("LOCKED",12);
                        DrawText("LOCKED",cx2+cellSz/2-lw/2,btnY+7,12,(Color){150,140,120,200});
                        if (bhov) {
                            const char *hint = ACH_SKIN_HINT[i-30];
                            int hw = MeasureText(hint,12);
                            DrawRectangleRounded((Rectangle){mp2.x+12,mp2.y+12,(float)(hw+16),24},0.3f,6,(Color){10,8,18,230});
                            DrawText(hint,(int)mp2.x+20,(int)mp2.y+18,12,(Color){255,225,120,230});
                        }
                    } else {
                        int price = SKIN_PRICES[i];
                        bool canBuy=(coin_balance>=price);
                        Color bf2=canBuy?(bhov?(Color){60,18,10,230}:(Color){40,12,8,200}):(Color){20,18,28,160};
                        DrawRectangleRounded(btnR,0.3f,6,bf2);
                        DrawRectangleRoundedLines(btnR,0.3f,6,canBuy?(Color){rareAc.r,rareAc.g,rareAc.b,bhov?220u:100u}:(Color){60,40,40,80});
                        DrawCircle(cx2+cellSz/2-26,btnY+13,5,canBuy?CCOIN:(Color){80,70,40,180});
                        char priceStr[16]; snprintf(priceStr, sizeof(priceStr), "%d", price);
                        DrawText(priceStr,cx2+cellSz/2-16,btnY+7,12,canBuy?(Color){255,215,0,230}:(Color){100,90,60,160});
                        if (canBuy && bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            coin_balance-=price; skin_owned[i]=1; skin_equipped=i;
                            SaveStats(); SaveShop();
                        }
                    }
                }
                curY += rareRows * skinRowH + 20;
            }

            // ── EPIC skins ───────────────────────────────────────────────────────
            {
                float lgp = 0.5f + 0.5f*sinf(t*1.5f + 2.4f);
                Color epicAc = (Color){180, 100, 255, 255};
                int lbY = curY;
                DrawRectangleRounded((Rectangle){(float)(cZoneX+rowOff-8),(float)(lbY-2),(float)(perRow*cellSz+(perRow-1)*cellGap+16),(float)(catHdrH)},
                                     0.3f,6,(Color){epicAc.r/8,epicAc.g/8,epicAc.b/8,200});
                DrawRectangleRoundedLines((Rectangle){(float)(cZoneX+rowOff-8),(float)(lbY-2),(float)(perRow*cellSz+(perRow-1)*cellGap+16),(float)(catHdrH)},
                                          0.3f,6,(Color){epicAc.r,epicAc.g,epicAc.b,(uchar)(60+(int)(40*lgp))});
                const char *epicLbl = "EPIC";
                DrawText(epicLbl, cZoneX+rowOff, lbY+4, 17, (Color){epicAc.r,epicAc.g,epicAc.b,230});
                curY += catHdrH + catGap;

                static const int EPIC_IDS[7] = { 27, 29, 32, 38, 39, 40, 42 };
                for (int ri=0; ri<7; ri++) {
                    int i = EPIC_IDS[ri];
                    bool isAchSkin = (i == 32);
                    int col=ri%perRow, row=ri/perRow;
                    int cx2 = cZoneX + rowOff + col*(cellSz+cellGap);
                    int cy2 = curY + row*skinRowH;

                    bool owned = skin_owned[i];
                    bool equip = (skin_equipped==i);
                    float gp   = 0.5f+0.5f*sinf(t*2.0f+(float)i*0.7f);
                    Color sc2  = GetSkinColor(i);

                    // epic glow effect (stronger than rare, slightly toned down)
                    if (owned) {
                        for (int g=6;g>=1;g--)
                            DrawRectangleRounded(
                                (Rectangle){(float)(cx2-g*3),(float)(cy2-g*3),(float)(cellSz+g*6),(float)(cellSz+g*6)},
                                0.22f,8,(Color){epicAc.r,epicAc.g,epicAc.b,(uchar)(4*g+(int)(3*g*gp))});
                    }
                    if (equip) {
                        for (int g=7;g>=1;g--)
                            DrawRectangleRounded(
                                (Rectangle){(float)(cx2-g*3),(float)(cy2-g*3),(float)(cellSz+g*6),(float)(cellSz+g*6)},
                                0.22f,8,(Color){sc2.r,sc2.g,sc2.b,(uchar)(9*g)});
                    }
                    DrawRectangleRounded((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                         0.18f,8,(Color){20,10,30,235});
                    Color bord2 = owned
                        ? (Color){sc2.r,sc2.g,sc2.b,equip?255u:(uchar)(120+(int)(100*gp))}
                        : (Color){epicAc.r,epicAc.g,epicAc.b,60};
                    DrawRectangleRoundedLines((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                             0.18f,8,bord2);
                    if (owned) {
                        DrawRectangleRoundedLines((Rectangle){(float)(cx2+3),(float)(cy2+3),(float)(cellSz-6),(float)(cellSz-6)},
                                                  0.18f,8,(Color){epicAc.r,epicAc.g,epicAc.b,(uchar)(55+(int)(40*gp))});
                        DrawRectangleRoundedLines((Rectangle){(float)(cx2+6),(float)(cy2+6),(float)(cellSz-12),(float)(cellSz-12)},
                                                  0.18f,8,(Color){epicAc.r,epicAc.g,epicAc.b,(uchar)(25+(int)(18*gp))});
                    }

                    int pcx=cx2+cellSz/2, pcy=cy2+cellSz/2-8;
                    if (i == 39) {
                        DrawBloodscaleVanguard((float)pcx, (float)pcy, 18.0f * 1.10f, t);
                    } else if (i == 40) {
                        DrawStormforgedAegis((float)pcx, (float)pcy, 18.0f * 1.10f, t);
                    } else if (i == 38) {
                        DrawSkullvoidCrowned((float)pcx, (float)pcy, 18.0f, t);
                    } else if (i == 42) {
                        DrawBlackHole((float)pcx, (float)pcy, 18.0f, t);
                    } else {
                        DrawEpicBack(i, (float)pcx, (float)pcy, 18.0f, t);
                        DrawCircle(pcx,pcy,18,sc2);
                        DrawEpicFace(i, (float)pcx, (float)pcy, 18.0f, t);
                    }

                    int nw2=MeasureText(SKIN_NAMES[i],13);
                    DrawText(SKIN_NAMES[i],cx2+cellSz/2-nw2/2,cy2+cellSz-22,13,
                             owned?(Color){225,205,245,240}:(Color){160,145,185,190});

                    int btnY=cy2+cellSz+6, btnH=26;
                    Rectangle btnR={(float)cx2,(float)btnY,(float)cellSz,(float)btnH};
                    Vector2 mp2=GetMousePosition();
                    bool bhov=CheckCollisionPointRec(mp2,btnR);

                    if (equip) {
                        DrawRectangleRounded(btnR,0.3f,6,(Color){35,12,55,220});
                        DrawRectangleRoundedLines(btnR,0.3f,6,epicAc);
                        int ew=MeasureText("EQUIPPED",12);
                        DrawText("EQUIPPED",cx2+cellSz/2-ew/2,btnY+7,12,(Color){70,215,140,255});
                    } else if (owned) {
                        Color bf2=(Color){bhov?45:28,bhov?20:12,bhov?68:42,230};
                        DrawRectangleRounded(btnR,0.3f,6,bf2);
                        DrawRectangleRoundedLines(btnR,0.3f,6,(Color){sc2.r,sc2.g,sc2.b,bhov?220u:120u});
                        int ew=MeasureText("EQUIP",12);
                        DrawText("EQUIP",cx2+cellSz/2-ew/2,btnY+7,12,
                                 (Color){CLAMP255(sc2.r+50),CLAMP255(sc2.g+50),CLAMP255(sc2.b+50),220});
                        if (bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { skin_equipped=i; SaveShop(); }
                    } else if (isAchSkin) {
                        DrawRectangleRounded(btnR,0.3f,6,(Color){20,18,28,180});
                        DrawRectangleRoundedLines(btnR,0.3f,6,(Color){110,100,90,120});
                        int lw=MeasureText("LOCKED",12);
                        DrawText("LOCKED",cx2+cellSz/2-lw/2,btnY+7,12,(Color){150,140,120,200});
                        if (bhov) {
                            const char *hint = ACH_SKIN_HINT[i-30];
                            int hw = MeasureText(hint,12);
                            DrawRectangleRounded((Rectangle){mp2.x+12,mp2.y+12,(float)(hw+16),24},0.3f,6,(Color){10,8,18,230});
                            DrawText(hint,(int)mp2.x+20,(int)mp2.y+18,12,(Color){255,225,120,230});
                        }
                    } else {
                        int price = SKIN_PRICES[i];
                        bool canBuy=(coin_balance>=price);
                        Color bf2=canBuy?(bhov?(Color){45,18,68,230}:(Color){32,12,48,200}):(Color){20,18,28,160};
                        DrawRectangleRounded(btnR,0.3f,6,bf2);
                        DrawRectangleRoundedLines(btnR,0.3f,6,canBuy?(Color){epicAc.r,epicAc.g,epicAc.b,bhov?220u:100u}:(Color){55,45,70,80});
                        DrawCircle(cx2+cellSz/2-26,btnY+13,5,canBuy?CCOIN:(Color){80,70,40,180});
                        char priceStr[16]; snprintf(priceStr, sizeof(priceStr), "%d", price);
                        DrawText(priceStr,cx2+cellSz/2-16,btnY+7,12,canBuy?(Color){255,215,0,230}:(Color){100,90,60,160});
                        if (canBuy && bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            coin_balance-=price; skin_owned[i]=1; skin_equipped=i;
                            SaveStats(); SaveShop();
                        }
                    }
                }
                curY += epicRows * skinRowH + 20;
            }

            // ── LEGENDARY skins ────────────────────────────────────────────────────
            {
                float lgp = 0.5f + 0.5f*sinf(t*1.5f + 3.2f);
                Color legendAc = (Color){255, 220, 90, 255};
                int lbY = curY;
                DrawRectangleRounded((Rectangle){(float)(cZoneX+rowOff-8),(float)(lbY-2),(float)(perRow*cellSz+(perRow-1)*cellGap+16),(float)(catHdrH)},
                                     0.3f,6,(Color){legendAc.r/8,legendAc.g/8,legendAc.b/8,200});
                DrawRectangleRoundedLines((Rectangle){(float)(cZoneX+rowOff-8),(float)(lbY-2),(float)(perRow*cellSz+(perRow-1)*cellGap+16),(float)(catHdrH)},
                                          0.3f,6,(Color){legendAc.r,legendAc.g,legendAc.b,(uchar)(60+(int)(40*lgp))});
                const char *legendLbl = "LEGENDARY";
                DrawText(legendLbl, cZoneX+rowOff, lbY+4, 17, (Color){legendAc.r,legendAc.g,legendAc.b,230});
                curY += catHdrH + catGap;

                static const int LEGENDARY_IDS[3] = { 36, 37, 41 };
                for (int ri=0; ri<3; ri++) {
                    int i = LEGENDARY_IDS[ri];
                    int col=ri%perRow, row=ri/perRow;
                    int cx2 = cZoneX + rowOff + col*(cellSz+cellGap);
                    int cy2 = curY + row*skinRowH;

                    bool owned = skin_owned[i];
                    bool equip = (skin_equipped==i);
                    float gp   = 0.5f+0.5f*sinf(t*2.0f+(float)i*0.7f);
                    Color sc2  = GetSkinColor(i);
                    bool isAchSkin = (i == 41);

                    // legendary glow — the strongest of all rarities
                    if (owned) {
                        for (int g=8;g>=1;g--)
                            DrawRectangleRounded(
                                (Rectangle){(float)(cx2-g*3),(float)(cy2-g*3),(float)(cellSz+g*6),(float)(cellSz+g*6)},
                                0.22f,8,(Color){legendAc.r,legendAc.g,legendAc.b,(uchar)(4*g+(int)(3*g*gp))});
                    }
                    if (equip) {
                        for (int g=9;g>=1;g--)
                            DrawRectangleRounded(
                                (Rectangle){(float)(cx2-g*3),(float)(cy2-g*3),(float)(cellSz+g*6),(float)(cellSz+g*6)},
                                0.22f,8,(Color){sc2.r,sc2.g,sc2.b,(uchar)(10*g)});
                    }
                    DrawRectangleRounded((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                         0.18f,8,(Color){24,18,8,235});
                    Color bord2 = owned
                        ? (Color){sc2.r,sc2.g,sc2.b,equip?255u:(uchar)(140+(int)(100*gp))}
                        : (Color){legendAc.r,legendAc.g,legendAc.b,70};
                    DrawRectangleRoundedLines((Rectangle){(float)cx2,(float)cy2,(float)cellSz,(float)cellSz},
                                             0.18f,8,bord2);
                    if (owned) {
                        DrawRectangleRoundedLines((Rectangle){(float)(cx2+3),(float)(cy2+3),(float)(cellSz-6),(float)(cellSz-6)},
                                                  0.18f,8,(Color){legendAc.r,legendAc.g,legendAc.b,(uchar)(60+(int)(45*gp))});
                        DrawRectangleRoundedLines((Rectangle){(float)(cx2+6),(float)(cy2+6),(float)(cellSz-12),(float)(cellSz-12)},
                                                  0.18f,8,(Color){legendAc.r,legendAc.g,legendAc.b,(uchar)(30+(int)(20*gp))});
                    }

                    int pcx=cx2+cellSz/2, pcy=cy2+cellSz/2-8;
                    if (i == 41) {
                        DrawSunforgedCrusader((float)pcx, (float)pcy, 18.0f * 1.10f, t);
                    } else {
                        DrawLegendaryBack(i, (float)pcx, (float)pcy, 18.0f, t);
                        if (i == 36) {
                            DrawPoptartCat((float)pcx, (float)pcy, 18.0f, t);
                        } else if (i == 37) {
                            DrawApexRex((float)pcx, (float)pcy, 18.0f, t);
                        } else {
                            DrawCircle(pcx,pcy,18,sc2);
                            DrawCircle(pcx-3,pcy-4,9,(Color){CLAMP255(sc2.r+55),CLAMP255(sc2.g+55),CLAMP255(sc2.b+55),90});
                            DrawLegendaryFace(i, (float)pcx, (float)pcy, 18.0f, t);
                        }
                    }

                    int nw2=MeasureText(SKIN_NAMES[i],13);
                    DrawText(SKIN_NAMES[i],cx2+cellSz/2-nw2/2,cy2+cellSz-22,13,
                             owned?(Color){255,235,190,240}:(Color){175,160,120,190});

                    int btnY=cy2+cellSz+6, btnH=26;
                    Rectangle btnR={(float)cx2,(float)btnY,(float)cellSz,(float)btnH};
                    Vector2 mp2=GetMousePosition();
                    bool bhov=CheckCollisionPointRec(mp2,btnR);

                    if (equip) {
                        DrawRectangleRounded(btnR,0.3f,6,(Color){55,42,10,220});
                        DrawRectangleRoundedLines(btnR,0.3f,6,legendAc);
                        int ew=MeasureText("EQUIPPED",12);
                        DrawText("EQUIPPED",cx2+cellSz/2-ew/2,btnY+7,12,(Color){70,215,140,255});
                    } else if (owned) {
                        Color bf2=(Color){bhov?68:42,bhov?52:32,bhov?15:10,230};
                        DrawRectangleRounded(btnR,0.3f,6,bf2);
                        DrawRectangleRoundedLines(btnR,0.3f,6,(Color){sc2.r,sc2.g,sc2.b,bhov?220u:120u});
                        int ew=MeasureText("EQUIP",12);
                        DrawText("EQUIP",cx2+cellSz/2-ew/2,btnY+7,12,
                                 (Color){CLAMP255(sc2.r+50),CLAMP255(sc2.g+50),CLAMP255(sc2.b+50),220});
                        if (bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { skin_equipped=i; SaveShop(); }
                    } else if (isAchSkin) {
                        DrawRectangleRounded(btnR,0.3f,6,(Color){35,30,12,180});
                        DrawRectangleRoundedLines(btnR,0.3f,6,(Color){150,130,70,120});
                        int lw=MeasureText("LOCKED",12);
                        DrawText("LOCKED",cx2+cellSz/2-lw/2,btnY+7,12,(Color){170,155,115,200});
                        if (bhov) {
                            int hw = MeasureText(SUNFORGED_UNLOCK_HINT,12);
                            DrawRectangleRounded((Rectangle){mp2.x+12,mp2.y+12,(float)(hw+16),24},0.3f,6,(Color){10,8,18,230});
                            DrawText(SUNFORGED_UNLOCK_HINT,(int)mp2.x+20,(int)mp2.y+18,12,(Color){255,225,120,230});
                        }
                    } else {
                        int price = SKIN_PRICES[i];
                        bool canBuy=(coin_balance>=price);
                        Color bf2=canBuy?(bhov?(Color){75,58,16,230}:(Color){48,34,8,200}):(Color){20,18,28,160};
                        DrawRectangleRounded(btnR,0.3f,6,bf2);
                        DrawRectangleRoundedLines(btnR,0.3f,6,canBuy?(Color){legendAc.r,legendAc.g,legendAc.b,bhov?220u:100u}:(Color){70,60,35,80});
                        DrawCircle(cx2+cellSz/2-26,btnY+13,5,canBuy?CCOIN:(Color){80,70,40,180});
                        char priceStr[16]; snprintf(priceStr, sizeof(priceStr), "%d", price);
                        DrawText(priceStr,cx2+cellSz/2-16,btnY+7,12,canBuy?(Color){255,215,0,230}:(Color){100,90,60,160});
                        if (canBuy && bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            coin_balance-=price; skin_owned[i]=1; skin_equipped=i;
                            SaveStats(); SaveShop();
                        }
                    }
                }
            }

            EndScissorMode();

            if (maxScrollSkin > 0) {
                Color aca = (Color){255,205,55,255};
                int sbX  = cZoneX + cZoneW - 10;
                int sbY  = skinZoneTop;
                int sbH  = availH_skin;
                int sbTH = (int)((float)availH_skin / totalSkinH * sbH);
                if (sbTH < 20) sbTH = 20;
                int sbTY = sbY + (int)((float)skinScroll / maxScrollSkin * (sbH - sbTH));
                DrawRectangleRounded((Rectangle){(float)sbX,(float)sbY,4.0f,(float)sbH},
                                     1.0f,4,(Color){aca.r,aca.g,aca.b,30});
                DrawRectangleRounded((Rectangle){(float)sbX,(float)sbTY,4.0f,(float)sbTH},
                                     1.0f,4,(Color){aca.r,aca.g,aca.b,140});
            }
        }

        // ── TAB 1: UPGRADES ──────────────────────────────────────────────────
    if (shopTab == 1) {
        static const char* s_spd[] = { "3s (base)","4s","5s","6s","7s","8s" };
        static const char* s_shd[] = { "5s (base)","10s","20s","30s","45s","Unlimited" };
        static const char* s_wlk[] = { "0.5s (base)","1s","1.5s","2s","2.5s","3s" };
        static const char* s_swd[] = { "Range 1 (base)","Range 2","Range 3","Range 4" };
        static const char* s_bmb[] = { "Range 1 (base)","Range 2","Range 3","Range 4","Range 5","Range 6" };
        static const char* s_gld[] = { "3s (base)","4s","5s","6s","7s","8s" };

        struct {
            const char *name, *desc;
            int maxlvl;
            int *lvl;
            const int *cost;
            Color col;
            const char **descs;
        } upgrades[] = {
            { "SPEED BOOST",  "Extends SPD item duration",        5, &spd_level, SPD_COST, IK_COL[IK_SPD], s_spd },
            { "SHIELD",       "Extends SHD item duration",        5, &shd_level, SHD_COST, IK_COL[IK_SHD], s_shd },
            { "GHOSTWALK",    "Extends WLK item duration",        5, &wlk_level, WLK_COST, IK_COL[IK_WLK], s_wlk },
            { "SWORD REACH",  "Extends SWD kill radius",          3, &swd_level, SWD_COST, IK_COL[IK_SWD], s_swd },
            { "BOMB RADIUS",  "Increases BMB explosion radius",   5, &bmb_level, BMB_COST, IK_COL[IK_BMB], s_bmb },
            { "GOLD TOUCH",   "Coin Rush: midas form duration",   5, &gld_level, GLD_COST, IK_COL[IK_GLD], s_gld },
        };
        
        int nUpg   = 6;
        int cardH  = 100;
        int cardGap = 8;

        // ── Scrollbar logic
        int upgZoneTop = cZoneTop;
        int upgZoneBot = py2 + ph - 70; 
        int availH_upg = upgZoneBot - upgZoneTop;
        
        int topScrollOffset = 8; 
        int totalH_upg = topScrollOffset + (nUpg * (cardH + cardGap)); 

        int maxScrollUpg = totalH_upg - availH_upg;
        if (maxScrollUpg < 0) maxScrollUpg = 0;

        float wheel = GetMouseWheelMoveV().y;
        if (wheel != 0.0f) { upgScroll -= (int)(wheel * 40.0f); }
        if (IsKeyDown(KEY_UP))   upgScroll -= 7;
        if (IsKeyDown(KEY_DOWN)) upgScroll += 7;
        if (upgScroll < 0) upgScroll = 0;
        if (upgScroll > maxScrollUpg) upgScroll = maxScrollUpg;

        BeginScissorMode(cZoneX, upgZoneTop, cZoneW, availH_upg);
        
        for (int i = 0; i < nUpg; i++) {
            int uy = (upgZoneTop + topScrollOffset) + i * (cardH + cardGap) - upgScroll;
            
            int ux = cZoneX + 20;
            int uw = cZoneW - 40;
            int uh = cardH;
            
            if (uy + uh < upgZoneTop || uy > upgZoneBot) continue;
            
            float gp = 0.5f + 0.5f * sinf(t * 1.6f + (float)i * 1.2f);

            DrawRectangleRounded((Rectangle){(float)ux,(float)uy,(float)uw,(float)uh},
                                 0.12f,8,(Color){16,13,26,200});
            DrawRectangleRoundedLines((Rectangle){(float)ux,(float)uy,(float)uw,(float)uh},
                                         0.12f,8,(Color){upgrades[i].col.r,upgrades[i].col.g,upgrades[i].col.b,
                                                         (uchar)(60+(int)(50*gp))});
            DrawRectangleRounded((Rectangle){(float)ux,(float)uy,4,(float)uh},
                                 1.0f,4,upgrades[i].col);

            int dix = ux + 22, diy = uy + uh / 2;
            DrawTriangle((Vector2){(float)dix,(float)(diy-12)},
                         (Vector2){(float)(dix+11),(float)diy},
                         (Vector2){(float)dix,(float)(diy+12)}, upgrades[i].col);
            DrawTriangle((Vector2){(float)dix,(float)(diy-12)},
                         (Vector2){(float)dix,(float)(diy+12)},
                         (Vector2){(float)(dix-11),(float)diy}, upgrades[i].col);

            DrawText(upgrades[i].name, ux+46,uy+10,16,upgrades[i].col);
            DrawText(upgrades[i].desc, ux+46,uy+30,12,(Color){140,135,170,210});

            DrawText(TextFormat("Lv %d/%d  %s",
                                *upgrades[i].lvl, upgrades[i].maxlvl,
                                upgrades[i].descs[*upgrades[i].lvl]),
                     ux+46,uy+48,12,(Color){170,165,200,220});

            int visMax = upgrades[i].maxlvl;
            for (int l = 0; l < visMax; l++) {
                int px3 = ux + 46 + l * 20, py3 = uy + 72;
                bool filled = (l < *upgrades[i].lvl);
                DrawCircle(px3,py3,6,filled ? upgrades[i].col : (Color){30,26,44,200});
                DrawCircleLines(px3,py3,6,upgrades[i].col);
                if (filled) DrawCircle(px3,py3,3,(Color){255,255,255,180});
            }

            bool maxed = (*upgrades[i].lvl >= upgrades[i].maxlvl);
            int btnX2 = ux + uw - 175, btnY2 = uy + uh / 2 - 18, btnW2 = 155, btnH2 = 36;
            Rectangle btnR2 = {(float)btnX2,(float)btnY2,(float)btnW2,(float)btnH2};
            Vector2 mp2 = GetMousePosition();
            bool bhov = CheckCollisionPointRec(mp2,btnR2);
            bool canBuy = (coin_balance >= upgrades[i].cost[*upgrades[i].lvl] && !maxed);

            Color bf = maxed ? (Color){20,18,30,160}
                             : canBuy ? (bhov ? (Color){35,60,28,240} : (Color){20,40,18,200})
                                      : (Color){20,18,30,160};
            DrawRectangleRounded(btnR2,0.25f,8,bf);
            DrawRectangleRoundedLines(btnR2,0.25f,8,
                maxed ? (Color){60,56,80,100}
                      : canBuy ? (Color){70,215,140,bhov ? 220u : 120u}
                               : (Color){60,56,80,100});

            if (maxed) {
                int mw3 = MeasureText("MAX",13);
                DrawText("MAX",btnX2+btnW2/2-mw3/2,btnY2+11,13,(Color){120,115,150,180});
            } else {
                DrawCircle(btnX2+24,btnY2+18,7,canBuy ? CCOIN : (Color){70,60,30,160});
                DrawText(TextFormat("%d",upgrades[i].cost[*upgrades[i].lvl]),btnX2+34,btnY2+11,13,
                         canBuy ? (Color){255,215,0,230} : (Color){100,90,55,160});
                int uw3 = MeasureText("UPGRADE",13);
                DrawText("UPGRADE",btnX2+btnW2/2-uw3/2+16,btnY2+11,13,
                         canBuy ? (Color){200,255,200,230} : (Color){90,85,115,160});
                if (canBuy && bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    int ucost = upgrades[i].cost[*upgrades[i].lvl];
                    coin_balance -= ucost;
                    (*upgrades[i].lvl)++;
                    SaveStats(); SaveShop();
                }
            }
        }
        EndScissorMode();

        // ── Scrollbar render 
        if (maxScrollUpg > 0) {
            Color upgAc = (Color){255,200,50,255};
            int sbX  = cZoneX + cZoneW - 10;
            int sbY  = upgZoneTop;
            int sbH  = availH_upg;
            int sbTH = (int)((float)availH_upg / totalH_upg * sbH);
            if (sbTH < 20) sbTH = 20;
            int sbTY = sbY + (int)((float)upgScroll / maxScrollUpg * (sbH - sbTH));
            
            DrawRectangleRounded((Rectangle){(float)sbX,(float)sbY,4.0f,(float)sbH},
                                 1.0f,4,(Color){upgAc.r,upgAc.g,upgAc.b,30});
            DrawRectangleRounded((Rectangle){(float)sbX,(float)sbTY,4.0f,(float)sbTH},
                                 1.0f,4,(Color){upgAc.r,upgAc.g,upgAc.b,140});
        }
    }

        // ── TAB 2: PERKS ─────────────────────────────────────────────────────
        if (shopTab == 2) {
            struct { const char*name; const char*desc; int*flag; int maxv; int cost; Color col; } perks[] = {
                { "EXTRA SLOT I",   "Carry one more item in your inventory",         &perk_slots,   4,  500, (Color){160,150,220,255} },
                { "EXTRA SLOT II",  "Carry two more items (requires Slot I)",        &perk_slots,   4, 1000, (Color){130,120,200,255} },
                { "EXTRA SLOT III", "Carry three more items (requires Slot II)",     &perk_slots,   4, 2000, (Color){110,100,185,255} },
                { "EXTRA SLOT IV",  "Carry four more items (requires Slot III)",     &perk_slots,   4, 3000, (Color){ 90, 80,170,255} },
                { "COIN MAGNET",    "Automatically collect adjacent coins",          &perk_magnet,  1, 2500, CCOIN                    },
                { "SHIELD CHARGE",  "Start each round with a shield charge",         &perk_shield2, 1, 3000, (Color){ 80,150,255,255} },
            };
            int nPerks  = 6;
            int cardH   = 100;
            int cardGap = 8;

            int perkZoneTop = cZoneTop;
            int perkZoneBot = py2 + ph - 70;
            int availH_perk = perkZoneBot - perkZoneTop;

            int topOff      = 8;
            int totalH_perk = topOff + nPerks * (cardH + cardGap);
            int maxScrollPerk = totalH_perk - availH_perk;
            if (maxScrollPerk < 0) maxScrollPerk = 0;

            float wheel2 = GetMouseWheelMoveV().y;
            if (wheel2 != 0.0f) upgScroll -= (int)(wheel2 * 40.0f);
            if (IsKeyDown(KEY_UP))   upgScroll -= 7;
            if (IsKeyDown(KEY_DOWN)) upgScroll += 7;
            if (upgScroll < 0) upgScroll = 0;
            if (upgScroll > maxScrollPerk) upgScroll = maxScrollPerk;

            BeginScissorMode(cZoneX, perkZoneTop, cZoneW, availH_perk);

            for (int i = 0; i < nPerks; i++) {
                int ux  = cZoneX + 20;
                int uy  = (perkZoneTop + topOff) + i * (cardH + cardGap) - upgScroll;
                int uw  = cZoneW - 40;
                int uh  = cardH - 10;   // visual height (matches old 90px)

                if (uy + uh < perkZoneTop || uy > perkZoneBot) continue;

                bool purchased = false;
                if (perks[i].maxv == 1) purchased = (*perks[i].flag >= 1);
                else                    purchased = (*perks[i].flag >= i + 1);
                bool locked = (perks[i].flag == &perk_slots && i > 0 && perk_slots < i);
                float gp = 0.5f + 0.5f * sinf(t * 1.6f + (float)i * 0.9f);

                DrawRectangleRounded((Rectangle){(float)ux,(float)uy,(float)uw,(float)uh},
                                     0.12f,8,(Color){16,13,26,200});
                DrawRectangleRoundedLines((Rectangle){(float)ux,(float)uy,(float)uw,(float)uh},
                                         0.12f,8,(Color){perks[i].col.r,perks[i].col.g,perks[i].col.b,
                                                         purchased?180u:(uchar)(40+(int)(40*gp))});
                DrawRectangleRounded((Rectangle){(float)ux,(float)uy,4,(float)uh},1.0f,4,
                                     purchased?perks[i].col:(Color){perks[i].col.r,perks[i].col.g,perks[i].col.b,80});

                DrawText(perks[i].name, ux+18,uy+12,17,purchased?perks[i].col:(Color){perks[i].col.r,perks[i].col.g,perks[i].col.b,150});
                DrawText(locked?TextFormat("Requires Extra Slot %s first", i==1?"I":i==2?"II":"III"):perks[i].desc,
                         ux+18,uy+34,13,(Color){140,135,170,210});

                if (purchased) {
                    int pw3 = MeasureText("ACTIVE",13);
                    DrawText("ACTIVE",ux+18,uy+56,13,(Color){70,215,140,220});
                    DrawCircle(ux+18+pw3+12,uy+62,5,(Color){70,215,140,200});
                } else {
                    int btnX3=ux+uw-185, btnY3=uy+uh/2-18, btnW3=165, btnH3=36;
                    Rectangle btnR3={(float)btnX3,(float)btnY3,(float)btnW3,(float)btnH3};
                    Vector2 mp2=GetMousePosition();
                    bool bhov=(!locked)&&CheckCollisionPointRec(mp2,btnR3);
                    bool canBuy=(!locked)&&(coin_balance>=perks[i].cost);

                    Color bf=locked?(Color){18,16,28,130}:canBuy?(bhov?(Color){35,28,60,240}:(Color){22,18,42,200}):(Color){18,16,28,160};
                    DrawRectangleRounded(btnR3,0.25f,8,bf);
                    DrawRectangleRoundedLines(btnR3,0.25f,8,locked?(Color){45,42,62,80}:canBuy?(Color){perks[i].col.r,perks[i].col.g,perks[i].col.b,bhov?220u:110u}:(Color){50,46,70,80});

                    if (!locked) {
                        DrawCircle(btnX3+24,btnY3+18,7,canBuy?CCOIN:(Color){70,60,30,160});
                        DrawText(TextFormat("%d",perks[i].cost),btnX3+35,btnY3+11,14,canBuy?(Color){255,215,0,220}:(Color){100,90,55,150});
                        int bw2=MeasureText("UNLOCK",14);
                        DrawText("UNLOCK",btnX3+btnW3/2-bw2/2+18,btnY3+11,14,canBuy?(Color){220,215,255,230}:(Color){90,85,115,150});
                        if (canBuy && bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            coin_balance-=perks[i].cost;
                            if (perks[i].maxv==1) *perks[i].flag=1;
                            else (*perks[i].flag)++;
                            SaveStats(); SaveShop();
                        }
                    } else {
                        int bw2=MeasureText("LOCKED",14);
                        DrawText("LOCKED",btnX3+btnW3/2-bw2/2,btnY3+11,14,(Color){70,65,95,160});
                    }
                }
            }

            EndScissorMode();

            if (maxScrollPerk > 0) {
                Color pAc = (Color){185,90,255,255};
                int sbX  = cZoneX + cZoneW - 10;
                int sbY  = perkZoneTop;
                int sbH  = availH_perk;
                int sbTH = (int)((float)availH_perk / totalH_perk * sbH);
                if (sbTH < 20) sbTH = 20;
                int sbTY = sbY + (int)((float)upgScroll / maxScrollPerk * (sbH - sbTH));
                DrawRectangleRounded((Rectangle){(float)sbX,(float)sbY,4.0f,(float)sbH},
                                     1.0f,4,(Color){pAc.r,pAc.g,pAc.b,30});
                DrawRectangleRounded((Rectangle){(float)sbX,(float)sbTY,4.0f,(float)sbTH},
                                     1.0f,4,(Color){pAc.r,pAc.g,pAc.b,140});
            }
        }

        return;
    }
// ── ACHIEVEMENTS ─────────────────────────────────────────────────────────────
    static int advScroll = 0;
    if (menuSub == 4) {
        Color ac = (Color){55, 205, 255, 255};
        DrawSubPanel("ACHIEVEMENTS", ac, t, sw, sh);
        int margin2 = 32, pw = sw - margin2 * 2, ph = sh - margin2 * 2;
        int px2 = margin2, py2 = margin2;

        int zoneTop    = py2 + 64;
        int zoneBottom = py2 + ph - 70;
        int availH     = zoneBottom - zoneTop;


        // Precomputed values for derived/self-referential achievements
        int _skins_b = 0;
        for (int _i = 1; _i < SKIN_COUNT; _i++) if (skin_owned[_i]) _skins_b++;
        int _upg_b  = spd_level + shd_level + wlk_level + swd_level + bmb_level + gld_level;
        int _perk_b = perk_slots + perk_magnet + perk_shield2;
        int _ach_comp = 0, _ach_maxn = 0;
        {
            // max_t: 6 for 5-star, 4 for 3-star — thresholds = ACHIEVEMENTS UI
            struct { int v; int t[6]; int max_t; } _a[] = {
                { global_games_played,     { 25,  50,  100,   250,   500,  1000  }, 6 },
                { global_rounds,           { 100, 250,  800,  2000,  4000, 10000  }, 6 },
                { global_totcoi,           { 250,1000, 5000, 12000, 40000,100000  }, 6 },
                { global_totkills,         {  75, 160,  350,  1000,  2500,  5000  }, 6 },
                { hs_trs,                  {  10,  20,   25,    30,    40,    50   }, 6 },
                { hs_coi,                  {  20,  50,   75,   100,   150,   250   }, 6 },
                { hs_kills,                {   5,  10,   20,    35,    50,    75   }, 6 },
                { global_tottrs,           { 100, 300,  800,  1500,  4000, 10000  }, 6 },
                { global_kills_sword,      {  25, 100,  300,   750, 999999, 999999 }, 4 },
                { global_kills_shield,     {  15,  60,  200,   500, 999999, 999999 }, 4 },
                { global_portals,          {  15,  50,  150,   400, 999999, 999999 }, 4 },
                { global_ghostwalk_walls,  {  10,  40,  120,   300, 999999, 999999 }, 4 },
                { global_inv_full,         {  10,  35,  100,   250, 999999, 999999 }, 4 },
                { global_use_spd,          {  20,  75,  200,   500, 999999, 999999 }, 4 },
                { global_use_shd,          {  20,  75,  200,   500, 999999, 999999 }, 4 },
                { global_use_bmb,          {  20,  75,  200,   500, 999999, 999999 }, 4 },
                { global_use_wlk,          {  20,  75,  200,   500, 999999, 999999 }, 4 },
                { global_use_swd,          {  20,  75,  200,   500, 999999, 999999 }, 4 },
                { _skins_b,                {   1,   5,   15,    25, 999999, 999999 }, 4 },
                { _upg_b,                  {   2,   6,   12,    20, 999999, 999999 }, 4 },
                { _perk_b,                 {   1,   2,    4,     6, 999999, 999999 }, 4 },
                { global_bosses_killed,    {   6,  18,   48,    90,   180,   300  }, 6 },
                { hs_round,                {   5,  10,   20,    30,    40,    50  }, 6 },
                { hs_coi_round,            {  38,  75,  125,   200, 999999, 999999 }, 4 },
                { global_use_gld,          {   5,  20,   60,   150, 999999, 999999 }, 4 },
                { global_rooms_trap,       {   3,  10,   30,    80, 999999, 999999 }, 4 },
                { global_rooms_dark,       {   3,  10,   30,    80, 999999, 999999 }, 4 },
                { global_rooms_lava,       {   3,  10,   30,    80, 999999, 999999 }, 4 },
                { global_rooms_coinrush,   {   2,   8,   25,    60, 999999, 999999 }, 4 },
                { global_cannon_hits,      {   5,  20,   50,   120, 999999, 999999 }, 4 },
            };
            for (int _i = 0; _i < 30; _i++) {
                int _tier = 0;
                for (int _j = 0; _j < 6; _j++) {
                    if (_a[_i].v >= _a[_i].t[_j]) _tier = _j + 1; else break;
                }
                _ach_comp += _tier;
                if (_tier == _a[_i].max_t) _ach_maxn++;
            }
        }

        // Each row: name + desc + progress + bar + reward badge — 96 px min
        int rowH = availH / 7;
        if (rowH < 96) rowH = 96;

        struct {
            const char *name;
            const char *desc;
            int         current_val;
            int         tiers[6];
            int         max_stars;   // 3 or 5; golden border appears at max_stars+1
            Color       col;
        } achievements[] = {
            // ── 5-star achievements (golden border at tier 6) ─────────────────
            { "Veteran",          "Play games",
              global_games_played, { 25, 50, 100, 250, 500, 1000 },
              5, (Color){200,185,255,255} },
            { "Explorer",         "Play rounds",
              global_rounds,       { 100, 250, 800, 2000, 4000, 10000 },
              5, (Color){200,195,235,255} },
            { "Coin Hoarder",     "Collect coins",
              global_totcoi,       { 250, 1000, 5000, 12000, 40000, 100000 },
              5, (Color){255,215,0,255} },
            { "Slayer",           "Kill enemies (total)",
              global_totkills,     { 75, 160, 350, 1000, 2500, 5000 },
              5, (Color){220,90,90,255} },
            { "Treasure Hunter",  "Find treasures in one game",
              hs_trs,              { 10, 20, 25, 30, 40, 50 },
              5, CTRES },
            { "Rich",             "Collect coins in one game",
              hs_coi,              { 20, 50, 75, 100, 150, 250 },
              5, (Color){255,215,0,255} },
            { "Kill Streak",      "Kill enemies in one game",
              hs_kills,            { 5, 10, 20, 35, 50, 75 },
              5, (Color){220,75,75,255} },
            { "Looter",           "Collect treasures (total)",
              global_tottrs,       { 100, 300, 800, 1500, 4000, 10000 },
              5, CTRES },
            // Indices 8-9 fixed for save-compat of ach_claimed[]
            { "Overachiever",     "Earn achievement tiers",
              _ach_comp,           { 5, 15, 30, 55, 80, 140 },
              5, (Color){55,205,255,255} },
            { "Perfectionist",    "Max out achievements",
              _ach_maxn,           { 1, 2, 4, 8, 15, 30 },
              5, (Color){255,215,0,255} },
            // ── 3-star achievements (golden border at tier 4) ─────────────────
            { "Blade Master",     "Kill enemies with Sword item",
              global_kills_sword,  { 25, 100, 300, 750, 999999, 999999 },
              3, (Color){255,200,0,255} },
            { "Bodyguard",        "Kill enemies with Shield",
              global_kills_shield, { 15, 60, 200, 500, 999999, 999999 },
              3, (Color){80,150,255,255} },
            { "Dimension Hopper", "Enter portals",
              global_portals,      { 15, 50, 150, 400, 999999, 999999 },
              3, (Color){100,200,255,255} },
            { "Phantom",          "Pass through walls",
              global_ghostwalk_walls, { 10, 40, 120, 300, 999999, 999999 },
              3, (Color){200,100,255,255} },
            { "Pack Rat",         "Miss item pickups (inv full)",
              global_inv_full,     { 10, 35, 100, 250, 999999, 999999 },
              3, (Color){155,205,155,255} },
            { "Speed Freak",      "Use Speed items",
              global_use_spd,      { 20, 75, 200, 500, 999999, 999999 },
              3, (Color){80,220,80,255} },
            { "Iron Wall",        "Use Shield items",
              global_use_shd,      { 20, 75, 200, 500, 999999, 999999 },
              3, (Color){80,150,255,255} },
            { "Demolisher",       "Use Bomb items",
              global_use_bmb,      { 20, 75, 200, 500, 999999, 999999 },
              3, (Color){255,140,30,255} },
            { "Phase Walker",     "Use Ghost Walk items",
              global_use_wlk,      { 20, 75, 200, 500, 999999, 999999 },
              3, (Color){200,100,255,255} },
            { "Swordsman",        "Use Sword items",
              global_use_swd,      { 20, 75, 200, 500, 999999, 999999 },
              3, (Color){255,200,0,255} },
            { "Fashionista",      "Buy skins",
              _skins_b,            { 1, 5, 15, 25, 999999, 999999 },
              3, (Color){220,120,220,255} },
            { "Min-Maxer",        "Buy upgrades",
              _upg_b,              { 2, 6, 12, 20, 999999, 999999 },
              3, (Color){70,215,140,255} },
            { "Privileged",       "Buy perks",
              _perk_b,             { 1, 2, 4, 6, 999999, 999999 },
              3, (Color){150,200,255,255} },
            // ── V1.6 (appended — preserves ach_claimed indices 0-22) ────────
            { "Boss Slayer",      "Defeat bosses",
              global_bosses_killed,{ 6, 18, 48, 90, 180, 300 },
              5, (Color){170,40,200,255} },
            // desc is patched per-frame below to the next goal text
            { "Unbeaten Champion","Reach Round 5",
              hs_round,            { 5, 10, 20, 30, 40, 50 },
              5, (Color){255,120,60,255} },
            { "Round Tycoon",     "Collect coins in one round",
              hs_coi_round,        { 38, 75, 125, 200, 999999, 999999 },
              3, (Color){255,215,0,255} },
            { "Midas",            "Use Gold Touch",
              global_use_gld,      { 5, 20, 60, 150, 999999, 999999 },
              3, (Color){255,200,40,255} },
            { "Arrow Dodger",     "Clear Trap rooms",
              global_rooms_trap,   { 3, 10, 30, 80, 999999, 999999 },
              3, (Color){235,150,60,255} },
            { "Night Owl",        "Clear Dark rooms",
              global_rooms_dark,   { 3, 10, 30, 80, 999999, 999999 },
              3, (Color){150,120,230,255} },
            { "Floor is Lava",    "Clear Lava rooms",
              global_rooms_lava,   { 3, 10, 30, 80, 999999, 999999 },
              3, (Color){255,90,40,255} },
            { "Coin Rusher",      "Clear Coin Rush rooms",
              global_rooms_coinrush,{ 2, 8, 25, 60, 999999, 999999 },
              3, (Color){255,210,40,255} },
            { "Artillerist",      "Hit bosses with cannons",
              global_cannon_hits,  { 5, 20, 50, 120, 999999, 999999 },
              3, (Color){255,160,80,255} },
        };
        int numAch     = (int)(sizeof(achievements) / sizeof(achievements[0]));
        int totalRowsH = numAch * rowH;

        int maxScroll = totalRowsH - availH;
        if (maxScroll < 0) maxScroll = 0;
        float wheel = GetMouseWheelMoveV().y;
        if (wheel != 0.0f) advScroll -= (int)(wheel * 40.0f);
        if (IsKeyDown(KEY_UP))   advScroll -= 7;
        if (IsKeyDown(KEY_DOWN)) advScroll += 7;
        if (advScroll < 0) advScroll = 0;
        if (advScroll > maxScroll) advScroll = maxScroll;

        int cx = px2 + 40;
        BeginScissorMode(px2, zoneTop, pw, availH);

        for (int i = 0; i < numAch; i++) {
            int ry2 = zoneTop + i * rowH - advScroll;
            if (ry2 + rowH < zoneTop || ry2 > zoneBottom) continue;

            // ── row background ──────────────────────────────────────────────
            Color rowbg = (i % 2 == 0) ? (Color){20,16,34,160} : (Color){15,12,26,90};
            DrawRectangleRounded(
                (Rectangle){(float)(cx-8),(float)(ry2-4),(float)(pw-64),(float)(rowH-6)},
                0.12f, 6, rowbg);
            DrawRectangleRounded(
                (Rectangle){(float)(cx-8),(float)(ry2-4),4.0f,(float)(rowH-6)},
                1.0f, 4, achievements[i].col);

            // ── achieved tier: tiers[0..max_stars-1]=stars, tiers[max_stars]=golden ──
            int achieved_tier = 0;
            for (int j = 0; j < 6; j++) {
                if (achievements[i].current_val >= achievements[i].tiers[j]) achieved_tier = j + 1;
                else break;
            }
            int goldenTier   = achievements[i].max_stars + 1;
            int stars_earned = (achieved_tier < achievements[i].max_stars) ? achieved_tier : achievements[i].max_stars;

            // Unbeaten Champion: desc = next goal (round milestones / boss names)
            static const char *UNBEATEN_GOAL[6] = {
                "Reach Round 5",
                "Defeat the Chaos Warden",
                "Defeat the Crimson Herald",
                "Defeat the Inferno Hydra",
                "Defeat the Obsidian Brute",
                "Defeat the Ashen Wraith",
            };
            const char *desc = achievements[i].desc;
            if (strcmp(achievements[i].name, "Unbeaten Champion") == 0)
                desc = (achieved_tier < 6) ? UNBEATEN_GOAL[achieved_tier] : UNBEATEN_GOAL[5];

            // ── name + description ─────────────────────────────────────────
            DrawText(achievements[i].name, cx+6, ry2+5,  16, achievements[i].col);
            DrawText(desc, cx+6, ry2+24, 12, (Color){140,135,170,200});

            // ── progress text (rounds for Unbeaten = best round reached) ──
            const char *progressText;
            if (achieved_tier < goldenTier)
                progressText = TextFormat("%d / %d", achievements[i].current_val, achievements[i].tiers[achieved_tier]);
            else
                progressText = TextFormat("%d (MAX)", achievements[i].current_val);
            DrawText(progressText, cx+6, ry2+42, 15, achievements[i].col);

            // ── progress bar (toward next tier) ───────────────────────────
            float pct = 0.0f;
            if (achieved_tier < goldenTier) {
                int from = (achieved_tier == 0) ? 0 : achievements[i].tiers[achieved_tier-1];
                int to   = achievements[i].tiers[achieved_tier];
                int range = to - from;
                if (range > 0) pct = (float)(achievements[i].current_val - from) / range;
            } else { pct = 1.0f; }
            if (pct < 0.0f) pct = 0.0f;
            if (pct > 1.0f) pct = 1.0f;

            int barW = 180, barH2 = 5;
            int barX = cx + 6, barY = ry2 + 62;
            DrawRectangleRounded((Rectangle){(float)barX,(float)barY,(float)barW,(float)barH2},
                                 1.0f,4,(Color){45,40,65,255});
            if (pct > 0.0f)
                DrawRectangleRounded((Rectangle){(float)barX,(float)barY,(float)(barW*pct),(float)barH2},
                                     1.0f,4, achievements[i].col);

            // ── Reward badge: lowest unclaimed earned tier ─────────────────
            int claim_tier = -1;
            for (int j = 0; j < 6; j++) {
                if (achievements[i].current_val >= achievements[i].tiers[j]
                    && !ach_claimed[i][j]) { claim_tier = j; break; }
            }
            if (claim_tier >= 0) {
                int reward = ACH_REWARDS[claim_tier];

                // anchor to the left of the star block
                int _ns = achievements[i].max_stars, _ss = 28;
                int _starX = cx + (pw - 64) - (_ns - 1) * _ss - 28;
                int _ctrY  = (ry2 - 4) + (rowH - 6) / 2;

                // two-part layout: coin pill (top) + CLAIM button (bottom)
                int pillW = 110, pillH = 28;
                int btnW  = 110, btnH  = 24;
                int gap   = 6;
                int totalH2 = pillH + gap + btnH;
                int bx = _starX - pillW - 22;
                int by = _ctrY - totalH2 / 2;

                Vector2 mp2  = GetMousePosition();
                Rectangle btnR = { (float)bx, (float)(by + pillH + gap), (float)btnW, (float)btnH };
                bool hoverBtn = CheckCollisionPointRec(mp2, btnR);

                // ── coin pill ─────────────────────────────────────────────
                DrawRectangleRounded(
                    (Rectangle){(float)bx, (float)by, (float)pillW, (float)pillH},
                    0.5f, 8, (Color){30, 24, 8, 200});
                DrawRectangleRoundedLines(
                    (Rectangle){(float)bx, (float)by, (float)pillW, (float)pillH},
                    0.5f, 8, (Color){255, 200, 0, 80});
                // coin circle
                int coinCX = bx + 16, coinCY = by + pillH / 2;
                DrawCircle(coinCX,     coinCY,     8,  (Color){200, 160,  0, 255});
                DrawCircle(coinCX,     coinCY,     6,  (Color){255, 215,  0, 255});
                DrawCircle(coinCX - 2, coinCY - 2, 2,  (Color){255, 248, 160, 200});
                // reward text
                const char *amtStr = TextFormat("+%d", reward);
                int amtW = MeasureText(amtStr, 17);
                DrawText(amtStr,
                         bx + pillW / 2 - amtW / 2 + 6, by + pillH / 2 - 9,
                         17, (Color){255, 220, 60, 255});

                // ── CLAIM button ──────────────────────────────────────────
                Color btnBg  = hoverBtn ? (Color){255, 200, 0, 230} : (Color){60, 48, 0, 200};
                Color btnBdr = hoverBtn ? (Color){255, 240, 120, 255} : (Color){200, 160, 0, 160};
                Color btnTxt = hoverBtn ? (Color){20, 14, 0, 255}     : (Color){220, 180, 40, 220};
                DrawRectangleRounded(btnR, 0.45f, 8, btnBg);
                DrawRectangleRoundedLines(btnR, 0.45f, 8, btnBdr);
                const char *clLbl = "COLLECT REWARD";
                int clW = MeasureText(clLbl, 10);
                DrawText(clLbl, bx + btnW / 2 - clW / 2, by + pillH + gap + btnH / 2 - 5,
                         10, btnTxt);

                if (hoverBtn && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    ach_claimed[i][claim_tier] = 1;
                    coin_balance    += reward;
                    global_totcoi   += reward;
                    SaveStats(); SaveShop();
                }
            }

            // ── stars + optional golden border ────────────────────────────
            int numStars      = achievements[i].max_stars;
            int starSpacing   = 28;
            int totalStarSpan = (numStars - 1) * starSpacing;
            int starStartX    = cx + (pw - 64) - totalStarSpan - 28;
            int starCenterY   = (ry2 - 4) + (rowH - 6) / 2;

            if (achieved_tier == goldenTier) {
                Rectangle borderRect = {
                    (float)(starStartX - 16),
                    (float)(starCenterY - 17),
                    (float)(totalStarSpan + 32),
                    34.0f
                };
                DrawRectangleRoundedLines(borderRect, 0.3f, 4, (Color){255,215,0,220});
            }
            for (int s = 0; s < numStars; s++) {
                int starX = starStartX + s * starSpacing;
                if (s < stars_earned)
                    DrawCustomStar((float)starX, (float)starCenterY, 12.0f, 5.0f,
                                   achieved_tier==goldenTier ? (Color){255,215,0,255} : (Color){255,205,55,255});
                else
                    DrawCustomStarLines((float)starX, (float)starCenterY, 12.0f, 5.0f, (Color){80,75,105,120});
            }
        }

        EndScissorMode();

        if (maxScroll > 0) {
            int sbX  = px2 + pw - 10;
            int sbY  = zoneTop;
            int sbH  = availH;
            int sbTH = (int)((float)availH / totalRowsH * sbH);
            if (sbTH < 20) sbTH = 20;
            int sbTY = sbY + (int)((float)advScroll / maxScroll * (sbH - sbTH));
            DrawRectangleRounded((Rectangle){(float)sbX,(float)sbY,4.0f,(float)sbH},
                                 1.0f,4,(Color){ac.r,ac.g,ac.b,30});
            DrawRectangleRounded((Rectangle){(float)sbX,(float)sbTY,4.0f,(float)sbTH},
                                 1.0f,4,(Color){ac.r,ac.g,ac.b,140});
        }
        return;
    }
    // ── FUTURE UPDATES ────────────────────────────────────────────────────────
    if (menuSub == 5) {
        Color ac = (Color){185,90,255,255};
        Rectangle backR = DrawSubPanel("FUTURE UPDATES", ac, t, sw, sh);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(GetMousePosition(), backR))
            { menuSub = 0; return; }

        int margin2=32, pw=sw-margin2*2, ph=sh-margin2*2;
        int px2=margin2, py2=margin2;

        // ── Update data ──────────────────────────────────────────────────────
        const char *ver_str  = "v1.7";
        const char *upd_name = "The Secret Chambers";
        static const char *features[] = {
            "Android: v1.7 will be compiled for Android",
            "Web: v1.7 will be compiled for the Web",
            "Play Store: v1.7 will be uploaded in the App Store",
            "Key Binds: Full controll over key binds",
            "Scroll to use Item: Scroll to switch Items and use with space",
            "New Perks:3+ New Perks will be added ",
            "Seeded Run: Challenge for special Seeds",
            "Challenges: New challenges will contain new things to do",
            "New Rooms: New Portals that lead to special rooms to explore with new rewards",
            "Screen Shake: Bosses will make the screen shake"
        };
        int feat_count = (int)(sizeof(features)/sizeof(features[0]));

        float pp  = 0.5f + 0.5f*sinf(t*1.4f);
        float pp2 = 0.5f + 0.5f*sinf(t*0.9f + 1.1f);

        // ── LEFT COLUMN: version card ─────────────────────────────────────────
        int contentTop = py2 + 66;
        int colGap = 28;
        int colL_w = 280;
        int colL_x = px2 + 24;

        int vcard_h = 180;
        Rectangle vcard = { (float)colL_x, (float)contentTop, (float)colL_w, (float)vcard_h };

        for (int g = 6; g >= 1; g--) {
            float gf = (float)g / 6.0f * pp2;
            DrawRectangleRounded(
                (Rectangle){vcard.x-g*2, vcard.y-g*2, vcard.width+g*4, vcard.height+g*4},
                0.18f, 8,
                (Color){ac.r, ac.g, ac.b, (uchar)(8.0f*gf)});
        }
        DrawRectangleRounded(vcard, 0.14f, 8, (Color){22,14,38,248});
        DrawRectangleRounded(
            (Rectangle){vcard.x+1, vcard.y+1, vcard.width-2, 38},
            0.14f, 8, (Color){ac.r/6, ac.g/6, ac.b/5, 255});
        Color vbdr = { (uchar)(ac.r*(0.6f+0.4f*pp)), (uchar)(ac.g*(0.6f+0.4f*pp)),
                       (uchar)(ac.b*(0.6f+0.4f*pp)), 220 };
        DrawRectangleRoundedLines(vcard, 0.14f, 8, vbdr);
        DrawCornerAccents(vcard, (Color){ac.r,ac.g,ac.b,180}, 14);

        const char *nxt = "NEXT UPDATE";
        int nxtw = MeasureText(nxt, 13);
        DrawText(nxt, (int)(vcard.x + vcard.width/2 - nxtw/2), (int)(vcard.y+12), 13,
                 (Color){ac.r,ac.g,ac.b,200});

        int vfs = 46;
        int vw  = MeasureText(ver_str, vfs);
        int vx  = (int)(vcard.x + vcard.width/2 - vw/2);
        int vy  = (int)(vcard.y + 46);
        for (int g=5;g>=1;g--) {
            uchar ga = (uchar)(13*g*(0.5f+0.5f*pp));
            DrawText(ver_str, vx-g, vy-g, vfs, (Color){ac.r,ac.g,ac.b,ga});
            DrawText(ver_str, vx+g, vy+g, vfs, (Color){ac.r,ac.g,ac.b,ga});
        }
        DrawText(ver_str, vx+2, vy+3, vfs, (Color){0,0,0,130});
        DrawText(ver_str, vx,   vy,   vfs, (Color){235,225,255,255});

        DrawLine((int)(vcard.x+20), (int)(vcard.y+105), (int)(vcard.x+vcard.width-20),
                 (int)(vcard.y+105), (Color){ac.r,ac.g,ac.b,55});

        int unfs = 15;
        int unw  = MeasureText(upd_name, unfs);
        DrawText(upd_name,
                 (int)(vcard.x + vcard.width/2 - unw/2),
                 (int)(vcard.y + 116), unfs,
                 (Color){(uchar)CLAMP255(ac.r+50),(uchar)CLAMP255(ac.g+55),(uchar)CLAMP255(ac.b+30),230});

        float pdp = 0.5f+0.5f*sinf(t*2.5f);
        int dot_y = (int)(vcard.y + 148);
        int dot_cx = (int)(vcard.x + vcard.width/2);
        for (int d=-1;d<=1;d++) {
            float df = (d==0) ? pdp : (0.3f+0.2f*sinf(t*2.5f + d*1.1f));
            DrawCircle(dot_cx + d*18, dot_y, (int)(4.0f*df)+2,
                       (Color){ac.r,ac.g,ac.b,(uchar)(60+140*df)});
            DrawCircle(dot_cx + d*18, dot_y, (int)(2.0f*df)+1,
                       (Color){230,215,255,(uchar)(120+120*df)});
        }

        // ── LEFT COLUMN: ETA badge below version card ─────────────────────────
        int eta_y = contentTop + vcard_h + 16;
        Rectangle etacard = { (float)colL_x, (float)eta_y, (float)colL_w, 52 };
        DrawRectangleRounded(etacard, 0.28f, 8, (Color){16,10,28,220});
        DrawRectangleRoundedLines(etacard, 0.28f, 8, (Color){ac.r,ac.g,ac.b,60});
        const char *eta_lbl = "STATUS";
        const char *eta_val = "IN DEVELOPMENT";
        int elw = MeasureText(eta_lbl, 11);
        int evw = MeasureText(eta_val, 14);
        DrawText(eta_lbl, (int)(etacard.x+etacard.width/2-elw/2), (int)(etacard.y+7),
                 11, (Color){120,110,155,200});
        DrawText(eta_val, (int)(etacard.x+etacard.width/2-evw/2), (int)(etacard.y+24),
                 14, (Color){(uchar)(100+60*pp),(uchar)(200+40*pp2),(uchar)(130+60*pp),255});

        for (int i=0;i<5;i++) {
            float a = (float)i/5*2*PI + t*0.55f;
            int orx = (int)(etacard.x + etacard.width/2 + cosf(a)*148);
            int ory = (int)(eta_y + 26 + sinf(a)*22);
            float pf = 0.5f+0.5f*sinf(t*2.0f+i*1.3f);
            DrawCircle(orx, ory, (int)(2.0f+pf), (Color){ac.r,ac.g,ac.b,(uchar)(30+50*pf)});
        }

        int colR_x = colL_x + colL_w + colGap;
        int colR_w = px2 + pw - colR_x - 24;
        int listTop = contentTop;

        const char *feat_hdr = "NEW FEATURES";
        DrawText(feat_hdr, colR_x, listTop, 14, (Color){ac.r,ac.g,ac.b,180});
        DrawLine(colR_x, listTop+18, colR_x+colR_w, listTop+18,
                 (Color){ac.r,ac.g,ac.b,45});

        int rowH2  = 44;
        int listY0 = listTop + 26;

        for (int i = 0; i < feat_count; i++) {
            int ry = listY0 + i * rowH2;
            if (ry + rowH2 > py2 + ph - 70) break;

            float rpp = 0.5f + 0.5f*sinf(t*1.3f + i*0.42f);

            Color rowbg = (i%2==0) ? (Color){20,14,34,150} : (Color){14,10,24,80};
            DrawRectangleRounded(
                (Rectangle){(float)colR_x, (float)(ry-4), (float)colR_w, (float)(rowH2-4)},
                0.10f, 6, rowbg);

            Color barCol = { (uchar)(ac.r*(0.5f+0.5f*rpp)),
                             (uchar)(ac.g*(0.5f+0.5f*rpp)),
                             (uchar)(ac.b*(0.5f+0.5f*rpp)), 255 };
            DrawRectangleRounded(
                (Rectangle){(float)colR_x, (float)(ry-4), 3.0f, (float)(rowH2-4)},
                1.0f, 4, barCol);

            int bx2 = colR_x + 14;
            int by2 = ry + rowH2/2 - 6;
            DrawRectanglePro(
                (Rectangle){(float)bx2, (float)by2, 8.0f, 8.0f},
                (Vector2){4.0f, 4.0f}, 45.0f,
                (Color){ac.r, ac.g, ac.b, (uchar)(140 + (int)(80*rpp))});
            DrawRectanglePro(
                (Rectangle){(float)bx2, (float)by2, 4.0f, 4.0f},
                (Vector2){2.0f, 2.0f}, 45.0f,
                (Color){230,215,255,(uchar)(160+80*rpp)});

            const char *full = features[i];
            const char *colon = strchr(full, ':');
            if (colon) {
                int tagLen = (int)(colon - full + 1);
                char tag[64]; if (tagLen > 63) tagLen=63;
                strncpy(tag, full, tagLen); tag[tagLen]='\0';
                int tagW = MeasureText(tag, 17);
                DrawText(tag, bx2+16, ry+2, 17,
                         (Color){(uchar)CLAMP255(ac.r+55),(uchar)CLAMP255(ac.g+55),
                                 (uchar)CLAMP255(ac.b+30), 255});
                DrawText(colon+1, bx2+16+tagW+4, ry+2, 17,
                         (Color){175,168,205,215});
            } else {
                DrawText(full, bx2+16, ry+2, 17, (Color){175,168,205,215});
            }

            if (i == 0) {
                char badge[16]; snprintf(badge,16,"%d FEATURES", feat_count);
                int bw = MeasureText(badge, 12);
                DrawRectangleRounded(
                    (Rectangle){(float)(colR_x+colR_w-bw-18),(float)(listTop-4),(float)(bw+16),22},
                    0.5f, 6, (Color){ac.r/5,ac.g/5,ac.b/4,200});
                DrawRectangleRoundedLines(
                    (Rectangle){(float)(colR_x+colR_w-bw-18),(float)(listTop-4),(float)(bw+16),22},
                    0.5f, 6, (Color){ac.r,ac.g,ac.b,100});
                DrawText(badge, colR_x+colR_w-bw-10, listTop, 12,
                         (Color){ac.r,ac.g,ac.b,200});
            }
        }

        return;
    }

    // ── SETTINGS ─────────────────────────────────────────────────────────────
    if (menuSub == 6) {
        Color ac = (Color){180,160,255,255};
        Rectangle backR = DrawSubPanel("SETTINGS", ac, t, sw, sh);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(GetMousePosition(), backR))
            { menuSub = 0; return; }

        // ── Responsive layout ─────────────────────────────────────────────────
        // Panel: margin=32px, title area=72px, back button starts at sh-88.
        int margin_s = 32;
        int panel_w  = sw - margin_s*2;
        int avail_h  = (sh - 88) - (margin_s + 72) - 12;

        // Row height: 9 rows + 8 row-gaps fill (avail_h - 98) where 98 = 3 section
        // headers (26px each) + 2 inter-section gaps (10px each).
        // Solving 9*rh + 8*(rh/7) ≈ avail_h-98  →  rh = 7*(avail_h-98)/71
        int rh = 7 * (avail_h - 98) / 71;
        if (rh < 26) rh = 26;
        if (rh > 44) rh = 44;
        int rgap = rh / 7;  if (rgap < 3) rgap = 3;
        int sgap = rgap + 4;

        // Row card width, centered inside the panel
        int rw = panel_w - 80;
        if (rw > 900) rw = 900;
        int cx = margin_s + (panel_w - rw) / 2;

        // Control positions within each row
        int label_area = rw / 4;
        int ctrlX = cx + label_area + 12;
        int togX  = cx + rw - 115;
        int slW   = togX - ctrlX - 70;
        if (slW < 80) slW = 80;

        // Control sizes centered vertically within rh
        int sli_h    = 18;
        int tog_h    = (rh >= 34) ? 30 : rh - 6;
        int sli_yoff = (rh - sli_h) / 2;
        int tog_yoff = (rh - tog_h) / 2; if (tog_yoff < 2) tog_yoff = 2;
        int fs_lbl   = (rh >= 38) ? 15 : 13;
        int fs_desc  = 11;
        bool show_desc = (rh >= 32);

        bool save_needed = false;
        int  ry = margin_s + 72;

        // ═══ AUDIO ══════════════════════════════════════════════════════════
        {
            int lw2 = MeasureText("AUDIO", 14);
            DrawLine(cx, ry+10, cx+40, ry+10, (Color){155,100,255,110});
            DrawText("AUDIO", cx+48, ry+3, 14, (Color){165,115,255,200});
            DrawLine(cx+58+lw2, ry+10, cx+rw, ry+10, (Color){155,100,255,110});
        }
        ry += 26;

        // Mute All
        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){155,100,255,45});
            int ly = show_desc ? ry+5 : ry+(rh-fs_lbl)/2;
            DrawText("MUTE ALL", cx+12, ly, fs_lbl, (Color){165,120,255,220});
            if (show_desc)
                DrawText("Silence all audio output", cx+12, ry+rh-16, fs_desc,
                         (Color){100,80,160,170});
            save_needed |= SettingsToggle(togX, ry+tog_yoff, 100, tog_h, &audio_mute,
                                          (Color){180,100,255,255});
        }
        ry += rh + rgap;

        // Master Volume
        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){155,100,255,45});
            DrawText("MASTER VOL", cx+12, ry+(rh-fs_lbl)/2, fs_lbl, (Color){165,120,255,220});
            save_needed |= SettingsSlider(ctrlX, ry+sli_yoff, slW, sli_h,
                                          &vol_master, 0.0f, 1.0f, (Color){180,100,255,255});
            DrawText(TextFormat("%.0f%%", vol_master*100.0f), ctrlX+slW+10, ry+sli_yoff, fs_lbl,
                     (Color){170,140,225,210});
        }
        ry += rh + rgap;

        // Music + SFX side-by-side in one row
        {
            int hw   = rw/2 - 4;
            int msw  = hw - 65 - 50; if (msw < 60) msw = 60;
            int sfx_x = cx + hw + 8;
            int sfw   = rw - hw - 8;
            int ssw   = sfw - 50 - 50; if (ssw < 60) ssw = 60;
            // Music card
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)hw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)hw,(float)rh},
                                       0.18f,6,(Color){155,100,255,45});
            DrawText("MUSIC", cx+10, ry+(rh-fs_lbl)/2, fs_lbl, (Color){165,120,255,220});
            save_needed |= SettingsSlider(cx+65, ry+sli_yoff, msw, sli_h,
                                          &vol_music, 0.0f, 1.0f, (Color){180,100,255,255});
            DrawText(TextFormat("%.0f%%",vol_music*100.0f), cx+65+msw+6, ry+sli_yoff, fs_desc,
                     (Color){160,130,210,200});
            // SFX card
            DrawRectangleRounded((Rectangle){(float)sfx_x,(float)ry,(float)sfw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)sfx_x,(float)ry,(float)sfw,(float)rh},
                                       0.18f,6,(Color){155,100,255,45});
            DrawText("SFX", sfx_x+10, ry+(rh-fs_lbl)/2, fs_lbl, (Color){165,120,255,220});
            save_needed |= SettingsSlider(sfx_x+50, ry+sli_yoff, ssw, sli_h,
                                          &vol_sfx, 0.0f, 1.0f, (Color){180,100,255,255});
            DrawText(TextFormat("%.0f%%",vol_sfx*100.0f), sfx_x+50+ssw+6, ry+sli_yoff, fs_desc,
                     (Color){160,130,210,200});
        }
        ry += rh + sgap;

        // ═══ GAMEPLAY ════════════════════════════════════════════════════════
        {
            int lw2 = MeasureText("GAMEPLAY", 14);
            DrawLine(cx, ry+10, cx+40, ry+10, (Color){80,200,140,110});
            DrawText("GAMEPLAY", cx+48, ry+3, 14, (Color){100,215,155,200});
            DrawLine(cx+58+lw2, ry+10, cx+rw, ry+10, (Color){80,200,140,110});
        }
        ry += 26;

        // Enemies (ON=red / OFF=green — game convention)
        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){80,200,140,45});
            int ly = show_desc ? ry+5 : ry+(rh-fs_lbl)/2;
            DrawText("ENEMIES", cx+12, ly, fs_lbl, (Color){100,215,155,220});
            if (show_desc)
                DrawText(eneon
                         ? "ON: Rounds count toward stats"
                         : "OFF: Practice, stats not saved",
                         cx+12, ry+rh-16,
                         fs_desc, eneon ? (Color){60,150,100,170}
                                        : (Color){200,160,70,200});
            {
                Rectangle br = {(float)togX,(float)(ry+tog_yoff),(float)100,(float)tog_h};
                bool ehovB = CheckCollisionPointRec(GetMousePosition(), br);
                Color eBg  = eneon ? (Color){50,12,12,220}  : (Color){10,38,18,200};
                Color eBdr = eneon ? (Color){220,65,65,200} : (Color){65,200,65,180};
                DrawRectangleRounded(br, 0.35f,8, eBg);
                DrawRectangleRoundedLines(br, 0.35f,8, eBdr);
                const char *el = eneon ? "ON" : "OFF";
                int elw = MeasureText(el, 16);
                DrawText(el, togX+50-elw/2, (int)(br.y + tog_h/2 - 8), 16,
                         eneon?(Color){255,100,100,255}:(Color){80,220,80,255});
                if (ehovB && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) eneon = !eneon;
            }
        }
        ry += rh + rgap;

        // Particle Density 3-way selector
        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){80,200,140,45});
            int ly = show_desc ? ry+5 : ry+(rh-fs_lbl)/2;
            DrawText("PARTICLES", cx+12, ly, fs_lbl, (Color){100,215,155,220});
            if (show_desc)
                DrawText("Burst density:  Low=50%  Med=100%  High=200%",
                         cx+12, ry+rh-16, fs_desc, (Color){60,150,100,170});
            const char *plbl[3] = {"LOW","MED","HIGH"};
            int pbw = 88, pbgap = 6;
            int pbsx = cx + rw - 15 - (3*pbw + 2*pbgap);
            for (int pi = 0; pi < 3; pi++) {
                int bx = pbsx + pi*(pbw+pbgap);
                bool sel = (par_density == pi);
                Rectangle pr = {(float)bx,(float)(ry+tog_yoff),(float)pbw,(float)tog_h};
                DrawRectangleRounded(pr, 0.35f,6,
                    sel?(Color){15,40,25,230}:(Color){14,18,22,180});
                DrawRectangleRoundedLines(pr, 0.35f,6,
                    sel?(Color){80,220,140,220}:(Color){50,140,80,140});
                int plw = MeasureText(plbl[pi], 14);
                DrawText(plbl[pi], bx+pbw/2-plw/2, (int)(pr.y+tog_h/2-7), 14,
                    sel?(Color){100,240,160,255}:(Color){65,130,90,200});
                if (CheckCollisionPointRec(GetMousePosition(),pr) &&
                    IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                    { par_density = pi; save_needed = true; }
            }
        }
        ry += rh + rgap;

        // Camera Smoothness
        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){80,200,140,45});
            DrawText("CAMERA SPEED", cx+12, ry+(rh-fs_lbl)/2, fs_lbl, (Color){100,215,155,220});
            save_needed |= SettingsSlider(ctrlX, ry+sli_yoff, slW, sli_h,
                                          &cam_smooth, 1.0f, 20.0f, (Color){80,220,150,255});
            DrawText(TextFormat("%.1f", cam_smooth), ctrlX+slW+10, ry+sli_yoff, fs_lbl,
                     (Color){110,200,160,210});
        }
        ry += rh + sgap;

        // ═══ GRAFIK / UI ══════════════════════════════════════════════════════
        {
            int lw2 = MeasureText("GRAFIK / UI", 14);
            DrawLine(cx, ry+10, cx+40, ry+10, (Color){80,150,255,110});
            DrawText("GRAFIK / UI", cx+48, ry+3, 14, (Color){100,170,255,200});
            DrawLine(cx+58+lw2, ry+10, cx+rw, ry+10, (Color){80,150,255,110});
        }
        ry += 26;

        // Fullscreen
        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){80,150,255,45});
            int ly = show_desc ? ry+5 : ry+(rh-fs_lbl)/2;
            DrawText("FULLSCREEN", cx+12, ly, fs_lbl, (Color){100,170,255,220});
            if (show_desc)
                DrawText("Toggle fullscreen mode  (also: F11)", cx+12, ry+rh-16,
                         fs_desc, (Color){60,110,200,170});
            {
                bool fs = IsWindowFullscreen();
                Rectangle br = {(float)togX,(float)(ry+tog_yoff),(float)100,(float)tog_h};
                Color fsBg  = fs ? (Color){12,24,50,230}  : (Color){14,18,28,200};
                Color fsBdr = fs ? (Color){80,150,255,200} : (Color){50,80,160,160};
                DrawRectangleRounded(br, 0.35f,8, fsBg);
                DrawRectangleRoundedLines(br, 0.35f,8, fsBdr);
                const char *fl = fs ? "ON" : "OFF";
                int flw = MeasureText(fl, 16);
                DrawText(fl, togX+50-flw/2, (int)(br.y+tog_h/2-8), 16,
                         fs?(Color){100,170,255,255}:(Color){70,90,150,200});
                if (CheckCollisionPointRec(GetMousePosition(),br) &&
                    IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                    ToggleFullscreenNow();
            }
        }
        ry += rh + rgap;

        // HUD Opacity
        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){80,150,255,45});
            DrawText("HUD OPACITY", cx+12, ry+(rh-fs_lbl)/2, fs_lbl, (Color){100,170,255,220});
            save_needed |= SettingsSlider(ctrlX, ry+sli_yoff, slW, sli_h,
                                          &hud_opacity, 0.2f, 1.0f, (Color){80,150,255,255});
            DrawText(TextFormat("%.0f%%", hud_opacity*100.0f), ctrlX+slW+10, ry+sli_yoff,
                     fs_lbl, (Color){100,155,225,210});
        }
        ry += rh + rgap;

        // Show FPS
        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){80,150,255,45});
            int ly = show_desc ? ry+5 : ry+(rh-fs_lbl)/2;
            DrawText("SHOW FPS", cx+12, ly, fs_lbl, (Color){100,170,255,220});
            if (show_desc)
                DrawText("Overlay frames-per-second counter", cx+12, ry+rh-16,
                         fs_desc, (Color){60,110,200,170});
            save_needed |= SettingsToggle(togX, ry+tog_yoff, 100, tog_h, &show_fps,
                                          (Color){100,170,255,255});
        }
        (void)ry;

        if (save_needed) SaveSettings();
        return;
    }

    // ── SOUNDTRACK SHOP ────────────────────────────────────────────────────────
    if (menuSub == 7) {
        Color ac = (Color){95,205,240,255};
        Rectangle backR = DrawSubPanel("SOUNDTRACK SHOP", ac, t, sw, sh);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(GetMousePosition(), backR))
            { menuSub = 0; return; }

        static int musicShopContext = 0;  // 0 = home menu, 1 = gameplay
        int contentW = sw - 128;
        if (contentW > 1450) contentW = 1450;
        if (contentW < 560) contentW = sw - 48;
        int cx = sw/2 - contentW/2;
        int trackCount = GetMusicTrackCount();
        int cardCount = trackCount + 1;
        bool save_needed = false;

        Rectangle resetR = {(float)(cx+contentW-188), 94.0f, 188.0f, 28.0f};
        bool resetHover = CheckCollisionPointRec(GetMousePosition(), resetR);
        Color resetAccent = resetHover ? (Color){215,190,255,255}
                                       : (Color){155,135,205,220};
        DrawRectangleRounded(resetR, 0.28f, 6,
                             resetHover ? (Color){35,25,55,240}
                                        : (Color){18,15,31,225});
        DrawRectangleRoundedLines(resetR, 0.28f, 6,
                                  (Color){resetAccent.r, resetAccent.g,
                                          resetAccent.b,
                                          (uchar)(resetHover ? 220 : 125)});
        const char *resetLabel = "RESET DEFAULTS";
        int resetW = MeasureText(resetLabel, 11);
        DrawText(resetLabel, (int)(resetR.x+resetR.width/2-resetW/2),
                 (int)resetR.y+8, 11, resetAccent);
        if (resetHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            music_menu_tracks = MUSIC_DEFAULT_MENU_MASK;
            music_gameplay_tracks = MUSIC_DEFAULT_GAMEPLAY_MASK;
            save_needed = true;
        }

        // These are tabs, not another settings row: they establish the two
        // soundtrack slots the player can actually customize.
        int tabY = 126;
        int tabGap = 12;
        int tabW = (contentW-tabGap)/2;
        const char *tabLabels[2] = { "HOME MENU", "GAMEPLAY" };
        for (int ti = 0; ti < 2; ti++) {
            int tx = cx + ti*(tabW+tabGap);
            Rectangle tab = {(float)tx,(float)tabY,(float)tabW,30.0f};
            bool activeTab = (musicShopContext == ti);
            bool tabHover = CheckCollisionPointRec(GetMousePosition(), tab);
            Color tc = activeTab ? ac : (Color){75,105,135,200};
            DrawRectangleRounded(tab, 0.28f, 6,
                                 activeTab ? (Color){13,40,54,235}
                                           : (Color){12,13,25,220});
            DrawRectangleRoundedLines(tab, 0.28f, 6,
                                      (Color){tc.r,tc.g,tc.b,
                                              (uchar)(activeTab ? 220 : (tabHover ? 150 : 80))});
            int tw = MeasureText(tabLabels[ti], 12);
            DrawText(tabLabels[ti], tx+tabW/2-tw/2, tabY+8, 12,
                     activeTab ? (Color){215,250,255,255}
                               : (Color){130,155,180,210});
            if (tabHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                musicShopContext = ti;
        }

        int cardGap = 14;
        int columns = cardCount > 5 ? 5 : cardCount;
        int rows = (cardCount + columns - 1) / columns;
        int cardW = (contentW - cardGap*(columns-1))/columns;
        int cardY = 169;
        int cardBottom = sh - 142;  // leave a clean breathing space above BACK
        int cardH = (cardBottom - cardY - cardGap*(rows-1))/rows;
        if (cardH > 360 && rows == 1) cardH = 360;
        if (cardH < 180) cardH = 180;
        unsigned int *activeTracks = (musicShopContext == 0)
                                       ? &music_menu_tracks : &music_gameplay_tracks;
        const int shopOrder[MUSIC_TRACK_COUNT + 1] = {
            0, MUSIC_DEFAULT_TRACK, 1, 2, 3, 5, 6,
            MUSIC_NEON_PULSE_TRACK, MUSIC_OVERACHIEVER_TRACK, -1
        };
        for (int ci = 0; ci < cardCount; ci++) {
            int track = shopOrder[ci];
            int col = ci % columns;
            int row = ci / columns;
            int cardX = cx + col*(cardW+cardGap);
            int cardTop = cardY + row*(cardH+cardGap);
            unsigned int activeMask = *activeTracks;
            bool unlocked = track < 0 || IsMusicTrackUnlocked(track);
            MusicAccess access = track < 0 ? MUSIC_ACCESS_INCLUDED
                                           : GetMusicTrackAccess(track);
            int price = track < 0 ? 0 : GetMusicTrackPrice(track);
            bool selected = (track < 0) ? (activeMask == 0u)
                                        : (unlocked && ((activeMask & (1u << track)) != 0u));
            Rectangle card = {(float)cardX,(float)cardTop,
                              (float)cardW,(float)cardH};
            if (DrawSoundtrackCard(card, track, selected,
                                   unlocked, access, price, t)) {
                if (track < 0) {
                    *activeTracks = 0u;
                    save_needed = true;
                } else if (unlocked) {
                    *activeTracks ^= (1u << track);
                    save_needed = true;
                } else if (access == MUSIC_ACCESS_PURCHASE &&
                           coin_balance >= price) {
                    coin_balance -= price;
                    music_owned[track] = 1;
                    *activeTracks |= (1u << track);
                    SaveStats();
                    SaveShop();
                    save_needed = true;
                }
            }
        }

        if (save_needed) SaveSettings();
        return;
    }

// Main Menu
    {
        float lf = 0.5f+0.5f*sinf(t*1.1f);
        DrawLine(0, sh/2-162, sw, sh/2-162,
                 (Color){(uchar)(40+30*lf),35,(uchar)(70+40*lf),60});
    }

    // ── TITLE ─────────────────────────────────────────────────────────────────
    {
        const char *TT = "DUNGEON CRAWLER";
        int ts = 68, ttw = MeasureText(TT, ts);
        int ty = 28;

        float gp = 0.5f+0.5f*sinf(t*1.1f);
        for (int g=7;g>=1;g--) {
            Color gc={(uchar)(50+30*gp),(uchar)(90+60*gp),255,(uchar)(8*g)};
            DrawText(TT, sw/2-ttw/2-g, ty-g, ts, gc);
            DrawText(TT, sw/2-ttw/2+g, ty+g, ts, gc);
        }
        DrawText(TT, sw/2-ttw/2+3, ty+4, ts, (Color){0,0,0,160});
        DrawText(TT, sw/2-ttw/2,   ty,   ts, (Color){230,225,250,255});

        int ulx1=sw/2-ttw/2-10, ulx2=sw/2+ttw/2+10;
        float ulf=0.5f+0.5f*sinf(t*2.0f);
        DrawLine(ulx1,   ty+ts+4,  ulx2,   ty+ts+4,
                 (Color){(uchar)(50+80*ulf),100,(uchar)(210+40*ulf),110});
        DrawLine(ulx1+30,ty+ts+9,  ulx2-30,ty+ts+9,
                 (Color){40,80,180,45});

        float pp=0.5f+0.5f*sinf(t*2.0f);
        float pp2=0.5f+0.5f*sinf(t*1.3f+1.2f);
        Color sc={(uchar)(200+50*pp),(uchar)(120+100*pp2),(uchar)(30+60*pp),255};
        const char *ST="PRESS  ENTER  TO  START";
        DrawText(ST, sw/2-MeasureText(ST,26)/2+1, ty+ts+18+1, 26, (Color){0,0,0,100});
        DrawText(ST, sw/2-MeasureText(ST,26)/2,   ty+ts+18,   26, sc);
    }

    // ── TOP-LEFT ICON BUTTONS ─────────────────────────────────────────────────
    {
        Rectangle settR, musicR, futR;
        GetMenuIconRects(&settR, &musicR, &futR);
        Vector2 mp2 = GetMousePosition();

        // Settings gear icon
        {
            bool hov = CheckCollisionPointRec(mp2, settR);
            Color ic  = hov ? (Color){230,220,255,255} : (Color){160,148,210,200};
            Color bgc = hov ? (Color){30,24,50,230}    : (Color){16,12,28,190};
            DrawRectangleRounded(settR, 0.18f, 8, bgc);
            DrawRectangleRoundedLines(settR, 0.18f, 8,
                hov ? (Color){190,170,255,210} : (Color){95,85,140,150});

            int gcx2 = (int)(settR.x + settR.width/2);
            int gcy2 = (int)(settR.y + settR.height/2);
            // 8 gear teeth (static)
            for (int k=0; k<8; k++) {
                float a = k * PI/4.0f;
                float tx3 = gcx2 + cosf(a)*21.0f;
                float ty4 = gcy2 + sinf(a)*21.0f;
                DrawRectanglePro((Rectangle){tx3, ty4, 11.0f, 8.0f},
                                 (Vector2){5.5f, 4.0f}, a*180.0f/PI, ic);
            }
            DrawCircle(gcx2, gcy2, 18, ic);
            DrawCircle(gcx2, gcy2, 13, bgc);
            DrawCircle(gcx2, gcy2,  7, ic);
            DrawCircle(gcx2, gcy2,  4, bgc);
        }

        // Music button: Beamed Gear 1:1 from Skin_Ideas/Icon-Ideas (menu-matched).
        {
            bool hov = CheckCollisionPointRec(mp2, musicR);
            DrawMenuMusicBeamedGear(musicR, t, hov);
        }

        // Future Updates rocket icon
        {
            bool hov = CheckCollisionPointRec(mp2, futR);
            Color ic  = hov ? (Color){225,175,255,255} : (Color){160,95,220,200};
            Color bgc = hov ? (Color){36,14,52,230}    : (Color){20,9,32,190};
            DrawRectangleRounded(futR, 0.18f, 8, bgc);
            DrawRectangleRoundedLines(futR, 0.18f, 8,
                hov ? (Color){210,130,255,210} : (Color){110,55,165,150});

            int rcx2 = (int)(futR.x + futR.width/2);
            int rcy2 = (int)(futR.y + futR.height/2);
            // Nose cone
            DrawTriangle((Vector2){(float)rcx2,      (float)(rcy2-26)},
                         (Vector2){(float)(rcx2-9),  (float)(rcy2-8)},
                         (Vector2){(float)(rcx2+9),  (float)(rcy2-8)}, ic);
            // Body
            DrawRectangle(rcx2-9, rcy2-8, 18, 22, ic);
            // Window porthole
            DrawCircle(rcx2, rcy2+2, 5, bgc);
            DrawCircleLines(rcx2, rcy2+2, 5, (Color){230,215,255,220});
            // Left fin
            DrawTriangle((Vector2){(float)(rcx2-9),  (float)(rcy2+8)},
                         (Vector2){(float)(rcx2-17), (float)(rcy2+14)},
                         (Vector2){(float)(rcx2-9),  (float)(rcy2+14)}, ic);
            // Right fin
            DrawTriangle((Vector2){(float)(rcx2+9),  (float)(rcy2+8)},
                         (Vector2){(float)(rcx2+9),  (float)(rcy2+14)},
                         (Vector2){(float)(rcx2+17), (float)(rcy2+14)}, ic);
            // Exhaust (static)
            DrawCircle(rcx2, rcy2+18, 6, (Color){255,140,50,200});
            DrawCircle(rcx2, rcy2+23, 4, (Color){255,240,100,210});
        }
    }

    // ── 4 NAV BUTTONS ────────────────────────────────────────────────────────
    int M = 14;                         
    int titleBottom = 152;              

    int cW = (int)(sw * 0.28f);        
               
    int midY      = (titleBottom + sh - M) / 2; 
    int cHtop     = midY - titleBottom - M/2;   
    int cHbot     = sh - M - (midY + M/2) - 1;  

    if (cHtop < 80)  cHtop = 80;
    if (cHbot < 80)  cHbot = 80;

    int topY = titleBottom;
    int botY = midY + M/2;

    int rightCardX = sw - M - cW;   // right cards flush with right margin
    Rectangle btnTL = { (float)M,           (float)topY, (float)cW, (float)cHtop };
    Rectangle btnTR = { (float)rightCardX,  (float)topY, (float)cW, (float)cHtop };
    Rectangle btnBL = { (float)M,           (float)botY, (float)cW, (float)cHbot };
    Rectangle btnBR = { (float)rightCardX,  (float)botY, (float)cW, (float)cHbot };

    DrawNavCard(btnTL, "TUTORIAL",       (Color){90,175,255,255}, t, 0.0f);
    DrawNavCard(btnTR, "STATS",          (Color){255,200,50,255}, t, 0.9f);
    DrawNavCard(btnBL, "SHOP",           (Color){65,210,130,255}, t, 1.8f);
    DrawNavCard(btnBR, "ACHIEVEMENTS", (Color){185,85,255,255}, t, 2.7f);

    // ── PLAY BUTTON – pure glowing text, title style ──────────────────────────
    {
        float cyc  = fmodf(t * 0.55f, 1.0f);
        Color cols[5] = { CTRES, CENE, CPORT, CWLK, CPL };
        float pos5 = cyc * 5.0f;
        int   idx0 = (int)pos5 % 5;
        int   idx1 = (idx0 + 1) % 5;
        float frac = pos5 - (float)(int)pos5;
        Color cA = cols[idx0], cB = cols[idx1];
        Color acPlay = {
            (uchar)(cA.r + (cB.r-cA.r)*frac),
            (uchar)(cA.g + (cB.g-cA.g)*frac),
            (uchar)(cA.b + (cB.b-cA.b)*frac), 255
        };

        int rightCardX2  = sw - M - cW;
        int centreColW = rightCardX2 - (7*M + cW);
        int pbW = centreColW - 16;
        int pbH = (int)(sh * 0.35f);
        int pbX = sw/2 - pbW/2;
        int pbY = midY - pbH/2 - 16;
        Rectangle pbRect = { (float)pbX,(float)pbY,(float)pbW,(float)pbH };

        Vector2 mp = GetMousePosition();
        bool hover = CheckCollisionPointRec(mp, pbRect);
        float gp2  = 0.5f + 0.5f*sinf(t*3.2f);

        int   playFS   = 72;
        float triH2    = playFS * 0.48f;
        int   playTW   = MeasureText("PLAY", playFS);
        float unitW    = triH2*2.0f + 18.0f + (float)playTW;
        float unitX    = sw/2.0f - unitW/2.0f;
        float unitCY   = (float)(midY); 

        float triCX = unitX + triH2;
        Vector2 tp0 = { triCX - triH2*0.65f, unitCY - triH2 };
        Vector2 tp1 = { triCX - triH2*0.65f, unitCY + triH2 };
        Vector2 tp2 = { triCX + triH2,        unitCY         };

        int plTx = (int)(triCX + triH2 + 18.0f);
        int plTy = (int)(unitCY - playFS/2);

        for (int g = 8; g >= 1; g--) {
            uchar al = hover ? (uchar)(18*g + (int)(8*g*gp2))
                             : (uchar)(10*g + (int)(4*g*gp2));
            Color gc = {acPlay.r, acPlay.g, acPlay.b, al};
            DrawText("PLAY", plTx - g, plTy - g, playFS, gc);
            DrawText("PLAY", plTx + g, plTy + g, playFS, gc);
            float hx = triCX + (tp2.x - triCX)*0.1f;
            DrawCircle((int)hx, (int)unitCY, triH2 + g*3, (Color){acPlay.r,acPlay.g,acPlay.b,(uchar)(al/3)});
        }

        DrawText("PLAY", plTx+3, plTy+4, playFS, (Color){0,0,0,160});

        Color triCol = hover ? (Color){255,255,255,255}
                             : (Color){acPlay.r,acPlay.g,acPlay.b,230};
        DrawTriangle(tp0, tp1, tp2, triCol);

        Color playCol = hover ? (Color){255,255,255,255}
                              : (Color){acPlay.r,acPlay.g,acPlay.b,235};
        DrawText("PLAY", plTx, plTy, playFS, playCol);

        float ulf2 = 0.5f + 0.5f*sinf(t*2.0f);
        int ulMid  = sw/2;
        int ulHalf = (int)(unitW/2) + 10;
        int ulY    = (int)(unitCY + triH2 + 12);
        DrawLine(ulMid - ulHalf,    ulY,   ulMid + ulHalf,    ulY,
                 (Color){acPlay.r, acPlay.g, acPlay.b,
                         hover ? (uchar)200 : (uchar)(80 + (int)(80*ulf2))});
        DrawLine(ulMid - ulHalf+30, ulY+5, ulMid + ulHalf-30, ulY+5,
                 (Color){acPlay.r, acPlay.g, acPlay.b, 35});
    }

    {
        int hy = midY + (int)(sh * 0.28f);
        const char *FT = "[F11]  Fullscreen";
        int fw = MeasureText(FT,18);
        DrawText(FT, sw/2-fw/2+1, hy+1, 18, (Color){0,0,0,80});
        DrawText(FT, sw/2-fw/2,   hy,   18, (Color){95,90,135,185});
    }

    {
        const char *ver = "v1.6";
        int vw2 = MeasureText(ver, 13);
        int vrx = sw - vw2 - 20, vry = 10;
        DrawRectangleRounded((Rectangle){(float)(vrx-6),(float)(vry-4),(float)(vw2+12),22},
                             0.4f, 6, (Color){18,14,30,210});
        DrawRectangleRoundedLines((Rectangle){(float)(vrx-6),(float)(vry-4),(float)(vw2+12),22},
                                  0.4f, 6, (Color){65,60,90,160});
        DrawText(ver, vrx, vry, 13, (Color){130,125,165,220});
    }
}

// ─── DRAW OVERLAY (death / next-round) ────────────────────────────────────────
void DrawOverlay(float t) {
    (void)t;
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, (Color){0,0,0,175});

    if (gs == GS_DEAD) {
        float pp = 0.5f + 0.5f*sinf(sttime * 5.0f);
        const char *M = "YOU  DIED";
        int ms = 64, mw2 = MeasureText(M, ms);
        DrawText(M, sw/2-mw2/2+2, sh/2-56+2, ms, (Color){0,0,0,140});
        DrawText(M, sw/2-mw2/2,   sh/2-56,   ms,
                 (Color){230,(uchar)(38+(int)(28*pp)),38,255});
        const char *S = TextFormat("Round %d   Coins %d   Treasures %d   Kills %d",
                                   roundn, totcoi, tottrs, kills_game);
        DrawText(S, sw/2-MeasureText(S,22)/2, sh/2+22, 22,
                 (Color){185,180,205,215});
        DrawText("ENTER to restart   |   Q for menu",
                 sw/2-MeasureText("ENTER to restart   |   Q for menu",20)/2,
                 sh/2+62, 20, (Color){140,135,165,195});

    } else if (gs == GS_NEXT) {
        const char *M = wasportal ? "PORTAL  USED !" : "TREASURE  FOUND !";
        Color mc     = wasportal ? CPORT : CTRES;
        int ms = 52, mw2 = MeasureText(M, ms);
        DrawText(M, sw/2-mw2/2+2, sh/2-48+2, ms, (Color){0,0,0,140});
        DrawText(M, sw/2-mw2/2,   sh/2-48,   ms, mc);

        const char *C = TextFormat("Coins : %d   Kills : %d", totcoi, kills_game);
        DrawText(C, sw/2-MeasureText(C,28)/2, sh/2+22, 28, CCOIN);
        DrawText("ENTER for next round",
                 sw/2-MeasureText("ENTER for next round",20)/2,
                 sh/2+64, 20, (Color){140,135,165,195});
    }
}
