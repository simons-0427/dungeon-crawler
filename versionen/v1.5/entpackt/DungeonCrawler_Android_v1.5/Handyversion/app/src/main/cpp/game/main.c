// main.c — window/init, the top-level state machine & game loop, and cleanup.
// All game state lives in globals.c; this file just wires the modules together.
#include "raylib.h"
#include "globals.h"
#include "constants.h"
#include "types.h"
#include "game_map.h"        // GenRound, DrawMap, WX, WY
#include "game_player.h"     // UseItm, TryMove, DrawPlayer
#include "game_entities.h"   // UpdEnes, DrawEnemies
#include "game_particles.h"  // UpdPars, DrawPars
#include "game_ui.h"         // DrawMenu/HUD/PauseMenu/Overlay + layout getters
#include "game_save.h"       // LoadStats, UpdateAndSaveStats
#include "game_shop.h"       // LoadShop
#include "game_settings.h"   // LoadSettings, SaveSettings
#include "game_audio.h"      // InitGameAudio, UpdateGameAudio, ShutdownGameAudio
#include "game_sfx.h"        // InitSfx, PlaySfx, ShutdownSfx
#include "game_utils.h"      // ToggleFullscreenNow, UpdateCameraOffset, PointInRect
#include "mobile_input.h"    // Android analog stick + multi-touch controls
#include <string.h>          // memset
#include <time.h>            // time

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

static void UpdateDrawFrame(void);
static bool exitRequested = false;

static bool BackPressed(void) {
    bool pressed = IsKeyPressed(KEY_ESCAPE);
#if defined(PLATFORM_ANDROID)
    pressed = pressed || IsKeyPressed(KEY_BACK);
#endif
    return pressed;
}

#if defined(PLATFORM_ANDROID)
// Invoked synchronously by the vendored raylib lifecycle callback before its
// event loop blocks while the app is in the background.
void AndroidGameOnPause(void) {
    if (gs == GS_PLAY) gs = GS_PAUSE;
    PauseGameAudio();
    UpdateAndSaveStats();
    SaveShop();
    SaveSettings();
}

void AndroidGameOnResume(void) {
    if (gs == GS_MENU) ResumeMenuAudio();
}
#endif

#if defined(PLATFORM_WEB)
// Emscripten GLFW already maps page coords → game coords when the canvas is
// CSS-scaled (see libglfw.js calculateMouseCoords). Extra SetMouseScale would
// double-scale and break clicks. Only reset to identity so host code can't leave
// a stale scale, and keep the window size fixed.
static void SyncWebMouseMapping(void) {
    SetMouseScale(1.0f, 1.0f);
    SetMouseOffset(0, 0);
}
#endif

#if defined(PLATFORM_ANDROID)
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
#else
int main(void) {
#endif
    SetRandomSeed((unsigned)time(NULL));
#if defined(PLATFORM_WEB) || defined(PLATFORM_ANDROID)
    // Fixed size on web — shell letterboxes via CSS; resizable desyncs hit tests.
    SetConfigFlags(FLAG_MSAA_4X_HINT);
#else
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
#endif
    InitWindow(SW, SH, "Dungeon Crawler");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
#if defined(PLATFORM_WEB)
    SyncWebMouseMapping();
#endif
    LoadStats();
    LoadShop();
    SetTargetFPS(60);

    gs = GS_MENU; roundn = 1; totcoi = 0; tottrs = 0;
    eneon = true; pinvn = 0;
    memset(pars, 0, sizeof(pars));
    LoadSettings();  // may override eneon and other gameplay defaults
    InitGameAudio();
    InitSfx();

#if defined(PLATFORM_WEB)
    // Browser: drive the game from requestAnimationFrame (no blocking while).
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    while (!WindowShouldClose() && !exitRequested) {
        UpdateDrawFrame();
    }

    UpdateAndSaveStats();
    SaveSettings();
    ShutdownSfx();
    ShutdownGameAudio();
    CloseWindow();
#endif
    return 0;
}

