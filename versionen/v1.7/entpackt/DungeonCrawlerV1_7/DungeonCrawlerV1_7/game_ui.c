// game_ui.c â€” HUD, pause menu, the full main menu with all sub-screens
// (tutorial / stats / shop / achievements / future-updates / settings) and the
// death / next-round overlay.
#include "game_ui.h"
#include "globals.h"
#include "constants.h"
#include "types.h"
#include "game_map.h"
#include "game_items.h"      // IK_NAME, IK_COL
#include "skins.h"           // GetSkinColor, DrawRare*/DrawEpic*/DrawSkull
#include "game_save.h"       // SaveStats
#include "game_shop.h"       // SaveShop
#include "game_utils.h"      // DrawCustomStar(Lines), PointInRect
#include "game_settings.h"   // SaveSettings (called on settings change)
#include "advancements.h"    // BuildAchievementProgress, ACH_COUNT
#include "game_console_win32.h"
#include "game_audio.h"      // GetNowPlayingName / GetNowPlayingAlpha
#include "game_boss.h"       // BossDisplayName (boss-room banner)
#include "game_entities.h"   // Azure Wisp room preview
#include "game_player.h"     // SpeedEffectMax / ShieldEffectMax / ...
#include "game_challenges.h" // ChallengeDrawMenu
#include "game_sfx.h"        // PlaySfx
#include "menu_icons.h"
#include <math.h>
#include <string.h>          // strchr, strncpy
#include <stdio.h>           // snprintf

static unsigned StatsToggleGroup(const char *label) {
    if(strcmp(label,"Total Games Played")==0)return 1u;
    if(strcmp(label,"Total Rounds Played")==0)return 2u;
    if(strcmp(label,"Total Coins Collected")==0)return 4u;
    if(strcmp(label,"Total Enemy Kills")==0)return 8u;
    if(strcmp(label,"Locks Opened by Key")==0)return 16u;
    return 0u;
}

static unsigned StatsDetailGroup(const char *label) {
    if(strcmp(label,"Played in Main Game")==0||strcmp(label,"Played in Challenges")==0||
       strcmp(label,"Played in Seeded Runs")==0)return 1u;
    if(strcmp(label,"Rounds in Main Game")==0||strcmp(label,"Rounds in Challenges")==0||
       strcmp(label,"Rounds in Seeded Runs")==0)return 2u;
    if(strcmp(label,"Coins in Main Game")==0||strcmp(label,"Coins from Achievements")==0||
       strcmp(label,"Coins in Challenges")==0||strcmp(label,"Coins in Seeded Runs")==0)return 4u;
    if(strcmp(label,"Kills in Main Game")==0||strcmp(label,"Kills in Challenges")==0||
       strcmp(label,"Kills in Seeded Runs")==0)return 8u;
    for(int i=0;i<KEY_COUNT;i++)
        if(strcmp(label,UniversalKeyDisplayName(i))==0)return 16u;
    return 0u;
}

// â”€â”€â”€ ACHIEVEMENT COIN REWARDS (indexed by tier 0-5) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
static const int ACH_REWARDS[6] = { 50, 100, 250, 500, 1000, 3000 };

// â”€â”€â”€ UPGRADE COSTS (indexed by current level; used by the SHOP upgrades tab) â”€â”€
static const int SPD_COST[] = {  200,  250,  350, 500, 600 };
static const int SHD_COST[] = {  250,  400,  600, 750, 1000};
static const int WLK_COST[] = {  500,  750,  1000, 1250, 1500};
static const int SWD_COST[] = {  500,  1000, 1500 };
static const int BMB_COST[] = {  250,  500, 750, 1000, 1250};
static const int GLD_COST[] = {  300,  450,  650, 900, 1200};

// Shop tab selector (0=skins, 1=upgrades, 2=perks). Local to the menu;
// menuSub / statScroll / upgScroll are shared with main.c via globals.h.
static int shopTab = 0;
static int musicShopContext = 0;  // 0 = home menu, 1 = gameplay
static int musicPreviewScrubTrack = -1;
static float tutBookK = 0.0f;
static bool tutBookWant = false;
#define TUTORIAL_PAGE_COUNT 20
#define TUTORIAL_SPREAD_COUNT (TUTORIAL_PAGE_COUNT / 2)
static int tutBookPage = 0;       // current two-page spread
static bool tutBookRear = false;  // closed cover: false = front, true = back
static bool tutContentsOpen = false;
static float tutContentsK = 0.0f;
static float tutContentsScroll = 0.0f;
static bool tutContentsDrag = false;
static float tutContentsDragOffset = 0.0f;

void TutorialBookReset(void) {
    tutBookK = 0.0f;
    tutBookWant = false;
    tutBookPage = 0;
    tutBookRear = false;
    tutContentsOpen = false;
    tutContentsK = 0.0f;
    tutContentsScroll = 0.0f;
    tutContentsDrag = false;
    tutContentsDragOffset = 0.0f;
}
static int subNavFocus = 0;       // 0 = BACK, 1+ = submenu tabs
static int capturingAction = -1;  // ACT_* while waiting for a key, else -1
static int capturingSlot = -1;    // bind index being rewritten, -1 = add
static int settingsScroll = 0;
static int settingsMaxScroll = 0;
static bool pauseSettingsOpen = false;
static bool pauseLeaveConfirm = false;
static bool menuNavOn = false;
static int  menuNavFocus = MN_PLAY;
static float menuEnter = -1.0f;
static bool  menuEnterFire = false;
#ifdef DEBUG_ROOMS
static int debugKeyPreviewId=-1;
#endif
#ifndef DEBUG_ROOMS
static char bindWarn[96];
static float bindWarnUntil = 0;
static bool bindDeleteMode = false;
#endif

// Internal forward declarations (helpers defined further down in this file).
static void DrawPauseButton(void);
static int DrawEffectBar(int x, int y, const char *label, Color col,
                         float remain, float maxDur, bool infinite);
static void DrawMenuBG(float t, int sw, int sh, Rectangle punch);
static void DrawCornerAccents(Rectangle r, Color c, int len);
static bool DrawNavCard(Rectangle r, const char *label, Color accent, int icon, float t, float phase, bool focused);
static Rectangle DrawSubPanel(const char *title, Color accent, float t, int sw, int sh);
// Music menu icon â€” 1:1 Icon-Ideas "Beamed Gear" (chunky beamed notes + gear lavender).
static void DrawMenuMusicBeamedGear(Rectangle bounds, float t, bool hover);
static void CheckAchievementSkinUnlocks(void);
static void DrawSettingsPanel(float t, bool fromPause);
static void DrawSettingsGearIcon(Rectangle settR, float t, bool hov, bool focused);
static void DrawMenuFocusRing(Rectangle r, Color ac, float t);

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  The remainder of this file is sliced verbatim from the original monolith:
//  pause/HUD layout, HUD, pause menu, menu-background helpers, and the large
//  DrawMenu / DrawOverlay routines.
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•

// â”€â”€ pause / HUD layout â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
void GetPauseMenuLayout(Rectangle *panel, Rectangle *resumeBtn, Rectangle *settingsBtn, Rectangle *menuBtn) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int pw = 460, ph = 272;
    int px2 = sw / 2 - pw / 2;
    int py2 = sh / 2 - ph / 2;

    if (panel)       *panel       = (Rectangle){ (float)px2, (float)py2, (float)pw, (float)ph };
    if (resumeBtn)   *resumeBtn   = (Rectangle){ (float)(px2 + pw/2 - 120), (float)(py2 + 104), 240.0f, 36.0f };
    if (settingsBtn) *settingsBtn = (Rectangle){ (float)(px2 + pw/2 - 120), (float)(py2 + 150), 240.0f, 36.0f };
    if (menuBtn)     *menuBtn     = (Rectangle){ (float)(px2 + pw/2 - 120), (float)(py2 + 196), 240.0f, 36.0f };
}

void GetPauseLeaveConfirmLayout(Rectangle *panel, Rectangle *confirmBtn, Rectangle *cancelBtn) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int pw = 460, ph = 228;
    int px2 = sw / 2 - pw / 2;
    int py2 = sh / 2 - ph / 2;
    float bw = 180.0f, bh = 40.0f, gap = 20.0f;
    float total = bw * 2.0f + gap;
    float bx = (float)px2 + (float)pw / 2.0f - total / 2.0f;
    float by = (float)py2 + 152.0f;

    if (panel)      *panel      = (Rectangle){ (float)px2, (float)py2, (float)pw, (float)ph };
    if (cancelBtn)  *cancelBtn  = (Rectangle){ bx, by, bw, bh };
    if (confirmBtn) *confirmBtn = (Rectangle){ bx + bw + gap, by, bw, bh };
}

Rectangle GetPauseButtonRect(void) {
    int sw = GetScreenWidth();
    return (Rectangle){ (float)(sw - 14 - 44), 15.0f, 44.0f, 44.0f };
}

int GetInventorySlotCount(void) {
    return INV_SZ + perk_slots;
}

Rectangle GetInventorySlotRect(int slot) {
    int n = GetInventorySlotCount();
    int sx = GetScreenWidth() - 14 - 64 - n * 68;
    return (Rectangle){ (float)(sx + slot * 68), 9.0f, 62.0f, 56.0f };
}

bool PauseSettingsOpen(void) { return pauseSettingsOpen; }
void PauseSettingsSet(bool open) {
    pauseSettingsOpen = open;
    if (open) {
        pauseLeaveConfirm = false;
        settingsScroll = 0;
    } else {
        capturingAction = -1;
        capturingSlot = -1;
#ifndef DEBUG_ROOMS
        bindDeleteMode = false;
#endif
    }
}

bool PauseLeaveConfirmOpen(void) { return pauseLeaveConfirm; }
void PauseLeaveConfirmSet(bool open) {
    pauseLeaveConfirm = open;
    if (open) pauseSettingsOpen = false;
}

bool SettingsCapturingKey(void) { return capturingAction >= 0; }
void SettingsCancelCapture(void) { capturingAction = -1; capturingSlot = -1; }
bool SettingsDeleteMode(void) {
#ifndef DEBUG_ROOMS
    return bindDeleteMode;
#else
    return false;
#endif
}
void SettingsCancelDelete(void) {
#ifndef DEBUG_ROOMS
    bindDeleteMode = false;
#endif
}

// â”€â”€â”€ DRAW HUD â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
static void DrawPauseButton(void);
static int DrawEffectBar(int x, int y, const char *label, Color col,
                         float remain, float maxDur, bool infinite) {
    const int bw = 176, bh = 18;
    DrawRectangleRounded((Rectangle){(float)x,(float)y,(float)bw,(float)bh},
                         0.35f, 6, (Color){12, 10, 20, (uchar)(220*hud_opacity)});
    DrawRectangleRoundedLines((Rectangle){(float)x,(float)y,(float)bw,(float)bh},
                         0.35f, 6, (Color){col.r, col.g, col.b, 100});
    float frac = infinite ? 1.0f : (maxDur > 0.05f ? remain / maxDur : 0.0f);
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    int fw = (int)((bw - 4) * frac);
    if (fw > 2)
        DrawRectangleRounded((Rectangle){(float)(x+2),(float)(y+2),(float)fw,(float)(bh-4)},
                             0.4f, 6, (Color){col.r, col.g, col.b, 165});
    uchar ta = (uchar)(255 * hud_opacity);
    DrawText(label, x+9, y+4, 12, (Color){0, 0, 0, (uchar)(200 * hud_opacity)});
    DrawText(label, x+8, y+3, 12, (Color){255, 255, 255, ta});
    if (!infinite) {
        const char *ts = TextFormat("%.1f", remain);
        int tx = x+bw-8-MeasureText(ts, 12);
        DrawText(ts, tx+1, y+4, 12, (Color){0,0,0,(uchar)(180*hud_opacity)});
        DrawText(ts, tx, y+3, 12, (Color){245,245,250,ta});
    }
    return bh + 4;
}
void DrawHUD(float t) {
#ifdef DEBUG_ROOMS
    if(IsKeyPressed(KEY_F10))debugKeyPreviewId=(debugKeyPreviewId+1)%KEY_COUNT;
#endif
    (void)t;
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, 76, (Color){8,6,14,(uchar)(235*hud_opacity)});
    DrawRectangle(0, 75, sw, 2, (Color){55,50,72,(uchar)(200*hud_opacity)});

    if(show_round_time||show_game_time){
        char timerText[96]={0};
        int len=0;
        if(show_round_time){
            int sec=(int)StatsCurrentRoundSeconds();
            len=snprintf(timerText,sizeof(timerText),"ROUND %02d:%02d",sec/60,sec%60);
        }
        if(show_game_time){
            int sec=(int)StatsCurrentRunSeconds();
            snprintf(timerText+(len>0?len:0),sizeof(timerText)-(size_t)(len>0?len:0),
                     "%sGAME %02d:%02d:%02d",len>0?"   ":"",sec/3600,(sec/60)%60,sec%60);
        }
        int tw=MeasureText(timerText,14);
        DrawRectangleRounded((Rectangle){10,81,(float)(tw+20),24},0.3f,6,
                             (Color){8,6,14,(uchar)(225*hud_opacity)});
        DrawRectangleRoundedLines((Rectangle){10,81,(float)(tw+20),24},0.3f,6,
                                  (Color){80,75,110,(uchar)(180*hud_opacity)});
        DrawText(timerText,20,86,14,(Color){205,195,235,255});
    }

    DrawText(ChallengeRunActive()?
             TextFormat("LEVEL %d/%d",roundn,CHALLENGE_FLOORS):
             TextFormat("Round  %d",roundn),
             16,8,26,(Color){210,205,230,255});

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
        else if (roomKind == RK_ARTIFACT) { rl = "ARTIFACT ROOM"; rlc = (Color){255,0,220,255}; }
        else if (eneon)                          { rl = "ENEMIES ON"; }
        else                                     { rl = "ENEMIES OFF";       rlc = (Color){80,80,100,130}; }
        DrawText(rl, sw/2 - MeasureText(rl,16)/2, 10, 16, rlc);
        if (show_seed) {
            const char *st = TextFormat("SEED: %04X-%04X",
                                        (run_seed >> 16) & 0xFFFFu, run_seed & 0xFFFFu);
            DrawText(st, sw/2 - MeasureText(st, 13)/2, 44, 13, (Color){155,145,185,210});
        }
    }

    if(ChallengeRunActive()){
        DrawLine(14,38,520,38,(Color){45,40,62,180});
    } else {
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
    }

    // â”€â”€ Item inventory slots â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    int itemsLeft=sw-14-64-GetInventorySlotCount()*68;
    int keyX=itemsLeft-30;
    for(int keyId=0;keyId<KEY_COUNT;keyId++) {
        int count=UniversalKeyCount(keyId);
        if(count>0&&keyX>420) {
            DrawUniversalKeyIcon(keyId,keyX,47,24,t);
            if(count>1)
                DrawText(TextFormat("%d",count),keyX+18,63,10,(Color){255,245,220,255});
            keyX-=28;
        }
    }
#ifdef DEBUG_ROOMS
    if(debugKeyPreviewId>=0&&keyX>420) {
        DrawUniversalKeyIcon(debugKeyPreviewId,keyX,47,24,t);
        keyX-=28;
    }
#endif

    int eff_slots = INV_SZ + perk_slots;
    int sx = sw - 14 - 64 - eff_slots * 68;
    DrawText("ITEMS", sx-70, 20, 15, (Color){130,125,160,190});
    for (int i = 0; i < eff_slots; i++) {
        int bx = sx + i*68, by = 9;
        bool has = (i < pinvn);
        bool isGld = has && pinv[i] == IK_GLD;

        if (isGld) {
            // Pulsing gold glow that spills past the slot â€” no metal sheen.
            float pulse = photosensitivity
                ? 0.45f
                : (0.5f + 0.5f * sinf((float)GetTime() * 4.0f + (float)i * 0.7f));
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

            // Slot body â€” flat warm gold, clean
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
            {
                char slotK[16];
                FormatKeyName(keybinds[ACT_USE_1 + i][0], slotK, (int)sizeof(slotK));
                DrawText(TextFormat("[%s]", slotK[0] ? slotK : "?"), bx + 6, by + 36, 12,
                         (Color){200, 160, 60, (uchar)(180 + 50 * p)});
            }
        } else {
            DrawRectangleRounded((Rectangle){(float)bx, (float)by, 62, 56}, 0.3f, 8,
                                 has ? (Color){35, 30, 52, 210} : (Color){20, 18, 32, 120});
            Color bc = has ? IK_COL[pinv[i]] : (Color){50, 46, 68, 140};
            DrawRectangleRoundedLines((Rectangle){(float)bx, (float)by, 62, 56}, 0.3f, 8, bc);
            if (has) {
                DrawText(IK_NAME[pinv[i]], bx + 6, by + 5, 14, IK_COL[pinv[i]]);
                char slotK[16];
                FormatKeyName(keybinds[ACT_USE_1 + i][0], slotK, (int)sizeof(slotK));
                DrawText(TextFormat("[%s]", slotK[0] ? slotK : "?"), bx + 6, by + 36, 12,
                         (Color){130, 125, 155, 200});
            } else {
                char slotK[16];
                FormatKeyName(keybinds[ACT_USE_1 + i][0], slotK, (int)sizeof(slotK));
                DrawText(TextFormat("[%s]", slotK[0] ? slotK : "?"), bx + 20, by + 22, 13,
                         (Color){65, 60, 85, 140});
            }
        }
        if (mouse_item_select && has && i == inv_sel) {
            DrawRectangleRoundedLines(
                (Rectangle){(float)(bx - 3), (float)(by - 3), 68, 62},
                0.32f, 8, (Color){255, 250, 220, 230});
            DrawRectangleRoundedLines(
                (Rectangle){(float)bx, (float)by, 62, 56},
                0.3f, 8, (Color){255, 255, 255, 200});
        }
    }

    // â”€â”€ Active effects (bottom-left duration bars) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        const int row = 22;
        int n = 0;
        if (pspd > 0) n++;
        if (pshield) n++;
        if (pshield_perk) n++;
        if (pwlk > 0) n++;
        if (pgold > 0) n++;
        int y = sh - 14 - n * row;
        int x = 14;
        if (pspd > 0)
            y += DrawEffectBar(x, y, "SPEED", IK_COL[IK_SPD],
                               pspd, SpeedEffectMax(), false);
        if (pshield) {
            bool inf = (pshield_dur >= 1e8f) || (ShieldEffectMax() < 0.0f);
            y += DrawEffectBar(x, y, "SHIELD", CSHD,
                               pshield_dur, ShieldEffectMax(), inf);
        }
        if (pshield_perk)
            y += DrawEffectBar(x, y, "PERK SHIELD",
                               (Color){100,180,255,255}, 1.0f, 1.0f, true);
        if (pwlk > 0)
            y += DrawEffectBar(x, y, "GHOSTWALK", CWLK,
                               pwlk, GhostwalkEffectMax(), false);
        if (pgold > 0)
            y += DrawEffectBar(x, y, "GOLD TOUCH", IK_COL[IK_GLD],
                               pgold, GoldTouchEffectMax(), false);
        (void)y;
    }

    DrawPauseButton();

    {
        char moveH[32], useH[32], selH[40], pauseH[24];
        FormatMoveHint(moveH, (int)sizeof(moveH));
        FormatUseSlotHint(useH, (int)sizeof(useH));
        FormatActionBinds(ACT_USE_SELECTED, selH, (int)sizeof(selH));
        FormatActionBinds(ACT_PAUSE, pauseH, (int)sizeof(pauseH));
        const char *help = mouse_item_select
            ? TextFormat("%s:Move   Click/Wheel:Select   %s:Use   %s:Slot   %s:Pause",
                         moveH, selH, useH, pauseH)
            : TextFormat("%s:Move   %s:Use   %s:Pause", moveH, useH, pauseH);
        int helpFs = 14;
        int helpX = sw - MeasureText(help, helpFs) - 14;
        if (helpX < 210) { helpFs = 12; helpX = sw - MeasureText(help, helpFs) - 14; }
        DrawText(help, helpX, sh-22, helpFs, (Color){80,75,100,160});
    }

    // â”€â”€ Round-start banner: announce special room types â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
        } else if (roomKind == RK_ARTIFACT) {
            bn="ARTIFACT ROOM"; sub="Find the Artifact, then the Treasure.";
            bc=(Color){255,0,220,255};
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

    // â”€â”€ Now Playing banner â€” below HUD bar, fades in then out â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
    if (pauseSettingsOpen) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 160});
        DrawSettingsPanel((float)GetTime(), true);
        return;
    }

    Rectangle panel, resumeBtn, settingsBtn, menuBtn;
    GetPauseMenuLayout(&panel, &resumeBtn, &settingsBtn, &menuBtn);

    DrawRectangleRounded(panel, 0.08f, 8, (Color){10, 8, 18, 240});
    DrawRectangleRoundedLines(panel, 0.08f, 8, CW);

    const char *TT = "PAUSED";
    int ttw = MeasureText(TT, 36);
    DrawText(TT, (int)(panel.x + panel.width/2 - ttw/2), (int)(panel.y + 20),
             36, (Color){215, 210, 235, 255});
    
    char pauseB[24];
    FormatActionBinds(ACT_PAUSE, pauseB, (int)sizeof(pauseB));
    const char *ST = TextFormat("%s or the pause icon resumes the game", pauseB);
    int stw = MeasureText(ST, 16);
    DrawText(ST, (int)(panel.x + panel.width/2 - stw/2), (int)(panel.y + 68),
             16, (Color){160, 155, 185, 255});

    DrawLine((int)panel.x + 16, (int)panel.y + 50, (int)panel.x + (int)panel.width - 16,
             (int)panel.y + 50, (Color){65, 58, 88, 180});

    bool hoverResume   = PointInRect(GetMousePosition(), resumeBtn);
    bool hoverSettings = PointInRect(GetMousePosition(), settingsBtn);
    bool hoverMenu     = PointInRect(GetMousePosition(), menuBtn);

    Color rb = hoverResume   ? (Color){CPL.r/6, CPL.g/3, CPL.b/6, 255} : (Color){20, 40, 25, 240};
    Color sb = hoverSettings ? (Color){40, 35, 70, 255} : (Color){24, 20, 42, 240};
    Color mb = hoverMenu     ? (Color){CPD.r/3, CPD.g/7, CPD.b/7, 255} : (Color){45, 20, 18, 240};
    Color rlc = hoverResume   ? CPL : (Color){40, 150, 50, 255};
    Color slc = hoverSettings ? (Color){180, 160, 255, 255} : (Color){110, 95, 180, 255};
    Color mlc = hoverMenu     ? CPD : (Color){150, 40, 40, 255};

    DrawRectangleRounded(resumeBtn, 0.25f, 8, rb);
    DrawRectangleRoundedLines(resumeBtn, 0.25f, 8, rlc);
    DrawRectangleRounded(settingsBtn, 0.25f, 8, sb);
    DrawRectangleRoundedLines(settingsBtn, 0.25f, 8, slc);
    DrawRectangleRounded(menuBtn,   0.25f, 8, mb);
    DrawRectangleRoundedLines(menuBtn,   0.25f, 8, mlc);

    const char *resumeTxt = "RESUME";
    const char *setTxt = "SETTINGS";
    const char *menuTxt = "MENU";
    DrawText(resumeTxt,
             (int)(resumeBtn.x + resumeBtn.width/2 - MeasureText(resumeTxt, 20)/2),
             (int)(resumeBtn.y + 8), 20, (Color){230, 235, 240, 255});
    DrawText(setTxt,
             (int)(settingsBtn.x + settingsBtn.width/2 - MeasureText(setTxt, 20)/2),
             (int)(settingsBtn.y + 8), 20, (Color){230, 235, 240, 255});
    DrawText(menuTxt,
             (int)(menuBtn.x + menuBtn.width/2 - MeasureText(menuTxt, 20)/2),
             (int)(menuBtn.y + 8), 20, (Color){230, 235, 240, 255});

    if (!pauseLeaveConfirm) return;

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 140});
    Rectangle cPanel, confirmBtn, cancelBtn;
    GetPauseLeaveConfirmLayout(&cPanel, &confirmBtn, &cancelBtn);
    DrawRectangleRounded(cPanel, 0.08f, 8, (Color){12, 8, 16, 250});
    DrawRectangleRoundedLines(cPanel, 0.08f, 8, (Color){180, 60, 60, 220});

    const char *CT = "LEAVE THIS RUN?";
    int ctw = MeasureText(CT, 28);
    DrawText(CT, (int)(cPanel.x + cPanel.width/2 - ctw/2), (int)(cPanel.y + 28),
             28, (Color){240, 210, 210, 255});
    const char *CS = "Your current run will be lost.";
    int csw = MeasureText(CS, 18);
    DrawText(CS, (int)(cPanel.x + cPanel.width/2 - csw/2), (int)(cPanel.y + 78),
             18, (Color){175, 160, 175, 230});
    DrawLine((int)cPanel.x + 16, (int)(cPanel.y + 118),
             (int)(cPanel.x + cPanel.width - 16), (int)(cPanel.y + 118),
             (Color){90, 50, 55, 180});

    bool hoverCancel  = PointInRect(GetMousePosition(), cancelBtn);
    bool hoverConfirm = PointInRect(GetMousePosition(), confirmBtn);
    Color cab = hoverCancel  ? (Color){32, 30, 50, 255} : (Color){22, 20, 36, 240};
    Color cob = hoverConfirm ? (Color){CPD.r/3, CPD.g/7, CPD.b/7, 255} : (Color){45, 20, 18, 240};
    Color cal = hoverCancel  ? (Color){180, 170, 220, 255} : (Color){110, 100, 150, 220};
    Color col = hoverConfirm ? CPD : (Color){150, 40, 40, 255};
    DrawRectangleRounded(cancelBtn, 0.25f, 8, cab);
    DrawRectangleRoundedLines(cancelBtn, 0.25f, 8, cal);
    DrawRectangleRounded(confirmBtn, 0.25f, 8, cob);
    DrawRectangleRoundedLines(confirmBtn, 0.25f, 8, col);
    const char *cancelTxt = "CANCEL";
    const char *confirmTxt = "CONFIRM";
    DrawText(cancelTxt,
             (int)(cancelBtn.x + cancelBtn.width/2 - MeasureText(cancelTxt, 20)/2),
             (int)(cancelBtn.y + 10), 20, (Color){230, 235, 240, 255});
    DrawText(confirmTxt,
             (int)(confirmBtn.x + confirmBtn.width/2 - MeasureText(confirmTxt, 20)/2),
             (int)(confirmBtn.y + 10), 20, (Color){230, 235, 240, 255});
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

// â”€â”€â”€ DRAW MENU â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
static void DrawMenuBG(float t, int sw, int sh, Rectangle punch) {
    ClearBackground((Color){ 6, 4, 14, 255 });
    for (int y = 0; y <= sh/CELL; y++)
    for (int x = 0; x <= sw/CELL; x++) {
        Rectangle tile = { (float)(x*CELL), (float)(y*CELL), (float)(CELL-1), (float)(CELL-1) };
        if (punch.width > 1.0f && CheckCollisionRecs(tile, punch)) continue;
        float f = 0.22f + 0.14f*sinf(x*0.41f + y*0.53f + t*0.35f)
                        + 0.08f*sinf(x*0.17f - y*0.29f + t*0.22f);
        DrawRectangle(x*CELL, y*CELL, CELL-1, CELL-1,
                      (Color){(uchar)(12*f),(uchar)(9*f),(uchar)(20*f),255});
    }
    for (int i = 7; i >= 1; i--)
        DrawCircle(sw/2, sh/2, (float)(i*160),
                   (Color){6,4,14,(uchar)(4+i*6)});
    if (punch.width > 1.0f)
        DrawRectangleRec(punch, (Color){ 7, 5, 12, 255 });
}

static void DrawCornerAccents(Rectangle r, Color c, int len) {
    int x1=(int)r.x, y1=(int)r.y;
    int x2=(int)(r.x+r.width-1), y2=(int)(r.y+r.height-1);
    DrawLine(x1,y1,   x1+len,y1,   c); DrawLine(x1,y1, x1,y1+len,   c);
    DrawLine(x2,y1,   x2-len,y1,   c); DrawLine(x2,y1, x2,y1+len,   c);
    DrawLine(x1,y2,   x1+len,y2,   c); DrawLine(x1,y2, x1,y2-len,   c);
    DrawLine(x2,y2,   x2-len,y2,   c); DrawLine(x2,y2, x2,y2-len,   c);
}

static void DrawMenuFocusRing(Rectangle r, Color ac, float t) {
    float p = 0.5f + 0.5f * sinf(t * 4.2f);
    for (int g = 5; g >= 1; g--) {
        DrawRectangleRoundedLines(
            (Rectangle){ r.x - g * 2.2f, r.y - g * 2.2f,
                         r.width + g * 4.4f, r.height + g * 4.4f },
            0.12f, 8,
            (Color){ ac.r, ac.g, ac.b, (uchar)(14 * g + (int)(28 * p)) });
    }
    DrawCornerAccents(r, (Color){ ac.r, ac.g, ac.b, (uchar)(170 + 70 * p) }, 16);
}

static void DrawSettingsGearIcon(Rectangle settR, float t, bool hov, bool focused) {
    float s = settR.width / 76.0f;
    Color ic  = hov ? (Color){230,220,255,255} : (Color){160,148,210,200};
    Color bgc = hov ? (Color){30,24,50,230}    : (Color){16,12,28,190};
    DrawRectangleRounded(settR, 0.18f, 8, bgc);
    DrawRectangleRoundedLines(settR, 0.18f, 8,
        hov ? (Color){190,170,255,210} : (Color){95,85,140,150});

    int gcx2 = (int)(settR.x + settR.width/2);
    int gcy2 = (int)(settR.y + settR.height/2);
    for (int k=0; k<8; k++) {
        float a = k * PI/4.0f;
        float tx3 = gcx2 + cosf(a)*21.0f * s;
        float ty4 = gcy2 + sinf(a)*21.0f * s;
        DrawRectanglePro((Rectangle){tx3, ty4, 11.0f * s, 8.0f * s},
                         (Vector2){5.5f * s, 4.0f * s}, a*180.0f/PI, ic);
    }
    DrawCircle(gcx2, gcy2, 18.0f * s, ic);
    DrawCircle(gcx2, gcy2, 13.0f * s, bgc);
    DrawCircle(gcx2, gcy2,  7.0f * s, ic);
    DrawCircle(gcx2, gcy2,  4.0f * s, bgc);
    if (focused) DrawMenuFocusRing(settR, (Color){190,170,255,255}, t);
}

// â”€â”€ Music icon: 1:1 from Skin_Ideas/Icon-Ideas "Beamed Gear" â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

#include "menu_gate.inc.c"

static bool DrawNavCard(Rectangle r, const char *label, Color accent, int icon, float t, float phase, bool focused) {
    Vector2 mp  = GetMousePosition();
    bool hover  = CheckCollisionPointRec(mp, r) || focused;
    float gp    = 0.5f + 0.5f*sinf(t*1.6f + phase);

    int fs = (int)(r.height * 0.105f);
    if (fs < 16) fs = 16;
    if (fs > 34) fs = 34;
    int lw  = MeasureText(label, fs);
    float iconS = fminf(r.width * 0.42f, r.height * 0.46f);
    int lx  = (int)(r.x + r.width/2 - lw/2);
    int ly  = (int)(r.y + r.height * 0.72f - fs/2);

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

    {
        Rectangle ir = {
            r.x + (r.width - iconS) * 0.5f,
            r.y + r.height * 0.18f,
            iconS, iconS
        };
        if (r.height < 110.0f) {
            ir.height = fmaxf(28.0f, r.height * 0.38f);
            ir.width = ir.height;
            ir.x = r.x + 12;
            ir.y = r.y + (r.height - ir.height) * 0.5f;
            lx = (int)(ir.x + ir.width + 12);
            ly = (int)(r.y + r.height / 2 - fs / 2);
        }
        DrawMenuGlyph(icon, SelectedMenuIconVariant(icon),
                      ir.x + ir.width * 0.5f, ir.y + ir.height * 0.5f,
                      fminf(ir.width, ir.height) * 0.34f, accent, t);
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
    if (focused) DrawMenuFocusRing(r, accent, t);

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

    // â”€â”€ Red BACK button (bottom-left of panel) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    int bkW = 130, bkH = 40;
    int bkX = px2 + 20, bkY = py2 + ph - bkH - 16;
    Rectangle backR = { (float)bkX, (float)bkY, (float)bkW, (float)bkH };

    Vector2 mp = GetMousePosition();
    bool backFocus = menuNavOn && (menuSub != 0 || pauseSettingsOpen) && subNavFocus == 0;
    bool bhov = CheckCollisionPointRec(mp, backR) || backFocus;
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
    if (backFocus) DrawMenuFocusRing(backR, bkAccent, t);

    return backR;
}

void GetMenuIconRects(Rectangle *settingsR, Rectangle *musicR, Rectangle *futureR) {
    Rectangle all[MN_COUNT];
    GetMainMenuRects(all);
    if (settingsR) *settingsR = all[MN_SETTINGS];
    if (musicR)    *musicR    = all[MN_MUSIC];
    if (futureR)   *futureR   = all[MN_FUTURE];
}

void GetMainMenuRects(Rectangle out[MN_COUNT]) {
    MenuUI u = MakeUI();
    int sw = u.sw, sh = u.sh;
    LayoutTopChips(u, out);
    float chipRight = out[MN_FUTURE].x + out[MN_FUTURE].width;
    int M = MenuPx(u, 14);
    int titleBottom = TitleBottomY(u, chipRight);
    int cW = (int)(sw * 0.28f);
    int midY = (titleBottom + sh - M) / 2;
    int cHtop = midY - titleBottom - M / 2;
    int cHbot = sh - M - (midY + M / 2) - 1;
    int minH = MenuPx(u, 80);
    if (cHtop < minH) cHtop = minH;
    if (cHbot < minH) cHbot = minH;
    int topY = titleBottom;
    int botY = midY + M / 2;
    int rightCardX = sw - M - cW;
    out[MN_SHOP]    = (Rectangle){ (float)M, (float)topY, (float)cW, (float)cHtop };
    out[MN_ACHIEVE] = (Rectangle){ (float)rightCardX, (float)topY, (float)cW, (float)cHtop };
    out[MN_STATS]   = (Rectangle){ (float)M, (float)botY, (float)cW, (float)cHbot };
    out[MN_CHAL]    = (Rectangle){ (float)rightCardX, (float)botY, (float)cW, (float)cHbot };
    int gapL = M + cW;
    int midW = rightCardX - gapL;
    int doorW = (int)(midW * 0.86f);
    int doorH = sh - titleBottom - M - MenuPx(u, 4);
    int minDoorW = MenuPx(u, 140);
    int minDoorH = MenuPx(u, 200);
    if (doorW < minDoorW) doorW = minDoorW;
    if (doorH < minDoorH) doorH = minDoorH;
    out[MN_PLAY] = (Rectangle){
        (float)(sw / 2 - doorW / 2), (float)(titleBottom + MenuPx(u, 4)),
        (float)doorW, (float)doorH
    };
}

bool MenuNavIsOn(void) { return menuNavOn; }
int  MenuNavFocus(void) { return menuNavFocus; }
void MenuNavExit(void) { menuNavOn = false; }

static Vector2 menuNavMouse;
static bool menuNavLockMouse = false;

static int SubNavCount(void) {
    if (menuSub == 3) return 4;  // BACK + shop tabs
    if (menuSub == 7) return 3;  // BACK + soundtrack tabs
    if (menuSub == 8) return 1;  // BACK; challenge cards handle their own input
    if (menuSub == 0) return 0;
    return 1;                    // BACK
}

void UpdateMenuNav(void) {
    static int lastSub = -1;
    int navSub = pauseSettingsOpen ? 6 : menuSub;
    if (navSub != lastSub) { lastSub = navSub; subNavFocus = 0; }
    if (SettingsCapturingKey()) return;
    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    bool next = ActionPressed(ACT_MENU_NEXT);
    bool left = ActionPressed(ACT_MENU_LEFT);
    bool right = ActionPressed(ACT_MENU_RIGHT);
    bool up = ActionPressed(ACT_MENU_UP);
    bool down = ActionPressed(ACT_MENU_DOWN);

    if (menuSub != 0 || pauseSettingsOpen) {
        int n = pauseSettingsOpen ? 1 : SubNavCount();
        if (subNavFocus < 0 || subNavFocus >= n) subNavFocus = 0;
        if (next) {
            if (!menuNavOn) { menuNavOn = true; subNavFocus = 0; }
            else {
                subNavFocus += shift ? -1 : 1;
                if (subNavFocus < 0) subNavFocus = n - 1;
                if (subNavFocus >= n) subNavFocus = 0;
            }
            menuNavLockMouse = true;
            menuNavMouse = GetMousePosition();
            return;
        }
        if (!menuNavOn) return;
        int step = 0;
        if (left || up) step = -1;
        else if (right || down) step = 1;
        if (step) {
            subNavFocus += step;
            if (subNavFocus < 0) subNavFocus = n - 1;
            if (subNavFocus >= n) subNavFocus = 0;
            menuNavLockMouse = true;
            menuNavMouse = GetMousePosition();
        }
        return;
    }

    if (next) {
        if (!menuNavOn) {
            menuNavOn = true;
            menuNavFocus = MN_PLAY;
        } else {
            menuNavFocus += shift ? -1 : 1;
            if (menuNavFocus < 0) menuNavFocus = MN_COUNT - 1;
            if (menuNavFocus >= MN_COUNT) menuNavFocus = 0;
        }
        menuNavLockMouse = true;
        menuNavMouse = GetMousePosition();
        return;
    }
    if (!menuNavOn) return;
    int dir = -1;
    if      (left)  dir = 0;
    else if (right) dir = 1;
    else if (up)    dir = 2;
    else if (down)  dir = 3;
    if (dir < 0) return;
    static const int L[MN_COUNT] = { 0, 0, 3, 1, 4, 6, 6, 5, 5 };
    static const int R[MN_COUNT] = { 1, 3, 2, 2, 5, 7, 5, 7, 8 };
    static const int U[MN_COUNT] = { 0, 1, 2, 3, 6, 3, 0, 2, 7 };
    static const int D[MN_COUNT] = { 6, 5, 7, 5, 4, 8, 4, 8, 8 };
    const int *tbl = (dir == 0) ? L : (dir == 1) ? R : (dir == 2) ? U : D;
    menuNavFocus = tbl[menuNavFocus];
    menuNavLockMouse = true;
    menuNavMouse = GetMousePosition();
}

int MenuNavActivatePressed(void) {
    if (!menuNavOn) return -1;
    if (!(IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) ||
          ActionPressedKey(ACT_CONFIRM)))
        return -1;
    if (menuSub == 0 && !pauseSettingsOpen) return menuNavFocus;
    if (subNavFocus == 0) return -2;
    if (menuSub == 3 && subNavFocus >= 1 && subNavFocus <= 3) {
        shopTab = subNavFocus - 1;
        upgScroll = 0;
        return -1;
    }
    if (menuSub == 7 && subNavFocus >= 1 && subNavFocus <= 2) {
        musicShopContext = subNavFocus - 1;
        return -1;
    }
    return -1;
}

