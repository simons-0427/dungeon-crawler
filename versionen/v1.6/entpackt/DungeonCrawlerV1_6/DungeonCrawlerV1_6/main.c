// main.c — window/init, the top-level state machine & game loop, and cleanup.
// All game state lives in globals.c; this file just wires the modules together.
#include "raylib.h"
#include "globals.h"
#include "constants.h"
#include "types.h"
#include "game_map.h"        // GenRound, DrawMap, WX, WY
#include "game_player.h"     // UseItm, TryMove, DrawPlayer, UpdPlayerFall
#include "game_entities.h"   // UpdEnes, DrawEnemies
#include "game_boss.h"       // UpdBoss, DrawBoss, DrawBossBar
#include "game_particles.h"  // UpdPars, DrawPars
#include "game_ui.h"         // DrawMenu/HUD/PauseMenu/Overlay + layout getters
#include "game_save.h"       // LoadStats, UpdateAndSaveStats
#include "game_shop.h"       // LoadShop
#include "game_settings.h"   // LoadSettings, SaveSettings
#include "game_audio.h"      // InitGameAudio, UpdateGameAudio, ShutdownGameAudio
#include "game_sfx.h"        // InitSfx, PlaySfx, ShutdownSfx
#include "game_utils.h"      // ToggleFullscreenNow, UpdateCameraOffset, PointInRect
#include <string.h>          // memset
#include <time.h>            // time

// Movement keys. In the DEBUG_ROOMS build the arrows drive the round-jumper
// instead, so movement there is WASD-only.
#ifdef DEBUG_ROOMS
#define MOVE_UP     IsKeyDown(KEY_W)
#define MOVE_DOWN   IsKeyDown(KEY_S)
#define MOVE_LEFT   IsKeyDown(KEY_A)
#define MOVE_RIGHT  IsKeyDown(KEY_D)
#else
#define MOVE_UP    (IsKeyDown(KEY_W)||IsKeyDown(KEY_UP))
#define MOVE_DOWN  (IsKeyDown(KEY_S)||IsKeyDown(KEY_DOWN))
#define MOVE_LEFT  (IsKeyDown(KEY_A)||IsKeyDown(KEY_LEFT))
#define MOVE_RIGHT (IsKeyDown(KEY_D)||IsKeyDown(KEY_RIGHT))
#endif