static void UpdateDrawFrame(void) {
#if defined(PLATFORM_WEB)
    SyncWebMouseMapping();
#endif
    float dt = GetFrameTime();
    float t  = GetTime();
    MobileInputUpdate(gs == GS_PLAY, mobile_control_mode);

    UpdateGameAudio(gs, dt);

#if !defined(PLATFORM_WEB) && !defined(PLATFORM_ANDROID)
    if (IsKeyPressed(KEY_F11)) ToggleFullscreenNow();
#endif
    UpdateCameraOffset();

    switch (gs) {

    case GS_MENU: {
        if (BackPressed()) {
            if (menuSub != 0) { if (menuSub == 2) statScroll = 0; if (menuSub == 3) upgScroll = 0; menuSub = 0; break; }
#if defined(PLATFORM_ANDROID)
            exitRequested = true;
#endif
        }
        if (menuSub != 0) {
            Rectangle backR = GetMenuBackButtonRect();
            bool mouseBack = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                             PointInRect(GetMousePosition(), backR);
            if (mouseBack || MobileTouchPressed(backR)) {
                if (menuSub == 2) statScroll = 0;
                if (menuSub == 3) upgScroll = 0;
                menuSub = 0;
                break;
            }
        }
        if (menuSub == 6) {
            Rectangle resetR = GetSettingsResetButtonRect();
            bool mouseReset = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                              PointInRect(GetMousePosition(), resetR);
            if (mouseReset || MobileTouchPressed(resetR)) {
                ResetSettingsToDefaults();
                break;
            }
        }
        if (menuSub == 0) {
            if (IsKeyPressed(KEY_E)) eneon = !eneon;
            if (IsKeyPressed(KEY_ENTER)) {
                roundn=1; totcoi=0; tottrs=0; kills_game=0; pinvn=0; coins_round=0;
                GenRound(); gs = GS_PLAY; global_games_played++;
            }
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            int sw2 = GetScreenWidth(), sh2 = GetScreenHeight();
            Vector2 mp = GetMousePosition();

            if (menuSub != 0) {
                Rectangle panelR = GetMenuSubPanelRect();
                if (!CheckCollisionPointRec(mp, panelR)) { if (menuSub == 2) statScroll = 0; if (menuSub == 3) upgScroll = 0; menuSub=0; break; }
            } else {
                // Icon buttons (top-left)
                {
                    Rectangle settR2, futR2;
                    GetMenuIconRects(&settR2, &futR2);
                    if (CheckCollisionPointRec(mp, settR2)) { menuSub = 6; break; }
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
                    roundn=1; totcoi=0; tottrs=0; kills_game=0; pinvn=0; coins_round=0;
                    GenRound(); global_games_played++; gs=GS_PLAY;
                }
            }
        }
        break;
    }

    case GS_PLAY: {
        Rectangle pauseBtn = GetPauseButtonRect();

        if (BackPressed()) { gs = GS_PAUSE; break; }
        if ((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
             PointInRect(GetMousePosition(), pauseBtn)) ||
            MobileTouchPressed(pauseBtn)) {
            gs = GS_PAUSE;
            break;
        }

        if (IsKeyPressed(KEY_ONE))   UseItm(0);
        if (IsKeyPressed(KEY_TWO))   UseItm(1);
        if (IsKeyPressed(KEY_THREE)) UseItm(2);
        if (IsKeyPressed(KEY_FOUR))  UseItm(3);
        if (IsKeyPressed(KEY_FIVE))  UseItm(4);
        for (int slot = 0; slot < INV_SZ + perk_slots; slot++) {
            if (MobileTouchPressed(GetInventorySlotRect(slot))) UseItm(slot);
        }

        if (pspd > 0) pspd -= dt;
        if (pwlk > 0) pwlk -= dt;
        if (pshield && pshield_dur < 1e8f) {
            pshield_dur -= dt;
            if (pshield_dur <= 0) { pshield = false; pshield_dur = 0; }
        }

        {
            Vector2 touchDirection = { 0.0f, 0.0f };
            bool touchMove = MobileGetMoveVector(&touchDirection, NULL);
            float md = (pspd > 0) ? pmd*0.5f : pmd;
            pmt += dt;

            if (pmt >= md) {
                bool moved = false;
                if (touchMove) {
                    TryMove((int)touchDirection.x, (int)touchDirection.y);
                    moved = true;
                } else if (IsKeyDown(KEY_W)||IsKeyDown(KEY_UP))
                    { TryMove(0,-1); moved=true; }
                else if (IsKeyDown(KEY_S)||IsKeyDown(KEY_DOWN))
                    { TryMove(0,+1); moved=true; }
                else if (IsKeyDown(KEY_A)||IsKeyDown(KEY_LEFT))
                    { TryMove(-1,0); moved=true; }
                else if (IsKeyDown(KEY_D)||IsKeyDown(KEY_RIGHT))
                    { TryMove(+1,0); moved=true; }
                if (moved) pmt = 0.0f;
            }
        }

        if (gs != GS_PLAY) break;

        // ── Coin Magnet perk ──────────────────────────────────────────────
        if (perk_magnet) {
            for (int i = 0; i < MAX_COI; i++) {
                if (!coins[i].on) continue;
                int dx = coins[i].x - px;
                int dy = coins[i].y - py;
                if (dx < -1 || dx > 1 || dy < -1 || dy > 1) continue;
                coins[i].on = false;
                map[coins[i].y][coins[i].x] = '.';
                totcoi++; global_totcoi++; coin_balance++;
                coins_round++;
                Burst(WX(coins[i].x), WY(coins[i].y), CCOIN, 8, 2);
                PlaySfx(SFX_COIN_PICKUP);
            }
        }

        UpdEnes(dt);
        UpdPars(dt);

        cam.target.x += (px*CELL + CELL/2 - cam.target.x) * dt * cam_smooth;
        cam.target.y += (py*CELL + CELL/2 - cam.target.y) * dt * cam_smooth;
        break;
    }

    case GS_PAUSE: {
        Rectangle panel, resumeBtn, menuBtn;
        GetPauseMenuLayout(&panel, &resumeBtn, &menuBtn);

        if (BackPressed()) gs = GS_PLAY;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mp = GetMousePosition();
            if (PointInRect(mp, resumeBtn)) gs = GS_PLAY;
            else if (PointInRect(mp, menuBtn)) { gs = GS_MENU; menuSub = 0; }
        }
        if (MobileTouchPressed(resumeBtn)) gs = GS_PLAY;
        else if (MobileTouchPressed(menuBtn)) { gs = GS_MENU; menuSub = 0; }

        UpdPars(dt);
        break;
    }

    case GS_DEAD:
    case GS_NEXT:
        sttime += dt;
        UpdPars(dt);
        if (sttime > 0.45f) {
            Rectangle primaryBtn, secondaryBtn;
            GetOverlayButtonLayout(&primaryBtn, &secondaryBtn);
            if (gs == GS_DEAD) {
                if (IsKeyPressed(KEY_ENTER) || MobileTouchPressed(primaryBtn))
                    { roundn=1;totcoi=0;tottrs=0;kills_game=0;pinvn=0;coins_round=0; global_games_played++; GenRound(); gs=GS_PLAY; }
                if (IsKeyPressed(KEY_Q) || BackPressed() || MobileTouchPressed(secondaryBtn)) { gs = GS_MENU; menuSub = 0; }
            } else {
                if (IsKeyPressed(KEY_ENTER) || MobileTouchPressed(primaryBtn))
                    { roundn++; coins_round=0; GenRound(); gs=GS_PLAY; }
                else if (BackPressed()) { gs = GS_MENU; menuSub = 0; }
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
        DrawPlayer(t);

        DrawPars();
        EndMode2D();

        DrawHUD(t);

#if defined(PLATFORM_ANDROID)
        if (gs == GS_PLAY) MobileDrawStick();
#endif

        if (gs == GS_PAUSE) DrawPauseMenu();
        if (gs == GS_DEAD || gs == GS_NEXT) DrawOverlay(t);
    }

    // FPS overlay: top-right in menu, just below the HUD bar in gameplay.
    if (show_fps)
        DrawFPS(GetScreenWidth() - 90, (gs == GS_MENU) ? 36 : 82);

    EndDrawing();
}