// â”€â”€â”€ SETTINGS UI HELPERS (private to the settings sub-screen) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

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

static void SettingsRowBg(int cx, int ry, int rw, int rh, Color line) {
    DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                          0.18f,6,(Color){16,13,26,180});
    DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                               0.18f,6, line);
}

#ifndef DEBUG_ROOMS
static const char *ACT_LABEL[ACT_COUNT] = {
    "MOVE UP", "MOVE DOWN", "MOVE LEFT", "MOVE RIGHT",
    "USE ITEM 1", "USE ITEM 2", "USE ITEM 3", "USE ITEM 4", "USE ITEM 5",
    "USE SELECTED", "PAUSE", "FULLSCREEN", "NEXT ROUND",
    "MENU NEXT", "MENU LEFT", "MENU RIGHT", "MENU UP", "MENU DOWN",
    "BACK TO MENU"
};

// Returns true if the chip was clicked this frame.
static bool DrawKeyChip(int x, int y, int h, const char *label, bool hot, Color accent) {
    int tw = MeasureText(label, 12);
    int w = tw + 16;
    if (w < 40) w = 40;
    Rectangle r = {(float)x, (float)y, (float)w, (float)h};
    bool hover = CheckCollisionPointRec(GetMousePosition(), r);
    Color bg  = hot ? (Color){accent.r/5, accent.g/5, accent.b/5, 230}
                    : (hover ? (Color){28, 22, 44, 230} : (Color){18, 15, 30, 220});
    Color bdr = hot ? (Color){accent.r, accent.g, accent.b, 220}
                    : (Color){accent.r, accent.g, accent.b, (uchar)(hover ? 180 : 90)};
    DrawRectangleRounded(r, 0.35f, 6, bg);
    DrawRectangleRoundedLines(r, 0.35f, 6, bdr);
    DrawText(label, x + w/2 - tw/2, y + h/2 - 6, 12,
             hot ? (Color){255,250,220,255} : (Color){200,190,230,230});
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static int KeyChipWidth(const char *label) {
    int w = MeasureText(label, 12) + 16;
    return w < 40 ? 40 : w;
}
#endif

// â”€â”€â”€ SOUNDTRACK SHOP ART â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
                               bool selected, bool previewPlaying,
                               bool unlocked, MusicAccess access,
                               int price, bool *previewClick, float t) {
    Vector2 mp = GetMousePosition();
    bool hover = CheckCollisionPointRec(mp, r);
    if (previewClick) *previewClick = false;
    Color ac = MusicTrackAccent(track);
    bool locked = track >= 0 && !unlocked;
    // The equipped state belongs to the card. A preview is shown on its own
    // playback control so it cannot be mistaken for a selected soundtrack.
    bool highlighted = selected;

    if (highlighted) {
        for (int g = 4; g >= 1; g--) {
            DrawRectangleRounded(
                (Rectangle){r.x-g*2.0f, r.y-g*2.0f,
                            r.width+g*4.0f, r.height+g*4.0f},
                0.14f, 8, (Color){ac.r, ac.g, ac.b,
                                  (uchar)((highlighted ? 6 : 3) * g)});
        }
    }

    Color fill = highlighted ? (Color){18,24,42,250}
                             : hover ? (Color){18,18,35,245}
                                     : (Color){11,10,23,235};
    DrawRectangleRounded(r, 0.10f, 8, fill);
    DrawRectangleRoundedLines(r, 0.10f, 8,
                              (Color){ac.r, ac.g, ac.b,
                                      (uchar)(highlighted ? 245 : (hover ? 185 : 95))});
    if (highlighted) {
        Rectangle inner = {r.x+4, r.y+4, r.width-8, r.height-8};
        DrawRectangleRoundedLines(inner, 0.09f, 8,
                                  (Color){ac.r, ac.g, ac.b, 55});
    }

    int pad = 12;
    // Match the larger V1.6 artwork when the card can fit it. On short cards,
    // cap the image height so the preview control and duration stay in the card.
    float artHeight = r.height * 0.45f;
    float maxArtHeight = r.height - 162.0f;
    if (maxArtHeight < 1.0f) maxArtHeight = 1.0f;
    if (artHeight > maxArtHeight) artHeight = maxArtHeight;
    Rectangle art = {r.x+(float)pad, r.y+(float)pad,
                     r.width-(float)(pad*2), artHeight};
    // Artwork is allowed to have energetic details, but none of them should
    // paint over the card frame.  This is especially important for the
    // Dungeon Crawler wall, whose staggered bricks deliberately extend past
    // both sides of the artwork area.
    BeginScissorMode((int)art.x, (int)art.y, (int)art.width, (int)art.height);
    DrawMusicArtwork(art, track, t, selected);
    EndScissorMode();
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
    DrawText(title, (int)r.x+pad, (int)(art.y+art.height+6), titleFs,
             (Color){235,232,248,245});
    const char *detail = track < 0 ? "SILENT SLOT"
                        : unlocked ? "ORIGINAL SCORE"
                        : access == MUSIC_ACCESS_PURCHASE
                            ? TextFormat("%d COINS", price)
                            : GetMusicTrackAccessLabel(track);
    DrawText(detail,
             (int)r.x+pad, (int)(art.y+art.height+28), 10,
              locked ? (Color){205,145,170,210} : (Color){145,140,175,185});

    Rectangle equip = {r.x+(float)pad, r.y+r.height-42.0f,
                       r.width-(float)(pad*2), 28.0f};
    bool playPressed = false;
    bool progressPressed = false;
    if (track >= 0) {
        const float previewW = 72.0f;
        const float previewH = 40.0f;
        const float groupH = previewH + 3.0f + 13.0f + 7.0f;
        float slotTop = art.y + art.height + 40.0f;
        float slotBottom = equip.y - 2.0f;
        float slotHeight = slotBottom - slotTop;
        float controlY = slotTop + 2.0f;
        if (slotHeight > groupH)
            controlY += (slotHeight - groupH) * 0.5f;
        Rectangle preview = {r.x+(float)pad, controlY,
                             previewW, previewH};
        bool playHover = CheckCollisionPointRec(mp, preview);
        playPressed = playHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        Color previewAccent = previewPlaying
            ? (Color){100,245,210,255} : ac;
        Color previewFill = previewPlaying
            ? (Color){13,53,49,250}
            : playHover ? (Color){ac.r/5,ac.g/5,ac.b/5,250}
                        : (Color){18,20,32,245};
        DrawRectangleRounded(preview,0.24f,8,previewFill);
        DrawRectangleRoundedLines(preview,0.24f,8,
            (Color){previewAccent.r,previewAccent.g,previewAccent.b,
                    (uchar)(previewPlaying ? 245 : (playHover ? 225 : 165))});
        float iconX = preview.x + preview.width*0.5f;
        float iconY = preview.y + preview.height*0.5f;
        if (previewPlaying) {
            // YouTube-like pause glyph while the preview is running.
            DrawRectangleRounded((Rectangle){iconX-7.0f,iconY-8.0f,5.0f,16.0f},
                                 0.20f,4,previewAccent);
            DrawRectangleRounded((Rectangle){iconX+2.0f,iconY-8.0f,5.0f,16.0f},
                                 0.20f,4,previewAccent);
        } else {
            DrawPoly((Vector2){iconX,iconY},3,12.0f,0.0f,
                     (Color){238,248,255,255});
        }

        float duration = GetMusicTrackLength(track);
        int durationSeconds = duration > 0.0f ? (int)(duration + 0.5f) : 0;
        const char *durationText = durationSeconds > 0
            ? TextFormat("%d:%02d", durationSeconds / 60, durationSeconds % 60)
            : "--:--";
        int durationWidth = MeasureText(durationText,10);
        float durationY = preview.y + preview.height + 3.0f;
        DrawText(durationText,
                 (int)(preview.x+preview.width*0.5f-durationWidth*0.5f),
                 (int)durationY, 10,
                 (Color){previewPlaying ? 175 : 155,
                         previewPlaying ? 245 : 165,
                         previewPlaying ? 220 : 190, 230});

        if (previewPlaying) {
            float duration = GetMusicTrackLength(track);
            float played = GetSoundtrackPreviewTimePlayed(track);
            float progress = duration > 0.0f ? played / duration : 0.0f;
            if (progress < 0.0f) progress = 0.0f;
            if (progress > 1.0f) progress = 1.0f;
            Rectangle progressBar = {
                r.x+(float)pad, durationY+13.0f,
                r.width-(float)(pad*2), 7.0f
            };
            Rectangle scrubHit = {
                progressBar.x, progressBar.y-9.0f,
                progressBar.width, progressBar.height+18.0f
            };
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                CheckCollisionPointRec(mp,scrubHit)) {
                musicPreviewScrubTrack = track;
                progressPressed = true;
            }
            if (musicPreviewScrubTrack == track) {
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    progress = (mp.x-progressBar.x)/progressBar.width;
                    if (progress < 0.0f) progress = 0.0f;
                    if (progress > 1.0f) progress = 1.0f;
                    SeekSoundtrackPreview(track,duration*progress);
                } else {
                    musicPreviewScrubTrack = -1;
                }
            }
            DrawRectangleRounded(progressBar,0.5f,8,(Color){8,13,24,240});
            DrawRectangleRoundedLines(progressBar,0.5f,8,(Color){65,120,140,180});
            if (progress > 0.0f) {
                Rectangle progressFill = {
                    progressBar.x, progressBar.y,
                    progressBar.width*progress, progressBar.height
                };
                DrawRectangleRounded(progressFill,0.5f,8,previewAccent);
            }
            float knobX = progressBar.x+progressBar.width*progress;
            float knobY = progressBar.y+progressBar.height*0.5f;
            DrawCircle((int)knobX,(int)knobY,5.0f,(Color){235,255,250,255});
            DrawCircleLines((int)knobX,(int)knobY,6.0f,previewAccent);
        }
    }
    if (previewClick) *previewClick = playPressed;

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
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
           !playPressed && !progressPressed;
}

static bool SettingsActionButton(int x, int y, int w, int h,
                                 const char *label, Color accent, bool canClick) {
    Rectangle r = {(float)x,(float)y,(float)w,(float)h};
    bool hover = canClick && CheckCollisionPointRec(GetMousePosition(), r);
    DrawRectangleRounded(r, 0.25f, 6,
                         hover ? (Color){46,34,27,235} : (Color){24,19,29,220});
    DrawRectangleRoundedLines(r, 0.25f, 6,
                              hover ? accent : (Color){accent.r,accent.g,accent.b,120});
    int tw = MeasureText(label, 12);
    DrawText(label, x + (w-tw)/2, y + (h-12)/2, 12,
             hover ? (Color){255,225,195,255} : (Color){210,175,145,230});
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// Auto-unlocks the achievement-locked skins once the underlying achievement(s)
// reach Tier 6. Checked once per frame while any menu is open.
static void CheckAchievementSkinUnlocks(void) {
    AchievementProgress progress[ACH_COUNT];
    BuildAchievementProgress(progress);
    bool changed = false;
    if (!music_owned[MUSIC_OVERACHIEVER_TRACK] &&
        progress[ACH_OVERACHIEVER_INDEX].value >= 200) {
        music_owned[MUSIC_OVERACHIEVER_TRACK] = 1;
        changed = true;
    }
    if (!skin_owned[30] && global_games_played >= 1000)               { skin_owned[30] = 1; changed = true; }
    if (!skin_owned[31] && global_rounds       >= 10000)              { skin_owned[31] = 1; changed = true; }
    if (!skin_owned[32] && global_totcoi >= 100000 && hs_coi >= 250)  { skin_owned[32] = 1; changed = true; }
    if (!skin_owned[33] && global_totkills >= 5000)                    { skin_owned[33] = 1; changed = true; }
    if (!skin_owned[41] && progress[ACH_OVERACHIEVER_INDEX].value >= 200) {
        skin_owned[41] = 1;
        changed = true;
    }
    if (changed) SaveShop();
}

static void DrawSettingsPanel(float t, bool fromPause) {
    static char diagnosticsStatus[64] = "";
    static float diagnosticsStatusUntil = 0.0f;
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    {
        Color ac = (Color){180,160,255,255};
        Rectangle backR = DrawSubPanel("SETTINGS", ac, t, sw, sh);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(GetMousePosition(), backR))
            { capturingAction = -1; capturingSlot = -1;
#ifndef DEBUG_ROOMS
              bindDeleteMode = false;
#endif
              if (fromPause) pauseSettingsOpen = false;
              else menuSub = 0;
              return; }

        int margin_s = 32;
        int panel_w  = sw - margin_s*2;
        int rw = panel_w - 80;
        if (rw > 900) rw = 900;
        int cx = margin_s + (panel_w - rw) / 2;

        int clipTop = margin_s + 62;
        int clipBot = sh - 88;
        int clipH   = clipBot - clipTop;
        if (clipH < 80) clipH = 80;

        int rh = 36;
        int rgap = 5;
        int sgap = 10;

        int label_area = rw / 4;
        int ctrlX = cx + label_area + 12;
        int togX  = cx + rw - 115;
        int slW   = togX - ctrlX - 70;
        if (slW < 80) slW = 80;

        int sli_h    = 18;
        int tog_h    = 28;
        int sli_yoff = (rh - sli_h) / 2;
        int tog_yoff = (rh - tog_h) / 2; if (tog_yoff < 2) tog_yoff = 2;
        int fs_lbl   = 14;
        int fs_desc  = 11;
        bool show_desc = true;

        bool save_needed = false;
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f && capturingAction < 0)
            settingsScroll -= (int)(wheel * 36);
        if (settingsScroll < 0) settingsScroll = 0;
        if (settingsScroll > settingsMaxScroll) settingsScroll = settingsMaxScroll;

        BeginScissorMode(cx - 8, clipTop, rw + 28, clipH);
        int ry = clipTop + 8 - settingsScroll;

        // â•â•â• AUDIO â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
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

        if (!fromPause) {
        // â•â•â• GAMEPLAY â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
        {
            int lw2 = MeasureText("GAMEPLAY", 14);
            DrawLine(cx, ry+10, cx+40, ry+10, (Color){80,200,140,110});
            DrawText("GAMEPLAY", cx+48, ry+3, 14, (Color){100,215,155,200});
            DrawLine(cx+58+lw2, ry+10, cx+rw, ry+10, (Color){80,200,140,110});
        }
        ry += 26;

        // Enemies (ON=red / OFF=green â€” game convention)
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
                Color eBg  = eneon ? (Color){CENE.r/5, CENE.g/5, CENE.b/5, 220}
                                   : (Color){CPL.r/8, CPL.g/8, CPL.b/8, 200};
                Color eBdr = eneon ? CENE : CPL;
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

        // In-game item select (click/wheel + highlight). Default OFF.
        {
            SettingsRowBg(cx, ry, rw, rh, (Color){80,200,140,45});
            int ly = show_desc ? ry+5 : ry+(rh-fs_lbl)/2;
            DrawText("ITEM SELECT", cx+12, ly, fs_lbl, (Color){100,215,155,220});
            if (show_desc) {
                char selB[40], slotB[32];
                FormatActionBinds(ACT_USE_SELECTED, selB, (int)sizeof(selB));
                FormatUseSlotHint(slotB, (int)sizeof(slotB));
                DrawText(mouse_item_select
                         ? TextFormat("ON: click/wheel pick a slot, highlight it, %s uses it", selB)
                         : TextFormat("OFF: no pick, no highlight. %s still use a slot", slotB),
                         cx+12, ry+rh-16, fs_desc, (Color){60,150,100,170});
            }
            bool was_select = mouse_item_select;
            save_needed |= SettingsToggle(togX, ry+tog_yoff, 100, tog_h, &mouse_item_select,
                                          (Color){80,220,150,255});
            if (was_select && !mouse_item_select) inv_sel = -1;
        }
        ry += rh + rgap;
        } else {
            int lw2 = MeasureText("GAMEPLAY", 14);
            DrawLine(cx, ry+10, cx+40, ry+10, (Color){80,200,140,110});
            DrawText("GAMEPLAY", cx+48, ry+3, 14, (Color){100,215,155,200});
            DrawLine(cx+58+lw2, ry+10, cx+rw, ry+10, (Color){80,200,140,110});
            ry += 26;
        }

        {
            SettingsRowBg(cx, ry, rw, rh, (Color){80,200,140,45});
            int ly = show_desc ? ry+5 : ry+(rh-fs_lbl)/2;
            DrawText("EXIT WARNING", cx+12, ly, fs_lbl, (Color){100,215,155,220});
            if (show_desc)
                DrawText("Confirm before leaving a run from the pause menu",
                         cx+12, ry+rh-16, fs_desc, (Color){60,150,100,170});
            save_needed |= SettingsToggle(togX, ry+tog_yoff, 100, tog_h,
                                          &exit_warning, (Color){80,220,150,255});
        }
        ry += rh + sgap;

        // â•â•â• GRAFIK / UI â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
        {
            int lw2 = MeasureText("GRAFIK / UI", 14);
            DrawLine(cx, ry+10, cx+40, ry+10, (Color){80,150,255,110});
            DrawText("GRAFIK / UI", cx+48, ry+3, 14, (Color){100,170,255,200});
            DrawLine(cx+58+lw2, ry+10, cx+rw, ry+10, (Color){80,150,255,110});
        }
        ry += 26;

        // Particle Density 3-way selector
        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){80,150,255,45});
            int ly = show_desc ? ry+5 : ry+(rh-fs_lbl)/2;
            DrawText("PARTICLES", cx+12, ly, fs_lbl, (Color){100,170,255,220});
            if (show_desc)
                DrawText("Burst density:  Low=50%  Med=100%  High=200%",
                         cx+12, ry+rh-16, fs_desc, (Color){60,110,200,170});
            const char *plbl[3] = {"LOW","MED","HIGH"};
            int pbw = 88, pbgap = 6;
            int pbsx = cx + rw - 15 - (3*pbw + 2*pbgap);
            for (int pi = 0; pi < 3; pi++) {
                int bx = pbsx + pi*(pbw+pbgap);
                bool sel = (par_density == pi);
                Rectangle pr = {(float)bx,(float)(ry+tog_yoff),(float)pbw,(float)tog_h};
                DrawRectangleRounded(pr, 0.35f,6,
                    sel?(Color){12,24,50,230}:(Color){14,18,22,180});
                DrawRectangleRoundedLines(pr, 0.35f,6,
                    sel?(Color){80,150,255,220}:(Color){50,80,160,140});
                int plw = MeasureText(plbl[pi], 14);
                DrawText(plbl[pi], bx+pbw/2-plw/2, (int)(pr.y+tog_h/2-7), 14,
                    sel?(Color){120,180,255,255}:(Color){70,90,150,200});
                if (CheckCollisionPointRec(GetMousePosition(),pr) &&
                    IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                    { par_density = pi; save_needed = true; }
            }
        }
        ry += rh + rgap;

        // Photosensitivity â€” harder than Low particles, kills strobes.
        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){80,150,255,45});
            int ly = show_desc ? ry+5 : ry+(rh-fs_lbl)/2;
            DrawText("PHOTOSENSITIVE", cx+12, ly, fs_lbl, (Color){100,170,255,220});
            if (show_desc)
                DrawText("Cuts bursts below Low and disables strobes / screen-flash",
                         cx+12, ry+rh-16, fs_desc, (Color){60,110,200,170});
            save_needed |= SettingsToggle(togX, ry+tog_yoff, 100, tog_h,
                                          &photosensitivity, (Color){100,170,255,255});
        }
        ry += rh + rgap;

        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){80,150,255,45});
            int ly = show_desc ? ry+5 : ry+(rh-fs_lbl)/2;
            DrawText("SCREEN SHAKE", cx+12, ly, fs_lbl, (Color){100,170,255,220});
            if (show_desc)
                DrawText("Camera kick on bombs, hits and boss impacts",
                         cx+12, ry+rh-16, fs_desc, (Color){60,110,200,170});
            save_needed |= SettingsToggle(togX, ry+tog_yoff, 100, tog_h,
                                          &screen_shake, (Color){100,170,255,255});
        }
        ry += rh + rgap;

        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){80,150,255,45});
            int ly = show_desc ? ry+5 : ry+(rh-fs_lbl)/2;
            DrawText("LOUD SFX", cx+12, ly, fs_lbl, (Color){100,170,255,220});
            if (show_desc)
                DrawText("Full-volume explosions, deaths and boss hits. Off tames them",
                         cx+12, ry+rh-16, fs_desc, (Color){60,110,200,170});
            save_needed |= SettingsToggle(togX, ry+tog_yoff, 100, tog_h,
                                          &loud_sfx, (Color){100,170,255,255});
        }
        ry += rh + rgap;

        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){80,150,255,45});
            int ly = show_desc ? ry+5 : ry+(rh-fs_lbl)/2;
            DrawText("FOCUS PAUSE", cx+12, ly, fs_lbl, (Color){100,170,255,220});
            if (show_desc)
                DrawText("Pause automatically when the window loses focus",
                         cx+12, ry+rh-16, fs_desc, (Color){60,110,200,170});
            save_needed |= SettingsToggle(togX, ry+tog_yoff, 100, tog_h,
                                          &focus_pause, (Color){100,170,255,255});
        }
        ry += rh + rgap;

        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){80,150,255,45});
            int ly = show_desc ? ry+5 : ry+(rh-fs_lbl)/2;
            DrawText("SKIP PLAY ANIMATION", cx+12, ly, fs_lbl, (Color){100,170,255,220});
            if (show_desc)
                DrawText("Skip the dungeon-door animation when starting a run",
                         cx+12, ry+rh-16, fs_desc, (Color){60,110,200,170});
            save_needed |= SettingsToggle(togX, ry+tog_yoff, 100, tog_h,
                                          &skip_play_anim, (Color){100,170,255,255});
        }
        ry += rh + rgap;

        // Camera Smoothness
        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){80,150,255,45});
            DrawText("CAMERA SPEED", cx+12, ry+(rh-fs_lbl)/2, fs_lbl, (Color){100,170,255,220});
            save_needed |= SettingsSlider(ctrlX, ry+sli_yoff, slW, sli_h,
                                          &cam_smooth, 1.0f, 20.0f, (Color){80,150,255,255});
            DrawText(TextFormat("%.1f", cam_smooth), ctrlX+slW+10, ry+sli_yoff, fs_lbl,
                     (Color){100,155,225,210});
        }
        ry += rh + rgap;

        // Fullscreen
        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){80,150,255,45});
            int ly = show_desc ? ry+5 : ry+(rh-fs_lbl)/2;
            DrawText("FULLSCREEN", cx+12, ly, fs_lbl, (Color){100,170,255,220});
            if (show_desc) {
                char fsB[24];
                FormatActionBinds(ACT_FULLSCREEN, fsB, (int)sizeof(fsB));
                DrawText(TextFormat("Toggle fullscreen mode  (also: %s)", fsB),
                         cx+12, ry+rh-16, fs_desc, (Color){60,110,200,170});
            }
            {
                bool fs = GameWindowIsFullscreen();
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
        ry += rh + rgap;

        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){80,150,255,45});
            int ly = show_desc ? ry+5 : ry+(rh-fs_lbl)/2;
            DrawText("SHOW SEED", cx+12, ly, fs_lbl, (Color){100,170,255,220});
            if (show_desc)
                DrawText("Show the current run seed on the HUD", cx+12, ry+rh-16,
                         fs_desc, (Color){60,110,200,170});
            save_needed |= SettingsToggle(togX, ry+tog_yoff, 100, tog_h, &show_seed,
                                          (Color){100,170,255,255});
        }
        ry += rh + rgap;

        // Optional active-time counters on the gameplay HUD
        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){80,150,255,45});
            DrawText("SHOW ROUND TIME",cx+12,ry+5,fs_lbl,(Color){100,170,255,220});
            DrawText("Active time in the current round",cx+12,ry+rh-16,fs_desc,
                     (Color){60,110,200,170});
            save_needed |= SettingsToggle(togX,ry+tog_yoff,100,tog_h,&show_round_time,
                                          (Color){100,170,255,255});
        }
        ry += rh + rgap;

        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){80,150,255,45});
            DrawText("SHOW GAME TIME",cx+12,ry+5,fs_lbl,(Color){100,170,255,220});
            DrawText("Active time since this run started",cx+12,ry+rh-16,fs_desc,
                     (Color){60,110,200,170});
            save_needed |= SettingsToggle(togX,ry+tog_yoff,100,tog_h,&show_game_time,
                                          (Color){100,170,255,255});
        }
        ry += rh + rgap;

        // Colorblind mode
        {
            DrawRectangleRounded((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                  0.18f,6,(Color){16,13,26,180});
            DrawRectangleRoundedLines((Rectangle){(float)cx,(float)ry,(float)rw,(float)rh},
                                       0.18f,6,(Color){80,150,255,45});
            int ly = show_desc ? ry+5 : ry+(rh-fs_lbl)/2;
            DrawText("COLORBLIND", cx+12, ly, fs_lbl, (Color){100,170,255,220});
            if (show_desc)
                DrawText("Remaps traps, enemies, player, loot and UI hues",
                         cx+12, ry+rh-16, fs_desc, (Color){60,110,200,170});
            const char *cbl[4] = {"OFF","RED-GRN","BLU-YEL","HIGH"};
            int cbw = 78, cbgap = 5;
            int cbsx = cx + rw - 15 - (4*cbw + 3*cbgap);
            for (int ci = 0; ci < 4; ci++) {
                int bx = cbsx + ci*(cbw+cbgap);
                bool sel = (colorblind_mode == ci);
                Rectangle cr = {(float)bx,(float)(ry+tog_yoff),(float)cbw,(float)tog_h};
                DrawRectangleRounded(cr, 0.35f,6,
                    sel?(Color){12,24,50,230}:(Color){14,18,22,180});
                DrawRectangleRoundedLines(cr, 0.35f,6,
                    sel?(Color){80,150,255,220}:(Color){50,80,160,140});
                int clw = MeasureText(cbl[ci], 12);
                DrawText(cbl[ci], bx+cbw/2-clw/2, (int)(cr.y+tog_h/2-6), 12,
                    sel?(Color){120,180,255,255}:(Color){70,90,150,200});
                if (CheckCollisionPointRec(GetMousePosition(),cr) &&
                    IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                    colorblind_mode != ci) {
                    colorblind_mode = ci;
                    ApplyColorblindMode();
                    save_needed = true;
                }
            }
        }
        ry += rh + sgap;

#ifndef DEBUG_ROOMS
        {
            Color kac = (Color){90, 210, 220, 255};
            {
                int lw2 = MeasureText("GAMEPLAY BINDS", 14);
                DrawLine(cx, ry+10, cx+40, ry+10, (Color){90,210,220,110});
                DrawText("GAMEPLAY BINDS", cx+48, ry+3, 14, (Color){120,230,235,200});
                DrawLine(cx+58+lw2, ry+10, cx+rw, ry+10, (Color){90,210,220,110});
            }
            ry += 26;
            DrawText(bindDeleteMode
                     ? "DELETE MODE  |  Click a bind to remove it.  ESC or DELETE to stop."
                     : "Click a bind to rebind.  + adds.  DELETE button/key to remove.",
                     cx+12, ry, 12,
                     bindDeleteMode ? (Color){230,120,120,210} : (Color){110,150,160,190});
            ry += 20;

            bool boundThisFrame = false;
            if (capturingAction < 0 && !bindDeleteMode && IsKeyPressed(KEY_DELETE)) {
                bindDeleteMode = true;
            } else if (bindDeleteMode &&
                       (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_ESCAPE))) {
                bindDeleteMode = false;
            }

            if (capturingAction >= 0) {
                int key = 0;
                if (IsKeyPressed(KEY_ESCAPE)) {
                    capturingAction = -1;
                    capturingSlot = -1;
                    while (GetKeyPressed() != 0) {}
                } else {
                    key = GetKeyPressed();
                    while (GetKeyPressed() != 0) {}
                    if (!key) {
                        const int mods[] = {
                            KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT,
                            KEY_LEFT_CONTROL, KEY_RIGHT_CONTROL,
                            KEY_LEFT_ALT, KEY_RIGHT_ALT
                        };
                        for (int mi = 0; mi < 6; mi++)
                            if (IsKeyPressed(mods[mi])) { key = mods[mi]; break; }
                    }
                    if (!key) {
                        for (int btn = 0; btn <= BIND_MOUSE_MAX_BTN; btn++) {
                            if (IsMouseButtonPressed(btn)) {
                                key = BIND_FROM_MOUSE(btn);
                                boundThisFrame = true;
                                break;
                            }
                        }
                    }
                    if (key && key != KEY_ESCAPE) {
                        bool ok = (capturingSlot >= 0)
                            ? ReplaceKeybind(capturingAction, capturingSlot, key)
                            : AddKeybind(capturingAction, key);
                        if (ok) {
                            capturingAction = -1;
                            capturingSlot = -1;
                            save_needed = true;
                        } else {
                            char kn[24];
                            FormatKeyName(key, kn, (int)sizeof(kn));
                            int other = FindConflictingAction(key, capturingAction);
                            if (other == capturingAction)
                                snprintf(bindWarn, sizeof(bindWarn),
                                         "%s is already on %s", kn, ACT_LABEL[capturingAction]);
                            else if (other >= 0)
                                snprintf(bindWarn, sizeof(bindWarn),
                                         "%s already used by %s at the same time",
                                         kn, ACT_LABEL[other]);
                            else if (capturingSlot < 0 &&
                                     CountKeybinds(capturingAction) >= KB_MAX_BINDS)
                                snprintf(bindWarn, sizeof(bindWarn),
                                         "Max %d binds on %s", KB_MAX_BINDS,
                                         ACT_LABEL[capturingAction]);
                            else
                                snprintf(bindWarn, sizeof(bindWarn),
                                         "Can't bind %s", kn);
                            bindWarnUntil = t + 2.6f;
                        }
                    }
                }
            }

            Color delAc = (Color){220, 70, 70, 255};
            for (int section = 0; section < 2; section++) {
            if (section == 1) {
                ry += sgap;
                int lw2 = MeasureText("MENU NAVIGATION", 14);
                DrawLine(cx, ry+10, cx+40, ry+10, (Color){200,170,255,110});
                DrawText("MENU NAVIGATION", cx+48, ry+3, 14, (Color){210,185,255,200});
                DrawLine(cx+58+lw2, ry+10, cx+rw, ry+10, (Color){200,170,255,110});
                ry += 26;
            }
            int a0 = (section == 0) ? 0 : ACT_MENU_NEXT;
            int a1 = (section == 0) ? ACT_MENU_NEXT : ACT_COUNT;
            Color rowAc = (section == 0) ? kac : (Color){200, 170, 255, 255};
            for (int a = a0; a < a1; a++) {
                int y = ry;
                bool cap = (capturingAction == a);
                SettingsRowBg(cx, y, rw, rh,
                              bindDeleteMode ? (Color){220, 70, 70, 80}
                              : cap ? (Color){rowAc.r, rowAc.g, rowAc.b, 140}
                                    : (Color){rowAc.r/5, rowAc.g/5, rowAc.b/5, 80});
                DrawText(ACT_LABEL[a], cx+12, y + rh/2 - 7, 14,
                         bindDeleteMode ? (Color){240, 160, 160, 230}
                                        : (Color){rowAc.r, rowAc.g, rowAc.b, 230});

                int chipX = cx + 210;
                int chipH = rh - 10;
                if (chipH < 20) chipH = 20;
                int chipY = y + (rh - chipH) / 2;
                int nb = CountKeybinds(a);
                for (int b = 0; b < nb; b++) {
                    bool rewriting = (capturingAction == a && capturingSlot == b);
                    char kn[24];
                    if (rewriting) strncpy(kn, "PRESS...", sizeof(kn));
                    else FormatKeyName(keybinds[a][b], kn, (int)sizeof(kn));
                    kn[sizeof(kn)-1] = '\0';
                    int cw = KeyChipWidth(kn);
                    if (chipX + cw > cx + rw - 88) break;
                    Color chipAc = bindDeleteMode ? delAc : rowAc;
                    if (DrawKeyChip(chipX, chipY, chipH, kn, rewriting, chipAc)) {
                        if (!boundThisFrame && capturingAction < 0) {
                            if (bindDeleteMode) {
                                if (RemoveKeybind(a, b)) save_needed = true;
                                else {
                                    snprintf(bindWarn, sizeof(bindWarn),
                                             "Can't remove the last bind on %s",
                                             ACT_LABEL[a]);
                                    bindWarnUntil = t + 2.6f;
                                }
                            } else {
                                capturingAction = a;
                                capturingSlot = b;
                                bindDeleteMode = false;
                            }
                        }
                    }
                    chipX += cw + 6;
                }
                if (!bindDeleteMode && nb < KB_MAX_BINDS) {
                    bool adding = (cap && capturingSlot < 0);
                    const char *plus = adding ? "PRESS..." : "+";
                    if (DrawKeyChip(chipX, chipY, chipH, plus, adding, rowAc)) {
                        if (!boundThisFrame) {
                            if (adding) { capturingAction = -1; capturingSlot = -1; }
                            else { capturingAction = a; capturingSlot = -1; }
                        }
                    }
                }
                ry += rh + rgap;
            }
            } // section

            ry += 6;
            Rectangle delR = {(float)cx, (float)ry, 158.0f, 34.0f};
            Rectangle resetR = {(float)(cx + rw - 168), (float)ry, 168.0f, 34.0f};
            bool dhov = CheckCollisionPointRec(GetMousePosition(), delR);
            bool rhov = CheckCollisionPointRec(GetMousePosition(), resetR);
            DrawRectangleRounded(delR, 0.28f, 6,
                                 bindDeleteMode ? (Color){70, 12, 16, 240}
                                 : dhov ? (Color){50, 16, 20, 230} : (Color){28, 12, 16, 220});
            DrawRectangleRoundedLines(delR, 0.28f, 6,
                                      bindDeleteMode || dhov ? delAc : (Color){160, 50, 55, 140});
            const char *dl = "DELETE BINDS";
            int dlw = MeasureText(dl, 12);
            DrawText(dl, (int)(delR.x + delR.width/2 - dlw/2),
                     (int)delR.y + 10, 12,
                     bindDeleteMode || dhov ? (Color){255,180,180,255}
                                            : (Color){210,110,110,220});
            if (dhov && !boundThisFrame && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                bindDeleteMode = !bindDeleteMode;
                capturingAction = -1;
                capturingSlot = -1;
            }

            DrawRectangleRounded(resetR, 0.28f, 6,
                                 rhov ? (Color){35, 25, 55, 240} : (Color){18, 15, 31, 225});
            DrawRectangleRoundedLines(resetR, 0.28f, 6,
                                      (Color){kac.r, kac.g, kac.b, (uchar)(rhov ? 220 : 120)});
            const char *rl = "RESET DEFAULTS";
            int rlw = MeasureText(rl, 12);
            DrawText(rl, (int)(resetR.x + resetR.width/2 - rlw/2),
                     (int)resetR.y + 10, 12,
                     rhov ? (Color){230, 250, 255, 255} : (Color){140, 210, 220, 220});
            if (rhov && !boundThisFrame && capturingAction < 0 &&
                IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                ResetKeybindsToDefault();
                capturingAction = -1;
                capturingSlot = -1;
                bindDeleteMode = false;
                SaveSettings();
                save_needed = false;
            }
            if (capturingAction >= 0) {
                const char *hint = "Press a key or mouse button  |  ESC to cancel";
                DrawText(hint, cx + 168, (int)resetR.y + 10, 13, (Color){220, 200, 120, 230});
            }
            ry += 42;
        }