int main(void) {
    SetRandomSeed((unsigned)time(NULL));
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(SW, SH, "Dungeon Crawler");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    LoadStats();
    LoadShop();
    SetTargetFPS(60);

    gs = GS_MENU; roundn = 1; totcoi = 0; tottrs = 0;
    eneon = true; pinvn = 0;
    memset(pars, 0, sizeof(pars));
    LoadSettings();  // may override eneon and other gameplay defaults
    InitGameAudio();
    InitSfx();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        float t  = GetTime();
        UpdateGameAudio(gs, dt);

        if (IsKeyPressed(KEY_F11)) ToggleFullscreenNow();
        UpdateCameraOffset();

        switch (gs) {

        case GS_MENU: {
            if (IsKeyPressed(KEY_ESCAPE)) {
                if (menuSub != 0) { if (menuSub == 2) statScroll = 0; if (menuSub == 3) upgScroll = 0; menuSub = 0; break; }
            }
            if (menuSub == 0) {
                if (IsKeyPressed(KEY_E)) eneon = !eneon;
                if (IsKeyPressed(KEY_ENTER)) {
                    roundn=1; normn=0; totcoi=0; tottrs=0; kills_game=0; pinvn=0; coins_round=0;
                    coinRush = false;
                    GenRound(); gs = GS_PLAY; StatsInc(&global_games_played);
                }
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                int sw2 = GetScreenWidth(), sh2 = GetScreenHeight();
                Vector2 mp = GetMousePosition();

                if (menuSub != 0) {
                    int margin2=32, pw2=sw2-margin2*2, ph2=sh2-margin2*2;
                    int px2=margin2, py2=margin2;
                    int bkW=130, bkH=40;
                    Rectangle backR={(float)(px2+20),(float)(py2+ph2-bkH-16),
                                     (float)bkW,(float)bkH};
                    if (CheckCollisionPointRec(mp, backR)) { if (menuSub == 2) statScroll = 0; if (menuSub == 3) upgScroll = 0; menuSub=0; break; }
                    Rectangle panelR={(float)px2,(float)py2,(float)pw2,(float)ph2};
                    if (!CheckCollisionPointRec(mp, panelR)) { if (menuSub == 2) statScroll = 0; if (menuSub == 3) upgScroll = 0; menuSub=0; break; }
                } else {
                    // Icon buttons (top-left)
                    {
                        Rectangle settR2, musicR2, futR2;
                        GetMenuIconRects(&settR2, &musicR2, &futR2);
                        if (CheckCollisionPointRec(mp, settR2)) { menuSub = 6; break; }
                        if (CheckCollisionPointRec(mp, musicR2)) { menuSub = 7; break; }
                        if (CheckCollisionPointRec(mp, futR2))  { menuSub = 5; break; }
                    }
                    // Card buttons
                    int M2 = 14;
                    int titleBottom2 = 152;
                    int cW2  = (int)(sw2 * 0.38f);
                    int midY2 = (titleBottom2 + sh2 - M2) / 2;
                    int cHtop2 = midY2 - titleBottom2 - M2/2;
                    int cHbot2 = sh2 - M2 - (midY2 + M2/2) - 1;
                    if (cHtop2 < 80) cHtop2 = 80;
                    if (cHbot2 < 80) cHbot2 = 80;
                    int topY2 = titleBottom2;
                    int botY2 = midY2 + M2/2;

                    int rightCardX2 = sw2 - M2 - cW2;
                    Rectangle bTL={(float)M2,           (float)topY2,(float)cW2,(float)cHtop2};
                    Rectangle bTR={(float)rightCardX2,  (float)topY2,(float)cW2,(float)cHtop2};
                    Rectangle bBL={(float)M2,           (float)botY2,(float)cW2,(float)cHbot2};
                    Rectangle bBR={(float)rightCardX2,  (float)botY2,(float)cW2,(float)cHbot2};

                    if (CheckCollisionPointRec(mp,bTL)) { menuSub=1; break; }
                    if (CheckCollisionPointRec(mp,bTR)) { menuSub=2; break; }
                    if (CheckCollisionPointRec(mp,bBL)) { menuSub=3; break; }
                    if (CheckCollisionPointRec(mp,bBR)) { menuSub=4; break; }

                    // Play button
                    int rightCardX3  = sw2 - M2 - cW2;
                    int centreColW2  = rightCardX3 - (M2 + cW2);
                    int pbW2 = centreColW2 - 16;
                    int pbH2 = 130;
                    int pbX2 = sw2/2 - pbW2/2;
                    int pbY2 = midY2 - pbH2/2 - 16;
                    Rectangle pbR2={(float)pbX2,(float)pbY2,(float)pbW2,(float)pbH2};
                    if (CheckCollisionPointRec(mp, pbR2)) {
                        roundn=1; normn=0; totcoi=0; tottrs=0; kills_game=0; pinvn=0; coins_round=0;
                        coinRush = false;
                        GenRound(); StatsInc(&global_games_played); gs=GS_PLAY;
                    }
                }
            }
            break;
        }

        case GS_PLAY: {
            Rectangle pauseBtn = GetPauseButtonRect();

            if (IsKeyPressed(KEY_ESCAPE)) { gs = GS_PAUSE; break; }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                PointInRect(GetMousePosition(), pauseBtn)) {
                gs = GS_PAUSE;
                break;
            }

#ifdef DEBUG_ROOMS
            // Dev shortcut: cycle room types in place.
            // Order: NORMAL → BOSS → TRAP → DARK → LAVA → COIN RUSH → …
            if (IsKeyPressed(KEY_F9)) {
                int step = (int)roomKind;
                if (step < 0 || step > 5) step = 0;
                forceRoom = (step + 1) % 6;
                GenRound();
                break;
            }
            // Dev shortcut: jump the round counter and regenerate immediately.
            // UP/DOWN = +/-1, RIGHT/LEFT = +/-5. Arrows are repurposed here, so
            // move with WASD in the debug build.
            {
                int rd = 0;
                if      (IsKeyPressed(KEY_UP))    rd = +1;
                else if (IsKeyPressed(KEY_DOWN))  rd = -1;
                else if (IsKeyPressed(KEY_RIGHT)) rd = +5;
                else if (IsKeyPressed(KEY_LEFT))  rd = -5;
                if (rd) {
                    roundn += rd;
                    if (roundn < 1) roundn = 1;
                    // Estimate completed normal floors for wall/trap density
                    // (boss every 10th round doesn't add to normn).
                    normn = roundn - 1 - roundn/10;
                    if (normn < 0) normn = 0;
                    coins_round = 0;
                    GenRound();
                    break;
                }
            }
#endif
            // Trapdoor fall locks input/movement for the short death anim.
            UpdPlayerFall(dt);
            if (gs != GS_PLAY) break;
            bool falling = (pfall > 0);

            if (!falling) {
                if (IsKeyPressed(KEY_ONE))   UseItm(0);
                if (IsKeyPressed(KEY_TWO))   UseItm(1);
                if (IsKeyPressed(KEY_THREE)) UseItm(2);
                if (IsKeyPressed(KEY_FOUR))  UseItm(3);
                if (IsKeyPressed(KEY_FIVE))  UseItm(4);
            }

            if (pspd > 0) pspd -= dt;
            if (pwlk > 0) pwlk -= dt;
            if (pgold > 0) { pgold -= dt; if (pgold < 0) pgold = 0; }
            if (pshield && pshield_dur < 1e8f) {
                pshield_dur -= dt;
                if (pshield_dur <= 0) { pshield = false; pshield_dur = 0; }
            }
            if (bannerT > 0) bannerT -= dt;
            if (roomKind == RK_TRAP) {
                // Fire the arrow cue exactly once per volley, on the frame the
                // lanes go lethal (ArrowLaneLethal flips false -> true).
                bool wasLethal = ArrowLaneLethal();
                trapPhase += dt;
                if (!wasLethal && ArrowLaneLethal()) PlaySfx(SFX_ARROW);
            }

            if (!falling) {
                float md = (pspd > 0) ? pmd * 0.5f : pmd;
                pmt += dt;
                if (pmt >= md) {
                    bool moved = false;
                    if      (MOVE_UP)    { TryMove(0,-1); moved=true; }
                    else if (MOVE_DOWN)  { TryMove(0,+1); moved=true; }
                    else if (MOVE_LEFT)  { TryMove(-1,0); moved=true; }
                    else if (MOVE_RIGHT) { TryMove(+1,0); moved=true; }
                    if (moved) pmt = 0;
                }

                UpdPlayerHazards();   // e.g. a blink trap firing underfoot
                if (gs != GS_PLAY) break;
            }

            // ── Coin Magnet perk ──────────────────────────────────────────────
            if (perk_magnet) {
                for (int i = 0; i < MAX_COI; i++) {
                    if (!coins[i].on) continue;
                    int dx = coins[i].x - px;
                    int dy = coins[i].y - py;
                    if (dx < -1 || dx > 1 || dy < -1 || dy > 1) continue;
                    coins[i].on = false;
                    map[coins[i].y][coins[i].x] = '.';
                    int val = coinRush ? 2 : 1;
                    totcoi += val; StatsAdd(&global_totcoi, val); coin_balance += val;
                    coins_round += val;
                    Burst(WX(coins[i].x), WY(coins[i].y), CCOIN, coinRush ? 12 : 8, 2);
                    PlaySfx(SFX_COIN_PICKUP);
                }
            }

            UpdLava(dt);
            if (gs != GS_PLAY) break;   // the lava front may have caught us
            UpdEnes(dt);
            UpdBoss(dt);
            UpdPars(dt);

            cam.target.x += (WX(px) - cam.target.x) * dt * cam_smooth;
            cam.target.y += (WY(py) - cam.target.y) * dt * cam_smooth;
            break;
        }

        case GS_PAUSE: {
            Rectangle panel, resumeBtn, menuBtn;
            GetPauseMenuLayout(&panel, &resumeBtn, &menuBtn);

            if (IsKeyPressed(KEY_ESCAPE)) gs = GS_PLAY;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 mp = GetMousePosition();
                if (PointInRect(mp, resumeBtn)) gs = GS_PLAY;
                else if (PointInRect(mp, menuBtn)) { gs = GS_MENU; menuSub = 0; }
            }

            UpdPars(dt);
            break;
        }

        case GS_DEAD:
        case GS_NEXT:
            sttime += dt;
            UpdPars(dt);
            if (sttime > 0.45f) {
                if (gs == GS_DEAD) {
                    if (IsKeyPressed(KEY_ENTER))
                        { roundn=1;normn=0;totcoi=0;tottrs=0;kills_game=0;pinvn=0;coins_round=0; coinRush=false; StatsInc(&global_games_played); GenRound(); gs=GS_PLAY; }
                    if (IsKeyPressed(KEY_Q)) { gs = GS_MENU; menuSub = 0; }
                } else {
                    if (IsKeyPressed(KEY_ENTER)) {
                        // Only completed NORMAL rounds densify walls/traps (normn).
                        // Map size itself tracks roundn.
                        if (roomKind == RK_NORMAL) normn++;
                        roundn++; coins_round=0; GenRound(); gs=GS_PLAY;
                        // "Best round reached" tracks the round just STARTED,
                        // not merely the last one completed — so it updates
                        // here too, not only on death/treasure/portal.
                        UpdateAndSaveStats();
                    }
                }
            }
            break;
        }

        BeginDrawing();
        ClearBackground(CB);

        if (gs == GS_MENU) {
            DrawMenu(t);
        } else {

            BeginMode2D(cam);

            DrawMap(t);
            DrawEnemies(t);
            DrawBoss(t);
            DrawPlayer(t);

            DrawPars();
            if (roomKind == RK_DARK) DrawDarkness();
            EndMode2D();

            DrawHUD(t);
            DrawBossBar();

            if (gs == GS_PAUSE) DrawPauseMenu();
            if (gs == GS_DEAD || gs == GS_NEXT) DrawOverlay(t);
        }

        // FPS overlay: top-right in menu, just below the HUD bar in gameplay.
        if (show_fps)
            DrawFPS(GetScreenWidth() - 90, (gs == GS_MENU) ? 36 : 82);

        EndDrawing();
    }

    UpdateAndSaveStats();
    SaveSettings();
    ShutdownSfx();
    ShutdownGameAudio();
    CloseWindow();
    return 0;
}
