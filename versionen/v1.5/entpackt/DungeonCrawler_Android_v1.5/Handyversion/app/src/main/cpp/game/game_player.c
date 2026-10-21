// game_player.c — item use, tile movement, collision, and player drawing.
#include "game_player.h"
#include "globals.h"
#include "constants.h"
#include "types.h"
#include "game_items.h"      // IK_COL
#include "game_particles.h"  // Burst
#include "game_map.h"        // WX, WY
#include "game_save.h"       // AddEnemyKill, UpdateAndSaveStats
#include "game_sfx.h"        // PlaySfx
#include "skins.h"           // GetSkinColor, DrawEpic*/DrawRare*/DrawSkull
#include <math.h>
#include <stdlib.h>          // abs

// ─── ITEM TUNING TABLES (indexed by upgrade level) ────────────────────────────
static const float SPD_DUR[] = { 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
static const float SHD_DUR[] = { 5.0f, 10.0f, 20.0f, 30.0f, 45.0f, -1.0f };
static const float WLK_DUR[] = { 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f };
static const int   SWD_RNG[] = { 1, 2, 3, 4 };
static const int   BMB_RNG[] = { 1, 2, 3, 4, 5, 6 };

// ─── USE ITEM ─────────────────────────────────────────────────────────────────
void UseItm(int slot) {
    if (slot >= pinvn || slot >= INV_SZ + 4) return;
    IKind k = pinv[slot];
    switch (k) {
        case IK_SPD:
            pspd = SPD_DUR[spd_level];
            Burst(WX(px), WY(py), IK_COL[IK_SPD], 16, 3);
            PlaySfx(SFX_SPEED);
            global_use_spd++;
            break;

        case IK_SHD: {
            float dur = SHD_DUR[shd_level];
            pshield_dur = (dur < 0) ? 1e9f : dur;
            pshield = true;
            Burst(WX(px), WY(py), CSHD, 18, 3);
            PlaySfx(SFX_SHIELD_ACTIVATE);
            global_use_shd++;
            break;
        }

        case IK_BMB: {
            int r = BMB_RNG[bmb_level];
            for (int dy=-r; dy<=r; dy++)
            for (int dx=-r; dx<=r; dx++) {
                int nx=px+dx, ny=py+dy;
                if (nx>=0&&ny>=0&&nx<mw&&ny<mh && map[ny][nx]=='+') {
                    map[ny][nx]='.';
                    Burst(WX(nx), WY(ny), IK_COL[IK_BMB], 8, 3);
                }
            }
            Burst(WX(px), WY(py), IK_COL[IK_BMB], 12, 4);
            PlaySfx(SFX_BOMB);
            global_use_bmb++;
            break;
        }

        case IK_WLK:
            pwlk = WLK_DUR[wlk_level];
            Burst(WX(px), WY(py), CWLK, 20, 3);
            PlaySfx(SFX_GHOSTWALK);
            global_use_wlk++;
            break;

        case IK_SWD: {
            int rng = SWD_RNG[swd_level];
            bool swd_killed = false;
            if (eneon) {
                for (int i = 0; i < MAX_ENE; i++) {
                    Ene *e = &enes[i]; if (!e->on) continue;
                    int ex = (int)(e->x+0.5f), ey = (int)(e->y+0.5f);
                    if (abs(ex-px) <= rng && abs(ey-py) <= rng) {
                        e->on = false;
                        AddEnemyKill();
                        global_kills_sword++;
                        swd_killed = true;
                        Burst(WX(ex), WY(ey), CENE, 20, 4);
                        Burst(WX(ex), WY(ey), CSWD, 12, 5);
                    }
                }
            }
            Burst(WX(px), WY(py), CSWD, 28, 5);
            PlaySfx(SFX_SWORD);
            if (swd_killed) PlaySfx(SFX_ENEMY_KILL);
            global_use_swd++;
            break;
        }

        default: break;
    }
    for (int i = slot; i < pinvn-1; i++) pinv[i] = pinv[i+1];
    pinvn--;
}

// ─── PLAYER MOVEMENT ──────────────────────────────────────────────────────────
void TryMove(int dx, int dy) {
    int nx = px+dx, ny = py+dy;
    if (nx<0||ny<0||nx>=mw||ny>=mh) return;
    char tg = map[ny][nx];

    if (tg=='#') {
        if (pwlk <= 0) return;
        px = nx; py = ny;
        Burst(WX(px), WY(py), CWLK, 4, 2);
        global_ghostwalk_walls++;
        return;
    }

    if (eneon) {
        for (int i = 0; i < MAX_ENE; i++) {
            Ene *e = &enes[i]; if (!e->on) continue;
            if ((int)(e->x+0.5f)==nx && (int)(e->y+0.5f)==ny) {
                if (pshield || pshield_perk) {
                    if (pshield) pshield = false;
                    else pshield_perk = false;
                    e->on   = false;
                    AddEnemyKill();
                    global_kills_shield++;
                    px = nx; py = ny;
                    Burst(WX(px), WY(py), CSHD, 24, 5);
                    Burst(WX(px), WY(py), CENE, 16, 3);
                    PlaySfx(SFX_SHIELD_TRIGGER);
                    PlaySfx(SFX_ENEMY_KILL);
                    return;
                } else {
                    px = nx; py = ny;
                    Burst(WX(px), WY(py), CPD,  32, 5);
                    Burst(WX(px), WY(py), CENE, 16, 3);
                    PlaySfx(SFX_DEATH_ENEMY);
                    global_deaths_enemy++;
                    pdead = true; gs = GS_DEAD; sttime = 0;
                    UpdateAndSaveStats();
                    return;
                }
            }
        }
    }

    px = nx; py = ny;

    // Trap
    if (tg=='+') {
        if (pshield || pshield_perk) {
            if (pshield) pshield = false;
            else pshield_perk = false;
            map[py][px] = '.';
            Burst(WX(px), WY(py), CSHD, 22, 4);
            PlaySfx(SFX_SHIELD_TRIGGER);
        } else {
            Burst(WX(px), WY(py), CPD, 32, 5);
            PlaySfx(SFX_DEATH_TRAP);
            global_deaths_trap++;
            pdead = true; gs = GS_DEAD; sttime = 0;
            UpdateAndSaveStats();
        }
        return;
    }
    // Treasure
    if (tg=='T') {
        Burst(WX(px), WY(py), CTRES, 32, 5);
        PlaySfx(SFX_TREASURE);
        tottrs++; global_tottrs++; wasportal = false; gs = GS_NEXT; sttime = 0;
        UpdateAndSaveStats();
        return;
    }
    // Coin
    if (tg=='$') {
        for (int i=0; i<MAX_COI; i++)
            if (coins[i].on && coins[i].x==px && coins[i].y==py)
                { coins[i].on=false; break; }
        map[py][px] = '.'; totcoi++;
        global_totcoi++;
        coin_balance++;
        coins_round++;
        Burst(WX(px), WY(py), CCOIN, 10, 2);
        PlaySfx(SFX_COIN_PICKUP);
        return;
    }
    // Item pickup
    if (tg=='I') {
        for (int i=0; i<MAX_ITM; i++) {
            if (itms[i].on && itms[i].x==px && itms[i].y==py) {
                if (pinvn < INV_SZ + perk_slots) {
                    IKind k = itms[i].k;
                    itms[i].on = false; map[py][px] = '.';
                    pinv[pinvn++] = k;
                    Burst(WX(px), WY(py), IK_COL[k], 16, 3);
                    PlaySfx(SFX_ITEM_PICKUP);
                } else {
                    global_inv_full++;
                }
                break;
            }
        }
        return;
    }
    // Portal
    if (tg=='O') {
        Burst(WX(px), WY(py), CPORT, 32, 5);
        PlaySfx(SFX_PORTAL);
        global_portals++;
        wasportal = true; gs = GS_NEXT; sttime = 0;
        UpdateAndSaveStats();
        return;
    }
}

// ─── DRAW PLAYER ──────────────────────────────────────────────────────────────
// Active-effect auras, equipped-skin back/face layers, then eyes/highlight.
void DrawPlayer(float t) {
    int cx = px*CELL + CELL/2;
    int cy = py*CELL + CELL/2;
    Color pc = pdead ? CPD : GetSkinColor(skin_equipped);

    if (pwlk > 0) {
        float pp = 0.5f + 0.5f*sinf(t*6.0f);
        DrawCircle(cx, cy, CELL/2+8, (Color){180,80,255,(int)(30+30*pp)});
        DrawCircleLines(cx, cy, CELL/2+8, (Color){180,80,255,(int)(120+80*pp)});
    }
    if (pshield) {
        float pp = 0.5f + 0.5f*sinf(t*5.0f);
        DrawCircle(cx, cy, CELL/2+6, (Color){80,150,255,(int)(38+28*pp)});
        DrawCircleLines(cx, cy, CELL/2+6,(Color){80,150,255,(int)(155+65*pp)});
    }
    if (pshield_perk) {
        float pp = 0.5f + 0.5f*sinf(t*3.5f);
        int ar = CELL/2+3;
        DrawCircle(cx, cy, ar, (Color){40,100,220,(int)(18+14*pp)});
        DrawCircleLines(cx, cy, ar, (Color){100,180,255,(int)(90+50*pp)});
        for (int s = 0; s < 4; s++) {
            float a0 = (float)s * 1.5708f + t*1.2f;
            float a1 = a0 + 0.55f;
            Vector2 v0 = { cx + cosf(a0)*(ar-2), cy + sinf(a0)*(ar-2) };
            Vector2 v1 = { cx + cosf(a1)*(ar-2), cy + sinf(a1)*(ar-2) };
            Vector2 v2 = { cx + cosf(a0)*(ar+2), cy + sinf(a0)*(ar+2) };
            Vector2 v3 = { cx + cosf(a1)*(ar+2), cy + sinf(a1)*(ar+2) };
            DrawTriangle(v0, v1, v2, (Color){120,200,255,(int)(60+40*pp)});
            DrawTriangle(v1, v3, v2, (Color){120,200,255,(int)(60+40*pp)});
        }
    }
    if (pspd > 0) {
        DrawCircle(cx, cy, CELL/2+3, (Color){80,220,80,28});
    }
    bool isEpicSkin      = (skin_equipped >= 27 && skin_equipped <= 29) || (skin_equipped >= 32 && skin_equipped <= 33);
    bool isRareSkin      = (skin_equipped >= 22 && skin_equipped <= 26) || (skin_equipped >= 30 && skin_equipped <= 31);
    bool isLegendarySkin = (skin_equipped >= 34 && skin_equipped <= 35);

    if (isEpicSkin)
        DrawEpicBack(skin_equipped, (float)cx, (float)cy, (float)(CELL/2-4), t);
    else if (isRareSkin)
        DrawRareBack(skin_equipped, (float)cx, (float)cy, (float)(CELL/2-4), t);
    else if (isLegendarySkin)
        DrawLegendaryBack(skin_equipped, (float)cx, (float)cy, (float)(CELL/2-4), t);

    if (skin_equipped == 28) {
        DrawSkull((float)cx, (float)cy, (float)(CELL/2-4), t);
    } else {
        DrawCircle(cx, cy, CELL/2-4, pc);

        if (skin_equipped == 27 || skin_equipped == 29 || skin_equipped == 32 || skin_equipped == 33) {
            DrawEpicFace(skin_equipped, (float)cx, (float)cy, (float)(CELL/2-4), t);
        } else if (isRareSkin) {
            DrawCircle(cx-2, cy-3, CELL/5,
                       (Color){CLAMP255(pc.r+55),CLAMP255(pc.g+55),CLAMP255(pc.b+55),90});
            DrawRareFace(skin_equipped, (float)cx, (float)cy, (float)(CELL/2-4), t);
        } else if (isLegendarySkin) {
            DrawCircle(cx-2, cy-3, CELL/5,
                       (Color){CLAMP255(pc.r+55),CLAMP255(pc.g+55),CLAMP255(pc.b+55),90});
            DrawLegendaryFace(skin_equipped, (float)cx, (float)cy, (float)(CELL/2-4), t);
        } else {
            DrawCircle(cx-2, cy-3, CELL/4,
                       (Color){CLAMP255(pc.r+55),CLAMP255(pc.g+55),CLAMP255(pc.b+55),
                               (uchar)(pc.a < 100 ? pc.a/2 : 120)});
            uchar eyeA = (pc.a < 100) ? pc.a : 255;
            DrawCircle(cx-4, cy-2, 2, (Color){10,10,22,eyeA});
            DrawCircle(cx+4, cy-2, 2, (Color){10,10,22,eyeA});
        }
    }
}