#endif

        // Keep diagnostics last, after the release-only key bindings.
        ry += sgap;
        {
            Color dc = (Color){200,150,100,255};
            int lw2 = MeasureText("DIAGNOSTICS", 14);
            DrawLine(cx, ry+10, cx+40, ry+10, (Color){dc.r,dc.g,dc.b,110});
            DrawText("DIAGNOSTICS", cx+48, ry+3, 14, (Color){dc.r,dc.g,dc.b,220});
            DrawLine(cx+58+lw2, ry+10, cx+rw, ry+10, (Color){dc.r,dc.g,dc.b,110});
        }
        ry += 26;
        SettingsRowBg(cx, ry, rw, rh, (Color){120,85,55,80});
        DrawText("WINDOWS CONSOLE", cx+12, ry+5, fs_lbl, (Color){225,175,125,230});
        DrawText("Show live game messages in a separate window", cx+12, ry+rh-16,
                 fs_desc, (Color){155,115,90,190});
        if (ry >= clipTop && ry + rh <= clipTop + clipH &&
            SettingsToggle(togX, ry+tog_yoff, 100, tog_h, &show_windows_console,
                           (Color){225,165,105,255})) {
            if (!GameConsoleSetEnabled(show_windows_console)) show_windows_console = false;
            save_needed = true;
        } else if (ry < clipTop || ry + rh > clipTop + clipH) {
            // Draw the control while clipped, without accepting a hidden click.
            Rectangle r = {(float)togX,(float)(ry+tog_yoff),100.0f,(float)tog_h};
            DrawRectangleRounded(r,0.35f,8,show_windows_console
                ? (Color){37,27,17,220} : (Color){18,14,28,200});
            DrawRectangleRoundedLines(r,0.35f,8,show_windows_console
                ? (Color){225,165,105,200} : (Color){55,48,82,140});
            const char *lbl = show_windows_console ? "ON" : "OFF";
            DrawText(lbl,togX+50-MeasureText(lbl,16)/2,ry+tog_yoff+tog_h/2-8,16,
                     show_windows_console ? (Color){225,165,105,255}
                                          : (Color){70,62,100,200});
        }
        ry += rh + rgap;

        static const char *levels[] = {"ERRORS", "WARNINGS", "INFO", "DEBUG"};
        {
            int buttonX = cx + rw - 175;
            Color dc = (Color){225,165,105,255};
            SettingsRowBg(cx, ry, rw, rh, (Color){120,85,55,80});
            DrawText("LOG DETAIL", cx+12, ry+5, fs_lbl, (Color){225,175,125,230});
            DrawText("Choose which messages are written to the log", cx+12,
                     ry+rh-16, fs_desc, (Color){155,115,90,190});
            if (SettingsActionButton(buttonX, ry+tog_yoff, 160, tog_h,
                                     levels[console_log_detail], dc,
                                     ry >= clipTop && ry+rh <= clipTop+clipH)) {
                console_log_detail = (console_log_detail + 1) % 4;
                GameConsoleSetLogDetail(console_log_detail);
                save_needed = true;
            }
        }
        ry += rh + rgap;

        {
            Color dc = (Color){225,165,105,255};
            SettingsRowBg(cx, ry, rw, rh, (Color){120,85,55,80});
            DrawText("LOG FILE", cx+12, ry+5, fs_lbl, (Color){225,175,125,230});
            DrawText("Open this session's messages in Notepad", cx+12,
                     ry+rh-16, fs_desc, (Color){155,115,90,190});
            if (SettingsActionButton(cx+rw-175, ry+tog_yoff, 160, tog_h,
                                     "OPEN LOG FILE", dc,
                                     ry >= clipTop && ry+rh <= clipTop+clipH)) {
                if (GameConsoleOpenLogFile()) {
                    snprintf(diagnosticsStatus, sizeof(diagnosticsStatus), "Log file opened");
                } else {
                    snprintf(diagnosticsStatus, sizeof(diagnosticsStatus), "Could not open log file");
                    TraceLog(LOG_WARNING, "Could not open diagnostics log file");
                }
                diagnosticsStatusUntil = t + 2.5f;
            }
        }
        ry += rh + rgap;

        {
            Color dc = (Color){225,165,105,255};
            SettingsRowBg(cx, ry, rw, rh, (Color){120,85,55,80});
            DrawText("SYSTEM INFORMATION", cx+12, ry+5, fs_lbl,
                     (Color){225,175,125,230});
            DrawText("Copy game and PC details for error reports", cx+12,
                     ry+rh-16, fs_desc, (Color){155,115,90,190});
            if (SettingsActionButton(cx+rw-175, ry+tog_yoff, 160, tog_h,
                                     "COPY DETAILS", dc,
                                     ry >= clipTop && ry+rh <= clipTop+clipH)) {
                char systemInfo[512], report[1024];
                GameConsoleGetSystemInfo(systemInfo, sizeof(systemInfo));
                snprintf(report, sizeof(report),
                         "Dungeon Crawler v1.7\nBuild: %s\nWindow: %d x %d (%s)\n"
                         "Monitor: %d x %d\nRound: %d\nSeed: %u\nLog detail: %s\n\n%s",
#ifdef DEBUG_ROOMS
                         "Debug",
#else
                         "Main Game",
#endif
                         GetScreenWidth(), GetScreenHeight(),
                         GameWindowIsFullscreen() ? "Fullscreen" : "Windowed",
                         GetMonitorWidth(GetCurrentMonitor()),
                         GetMonitorHeight(GetCurrentMonitor()),
                         roundn, run_seed, levels[console_log_detail], systemInfo);
                SetClipboardText(report);
                TraceLog(LOG_INFO, "System information copied to clipboard");
                snprintf(diagnosticsStatus, sizeof(diagnosticsStatus),
                         "System information copied");
                diagnosticsStatusUntil = t + 2.5f;
            }
        }
        ry += rh + rgap;
        if (t < diagnosticsStatusUntil) {
            DrawText(diagnosticsStatus, cx+12, ry+2, 12,
                     (Color){225,190,150,220});
        }
        ry += 18;

        int contentBottom = ry + 12;
        EndScissorMode();

        int contentH = contentBottom - (clipTop + 8 - settingsScroll);
        int maxScroll = contentH - clipH;
        if (maxScroll < 0) maxScroll = 0;
        settingsMaxScroll = maxScroll;
        if (settingsScroll < 0) settingsScroll = 0;
        if (settingsScroll > settingsMaxScroll) settingsScroll = settingsMaxScroll;

        if (maxScroll > 0) {
            float frac = (float)settingsScroll / (float)maxScroll;
            float barH = (float)clipH * (float)clipH / (float)contentH;
            if (barH < 28.0f) barH = 28.0f;
            float barY = (float)clipTop + frac * ((float)clipH - barH);
            DrawRectangleRounded(
                (Rectangle){ (float)(cx + rw + 10), barY, 5.0f, barH },
                1.0f, 4, (Color){ ac.r, ac.g, ac.b, 140 });
        }

#ifndef DEBUG_ROOMS
        if (t < bindWarnUntil && bindWarn[0]) {
            float left = bindWarnUntil - t;
            float a = (left < 0.4f) ? left / 0.4f : 1.0f;
            int fs = 16;
            int tw = MeasureText(bindWarn, fs);
            int pw = tw + 48;
            if (pw < 280) pw = 280;
            int ph = 52;
            Rectangle pop = { (float)(sw/2 - pw/2), (float)(sh/2 - ph/2),
                              (float)pw, (float)ph };
            DrawRectangleRounded(pop, 0.22f, 8,
                                 (Color){28, 12, 18, (uchar)(235 * a)});
            DrawRectangleRoundedLines(pop, 0.22f, 8,
                                      (Color){230, 90, 90, (uchar)(220 * a)});
            DrawText(bindWarn, (int)(pop.x + pop.width/2 - tw/2),
                     (int)(pop.y + ph/2 - 8), fs,
                     (Color){255, 210, 210, (uchar)(255 * a)});
        }
#endif

        if (save_needed) SaveSettings();
    }
}

static void DrawBookCorner(Rectangle r, int len, int thick, Color gold) {
    DrawRectangle((int)r.x, (int)r.y, len, thick, gold);
    DrawRectangle((int)r.x, (int)r.y, thick, len, gold);
    DrawRectangle((int)(r.x + r.width - len), (int)r.y, len, thick, gold);
    DrawRectangle((int)(r.x + r.width - thick), (int)r.y, thick, len, gold);
    DrawRectangle((int)r.x, (int)(r.y + r.height - thick), len, thick, gold);
    DrawRectangle((int)r.x, (int)(r.y + r.height - len), thick, len, gold);
    DrawRectangle((int)(r.x + r.width - len), (int)(r.y + r.height - thick), len, thick, gold);
    DrawRectangle((int)(r.x + r.width - thick), (int)(r.y + r.height - len), thick, len, gold);
}

static Color TutorialColorA(Color c, int a) {
    if (a < 0) a = 0;
    if (a > 255) a = 255;
    return (Color){ c.r, c.g, c.b, (uchar)a };
}

static Color TutorialLighten(Color c, float amount) {
    return (Color){
        (uchar)fminf(255.0f, c.r + (255.0f - c.r) * amount),
        (uchar)fminf(255.0f, c.g + (255.0f - c.g) * amount),
        (uchar)fminf(255.0f, c.b + (255.0f - c.b) * amount),
        c.a
    };
}

// Item pickups stay bright for most of the cycle.  The short dim tail keeps
// them alive without making their readable colour disappear for half the time.
static float TutorialItemLight(float t, float phase) {
    if (photosensitivity) return 0.94f;
    float q = fmodf(t * 0.72f + phase, 1.0f);
    if (q < 0.0f) q += 1.0f;
    if (q < 0.06f) return 0.78f + 0.22f * (q / 0.06f);
    if (q < 0.92f) return 1.0f;
    if (q < 0.97f) return 1.0f - 0.22f * ((q - 0.92f) / 0.05f);
    return 0.78f;
}

static int TutorialFitFont(const char *text, int fontSize, int maxWidth) {
    if (!text || maxWidth <= 0) return 1;
    if (fontSize < 1) fontSize = 1;
    while (fontSize > 8 && MeasureText(text, fontSize) > maxWidth)
        fontSize--;
    return fontSize;
}

static void DrawTutorialTextFit(const char *text, int x, int y, int fontSize,
                               int maxWidth, Color color) {
    if (!text || maxWidth <= 0) return;
    int fs = TutorialFitFont(text, fontSize, maxWidth);
    DrawText(text, x, y, fs, color);
}

static int DrawTutorialTextWrapped(const char *text, int x, int y, int fontSize,
                                   int maxWidth, int maxHeight, int lineGap,
                                   Color color) {
    if (!text || maxWidth <= 0 || maxHeight <= 0) return 0;
    if (fontSize < 1) fontSize = 1;
    if (lineGap < 0) lineGap = 0;

    // Keep a whole word readable on narrow cards; wrapping, rather than
    // shrinking the complete sentence to one line, is the normal path.
    int fs = fontSize;
    while (fs > 8) {
        int longest = 0;
        const char *p = text;
        while (*p) {
            while (*p == ' ' || *p == '\t' || *p == '\n') p++;
            const char *start = p;
            while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
            int len = (int)(p - start);
            if (len > 0) {
                char word[256];
                int n = len < (int)sizeof(word) - 1 ? len : (int)sizeof(word) - 1;
                memcpy(word, start, (size_t)n);
                word[n] = '\0';
                int ww = MeasureText(word, fs);
                if (ww > longest) longest = ww;
            }
        }
        if (longest <= maxWidth) break;
        fs--;
    }

    int lineStep = fs + lineGap;
    if (lineStep < 1) lineStep = 1;
    int maxLines = maxHeight / lineStep;
    if (maxLines < 1) maxLines = 1;

    char line[512] = { 0 };
    int lineLen = 0;
    int lines = 0;
    const char *p = text;
    while (*p && lines < maxLines) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\n') {
            if (lineLen > 0) {
                DrawText(line, x, y + lines * lineStep, fs, color);
                lines++;
                lineLen = 0;
                line[0] = '\0';
            }
            p++;
            continue;
        }
        if (!*p) break;

        const char *start = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
        int wordLen = (int)(p - start);
        if (wordLen <= 0) continue;
        if (wordLen >= (int)sizeof(line)) wordLen = (int)sizeof(line) - 1;

        char word[512];
        memcpy(word, start, (size_t)wordLen);
        word[wordLen] = '\0';
        if (lineLen == 0) {
            memcpy(line, word, (size_t)wordLen + 1);
            lineLen = wordLen;
        } else {
            char candidate[512];
            int candidateLen = lineLen + 1 + wordLen;
            if (candidateLen >= (int)sizeof(candidate)) candidateLen = (int)sizeof(candidate) - 1;
            memcpy(candidate, line, (size_t)lineLen);
            candidate[lineLen] = ' ';
            int copyLen = candidateLen - lineLen - 1;
            if (copyLen > 0) memcpy(candidate + lineLen + 1, word, (size_t)copyLen);
            candidate[candidateLen] = '\0';
            if (MeasureText(candidate, fs) <= maxWidth) {
                memcpy(line, candidate, (size_t)candidateLen + 1);
                lineLen = candidateLen;
            } else {
                DrawText(line, x, y + lines * lineStep, fs, color);
                lines++;
                if (lines >= maxLines) break;
                memcpy(line, word, (size_t)wordLen + 1);
                lineLen = wordLen;
            }
        }
    }
    if (lineLen > 0 && lines < maxLines)
        DrawText(line, x, y + lines * lineStep, fs, color);
    return lines + (lineLen > 0 && lines < maxLines ? 1 : 0);
}

enum {
    TUT_ICON_TREASURE = 0,
    TUT_ICON_PORTAL,
    TUT_ICON_TRAP,
    TUT_ICON_TRAPDOOR,
    TUT_ICON_COIN,
    TUT_ICON_ITEM,
    TUT_ICON_ENEMY,
    TUT_ICON_BOSS
};

static Color TutorialIconAccent(int kind, IKind itemKind) {
    switch (kind) {
        case TUT_ICON_TREASURE: return CTRES;
        case TUT_ICON_PORTAL:   return CPORT;
        case TUT_ICON_TRAP:     return CTRAP;
        case TUT_ICON_TRAPDOOR: return (Color){ 116, 94, 150, 255 };
        case TUT_ICON_COIN:     return CCOIN;
        case TUT_ICON_ITEM:
            if (itemKind >= IK_SPD && itemKind <= IK_GLD) return IK_COL[itemKind];
            return (Color){ 180, 140, 220, 255 };
        case TUT_ICON_ENEMY:    return CENE;
        case TUT_ICON_BOSS:     return CBOSS;
        default:                return (Color){ 180, 150, 90, 255 };
    }
}

static void DrawTutorialDiamond(float cx, float cy, float size, Color color) {
    Vector2 top = { cx, cy - size };
    Vector2 right = { cx + size, cy };
    Vector2 bottom = { cx, cy + size };
    Vector2 left = { cx - size, cy };
    DrawTriangle(top, right, bottom, color);
    DrawTriangle(top, bottom, left, color);
}

static void DrawTutorialItemGlyph(float cx, float cy, float size, IKind itemKind,
                                  Color accent) {
    float r = size * 0.28f;
    switch (itemKind) {
        case IK_SPD:
            DrawTriangle((Vector2){ cx - size * 0.44f, cy },
                         (Vector2){ cx + size * 0.02f, cy - size * 0.32f },
                         (Vector2){ cx + size * 0.02f, cy + size * 0.32f }, accent);
            DrawTriangle((Vector2){ cx - size * 0.02f, cy },
                         (Vector2){ cx + size * 0.44f, cy - size * 0.32f },
                         (Vector2){ cx + size * 0.44f, cy + size * 0.32f },
                         TutorialLighten(accent, 0.22f));
            break;
        case IK_SHD:
            DrawTriangle((Vector2){ cx, cy - size * 0.44f },
                         (Vector2){ cx + size * 0.38f, cy - size * 0.23f },
                         (Vector2){ cx + size * 0.28f, cy + size * 0.25f }, accent);
            DrawTriangle((Vector2){ cx, cy - size * 0.44f },
                         (Vector2){ cx + size * 0.28f, cy + size * 0.25f },
                         (Vector2){ cx, cy + size * 0.46f }, accent);
            DrawTriangle((Vector2){ cx, cy - size * 0.44f },
                         (Vector2){ cx, cy + size * 0.46f },
                         (Vector2){ cx - size * 0.28f, cy + size * 0.25f },
                         TutorialLighten(accent, 0.18f));
            DrawTriangle((Vector2){ cx, cy - size * 0.44f },
                         (Vector2){ cx - size * 0.28f, cy + size * 0.25f },
                         (Vector2){ cx - size * 0.38f, cy - size * 0.23f },
                         TutorialLighten(accent, 0.08f));
            break;
        case IK_BMB:
            DrawCircleV((Vector2){ cx, cy + size * 0.08f }, r, accent);
            DrawCircleV((Vector2){ cx - size * 0.10f, cy - size * 0.04f },
                        r * 0.34f, TutorialLighten(accent, 0.42f));
            DrawLineEx((Vector2){ cx + size * 0.10f, cy - size * 0.20f },
                       (Vector2){ cx + size * 0.26f, cy - size * 0.40f },
                       fmaxf(1.5f, size * 0.10f), TutorialLighten(accent, 0.25f));
            DrawCircleV((Vector2){ cx + size * 0.29f, cy - size * 0.43f },
                        fmaxf(1.0f, size * 0.08f), (Color){ 255, 215, 90, 255 });
            break;
        case IK_WLK:
            DrawCircleLines((int)cx, (int)cy, size * 0.33f, accent);
            DrawCircleLines((int)cx, (int)cy, size * 0.18f,
                            TutorialLighten(accent, 0.28f));
            DrawLineEx((Vector2){ cx - size * 0.40f, cy + size * 0.28f },
                       (Vector2){ cx + size * 0.38f, cy - size * 0.28f },
                       fmaxf(1.5f, size * 0.09f), TutorialLighten(accent, 0.38f));
            break;
        case IK_SWD:
            DrawLineEx((Vector2){ cx - size * 0.30f, cy + size * 0.34f },
                       (Vector2){ cx + size * 0.31f, cy - size * 0.35f },
                       fmaxf(2.0f, size * 0.13f), accent);
            DrawLineEx((Vector2){ cx - size * 0.42f, cy + size * 0.30f },
                       (Vector2){ cx - size * 0.22f, cy + size * 0.48f },
                       fmaxf(1.5f, size * 0.10f), TutorialLighten(accent, 0.25f));
            DrawLineEx((Vector2){ cx - size * 0.44f, cy + size * 0.10f },
                       (Vector2){ cx - size * 0.23f, cy + size * 0.28f },
                       fmaxf(1.5f, size * 0.09f), TutorialLighten(accent, 0.12f));
            break;
        case IK_GLD:
            DrawCircleV((Vector2){ cx - size * 0.14f, cy + size * 0.04f },
                        size * 0.24f, accent);
            DrawCircleV((Vector2){ cx + size * 0.17f, cy - size * 0.08f },
                        size * 0.24f, TutorialLighten(accent, 0.18f));
            DrawCircleV((Vector2){ cx - size * 0.21f, cy - size * 0.05f },
                        size * 0.08f, (Color){ 255, 248, 190, 210 });
            break;
        default:
            DrawTutorialDiamond(cx, cy, size * 0.38f, accent);
            break;
    }
}

static void DrawTutorialIcon(Rectangle bounds, int kind, IKind itemKind, float t) {
    float size = fminf(bounds.width, bounds.height);
    float cx = bounds.x + bounds.width * 0.5f;
    float cy = bounds.y + bounds.height * 0.5f;
    Color accent = TutorialIconAccent(kind, itemKind);
    Color plate = { 57, 43, 36, 235 };
    Color plateLo = { 31, 24, 25, 235 };
    Rectangle shadow = { bounds.x + 2, bounds.y + 3, bounds.width, bounds.height };
    DrawRectangleRounded(shadow, 0.18f, 6, TutorialColorA(plateLo, 125));
    DrawRectangleRounded(bounds, 0.18f, 6, plate);
    DrawRectangleRoundedLines(bounds, 0.18f, 6, TutorialColorA(accent, 190));
    DrawRectangleRoundedLines(
        (Rectangle){ bounds.x + 3, bounds.y + 3, bounds.width - 6, bounds.height - 6 },
        0.16f, 6, TutorialColorA(TutorialLighten(accent, 0.18f), 60));

    float s = size;
    float pulse = 0.5f + 0.5f * sinf(t * 2.6f);
    switch (kind) {
        case TUT_ICON_TREASURE: {
            float cw = s * 0.52f, ch = s * 0.27f;
            Rectangle chest = { cx - cw * 0.5f, cy - ch * 0.02f, cw, ch };
            DrawRectangleRounded((Rectangle){ chest.x + 1, chest.y + 2, chest.width, chest.height },
                                 0.16f, 4, TutorialColorA((Color){ 70, 42, 20, 255 }, 160));
            DrawRectangleRounded(chest, 0.16f, 4, accent);
            DrawRectangle((int)chest.x, (int)chest.y, (int)chest.width,
                          fmaxf(2.0f, s * 0.07f), TutorialLighten(accent, 0.35f));
            DrawLine((int)cx, (int)(chest.y + 2), (int)cx,
                     (int)(chest.y + chest.height - 2), TutorialColorA((Color){ 82, 44, 20, 255 }, 210));
            DrawRectangleRounded((Rectangle){ cx - s * 0.055f, chest.y + ch * 0.36f,
                                               s * 0.11f, s * 0.13f },
                                 0.25f, 3, (Color){ 255, 232, 126, 255 });
            break;
        }
        case TUT_ICON_PORTAL:
            DrawCircleV((Vector2){ cx, cy }, s * (0.36f + 0.02f * pulse),
                        TutorialColorA(accent, 42));
            DrawCircleV((Vector2){ cx, cy }, s * 0.25f, (Color){ 18, 29, 48, 255 });
            DrawCircleLines((int)cx, (int)cy, s * (0.36f + 0.02f * pulse),
                            TutorialColorA(TutorialLighten(accent, 0.25f), 220));
            DrawCircleLines((int)cx, (int)cy, s * 0.25f,
                            TutorialColorA(accent, 180));
            DrawCircleV((Vector2){ cx - s * 0.08f, cy - s * 0.09f },
                        s * 0.07f, TutorialColorA((Color){ 220, 245, 255, 255 }, 220));
            break;
        case TUT_ICON_TRAP: {
            Rectangle plateR = { cx - s * 0.30f, cy - s * 0.25f, s * 0.60f, s * 0.50f };
            DrawRectangleRounded(plateR, 0.12f, 4, (Color){ 27, 23, 30, 255 });
            DrawRectangleRoundedLines(plateR, 0.12f, 4, TutorialColorA(accent, 230));
            for (int i = -1; i <= 1; i++) {
                float sx = cx + i * s * 0.18f;
                DrawTriangle((Vector2){ sx, cy - s * 0.15f },
                             (Vector2){ sx + s * 0.07f, cy + s * 0.19f },
                             (Vector2){ sx - s * 0.07f, cy + s * 0.19f },
                             TutorialLighten(accent, 0.18f));
            }
            break;
        }
        case TUT_ICON_TRAPDOOR: {
            Rectangle hatch = { cx - s * 0.29f, cy - s * 0.29f, s * 0.58f, s * 0.58f };
            Color hatchC = { 43, 36, 49, 255 };
            DrawRectangleRounded(hatch, 0.10f, 4, hatchC);
            DrawRectangleRoundedLines(hatch, 0.10f, 4, TutorialColorA(accent, 220));
            DrawLineEx((Vector2){ cx - s * 0.22f, cy },
                       (Vector2){ cx + s * 0.22f, cy }, fmaxf(1.0f, s * 0.055f),
                       TutorialColorA(accent, 175));
            DrawLineEx((Vector2){ cx, cy - s * 0.22f },
                       (Vector2){ cx, cy + s * 0.22f }, fmaxf(1.0f, s * 0.055f),
                       TutorialColorA(accent, 175));
            DrawCircleV((Vector2){ cx + s * 0.11f, cy - s * 0.08f },
                        fmaxf(1.5f, s * 0.065f), TutorialLighten(accent, 0.26f));
            break;
        }
        case TUT_ICON_COIN:
            DrawCircleV((Vector2){ cx + s * 0.04f, cy + s * 0.05f }, s * 0.28f,
                        TutorialColorA((Color){ 112, 74, 12, 255 }, 160));
            DrawCircleV((Vector2){ cx, cy }, s * 0.28f, accent);
            DrawCircleLines((int)cx, (int)cy, s * 0.28f,
                            TutorialLighten(accent, 0.16f));
            DrawCircleV((Vector2){ cx - s * 0.09f, cy - s * 0.10f },
                        s * 0.085f, (Color){ 255, 247, 184, 220 });
            break;
        case TUT_ICON_ITEM:
            pulse = TutorialItemLight(t, (float)itemKind * 0.17f);
            {
                Color itemAccent = TutorialLighten(accent, 0.10f + 0.16f * pulse);
                DrawCircleV((Vector2){ cx, cy }, s * (0.29f + 0.015f * pulse),
                            TutorialColorA(itemAccent, (int)(34 + 48 * pulse)));
                DrawTutorialItemGlyph(cx, cy, s * 0.68f, itemKind, itemAccent);
            }
            break;
        case TUT_ICON_ENEMY:
            DrawCircleV((Vector2){ cx, cy }, s * (0.36f + 0.02f * pulse),
                        TutorialColorA(accent, 42));
            DrawCircleV((Vector2){ cx, cy + s * 0.03f }, s * 0.27f, accent);
            DrawCircleV((Vector2){ cx - s * 0.10f, cy - s * 0.06f }, s * 0.07f,
                        (Color){ 255, 205, 70, 255 });
            DrawCircleV((Vector2){ cx + s * 0.10f, cy - s * 0.06f }, s * 0.07f,
                        (Color){ 255, 205, 70, 255 });
            DrawCircleV((Vector2){ cx - s * 0.10f, cy - s * 0.06f }, s * 0.027f,
                        (Color){ 30, 8, 18, 255 });
            DrawCircleV((Vector2){ cx + s * 0.10f, cy - s * 0.06f }, s * 0.027f,
                        (Color){ 30, 8, 18, 255 });
            break;
        case TUT_ICON_BOSS:
            // Cycle through the five real boss skins, not a generic placeholder.
            // The preview calls the same renderer used in the live boss fight.
            {
                int pick = ((int)floorf(fmaxf(0.0f, t) / 3.0f)) % 5;
                DrawBossPreview(cx, cy + s * 0.02f, s * 0.255f, pick, t);
            }
            break;
        default:
            DrawTutorialDiamond(cx, cy, s * 0.28f, accent);
            break;
    }
}

static void DrawTutorialPageBase(Rectangle page, Color paper, Color edge, bool shadeRight) {
    int shadowX = (int)fmaxf(1.0f, fminf(3.0f, page.width * 0.005f));
    int shadowY = (int)fmaxf(1.0f, fminf(4.0f, page.height * 0.008f));
    int innerPad = (int)fmaxf(2.0f, fminf(3.0f, page.width * 0.006f));
    int linePad = (int)fmaxf(3.0f, fminf(8.0f, page.height * 0.016f));
    DrawRectangleRounded((Rectangle){ page.x + shadowX, page.y + shadowY,
                                     page.width, page.height },
                         0.025f, 8, TutorialColorA((Color){ 48, 29, 22, 255 }, 72));
    DrawRectangleRounded(page, 0.025f, 8, paper);
    DrawRectangleRoundedLines(page, 0.025f, 8, TutorialColorA(edge, 175));
    DrawRectangleRoundedLines(
        (Rectangle){ page.x + innerPad, page.y + innerPad,
                     page.width - innerPad * 2, page.height - innerPad * 2 },
        0.025f, 8, TutorialColorA(edge, 42));

    int stripW = (int)fmaxf(5.0f, page.width * 0.022f);
    if (shadeRight) {
        DrawRectangle((int)(page.x + page.width - stripW), (int)page.y,
                      stripW, (int)page.height,
                      TutorialColorA((Color){ 77, 49, 33, 255 }, 34));
        DrawLine((int)(page.x + page.width - stripW), (int)page.y + linePad,
                 (int)(page.x + page.width - stripW),
                 (int)(page.y + page.height - linePad), TutorialColorA(edge, 95));
    } else {
        DrawRectangle((int)page.x, (int)page.y, stripW, (int)page.height,
                      TutorialColorA((Color){ 77, 49, 33, 255 }, 26));
        DrawLine((int)(page.x + stripW), (int)page.y + linePad,
                 (int)(page.x + stripW), (int)(page.y + page.height - linePad),
                  TutorialColorA(edge, 75));
    }
}

static int DrawTutorialKeycap(int x, int y, int h, const char *label,
                              int fontSize, int maxWidth, Color accent,
                              Color paperDark) {
    if (maxWidth < 1) maxWidth = 1;
    int pad = h / 3;
    if (pad < 3) pad = 3;
    if (pad > 12) pad = 12;
    int textMax = maxWidth - pad * 2;
    if (textMax < 1) textMax = 1;
    int fs = TutorialFitFont(label, fontSize, textMax);
    int w = MeasureText(label, fs) + pad * 2;
    if (w > maxWidth) w = maxWidth;
    int minW = h * 2;
    if (minW < 18) minW = 18;
    if (w < minW) w = (maxWidth < minW) ? maxWidth : minW;
    Rectangle r = { (float)x, (float)y, (float)w, (float)h };
    DrawRectangleRounded((Rectangle){ r.x + 2, r.y + 2, r.width, r.height },
                         0.25f, 6, TutorialColorA((Color){ 47, 29, 23, 255 }, 110));
    DrawRectangleRounded(r, 0.25f, 6, TutorialColorA(paperDark, 145));
    DrawRectangleRoundedLines(r, 0.25f, 6, TutorialColorA(accent, 170));
    int tw = MeasureText(label, fs);
    DrawText(label, x + w / 2 - tw / 2, y + h / 2 - fs / 2 - 1, fs,
             TutorialLighten(accent, 0.10f));
    return w;
}

static int TutorialTilePx(float value, float scale) {
    int p = (int)(value * scale + 0.5f);
    return (p < 1) ? 1 : p;
}

static int TutorialTileSpan(int total, int pad) {
    int span = total - pad * 2;
    return (span < 1) ? 1 : span;
}

static Color TutorialMapSymbolAccent(char tile, IKind itemKind) {
    switch (tile) {
        case '.': return CWH;
        case '#': return CWH;
        case 'T': return CTRES;
        case 'O': return CPORT;
        case '$': return CCOIN;
        case '+': return CTRAP;
        case 'h': return (Color){ 116, 94, 150, 255 };
        case 'H': return (Color){ 116, 94, 150, 255 };
        case 'r': return (Color){ 230, 125, 55, 255 };
        case 'D': return CCANN;
        case 'L': return CFIRE;
        case '^': return CPLATE;
        case '*': return CPLATE;
        case 'C': return CCANN;
        case 'I':
            if (itemKind >= IK_SPD && itemKind <= IK_GLD) return IK_COL[itemKind];
            return (Color){ 180, 140, 220, 255 };
        default: return CF;
    }
}

// The legend deliberately mirrors DrawWorldTile() in game_map.c.  It uses the
// same tile letters, runtime palette and proportions, but draws into a small
// square so it remains responsive inside the book.
static void DrawTutorialMapSymbol(Rectangle bounds, char tile, IKind itemKind,
                                  float t) {
    float side = fminf(bounds.width, bounds.height);
    if (side < 1.0f) return;
    int cell = (int)(side + 0.5f);
    int x = (int)(bounds.x + (bounds.width - side) * 0.5f + 0.5f);
    int y = (int)(bounds.y + (bounds.height - side) * 0.5f + 0.5f);
    float scale = side / (float)CELL;
#define TUT_TILE_PX(v) TutorialTilePx((v), scale)

    DrawRectangle(x, y, cell, cell, CB);
    int p1 = TUT_TILE_PX(1.0f);
    int floorSpan = TutorialTileSpan(cell, p1);
    if (tile == '#') {
        DrawRectangle(x + p1, y + p1, floorSpan, floorSpan, CW);
        DrawRectangle(x + p1, y + p1, floorSpan, TUT_TILE_PX(3.0f), CWH);
        DrawRectangle(x + p1, y + p1, TUT_TILE_PX(3.0f), floorSpan, CWH);
        DrawRectangle(x + p1, y + cell - TUT_TILE_PX(4.0f), floorSpan,
                      TUT_TILE_PX(3.0f), CWD);
        DrawRectangle(x + cell - TUT_TILE_PX(4.0f), y + p1,
                      TUT_TILE_PX(3.0f), floorSpan, CWD);
        return;
    }

    DrawRectangle(x + p1, y + p1, floorSpan, floorSpan, CF);
    if (tile == '+') {
        int p3 = TUT_TILE_PX(3.0f);
        int trapSpan = TutorialTileSpan(cell, p3);
        DrawRectangle(x + p3, y + p3, trapSpan, trapSpan,
                      (Color){20, 15, 25, 255});
        DrawRectangleLines(x + p3, y + p3, trapSpan, trapSpan,
                           (Color){60, 50, 70, 255});
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                int spx = x + TUT_TILE_PX(8.0f + i * 8.0f);
                int spy = y + TUT_TILE_PX(8.0f + j * 8.0f);
                DrawCircle(spx, spy, TUT_TILE_PX(2.0f),
                           (Color){190, 190, 210, 255});
                DrawRectangle(spx - TUT_TILE_PX(1.0f), spy - TUT_TILE_PX(2.0f),
                              TUT_TILE_PX(2.0f), TUT_TILE_PX(2.0f), CTRAP);
            }
        }
    } else if (tile == 'h') {
        DrawRectangle(x + p1, y + p1, floorSpan, floorSpan,
                      (Color){20, 19, 32, 255});
        DrawLine(x + TUT_TILE_PX(5.0f), y + cell / 2,
                 x + cell - TUT_TILE_PX(5.0f), y + cell / 2,
                 (Color){14, 12, 22, 100});
        DrawPixel(x + TUT_TILE_PX(4.0f), y + TUT_TILE_PX(4.0f),
                  (Color){42, 38, 56, 150});
        DrawPixel(x + cell - TUT_TILE_PX(5.0f), y + TUT_TILE_PX(4.0f),
                  (Color){42, 38, 56, 150});
        DrawPixel(x + TUT_TILE_PX(4.0f), y + cell - TUT_TILE_PX(5.0f),
                  (Color){42, 38, 56, 130});
        DrawPixel(x + cell - TUT_TILE_PX(5.0f), y + cell - TUT_TILE_PX(5.0f),
                  (Color){42, 38, 56, 130});
        DrawRectangleLines(x + TUT_TILE_PX(2.0f), y + TUT_TILE_PX(2.0f),
                           TutorialTileSpan(cell, TUT_TILE_PX(2.0f)),
                           TutorialTileSpan(cell, TUT_TILE_PX(2.0f)),
                           (Color){16, 14, 26, 55});
    } else if (tile == 'H') {
        DrawRectangle(x + p1, y + p1, floorSpan, floorSpan,
                      (Color){8, 6, 12, 255});
        int p4 = TUT_TILE_PX(4.0f);
        int pitSpan = TutorialTileSpan(cell, p4);
        DrawRectangle(x + p4, y + p4, pitSpan, pitSpan,
                      (Color){2, 1, 4, 255});
        DrawRectangleLines(x + TUT_TILE_PX(3.0f), y + TUT_TILE_PX(3.0f),
                           TutorialTileSpan(cell, TUT_TILE_PX(3.0f)),
                           TutorialTileSpan(cell, TUT_TILE_PX(3.0f)),
                           (Color){55, 48, 70, 180});
        float pulse = 0.5f + 0.5f * sinf(t * 5.0f);
        DrawCircle(x + cell / 2, y + cell / 2, TUT_TILE_PX(5.0f),
                   (Color){20, 12, 30, (uchar)(40 + 40 * pulse)});
    } else if (tile == 'r') {
        float cyc = fmodf(t, ARROW_PERIOD);
        if (cyc < 0.0f) cyc += ARROW_PERIOD;
        float fireStart = ARROW_PERIOD - ARROW_FIRE;
        float warnStart = fireStart - ARROW_WARN;
        bool lethal = cyc >= fireStart;
        bool warn = cyc >= warnStart && cyc < fireStart;
        if (warn) {
            DrawRectangle(x + TUT_TILE_PX(2.0f), y + TUT_TILE_PX(2.0f),
                          TutorialTileSpan(cell, TUT_TILE_PX(2.0f)),
                          TutorialTileSpan(cell, TUT_TILE_PX(2.0f)),
                          (Color){235, 200, 60, 35});
            int fs = TUT_TILE_PX(22.0f);
            if (fs < 6) fs = 6;
            const char *ex = "!";
            DrawText(ex, x + cell / 2 - MeasureText(ex, fs) / 2,
                     y + cell / 2 - fs / 2, fs,
                     (Color){255, 220, 60, 240});
        } else {
            Color arrow = lethal ? (Color){255, 80, 60, 255}
                                 : (Color){95, 85, 78, 180};
            int my = y + cell / 2;
            for (int a = 0; a < 2; a++) {
                int ox = x + TUT_TILE_PX(6.0f + a * 12.0f);
                DrawTriangle((Vector2){(float)ox, (float)(my - TUT_TILE_PX(5.0f))},
                             (Vector2){(float)(ox + TUT_TILE_PX(7.0f)), (float)my},
                             (Vector2){(float)ox, (float)(my + TUT_TILE_PX(5.0f))}, arrow);
            }
            if (lethal) {
                DrawCircle(x + cell / 2, y + cell / 2,
                           TUT_TILE_PX(11.0f),
                           (Color){255, 80, 60, 30});
            }
        }
    } else if (tile == 'D') {
        float cyc = fmodf(t, ARROW_PERIOD);
        if (cyc < 0.0f) cyc += ARROW_PERIOD;
        float fireStart = ARROW_PERIOD - ARROW_FIRE;
        float warnStart = fireStart - ARROW_WARN;
        bool lethal = cyc >= fireStart;
        bool warn = cyc >= warnStart && cyc < fireStart;
        float wp = warn ? (cyc - warnStart) / ARROW_WARN : 0.0f;
        int p2 = TUT_TILE_PX(2.0f);
        DrawRectangle(x + p1, y + p1, floorSpan, floorSpan,
                      (Color){40, 34, 30, 255});
        DrawRectangleLines(x + p1, y + p1, floorSpan, floorSpan, CCANN);
        Color muzzle = lethal ? (Color){255, 235, 180, 255}
                     : warn ? (Color){235, (uchar)(150 + 80 * wp), 60, 255}
                            : (Color){120, 95, 70, 255};
        DrawRectangle(x + cell - TUT_TILE_PX(12.0f),
                      y + cell / 2 - TUT_TILE_PX(4.0f), TUT_TILE_PX(10.0f),
                      TUT_TILE_PX(8.0f), muzzle);
        DrawTriangle((Vector2){(float)(x + cell - TUT_TILE_PX(3.0f)),
                              (float)(y + cell / 2 - TUT_TILE_PX(6.0f))},
                     (Vector2){(float)(x + cell - p2), (float)(y + cell / 2)},
                     (Vector2){(float)(x + cell - TUT_TILE_PX(3.0f)),
                              (float)(y + cell / 2 + TUT_TILE_PX(6.0f))}, muzzle);
    } else if (tile == 'L') {
        float pulse = 0.5f + 0.5f * sinf(t * 2.2f);
        DrawRectangle(x + p1, y + p1, floorSpan, floorSpan,
                      (Color){185, 55, 12, 255});
        int p3 = TUT_TILE_PX(3.0f);
        DrawRectangle(x + p3, y + p3, TutorialTileSpan(cell, p3),
                      TutorialTileSpan(cell, p3),
                      (Color){235, (uchar)(90 + 50 * pulse), 22, 255});
        DrawCircle(x + cell / 2, y + cell / 2, TUT_TILE_PX(5.0f),
                   (Color){255, (uchar)(175 + 60 * pulse), 60, 190});
    } else if (tile == '^' || tile == '*') {
        bool armed = tile == '*';
        Color base = armed ? (Color){11, 52, 48, 255}
                           : (Color){22, 22, 30, 255};
        Color rim = armed ? TutorialLighten(CPLATE, 0.22f)
                          : (Color){55, 55, 70, 255};
        Color spike = armed ? TutorialLighten(CPLATE, 0.28f)
                            : (Color){95, 100, 115, 255};
        int p3 = TUT_TILE_PX(3.0f);
        int span = TutorialTileSpan(cell, p3);
        if (armed) {
            float pulse = TutorialItemLight(t, bounds.x * 0.02f + bounds.y * 0.01f);
            DrawCircle(x + cell / 2, y + cell / 2,
                       TUT_TILE_PX(15.0f + 3.0f * pulse),
                       (Color){CPLATE.r, CPLATE.g, CPLATE.b,
                               (uchar)(28 + 32 * pulse)});
        }
        DrawRectangle(x + p3, y + p3, span, span, base);
        DrawRectangleLines(x + p3, y + p3, span, span, rim);
        for (int i = 0; i < 3; i++) {
            float sx0 = (float)(x + TUT_TILE_PX(8.0f + i * 8.0f));
            DrawTriangle((Vector2){sx0, (float)(y + TUT_TILE_PX(9.0f))},
                         (Vector2){sx0 + TUT_TILE_PX(3.0f),
                                   (float)(y + cell - TUT_TILE_PX(9.0f))},
                         (Vector2){sx0 - TUT_TILE_PX(3.0f),
                                   (float)(y + cell - TUT_TILE_PX(9.0f))}, spike);
        }
        if (armed) {
            float pulse = TutorialItemLight(t, bounds.x * 0.02f + bounds.y * 0.02f);
            DrawCircleLines(x + cell / 2, y + cell / 2,
                            TUT_TILE_PX(12.0f + 3.0f * pulse),
                            (Color){CPLATE.r, CPLATE.g, CPLATE.b,
                                    (uchar)(150 + 80 * pulse)});
        }
    } else if (tile == 'C') {
        // Ready cannon: compact top-down silhouette, kept inside the tile.
        // The barrel sits behind the turret so it reads as a cannon instead
        // of a line crossing the whole box.
        float pulse = TutorialItemLight(t, bounds.x * 0.02f + bounds.y * 0.02f);
        int p2 = TUT_TILE_PX(2.0f);
        int span = TutorialTileSpan(cell, p2);
        int mx = x + cell / 2, my = y + cell / 2;
        Color metal = (Color){58, 51, 46, 255};
        Color metalHi = TutorialLighten(CCANN, 0.16f);
        Color barrelDark = (Color){29, 27, 27, 255};
        Color hot = TutorialLighten(CCANN, 0.34f);
        DrawCircle(mx, my, TUT_TILE_PX(10.0f + 2.0f * pulse),
                   (Color){CCANN.r, CCANN.g, CCANN.b,
                           (uchar)(26 + 30 * pulse)});
        DrawRectangle(x + p2, y + p2, span, span, (Color){39, 31, 24, 255});
        DrawRectangleLines(x + p1, y + p1, floorSpan, floorSpan,
                           TutorialLighten(CCANN, 0.10f));
        DrawRectangleLines(x + p2, y + p2, span, span, (Color){103, 83, 58, 255});

        Vector2 dir = { 0.7071f, -0.7071f };
        Vector2 rear = { mx - dir.x * TUT_TILE_PX(4.0f),
                         my - dir.y * TUT_TILE_PX(4.0f) };
        Vector2 tip = { mx + dir.x * TUT_TILE_PX(11.0f),
                        my + dir.y * TUT_TILE_PX(11.0f) };
        DrawLineEx(rear, tip, fmaxf(3.0f, TUT_TILE_PX(7.0f)), barrelDark);
        DrawLineEx(rear, tip, fmaxf(2.0f, TUT_TILE_PX(3.0f)), metalHi);
        DrawCircle((int)tip.x, (int)tip.y, TUT_TILE_PX(3.5f),
                   TutorialColorA(hot, (int)(150 + 80 * pulse)));
        DrawCircle((int)tip.x, (int)tip.y, TUT_TILE_PX(1.5f),
                   (Color){255, 247, 190, (uchar)(205 + 45 * pulse)});

        DrawCircle(mx, my, TUT_TILE_PX(8.0f), metal);
        DrawCircleLines(mx, my, TUT_TILE_PX(8.0f), metalHi);
        DrawCircle(mx, my, TUT_TILE_PX(4.0f),
                   TutorialColorA(hot, (int)(205 + 45 * pulse)));
        DrawCircle(mx, my, TUT_TILE_PX(1.8f),
                   (Color){255, 247, 190, (uchar)(205 + 45 * pulse)});
        DrawCircleLines(mx, my, TUT_TILE_PX(11.0f + 1.5f * pulse),
                        (Color){CCANN.r, CCANN.g, CCANN.b,
                                (uchar)(165 + 80 * pulse)});
    } else if (tile == 'T') {
        float pulse = 0.5f + 0.5f * sinf(t * 3.0f);
        Color treasure = CTRES;
        treasure.a = (uchar)(200 + 55 * pulse);
        DrawRectangle(x + TUT_TILE_PX(6.0f), y + TUT_TILE_PX(9.0f),
                      TutorialTileSpan(cell, TUT_TILE_PX(6.0f)),
                      TutorialTileSpan(cell, TUT_TILE_PX(7.0f)), treasure);
        DrawRectangle(x + TUT_TILE_PX(6.0f), y + TUT_TILE_PX(9.0f),
                      TutorialTileSpan(cell, TUT_TILE_PX(6.0f)), TUT_TILE_PX(4.0f),
                      (Color){255, 240, 180, 230});
        DrawCircle(x + cell / 2, y + cell / 2 + TUT_TILE_PX(2.0f),
                   TUT_TILE_PX(3.0f), (Color){200, 150, 30, 255});
    } else if (tile == '$') {
        float pulse = sinf(t * 4.0f);
        int radius = TUT_TILE_PX(5.0f + pulse * 1.5f);
        if (radius < 1) radius = 1;
        DrawCircle(x + cell / 2 + TUT_TILE_PX(1.0f),
                   y + cell / 2 + TUT_TILE_PX(2.0f), radius,
                   (Color){160, 120, 0, 80});
        DrawCircle(x + cell / 2, y + cell / 2, radius, CCOIN);
        DrawCircle(x + cell / 2 - TUT_TILE_PX(1.0f),
                   y + cell / 2 - TUT_TILE_PX(1.0f),
                   TUT_TILE_PX(2.0f), (Color){255, 245, 160, 200});
    } else if (tile == 'I') {
        if (itemKind >= IK_SPD && itemKind <= IK_GLD) {
            float pulse = TutorialItemLight(t, (float)itemKind * 0.17f);
            Color item = TutorialLighten(IK_COL[itemKind], 0.10f + 0.16f * pulse);
            item.a = 255;
            DrawTriangle((Vector2){(float)(x + cell / 2), (float)(y + TUT_TILE_PX(5.0f))},
                         (Vector2){(float)(x + cell - TUT_TILE_PX(5.0f)),
                                   (float)(y + cell / 2)},
                         (Vector2){(float)(x + cell / 2),
                                   (float)(y + cell - TUT_TILE_PX(5.0f))}, item);
            DrawTriangle((Vector2){(float)(x + cell / 2), (float)(y + TUT_TILE_PX(5.0f))},
                         (Vector2){(float)(x + cell / 2),
                                   (float)(y + cell - TUT_TILE_PX(5.0f))},
                         (Vector2){(float)(x + TUT_TILE_PX(5.0f)),
                                   (float)(y + cell / 2)}, item);
            DrawCircle(x + cell / 2, y + cell / 2,
                       TUT_TILE_PX(12.0f + 3.0f * pulse),
                       (Color){item.r, item.g, item.b, (uchar)(34 + 48 * pulse)});
        }
    } else if (tile == 'O') {
        float pulse = 0.5f + 0.5f * sinf(t * 2.0f);
        DrawCircle(x + cell / 2, y + cell / 2, TUT_TILE_PX(12.0f + 5.0f * pulse),
                   (Color){80, 160, 255, (uchar)(65 + 50 * pulse)});
        DrawCircle(x + cell / 2, y + cell / 2, TUT_TILE_PX(7.0f + 2.0f * pulse),
                   (Color){160, 210, 255, (uchar)(140 + 60 * pulse)});
        DrawCircleLines(x + cell / 2, y + cell / 2,
                        TUT_TILE_PX(12.0f + 5.0f * pulse),
                        (Color){120, 190, 255, 200});
    }
