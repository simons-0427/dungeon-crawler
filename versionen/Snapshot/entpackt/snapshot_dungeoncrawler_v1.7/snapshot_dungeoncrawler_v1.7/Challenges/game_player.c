// game_player.c — item use, tile-by-tile movement/collision, and player drawing.
#include "game_player.h"
#include "globals.h"
#include "constants.h"
#include "types.h"
#include "game_items.h"      // IK_COL
#include "game_particles.h"  // Burst
#include "game_map.h"        // WX, WY
#include "game_save.h"       // AddEnemyKill, UpdateAndSaveStats
#include "game_sfx.h"        // PlaySfx
#include "game_boss.h"       // BossCovers, BossInRange, HitBoss
#include "game_utils.h"      // AddScreenShake
#include "game_challenges.h"
#include "skins.h"           // Skin rendering helpers
#include <math.h>
#include <stdlib.h>          // abs

// ─── ITEM TUNING TABLES (indexed by upgrade level) ────────────────────────────
static const float SPD_DUR[] = { 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
static const float SHD_DUR[] = { 5.0f, 10.0f, 20.0f, 30.0f, 45.0f, -1.0f };
static const float WLK_DUR[] = { 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f };
static const float GLD_DUR[] = { 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f }; // L0=3s … L5=8s
static const int   SWD_RNG[] = { 1, 2, 3, 4 };
static const int   BMB_RNG[] = { 1, 2, 3, 4, 5, 6 };

static int ClampTier(int lv, int maxLv) {
    if (lv < 0) return 0;
    if (lv > maxLv) return maxLv;
    return lv;
}

float SpeedEffectMax(void)     { return SPD_DUR[ClampTier(spd_level, 5)]; }
float ShieldEffectMax(void)    { return SHD_DUR[ClampTier(shd_level, 5)]; }
float GhostwalkEffectMax(void) { return WLK_DUR[ClampTier(wlk_level, 5)]; }
float GoldTouchEffectMax(void) { return GLD_DUR[ClampTier(gld_level, 5)]; }

// Gold Touch: convert ONE enemy / trap on contact (flat coins, no CR x2).
void GoldTouchHitEnemy(Ene *e) {
    if (pgold <= 0 || !e || !e->on) return;
    int ex = (int)(e->x + 0.5f), ey = (int)(e->y + 0.5f);
    e->on = false;
    AddEnemyKill();
    totcoi += 10; StatsAdd(&global_totcoi, 10); coin_balance += 10; coins_round += 10;
    Burst(WX(ex), WY(ey), CCOIN, 14, 3);
    Burst(WX(ex), WY(ey), IK_COL[IK_GLD], 10, 4);
    PlaySfx(SFX_COIN_PICKUP);
}

bool GoldTouchHitTrap(int tx, int ty) {
    if (pgold <= 0) return false;
    if (tx < 0 || ty < 0 || tx >= mw || ty >= mh) return false;
    if (map[ty][tx] != '+') return false;
    map[ty][tx] = '.';
    totcoi += 5; StatsAdd(&global_totcoi, 5); coin_balance += 5; coins_round += 5;
    Burst(WX(tx), WY(ty), CCOIN, 8, 2);
    Burst(WX(tx), WY(ty), IK_COL[IK_GLD], 6, 3);
    PlaySfx(SFX_COIN_PICKUP);
    return true;
}

// ─── USE ITEM ─────────────────────────────────────────────────────────────────
void UseItm(int slot) {
    if (slot >= pinvn || slot >= INV_SZ + 4) return;
    IKind k = pinv[slot];
    switch (k) {
        case IK_SPD:
            pspd = SPD_DUR[spd_level];
            Burst(WX(px), WY(py), IK_COL[IK_SPD], 16, 3);
            PlaySfx(SFX_SPEED);
            StatsInc(&global_use_spd);
            break;

        case IK_SHD: {
            float dur = SHD_DUR[shd_level];
            pshield_dur = (dur < 0) ? 1e9f : dur;
            pshield = true;
            Burst(WX(px), WY(py), CSHD, 18, 3);
            PlaySfx(SFX_SHIELD_ACTIVATE);
            StatsInc(&global_use_shd);
            break;
        }

        case IK_BMB: {
            int r = BMB_RNG[bmb_level];
            for (int dy=-r; dy<=r; dy++)
            for (int dx=-r; dx<=r; dx++) {
                int nx=px+dx, ny=py+dy;
                if (nx<0||ny<0||nx>=mw||ny>=mh) continue;
                char c = map[ny][nx];
                if (c=='+' || c=='r') {   // static spikes AND arrow-lane tiles
                    map[ny][nx]='.';
                    Burst(WX(nx), WY(ny), IK_COL[IK_BMB], 8, 3);
                }
            }
            // Deliberately no boss damage: bombs are utility (trap clearing).
            // Hurting the boss is the job of sword, plates and cannons.
            Burst(WX(px), WY(py), IK_COL[IK_BMB], 12, 4);
            PlaySfx(SFX_BOMB);
            AddScreenShake(10.0f, 0.22f);
            StatsInc(&global_use_bmb);
            break;
        }

        case IK_WLK:
            pwlk = WLK_DUR[wlk_level];
            Burst(WX(px), WY(py), CWLK, 20, 3);
            PlaySfx(SFX_GHOSTWALK);
            StatsInc(&global_use_wlk);
            break;

        case IK_SWD: {
            int rng = SWD_RNG[swd_level];
            if (eneon) {
                for (int i = 0; i < MAX_ENE; i++) {
                    Ene *e = &enes[i]; if (!e->on) continue;
                    int ex = (int)(e->x+0.5f), ey = (int)(e->y+0.5f);
                    if (abs(ex-px) <= rng && abs(ey-py) <= rng) {
                        e->on = false;
                        AddEnemyKill();
                        StatsInc(&global_kills_sword);
                        Burst(WX(ex), WY(ey), CENE, 20, 4);
                        Burst(WX(ex), WY(ey), CSWD, 12, 5);
                    }
                }
            }
            if (roomKind == RK_BOSS && BossInRange(rng))
                HitBoss(1, 0, true);   // 1 damage + retreat teleport
            Burst(WX(px), WY(py), CSWD, 28, 5);
            PlaySfx(SFX_SWORD);
            AddScreenShake(5.0f, 0.12f);
            StatsInc(&global_use_swd);
            break;
        }

        case IK_GLD: {
            // Activate midas form only — enemies/traps convert on contact while active.
            int lv = gld_level;
            if (lv < 0) lv = 0;
            if (lv > 5) lv = 5;
            pgold = GLD_DUR[lv];
            Burst(WX(px), WY(py), CCOIN, 22, 4);
            Burst(WX(px), WY(py), IK_COL[IK_GLD], 28, 5);
            PlaySfx(SFX_SPEED);
            StatsInc(&global_use_gld);
            break;
        }

        default: break;
    }
    for (int i = slot; i < pinvn-1; i++) pinv[i] = pinv[i+1];
    pinvn--;
    if (inv_sel > slot) inv_sel--;
    if (pinvn <= 0) inv_sel = -1;
    else if (inv_sel >= pinvn) inv_sel = pinvn - 1;
}

// Horizontal facing is intentionally preserved during vertical movement so
// directional skins do not flip merely because the player walks up or down.
static bool playerFacingLeft = false;

// ─── PLAYER MOVEMENT ──────────────────────────────────────────────────────────
void TryMove(int dx, int dy) {
    if (dx < 0) playerFacingLeft = true;
    else if (dx > 0) playerFacingLeft = false;

    int nx = px+dx, ny = py+dy;
    if (nx<0||ny<0||nx>=mw||ny>=mh) return;
    char tg = map[ny][nx];
    if(tg==' ') return; // No floor outside an annex, including during Ghost Walk.

    if (tg=='G') {
        if (!OpenSideRoomGateAt(nx,ny)) return;
        tg='o';
        Burst(WX(nx),WY(ny),(Color){242,197,105,255},16,3);
        PlaySfx(SFX_ITEM_PICKUP);
    }

    if(tg=='Q') {
        BeginLockedObjectOpeningAt(nx,ny);
        return;
    }

    if (tg=='#' || tg=='D') {          // walls and arrow dispensers are solid
        if (pwlk <= 0) return;
        px = nx; py = ny;
        Burst(WX(px), WY(py), CWLK, 4, 2);
        StatsInc(&global_ghostwalk_walls);
        return;
    }

    if (ChallengeRunActive() && tg=='g') {
        if (!ChallengeHasKey()) return;
        map[ny][nx]='.';
        Burst(WX(nx),WY(ny),(Color){242,197,105,255},16,3);
        PlaySfx(SFX_ITEM_PICKUP);
    }
    // Boss body: shield saves (and hurts the boss), otherwise lethal.
    if (roomKind == RK_BOSS && BossCovers(nx, ny)) {
        if (pshield || pshield_perk) {
            if (pshield) pshield = false;
            else pshield_perk = false;
            Burst(WX(nx), WY(ny), CSHD, 24, 5);
            PlaySfx(SFX_SHIELD_TRIGGER);
            HitBoss(1, 2, false);       // boss survives and is pushed back
        } else {
            px = nx; py = ny;
            Burst(WX(px), WY(py), CPD,   32, 5);
            Burst(WX(px), WY(py), CBOSS, 16, 3);
            PlaySfx(SFX_DEATH_ENEMY);
            AddScreenShake(12.0f, 0.28f);
            StatsInc(&global_deaths_boss);
            pdead = true; gs = GS_DEAD; sttime = 0;
            UpdateAndSaveStats();
        }
        return;
    }

    if (eneon) {
        for (int i = 0; i < MAX_ENE; i++) {
            Ene *e = &enes[i]; if (!e->on) continue;
            if ((int)(e->x+0.5f)==nx && (int)(e->y+0.5f)==ny) {
                if (pgold > 0) {
                    px = nx; py = ny;
                    GoldTouchHitEnemy(e);
                    return;
                }
                if (pshield || pshield_perk) {
                    if (pshield) pshield = false;
                    else pshield_perk = false;
                    e->on   = false;
                    AddEnemyKill();
                    StatsInc(&global_kills_shield);
                    px = nx; py = ny;
                    Burst(WX(px), WY(py), CSHD, 24, 5);
                    Burst(WX(px), WY(py), CENE, 16, 3);
                    PlaySfx(SFX_SHIELD_TRIGGER);
                    return;
                } else {
                    px = nx; py = ny;
                    Burst(WX(px), WY(py), CPD,  32, 5);
                    Burst(WX(px), WY(py), CENE, 16, 3);
                    PlaySfx(SFX_DEATH_ENEMY);
                    AddScreenShake(12.0f, 0.28f);
                    StatsInc(&global_deaths_enemy);
                    e->on = false;   // only the killer disappears
                    pdead = true; gs = GS_DEAD; sttime = 0;
                    UpdateAndSaveStats();
                    return;
                }
            }
        }
    }

    px = nx; py = ny;

    int universalKeyId=UniversalKeyIdFromTile(tg);
    if (tg=='k'||universalKeyId>=0) {
        int keyId=tg=='k'?KEY_TUTORIAL_BOOK:universalKeyId;
        CollectSideRoomKeyAt(px,py);
        CollectUniversalKey(keyId);
        if(ChallengeRunActive())ChallengeCollectKey(keyId);
        map[py][px]='.';
        Color keyColor=keyId==KEY_RIFT_THORN?(Color){173,81,249,255}:
                       keyId==KEY_BONE_RELIQUARY?(Color){232,218,180,255}:
                       keyId==KEY_STARFORGE?(Color){255,206,116,255}:
                       keyId==KEY_SUNBREAKER?(Color){255,199,78,255}:
                       (Color){252,215,132,255};
        Burst(WX(px),WY(py),keyColor,20,3);
        PlaySfx(SFX_KEY_PICKUP);
        return;
    }
    if (tg=='h') {
        map[py][px] = 'H';
        pfall = TRAPDOOR_FALL_TIME;
        Burst(WX(px), WY(py), (Color){30, 20, 40, 255}, 18, 3);
        Burst(WX(px), WY(py), (Color){8, 6, 12, 255}, 10, 2);
        PlaySfx(SFX_DEATH_TRAP);
        return;
    }

    // Lava: lethal — a shield saves you once and keeps the tile dry.
    if (tg=='L') {
        if (pshield || pshield_perk) {
            if (pshield) pshield = false;
            else pshield_perk = false;
            map[py][px] = '.';
            Burst(WX(px), WY(py), CSHD, 22, 4);
            PlaySfx(SFX_SHIELD_TRIGGER);
        } else {
            Burst(WX(px), WY(py), CFIRE, 32, 5);
            PlaySfx(SFX_DEATH_TRAP);
            AddScreenShake(12.0f, 0.28f);
            StatsInc(&global_deaths_lava);
            pdead = true; gs = GS_DEAD; sttime = 0;
            UpdateAndSaveStats();
        }
        return;
    }
    // Arrow lane: only lethal during a volley; safe floor between volleys.
    if (tg=='r') {
        if (!ArrowLaneLethal()) return;       // arrows at rest — walk on through
        if (pshield || pshield_perk) {
            if (pshield) pshield = false;
            else pshield_perk = false;
            map[py][px] = '.';
            Burst(WX(px), WY(py), CSHD, 22, 4);
            PlaySfx(SFX_SHIELD_TRIGGER);
        } else {
            Burst(WX(px), WY(py), CPD, 32, 5);
            PlaySfx(SFX_DEATH_TRAP);
            AddScreenShake(12.0f, 0.28f);
            StatsInc(&global_deaths_trap);
            pdead = true; gs = GS_DEAD; sttime = 0;
            UpdateAndSaveStats();
        }
        return;
    }
    // Trap — Gold Touch converts on contact (no death).
    if (tg=='+') {
        if (pgold > 0) {
            GoldTouchHitTrap(px, py);
            return;
        }
        if (pshield || pshield_perk) {
            if (pshield) pshield = false;
            else pshield_perk = false;
            map[py][px] = '.';
            Burst(WX(px), WY(py), CSHD, 22, 4);
            PlaySfx(SFX_SHIELD_TRIGGER);
        } else {
            Burst(WX(px), WY(py), CPD, 32, 5);
            PlaySfx(SFX_DEATH_TRAP);
            AddScreenShake(12.0f, 0.28f);
            StatsInc(&global_deaths_trap);
            pdead = true; gs = GS_DEAD; sttime = 0;
            UpdateAndSaveStats();
        }
        return;
    }
    // Spike plate: stepping on it arms it against the boss.
    if (tg=='^') {
        map[py][px] = '*';
        Burst(WX(px), WY(py), CPLATE, 14, 3);
        PlaySfx(SFX_ITEM_PICKUP);
        return;
    }
    if (tg=='l') {
        if(!CanEnterArtifactVaultRoom())return;
        PlaySfx(SFX_PORTAL);
        EnterArtifactVaultRoom();
        return;
    }
    if(tg=='A') { CollectArtifact();return; }
    if(tg=='%') { CollectCoinPile();return; }
    if(tg=='M') {
        PlaySfx(SFX_PORTAL);StatsInc(&global_portals);
        roundn++;StatsInc(&global_rounds);
        EnterArtifactRoom();UpdateAndSaveStats();return;
    }
    // Treasure
    if (tg=='T') {
        Burst(WX(px), WY(py), CTRES, 32, 5);
        PlaySfx(SFX_TREASURE);
        tottrs++; StatsInc(&global_tottrs); wasportal = false; gs = GS_NEXT; sttime = 0;
        if      (roomKind == RK_TRAP)      StatsInc(&global_rooms_trap);
        else if (roomKind == RK_DARK)      StatsInc(&global_rooms_dark);
        else if (roomKind == RK_LAVA)      StatsInc(&global_rooms_lava);
        else if (roomKind == RK_COIN_RUSH) StatsInc(&global_rooms_coinrush);
        UpdateAndSaveStats();
        return;
    }
    // Coin (Coin Rush pays double — two coins shown side-by-side)
    if (tg=='$') {
        for (int i=0; i<MAX_COI; i++)
            if (coins[i].on && coins[i].x==px && coins[i].y==py)
                { coins[i].on=false; break; }
        int val = coinRush ? 2 : 1;
        map[py][px] = '.';
        totcoi += val; StatsAdd(&global_totcoi, val); coin_balance += val; coins_round += val;
        Burst(WX(px), WY(py), CCOIN, coinRush ? 14 : 10, 2);
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
                    if (mouse_item_select && inv_sel < 0) inv_sel = pinvn - 1;
                    Burst(WX(px), WY(py), IK_COL[k], 16, 3);
                    PlaySfx(SFX_ITEM_PICKUP);
                } else {
                    StatsInc(&global_inv_full);
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
        StatsInc(&global_portals);
        wasportal = true; gs = GS_NEXT; sttime = 0;
        if      (roomKind == RK_TRAP)      StatsInc(&global_rooms_trap);
        else if (roomKind == RK_DARK)      StatsInc(&global_rooms_dark);
        else if (roomKind == RK_LAVA)      StatsInc(&global_rooms_lava);
        else if (roomKind == RK_COIN_RUSH) StatsInc(&global_rooms_coinrush);
        UpdateAndSaveStats();
        return;
    }
}

// ─── STANDING-STILL HAZARDS ───────────────────────────────────────────────────
// An arrow volley sweeping over you while you stand in a lane is lethal too —
// that's the timing challenge: don't loiter in a lane when it fires.
void UpdPlayerHazards(void) {
    if (gs != GS_PLAY) return;
    char c = map[py][px];
    if (c=='r' && ArrowLaneLethal()) {
        if (pshield || pshield_perk) {
            if (pshield) pshield = false;
            else pshield_perk = false;
            map[py][px] = '.';
            Burst(WX(px), WY(py), CSHD, 22, 4);
            PlaySfx(SFX_SHIELD_TRIGGER);
        } else {
            Burst(WX(px), WY(py), CPD, 32, 5);
            PlaySfx(SFX_DEATH_TRAP);
            StatsInc(&global_deaths_trap);
            pdead = true; gs = GS_DEAD; sttime = 0;
            UpdateAndSaveStats();
        }
    }
}

// ─── TRAPDOOR FALL ────────────────────────────────────────────────────────────
// Ticks the short fall animation; on expiry, counts a trapdoor death.
void UpdPlayerFall(float dt) {
    if (pfall <= 0 || gs != GS_PLAY) return;
    pfall -= dt;
    if (pfall > 0) return;
    pfall = 0;
    StatsInc(&global_deaths_trapdoor);
    AddScreenShake(10.0f, 0.22f);
    pdead = true; gs = GS_DEAD; sttime = 0;
    UpdateAndSaveStats();
}

// ─── DRAW PLAYER ──────────────────────────────────────────────────────────────
// Active-effect auras, equipped-skin back/face layers, then eyes/highlight.
// Gold Touch overrides the body with a solid midas-gold look while active.
// Death always draws the Angel silhouette (wings/halo/face) in CPD red —
// skin_equipped is never changed, only the visual for this frame.
// Trapdoor deaths hide the player completely once the fall finishes (in the pit).
// Other deaths: gentle upward drift for DEATH_FLOAT_TIME using sttime.
void DrawPlayer(float t) {
    int cx = px*CELL + CELL/2, cy = py*CELL + CELL/2;
    // Visual-only skin: Angel (29) while dead, otherwise equipped.
    const int ANGEL_SKIN = 29;
    int drawSkin = pdead ? ANGEL_SKIN : skin_equipped;
    Color pc = pdead ? CPD : GetSkinColor(drawSkin);

    // Trapdoor fall: shrink + drop into the open hatch (≤ TRAPDOOR_FALL_TIME).
    if (pfall > 0) {
        float prog = 1.0f - (pfall / TRAPDOOR_FALL_TIME);   // 0 → 1
        if (prog < 0.0f) prog = 0.0f;
        if (prog > 1.0f) prog = 1.0f;
        float scale = 1.0f - 0.85f * prog;
        int drop = (int)(prog * (CELL * 0.55f));
        int r = (int)((CELL/2 - 4) * scale);
        if (r < 1) r = 1;
        uchar a = (uchar)(255 * (1.0f - prog * 0.85f));
        int dy = cy + drop;
        DrawCircle(cx, dy, r + 2, (Color){0, 0, 0, (uchar)(80 + 100*prog)});
        DrawCircle(cx, dy, r, (Color){pc.r, pc.g, pc.b, a});
        if (r >= 3) {
            DrawCircle(cx - r/3, dy - r/3, r/3, (Color){255, 255, 255, (uchar)(a/3)});
            DrawCircle(cx - r/2, dy - r/4, 1, (Color){10, 10, 22, a});
            DrawCircle(cx + r/2, dy - r/4, 1, (Color){10, 10, 22, a});
        }
        return;
    }

    // After a trapdoor death the body is gone — open hatch only, no angel corpse.
    if (pdead && map[py][px] == 'H')
        return;

    // Soft rise on normal deaths (sttime starts at 0 when GS_DEAD begins).
    if (pdead) {
        float ft = sttime;
        if (ft > DEATH_FLOAT_TIME) ft = DEATH_FLOAT_TIME;
        // Ease-out so the lift feels gentle, then holds at the peak.
        float u = ft / DEATH_FLOAT_TIME;
        float ease = 1.0f - (1.0f - u) * (1.0f - u);
        cy -= (int)(ease * DEATH_FLOAT_PX);
    }

    // Skip buff auras on the corpse — death pose is Angel-only.
    if (!pdead) {
        if (pwlk > 0) {
            float pp = photosensitivity ? 0.5f : (0.5f + 0.5f*sinf(t*6.0f));
            DrawCircle(cx, cy, CELL/2+8, (Color){180,80,255,(int)(30+30*pp)});
            DrawCircleLines(cx, cy, CELL/2+8, (Color){180,80,255,(int)(120+80*pp)});
        }
        if (pshield) {
            float pp = photosensitivity ? 0.5f : (0.5f + 0.5f*sinf(t*5.0f));
            DrawCircle(cx, cy, CELL/2+6, (Color){80,150,255,(int)(38+28*pp)});
            DrawCircleLines(cx, cy, CELL/2+6,(Color){80,150,255,(int)(155+65*pp)});
        }
        if (pshield_perk) {
            float pp = photosensitivity ? 0.5f : (0.5f + 0.5f*sinf(t*3.5f));
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
    }

    // ── Gold Touch: totally golden midas form (no regular skin shows through) ─
    if (pgold > 0 && !pdead) {
        float pp  = photosensitivity ? 0.4f : (0.5f + 0.5f * sinf(t * 9.0f));
        float pp2 = photosensitivity ? 0.4f : (0.5f + 0.5f * sinf(t * 5.5f + 1.1f));
        float pp3 = photosensitivity ? 0.4f : (0.5f + 0.5f * sinf(t * 3.2f + 2.0f));
        int R = CELL / 2 - 2;

        // Outer molten-gold blaze (static + dimmer when photosensitivity is on)
        if (!photosensitivity) BeginBlendMode(BLEND_ADDITIVE);
        DrawCircle(cx, cy, (int)(R + 18 + 5 * pp),  (Color){255, 160,  0, (uchar)(photosensitivity ? 18 : (35 + 30 * pp))});
        DrawCircle(cx, cy, (int)(R + 12 + 4 * pp2), (Color){255, 200, 40, (uchar)(photosensitivity ? 24 : (55 + 40 * pp2))});
        DrawCircle(cx, cy, (int)(R +  7 + 3 * pp3), (Color){255, 240,100, (uchar)(photosensitivity ? 28 : (70 + 50 * pp3))});
        if (!photosensitivity) EndBlendMode();

        // Solid gold body — layered pure gold, no dark rim
        DrawCircle(cx, cy, R + 3, (Color){255, 190,  20, 255});
        DrawCircle(cx, cy, R + 1, (Color){255, 215,  40, 255});
        DrawCircle(cx, cy, R,     (Color){255, 230,  70, 255});
        // Hot highlight plate
        DrawCircle(cx - 3, cy - 4, (int)(R * 0.55f),
                   (Color){255, 250, 180, (uchar)(200 + 40 * pp)});
        DrawCircle(cx - 4, cy - 5, (int)(R * 0.28f),
                   (Color){255, 255, 230, (uchar)(220 + 30 * pp2)});

        if (!photosensitivity) {
            // Orbiting gold flakes
            for (int k = 0; k < 12; k++) {
                float a = t * 4.0f + k * (2.0f * 3.14159f / 12.0f);
                float dist = R + 5.0f + 4.0f * sinf(t * 7.0f + k * 0.9f);
                int sz = 1 + (k % 3);
                DrawCircle((int)(cx + cosf(a) * dist),
                           (int)(cy + sinf(a) * dist * 0.85f),
                           sz, (Color){255, 245, 120, (uchar)(200 + 50 * pp)});
            }
            // Rising glitter
            for (int k = 0; k < 8; k++) {
                float p = fmodf(t * 1.4f + k * 0.13f, 1.0f);
                float gx = cx - R + (k * 7 % (R * 2));
                float gy = cy + R - p * (R * 2.4f);
                DrawCircle((int)gx, (int)gy, 2,
                           (Color){255, 255, 200, (uchar)(230 * sinf(p * 3.14159f))});
            }
        }

        // Golden eyes (bright gold, not dark)
        DrawCircle(cx - 4, cy - 2, 3, (Color){255, 240, 100, 255});
        DrawCircle(cx + 4, cy - 2, 3, (Color){255, 240, 100, 255});
        DrawCircle(cx - 4, cy - 2, 1, (Color){255, 255, 255, 255});
        DrawCircle(cx + 4, cy - 2, 1, (Color){255, 255, 255, 255});
        return;
    }

    // These Skin-Ideas designs are complete compositions, not a generic body
    // plus face/back layers. Draw them directly so the transfer stays 1:1.
    if (drawSkin == 33) {
        DrawApexBladelord((float)cx, (float)cy, (float)(CELL/2-4), t);
        return;
    }
    if (drawSkin == 34) {
        DrawConstellation((float)cx, (float)cy, (float)(CELL/2-4), t);
        return;
    }
    if (drawSkin == 36) {
        DrawPoptartCatInGame((float)cx, (float)cy, (float)(CELL/2-4), t, playerFacingLeft);
        return;
    }
    if (drawSkin == 37) {
        DrawApexRexInGame((float)cx, (float)cy, (float)(CELL/2-4), t, playerFacingLeft);
        return;
    }
    if (drawSkin == 38) {
        DrawSkullvoidCrowned((float)cx, (float)cy, (float)(CELL/2-4), t);
        return;
    }
    if (drawSkin == 39) {
        DrawBloodscaleVanguard((float)cx, (float)cy, (float)((CELL/2-4) * 1.10f), t);
        return;
    }
    if (drawSkin == 40) {
        DrawStormforgedAegis((float)cx, (float)cy, (float)((CELL/2-4) * 1.10f), t);
        return;
    }
    if (drawSkin == 41) {
        DrawSunforgedCrusader((float)cx, (float)cy, (float)((CELL/2-4) * 1.10f), t);
        return;
    }
    if (drawSkin == 42) {
        DrawBlackHole((float)cx, (float)cy, (float)(CELL/2-4), t);
        return;
    }

    bool isEpicSkin      = (drawSkin == 27 || drawSkin == 29 || drawSkin == 32 || drawSkin == 38 ||
                            drawSkin == 39 || drawSkin == 40);
    bool isRareSkin      = ((drawSkin >= 22 && drawSkin <= 26) || drawSkin == 28 ||
                            drawSkin == 30 || drawSkin == 31 || drawSkin == 33 || drawSkin == 35);
    bool isLegendarySkin = (drawSkin == 36 || drawSkin == 37 || drawSkin == 41);

    if (isEpicSkin)
        DrawEpicBack(drawSkin, (float)cx, (float)cy, (float)(CELL/2-4), t);
    else if (isRareSkin)
        DrawRareBack(drawSkin, (float)cx, (float)cy, (float)(CELL/2-4), t);
    else if (isLegendarySkin)
        DrawLegendaryBack(drawSkin, (float)cx, (float)cy, (float)(CELL/2-4), t);

    if (drawSkin == 28) {
        DrawSkull((float)cx, (float)cy, (float)(CELL/2-4), t);
    } else {
        DrawCircle(cx, cy, CELL/2-4, pc);

        if (drawSkin == 27 || drawSkin == 29 || drawSkin == 32) {
            DrawEpicFace(drawSkin, (float)cx, (float)cy, (float)(CELL/2-4), t);
        } else if (isRareSkin) {
            DrawCircle(cx-2, cy-3, CELL/5,
                       (Color){CLAMP255(pc.r+55),CLAMP255(pc.g+55),CLAMP255(pc.b+55),90});
            DrawRareFace(drawSkin, (float)cx, (float)cy, (float)(CELL/2-4), t);
        } else if (isLegendarySkin) {
            DrawCircle(cx-2, cy-3, CELL/5,
                       (Color){CLAMP255(pc.r+55),CLAMP255(pc.g+55),CLAMP255(pc.b+55),90});
            DrawLegendaryFace(drawSkin, (float)cx, (float)cy, (float)(CELL/2-4), t);
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