#undef TUT_TILE_PX
}

typedef struct {
    MenuUI u;
    Color paper;
    Color paperLo;
    Color paperDeep;
    Color ink;
    Color inkDim;
    Color gold;
    Color goldLo;
    Color rub;
    int fsT;
    int fsH;
    int fsB;
    int bodyLine;
} TutorialBookStyle;

static int DrawTutorialPageHeading(Rectangle page, const char *title,
                                   TutorialBookStyle b) {
    int x = (int)page.x + MenuPx(b.u, 18);
    int y = (int)page.y + MenuPx(b.u, 14);
    int w = (int)page.width - MenuPx(b.u, 36);
    if (w < 1) w = 1;
    DrawText(title, x, y, b.fsT, b.ink);
    DrawLine(x, y + b.fsT + MenuPx(b.u, 4), x + w,
             y + b.fsT + MenuPx(b.u, 4), b.goldLo);
    DrawLine(x, y + b.fsT + MenuPx(b.u, 8), x + MenuPx(b.u, 44),
             y + b.fsT + MenuPx(b.u, 8), TutorialColorA(b.gold, 80));
    return y + b.fsT + MenuPx(b.u, 14);
}

static void BeginTutorialPageClip(Rectangle page, MenuUI u) {
    int pad = MenuPx(u, 4);
    if (pad < 2) pad = 2;
    int x = (int)page.x + pad;
    int y = (int)page.y + pad;
    int w = (int)page.width - pad * 2;
    int h = (int)page.height - pad * 2;
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    BeginScissorMode(x, y, w, h);
}

static void DrawTutorialPageNumber(Rectangle page, const char *number,
                                   TutorialBookStyle b) {
    int fs = b.fsB;
    int tw = MeasureText(number, fs);
    DrawText(number, (int)(page.x + page.width / 2 - tw / 2),
             (int)(page.y + page.height - MenuPx(b.u, 16)), fs, b.goldLo);
}

static void DrawTutorialPageOne(Rectangle page, TutorialBookStyle b) {
    int lx = (int)page.x + MenuPx(b.u, 18);
    int ly = DrawTutorialPageHeading(page, "HOW TO PLAY", b);
    int lw = (int)page.width - MenuPx(b.u, 36);
    if (lw < 1) lw = 1;

    struct { const char *h; const char *a; const char *b; const char *c; } sec[] = {
        { "THE GOAL",
          "Each round is one floor of the dungeon.",
          "Reach the Treasure to open the next floor.",
          "A Portal skips a floor if you can get to it." },
        { "DEATH",
          "Traps, lava, enemies and bosses kill on contact.",
          "Trapdoors look like normal floor. One step and you fall.",
          "A Shield blocks one hit. Without it, one mistake ends the run." },
        { "ITEMS",
          "Gems on the floor go into your inventory.",
          "Use a slot with your item keys. They are spent on use.",
          "Speed, Shield, Bomb, Ghost Walk, Sword, Gold Touch." },
        { "SPECIAL ROOMS",
          "From round 6 the dungeon mixes extra floors:",
          "Trap rooms fire arrow lanes. Dark rooms hide the map.",
          "Lava rises from the south. Coin Rush packs gold and danger." },
        { "BOSSES",
          "Every 10th round is a Boss arena.",
          "There is no Treasure until the Boss is dead.",
          "Use cannons and spike plates in the arena to hurt it." },
        { "THE DUNGEON GROWS",
          "Each round the floor gets larger.",
          "Walls, traps and enemies get worse as you go.",
          "Coins and items are how you keep up." },
    };
    int nsec = (int)(sizeof(sec) / sizeof(sec[0]));
    int bottom = (int)(page.y + page.height - MenuPx(b.u, 22));
    int gap = MenuPx(b.u, 4);
    if (gap < 3) gap = 3;
    int secH = (bottom - ly - gap * (nsec - 1)) / nsec;
    int secMin = b.fsH + b.bodyLine * 3 + MenuPx(b.u, 7);
    if (secH < secMin) secH = secMin;
    if (secH < 1) secH = 1;
    for (int i = 0; i < nsec; i++) {
        int sy = ly + i * (secH + gap);
        if (sy + b.fsH + b.bodyLine * 3 > bottom) break;
        Rectangle sr = { (float)(lx - MenuPx(b.u, 6)), (float)sy,
                         (float)(lw + MenuPx(b.u, 6)), (float)(secH - 1) };
        DrawRectangleRounded(sr, 0.08f, 5, TutorialColorA(b.paperDeep, 20));
        DrawRectangle((int)sr.x, (int)sr.y + MenuPx(b.u, 3), MenuPx(b.u, 3),
                      (int)sr.height - MenuPx(b.u, 6), TutorialColorA(b.rub, 170));
        DrawText(sec[i].h, lx + MenuPx(b.u, 4), sy, b.fsH, b.rub);
        DrawLine(lx + MenuPx(b.u, 4), sy + b.fsH + MenuPx(b.u, 2),
                 lx + MenuPx(b.u, 4) + MeasureText(sec[i].h, b.fsH) + MenuPx(b.u, 6),
                 sy + b.fsH + MenuPx(b.u, 2), TutorialColorA(b.goldLo, 150));
        int bodyX = lx + MenuPx(b.u, 4);
        int bodyW = lw - MenuPx(b.u, 4);
        int by = sy + b.fsH + MenuPx(b.u, 5);
        DrawTutorialTextFit(sec[i].a, bodyX, by, b.fsB, bodyW, b.ink);
        DrawTutorialTextFit(sec[i].b, bodyX, by + b.bodyLine, b.fsB, bodyW, b.inkDim);
        DrawTutorialTextFit(sec[i].c, bodyX, by + b.bodyLine * 2, b.fsB, bodyW, b.inkDim);
    }
    DrawTutorialPageNumber(page, "I", b);
}

static Color TutorialLegendAccent(char tile, IKind itemKind, int iconKind) {
    if (iconKind >= TUT_ICON_ENEMY) return TutorialIconAccent(iconKind, itemKind);
    return TutorialMapSymbolAccent(tile, itemKind);
}

static void DrawTutorialLegendCard(Rectangle card, const char *name,
                                   const char *desc, char tile, IKind itemKind,
                                   int iconKind, TutorialBookStyle b, float t) {
    Color accent = TutorialLegendAccent(tile, itemKind, iconKind);
    int strip = MenuPx(b.u, 3);
    if (strip < 1) strip = 1;
    DrawRectangleRounded(card, 0.10f, 5, TutorialColorA(b.paperDeep, 24));
    DrawRectangle((int)card.x, (int)card.y + MenuPx(b.u, 4), strip,
                  (int)card.height - MenuPx(b.u, 8), TutorialColorA(accent, 175));

    int iconPad = MenuPx(b.u, 5);
    int iconSize = MenuPx(b.u, 25);
    if (iconSize < 7) iconSize = 7;
    if (iconSize > (int)card.height - MenuPx(b.u, 6))
        iconSize = (int)card.height - MenuPx(b.u, 6);
    if (iconSize < 4) iconSize = 4;
    Rectangle icon = { card.x + iconPad,
                       card.y + (card.height - iconSize) * 0.5f,
                       (float)iconSize, (float)iconSize };
    if (iconKind >= TUT_ICON_ENEMY)
        DrawTutorialIcon(icon, iconKind, itemKind, t);
    else
        DrawTutorialMapSymbol(icon, tile, itemKind, t);

    int tx = (int)(icon.x + icon.width + MenuPx(b.u, 5));
    int textW = (int)(card.x + card.width) - tx - MenuPx(b.u, 5);
    if (textW < 1) textW = 1;
    int nameY = (int)card.y + MenuPx(b.u, 4);
    if (b.fsB <= 7) nameY = (int)(card.y + card.height / 2 - b.fsB / 2);
    DrawTutorialTextFit(name, tx, nameY, b.fsB, textW, accent);
    if (b.fsB >= 8) {
        DrawTutorialTextFit(desc, tx, nameY + b.fsB + MenuPx(b.u, 1),
                            b.fsB - 1, textW, b.inkDim);
    }
}

static void DrawTutorialPageTwo(Rectangle page, TutorialBookStyle b, float t) {
    int x = (int)page.x + MenuPx(b.u, 18);
    int w = (int)page.width - MenuPx(b.u, 36);
    if (w < 1) w = 1;
    int y = DrawTutorialPageHeading(page, "SYMBOLS & ITEMS", b);
    int sectionGap = MenuPx(b.u, 6);
    if (sectionGap < 3) sectionGap = 3;
    DrawText("DUNGEON MARKS", x, y, b.fsH, b.rub);
    y += b.fsH + MenuPx(b.u, 5);

    struct { const char *name; const char *desc; char tile; IKind item; int icon; } marks[] = {
        { "FLOOR",         "Walkable ground.",                 '.', IK_NONE, -1 },
        { "WALL",         "Blocks the path.",                 '#', IK_NONE, -1 },
        { "TREASURE",     "Reach it to advance.",             'T', IK_NONE, -1 },
        { "PORTAL",       "Warp to the next floor.",          'O', IK_NONE, -1 },
        { "COIN",         "Collect for score and Shop.",      '$', IK_NONE, -1 },
        { "TRAP",         "Contact is deadly.",               '+', IK_NONE, -1 },
        { "TRAPDOOR",     "Looks like floor, drops you .",     'h', IK_NONE, -1 },
        { "ARROW LANE",   "Don't run into the red dots.",    'r', IK_NONE, -1 },
        { "DISPENSER",    "Starts an arrow sweep.",            'D', IK_NONE, -1 },
        { "LAVA",         "Avoid! Kills instantly.",                'L', IK_NONE, -1 },
        { "SPIKE PLATE",  "Step on this tile to activate and damage boss", '*', IK_NONE, -1 },
        { "CANNON",       "Step on it to fire.",               'C', IK_NONE, -1 },
        { "ENEMY",        "One touch ends the run.",           0,  IK_NONE, TUT_ICON_ENEMY },
        { "BOSS",         "Defeat it for the Treasure.",       0,  IK_NONE, TUT_ICON_BOSS },
    };
    int markCount = (int)(sizeof(marks) / sizeof(marks[0]));
    int colGap = MenuPx(b.u, 7);
    if (colGap < 3) colGap = 3;
    // Two columns keep the active spike instruction readable at normal and
    // narrow book sizes; the removed dead-state entries free the extra height.
    int columns = 2;
    int cardW = (w - colGap * (columns - 1)) / columns;
    if (cardW < 1) cardW = 1;
    int cardH = MenuPx(b.u, 37);
    if (cardH < 11) cardH = 11;
    int markRows = (markCount + columns - 1) / columns;
    for (int i = 0; i < markCount; i++) {
        int col = i % columns;
        int row = i / columns;
        Rectangle card = {
            (float)(x + col * (cardW + colGap)),
            (float)(y + row * cardH),
            (float)cardW,
            (float)(cardH - 1)
        };
        DrawTutorialLegendCard(card, marks[i].name, marks[i].desc,
                               marks[i].tile, marks[i].item, marks[i].icon, b, t);
    }
    y += markRows * cardH + sectionGap;
    DrawText("ITEMS", x, y, b.fsH, b.rub);
    y += b.fsH + MenuPx(b.u, 5);
    const char *itemDesc[] = {
        "Move faster", "Block one hit", "Clear nearby traps",
        "Walk through walls", "Strike nearby enemies", "Turn danger into coins"
    };
    for (int i = 0; i < IK_NONE; i++) {
        int col = i % columns;
        int row = i / columns;
        Rectangle card = {
            (float)(x + col * (cardW + colGap)),
            (float)(y + row * cardH),
            (float)cardW,
            (float)(cardH - 1)
        };
        DrawTutorialLegendCard(card, IK_NAME[i], itemDesc[i], 'I', (IKind)i,
                               -1, b, t);
    }
    DrawTutorialPageNumber(page, "II", b);
}

static const char *TutorialActionLabel(int act) {
    if (act < 0 || act >= ACT_COUNT) return "ACTION";
#ifdef DEBUG_ROOMS
    switch (act) {
        case ACT_MOVE_UP:      return "MOVE UP";
        case ACT_MOVE_DOWN:    return "MOVE DOWN";
        case ACT_MOVE_LEFT:    return "MOVE LEFT";
        case ACT_MOVE_RIGHT:   return "MOVE RIGHT";
        case ACT_USE_1:        return "USE ITEM 1";
        case ACT_USE_2:        return "USE ITEM 2";
        case ACT_USE_3:        return "USE ITEM 3";
        case ACT_USE_4:        return "USE ITEM 4";
        case ACT_USE_5:        return "USE ITEM 5";
        case ACT_USE_SELECTED: return "USE SELECTED";
        case ACT_PAUSE:        return "PAUSE";
        case ACT_FULLSCREEN:   return "FULLSCREEN";
        case ACT_CONFIRM:      return "NEXT ROUND";
        case ACT_MENU_NEXT:    return "MENU NEXT";
        case ACT_MENU_LEFT:    return "MENU LEFT";
        case ACT_MENU_RIGHT:   return "MENU RIGHT";
        case ACT_MENU_UP:      return "MENU UP";
        case ACT_MENU_DOWN:    return "MENU DOWN";
        case ACT_MENU_BACK:    return "BACK TO MENU";
        default:               return "ACTION";
    }
#else
    return ACT_LABEL[act];
#endif
}

static void DrawTutorialControlRow(Rectangle row, int act, Color accent,
                                   TutorialBookStyle b) {
    int inset = MenuPx(b.u, 5);
    if (inset < 2) inset = 2;
    DrawRectangleRounded(row, 0.10f, 5, TutorialColorA(b.paperDeep, 24));
    int strip = MenuPx(b.u, 3);
    if (strip < 1) strip = 1;
    DrawRectangle((int)row.x, (int)row.y + inset, strip,
                  (int)row.height - inset * 2, TutorialColorA(accent, 175));

    char binds[96];
    FormatActionBinds(act, binds, (int)sizeof(binds));
    int keyH = (int)row.height - MenuPx(b.u, 6);
    if (keyH < 8) keyH = 8;
    int keyMax = (int)(row.width * 0.54f);
    if (keyMax < MenuPx(b.u, 34)) keyMax = MenuPx(b.u, 34);
    int keyPad = keyH / 3;
    if (keyPad < 3) keyPad = 3;
    if (keyPad > 12) keyPad = 12;
    int keyTextMax = keyMax - keyPad * 2;
    if (keyTextMax < 1) keyTextMax = 1;
    int keyFs = TutorialFitFont(binds, b.fsB - 1, keyTextMax);
    int keyW = MeasureText(binds, keyFs) + keyPad * 2;
    if (keyW > keyMax) keyW = keyMax;
    int minKeyW = keyH * 2;
    if (minKeyW < 18) minKeyW = 18;
    if (keyW < minKeyW) keyW = (keyMax < minKeyW) ? keyMax : minKeyW;
    int rightPad = MenuPx(b.u, 6);
    int keyX = (int)(row.x + row.width) - rightPad - keyW;
    int keyY = (int)(row.y + (row.height - keyH) * 0.5f);
    if (keyX < (int)row.x + inset) keyX = (int)row.x + inset;
    DrawTutorialKeycap(keyX, keyY, keyH, binds, b.fsB - 1,
                       keyMax, accent, b.paperDeep);

    int labelX = (int)row.x + inset + strip + MenuPx(b.u, 5);
    int labelW = keyX - labelX - MenuPx(b.u, 5);
    if (labelW < 1) labelW = 1;
    int labelY = (int)(row.y + row.height / 2 - b.fsB / 2);
    DrawTutorialTextFit(TutorialActionLabel(act), labelX, labelY, b.fsB,
                        labelW, b.ink);
}

static void DrawTutorialPageThree(Rectangle page, TutorialBookStyle b) {
    int x = (int)page.x + MenuPx(b.u, 18);
    int w = (int)page.width - MenuPx(b.u, 36);
    if (w < 1) w = 1;
    int y = DrawTutorialPageHeading(page, "CONTROLS", b);
    DrawTutorialTextFit("CURRENT SETTINGS - shown exactly as configured", x, y,
                        b.fsB - 1, w, b.inkDim);
    y += b.fsB + MenuPx(b.u, 7);

    int colGap = MenuPx(b.u, 9);
    if (colGap < 3) colGap = 3;
    int colW = (w - colGap) / 2;
    if (colW < 1) colW = 1;
    int groupTitleH = b.fsH + MenuPx(b.u, 7);
    int bottom = (int)(page.y + page.height - MenuPx(b.u, 23));
    int avail = bottom - y - groupTitleH;
    int rowH = MenuPx(b.u, 25);
    if (avail > 0 && rowH * ACT_MENU_NEXT > avail)
        rowH = avail / ACT_MENU_NEXT;
    if (rowH < 9) rowH = 9;

    int leftX = x;
    int rightX = x + colW + colGap;
    DrawText("GAMEPLAY BINDS", leftX, y, b.fsH, b.rub);
    DrawText("MENU NAVIGATION", rightX, y, b.fsH, b.rub);
    int rowsY = y + groupTitleH;
    Color gameplay = (Color){ 112, 198, 232, 255 };
    Color navigation = (Color){ 176, 142, 232, 255 };
    for (int i = 0; i < ACT_MENU_NEXT; i++) {
        Rectangle row = { (float)leftX, (float)(rowsY + i * rowH),
                          (float)colW, (float)(rowH - 2) };
        DrawTutorialControlRow(row, i, gameplay, b);
    }
    int menuCount = ACT_COUNT - ACT_MENU_NEXT;
    for (int i = 0; i < menuCount; i++) {
        Rectangle row = { (float)rightX, (float)(rowsY + i * rowH),
                          (float)colW, (float)(rowH - 2) };
        DrawTutorialControlRow(row, ACT_MENU_NEXT + i, navigation, b);
    }
    DrawTutorialPageNumber(page, "III", b);
}

static Color TutorialRoomAccent(int room) {
    switch (room) {
        case RK_BOSS:      return CBOSS;
        case RK_TRAP:      return (Color){ 236, 136, 68, 255 };
        case RK_DARK:      return (Color){ 164, 125, 235, 255 };
        case RK_LAVA:      return CFIRE;
        case RK_COIN_RUSH: return CCOIN;
        default:           return (Color){ 148, 137, 116, 255 };
    }
}

static void DrawTutorialRoomGlyph(Rectangle bounds, int room, float t) {
    switch (room) {
        case RK_BOSS:
            DrawTutorialIcon(bounds, TUT_ICON_BOSS, IK_NONE, t);
            break;
        case RK_TRAP:
            DrawTutorialMapSymbol(bounds, '+', IK_NONE, t);
            break;
        case RK_DARK: {
            DrawTutorialMapSymbol(bounds, '.', IK_NONE, t);
            float side = fminf(bounds.width, bounds.height);
            float cx = bounds.x + bounds.width * 0.5f;
            float cy = bounds.y + bounds.height * 0.5f;
            DrawRectangle((int)bounds.x, (int)bounds.y, (int)side, (int)side,
                          (Color){ 8, 7, 18, 145 });
            DrawCircleV((Vector2){ cx, cy }, side * 0.27f,
                        (Color){ 188, 154, 255, 75 });
            DrawCircleV((Vector2){ cx, cy }, side * 0.11f,
                        (Color){ 226, 207, 255, 230 });
            break;
        }
        case RK_LAVA:
            DrawTutorialMapSymbol(bounds, 'L', IK_NONE, t);
            break;
        case RK_COIN_RUSH: {
            // Coin Rush uses the same two side-by-side coins as the live map,
            // rather than one centred coin plus a decorative offset coin.
            DrawTutorialMapSymbol(bounds, '.', IK_NONE, t);
            float side = fminf(bounds.width, bounds.height);
            int cx = (int)(bounds.x + bounds.width * 0.5f);
            int cy = (int)(bounds.y + bounds.height * 0.5f);
            float pulse = 0.5f + 0.5f * sinf(t * 4.0f);
            int r = (int)(side * (0.15f + 0.03f * pulse));
            if (r < 2) r = 2;
            int coinR = r - 1;
            int ox = (int)(side * 0.16f);
            if (ox < r) ox = r;
            for (int s = -1; s <= 1; s += 2) {
                int coinX = cx + s * ox;
                DrawCircle(coinX + 1, cy + 2, coinR, (Color){160, 120, 0, 80});
                DrawCircle(coinX, cy, coinR, CCOIN);
                DrawCircle(coinX - 1, cy - 1, coinR * 0.4f + 1,
                           (Color){255, 245, 160, 210});
            }
            break;
        }
        default:
            DrawTutorialMapSymbol(bounds, '.', IK_NONE, t);
            break;
    }
}

static void DrawTutorialPageFour(Rectangle page, TutorialBookStyle b, float t) {
    int x = (int)page.x + MenuPx(b.u, 18);
    int w = (int)page.width - MenuPx(b.u, 36);
    if (w < 1) w = 1;
    int y = DrawTutorialPageHeading(page, "ROOMS", b);
    DrawTutorialTextFit("Each room changes the safest way forward.", x, y,
                        b.fsB - 1, w, b.inkDim);
    y += b.fsB + MenuPx(b.u, 7);
    int colGap = MenuPx(b.u, 9);
    if (colGap < 3) colGap = 3;
    int rowGap = MenuPx(b.u, 7);
    if (rowGap < 3) rowGap = 3;
    int colW = (w - colGap) / 2;
    if (colW < 1) colW = 1;
    int bottom = (int)(page.y + page.height - MenuPx(b.u, 23));
    int rowH = (bottom - y - rowGap * 2) / 3;
    if (rowH < MenuPx(b.u, 30)) rowH = MenuPx(b.u, 30);

    struct { const char *name; const char *desc; int kind; } rooms[] = {
        { "NORMAL ROOM",    "Standard floor: Find the Treasure, avoid enemies, collect items, coins and advance.",
          RK_NORMAL },
        { "BOSS ROOM",      "Each 10 rounds: Defeat the boss by activating spike plates or firing cannons every 30 seconds. Avoid his (double)dash, fire breath, and spawns, and never touch him without a shield or you will die. Each boss grows stronger.",
          RK_BOSS },
        { "TRAP ROOM", "5% Chance after Level 5: Arrow lanes fire in timed sweeps. Watch the warning. Trapdoors that look similar to the ground kill you instantly. Lava Pools are around too. No enemies lurk in this room. Increased coins in this room. ",
          RK_TRAP },
        { "DARK ROOM", "5% Chance after Level 5: The map is dark and harder to see. Follow the glow of coins, treasures and portals and remember the path to advance. Much more coins than usual are in this Level.",
          RK_DARK },
        { "LAVA ROOM",      "5% Chance after Level 5: Lava rises from the south. Run north without stopping and advance into the next round. More coins than usual are in this Level.",
          RK_LAVA },
        { "COIN RUSH ROOM", "5% Chance after Level 5: Double coins per tile and much more tiles with coins, but a denser, riskier floor with more enemies. Pick up the special item Gold Touch to turn everything into gold for a few seconds.",
          RK_COIN_RUSH },
    };
    int iconSize = MenuPx(b.u, 54);
    if (iconSize < 12) iconSize = 12;
    for (int i = 0; i < 6; i++) {
        int col = i / 3;
        int row = i % 3;
        Rectangle card = { (float)(x + col * (colW + colGap)),
                           (float)(y + row * (rowH + rowGap)),
                           (float)colW, (float)(rowH - 1) };
        Color accent = TutorialRoomAccent(rooms[i].kind);
        DrawRectangleRounded(card, 0.10f, 5, TutorialColorA(b.paperDeep, 25));
        int strip = MenuPx(b.u, 3);
        if (strip < 1) strip = 1;
        DrawRectangle((int)card.x, (int)card.y + MenuPx(b.u, 5), strip,
                      (int)card.height - MenuPx(b.u, 10), TutorialColorA(accent, 180));
        int glyph = iconSize;
        if (glyph > (int)card.height - MenuPx(b.u, 10))
            glyph = (int)card.height - MenuPx(b.u, 10);
        if (glyph < 6) glyph = 6;
        Rectangle glyphR = { card.x + MenuPx(b.u, 8),
                             card.y + (card.height - glyph) * 0.5f,
                             (float)glyph, (float)glyph };
        DrawTutorialRoomGlyph(glyphR, rooms[i].kind, t);
        int tx = (int)(glyphR.x + glyphR.width + MenuPx(b.u, 7));
        int textW = (int)(card.x + card.width) - tx - MenuPx(b.u, 6);
        if (textW < 1) textW = 1;
        int nameY = (int)card.y + MenuPx(b.u, 7);
        DrawTutorialTextFit(rooms[i].name, tx, nameY, b.fsB, textW, accent);
        int descY = nameY + b.fsB + MenuPx(b.u, 3);
        int descBottom = (int)(card.y + card.height) - MenuPx(b.u, 6);
        int descHeight = descBottom - descY;
        if (b.fsB >= 7 && descHeight > 0) {
            DrawTutorialTextWrapped(rooms[i].desc, tx, descY, b.fsB - 1,
                                    textW, descHeight, MenuPx(b.u, 2), b.inkDim);
        }
    }
    DrawTutorialPageNumber(page, "IV", b);
}

static void DrawTutorialLockAndKeyIcons(int artifactId,int x,int y,int w,int h,
                                        TutorialBookStyle b) {
    int count=ArtifactLockOptionCount(artifactId);
    if(count<=0) {
        DrawTutorialTextFit("OPEN",x,y+(h-b.fsB)/2,b.fsB,w,b.inkDim);
        return;
    }
    int gap=MenuPx(b.u,2),groupGap=MenuPx(b.u,8);
    int arrowW=MenuPx(b.u,10);
    if(gap<1)gap=1;
    if(groupGap<2)groupGap=2;
    if(arrowW<4)arrowW=4;
    int totalIcons=count,totalKeys=0,arrows=0;
    for(int option=0;option<count;option++) {
        unsigned keys=LockedObjectAssignedKeys(ArtifactLockOptionAt(artifactId,option));
        int keyCount=0;
        for(int keyId=0;keyId<KEY_COUNT;keyId++)
            if(keys&(1u<<keyId))keyCount++;
        totalKeys+=keyCount;
        totalIcons+=keyCount;
        if(keyCount>0)arrows++;
    }
    int fixedW=arrows*arrowW+(totalKeys+arrows)*gap+(count-1)*groupGap;
    int size=h-4;
    if(size>36)size=36;
    int fit=(w-fixedW)/totalIcons;
    if(size>fit)size=fit;
    if(size<1)return;
    int usedW=totalIcons*size+fixedW;
    int iconX=x+(w-usedW)/2,iconY=y+(h-size)/2;
    for(int option=0;option<count;option++) {
        int designId=ArtifactLockOptionAt(artifactId,option);
        unsigned keys=LockedObjectAssignedKeys(designId);
        DrawRectangleRounded((Rectangle){(float)iconX,(float)iconY,
                             (float)size,(float)size},0.2f,4,
                             (Color){30,25,38,225});
        DrawLockedObjectIcon(designId,iconX,iconY,size);
        iconX+=size;
        if(keys) {
            iconX+=gap;
            DrawTutorialTextFit(">",iconX,y+(h-b.fsB)/2,b.fsB,
                                arrowW,b.inkDim);
            iconX+=arrowW+gap;
            for(int keyId=0;keyId<KEY_COUNT;keyId++)if(keys&(1u<<keyId)) {
                DrawRectangleRounded((Rectangle){(float)iconX,(float)iconY,
                                     (float)size,(float)size},0.2f,4,
                                     (Color){70,53,31,180});
                DrawUniversalKeyIcon(keyId,iconX,iconY,size,(float)GetTime());
                iconX+=size+gap;
            }
            iconX-=gap;
        }
        if(option<count-1)iconX+=groupGap;
    }
}

static void DrawArtifactLocationTooltip(int artifactId,Vector2 mouse,int sw,int sh) {
    int count=ArtifactLockOptionCount(artifactId);
    int columns=count>6?2:1;
    int rows=count>0?(count+columns-1)/columns:1;
    int width=columns==2?480:326;
    if(width>sw-16)width=sw-16;
    int height=82+rows*27;
    int x=(int)mouse.x+18,y=(int)mouse.y+14;
    if(x+width>sw-8)x=(int)mouse.x-width-18;
    if(y+height>sh-8)y=(int)mouse.y-height-14;
    if(x<8)x=8;
    if(y<8)y=8;
    Rectangle panel={(float)x,(float)y,(float)width,(float)height};
    DrawRectangleRounded((Rectangle){panel.x+3,panel.y+4,panel.width,panel.height},
                         0.08f,6,(Color){5,4,12,170});
    DrawRectangleRounded(panel,0.08f,6,(Color){24,19,35,250});
    DrawRectangleRoundedLinesEx(panel,0.08f,6,1.5f,(Color){199,151,224,255});
    DrawText(ARTIFACT_NAME[artifactId],x+12,y+9,16,(Color){245,222,251,255});
    bool mainRoom=ArtifactAppearsInLocation(artifactId,false);
    bool sideRoom=ArtifactAppearsInLocation(artifactId,true);
    const char *location=mainRoom&&sideRoom?"ARTIFACT ROOM + SIDE ROOM":
                         mainRoom?"ARTIFACT ROOM":"SIDE ROOM";
    DrawText(location,x+12,y+32,13,(Color){247,202,124,255});
    DrawLine(x+12,y+51,x+width-12,y+51,(Color){112,81,133,190});
    DrawText("POSSIBLE CHESTS",x+12,y+57,11,(Color){184,165,199,255});
    if(count==0) {
        DrawText("Open, no chest",x+12,y+79,13,(Color){195,225,203,255});
    } else {
        int columnW=(width-24)/columns;
        for(int option=0;option<count;option++) {
            int designId=ArtifactLockOptionAt(artifactId,option);
            int col=option/rows,row=option%rows;
            int rowX=x+12+col*columnW,rowY=y+77+row*27;
            DrawLockedObjectIcon(designId,rowX,rowY,22);
            DrawText(LockedObjectDisplayName(designId),rowX+28,rowY+4,12,
                     (Color){233,220,236,255});
        }
    }
}

static void TutorialKeyList(unsigned mask,char *out,size_t capacity) {
    if(mask==0) {
        snprintf(out,capacity,"No key assigned: remains sealed");
        return;
    }
    out[0]='\0';
    int listed=0;
    for(int keyId=0;keyId<KEY_COUNT;keyId++)if(mask&(1u<<keyId)) {
        size_t used=strlen(out);
        if(used<capacity)
            snprintf(out+used,capacity-used,"%s%s",listed?" / ":"",
                     UniversalKeyDisplayName(keyId));
        listed++;
    }
}

static void DrawTutorialArtifactPage(Rectangle page,TutorialBookStyle b,
                                     int half,const char *pageNumber) {
    int x=(int)page.x+MenuPx(b.u,18);
    int w=(int)page.width-MenuPx(b.u,36);
    int y=DrawTutorialPageHeading(page,half?"ARTIFACTS / II":"ARTIFACTS / I",b);
    DrawTutorialTextFit("ARTIFACT: ARTIFACT ROOM   SIDE: SIDE ROOM   BOTH: EITHER",
                        x,y,b.fsB-2,w,b.inkDim);
    y+=b.fsB+MenuPx(b.u,2);
    DrawTutorialTextFit("ARTIFACT / LOCATION                   LOCK  >  KEY",
                        x,y,b.fsB-2,w,b.inkDim);
    y+=b.fsB+MenuPx(b.u,6);
    int ids[ARTIFACT_COUNT],total=0;
    for(int id=0;id<ARTIFACT_COUNT;id++)
        if(ArtifactDesignPlayable(id))ids[total++]=id;
    int middle=(total+1)/2;
    int first=half?middle:0;
    int count=half?total-middle:middle;
    int bottom=(int)(page.y+page.height-MenuPx(b.u,25));
    int gap=MenuPx(b.u,3);
    if(gap<2)gap=2;
    int rowH=count>0?(bottom-y-gap*(count-1))/count:0;
    if(rowH>43)rowH=43;
    if(rowH<1)rowH=1;
    for(int index=0;index<count;index++) {
        int id=ids[first+index],rowY=y+index*(rowH+gap);
        Rectangle row={(float)x,(float)rowY,(float)w,(float)rowH};
        DrawRectangleRounded(row,0.12f,4,
            TutorialColorA(b.paperDeep,index%2?33:48));
        int iconSize=rowH-6;
        if(iconSize>34)iconSize=34;
        if(iconSize<6)iconSize=6;
        DrawArtifactIcon(id,(Vector2){(float)(x+iconSize/2+3),
                                       (float)(rowY+rowH/2)},(float)iconSize,true);
        int textX=x+iconSize+MenuPx(b.u,9);
        int locationX=x+w*42/100;
        int chestX=x+w*60/100;
        int chestW=x+w-chestX-MenuPx(b.u,4);
        int textW=locationX-textX-MenuPx(b.u,5);
        DrawTutorialTextFit(ARTIFACT_NAME[id],textX,
                            rowY+(rowH-b.fsB)/2,b.fsB,textW,b.ink);
        bool mainRoom=ArtifactAppearsInLocation(id,false);
        bool sideRoom=ArtifactAppearsInLocation(id,true);
        const char *location=mainRoom&&sideRoom?"BOTH":
                             mainRoom?"ARTIFACT":"SIDE";
        Color locationColor=mainRoom&&sideRoom?(Color){92,70,142,255}:
                            mainRoom?(Color){55,111,148,255}:
                                     (Color){55,126,100,255};
        Rectangle badge={(float)locationX,(float)(rowY+(rowH-19)/2),
                         (float)(chestX-locationX-MenuPx(b.u,8)),19.0f};
        DrawRectangleRounded(badge,0.32f,4,TutorialColorA(locationColor,72));
        DrawTutorialTextFit(location,(int)badge.x+MenuPx(b.u,4),
                            (int)badge.y+4,b.fsB-2,
                            (int)badge.width-MenuPx(b.u,8),locationColor);
        DrawTutorialLockAndKeyIcons(id,chestX,rowY,chestW,rowH,b);
    }
    DrawTutorialPageNumber(page,pageNumber,b);
}

static void DrawTutorialLocksAndKeys(Rectangle page,TutorialBookStyle b) {
    int x=(int)page.x+MenuPx(b.u,18);
    int w=(int)page.width-MenuPx(b.u,36);
    int y=DrawTutorialPageHeading(page,"LOCKS AND KEYS",b);
    DrawTutorialTextFit("CURRENT ASSIGNMENTS / ANY LISTED KEY OPENS THE LOCK",
                         x,y,b.fsB-1,w,b.inkDim);
    y+=b.fsB+MenuPx(b.u,7);
    bool used[LOCKED_OBJECT_DESIGN_COUNT]={false};
    used[PORTAL_ECLIPSE_IRIS_SEAL_DESIGN_ID]=true;
    for(int artifact=0;artifact<ARTIFACT_COUNT;artifact++)
        if(ArtifactDesignPlayable(artifact))
            for(int option=0;option<ArtifactLockOptionCount(artifact);option++) {
                int designId=ArtifactLockOptionAt(artifact,option);
                if(designId>=0&&designId<LOCKED_OBJECT_DESIGN_COUNT)used[designId]=true;
            }
    int count=0;
    for(int id=0;id<LOCKED_OBJECT_DESIGN_COUNT;id++)if(used[id])count++;
    int bottom=(int)(page.y+page.height-MenuPx(b.u,25));
    int rowH=count>0?(bottom-y)/count:1;
    if(rowH<1)rowH=1;
    int rowIndex=0;
    for(int id=0;id<LOCKED_OBJECT_DESIGN_COUNT;id++)if(used[id]) {
        int rowY=y+rowIndex*rowH;
        Rectangle row={(float)x,(float)rowY,(float)w,(float)(rowH-2)};
        DrawRectangleRounded(row,0.12f,4,
                             TutorialColorA(b.paperDeep,rowIndex%2?33:48));
        int iconSize=rowH-MenuPx(b.u,8);
        if(iconSize>34)iconSize=34;
        if(iconSize<8)iconSize=8;
        int iconY=rowY+(rowH-iconSize)/2;
        DrawLockedObjectIcon(id,x+MenuPx(b.u,5),iconY,iconSize);
        int textX=x+iconSize+MenuPx(b.u,13);
        int keyX=x+w*53/100;
        DrawTutorialTextFit(LockedObjectDisplayName(id),textX,
                             rowY+MenuPx(b.u,5),b.fsB,keyX-textX-MenuPx(b.u,6),b.ink);
        char keys[512];
        unsigned mask=LockedObjectAssignedKeys(id);
        TutorialKeyList(mask,keys,sizeof(keys));
        int keyIconX=keyX;
        for(int keyId=0;keyId<KEY_COUNT;keyId++)if(mask&(1u<<keyId)) {
            if(keyIconX+iconSize>x+w-MenuPx(b.u,5))break;
            DrawUniversalKeyIcon(keyId,keyIconX,iconY,iconSize,(float)GetTime());
            keyIconX+=iconSize+MenuPx(b.u,2);
        }
        DrawTutorialTextFit(keys,textX,rowY+rowH/2,
                             b.fsB-1,w-(textX-x)-MenuPx(b.u,8),b.rub);
        rowIndex++;
    }
    DrawTutorialPageNumber(page,"XVI",b);
}

static void DrawTutorialArtifactRules(Rectangle page,TutorialBookStyle b) {
    int x=(int)page.x+MenuPx(b.u,18);
    int w=(int)page.width-MenuPx(b.u,36);
    int y=DrawTutorialPageHeading(page,"DOORS, LOCKS & KEYS",b);
    struct {const char *title;const char *text;} rules[]={
        {"MATCH A KEY",
         "Only an assigned key opens a lock. It is consumed; the opening takes half a second."},
        {"NO LOCK / NO KEY",
         "An artifact with no lock appears openly. A lock with no assigned key stays sealed."},
        {"PORTAL SEAL",
         "Half of Artifact Room portals use Eclipse Iris Seal. Others are not locked."},
        {"SIDE ROOM GATE",
         "Its entrance uses the Jadeeye. A lock inside may need another key."},
        {"MISSED OR UNUSED KEYS",
         "An Artifact Room key missed in part one appears in part two. Unused keys vanish after the round."},
        {"ARTIFACT ALREADY OWNED",
         "The same reward spot holds a coin pile worth 100 Coins, even behind a lock."},
    };
    int count=(int)(sizeof(rules)/sizeof(rules[0]));
    int bottom=(int)(page.y+page.height-MenuPx(b.u,25));
    int gap=MenuPx(b.u,5);
    int rowH=(bottom-y-gap*(count-1))/count;
    if(rowH<1)rowH=1;
    for(int i=0;i<count;i++) {
        int rowY=y+i*(rowH+gap);
        Rectangle row={(float)x,(float)rowY,(float)w,(float)(rowH-1)};
        DrawRectangleRounded(row,0.11f,4,TutorialColorA(b.paperDeep,39));
        DrawRectangle(x,rowY+MenuPx(b.u,4),MenuPx(b.u,3),
                      rowH-MenuPx(b.u,8),b.rub);
        int textX=x+MenuPx(b.u,11);
        DrawTutorialTextFit(rules[i].title,textX,rowY+MenuPx(b.u,5),
                            b.fsH,w-MenuPx(b.u,20),b.ink);
        int bodyY=rowY+MenuPx(b.u,5)+b.fsH+MenuPx(b.u,3);
        DrawTutorialTextWrapped(rules[i].text,textX,bodyY,b.fsB-1,
             w-MenuPx(b.u,20),rowY+rowH-bodyY-MenuPx(b.u,5),
             MenuPx(b.u,2),b.inkDim);
    }
    DrawTutorialPageNumber(page,"XV",b);
}

#include "tutorial_book_extra.inc.c"

static void DrawTutorialPageArrow(Rectangle r, bool left, bool enabled,
                                  TutorialBookStyle b, float t) {
    Color accent = enabled ? b.gold : TutorialColorA(b.goldLo, 115);
    Color fill = enabled ? TutorialColorA(b.rub, 235)
                         : TutorialColorA((Color){ 55, 39, 37, 255 }, 150);
    DrawRectangleRounded((Rectangle){ r.x + MenuPx(b.u, 2), r.y + MenuPx(b.u, 3),
                                     r.width, r.height },
                         0.28f, 6, TutorialColorA((Color){ 35, 20, 24, 255 }, 120));
    DrawRectangleRounded(r, 0.28f, 6, fill);
    DrawRectangleRoundedLines(r, 0.28f, 6, TutorialColorA(accent, enabled ? 210 : 90));
    if (enabled) {
        float pulse = 0.5f + 0.5f * sinf(t * 2.2f + (left ? 0.0f : 1.0f));
        DrawRectangleRoundedLines(
            (Rectangle){ r.x + MenuPx(b.u, 2), r.y + MenuPx(b.u, 2),
                         r.width - MenuPx(b.u, 4), r.height - MenuPx(b.u, 4) },
            0.24f, 6, TutorialColorA(TutorialLighten(accent, 0.15f),
                                     (int)(35 + 35 * pulse)));
    }
    const char *glyph = left ? "<" : ">";
    int fs = MenuPx(b.u, 22);
    if (fs < 9) fs = 9;
    int tw = MeasureText(glyph, fs);
    DrawText(glyph, (int)(r.x + r.width / 2 - tw / 2),
             (int)(r.y + r.height / 2 - fs / 2 - 1), fs, accent);
}

static void DrawTutorialCoverArt(Rectangle cover, bool rear,
                                 TutorialBookStyle b, float t) {
    int x = (int)cover.x;
    int y = (int)cover.y;
    int w = (int)cover.width;
    int h = (int)cover.height;
    if (w < 1 || h < 1) return;

    // A book cover needs a board, a spine, a page block and a face.  Keep all
    // of those parts inside the same one-page rectangle used by the spread.
    int inset = MenuPx(b.u, 13);
    if (inset < 5) inset = 5;
    int insetMax = (int)(fminf(cover.width, cover.height) * 0.10f);
    if (insetMax < 2) insetMax = 2;
    if (inset > insetMax) inset = insetMax;
    int faceW = w - inset * 2;
    int faceH = h - inset * 2;
    if (faceW < 1) faceW = 1;
    if (faceH < 1) faceH = 1;
    Rectangle face = { cover.x + inset, cover.y + inset,
                       (float)faceW, (float)faceH };
    // Midnight blue leather gives the closed book a stronger silhouette than
    // the old burgundy cover and lets the amber/teal details read at a glance.
    Color board = rear ? (Color){ 14, 24, 39, 255 }
                       : (Color){ 24, 40, 66, 255 };
    DrawRectangleRounded(face, 0.025f, 8, board);
    DrawRectangleRoundedLines(face, 0.025f, 8,
                              TutorialColorA(b.goldLo, rear ? 185 : 220));

    int frame = MenuPx(b.u, 8);
    if (frame < 3) frame = 3;
    if (frame * 2 >= faceW || frame * 2 >= faceH)
        frame = (int)(fminf(face.width, face.height) * 0.20f);
    if (frame < 1) frame = 1;
    Rectangle inner = { face.x + frame, face.y + frame,
                        (float)(faceW - frame * 2),
                        (float)(faceH - frame * 2) };
    if (inner.width < 1) inner.width = 1;
    if (inner.height < 1) inner.height = 1;
    DrawRectangleRoundedLines(inner, 0.018f, 8,
                              TutorialColorA(b.gold, rear ? 70 : 110));

    int pageBand = MenuPx(b.u, 22);
    if (pageBand < 8) pageBand = 8;
    if (pageBand > w / 3) pageBand = w / 3;
    if (pageBand < 1) pageBand = 1;
    int pageY = y + MenuPx(b.u, 10);
    int pageH = h - (pageY - y) * 2;
    if (pageH < 1) pageH = 1;
    // A front cover is read with the fore-edge on the right.  The back cover
    // is the mirrored physical side, so its page block is on the left.
    int pageX = rear ? x : x + w - pageBand;
    DrawRectangle(pageX + MenuPx(b.u, 2), pageY + MenuPx(b.u, 3), pageBand,
                  pageH, TutorialColorA((Color){ 8, 13, 23, 255 }, 160));
    DrawRectangle(pageX, pageY, pageBand, pageH, b.paper);
    DrawRectangleLines(pageX, pageY, pageBand, pageH,
                       TutorialColorA(b.paperDeep, 185));
    int pageStep = MenuPx(b.u, 6);
    if (pageStep < 3) pageStep = 3;
    for (int py = pageY + pageStep; py < pageY + pageH; py += pageStep)
        DrawLine(pageX + 2, py, pageX + pageBand - 2, py,
                 TutorialColorA(b.paperLo, 170));

    int spineW = MenuPx(b.u, 24);
    if (spineW < 8) spineW = 8;
    if (spineW > w / 3) spineW = w / 3;
    if (spineW < 1) spineW = 1;
    int spineX = rear ? x + w - spineW : x;
    int spineY = y + MenuPx(b.u, 3);
    int spineH = h - MenuPx(b.u, 6);
    if (spineH < 1) spineH = 1;
    DrawRectangle(spineX, spineY, spineW, spineH,
                  rear ? (Color){ 8, 15, 27, 255 }
                       : (Color){ 14, 25, 43, 255 });
    DrawRectangleLines(spineX, spineY, spineW, spineH,
                       TutorialColorA(b.goldLo, 165));
    int ribCount = 3;
    for (int rib = 1; rib <= ribCount; rib++) {
        int rx = spineX + (spineW * rib) / (ribCount + 1);
        DrawLine(rx, spineY + MenuPx(b.u, 10), rx,
                 spineY + spineH - MenuPx(b.u, 10),
                 TutorialColorA(b.gold, 95));
    }

    float artLeft = rear ? face.x + MenuPx(b.u, 12)
                         : (float)(spineX + spineW + MenuPx(b.u, 12));
    float artRight = rear ? (float)(spineX - MenuPx(b.u, 12))
                          : face.x + face.width - pageBand - MenuPx(b.u, 12);
    if (artRight <= artLeft) {
        artLeft = face.x + 2;
        artRight = face.x + face.width - 2;
    }
    float artW = artRight - artLeft;
    if (artW < 1.0f) artW = 1.0f;
    float cx = (artLeft + artRight) * 0.5f;
    float side = fminf(artW, face.height);
    float pulse = 0.5f + 0.5f * sinf(t * 2.3f);
    Color metal = TutorialColorA(b.gold, rear ? 180 : 230);
    Color metalDim = TutorialColorA(b.goldLo, rear ? 135 : 180);
    Color teal = (Color){ 77, 191, 177, 255 };

    // Four small cover rivets make the board read as a bound object rather
    // than as a flat menu card.
    float rivet = fmaxf(2.0f, side * 0.018f);
    Vector2 rivets[4] = {
        { face.x + MenuPx(b.u, 9), face.y + MenuPx(b.u, 9) },
        { face.x + face.width - MenuPx(b.u, 9), face.y + MenuPx(b.u, 9) },
        { face.x + MenuPx(b.u, 9), face.y + face.height - MenuPx(b.u, 9) },
        { face.x + face.width - MenuPx(b.u, 9), face.y + face.height - MenuPx(b.u, 9) }
    };
    for (int i = 0; i < 4; i++) {
        DrawCircleV(rivets[i], rivet, TutorialColorA(b.goldLo, 170));
        DrawCircleV(rivets[i], rivet * 0.38f,
                    TutorialColorA(TutorialLighten(b.gold, 0.18f), 210));
    }

    if (!rear) {
        // Front cover: a typographic title plate and a crowned dungeon shield.  The
        // cover is intentionally more restrained than the inside legend so
        // it reads as a real book at the closed size.
        int textLeft = (int)artLeft + MenuPx(b.u, 4);
        int textRight = (int)artRight - MenuPx(b.u, 4);
        int textW = textRight - textLeft;
        if (textW < 1) textW = 1;
        int eyebrowFs = MenuPx(b.u, 9);
        if (eyebrowFs < 6) eyebrowFs = 6;
        const char *eyebrow = "FIELD GUIDE";
        eyebrowFs = TutorialFitFont(eyebrow, eyebrowFs, textW);
        int eyebrowW = MeasureText(eyebrow, eyebrowFs);
        int topY = (int)(face.y + face.height * 0.105f);
        DrawText(eyebrow, (int)(cx - eyebrowW * 0.5f), topY, eyebrowFs,
                 TutorialColorA(teal, 235));
        DrawLine(textLeft, topY + eyebrowFs + MenuPx(b.u, 7), textRight,
                 topY + eyebrowFs + MenuPx(b.u, 7),
                 TutorialColorA(b.goldLo, 190));

        const char *titleA = "DUNGEON";
        const char *titleB = "CRAWLER";
        int titleFs = MenuPx(b.u, 24);
        if (titleFs < 9) titleFs = 9;
        titleFs = TutorialFitFont(titleA, titleFs, textW);
        int titleFsB = TutorialFitFont(titleB, titleFs, textW);
        if (titleFsB < titleFs) titleFs = titleFsB;
        int titleAW = MeasureText(titleA, titleFs);
        int titleBW = MeasureText(titleB, titleFs);
        int titleY = topY + eyebrowFs + MenuPx(b.u, 16);
        DrawText(titleA, (int)(cx - titleAW * 0.5f), titleY, titleFs, metal);
        DrawText(titleB, (int)(cx - titleBW * 0.5f), titleY + titleFs + 1,
                 titleFs, TutorialLighten(metal, 0.12f));
        DrawLine((int)(cx - fminf((float)textW * 0.38f, side * 0.28f)),
                 titleY + titleFs * 2 + MenuPx(b.u, 8),
                 (int)(cx + fminf((float)textW * 0.38f, side * 0.28f)),
                 titleY + titleFs * 2 + MenuPx(b.u, 8),
                 TutorialColorA(teal, 190));

        // New front logo: a crowned dungeon shield.  It replaces the old gate
        // while keeping the same restrained amber/teal cover language.
        float logoSize = fminf(artW * 0.52f, side * 0.44f);
        if (logoSize < 22.0f) logoSize = 22.0f;
        float logoY = face.y + face.height * 0.60f;
        float logoR = logoSize * 0.50f;
        Color logoTeal = (Color){ 77, 198, 184, 255 };
        Vector2 logoCenter = { cx, logoY };
        DrawCircleV(logoCenter, logoR * (1.20f + pulse * 0.06f),
                    TutorialColorA(logoTeal, 24));
        DrawCircleV(logoCenter, logoR, TutorialColorA((Color){ 8, 17, 30, 255 }, 242));
        DrawCircleLines((int)cx, (int)logoY, logoR, TutorialColorA(metal, 225));
        DrawCircleLines((int)cx, (int)logoY, logoR * 0.83f,
                        TutorialColorA(logoTeal, 180));

        float shieldW = logoR * 1.08f;
        float shieldTopY = logoY - logoR * 0.54f;
        Vector2 shieldTop = { cx, shieldTopY };
        Vector2 shieldRight = { cx + shieldW * 0.50f, logoY - logoR * 0.16f };
        Vector2 shieldBottom = { cx, logoY + logoR * 0.58f };
        Vector2 shieldLeft = { cx - shieldW * 0.50f, logoY - logoR * 0.16f };
        DrawTriangle(shieldTop, shieldRight, shieldBottom,
                     TutorialColorA(metal, 225));
        DrawTriangle(shieldTop, shieldBottom, shieldLeft,
                     TutorialColorA(logoTeal, 220));
        DrawLineEx(shieldTop, shieldRight, fmaxf(1.0f, MenuPxf(b.u, 1.5f)),
                   TutorialColorA(TutorialLighten(metal, 0.16f), 220));
        DrawLineEx(shieldRight, shieldBottom, fmaxf(1.0f, MenuPxf(b.u, 1.5f)),
                   metalDim);
        DrawLineEx(shieldBottom, shieldLeft, fmaxf(1.0f, MenuPxf(b.u, 1.5f)),
                   TutorialColorA(logoTeal, 200));
        DrawLineEx(shieldLeft, shieldTop, fmaxf(1.0f, MenuPxf(b.u, 1.5f)),
                   metalDim);

        // Keyhole core: a small, readable mark even at the minimum UI scale.
        float lockR = fmaxf(3.0f, logoR * 0.12f);
        float lockY = logoY - logoR * 0.03f;
        DrawCircleV((Vector2){ cx, lockY }, lockR, TutorialColorA(b.gold, 235));
        DrawTriangle((Vector2){ cx - lockR * 0.82f, lockY + lockR * 0.22f },
                     (Vector2){ cx + lockR * 0.82f, lockY + lockR * 0.22f },
                     (Vector2){ cx, lockY + lockR * 1.85f },
                     TutorialColorA(b.gold, 235));
        DrawCircleV((Vector2){ cx, lockY }, lockR * 0.34f,
                    TutorialColorA((Color){ 8, 17, 30, 255 }, 230));

        // Three crown points make the new logo distinct from the former gate.
        for (int spike = 0; spike < 3; spike++) {
            float sx = cx + (spike - 1) * logoR * 0.25f;
            float baseY = logoY - logoR * 0.66f;
            float pointY = baseY - logoR * (spike == 1 ? 0.25f : 0.16f);
            DrawTriangle((Vector2){ sx - logoR * 0.13f, baseY },
                         (Vector2){ sx, pointY },
                         (Vector2){ sx + logoR * 0.13f, baseY },
                         TutorialColorA(b.gold, 225));
        }
    } else {
        // Rear cover: a large dungeon key and seal.  This is deliberately not
        // another map or a variation of the front gate, so the two sides are
        // easy to distinguish while the page block mirrors to the left.

        float keyLen = fminf(artW * 0.76f, face.height * 0.64f);
        if (keyLen < 26.0f) keyLen = 26.0f;
        float keyCy = face.y + face.height * 0.52f;
        Vector2 dir = { 0.78f, -0.63f };
        Vector2 normal = { -dir.y, dir.x };
        Vector2 keyStart = { cx - keyLen * 0.38f, keyCy + keyLen * 0.25f };
        Vector2 keyEnd = { cx + keyLen * 0.31f, keyCy - keyLen * 0.31f };
        float shaftW = fmaxf(2.0f, keyLen * 0.075f);
        float bowR = fmaxf(7.0f, keyLen * 0.15f);
        Color keyGold = TutorialColorA(TutorialLighten(b.gold, 0.08f), 235);
        Color keyShade = TutorialColorA(b.goldLo, 190);

        DrawCircleV(keyEnd, bowR * (1.35f + pulse * 0.08f),
                    TutorialColorA(teal, 24));
        DrawLineEx((Vector2){ keyStart.x + 3.0f, keyStart.y + 4.0f },
                   (Vector2){ keyEnd.x + 3.0f, keyEnd.y + 4.0f },
                   shaftW + MenuPxf(b.u, 3.0f),
                   TutorialColorA((Color){ 3, 8, 16, 190 }, 180));
        DrawLineEx(keyStart, keyEnd, shaftW, keyShade);
        DrawLineEx((Vector2){ keyStart.x - normal.x * shaftW * 0.25f,
                             keyStart.y - normal.y * shaftW * 0.25f },
                   (Vector2){ keyEnd.x - normal.x * shaftW * 0.25f,
                              keyEnd.y - normal.y * shaftW * 0.25f },
                   fmaxf(1.0f, shaftW * 0.34f), keyGold);

        DrawCircleV(keyEnd, bowR, keyShade);
        DrawCircleV(keyEnd, bowR * 0.64f,
                    TutorialColorA((Color){ 8, 16, 28, 255 }, 245));
        DrawCircleLines((int)keyEnd.x, (int)keyEnd.y, bowR, keyGold);
        DrawCircleLines((int)keyEnd.x, (int)keyEnd.y, bowR * 0.64f,
                        TutorialColorA(teal, 185));
        DrawTutorialDiamond(keyEnd.x, keyEnd.y, fmaxf(3.0f, bowR * 0.25f),
                            TutorialColorA(teal, 225));

        // Three short teeth turn the diagonal stroke into a readable key.
        for (int tooth = 0; tooth < 3; tooth++) {
            float along = keyLen * (0.08f + tooth * 0.075f);
            float toothLen = keyLen * (0.10f - tooth * 0.014f);
            Vector2 base = { keyStart.x + dir.x * along,
                             keyStart.y + dir.y * along };
            Vector2 toothEnd = { base.x + normal.x * toothLen,
                                 base.y + normal.y * toothLen };
            DrawLineEx(base, toothEnd, fmaxf(1.5f, shaftW * 0.62f), keyGold);
        }
        DrawLineEx((Vector2){ keyStart.x - dir.x * keyLen * 0.025f,
                             keyStart.y - dir.y * keyLen * 0.025f },
                   (Vector2){ keyStart.x + normal.x * keyLen * 0.11f,
                              keyStart.y + normal.y * keyLen * 0.11f },
                   fmaxf(1.5f, shaftW * 0.55f), keyGold);

        const char *backLine = "KEEP MOVING  /  FIND THE WAY OUT";
        int backFs = MenuPx(b.u, 7);
        if (backFs < 6) backFs = 6;
        int backTextW = (int)artW - MenuPx(b.u, 8);
        if (backTextW < 1) backTextW = 1;
        backFs = TutorialFitFont(backLine, backFs, backTextW);
        int backW = MeasureText(backLine, backFs);
        DrawText(backLine, (int)(cx - backW * 0.5f),
                 (int)(face.y + face.height * 0.875f), backFs,
                 TutorialColorA(b.gold, 205));
    }
}

static int DrawTutorialContents(Rectangle sidebar,TutorialBookStyle b,
                                int currentSpread) {
    static const struct {
        const char *number,*title,*shortTitle;
    } entry[]={
        {"I","HOW TO PLAY","HOW TO PLAY"},
        {"II","SYMBOLS & ITEMS","SYMBOLS"},
        {"III","CONTROLS","CONTROLS"},
        {"IV","ROOMS","ROOMS"},
        {"V","BOSS ROOM","BOSS ROOM"},
        {"VI","KNOW THE BOSSES","BOSS STATS"},
        {"VII","SIDE ROOM / MAP","SIDE MAP"},
        {"VIII","INSIDE THE SIDE ROOM","SIDE RULES"},
        {"IX","ARTIFACT ROOM / MAP","ARTIFACT MAP"},
        {"X","FIRST ARTIFACT ROOM","FIRST PART"},
        {"XI","ARTIFACT VAULT / MAP","VAULT MAP"},
        {"XII","THE ARTIFACT VAULT","VAULT RULES"},
        {"XIII","ARTIFACTS / I","ARTIFACTS I"},
        {"XIV","ARTIFACTS / II","ARTIFACTS II"},
        {"XV","DOORS, LOCKS & KEYS","LOCK RULES"},
        {"XVI","LOCKS AND KEYS","LOCKS & KEYS"},
        {"XVII","CHALLENGES","CHALLENGES"},
        {"XVIII","PROGRESS & SHOP","PROGRESS"},
        {"XIX","RECORDS & DISCOVERIES","RECORDS"},
        {"XX","WHEN YOU GET STUCK","HELP"}
    };
    const int count=(int)(sizeof(entry)/sizeof(entry[0]));
    int x=(int)sidebar.x,y=(int)sidebar.y,w=(int)sidebar.width;
    int h=(int)sidebar.height;
    if(w<1||h<1)return -1;
    int pad=MenuPx(b.u,8),headerH=MenuPx(b.u,38);
    if(pad<4)pad=4;
    if(headerH<25)headerH=25;
    int rowH=MenuPx(b.u,32);
    if(rowH<26)rowH=26;
    DrawRectangleRounded((Rectangle){sidebar.x+3,sidebar.y+4,
                                  sidebar.width,sidebar.height},
                         0.08f,6,(Color){5,8,15,150});
    DrawRectangleRounded(sidebar,0.08f,6,(Color){22,34,54,245});
    DrawRectangleRoundedLines(sidebar,0.08f,6,
                                TutorialColorA(b.gold,210));
    int headerFs=MenuPx(b.u,14);
    if(headerFs<10)headerFs=10;
    DrawTutorialTextFit("CONTENTS",x+pad,y+pad,headerFs,w-pad*2,b.gold);
    DrawLine(x+pad,y+headerH-4,x+w-pad,y+headerH-4,
             TutorialColorA(b.gold,135));
    int listY=y+headerH+pad;
    int viewportH=h-headerH-pad*2;
    if(viewportH<1)return -1;
    int maxScroll=count*rowH-viewportH;
    if(maxScroll<0)maxScroll=0;
    if(tutContentsScroll<0.0f)tutContentsScroll=0.0f;
    if(tutContentsScroll>(float)maxScroll)tutContentsScroll=(float)maxScroll;
    int scrollW=maxScroll>0?9:0;
    Rectangle viewport={(float)(x+pad),(float)listY,
                        (float)(w-pad*2-scrollW),(float)viewportH};
    if(viewport.width<1)return -1;
    Vector2 mouse=GetMousePosition();
    bool click=IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if(maxScroll>0&&CheckCollisionPointRec(mouse,sidebar)) {
        float wheel=GetMouseWheelMove();
        tutContentsScroll-=wheel*rowH*2.0f;
        if(tutContentsScroll<0.0f)tutContentsScroll=0.0f;
        if(tutContentsScroll>(float)maxScroll)tutContentsScroll=(float)maxScroll;
    }
    int trackX=x+w-pad-5,trackY=listY,trackH=viewportH;
    int thumbH=0,thumbY=trackY;
    if(maxScroll>0) {
        thumbH=(int)((float)trackH*viewportH/(count*rowH));
        if(thumbH<18)thumbH=18;
        if(thumbH>trackH)thumbH=trackH;
        int travel=trackH-thumbH;
        thumbY=trackY+(int)(travel*tutContentsScroll/maxScroll);
        Rectangle track={(float)(trackX-2),(float)trackY,9.0f,(float)trackH};
        Rectangle thumb={(float)(trackX-2),(float)thumbY,9.0f,(float)thumbH};
        if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT))tutContentsDrag=false;
        if(click&&CheckCollisionPointRec(mouse,track)) {
            tutContentsDrag=true;
            tutContentsDragOffset=CheckCollisionPointRec(mouse,thumb)?
                                  mouse.y-thumb.y:thumbH/2.0f;
        }
        if(tutContentsDrag&&travel>0) {
            float position=(mouse.y-tutContentsDragOffset-trackY)/travel;
            if(position<0.0f)position=0.0f;
            if(position>1.0f)position=1.0f;
            tutContentsScroll=position*maxScroll;
            thumbY=trackY+(int)(travel*position);
        }
    } else tutContentsDrag=false;
    int selected=-1;
    BeginScissorMode((int)viewport.x,(int)viewport.y,
                     (int)viewport.width,(int)viewport.height);
    for(int i=0;i<count;i++) {
        int rowY=listY+i*rowH-(int)tutContentsScroll;
        if(rowY+rowH<=listY||rowY>=listY+viewportH)continue;
        Rectangle row={viewport.x,(float)rowY,viewport.width,(float)(rowH-2)};
        bool active=i/2==currentSpread;
        bool hover=CheckCollisionPointRec(mouse,row)&&
                   CheckCollisionPointRec(mouse,viewport);
        if(active||hover) {
            DrawRectangleRounded(row,0.16f,4,
                active?(Color){60,95,104,225}:(Color){48,69,88,220});
            if(active)DrawRectangleRoundedLines(row,0.16f,4,
                                                 TutorialColorA(b.gold,175));
        }
        int fs=MenuPx(b.u,12);
        if(fs<8)fs=8;
        if(fs>rowH-4)fs=rowH-4;
        if(fs<6)fs=6;
        int numberW=MenuPx(b.u,25);
        if(numberW<18)numberW=18;
        int textY=rowY+(rowH-fs)/2-1;
        DrawTutorialTextFit(entry[i].number,x+pad+5,textY,fs,
                            numberW,active?b.gold:b.goldLo);
        int titleX=x+pad+numberW+5;
        DrawTutorialTextFit(w>=175?entry[i].title:entry[i].shortTitle,
                            titleX,textY,fs,
                            (int)(viewport.x+viewport.width)-4-titleX,
                            active?(Color){245,229,193,255}:
                                   (Color){190,205,210,255});
        if(hover&&click)selected=i/2;
    }
    EndScissorMode();
    if(maxScroll>0) {
        DrawRectangleRounded((Rectangle){(float)trackX,(float)trackY,
                                         5.0f,(float)trackH},
                             0.5f,4,(Color){9,16,27,220});
        DrawRectangleRounded((Rectangle){(float)trackX,(float)thumbY,
                                         5.0f,(float)thumbH},
                             0.5f,4,b.gold);
    }
    return selected;
}

static void DrawTutorialScreen(float t, int sw, int sh) {
    Color ac = (Color){ 100, 190, 255, 255 };
    DrawSubPanel("TUTORIAL", ac, t, sw, sh);
    MenuUI u = MakeUI();
    const int panelMargin = 32;
    int panelX = panelMargin;
    int panelY = panelMargin;
    int panelW = sw - panelMargin * 2;
    int panelH = sh - panelMargin * 2;
    if (panelW < 1) panelW = 1;
    if (panelH < 1) panelH = 1;

    float bookDt = GetFrameTime();
    if (tutBookWant) {
        tutBookK += bookDt / 0.42f;
    } else {
        tutBookK -= bookDt / 0.34f;
    }
    if (tutBookK < 0.0f) tutBookK = 0.0f;
    if (tutBookK > 1.0f) tutBookK = 1.0f;
    float k = EaseOutQuart(tutBookK);
    if(!tutBookWant)tutContentsOpen=false;
    if(tutBookWant&&tutBookK>=1.0f) {
        Rectangle contentsButton={(float)(panelX+panelW-61),
                                  (float)(panelY+10),40.0f,34.0f};
        bool hover=CheckCollisionPointRec(GetMousePosition(),contentsButton);
        if(hover&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            tutContentsOpen=!tutContentsOpen;
            tutContentsDrag=false;
        }
        DrawRectangleRounded(contentsButton,0.22f,5,
            tutContentsOpen?(Color){55,94,101,255}:
            hover?(Color){42,61,82,255}:(Color){24,39,64,255});
        DrawRectangleRoundedLines(contentsButton,0.22f,5,
            TutorialColorA(tutContentsOpen?(Color){205,161,72,255}:
                           (Color){100,190,255,255},hover?255:190));
        const char *mark="[I]";
        int markFs=17;
        DrawText(mark,(int)(contentsButton.x+contentsButton.width/2-
                            MeasureText(mark,markFs)/2),
                 (int)(contentsButton.y+contentsButton.height/2-markFs/2-1),
                 markFs,(Color){245,229,193,255});
    }
    if(tutContentsOpen&&tutBookWant)tutContentsK+=bookDt/0.22f;
    else tutContentsK-=bookDt/0.22f;
    if(tutContentsK<0.0f)tutContentsK=0.0f;
    if(tutContentsK>1.0f)tutContentsK=1.0f;
    float contentsK=EaseOutQuart(tutContentsK);

    // Fit the open book into the real panel content area.  The aspect ratio is
    // fixed so the pages never stretch, while both dimensions follow resize.
    int bookSidePad = MenuPx(u, 18);
    int bookTopPad = MenuPx(u, 62);
    int bookBottomPad = MenuPx(u, 72);
    if (bookSidePad < 12) bookSidePad = 12;
    if (bookTopPad < 58) bookTopPad = 58;
    if (bookBottomPad < 58) bookBottomPad = 58;
    int contentsW=panelW/5;
    if(contentsW>210)contentsW=210;
    if(contentsW<90)contentsW=90;
    int contentsGap=MenuPx(u,12);
    int arrowReserve=MenuPx(u,44);
    if(contentsGap<8)contentsGap=8;
    if(arrowReserve<32)arrowReserve=32;
    int contentsReserve=(int)((contentsW+contentsGap+arrowReserve)*k*contentsK+0.5f);
    int zoneX = panelX + bookSidePad;
    int zoneY = panelY + bookTopPad;
    int zoneW = panelW - bookSidePad * 2 - contentsReserve;
    int zoneH = panelH - bookTopPad - bookBottomPad;
    if (zoneW < 1) zoneW = 1;
    if (zoneH < 1) zoneH = 1;

    const float bookAspect = 1180.0f / 522.0f;
    int openW = zoneW;
    int openH = zoneH;
    if ((float)openW / (float)openH > bookAspect)
        openW = (int)((float)openH * bookAspect + 0.5f);
    else
        openH = (int)((float)openW / bookAspect + 0.5f);
    if (openW < 1) openW = 1;
    if (openH < 1) openH = 1;
    int openX = zoneX + (zoneW - openW) / 2;
    int openY = zoneY + (zoneH - openH) / 2;

    float bookScaleW = (float)openW / 1180.0f;
    float bookScaleH = (float)openH / 522.0f;
    float bookScale = u.s;
    if (bookScaleW < bookScale) bookScale = bookScaleW;
    if (bookScaleH < bookScale) bookScale = bookScaleH;
    if (bookScale < 0.24f) bookScale = 0.24f;
    u.s = bookScale;

    Color leather = { 24, 39, 64, 255 };
    Color leatherLo = { 9, 14, 25, 255 };
    Color leatherHi = { 42, 59, 87, 255 };
    Color gold = { 205, 161, 72, 255 };
    Color goldLo = { 116, 84, 36, 255 };
    Color paper = { 198, 180, 145, 255 };
    Color paperLo = { 169, 148, 111, 255 };
    Color paperDeep = { 118, 91, 63, 255 };
    Color ink = { 34, 25, 21, 255 };
    Color inkDim = { 77, 58, 44, 255 };
    Color rub = { 69, 158, 151, 255 };

    int fsT = MenuPx(u, 20);
    int fsH = MenuPx(u, 13);
    int fsB = MenuPx(u, 12);
    if (fsT < 10) fsT = 10;
    if (fsH < 7) fsH = 7;
    if (fsB < 6) fsB = 6;
    int bodyLine = fsB + MenuPx(u, 3);
    if (bodyLine < fsB + 2) bodyLine = fsB + 2;
    TutorialBookStyle b = { u, paper, paperLo, paperDeep, ink, inkDim, gold, goldLo,
                            rub, fsT, fsH, fsB, bodyLine };

    // Derive the closed cover from one real page.  This keeps the folded
    // book in the same proportions as the open spread instead of using a
    // separate, narrow 300px cover shape.
    int bookPagePad = MenuPx(u, 16);
    int bookPageGap = MenuPx(u, 9);
    if (bookPagePad < 3) bookPagePad = 3;
    if (bookPageGap < 2) bookPageGap = 2;
    int pageWidth = openW / 2 - bookPagePad - bookPageGap;
    int pageHeight = openH - bookPagePad * 2;
    if (pageWidth < 1) pageWidth = 1;
    if (pageHeight < 1) pageHeight = 1;
    int coverBleed = MenuPx(u, 4);
    if (coverBleed < 2) coverBleed = 2;
    int closedW = pageWidth + coverBleed * 2;
    int closedH = pageHeight + coverBleed * 2;
    if (closedW > openW) closedW = openW;
    if (closedH > openH) closedH = openH;
    if (closedW < 1) closedW = 1;
    if (closedH < 1) closedH = 1;
    int closedX = openX + (openW - closedW) / 2;
    int closedY = openY + (openH - closedH) / 2;
    float coverX = closedX + (openX - closedX) * k;
    float coverY = closedY + (openY - closedY) * k;
    float coverW = closedW + (openW - closedW) * k;
    float coverH = closedH + (openH - closedH) * k;
    Rectangle cover = { coverX, coverY, coverW, coverH };

    int coverShadowX = MenuPx(u, 6);
    int coverShadowY = MenuPx(u, 8);
    int coverInset = MenuPx(u, 9);
    int coverEdgeW = MenuPx(u, 12);
    int coverEdgeTop = MenuPx(u, 16);
    int coverEdgeBottom = MenuPx(u, 32);
    if (coverShadowX < 2) coverShadowX = 2;
    if (coverShadowY < 2) coverShadowY = 2;
    if (coverInset < 3) coverInset = 3;
    if (coverEdgeW < 3) coverEdgeW = 3;
    if (coverEdgeTop < 4) coverEdgeTop = 4;
    if (coverEdgeBottom < 8) coverEdgeBottom = 8;
    if (coverInset * 2 >= (int)cover.width) coverInset = (int)cover.width / 3;
    if (coverInset < 1) coverInset = 1;
    int innerW = (int)cover.width - coverInset * 2;
    int innerH = (int)cover.height - coverInset * 2;
    if (innerW < 1) innerW = 1;
    if (innerH < 1) innerH = 1;
    DrawRectangleRounded(
        (Rectangle){ cover.x + coverShadowX, cover.y + coverShadowY,
                     cover.width, cover.height },
        0.03f, 8, leatherLo);
    DrawRectangleRounded(cover, 0.03f, 8, leather);
    DrawRectangleRounded(
        (Rectangle){ cover.x + coverInset, cover.y + coverInset,
                     (float)innerW, (float)innerH },
        0.025f, 8, TutorialColorA(leatherHi, 95));
    DrawRectangleRoundedLines(cover, 0.03f, 8, goldLo);
    int cornerLen = MenuPx(u, 22);
    int cornerThick = MenuPx(u, 4);
    if (cornerLen > (int)cover.width / 3) cornerLen = (int)cover.width / 3;
    if (cornerLen > (int)cover.height / 3) cornerLen = (int)cover.height / 3;
    if (cornerThick < 1) cornerThick = 1;
    if (cornerLen < cornerThick) cornerLen = cornerThick;
    DrawBookCorner(cover, cornerLen, cornerThick, gold);

    if (k < 0.55f) {
        if (coverEdgeW > (int)cover.width / 3) coverEdgeW = (int)cover.width / 3;
        if (coverEdgeW < 1) coverEdgeW = 1;
        int edgeH = (int)cover.height - coverEdgeBottom;
        if (edgeH < 1) edgeH = 1;
        int edgeInset = MenuPx(u, 4);
        int edgeStrip = MenuPx(u, 8);
        if (edgeInset < 1) edgeInset = 1;
        if (edgeStrip < 1) edgeStrip = 1;
        int edgeSpace = coverEdgeW - edgeInset * 2;
        if (edgeSpace < 1) edgeSpace = 1;
        if (edgeStrip > edgeSpace) edgeStrip = edgeSpace;
        int edgeX = tutBookRear ? (int)cover.x
                                : (int)(cover.x + cover.width - coverEdgeW);
        DrawRectangle(edgeX, (int)cover.y + coverEdgeTop,
                      coverEdgeW, edgeH, paper);
        int edgeInnerH = (int)cover.height - MenuPx(u, 40);
        if (edgeInnerH < 1) edgeInnerH = 1;
        int edgeStripX = tutBookRear ? edgeX + edgeInset
                                     : edgeX + coverEdgeW - edgeInset - edgeStrip;
        DrawRectangle(edgeStripX,
                      (int)cover.y + MenuPx(u, 20), edgeStrip, edgeInnerH, paperLo);
        DrawTutorialCoverArt(cover, tutBookRear, b, t);
        if (k < 0.08f && !tutBookRear) {
            // A soft pulse marks the closed cover as clickable without
            // competing with the new title/motif on the cover itself.
            float gp = 0.5f + 0.5f * sinf(t * 2.4f);
            float pulseR = fminf(cover.width, cover.height) * (0.17f + 0.02f * gp);
            if (pulseR < 8.0f) pulseR = 8.0f;
            DrawCircleLines((int)(cover.x + cover.width * 0.5f),
                            (int)(cover.y + cover.height * 0.5f), pulseR,
                            TutorialColorA(gold, (int)(60 + 55 * gp)));
        }
        if (k < 0.35f) {
            // Only the arrow pointing toward the pages is active on each cover.
            int closedArrowW = MenuPx(u, 28);
            int closedArrowH = MenuPx(u, 46);
            int closedArrowGap = MenuPx(u, 10);
            if (closedArrowW < 20) closedArrowW = 20;
            if (closedArrowH < 24) closedArrowH = 24;
            if (closedArrowGap < 4) closedArrowGap = 4;
            int closedPrevX = (int)cover.x - closedArrowGap - closedArrowW;
            int closedNextX = (int)(cover.x + cover.width) + closedArrowGap;
            if (closedPrevX < 8) closedPrevX = 8;
            if (closedNextX + closedArrowW > sw - 8)
                closedNextX = sw - 8 - closedArrowW;
            if (closedNextX < 8) closedNextX = 8;
            int closedArrowY = (int)(cover.y + cover.height * 0.5f -
                                     closedArrowH * 0.5f);
            if (closedArrowY < panelY + 58) closedArrowY = panelY + 58;
            Rectangle closedPrevR = { (float)closedPrevX, (float)closedArrowY,
                                      (float)closedArrowW, (float)closedArrowH };
            Rectangle closedNextR = { (float)closedNextX, (float)closedArrowY,
                                      (float)closedArrowW, (float)closedArrowH };
            Vector2 closedMp = GetMousePosition();
            bool closedPrevClick = tutBookRear &&
                                   IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                                   CheckCollisionPointRec(closedMp, closedPrevR);
            bool closedNextClick = !tutBookRear &&
                                   IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                                   CheckCollisionPointRec(closedMp, closedNextR);
            bool closedArrowClick = closedPrevClick || closedNextClick;
            bool closedCoverClick = !tutBookRear &&
                                    IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                                    CheckCollisionPointRec(closedMp, cover);
            bool closedPrevKey = tutBookRear &&
                                 (ActionPressed(ACT_MENU_LEFT) ||
                                  IsKeyPressed(KEY_PAGE_UP));
            bool closedNextKey = !tutBookRear &&
                                 (ActionPressed(ACT_MENU_RIGHT) ||
                                  IsKeyPressed(KEY_PAGE_DOWN));
            if (closedArrowClick || closedCoverClick || closedPrevKey ||
                closedNextKey) {
                tutBookPage=tutBookRear?TUTORIAL_SPREAD_COUNT-1:0;
                tutBookWant = true;
            }
            DrawTutorialPageArrow(closedPrevR, true, tutBookRear, b, t);
            DrawTutorialPageArrow(closedNextR, false, !tutBookRear, b, t);
            return;
        }
    }

    int bookX = (int)cover.x;
    int bookY = (int)cover.y;
    int bookW = (int)cover.width;
    int bookH = (int)cover.height;
    int contentsSelection=-1;
    if(tutBookWant&&tutBookK>=1.0f&&tutContentsOpen&&tutContentsK>=1.0f) {
        Rectangle contents={(float)(panelX+panelW-bookSidePad-contentsW),
                            (float)bookY,(float)contentsW,(float)bookH};
        contentsSelection=DrawTutorialContents(contents,b,tutBookPage);
        if(contentsSelection>=0)tutBookPage=contentsSelection;
    }
    int spineX = bookX + bookW / 2;
    int spineW = MenuPx(u, 20);
    int spineInset = MenuPx(u, 4);
    int spineLinePad = MenuPx(u, 10);
    if (spineW < 3) spineW = 3;
    if (spineInset < 2) spineInset = 2;
    if (spineLinePad < 4) spineLinePad = 4;
    int spineH = bookH - spineInset * 2;
    if (spineH < 1) spineH = 1;
    DrawRectangle(spineX - spineW / 2, bookY + spineInset, spineW, spineH, leatherLo);
    DrawLine(spineX - MenuPx(u, 2), bookY + spineLinePad,
             spineX - MenuPx(u, 2), bookY + bookH - spineLinePad,
             (Color){ 5, 10, 19, 190 });
    DrawLine(spineX + MenuPx(u, 2), bookY + spineLinePad,
             spineX + MenuPx(u, 2), bookY + bookH - spineLinePad,
             (Color){ 64, 91, 123, 120 });

    int ribX = spineX + MenuPx(u, 7);
    int ribW = MenuPx(u, 14);
    int ribH = MenuPx(u, 70);
    DrawRectangle(ribX, bookY - 2, ribW, ribH, rub);
    DrawTriangle((Vector2){ (float)ribX, (float)(bookY + ribH) },
                 (Vector2){ (float)(ribX + ribW), (float)(bookY + ribH) },
                 (Vector2){ (float)(ribX + ribW / 2), (float)(bookY + ribH + MenuPx(u, 14)) },
                 rub);

    int pad = bookPagePad;
    int pageGap = bookPageGap;
    int pageH = bookH - pad * 2;
    int leftW = spineX - bookX - pad - pageGap;
    int rightW = bookX + bookW - pad - (spineX + pageGap);
    if (pageH < 1) pageH = 1;
    if (leftW < 1) leftW = 1;
    if (rightW < 1) rightW = 1;
    Rectangle left = { (float)(bookX + pad), (float)(bookY + pad),
                       (float)leftW, (float)pageH };
    Rectangle right = { (float)(spineX + pageGap), (float)(bookY + pad),
                        (float)rightW, (float)pageH };
    DrawTutorialPageBase(left, paper, paperLo, true);
    DrawTutorialPageBase(right, paper, paperLo, false);

    BeginTutorialPageClip(left, u);
    if (tutBookPage == 0) DrawTutorialPageOne(left, b);
    else if (tutBookPage == 1) DrawTutorialPageThree(left, b);
    else if (tutBookPage == 2) DrawTutorialBossArenaPage(left,b,t);
    else if (tutBookPage == 3) DrawTutorialFullMapPage(left,b,t,1,"VII");
    else if (tutBookPage == 4) DrawTutorialFullMapPage(left,b,t,2,"IX");
    else if (tutBookPage == 5) DrawTutorialFullMapPage(left,b,t,3,"XI");
    else if (tutBookPage == 6) DrawTutorialArtifactPage(left,b,0,"XIII");
    else if (tutBookPage == 7) DrawTutorialArtifactRules(left,b);
    else if (tutBookPage == 8) DrawTutorialChallengePage(left,b,t);
    else DrawTutorialRecordsPage(left,b);
    EndScissorMode();
    BeginTutorialPageClip(right, u);
    if (tutBookPage == 0) DrawTutorialPageTwo(right, b, t);
    else if (tutBookPage == 1) DrawTutorialPageFour(right, b, t);
    else if (tutBookPage == 2) DrawTutorialBossRosterPage(right,b,t);
    else if (tutBookPage == 3) DrawTutorialSideGuidePage(right,b,t);
    else if (tutBookPage == 4) DrawTutorialArtifactGuidePage(right,b,t);
    else if (tutBookPage == 5) DrawTutorialVaultGuidePage(right,b,t);
    else if (tutBookPage == 6) DrawTutorialArtifactPage(right,b,1,"XIV");
    else if (tutBookPage == 7) DrawTutorialLocksAndKeys(right,b);
    else if (tutBookPage == 8) DrawTutorialProgressPage(right,b);
    else DrawTutorialHelpPage(right,b);
    EndScissorMode();

    int arrowW = MenuPx(u, 28);
    int arrowH = MenuPx(u, 46);
    int arrowGap = MenuPx(u, 10);
    if (arrowW < 20) arrowW = 20;
    if (arrowH < 24) arrowH = 24;
    if (arrowGap < 4) arrowGap = 4;
    // The first spread closes from its left edge; the last from the right.
    int prevX = bookX - arrowGap - arrowW;
    int nextX = bookX + bookW + arrowGap;
    if (prevX < 8) prevX = 8;
    if (nextX + arrowW > sw - 8) nextX = sw - 8 - arrowW;
    if (nextX < 8) nextX = 8;
    int arrowY = bookY + bookH / 2 - arrowH / 2;
    if (arrowY < panelY + 58) arrowY = panelY + 58;
    Rectangle prevR = { (float)prevX, (float)arrowY,
                        (float)arrowW, (float)arrowH };
    Rectangle nextR = { (float)nextX, (float)arrowY,
                        (float)arrowW, (float)arrowH };
    Vector2 mp = GetMousePosition();
    bool prevClick = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                     CheckCollisionPointRec(mp, prevR);
    bool nextClick = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                     CheckCollisionPointRec(mp, nextR);
    bool prevKey = ActionPressed(ACT_MENU_LEFT) || IsKeyPressed(KEY_PAGE_UP);
    bool nextKey = ActionPressed(ACT_MENU_RIGHT) || IsKeyPressed(KEY_PAGE_DOWN);
    if (tutBookWant&&contentsSelection<0) {
        if (prevClick || prevKey) {
            if(tutBookPage==0) {
                tutBookRear=false;
                tutBookWant=false;
            } else tutBookPage--;
        } else if (nextClick || nextKey) {
            if(tutBookPage==TUTORIAL_SPREAD_COUNT-1) {
                tutBookRear=true;
                tutBookWant=false;
            } else tutBookPage++;
        }
    }
    DrawTutorialPageArrow(prevR, true, true, b, t);
    DrawTutorialPageArrow(nextR, false, true, b, t);

    const char *pageLabel = TextFormat("PAGES %d-%d / %d",
                                       tutBookPage*2+1,tutBookPage*2+2,
                                       TUTORIAL_PAGE_COUNT);
    int labelFs = MenuPx(u, 11);
    if (labelFs < 7) labelFs = 7;
    int labelW = MeasureText(pageLabel, labelFs);
    int labelX = spineX - labelW / 2;
    int labelY = bookY + bookH + MenuPx(u, 5);
    int backTop = panelY + panelH - 40 - 16;
    if (labelY + labelFs > backTop) labelY = backTop - labelFs - 2;
    if (labelY < panelY + 58) labelY = panelY + 58;
    DrawText(pageLabel, labelX, labelY, labelFs, TutorialColorA(gold, 220));
}

#if 0
// Retained only as a reference while the four-page codex is iterated.
static void DrawTutorialScreenLegacy(float t, int sw, int sh) {
    Color ac = (Color){ 100, 190, 255, 255 };
    DrawSubPanel("TUTORIAL", ac, t, sw, sh);
    MenuUI u = MakeUI();
    // DrawSubPanel returns the BACK button for its callers.  Recreate the
    // actual panel rectangle here; using that button as the panel made the
    // book zone collapse to a tiny, effectively invisible area.
    const int panelMargin = 32;
    int panelX = panelMargin;
    int panelY = panelMargin;
    int panelW = sw - panelMargin * 2;
    int panelH = sh - panelMargin * 2;
    int bookSidePad = MenuPx(u, 18);
    int bookTopPad = MenuPx(u, 62);
    int bookBottomPad = MenuPx(u, 72);

    // DrawSubPanel keeps a title strip and a BACK button.  Derive the book
    // zone from that returned panel instead of from the design resolution so
    // resizing the game cannot push the book into either UI element.
    if (bookSidePad < 12) bookSidePad = 12;
    if (bookTopPad < 58) bookTopPad = 58;
    if (bookBottomPad < 58) bookBottomPad = 58;
    int zoneX = panelX + bookSidePad;
    int zoneY = panelY + bookTopPad;
    int zoneW = panelW - bookSidePad * 2;
    int zoneH = panelH - bookTopPad - bookBottomPad;
    if (zoneW < 1) zoneW = 1;
    if (zoneH < 1) zoneH = 1;

    // Keep the open book's proportions instead of stretching it to whatever
    // aspect ratio the window happens to have.  The available zone is still
    // fully responsive; the book is simply fitted and centered inside it.
    const float bookAspect = 1180.0f / 522.0f;
    int openW = zoneW;
    int openH = zoneH;
    if ((float)openW / (float)openH > bookAspect)
        openW = (int)((float)openH * bookAspect + 0.5f);
    else
        openH = (int)((float)openW / bookAspect + 0.5f);
    if (openW < 1) openW = 1;
    if (openH < 1) openH = 1;
    int openX = zoneX + (zoneW - openW) / 2;
    int openY = zoneY + (zoneH - openH) / 2;

    // Use the smaller of the fitted width/height ratios as the local UI
    // scale, allowing the book to shrink with the window independently of
    // the main-menu minimum scale.
    float bookScaleW = (float)openW / 1180.0f;
    float bookScaleH = (float)openH / 522.0f;
    float bookScale = u.s;
    if (bookScaleW < bookScale) bookScale = bookScaleW;
    if (bookScaleH < bookScale) bookScale = bookScaleH;
    if (bookScale < 0.24f) bookScale = 0.24f;
    u.s = bookScale;

    // A restrained, darker codex palette: the parchment stays readable while
    // the leather and page edges carry more depth than the old flat fills.
    Color leather = { 40, 16, 22, 255 };
    Color leatherLo = { 22, 8, 13, 255 };
    Color leatherHi = { 56, 22, 28, 255 };
    Color gold = { 184, 137, 57, 255 };
    Color goldLo = { 110, 73, 30, 255 };
    Color paper = { 198, 180, 145, 255 };
    Color paperLo = { 169, 148, 111, 255 };
    Color paperDeep = { 118, 91, 63, 255 };
    Color ink = { 34, 25, 21, 255 };
    Color inkDim = { 77, 58, 44, 255 };
    Color rub = { 83, 34, 43, 255 };

    if (tutBookWant) {
        tutBookK += GetFrameTime() / 0.42f;
        if (tutBookK > 1.0f) tutBookK = 1.0f;
    }
    float k = EaseOutQuart(tutBookK);

    int closedW = MenuPx(u, 300);
    int closedH = openH - MenuPx(u, 8);
    if (closedH > MenuPx(u, 460)) closedH = MenuPx(u, 460);
    if (closedH < 1) closedH = 1;
    int closedX = openX + (openW - closedW) / 2;
    int closedY = openY + (openH - closedH) / 2;
    float coverX = closedX + (openX - closedX) * k;
    float coverY = closedY + (openY - closedY) * k;
    float coverW = closedW + (openW - closedW) * k;
    float coverH = closedH + (openH - closedH) * k;
    Rectangle cover = { coverX, coverY, coverW, coverH };

    int coverShadowX = MenuPx(u, 6);
    int coverShadowY = MenuPx(u, 8);
    int coverInset = MenuPx(u, 9);
    int coverEdgeW = MenuPx(u, 12);
    int coverEdgeTop = MenuPx(u, 16);
    int coverEdgeBottom = MenuPx(u, 32);
    int coverLineInset = MenuPx(u, 40);
    if (coverShadowX < 2) coverShadowX = 2;
    if (coverShadowY < 2) coverShadowY = 2;
    if (coverInset < 3) coverInset = 3;
    if (coverEdgeW < 3) coverEdgeW = 3;
    if (coverEdgeTop < 4) coverEdgeTop = 4;
    if (coverEdgeBottom < 8) coverEdgeBottom = 8;
    if (coverLineInset < 10) coverLineInset = 10;
    DrawRectangleRounded(
        (Rectangle){ cover.x + coverShadowX, cover.y + coverShadowY,
                     cover.width, cover.height },
        0.03f, 8, leatherLo);
    DrawRectangleRounded(cover, 0.03f, 8, leather);
    DrawRectangleRounded(
        (Rectangle){ cover.x + coverInset, cover.y + coverInset,
                     cover.width - coverInset * 2, cover.height - coverInset * 2 },
        0.025f, 8, TutorialColorA(leatherHi, 95));
    DrawRectangleRoundedLines(cover, 0.03f, 8, goldLo);
    DrawBookCorner(cover, MenuPx(u, 22), MenuPx(u, 4), gold);

    if (k < 0.55f) {
        DrawRectangle((int)(cover.x + cover.width - coverEdgeW),
                      (int)cover.y + coverEdgeTop, coverEdgeW,
                      (int)cover.height - coverEdgeBottom, paper);
        DrawRectangle((int)(cover.x + cover.width - coverEdgeW + MenuPx(u, 4)),
                      (int)cover.y + MenuPx(u, 20), MenuPx(u, 8),
                      (int)cover.height - MenuPx(u, 40), paperLo);
        const char *ttl = "DUNGEON CODEX";
        int tfs = MenuPx(u, 22);
        DrawText(ttl, (int)(cover.x + cover.width / 2 - MeasureText(ttl, tfs) / 2),
                 (int)(cover.y + cover.height * 0.38f), tfs, gold);
        DrawLine((int)(cover.x + coverLineInset),
                 (int)(cover.y + cover.height * 0.38f + tfs + MenuPx(u, 8)),
                  (int)(cover.x + cover.width - coverLineInset),
                  (int)(cover.y + cover.height * 0.38f + tfs + MenuPx(u, 8)), goldLo);
        float sealX = cover.x + cover.width * 0.5f;
        float sealY = cover.y + cover.height * 0.56f;
        DrawCircleV((Vector2){ sealX, sealY }, MenuPx(u, 22), TutorialColorA(goldLo, 90));
        DrawCircleLines((int)sealX, (int)sealY, MenuPx(u, 20), TutorialColorA(gold, 180));
        DrawTutorialDiamond(sealX, sealY, MenuPx(u, 8), TutorialColorA(gold, 220));
        DrawCircleV((Vector2){ sealX, sealY }, MenuPx(u, 3), TutorialColorA(leatherLo, 220));
        if (k < 0.08f) {
            const char *hint = "CLICK TO OPEN";
            int hfs = MenuPx(u, 16);
            float gp = 0.5f + 0.5f * sinf(t * 2.4f);
            DrawText(hint, (int)(cover.x + cover.width / 2 - MeasureText(hint, hfs) / 2),
                     (int)(cover.y + cover.height * 0.72f), hfs,
                     (Color){ gold.r, gold.g, gold.b, (uchar)(140 + 80 * gp) });
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                CheckCollisionPointRec(GetMousePosition(), cover))
                tutBookWant = true;
        }
        if (k < 0.35f) return;
    }

    int zoneXOpen = (int)cover.x;
    int zoneYOpen = (int)cover.y;
    int zoneWOpen = (int)cover.width;
    int zoneHOpen = (int)cover.height;
    zoneX = zoneXOpen; zoneY = zoneYOpen; zoneW = zoneWOpen; zoneH = zoneHOpen;

    int spineX = zoneX + zoneW / 2;
    int spineW = MenuPx(u, 20);
    int spineInset = MenuPx(u, 4);
    int spineLinePad = MenuPx(u, 10);
    if (spineInset < 2) spineInset = 2;
    if (spineLinePad < 4) spineLinePad = 4;
    DrawRectangle(spineX - spineW / 2, zoneY + spineInset, spineW,
                  zoneH - spineInset * 2, leatherLo);
    DrawLine(spineX - MenuPx(u, 2), zoneY + spineLinePad, spineX - MenuPx(u, 2),
             zoneY + zoneH - spineLinePad, (Color){ 12, 6, 8, 190 });
    DrawLine(spineX + MenuPx(u, 2), zoneY + spineLinePad, spineX + MenuPx(u, 2),
             zoneY + zoneH - spineLinePad, (Color){ 83, 42, 49, 120 });

    int ribX = spineX + MenuPx(u, 7);
    int ribW = MenuPx(u, 14);
    DrawRectangle(ribX, zoneY - 2, ribW, MenuPx(u, 70), rub);
    DrawTriangle((Vector2){ (float)ribX, (float)(zoneY + MenuPx(u, 70)) },
                 (Vector2){ (float)(ribX + ribW), (float)(zoneY + MenuPx(u, 70)) },
                 (Vector2){ (float)(ribX + ribW / 2), (float)(zoneY + MenuPx(u, 84)) },
                 rub);

    int pad = MenuPx(u, 16);
    int pageGap = MenuPx(u, 9);
    Rectangle left = {
        (float)(zoneX + pad), (float)(zoneY + pad),
        (float)(spineX - zoneX - pad - pageGap), (float)(zoneH - pad * 2)
    };
    Rectangle right = {
        (float)(spineX + pageGap), (float)(zoneY + pad),
        (float)(zoneX + zoneW - pad - (spineX + pageGap)), (float)(zoneH - pad * 2)
    };
    DrawTutorialPageBase(left, paper, paperLo, true);
    DrawTutorialPageBase(right, paper, paperLo, false);

    int fsT = MenuPx(u, 20);
    int fsH = MenuPx(u, 13);
    int fsB = MenuPx(u, 12);
    int minTitle = (u.s < 0.60f) ? 10 : 15;
    int minHeading = (u.s < 0.60f) ? 7 : 11;
    int minBody = (u.s < 0.60f) ? 6 : 10;
    if (fsT < minTitle) fsT = minTitle;
    if (fsH < minHeading) fsH = minHeading;
    if (fsB < minBody) fsB = minBody;
    int bodyLine = fsB + MenuPx(u, 3);
    if (bodyLine < fsB + 2) bodyLine = fsB + 2;

    int pageClipPad = MenuPx(u, 4);
    if (pageClipPad < 2) pageClipPad = 2;
    BeginScissorMode((int)left.x + pageClipPad, (int)left.y + pageClipPad,
                     (int)left.width - pageClipPad * 2,
                     (int)left.height - pageClipPad * 2);
    int lx = (int)left.x + MenuPx(u, 18);
    int ly = (int)left.y + MenuPx(u, 14);
    int lw = (int)left.width - MenuPx(u, 36);
    DrawText("HOW TO PLAY", lx, ly, fsT, ink);
    DrawLine(lx, ly + fsT + MenuPx(u, 4), lx + lw, ly + fsT + MenuPx(u, 4), goldLo);
    DrawLine(lx, ly + fsT + MenuPx(u, 8), lx + MenuPx(u, 44),
             ly + fsT + MenuPx(u, 8), TutorialColorA(gold, 80));
    ly += fsT + MenuPx(u, 14);

    struct { const char *h; const char *a; const char *b; const char *c; } sec[] = {
        { "THE GOAL",
          "Each round is one floor of the dungeon.",
          "Reach the Treasure to open the next floor.",
          "A Portal skips a floor if you can get to it." },
        { "DEATH",
          "Traps, lava, enemies and bosses kill on contact.",
          "Trapdoors look like normal floor. One step and you fall.",
          "A Shield blocks one hit. Without it, one mistake ends the run." },
        { "ITEMS",
          "Gems on the floor go into your inventory.",
          "Use a slot with your item keys. They are spent on use.",
          "Speed, Shield, Bomb, Ghost Walk, Sword, Gold Touch." },
        { "SPECIAL ROOMS",
          "From round 6 the dungeon mixes extra floors:",
          "Trap rooms fire arrow lanes. Dark rooms hide the map.",
          "Lava rises from the south. Coin Rush packs gold and danger." },
        { "BOSSES",
          "Every 10th round is a Boss arena.",
          "There is no Treasure until the Boss is dead.",
          "Use cannons and spike plates in the arena to hurt it." },
        { "THE DUNGEON GROWS",
          "Each round the floor gets larger.",
          "Walls, traps and enemies get worse as you go.",
          "Coins and items are how you keep up." },
    };
    int nsec = (int)(sizeof(sec) / sizeof(sec[0]));
    int leftBot = (int)(left.y + left.height - MenuPx(u, 22));
    int secGap = MenuPx(u, 4);
    if (secGap < 3) secGap = 3;
    int secH = (leftBot - ly - secGap * (nsec - 1)) / nsec;
    int secMin = fsH + bodyLine * 3 + MenuPx(u, 7);
    if (secH < secMin) secH = secMin;
    for (int i = 0; i < nsec; i++) {
        int sy = ly + i * (secH + secGap);
        if (sy + fsH + bodyLine * 3 > leftBot) break;
        Rectangle sr = { (float)(lx - MenuPx(u, 6)), (float)sy,
                         (float)(lw + MenuPx(u, 6)), (float)(secH - 1) };
        DrawRectangleRounded(sr, 0.08f, 5, TutorialColorA(paperDeep, 20));
        DrawRectangle((int)sr.x, (int)sr.y + MenuPx(u, 3), MenuPx(u, 3),
                      (int)sr.height - MenuPx(u, 6), TutorialColorA(rub, 170));
        DrawText(sec[i].h, lx + MenuPx(u, 4), sy, fsH, rub);
        DrawLine(lx + MenuPx(u, 4), sy + fsH + MenuPx(u, 2),
                 lx + MenuPx(u, 4) + MeasureText(sec[i].h, fsH) + MenuPx(u, 6),
                 sy + fsH + MenuPx(u, 2), TutorialColorA(goldLo, 150));
        int bodyX = lx + MenuPx(u, 4);
        int bodyW = lw - MenuPx(u, 4);
        int by = sy + fsH + MenuPx(u, 5);
        DrawTutorialTextFit(sec[i].a, bodyX, by, fsB, bodyW, ink);
        DrawTutorialTextFit(sec[i].b, bodyX, by + bodyLine, fsB, bodyW, inkDim);
        DrawTutorialTextFit(sec[i].c, bodyX, by + bodyLine * 2, fsB, bodyW, inkDim);
    }
    DrawText("I", (int)(left.x + left.width / 2 - 4),
             (int)(left.y + left.height - MenuPx(u, 16)), fsB, goldLo);
    EndScissorMode();

    int rx = (int)right.x + MenuPx(u, 18);
    int ry = (int)right.y + MenuPx(u, 14);
    int rw = (int)right.width - MenuPx(u, 36);
    BeginScissorMode((int)right.x + pageClipPad, (int)right.y + pageClipPad,
                     (int)right.width - pageClipPad * 2,
                     (int)right.height - pageClipPad * 2);
    DrawText("THE DUNGEON", rx, ry, fsT, ink);
    DrawLine(rx, ry + fsT + MenuPx(u, 4), rx + rw, ry + fsT + MenuPx(u, 4), goldLo);
    DrawLine(rx, ry + fsT + MenuPx(u, 8), rx + MenuPx(u, 44),
             ry + fsT + MenuPx(u, 8), TutorialColorA(gold, 80));
    ry += fsT + MenuPx(u, 12);

    DrawText("SYMBOLS", rx, ry, fsH, rub);
    ry += fsH + MenuPx(u, 5);
    struct { const char *name; const char *desc; char tile; IKind ik; int kind; } sym[] = {
        { "Treasure", "Reach it to advance a round.", 'T', IK_NONE, TUT_ICON_TREASURE },
        { "Portal",   "Skip to the next floor.",      'O', IK_NONE, TUT_ICON_PORTAL },
        { "Trap",     "Instant death on touch.",      '+', IK_NONE, TUT_ICON_TRAP },
        { "Trapdoor", "Looks like floor. You fall.",  'h', IK_NONE, TUT_ICON_TRAPDOOR },
        { "Coin",     "Score. Spend it in the Shop.", '$', IK_NONE, TUT_ICON_COIN },
        { "Item",     "Pick up, then use a slot key.",'I', IK_SPD,  TUT_ICON_ITEM },
        { "Enemy",    "Chases you. One hit kills.",   0,   IK_NONE, TUT_ICON_ENEMY },
        { "Boss",     "Kill it to drop the Treasure.",0,  IK_NONE, TUT_ICON_BOSS },
    };
    int rowGap = MenuPx(u, 5);
    if (rowGap < 3) rowGap = 3;
    int symH = MenuPx(u, 36);
    int itemH = MenuPx(u, 34);
    int ctrlH = MenuPx(u, 20);
    int minSymH = MenuPx(u, 24);
    int minItemH = MenuPx(u, 22);
    int minCtrlH = MenuPx(u, 16);
    if (minSymH < 9) minSymH = 9;
    if (minItemH < 9) minItemH = 9;
    if (minCtrlH < 7) minCtrlH = 7;
    if (symH < minSymH) symH = minSymH;
    if (itemH < minItemH) itemH = minItemH;
    if (ctrlH < minCtrlH) ctrlH = minCtrlH;
    int rightBot = (int)(right.y + right.height - MenuPx(u, 20));
    int rightNeed = 4 * symH + rowGap
                  + fsH + MenuPx(u, 5) + 3 * itemH + rowGap
                  + fsH + MenuPx(u, 5) + 5 * ctrlH;
    int rightAvail = rightBot - ry;
    while (rightNeed > rightAvail && symH > minSymH) { symH--; rightNeed--; }
    while (rightNeed > rightAvail && itemH > minItemH) { itemH--; rightNeed--; }
    while (rightNeed > rightAvail && ctrlH > minCtrlH) { ctrlH--; rightNeed--; }
    int iconSize = MenuPx(u, 29);
    int minIconSize = MenuPx(u, 22);
    if (minIconSize < 7) minIconSize = 7;
    if (iconSize < minIconSize) iconSize = minIconSize;
    if (iconSize > symH - 5) iconSize = symH - 5;
    if (iconSize > itemH - 5) iconSize = itemH - 5;
    int colGap = MenuPx(u, 8);
    if (colGap < 4) colGap = 4;
    int cardW = (rw - colGap) / 2;
    int textGap = MenuPx(u, 7);
    if (textGap < 4) textGap = 4;
    for (int i = 0; i < 8; i++) {
        int col = i / 4, row = i % 4;
        int sx = rx + col * (cardW + colGap);
        int sy = ry + row * symH;
        Rectangle card = { (float)sx, (float)sy, (float)cardW, (float)(symH - 1) };
        DrawRectangleRounded(card, 0.10f, 5, TutorialColorA(paperDeep, 24));
        DrawRectangle((int)card.x, (int)card.y + MenuPx(u, 4), MenuPx(u, 3),
                      (int)card.height - MenuPx(u, 8),
                      TutorialColorA(TutorialIconAccent(sym[i].kind, sym[i].ik), 170));
        Rectangle iconR = { card.x + MenuPx(u, 6),
                            card.y + (card.height - iconSize) * 0.5f,
                            (float)iconSize, (float)iconSize };
        DrawTutorialIcon(iconR, sym[i].kind, sym[i].ik, t);
        int tx = (int)(iconR.x + iconR.width + textGap);
        int textW = sx + cardW - tx - MenuPx(u, 5);
        DrawTutorialTextFit(sym[i].name, tx, sy + MenuPx(u, 5), fsB, textW, ink);
        DrawTutorialTextFit(sym[i].desc, tx, sy + MenuPx(u, 5) + fsB + MenuPx(u, 2),
                            fsB - 1, textW, inkDim);
    }
    ry += 4 * symH + rowGap;

    DrawText("ITEMS", rx, ry, fsH, rub);
    ry += fsH + MenuPx(u, 5);
    const char *idesc[] = {
        "Double movement speed",
        "Block one trap or hit",
        "Destroy nearby traps",
        "Walk through walls",
        "Kill nearby enemies",
        "Touch turns foes and traps to coins"
    };
    for (int i = 0; i < 6; i++) {
        int col = i % 2, row = i / 2;
        int ix = rx + col * (cardW + colGap);
        int iy = ry + row * itemH;
        Rectangle card = { (float)ix, (float)iy, (float)cardW, (float)(itemH - 1) };
        DrawRectangleRounded(card, 0.10f, 5, TutorialColorA(paperDeep, 24));
        DrawRectangle((int)card.x, (int)card.y + MenuPx(u, 4), MenuPx(u, 3),
                      (int)card.height - MenuPx(u, 8), TutorialColorA(IK_COL[i], 170));
        Rectangle iconR = { card.x + MenuPx(u, 6),
                            card.y + (card.height - iconSize) * 0.5f,
                            (float)iconSize, (float)iconSize };
        DrawTutorialIcon(iconR, TUT_ICON_ITEM, (IKind)i, t);
        int tx = (int)(iconR.x + iconR.width + textGap);
        int textW = ix + cardW - tx - MenuPx(u, 5);
        DrawTutorialTextFit(IK_NAME[i], tx, iy + MenuPx(u, 4), fsB, textW, IK_COL[i]);
        DrawTutorialTextFit(idesc[i], tx, iy + MenuPx(u, 4) + fsB + MenuPx(u, 2),
                            fsB - 1, textW, inkDim);
    }
    ry += 3 * itemH + rowGap;

    DrawText("CONTROLS", rx, ry, fsH, rub);
    ry += fsH + MenuPx(u, 5);
    char confirmB[40], pauseB[24], fullB[24], moveB[32], useB[32];
    FormatActionBinds(ACT_CONFIRM, confirmB, (int)sizeof(confirmB));
    FormatActionBinds(ACT_PAUSE, pauseB, (int)sizeof(pauseB));
    FormatActionBinds(ACT_FULLSCREEN, fullB, (int)sizeof(fullB));
    FormatMoveHint(moveB, (int)sizeof(moveB));
    FormatUseSlotHint(useB, (int)sizeof(useB));
    struct { const char *key; const char *desc; } ctrl[] = {
        { moveB,    "Move" },
        { useB,     "Use item" },
        { confirmB, "Start / next round" },
        { pauseB,   "Pause / back" },
        { fullB,    "Fullscreen" },
    };
    int ctrlBot = (int)(right.y + right.height - MenuPx(u, 20));
    int keyH = ctrlH - MenuPx(u, 3);
    int minKeyH = MenuPx(u, 14);
    if (minKeyH < 6) minKeyH = 6;
    if (keyH < minKeyH) keyH = minKeyH;
    int descX = rx + rw / 2;
    for (int i = 0; i < 5; i++) {
        int cy = ry + i * ctrlH;
        if (cy + ctrlH > ctrlBot) break;
        int keyMax = descX - rx - MenuPx(u, 6);
        DrawTutorialKeycap(rx, cy + (ctrlH - keyH) / 2, keyH, ctrl[i].key,
                           fsB - 1, keyMax, rub, paperDeep);
        DrawTutorialTextFit(ctrl[i].desc, descX, cy + (ctrlH - fsB) / 2,
                            fsB, rw - (descX - rx) - MenuPx(u, 4), ink);
    }
    DrawText("II", (int)(right.x + right.width / 2 - 6),
             (int)(right.y + right.height - MenuPx(u, 16)), fsB, goldLo);
    EndScissorMode();
}
#endif

void DrawMenu(float t) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    SetSoundtrackShopOpen(menuSub == 7);
    if (menuSub != 7) musicPreviewScrubTrack = -1;

    CheckAchievementSkinUnlocks();
    {
        Rectangle punch = { 0, 0, 0, 0 };
        if (menuSub == 0) {
            Rectangle mr[MN_COUNT];
            GetMainMenuRects(mr);
            punch = mr[MN_PLAY];
        }
        DrawMenuBG(t, sw, sh, punch);
    }

    // â”€â”€ TUTORIAL â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (menuSub == 1) {
        DrawTutorialScreen(t, sw, sh);
        return;
    }

    // â”€â”€ STATS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (menuSub == 2) {
        Color ac = (Color){255,205,55,255};
        DrawSubPanel("STATS", ac, t, sw, sh);
        int margin2=32, pw=sw-margin2*2, ph=sh-margin2*2;
        int px2=margin2, py2=margin2;

        int zoneTop    = py2 + 64;
        int zoneBottom = py2 + ph - 70;
        int availH     = zoneBottom - zoneTop;
        
        const StatsBreakdown *breakdown=GetStatsBreakdown();
        ChallengeStatsSummary challengeSummary=ChallengeGetStatsSummary();
        const RunTotals *mainTotals=&breakdown->mode[STATS_MAIN_RUN];
        const RunTotals *challengeTotals=&breakdown->mode[STATS_CHALLENGE_RUN];
        const RunTotals *seededTotals=&breakdown->mode[STATS_SEEDED_RUN];
        const ExtendedStats *extra=&breakdown->extra;
        int mainGames=breakdown->legacy.games+mainTotals->games;
        int mainRounds=breakdown->legacy.rounds+mainTotals->rounds;
        // Older coin totals include claimed achievement payouts. Derive that
        // share from the saved claim flags instead of changing historic saves.
        int achievementCoins=0;
        for(int i=0;i<ACH_COUNT;i++)for(int j=0;j<6;j++)
            if(ach_claimed[i][j])achievementCoins+=ACH_REWARDS[j];
        int mainCoinPool=breakdown->legacy.coins+mainTotals->coins;
        if(achievementCoins>mainCoinPool)achievementCoins=mainCoinPool;
        int mainCoins=mainCoinPool-achievementCoins;
        int mainTreasures=breakdown->legacy.treasures+mainTotals->treasures;
        int mainKills=breakdown->legacy.kills+mainTotals->kills;
        float avg_round=mainGames?(float)mainRounds/mainGames:0.0f;
        float avg_coi=mainGames?(float)mainCoins/mainGames:0.0f;
        float avg_trs=mainGames?(float)mainTreasures/mainGames:0.0f;
        float avg_kills=mainGames?(float)mainKills/mainGames:0.0f;
        int allGames=mainGames+challengeTotals->games+seededTotals->games;
        int allRounds=mainRounds+challengeTotals->rounds+seededTotals->rounds;
        int allCoins=mainCoins+achievementCoins+challengeTotals->coins+seededTotals->coins;
        int allKills=mainKills+challengeTotals->kills+seededTotals->kills;
        int allTreasures=breakdown->legacy.treasures+mainTotals->treasures+challengeTotals->treasures+seededTotals->treasures;
        int lethalDeaths=0,lethalRound=StatsMostLethalRound(&lethalDeaths);
        int totalLocksOpened=0;
        for(int i=0;i<KEY_COUNT;i++)totalLocksOpened+=extra->locksOpenedByKey[i];
        int totalShopSpent=0;
        for(int i=0;i<STATS_SHOP_TYPE_COUNT;i++)totalShopSpent+=extra->shopSpent[i];
        float challengeAvgMs=extra->challengeCompletionCount?
            (float)extra->challengeCompletionTimeMs/extra->challengeCompletionCount:0.0f;

        // Row/header heights
        int dataH = availH / 7;
        if (dataH < 52) dataH = 52;
        int hdrH  = 28;

        // Computed shop totals
        int stat_skins_bought = 0;
        for (int _i = 1; _i < SKIN_COUNT; _i++) if (skin_owned[_i]) stat_skins_bought++;
        int stat_upgrades_bought = spd_level + shd_level + wlk_level + swd_level + bmb_level;
        int stat_perks_bought    = perk_slots + perk_magnet + perk_shield2;

        AchievementProgress statProgress[ACH_COUNT];
        BuildAchievementProgress(statProgress);
        int stat_ach_completed = statProgress[ACH_OVERACHIEVER_INDEX].value;
        int stat_ach_maxed = statProgress[ACH_PERFECTIONIST_INDEX].value;

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
        #define GRP_CHAL  (Color){187,126,240,255}
        #define GRP_SIDE  (Color){117,205,174,255}
        #define GRP_ART   (Color){231,143,209,255}
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
        // Size the statistics from their actual entries.
        struct {
            const char *label; float val; int format; bool is_hdr;
            Color bar; Color vcol;
        } stats[] = {
            // â”€â”€ GENERAL â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            { "GAMES",                0,                           false, true,  GRP_GEN,  GRP_GEN  },
            { "Total Games Played",     (float)allGames,             false, false, GRP_GEN,  VC_GEN   },
            { "Played in Main Game",  (float)mainGames,            false, false, GRP_GEN,  VC_GEN   },
            { "Played in Challenges",  (float)challengeTotals->games,false,false,GRP_GEN, VC_GEN   },
            { "Played in Seeded Runs", (float)seededTotals->games,  false, false, GRP_GEN,  VC_GEN   },
            { "Total Rounds Played",   (float)allRounds,            false, false, GRP_GEN,  VC_GEN   },
            { "Rounds in Main Game",  (float)mainRounds,           false, false, GRP_GEN,  VC_GEN   },
            { "Rounds in Challenges", (float)challengeTotals->rounds,false,false,GRP_GEN,VC_GEN  },
            { "Rounds in Seeded Runs",(float)seededTotals->rounds, false, false, GRP_GEN,  VC_GEN   },
            { "Best Round Reached",     (float)hs_round,             false, false, GRP_GEN,  VC_GEN   },
            { "MAIN GAME RUNS",        0,                           false, true, GRP_GEN,GRP_GEN },
            { "Active Play Time",      (float)extra->activePlaySeconds,4,false,GRP_GEN,VC_GEN },
            { "Longest Run",           (float)extra->longestRunSeconds,4,false,GRP_GEN,VC_GEN },
            { "Fastest 10 Rounds",     (float)extra->fastestTenSeconds,4,false,GRP_GEN,VC_GEN },
            { "Runs Reaching Round 5", (float)extra->runsReached5,0,false,GRP_GEN,VC_GEN },
            { "Runs Reaching Round 10",(float)extra->runsReached10,0,false,GRP_GEN,VC_GEN },
            { "Runs Reaching Round 20",(float)extra->runsReached20,0,false,GRP_GEN,VC_GEN },
            { "Tiles Walked",          (float)extra->tilesWalked,0,false,GRP_GEN,VC_GEN },
            { "CHALLENGES & SEEDS",    0,                           false, true,  GRP_CHAL, GRP_CHAL },
            { "Challenges Completed",  (float)challengeSummary.completions,0,false,GRP_CHAL,GRP_CHAL },
            { "Different Challenges Completed",(float)challengeSummary.distinctChallengesCompleted,0,false,GRP_CHAL,GRP_CHAL },
            { "Challenge Attempts",    (float)challengeSummary.attempts,0,false,GRP_CHAL,GRP_CHAL },
            { "Completion Rate",       challengeSummary.attempts?100.0f*challengeSummary.completions/challengeSummary.attempts:0.0f,2,false,GRP_CHAL,GRP_CHAL },
            { "Attempts per Completion",challengeSummary.completions?(float)challengeSummary.attempts/challengeSummary.completions:0.0f,1,false,GRP_CHAL,GRP_CHAL },
            { "Seeded Runs",           (float)challengeSummary.seededRuns,0,false,GRP_CHAL,GRP_CHAL },
            { "Different Seeds Played",(float)challengeSummary.distinctSeeds,0,false,GRP_CHAL,GRP_CHAL },
            { "Different Daily Seeds",(float)challengeSummary.distinctDailySeeds,0,false,GRP_CHAL,GRP_CHAL },
            { "First-Try Challenge Clears",(float)extra->firstTryChallengeCompletions,0,false,GRP_CHAL,GRP_CHAL },
            { "Avg. Challenge Clear Time",challengeAvgMs,3,false,GRP_CHAL,GRP_CHAL },
            { "Longest Daily Streak",  (float)extra->dailyLongestStreak,0,false,GRP_CHAL,GRP_CHAL },
            // â”€â”€ COINS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            { "COINS",                  0,                           false, true,  GRP_COIN, GRP_COIN },
            { "Coin Balance",           (float)coin_balance,         false, false, GRP_COIN, VC_COIN  },
            { "Total Coins Collected",  (float)allCoins,             false, false, GRP_COIN, VC_COIN2 },
            { "Coins in Main Game",    (float)mainCoins,            false, false, GRP_COIN, VC_COIN2 },
            { "Coins from Achievements",(float)achievementCoins,    false, false, GRP_COIN, VC_COIN2 },
            { "Coins in Challenges",   (float)challengeTotals->coins,false,false, GRP_COIN, VC_COIN2 },
            { "Coins in Seeded Runs",  (float)seededTotals->coins,  false, false, GRP_COIN, VC_COIN2 },
            { "Most Coins in One Game", (float)hs_coi,               false, false, GRP_COIN, VC_COIN  },
            { "Most Coins in One Round",(float)hs_coi_round,         false, false, GRP_COIN, VC_COIN  },
            { "MAIN GAME COIN SOURCES",0,                           false, true, GRP_COIN,GRP_COIN },
            { "Ground Coins",          (float)extra->coinSources[STATS_COIN_GROUND],0,false,GRP_COIN,VC_COIN2 },
            { "Enemy Coins",           (float)extra->coinSources[STATS_COIN_ENEMY],0,false,GRP_COIN,VC_COIN2 },
            { "Boss Coins",            (float)extra->coinSources[STATS_COIN_BOSS],0,false,GRP_COIN,VC_COIN2 },
            { "Coin Piles",            (float)extra->coinSources[STATS_COIN_PILE],0,false,GRP_COIN,VC_COIN2 },
            { "Gold Touch Trap Coins", (float)extra->coinSources[STATS_COIN_GOLD_TRAP],0,false,GRP_COIN,VC_COIN2 },
            { "Coins Left on Death",  (float)extra->coinsLeftOnDeath,0,false,GRP_COIN,VC_COIN2 },
            // â”€â”€ TREASURES â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            { "TREASURES",              0,                           false, true,  GRP_TRES, GRP_TRES },
            { "Total Treasures",        (float)allTreasures,         false, false, GRP_TRES, GRP_TRES  },
            { "Most Treasures/Game",    (float)hs_trs,               false, false, GRP_TRES, GRP_TRES  },
            // â”€â”€ KILLS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            { "KILLS",                  0,                           false, true,  GRP_KILL, GRP_KILL },
            { "Total Enemy Kills",      (float)allKills,             false, false, GRP_KILL, GRP_KILL  },
            { "Kills in Main Game",    (float)mainKills,            false, false, GRP_KILL, GRP_KILL  },
            { "Kills in Challenges",   (float)challengeTotals->kills,false,false, GRP_KILL,GRP_KILL  },
            { "Kills in Seeded Runs",  (float)seededTotals->kills,  false, false, GRP_KILL, GRP_KILL  },
            { "Most Kills in One Game",(float)hs_kills,             false, false, GRP_KILL, GRP_KILL  },
            { "Kills by Sword",         (float)global_kills_sword,   false, false, GRP_KILL, GRP_KILL   },
            { "Kills by Shield",        (float)global_kills_shield,  false, false, GRP_KILL, GRP_KILL   },
            { "Gold Touch Kills",      (float)extra->goldTouchKills,0,false,GRP_KILL,GRP_KILL },
            // â”€â”€ BOSSES â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            { "BOSSES",                 0,                           false, true,  GRP_BOSS, GRP_BOSS },
            { "Bosses Defeated",        (float)global_bosses_killed, false, false, GRP_BOSS, GRP_BOSS },
            { "Boss Damage Dealt",      (float)global_boss_dmg,      false, false, GRP_BOSS, GRP_BOSS },
            { "Cannon Hits on Bosses",  (float)global_cannon_hits,   false, false, GRP_BOSS, GRP_BOSS },
            { "MAIN GAME BOSS FIGHTS",0,                            false, true, GRP_BOSS,GRP_BOSS },
            { "Boss Fights Started",   (float)extra->bossFightsStarted,0,false,GRP_BOSS,GRP_BOSS },
            { "Boss Fights Won",       (float)extra->bossFightsWon,0,false,GRP_BOSS,GRP_BOSS },
            { "Boss Win Rate",         extra->bossFightsStarted?
                100.0f*extra->bossFightsWon/extra->bossFightsStarted:0.0f,2,false,GRP_BOSS,GRP_BOSS },
            { "Fastest Boss Fight",    (float)extra->fastestBossSeconds,4,false,GRP_BOSS,GRP_BOSS },
            // â”€â”€ CHALLENGE ROOMS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            { "CHALLENGE ROOMS",        0,                           false, true,  GRP_ROOM, GRP_ROOM },
            { "Trap Rooms Cleared",     (float)global_rooms_trap,    false, false, GRP_ROOM, GRP_ROOM },
            { "Dark Rooms Cleared",     (float)global_rooms_dark,    false, false, GRP_ROOM, GRP_ROOM },
            { "Lava Rooms Cleared",     (float)global_rooms_lava,    false, false, GRP_ROOM, GRP_ROOM },
            { "Coin Rush Rooms Cleared",(float)global_rooms_coinrush,false, false, GRP_ROOM, GRP_ROOM },
            { "MAIN GAME CLEAR RATES",0,                            false, true, GRP_ROOM,GRP_ROOM },
            { "Trap Rooms Started",    (float)extra->roomStarts[STATS_ROOM_TRAP],0,false,GRP_ROOM,GRP_ROOM },
            { "Trap Clear Rate",       extra->roomStarts[STATS_ROOM_TRAP]?
                100.0f*extra->roomClears[STATS_ROOM_TRAP]/extra->roomStarts[STATS_ROOM_TRAP]:0.0f,2,false,GRP_ROOM,GRP_ROOM },
            { "Dark Rooms Started",    (float)extra->roomStarts[STATS_ROOM_DARK],0,false,GRP_ROOM,GRP_ROOM },
            { "Dark Clear Rate",       extra->roomStarts[STATS_ROOM_DARK]?
                100.0f*extra->roomClears[STATS_ROOM_DARK]/extra->roomStarts[STATS_ROOM_DARK]:0.0f,2,false,GRP_ROOM,GRP_ROOM },
            { "Lava Rooms Started",    (float)extra->roomStarts[STATS_ROOM_LAVA],0,false,GRP_ROOM,GRP_ROOM },
            { "Lava Clear Rate",       extra->roomStarts[STATS_ROOM_LAVA]?
                100.0f*extra->roomClears[STATS_ROOM_LAVA]/extra->roomStarts[STATS_ROOM_LAVA]:0.0f,2,false,GRP_ROOM,GRP_ROOM },
            { "Coin Rush Rooms Started",(float)extra->roomStarts[STATS_ROOM_COIN_RUSH],0,false,GRP_ROOM,GRP_ROOM },
            { "Coin Rush Clear Rate",  extra->roomStarts[STATS_ROOM_COIN_RUSH]?
                100.0f*extra->roomClears[STATS_ROOM_COIN_RUSH]/extra->roomStarts[STATS_ROOM_COIN_RUSH]:0.0f,2,false,GRP_ROOM,GRP_ROOM },
            { "Most Lethal Round",     (float)lethalRound,0,false,GRP_ROOM,GRP_ROOM },
            { "Deaths on That Round",  (float)lethalDeaths,0,false,GRP_ROOM,GRP_ROOM },
            { "SIDE & ARTIFACT ROOMS", 0,                           false, true, GRP_SIDE, GRP_SIDE },
            { "Side Rooms Appeared",   (float)breakdown->side.appeared,0,false,GRP_SIDE,GRP_SIDE },
            { "Side Rooms Entered",    (float)breakdown->side.entered,0,false,GRP_SIDE,GRP_SIDE },
            { "Side Rooms Skipped",    (float)breakdown->side.skipped,0,false,GRP_SIDE,GRP_SIDE },
            { "Side Gates Opened",     (float)breakdown->side.gatesOpened,0,false,GRP_SIDE,GRP_SIDE },
            { "Artifact Portals Entered",(float)breakdown->artifact.portalsEntered,0,false,GRP_ART,GRP_ART },
            { "First Arenas Survived", (float)breakdown->artifact.arenasSurvived,0,false,GRP_ART,GRP_ART },
            { "Artifact Ladders Used", (float)breakdown->artifact.laddersUsed,0,false,GRP_ART,GRP_ART },
            { "Artifact Vaults Unlocked",(float)breakdown->artifact.vaultsReached,0,false,GRP_ART,GRP_ART },
            { "Artifact Treasures Collected",(float)breakdown->artifact.treasuresCollected,0,false,GRP_ART,GRP_ART },
            { "Fastest Artifact Recovery",(float)breakdown->artifact.fastestRecoveryMs,3,false,GRP_ART,GRP_ART },
            // â”€â”€ DEATHS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            { "DEATHS",                 0,                           false, true,  GRP_DETH, GRP_DETH },
            { "Total Deaths",           (float)global_games_played,  false, false, GRP_DETH, GRP_DETH  },
            { "Deaths by Enemy",        (float)global_deaths_enemy,  false, false, GRP_DETH, GRP_DETH  },
            { "Deaths by Boss",         (float)global_deaths_boss,   false, false, GRP_DETH, GRP_DETH  },
            { "Deaths by Trap",         (float)global_deaths_trap,   false, false, GRP_DETH, GRP_DETH  },
            { "Deaths by Trapdoor",     (float)global_deaths_trapdoor,false, false, GRP_DETH, GRP_DETH  },
            { "Deaths by Lava",         (float)global_deaths_lava,   false, false, GRP_DETH, GRP_DETH  },
            { "Deaths by Quitting",     (float)(global_games_played - global_deaths_enemy - global_deaths_boss - global_deaths_trap - global_deaths_trapdoor - global_deaths_lava), false, false, GRP_DETH, GRP_DETH  },
            // â”€â”€ GAMEPLAY â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            { "GAMEPLAY",            0,                           false, true,  GRP_EXPL, GRP_EXPL },
            { "Portals Entered",        (float)global_portals,        false, false, GRP_EXPL, GRP_EXPL  },
            { "Keys Collected",        (float)breakdown->keysCollected,0,false,GRP_EXPL,GRP_EXPL },
            { "Keys Spent",            (float)breakdown->keysSpent,0,false,GRP_EXPL,GRP_EXPL },
            { "Key Usage Rate",        breakdown->keysCollected?
                100.0f*breakdown->keysSpent/breakdown->keysCollected:0.0f,2,false,GRP_EXPL,GRP_EXPL },
            { "Keys Lost Unused",      (float)extra->unusedKeys,0,false,GRP_EXPL,GRP_EXPL },
            { "Locks Opened by Key",   (float)totalLocksOpened,0,false,GRP_EXPL,GRP_EXPL },
            { UniversalKeyDisplayName(KEY_JADEEYE),(float)extra->locksOpenedByKey[KEY_JADEEYE],0,false,GRP_EXPL,GRP_EXPL },
            { UniversalKeyDisplayName(KEY_STARFORGE),(float)extra->locksOpenedByKey[KEY_STARFORGE],0,false,GRP_EXPL,GRP_EXPL },
            { UniversalKeyDisplayName(KEY_RIFT_THORN),(float)extra->locksOpenedByKey[KEY_RIFT_THORN],0,false,GRP_EXPL,GRP_EXPL },
            { UniversalKeyDisplayName(KEY_BONE_RELIQUARY),(float)extra->locksOpenedByKey[KEY_BONE_RELIQUARY],0,false,GRP_EXPL,GRP_EXPL },
            { UniversalKeyDisplayName(KEY_SUNBREAKER),(float)extra->locksOpenedByKey[KEY_SUNBREAKER],0,false,GRP_EXPL,GRP_EXPL },
            { UniversalKeyDisplayName(KEY_MOONVEIL),(float)extra->locksOpenedByKey[KEY_MOONVEIL],0,false,GRP_EXPL,GRP_EXPL },
            { UniversalKeyDisplayName(KEY_VERDANT_COIL),(float)extra->locksOpenedByKey[KEY_VERDANT_COIL],0,false,GRP_EXPL,GRP_EXPL },
            { UniversalKeyDisplayName(KEY_FROSTGLASS),(float)extra->locksOpenedByKey[KEY_FROSTGLASS],0,false,GRP_EXPL,GRP_EXPL },
            { UniversalKeyDisplayName(KEY_CLOCKHEART),(float)extra->locksOpenedByKey[KEY_CLOCKHEART],0,false,GRP_EXPL,GRP_EXPL },
            { UniversalKeyDisplayName(KEY_ABYSSAL_TRIDENT),(float)extra->locksOpenedByKey[KEY_ABYSSAL_TRIDENT],0,false,GRP_EXPL,GRP_EXPL },
            { UniversalKeyDisplayName(KEY_DRAGONFANG),(float)extra->locksOpenedByKey[KEY_DRAGONFANG],0,false,GRP_EXPL,GRP_EXPL },
            { UniversalKeyDisplayName(KEY_STORM_COMPASS),(float)extra->locksOpenedByKey[KEY_STORM_COMPASS],0,false,GRP_EXPL,GRP_EXPL },
            { UniversalKeyDisplayName(KEY_MOURNING_BELL),(float)extra->locksOpenedByKey[KEY_MOURNING_BELL],0,false,GRP_EXPL,GRP_EXPL },
            { UniversalKeyDisplayName(KEY_WARDENS_OATH),(float)extra->locksOpenedByKey[KEY_WARDENS_OATH],0,false,GRP_EXPL,GRP_EXPL },
            { UniversalKeyDisplayName(KEY_MAGMA_CROWN),(float)extra->locksOpenedByKey[KEY_MAGMA_CROWN],0,false,GRP_EXPL,GRP_EXPL },
            { "Ghost Walk Passes",      (float)global_ghostwalk_walls,false, false, GRP_EXPL, GRP_EXPL   },
            { "Inventory Full Misses",  (float)global_inv_full,      false, false, GRP_EXPL, GRP_EXPL  },
            // â”€â”€ ITEMS USED â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            { "ITEMS USED",             0,                           false, true,  GRP_ITEM, GRP_ITEM },
            { "Speed Items Used",       (float)global_use_spd,       false, false, GRP_ITEM, GRP_ITEM   },
            { "Shield Items Used",      (float)global_use_shd,       false, false, GRP_ITEM, GRP_ITEM   },
            { "Bomb Items Used",        (float)global_use_bmb,       false, false, GRP_ITEM, GRP_ITEM   },
            { "Ghost Walk Items Used",  (float)global_use_wlk,       false, false, GRP_ITEM, GRP_ITEM   },
            { "Sword Items Used",       (float)global_use_swd,       false, false, GRP_ITEM, GRP_ITEM   },
            { "Gold Touch Items Used",  (float)global_use_gld,       false, false, GRP_ITEM, GRP_ITEM   },
            // â”€â”€ SHOP â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            { "SHOP",                        0,                           false, true,  GRP_SHOP, GRP_SHOP },
            { "Skins Bought",                (float)stat_skins_bought,    false, false, GRP_SHOP, GRP_SHOP },
            { "Upgrades Bought",             (float)stat_upgrades_bought, false, false, GRP_SHOP, GRP_SHOP },
            { "Perks Bought",                (float)stat_perks_bought,    false, false, GRP_SHOP, GRP_SHOP },
            { "Total Shop Coins Spent",      (float)totalShopSpent,0,false,GRP_SHOP,GRP_SHOP },
            { "Coins Spent on Skins",        (float)extra->shopSpent[STATS_SHOP_SKINS],0,false,GRP_SHOP,GRP_SHOP },
            { "Coins Spent on Upgrades",     (float)extra->shopSpent[STATS_SHOP_UPGRADES],0,false,GRP_SHOP,GRP_SHOP },
            { "Coins Spent on Perks",        (float)extra->shopSpent[STATS_SHOP_PERKS],0,false,GRP_SHOP,GRP_SHOP },
            { "Coins Spent on Music",        (float)extra->shopSpent[STATS_SHOP_MUSIC],0,false,GRP_SHOP,GRP_SHOP },
            // â”€â”€ ACHIEVEMENTS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            { "ACHIEVEMENTS",                0,                           false, true,  GRP_ACH,  GRP_ACH  },
            { "Achievements Completed",      (float)stat_ach_completed,   false, false, GRP_ACH,  GRP_ACH  },
            { "Achievements Maxed", (float)stat_ach_maxed,       false, false, GRP_ACH,  GRP_ACH  },
            // â”€â”€ AVERAGES â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            { "AVERAGES (MAIN GAME)",0,                       false, true,  GRP_AVG,  GRP_AVG  },
            { "Avg. Rounds per Game",   avg_round,                   true,  false, GRP_AVG,  VC_AVG   },
            { "Avg. Coins per Game",    avg_coi,                     true,  false, GRP_AVG,  VC_AVG   },
            { "Avg. Treasures per Game",avg_trs,                     true,  false, GRP_AVG,  VC_AVG   },
            { "Avg. Kills per Game",    avg_kills,                   true,  false, GRP_AVG,  VC_AVG   },
        };

        int playableArtifacts=0;
        for(int i=0;i<ARTIFACT_COUNT;i++)if(ArtifactDesignPlayable(i))playableArtifacts++;
        int artifactGridRows=(playableArtifacts+5)/6;
        static unsigned expandedStats=0;
        int statCount=(int)(sizeof(stats)/sizeof(stats[0]));
        int cx=px2+40;
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            Vector2 mouse=GetMousePosition();
            if(mouse.x>=cx-8&&mouse.x<cx+pw-72&&mouse.y>=zoneTop&&mouse.y<zoneBottom){
                int rowY=zoneTop-statScroll;
                for(int i=0;i<statCount;i++){
                    unsigned detail=StatsDetailGroup(stats[i].label);
                    if(detail&&!(expandedStats&detail))continue;
                    int rowH=stats[i].is_hdr?hdrH:dataH;
                    unsigned toggle=StatsToggleGroup(stats[i].label);
                    if(toggle&&mouse.y>=rowY&&mouse.y<rowY+rowH){
                        expandedStats^=toggle;
                        break;
                    }
                    rowY+=rowH;
                }
            }
        }
        // Only visible detail rows contribute to the scroll height.
        int totalH=114+artifactGridRows*144;
        for(int i=0;i<statCount;i++){
            unsigned detail=StatsDetailGroup(stats[i].label);
            if(detail&&!(expandedStats&detail))continue;
            totalH+=stats[i].is_hdr?hdrH:dataH;
        }
        int maxScroll = totalH - availH;
        if (maxScroll < 0) maxScroll = 0;
        float wheel = GetMouseWheelMoveV().y;
        if (wheel != 0.0f) statScroll -= (int)(wheel * 40.0f);
        if (!menuNavOn && IsKeyDown(KEY_UP))   statScroll -= 7;
        if (!menuNavOn && IsKeyDown(KEY_DOWN)) statScroll += 7;
        if (statScroll < 0) statScroll = 0;
        if (statScroll > maxScroll) statScroll = maxScroll;

        int dataIdx = 0; // for alternating row bg
        Vector2 statsMouse=GetMousePosition();

        BeginScissorMode(px2, zoneTop, pw, availH);
        int cy2 = zoneTop - statScroll;
        for (int i = 0; i < statCount; i++) {
            unsigned detail=StatsDetailGroup(stats[i].label);
            if(detail&&!(expandedStats&detail))continue;
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
                unsigned toggle=StatsToggleGroup(stats[i].label);
                bool hovered=toggle&&statsMouse.x>=cx-8&&statsMouse.x<cx+pw-72&&
                    statsMouse.y>=ry2&&statsMouse.y<ry2+entH&&
                    statsMouse.y>=zoneTop&&statsMouse.y<zoneBottom;
                Color rowbg = (dataIdx % 2 == 0) ? (Color){20,16,34,140}
                                                  : (Color){14,11,24,70};
                if(hovered)rowbg=(Color){47,39,64,205};
                DrawRectangleRounded(
                    (Rectangle){(float)(cx-8),(float)(ry2-2),(float)(pw-64),(float)(dataH-4)},
                    0.12f, 6, rowbg);
                DrawRectangleRounded(
                    (Rectangle){(float)(cx-8),(float)(ry2-2),4.0f,(float)(dataH-4)},
                    1.0f, 4, stats[i].bar);
                int textX=cx+6+(detail?18:0);
                DrawText(stats[i].label,textX,ry2+2,16,
                         detail?(Color){151,145,180,215}:(Color){140,135,170,210});
                if(toggle){
                    int buttonX=cx+pw-122;
                    DrawRectangleRounded((Rectangle){(float)buttonX,(float)(ry2+7),32,28},
                                         0.25f,5,hovered?(Color){89,73,118,255}:(Color){47,38,67,230});
                    DrawRectangleRoundedLinesEx((Rectangle){(float)buttonX,(float)(ry2+7),32,28},
                                                0.25f,5,1.0f,stats[i].bar);
                    DrawText((expandedStats&toggle)?"-":"+",buttonX+10,ry2+7,23,stats[i].vcol);
                }
                const char *value=stats[i].format==1?TextFormat("%.2f",stats[i].val):
                    stats[i].format==2?TextFormat("%.1f%%",stats[i].val):
                    stats[i].format==3?(stats[i].val>0?TextFormat("%.2f s",stats[i].val/1000.0f):"--"):
                    stats[i].format==4?(stats[i].val>0?TextFormat("%d:%02d:%02d",
                        (int)stats[i].val/3600,((int)stats[i].val/60)%60,(int)stats[i].val%60):"--"):
                    TextFormat("%d",(int)stats[i].val);
                DrawText(value,
                         textX, ry2+22, 24, stats[i].vcol);
                dataIdx++;
            }
        }
        // A separate collection display below the final statistics section.
        int collectionY=cy2+24;
        int owned=0;
        for(int i=0;i<ARTIFACT_COUNT;i++)
            if(ArtifactDesignPlayable(i))owned+=artifactOwned[i]!=0;
        DrawLine(cx-8,collectionY,px2+pw-24,collectionY,(Color){98,65,116,255});
        DrawText(TextFormat("ARTIFACTS  %d / %d",owned,playableArtifacts),cx,collectionY+14,18,
                 (Color){237,197,255,255});
        Rectangle artifactBar={(float)(cx-8),(float)(collectionY+40),
                               (float)(pw-64),11.0f};
        DrawRectangleRounded(artifactBar,0.35f,6,(Color){26,19,37,255});
        if(playableArtifacts>0&&owned>0){
            Rectangle filled=artifactBar;
            filled.width*=((float)owned/(float)playableArtifacts);
            DrawRectangleRounded(filled,0.35f,6,
                                 (Color){205,112,238,255});
        }
        for(int marker=1;marker<playableArtifacts;marker++){
            int mx=(int)(artifactBar.x+artifactBar.width*marker/playableArtifacts);
            DrawLine(mx,collectionY+42,mx,collectionY+49,
                     (Color){12,9,21,155});
        }
        DrawRectangleRoundedLinesEx(artifactBar,0.35f,6,1.0f,
                                    (Color){143,81,172,255});
        float cardW=(pw-64.0f)/6.0f;
        int visibleArtifact=0;
        int hoveredArtifact=-1;
        for(int i=0;i<ARTIFACT_COUNT;i++) {
            if(!ArtifactDesignPlayable(i))continue;
            int col=visibleArtifact%6,row=visibleArtifact/6;
            float x=cx-8+col*cardW,mid=x+cardW*.5f;
            int cardY=collectionY+66+row*144;
            bool found=artifactOwned[i]!=0;
            Rectangle card={x+3,(float)cardY,cardW-6,138};
            bool hovered=statsMouse.y>=zoneTop&&statsMouse.y<zoneBottom&&
                         CheckCollisionPointRec(statsMouse,card);
            if(hovered)hoveredArtifact=i;
            DrawRectangleRounded(card,.08f,4,
                                  found?(Color){40,28,54,255}:(Color){22,19,30,255});
            if(hovered)DrawRectangleRoundedLinesEx(card,.08f,4,1.5f,
                                                   (Color){223,169,244,255});
            float iconSize=fminf(62,cardW*.52f);
            DrawArtifactIcon(i,(Vector2){mid,(float)cardY+40},iconSize,found);
            int split=TextFindIndex(ARTIFACT_NAME[i]," ");
            const char *first=TextSubtext(ARTIFACT_NAME[i],0,split);
            const char *second=ARTIFACT_NAME[i]+split+1;
            int font=14;
            while(font>9 && (MeasureText(first,font)>cardW-12 || MeasureText(second,font)>cardW-12)) font--;
            Color label=found?(Color){240,226,247,255}:(Color){157,147,167,255};
            DrawText(first,mid-MeasureText(first,font)*.5f,cardY+77,font,label);
            DrawText(second,mid-MeasureText(second,font)*.5f,cardY+95,font,label);
            const char *status=found?"Collected":"Not found";
            DrawText(status,mid-MeasureText(status,11)*.5f,cardY+117,11,label);
            visibleArtifact++;
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
        if(hoveredArtifact>=0)
            DrawArtifactLocationTooltip(hoveredArtifact,statsMouse,sw,sh);
        return;
    }

    // â”€â”€ SHOP â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (menuSub == 3) {
        Color ac = (Color){70,215,140,255};
        DrawSubPanel("SHOP", ac, t, sw, sh);
        int margin2=32, pw=sw-margin2*2, ph=sh-margin2*2;
        int px2=margin2, py2=margin2;
        

        // â”€â”€ coin balance header â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        int balY = py2 + 64;
        DrawCircle(px2+pw-80, balY+10, 9, CCOIN);
        DrawCircle(px2+pw-83, balY+7,  4, (Color){255,245,160,200});
        int balw = MeasureText(TextFormat("%d", coin_balance), 20);
        DrawText(TextFormat("%d", coin_balance), px2+pw-65, balY+1, 20, CCOIN);
        (void)balw;

        // â”€â”€ tab bar â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
            Rectangle tr={(float)tx,(float)tabY,(float)tabW,34};
            if (menuNavOn && subNavFocus == 1 + i)
                DrawMenuFocusRing(tr, tabAc[i], t);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mp2=GetMousePosition();
                if (CheckCollisionPointRec(mp2,tr)) { shopTab=i; upgScroll=0; }
            }
        }

        int cZoneTop  = tabY + 44;
        int cZoneX    = px2 + 12;
        int cZoneW    = pw - 24;

        // â”€â”€ TAB 0: SKINS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        if (shopTab == 0) {
            static const char* SKIN_NAMES[SKIN_COUNT] = {
                // Common (0-21)
                "Green","Red","Orange","Yellow","Lime",
                "Cyan","Blue","Violet","Magenta","White",
                "Brown","Black","Pink","Grey",
                // Former Uncommon skins, now Common (14-21)
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
                // Rare (34)
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
                "Black Hole",
        // Accepted design ideas (43-48)
                "Ember Fox","Veil Dancer","Ossuary Knight",
                "Glass Seraph","Gilt Jackal","Ember Marrow",
                // Selected Player Skin Fixes (49-51)
                "Glass Echo","Cinder Sovereign","Gilded Oracle"
            };
            static const int SKIN_PRICES[SKIN_COUNT] = {
                // Common (0-13)
                100, 100, 100, 100, 100,
                100, 100, 100, 100, 100,
                100, 100, 100, 100,
                // Common (14-21)
                150, 150, 150, 150, 150,
                150, 150, 150,
                // Rare (22-26), prices unchanged
                600, 600, 400, 400, 600,
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
                // Rare Constellation (34), price unchanged
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
        // Accepted design ideas (43-48): Gilt Jackal Uncommon, Ember Marrow Rare
                400, 400, 400, 400, 400, 400,
                // Selected Player Skin Fixes (49-51)
                400, 500, 500,
            };
            // Unlock hint shown on hover for achievement-locked skins (index = id-30).
            static const char* ACH_SKIN_HINT[4] = {
                "Unlocks at Tier 6: Veteran",
                "Unlocks at Tier 6: Explorer",
                "Unlocks at Tier 6: Coin Hoarder & Rich",
                "Unlocks at Tier 6: Slayer",
            };
            static const char* SUNFORGED_UNLOCK_HINT =
                "Unlocks at Tier 6: Overachiever";

            int cellSz  = 110, cellGap = 16;
            int perRow  = 5;
            int rowOff  = (cZoneW - (perRow * cellSz + (perRow-1) * cellGap)) / 2;

            int catHdrH  = 28; 
            int catGap   = 8;

            int skinRowH     = cellSz + cellGap + 28 + 10;
            int commonRows   = 5;   // 0-21 (22 skins)
            int uncommonRows = 2;   // 43-47, 49-51 (8 skins)
            int rareRows     = 3;   // 22-26, 28, 30-31, 33-35, 48 (12 skins)
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
            if (!menuNavOn && IsKeyDown(KEY_UP))   skinScroll -= 7;
            if (!menuNavOn && IsKeyDown(KEY_DOWN)) skinScroll += 7;
            if (skinScroll < 0) skinScroll = 0;
            if (skinScroll > maxScrollSkin) skinScroll = maxScrollSkin;

            BeginScissorMode(cZoneX, skinZoneTop, cZoneW, availH_skin);

            int curY = skinZoneTop - skinScroll + 8;

            // â”€â”€ COMMON skins â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

                for (int i=0; i<22; i++) {
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
                            StatsShopSpent(STATS_SHOP_SKINS,price);
                            SaveStats(); SaveShop();
                        }
                    }
                }
                curY += commonRows * skinRowH + 20;
            }

            // â”€â”€ UNCOMMON skins â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

                static const int UNCOMMON_IDS[8] = { 43, 44, 45, 46, 47, 49, 50, 51 };
                for (int ri=0; ri<8; ri++) {
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
                      if (i >= 49 && i <= 51) {
                          DrawSelectedFixSkin(i, (float)pcx, (float)pcy, 18.0f);
                      } else if (i >= 43 && i <= 48) {
                          DrawAcceptedSkin(i, (float)pcx, (float)pcy, 18.0f, t);
                      } else if (i == 34) {
                          DrawConstellation((float)pcx,(float)pcy,18.0f,t);
                      } else {
                          DrawRareBack(i, (float)pcx, (float)pcy, 18.0f, t);
                          DrawCircle(pcx,pcy,18,sc2);
                          DrawCircle(pcx-3,pcy-4,9,(Color){CLAMP255(sc2.r+55),CLAMP255(sc2.g+55),CLAMP255(sc2.b+55),120});
                          DrawRareFace(i, (float)pcx, (float)pcy, 18.0f, t);
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
                            StatsShopSpent(STATS_SHOP_SKINS,price);
                            SaveStats(); SaveShop();
                        }
                    }
                }
                curY += uncommonRows * skinRowH + 20;
            }

            // â”€â”€ RARE skins â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

                static const int RARE_IDS[12] = { 22, 23, 24, 25, 26, 28, 30, 31, 33, 34, 35, 48 };
                for (int ri=0; ri<12; ri++) {
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
                    if (i == 48) {
                        DrawAcceptedSkin(i, (float)pcx, (float)pcy, 18.0f, t);
                    } else if (i == 34) {
                        DrawConstellation((float)pcx, (float)pcy, 18.0f, t);
                    } else if (i == 28) {
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
                            StatsShopSpent(STATS_SHOP_SKINS,price);
                            SaveStats(); SaveShop();
                        }
                    }
                }
                curY += rareRows * skinRowH + 20;
            }

            // â”€â”€ EPIC skins â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
                            StatsShopSpent(STATS_SHOP_SKINS,price);
                            SaveStats(); SaveShop();
                        }
                    }
                }
                curY += epicRows * skinRowH + 20;
            }

            // â”€â”€ LEGENDARY skins â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

                    // legendary glow â€” the strongest of all rarities
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
                            StatsShopSpent(STATS_SHOP_SKINS,price);
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

        // â”€â”€ TAB 1: UPGRADES â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

        // â”€â”€ Scrollbar logic
        int upgZoneTop = cZoneTop;
        int upgZoneBot = py2 + ph - 70; 
        int availH_upg = upgZoneBot - upgZoneTop;
        
        int topScrollOffset = 8; 
        int totalH_upg = topScrollOffset + (nUpg * (cardH + cardGap)); 

        int maxScrollUpg = totalH_upg - availH_upg;
        if (maxScrollUpg < 0) maxScrollUpg = 0;

        float wheel = GetMouseWheelMoveV().y;
        if (wheel != 0.0f) { upgScroll -= (int)(wheel * 40.0f); }
        if (!menuNavOn && IsKeyDown(KEY_UP))   upgScroll -= 7;
        if (!menuNavOn && IsKeyDown(KEY_DOWN)) upgScroll += 7;
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
                    StatsShopSpent(STATS_SHOP_UPGRADES,ucost);
                    (*upgrades[i].lvl)++;
                    SaveStats(); SaveShop();
                }
            }
        }
        EndScissorMode();

        // â”€â”€ Scrollbar render 
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

        // â”€â”€ TAB 2: PERKS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
            if (!menuNavOn && IsKeyDown(KEY_UP))   upgScroll -= 7;
            if (!menuNavOn && IsKeyDown(KEY_DOWN)) upgScroll += 7;
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
                            StatsShopSpent(STATS_SHOP_PERKS,perks[i].cost);
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
// â”€â”€ ACHIEVEMENTS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    static int advScroll = 0;
    if (menuSub == 4) {
        Color ac = (Color){55, 205, 255, 255};
        DrawSubPanel("ACHIEVEMENTS", ac, t, sw, sh);
        int margin2 = 32, pw = sw - margin2 * 2, ph = sh - margin2 * 2;
        int px2 = margin2, py2 = margin2;

        int zoneTop    = py2 + 64;
        int zoneBottom = py2 + ph - 70;
        int availH     = zoneBottom - zoneTop;


        AchievementProgress achievementProgress[ACH_COUNT];
        BuildAchievementProgress(achievementProgress);
        int _skins_b = 0;
        for (int _i = 1; _i < SKIN_COUNT; _i++) if (skin_owned[_i]) _skins_b++;
        int _upg_b = spd_level + shd_level + wlk_level + swd_level + bmb_level + gld_level;
        int _perk_b = perk_slots + perk_magnet + perk_shield2;
        int _ach_comp = achievementProgress[ACH_OVERACHIEVER_INDEX].value;
        int _ach_maxn = achievementProgress[ACH_PERFECTIONIST_INDEX].value;

        // Each row: name + desc + progress + bar + reward badge â€” 96 px min
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
            // â”€â”€ 5-star achievements (golden border at tier 6) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
            { "Overachiever",     "Earn achievement tiers",
              _ach_comp,           { 5, 20, 40, 70, 110, 200 },
              5, (Color){55,205,255,255} },
            { "Perfectionist",    "Max out achievements",
              _ach_maxn,           { 1, 3, 5, 10, 20, 45 },
              5, (Color){255,215,0,255} },
            // â”€â”€ 3-star achievements (golden border at tier 4) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
            { "Boss Slayer",      "Defeat bosses",
              global_bosses_killed,{ 6, 18, 48, 90, 180, 300 },
              5, (Color){170,40,200,255} },
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
            { "Dungeon Regular",  "Reach Round 10 in main-game runs",
              0, {0}, 5, (Color){190,180,255,255} },
            { "Deep Runner",      "Reach Round 20 in main-game runs",
              0, {0}, 5, (Color){175,170,245,255} },
            { "Trailblazer",      "Walk tiles in the main game",
              0, {0}, 5, (Color){125,220,175,255} },
            { "Golden Hand",      "Gold Touch enemy kills in the main game",
              0, {0}, 3, (Color){245,195,65,255} },
            { "Hidden Explorer",  "Enter side rooms",
              0, {0}, 3, (Color){155,220,220,255} },
            { "Gate Breaker",     "Unlock side-room entrances",
              0, {0}, 3, (Color){225,165,100,255} },
            { "Through the Violet", "Enter artifact-room portals",
              0, {0}, 3, (Color){195,130,250,255} },
            { "Relic Collection", "Own different artifacts",
              0, {0}, 5, (Color){115,215,245,255} },
            { "Key Finder",      "Collect keys in all modes",
              0, {0}, 5, (Color){230,200,120,255} },
            { "Lockbreaker",      "Open locks with keys",
              0, {0}, 5, (Color){210,165,110,255} },
            { "Key Collector",   "Use different key types on locks",
              0, {0}, 3, (Color){205,190,135,255} },
            { "Challenger",      "Complete challenges",
              0, {0}, 5, (Color){245,130,145,255} },
            { "Challenge Tour",  "Complete different challenges",
              0, {0}, 5, (Color){245,165,140,255} },
            { "Daily Habit",     "Start daily seeds on consecutive days",
              0, {0}, 3, (Color){145,205,235,255} },
            { "Daily Explorer",  "Play different daily seeds",
              0, {0}, 5, (Color){125,195,245,255} },
        };
        int numAch     = (int)(sizeof(achievements) / sizeof(achievements[0]));
        _Static_assert(sizeof(achievements) / sizeof(achievements[0]) == ACH_COUNT,
                       "Achievement rows must match saved claim count");
        for (int i = 0; i < numAch && i < ACH_COUNT; i++) {
            achievements[i].current_val = achievementProgress[i].value;
            memcpy(achievements[i].tiers, achievementProgress[i].thresholds,
                   sizeof(achievements[i].tiers));
            achievements[i].max_stars = achievementProgress[i].tierCount - 1;
        }
        int totalRowsH = numAch * rowH;

        int maxScroll = totalRowsH - availH;
        if (maxScroll < 0) maxScroll = 0;
        float wheel = GetMouseWheelMoveV().y;
        if (wheel != 0.0f) advScroll -= (int)(wheel * 40.0f);
        if (!menuNavOn && IsKeyDown(KEY_UP))   advScroll -= 7;
        if (!menuNavOn && IsKeyDown(KEY_DOWN)) advScroll += 7;
        if (advScroll < 0) advScroll = 0;
        if (advScroll > maxScroll) advScroll = maxScroll;

        int cx = px2 + 40;
        BeginScissorMode(px2, zoneTop, pw, availH);

        for (int i = 0; i < numAch; i++) {
            int ry2 = zoneTop + i * rowH - advScroll;
            if (ry2 + rowH < zoneTop || ry2 > zoneBottom) continue;

            // â”€â”€ row background â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            Color rowbg = (i % 2 == 0) ? (Color){20,16,34,160} : (Color){15,12,26,90};
            DrawRectangleRounded(
                (Rectangle){(float)(cx-8),(float)(ry2-4),(float)(pw-64),(float)(rowH-6)},
                0.12f, 6, rowbg);
            DrawRectangleRounded(
                (Rectangle){(float)(cx-8),(float)(ry2-4),4.0f,(float)(rowH-6)},
                1.0f, 4, achievements[i].col);

            // â”€â”€ achieved tier: tiers[0..max_stars-1]=stars, tiers[max_stars]=golden â”€â”€
            int achieved_tier = 0;
            for (int j = 0; j < achievements[i].max_stars + 1; j++) {
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

            // â”€â”€ name + description â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            DrawText(achievements[i].name, cx+6, ry2+5,  16, achievements[i].col);
            DrawText(desc, cx+6, ry2+24, 12, (Color){140,135,170,200});

            // â”€â”€ progress text (rounds for Unbeaten = best round reached) â”€â”€
            const char *progressText;
            if (achieved_tier < goldenTier)
                progressText = TextFormat("%d / %d", achievements[i].current_val, achievements[i].tiers[achieved_tier]);
            else
                progressText = TextFormat("%d (MAX)", achievements[i].current_val);
            DrawText(progressText, cx+6, ry2+42, 15, achievements[i].col);

            // â”€â”€ progress bar (toward next tier) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

            // â”€â”€ Reward badge: lowest unclaimed earned tier â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
            int claim_tier = -1;
            for (int j = 0; j < goldenTier; j++) {
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

                // â”€â”€ coin pill â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

                // â”€â”€ CLAIM button â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
                    StatsRecordUnclassifiedCoins(reward);
                    SaveStats(); SaveShop();
                }
            }

            // â”€â”€ stars + optional golden border â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
    // â”€â”€ FUTURE UPDATES â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (menuSub == 5) {
        Color ac = (Color){185,90,255,255};
        Rectangle backR = DrawSubPanel("UPCOMING...", ac, t, sw, sh);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(GetMousePosition(), backR))
            { menuSub = 0; return; }

        int margin2=32, pw=sw-margin2*2, ph=sh-margin2*2;
        int px2=margin2, py2=margin2;

        // â”€â”€ Update data â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        const char *ver_str  = "v1.7";
        const char *upd_name = "The Secret Chambers for Web and Mobile";
        const char *upd_name_top = "The Secret Chambers";
        const char *upd_name_bottom = upd_name + strlen(upd_name_top) + 1;
        static const struct {
            const char *title;
            const char *detail;
        } features[] = {
            { "Android: v1.7 will be compiled for Android",
              "Touch movement and combat with a virtual mobile controller, "
              "including a movement stick and action buttons.\n"
              "Menus and text adapted to different phone screens.\n"
              "Saves remain available after closing and reopening the app.\n"
              "The Android build will be checked on phones and tablets." },
            { "Web: v1.7 will be compiled for the Web",
              "Keyboard controls and an on-screen controller for touch browsers.\n"
              "The game view adapts to desktop and mobile browser windows.\n"
              "Progress and settings can be saved in the browser.\n"
              "The playable web build will include music and sound effects "
              "and be linked from the project documentation." },
            { "Play Store: release the Android version",
              "Finish the Android build with touch controls and saved progress.\n"
              "Prepare a store page with screenshots, the icon and game details.\n"
              "Check installation, audio and screen sizes on devices.\n"
              "Submit the finished Android version to Google Play." },
        };
        int feat_count = (int)(sizeof(features)/sizeof(features[0]));

        float pp  = 0.5f + 0.5f*sinf(t*1.4f);
        float pp2 = 0.5f + 0.5f*sinf(t*0.9f + 1.1f);

        // â”€â”€ LEFT COLUMN: version card â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
        int contentTop = py2 + 66;
        int colGap = 28;
        int colL_w = sw < 1000 ? 240 : 280;
        int colL_x = px2 + 24;

        int vcard_h = 210;
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

        const char *nxt = "COMING NEXT";
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

        int unfs = 16;
        Color nameColor = {(uchar)CLAMP255(ac.r+50),(uchar)CLAMP255(ac.g+55),
                           (uchar)CLAMP255(ac.b+30),230};
        DrawText(upd_name_top,
                 (int)(vcard.x + vcard.width/2 - MeasureText(upd_name_top,unfs)/2),
                 (int)(vcard.y + 116), unfs, nameColor);
        DrawText(upd_name_bottom,
                 (int)(vcard.x + vcard.width/2 - MeasureText(upd_name_bottom,unfs)/2),
                 (int)(vcard.y + 139), unfs, nameColor);

        float pdp = 0.5f+0.5f*sinf(t*2.5f);
        int dot_y = (int)(vcard.y + 181);
        int dot_cx = (int)(vcard.x + vcard.width/2);
        for (int d=-1;d<=1;d++) {
            float df = (d==0) ? pdp : (0.3f+0.2f*sinf(t*2.5f + d*1.1f));
            DrawCircle(dot_cx + d*18, dot_y, (int)(4.0f*df)+2,
                       (Color){ac.r,ac.g,ac.b,(uchar)(60+140*df)});
            DrawCircle(dot_cx + d*18, dot_y, (int)(2.0f*df)+1,
                       (Color){230,215,255,(uchar)(120+120*df)});
        }

        // â”€â”€ LEFT COLUMN: ETA badge below version card â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

        static const struct {
            const char *label;
            const char *path;
            const char *url;
        } updateLinks[] = {
            { "PROJECT DOCUMENTATION", "/projects/dungeon-crawler",
              "https://smstark-projekt.de/en/projects/dungeon-crawler" },
            { "PROJECT PIPELINE", "/project-pipeline",
              "https://smstark-projekt.de/en/project-pipeline" },
        };
        int linkGap = sh < 680 ? 7 : 12;
        int linkH = sh < 680 ? 55 : 72;
        int linkY = (int)(etacard.y + etacard.height) + (sh < 680 ? 10 : 12);
        for (int i = 0; i < 2; i++) {
            Rectangle linkCard = { (float)colL_x, (float)(linkY + i*(linkH+linkGap)),
                                   (float)colL_w, (float)linkH };
            bool linkHover = CheckCollisionPointRec(GetMousePosition(), linkCard);
            int ty = (int)linkCard.y + (linkH-50)/2;
            DrawRectangleRounded(linkCard, 0.18f, 8,
                                 linkHover ? (Color){37,23,57,240} : (Color){16,10,28,220});
            DrawRectangleRoundedLines(linkCard, 0.18f, 8,
                                      (Color){ac.r,ac.g,ac.b,linkHover ? 180 : 80});
            DrawText(updateLinks[i].label, colL_x+14, ty, 11,
                     (Color){ac.r,ac.g,ac.b,230});
            DrawText("smstark-projekt.de/en", colL_x+14, ty+18, 13,
                     (Color){230,215,255,255});
            DrawText(updateLinks[i].path, colL_x+14, ty+36, 11,
                     (Color){175,168,205,235});
            if (linkHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
#if defined(_WIN32)
                if (!GameConsoleOpenUrl(updateLinks[i].url))
                    TraceLog(LOG_WARNING, "Could not open update link: %s",
                             updateLinks[i].url);
#else
                OpenURL(updateLinks[i].url);
#endif
            }
        }

        int colR_x = colL_x + colL_w + colGap;
        int colR_w = px2 + pw - colR_x - 24;
        int listTop = contentTop;

        const char *feat_hdr = "WHAT IS COMING";
        DrawText(feat_hdr, colR_x, listTop, 14, (Color){ac.r,ac.g,ac.b,180});
        DrawLine(colR_x, listTop+18, colR_x+colR_w, listTop+18,
                 (Color){ac.r,ac.g,ac.b,45});

        int listY0 = listTop + 30;
        int rowGap = 12;
        int listBottom = py2 + ph - 78;
        int listHeight = listBottom - listY0;
        int rowH2 = 272;
        int totalRowsHeight = feat_count * rowH2 + (feat_count - 1) * rowGap;
        int maxFeatureScroll = totalRowsHeight - listHeight;
        if (maxFeatureScroll < 0) maxFeatureScroll = 0;
        static int featureScroll = 0;
        if (featureScroll > maxFeatureScroll) featureScroll = maxFeatureScroll;
        if (CheckCollisionPointRec(GetMousePosition(),
                                   (Rectangle){(float)colR_x,(float)listY0,
                                               (float)colR_w,(float)listHeight})) {
            featureScroll -= (int)(GetMouseWheelMove() * 48.0f);
            if (featureScroll < 0) featureScroll = 0;
            if (featureScroll > maxFeatureScroll) featureScroll = maxFeatureScroll;
        }

        char badge[16]; snprintf(badge, sizeof(badge), "%d FEATURES", feat_count);
        int bw = MeasureText(badge, 12);
        DrawRectangleRounded(
            (Rectangle){(float)(colR_x+colR_w-bw-18),(float)(listTop-4),(float)(bw+16),22},
            0.5f, 6, (Color){ac.r/5,ac.g/5,ac.b/4,200});
        DrawRectangleRoundedLines(
            (Rectangle){(float)(colR_x+colR_w-bw-18),(float)(listTop-4),(float)(bw+16),22},
            0.5f, 6, (Color){ac.r,ac.g,ac.b,100});
        DrawText(badge, colR_x+colR_w-bw-10, listTop, 12,
                 (Color){ac.r,ac.g,ac.b,200});

        BeginScissorMode(colR_x, listY0, colR_w, listHeight);
        for (int i = 0; i < feat_count; i++) {
            int ry = listY0 + i * (rowH2 + rowGap) - featureScroll;

            float rpp = 0.5f + 0.5f*sinf(t*1.3f + i*0.42f);

            Color rowbg = (i%2==0) ? (Color){20,14,34,150} : (Color){14,10,24,80};
            DrawRectangleRounded(
                (Rectangle){(float)colR_x, (float)ry, (float)colR_w, (float)rowH2},
                0.06f, 6, rowbg);
            DrawRectangleRoundedLines(
                (Rectangle){(float)colR_x, (float)ry, (float)colR_w, (float)rowH2},
                0.06f, 6, (Color){ac.r,ac.g,ac.b,45});

            Color barCol = { (uchar)(ac.r*(0.5f+0.5f*rpp)),
                             (uchar)(ac.g*(0.5f+0.5f*rpp)),
                             (uchar)(ac.b*(0.5f+0.5f*rpp)), 255 };
            DrawRectangleRounded(
                (Rectangle){(float)colR_x, (float)ry, 3.0f, (float)rowH2},
                1.0f, 4, barCol);

            int bx2 = colR_x + 19;
            int by2 = ry + 22;
            DrawRectanglePro(
                (Rectangle){(float)bx2, (float)by2, 8.0f, 8.0f},
                (Vector2){4.0f, 4.0f}, 45.0f,
                (Color){ac.r, ac.g, ac.b, (uchar)(140 + (int)(80*rpp))});
            DrawRectanglePro(
                (Rectangle){(float)bx2, (float)by2, 4.0f, 4.0f},
                (Vector2){2.0f, 2.0f}, 45.0f,
                (Color){230,215,255,(uchar)(160+80*rpp)});

            int textX = colR_x + 42;
            int textW = colR_w - 56;
            int titleLines = DrawTutorialTextWrapped(features[i].title, textX, ry+13,
                17, textW, 50, 3, (Color){225,210,250,255});
            int detailY = ry + 16 + titleLines * 20 + 9;
            DrawLine(textX, detailY-7, colR_x+colR_w-16, detailY-7,
                     (Color){ac.r,ac.g,ac.b,55});
            int detailHeight = ry + rowH2 - detailY - 12;
            if (detailHeight > 0)
                DrawTutorialTextWrapped(features[i].detail, textX, detailY,
                    14, textW, detailHeight, 3, (Color){175,168,205,225});
        }
        EndScissorMode();

        if (maxFeatureScroll > 0) {
            int trackX = colR_x + colR_w + 7;
            int thumbH = listHeight * listHeight / totalRowsHeight;
            if (thumbH < 30) thumbH = 30;
            int thumbY = listY0 + featureScroll * (listHeight - thumbH) / maxFeatureScroll;
            DrawRectangleRounded((Rectangle){(float)trackX,(float)listY0,4,(float)listHeight},
                                 1.0f, 4, (Color){ac.r,ac.g,ac.b,40});
            DrawRectangleRounded((Rectangle){(float)trackX,(float)thumbY,4,(float)thumbH},
                                 1.0f, 4, (Color){ac.r,ac.g,ac.b,170});
        }

        return;
    }

    if (menuSub == 6) {
        DrawSettingsPanel(t, false);
        return;
    }


    // â”€â”€ SOUNDTRACK SHOP â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (menuSub == 7) {
        Color ac = (Color){95,205,240,255};
        Rectangle backR = DrawSubPanel("SOUNDTRACK SHOP", ac, t, sw, sh);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(GetMousePosition(), backR))
            { SetSoundtrackShopOpen(false); menuSub = 0; return; }

        // musicShopContext is file-static (keyboard nav + mouse)
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
            if (menuNavOn && subNavFocus == 1 + ti)
                DrawMenuFocusRing(tab, tc, t);
        }

        int cardGap = 14;
        int columns = cardCount > 5 ? 5 : cardCount;
        int rows = (cardCount + columns - 1) / columns;
        int cardW = (contentW - cardGap*(columns-1))/columns;
        int cardY = 169;
        int cardBottom = sh - 104;  // room for artwork, playback, and length above BACK
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
            bool previewPlaying = track >= 0 &&
                                  GetSoundtrackPreviewTrack() == track;
            Rectangle card = {(float)cardX,(float)cardTop,
                              (float)cardW,(float)cardH};
            bool previewClick = false;
            bool cardClick = DrawSoundtrackCard(card, track, selected,
                                                previewPlaying, unlocked, access,
                                                price, &previewClick, t);
            if (previewClick)
                SetSoundtrackPreviewTrack(previewPlaying ? -1 : track);
            if (cardClick) {
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
                    StatsShopSpent(STATS_SHOP_MUSIC,price);
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

    // â”€â”€ CHALLENGES / SEEDED RUNS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (menuSub == 8) {
        DrawSubPanel("CHALLENGES / SEEDED RUNS", COL_CHAL, t, sw, sh);
        ChallengeDrawMenu((Rectangle){32, 32, (float)(sw - 64), (float)(sh - 64)}, t);
        return;
    }

    // Main Menu
    MenuUI uHome = MakeUI();
    Rectangle menuR[MN_COUNT];
    GetMainMenuRects(menuR);

    // â”€â”€ TITLE â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        const char *TT = "DUNGEON CRAWLER";
        float chipRight = menuR[MN_FUTURE].x + menuR[MN_FUTURE].width;
        int ts, tx, ty;
        LayoutTitle(uHome, chipRight, &ts, &tx, &ty);
        int ttw = MeasureText(TT, ts);
        float gp = 0.5f+0.5f*sinf(t*1.1f);
        for (int g=7;g>=1;g--) {
            Color gc={(uchar)(50+30*gp),(uchar)(90+60*gp),255,(uchar)(8*g)};
            DrawText(TT, tx-g, ty-g, ts, gc);
            DrawText(TT, tx+g, ty+g, ts, gc);
        }
        DrawText(TT, tx+3, ty+4, ts, (Color){0,0,0,160});
        DrawText(TT, tx,   ty,   ts, (Color){230,225,250,255});
        int ulx1=tx-MenuPx(uHome,10), ulx2=tx+ttw+MenuPx(uHome,10);
        float ulf=0.5f+0.5f*sinf(t*2.0f);
        DrawLine(ulx1,   ty+ts+MenuPx(uHome,4),  ulx2,   ty+ts+MenuPx(uHome,4),
                 (Color){(uchar)(50+80*ulf),100,(uchar)(210+40*ulf),110});
        DrawLine(ulx1+MenuPx(uHome,30),ty+ts+MenuPx(uHome,9),  ulx2-MenuPx(uHome,30),ty+ts+MenuPx(uHome,9),
                 (Color){40,80,180,45});
    }

    // â”€â”€ TOP-LEFT ICON BUTTONS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (menuNavOn) {
        Vector2 mpNav = GetMousePosition();
        float moved = fabsf(mpNav.x - menuNavMouse.x) + fabsf(mpNav.y - menuNavMouse.y);
        if (menuNavLockMouse) {
            if (moved > 8.0f) menuNavLockMouse = false;
        }
        if (!menuNavLockMouse) {
            for (int i = 0; i < MN_COUNT; i++)
                if (CheckCollisionPointRec(mpNav, menuR[i])) {
                    menuNavFocus = i;
                    break;
                }
            menuNavMouse = mpNav;
        }
    }
    {
        Rectangle settR = menuR[MN_SETTINGS];
        Rectangle musicR = menuR[MN_MUSIC];
        Rectangle futR = menuR[MN_FUTURE];
        Vector2 mp2 = GetMousePosition();

        // Settings gear icon
        {
            bool hov = CheckCollisionPointRec(mp2, settR) ||
                       (menuNavOn && menuNavFocus == MN_SETTINGS);
            DrawSettingsGearIcon(settR, t, hov, menuNavOn && menuNavFocus == MN_SETTINGS);
        }

        // Music button: Beamed Gear 1:1 from Skin_Ideas/Icon-Ideas (menu-matched).
        {
            bool hov = CheckCollisionPointRec(mp2, musicR) ||
                       (menuNavOn && menuNavFocus == MN_MUSIC);
            DrawMenuMusicBeamedGear(musicR, t, hov);
            if (menuNavOn && menuNavFocus == MN_MUSIC)
                DrawMenuFocusRing(musicR, (Color){190,170,255,255}, t);
        }

        // Tutorial chip
        {
            Rectangle tutR = menuR[MN_TUTORIAL];
            bool hov = CheckCollisionPointRec(mp2, tutR) ||
                       (menuNavOn && menuNavFocus == MN_TUTORIAL);
            DrawTutorialChip(tutR, t, hov);
            if (menuNavOn && menuNavFocus == MN_TUTORIAL)
                DrawMenuFocusRing(tutR, COL_TUT, t);
        }

        // Future Updates rocket icon
        {
            bool hov = CheckCollisionPointRec(mp2, futR) ||
                       (menuNavOn && menuNavFocus == MN_FUTURE);
            float s = futR.width / 76.0f;
            Color ic  = hov ? (Color){225,175,255,255} : (Color){160,95,220,200};
            Color bgc = hov ? (Color){36,14,52,230}    : (Color){20,9,32,190};
            DrawRectangleRounded(futR, 0.18f, 8, bgc);
            DrawRectangleRoundedLines(futR, 0.18f, 8,
                hov ? (Color){210,130,255,210} : (Color){110,55,165,150});

            float rcx2 = futR.x + futR.width/2;
            float rcy2 = futR.y + futR.height/2;
            DrawTriangle((Vector2){ rcx2, rcy2 - 26.0f * s },
                         (Vector2){ rcx2 - 9.0f * s, rcy2 - 8.0f * s },
                         (Vector2){ rcx2 + 9.0f * s, rcy2 - 8.0f * s }, ic);
            DrawRectangle((int)(rcx2 - 9.0f * s), (int)(rcy2 - 8.0f * s),
                          (int)(18.0f * s), (int)(22.0f * s), ic);
            DrawCircle((int)rcx2, (int)(rcy2 + 2.0f * s), 5.0f * s, bgc);
            DrawCircleLines((int)rcx2, (int)(rcy2 + 2.0f * s), 5.0f * s, (Color){230,215,255,220});
            DrawTriangle((Vector2){ rcx2 - 9.0f * s, rcy2 + 8.0f * s },
                         (Vector2){ rcx2 - 17.0f * s, rcy2 + 14.0f * s },
                         (Vector2){ rcx2 - 9.0f * s, rcy2 + 14.0f * s }, ic);
            DrawTriangle((Vector2){ rcx2 + 9.0f * s, rcy2 + 8.0f * s },
                         (Vector2){ rcx2 + 9.0f * s, rcy2 + 14.0f * s },
                         (Vector2){ rcx2 + 17.0f * s, rcy2 + 14.0f * s }, ic);
            DrawCircle((int)rcx2, (int)(rcy2 + 18.0f * s), 6.0f * s, (Color){255,140,50,200});
            DrawCircle((int)rcx2, (int)(rcy2 + 23.0f * s), 4.0f * s, (Color){255,240,100,210});
            if (menuNavOn && menuNavFocus == MN_FUTURE)
                DrawMenuFocusRing(futR, (Color){210,130,255,255}, t);
        }
    }

    DrawNavCard(menuR[MN_SHOP],    "SHOP",         COL_SHOP, IC_SHOP, t, 1.8f,
                menuNavOn && menuNavFocus == MN_SHOP);
    DrawNavCard(menuR[MN_ACHIEVE], "ACHIEVEMENTS", COL_ACH,  IC_ACH,  t, 0.9f,
                menuNavOn && menuNavFocus == MN_ACHIEVE);
    DrawNavCard(menuR[MN_STATS],   "STATS",        COL_STAT, IC_STAT, t, 0.0f,
                menuNavOn && menuNavFocus == MN_STATS);
    DrawNavCard(menuR[MN_CHAL],    "CHALLENGES",   COL_CHAL, IC_CHAL, t, 2.7f,
                menuNavOn && menuNavFocus == MN_CHAL);

    {
        bool hover = CheckCollisionPointRec(GetMousePosition(), menuR[MN_PLAY]) ||
                     (menuNavOn && menuNavFocus == MN_PLAY);
        DrawDungeonGate(menuR[MN_PLAY], t, hover && menuEnter < 0.0f);
        if (menuNavOn && menuNavFocus == MN_PLAY)
            DrawMenuFocusRing(menuR[MN_PLAY], PlayAccent(t), t);
    }

    {
        const char *ver = "v1.7";
        int vfs = MenuPx(uHome, 13);
        int vw2 = MeasureText(ver, vfs);
        int vrx = sw - vw2 - MenuPx(uHome, 20), vry = MenuPx(uHome, 10);
        DrawRectangleRounded((Rectangle){(float)(vrx-MenuPx(uHome,6)),(float)(vry-MenuPx(uHome,4)),
                                         (float)(vw2+MenuPx(uHome,12)),(float)MenuPx(uHome,22)},
                             0.4f, 6, (Color){18,14,30,210});
        DrawRectangleRoundedLines((Rectangle){(float)(vrx-MenuPx(uHome,6)),(float)(vry-MenuPx(uHome,4)),
                                              (float)(vw2+MenuPx(uHome,12)),(float)MenuPx(uHome,22)},
                                  0.4f, 6, (Color){65,60,90,160});
        DrawText(ver, vrx, vry, vfs, (Color){130,125,165,220});
    }
}

// â”€â”€â”€ DRAW OVERLAY (death / next-round) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
void DrawOverlay(float t) {
    (void)t;
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, (Color){0,0,0,175});

    if (gs == GS_DEAD) {
        float pp = photosensitivity ? 0.55f : (0.5f + 0.5f*sinf(sttime * 5.0f));
        const char *M = "YOU  DIED";
        int ms = 64, mw2 = MeasureText(M, ms);
        DrawText(M, sw/2-mw2/2+2, sh/2-56+2, ms, (Color){0,0,0,140});
        DrawText(M, sw/2-mw2/2,   sh/2-56,   ms,
                 (Color){230,(uchar)(38+(int)(28*pp)),38,255});
        const char *S = ChallengeRunActive()?
            TextFormat("Level %d/%d   Coins %d   Kills %d",
                       roundn,CHALLENGE_FLOORS,totcoi,kills_game):
            TextFormat("Round %d   Coins %d   Treasures %d   Kills %d",
                       roundn,totcoi,tottrs,kills_game);
        DrawText(S, sw/2-MeasureText(S,22)/2, sh/2+22, 22,
                 (Color){185,180,205,215});
        char confirmB[40];
        FormatActionBinds(ACT_CONFIRM, confirmB, (int)sizeof(confirmB));
        char menuB[40];
        FormatActionBinds(ACT_MENU_BACK, menuB, (int)sizeof(menuB));
        const char *restart = TextFormat("%s to restart   |   %s for menu", confirmB, menuB);
        DrawText(restart,
                 sw/2-MeasureText(restart,20)/2,
                 sh/2+62, 20, (Color){140,135,165,195});

    } else if (gs == GS_NEXT) {
        bool challenge=ChallengeRunActive();
        bool finished=challenge&&roundn>=CHALLENGE_FLOORS;
        const char *M = finished?"CHALLENGE COMPLETE !":
                        challenge?"LEVEL CLEARED !":
                        wasportal?"PORTAL  USED !":"TREASURE  FOUND !";
        Color mc     = challenge?(Color){233,188,113,255}:
                       wasportal?CPORT:CTRES;
        int ms = 52, mw2 = MeasureText(M, ms);
        DrawText(M, sw/2-mw2/2+2, sh/2-48+2, ms, (Color){0,0,0,140});
        DrawText(M, sw/2-mw2/2,   sh/2-48,   ms, mc);

        const char *C = challenge?
            TextFormat("Time %02d:%02d   Coins %d   Kills %d",
                       (int)ChallengeElapsed()/60,(int)ChallengeElapsed()%60,
                       totcoi,kills_game):
            TextFormat("Coins : %d   Kills : %d", totcoi, kills_game);
        DrawText(C, sw/2-MeasureText(C,28)/2, sh/2+22, 28, CCOIN);
        char confirmB[40];
        FormatActionBinds(ACT_CONFIRM, confirmB, (int)sizeof(confirmB));
        const char *nextR = challenge?
            TextFormat("%s for %s",confirmB,finished?"challenge menu":"next level"):
            TextFormat("%s for next round", confirmB);
        DrawText(nextR,
                 sw/2-MeasureText(nextR,20)/2,
                 sh/2+64, 20, (Color){140,135,165,195});
    }
}

#ifdef DEBUG_ROOMS
static bool debugRoomPanelOpen=false;
static int debugChanceSelection=0;
static Rectangle DebugRoomPanelBounds(void) {
    float w=fminf(580,GetScreenWidth()-32.0f);
    return (Rectangle){(GetScreenWidth()-w)*.5f,(GetScreenHeight()-330)*.5f,w,330};
}
static Rectangle DebugChanceButton(int row,bool plus) {
    Rectangle p=DebugRoomPanelBounds();
    return (Rectangle){p.x+p.width-(plus?64:168),p.y+96+row*78,40,40};
}
bool UpdateDebugRoomPanel(void) {
    if(IsKeyPressed(KEY_F8)) { debugRoomPanelOpen=!debugRoomPanelOpen;return true; }
    if(!debugRoomPanelOpen)return false;
    if(IsKeyPressed(KEY_ESCAPE)) { debugRoomPanelOpen=false;return true; }
    if(IsKeyPressed(KEY_ONE))debugChanceSelection=0;
    if(IsKeyPressed(KEY_TWO))debugChanceSelection=1;
    int step=(IsKeyDown(KEY_LEFT_SHIFT)||IsKeyDown(KEY_RIGHT_SHIFT))?10:1;
    int delta=0;
    if(IsKeyPressed(KEY_LEFT))delta=-step;
    if(IsKeyPressed(KEY_RIGHT))delta=step;
    if(delta)SetDebugRoomChance(debugChanceSelection!=0,
        (debugChanceSelection?debugPortalChance:debugSecretChance)+delta);
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse=GetMousePosition();
        for(int row=0;row<2;row++)for(int plus=0;plus<2;plus++)
            if(CheckCollisionPointRec(mouse,DebugChanceButton(row,plus!=0))) {
                debugChanceSelection=row;
                SetDebugRoomChance(row!=0,(row?debugPortalChance:debugSecretChance)+(plus?step:-step));
            }
    }
    return true; // Pause the simulation while editing probabilities.
}
void DrawDebugRoomPanel(void) {
    const char *names[]={"Normal","Boss","Traps","Dark","Lava","Coin Rush",
                         "Artifact Room","Guaranteed Secret Room","Guaranteed Artifact Portal"};
    int current=debugMapScenario>=0?debugMapScenario:(int)roomKind;
    if(current<0||current>DEBUG_PORTAL_ROOM)current=0;
    const char *headline="DEBUG   F9: next room   F8: spawn chances";
    const char *details=TextFormat("%s  |  Secret: %d%%  Portal: %d%%",
                                   names[current],debugSecretChance,debugPortalChance);
    int panelW=MeasureText(headline,14);
    int detailsW=MeasureText(details,13);
    if(detailsW>panelW)panelW=detailsW;
    panelW+=16;
    DrawRectangle(8,82,panelW,46,(Color){12,9,20,210});
    DrawText(headline,16,87,14,(Color){255,159,239,255});
    DrawText(details,16,106,13,(Color){215,205,225,255});
    if(!debugRoomPanelOpen)return;
    DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),(Color){0,0,0,180});
    Rectangle p=DebugRoomPanelBounds();
    DrawRectangleRounded(p,.05f,6,(Color){24,18,33,255});
    DrawText("DEBUG ROOM CHANCES",p.x+24,p.y+20,22,(Color){255,159,239,255});
    DrawText("Per eligible map. Saved automatically. Game paused.",p.x+24,p.y+55,14,(Color){196,185,207,255});
    for(int row=0;row<2;row++) {
        int y=p.y+96+row*78;
        DrawText(row?"2  Artifact portal":"1  Secret room",p.x+24,y+10,18,
                 debugChanceSelection==row?WHITE:(Color){160,150,176,255});
        if(row==0)DrawText("Normal / Trap rooms only",p.x+24,y+43,12,(Color){166,152,181,255});
        for(int plus=0;plus<2;plus++) {
            Rectangle b=DebugChanceButton(row,plus!=0);
            bool hover=CheckCollisionPointRec(GetMousePosition(),b);
            DrawRectangleRounded(b,.15f,4,hover?(Color){126,48,119,255}:(Color){67,38,78,255});
            DrawText(plus?"+":"-",b.x+13,b.y+7,24,WHITE);
        }
        const char *value=TextFormat("%d%%",row?debugPortalChance:debugSecretChance);
        DrawText(value,p.x+p.width-96-MeasureText(value,18)*.5f,y+11,18,WHITE);
    }
    DrawText("1 / 2: select   Left / Right: change   Shift: 10%",p.x+24,p.y+255,14,(Color){196,185,207,255});
    DrawText("Range: 1-100%    F8 / Esc: close",p.x+24,p.y+281,14,(Color){196,185,207,255});
}
#endif
