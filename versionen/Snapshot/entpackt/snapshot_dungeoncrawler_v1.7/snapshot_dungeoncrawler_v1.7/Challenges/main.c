// main.c — window/init, the top-level state machine & game loop, and cleanup.
// All game state lives in globals.c; this file just wires the modules together.
#include "raylib.h"
#include "globals.h"
#include "constants.h"
#include "types.h"
#include "game_map.h"        // GenRound, DrawMap, WX, WY
#include "game_items.h"      // ShutdownArtifactAssets
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
#include "game_challenges.h" // isolated challenge expeditions
#include <stdio.h>           // snprintf
#include <string.h>          // memset
#include <time.h>            // time

// Movement and actions go through ActionDown/ActionPressed so release builds
// honour extra keybinds. DEBUG_ROOMS keeps the hardcoded WASD / 1-5 / F11 set.

static bool overlayConfirmQueued = false;
#ifdef CHALLENGES_ONLY
static void ConfigureChallengeLoadout(void) {
    skin_equipped = 0;
    for (int i=0; i<SKIN_COUNT; i++) skin_owned[i] = i==0;
    spd_level=5; shd_level=5; wlk_level=5;
    swd_level=3; bmb_level=5; gld_level=5;
    perk_slots=4; perk_magnet=1; perk_shield2=1;
    for (int i=0; i<=MUSIC_DEFAULT_TRACK && i<MUSIC_TRACK_COUNT; i++)
        music_owned[i]=1;
}
#endif

int main(void) {
    SetRandomSeed((unsigned)time(NULL));
#ifdef CHALLENGES_ONLY
    ChangeDirectory(GetApplicationDirectory());
#endif
#ifdef DEBUG_ROOMS
    LoadDebugRoomSettings();
#endif
    LoadSettings();  // file only — window geometry is applied after InitWindow
    int win_w = SW, win_h = SH;
    GetPersistedWindowState(NULL, NULL, NULL, &win_w, &win_h);
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(win_w, win_h, "Dungeon Crawler");
    {
        const char *rel = "assets/selected_icon.png";
        char full[512];
        const char *path = rel;
        if (!FileExists(path)) {
            snprintf(full, sizeof(full), "%s%s", GetApplicationDirectory(), rel);
            path = full;
        }
        if (FileExists(path)) {
            Image appIcon = LoadImage(path);
            if (appIcon.data) {
                /* PNG glow layers are semi-transparent. Windows composites
                   those onto a light title bar / Explorer preview, which
                   washes the reds out. Flatten onto the icon's own canvas. */
                Color canvas = GetImageColor(appIcon, 0, 0);
                canvas.a = 255;
                Image flat = GenImageColor(appIcon.width, appIcon.height, canvas);
                Rectangle src = { 0, 0, (float)appIcon.width, (float)appIcon.height };
                ImageDraw(&flat, appIcon, src, src, WHITE);
                SetWindowIcon(flat);
                UnloadImage(flat);
                UnloadImage(appIcon);
            }
        }
    }
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    ApplyPersistedWindowState();
    LoadStats();
    ChallengeLoadData();
#ifdef CHALLENGES_ONLY
    ConfigureChallengeLoadout();
#else
    LoadShop();
#endif
    SetTargetFPS(60);

    gs = GS_MENU; roundn = 1; totcoi = 0; tottrs = 0;
#ifdef CHALLENGES_ONLY
    menuSub=8;
#endif
    eneon = true; pinvn = 0; inv_sel = -1;
    memset(pars, 0, sizeof(pars));
    InitGameAudio();
    InitSfx();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        float t  = GetTime();
        UpdateGameAudio(gs, dt);
        UpdateSfx(dt);

        if (!SettingsCapturingKey() && ActionPressed(ACT_FULLSCREEN))
            ToggleFullscreenNow();
        if (IsWindowResized() && !GameWindowIsFullscreen())
            CaptureWindowState();
        if (gs == GS_PLAY && focus_pause && !IsWindowFocused())
            gs = GS_PAUSE;
        UpdateCameraOffset();

        switch (gs) {

        case GS_MENU: {
#ifdef CHALLENGES_ONLY
            menuSub=8;
            if (ActionPressedKey(ACT_CONFIRM)) ChallengeHandleMenuConfirm();
            unsigned selectedSeed=0;
            if (ChallengeTakeRunStart(&selectedSeed)==1) {
                ConfigureChallengeLoadout();
                roundn=1; normn=0; totcoi=0; tottrs=0; kills_game=0;
                pinvn=0; inv_sel=-1; coins_round=0; coinRush=false;
                run_seed=selectedSeed;
                GenRound(); gs=GS_PLAY; StatsInc(&global_games_played);
                MenuNavExit();
            }
#else
            UpdateMenuPlayAnim();
            UpdateMenuNav();
            if (IsKeyPressed(KEY_ESCAPE)) {
                if (menuSub == 6 && SettingsCapturingKey()) {
                    SettingsCancelCapture();
                    break;
                }
                if (menuSub == 6 && SettingsDeleteMode()) {
                    SettingsCancelDelete();
                    break;
                }
                if (MenuNavIsOn()) { MenuNavExit(); break; }
                if (menuSub != 0) { if (menuSub == 2) statScroll = 0; if (menuSub == 3) upgScroll = 0; menuSub = 0; break; }
            }
            if (menuSub != 0 && !SettingsCapturingKey() &&
                !(menuSub == 8 && ChallengeEditingSeed()) && ActionPressed(ACT_MENU_BACK)) {
                if (menuSub == 2) statScroll = 0;
                if (menuSub == 3) upgScroll = 0;
                menuSub = 0;
            }
            {
                int navAct = MenuNavActivatePressed();
                if (navAct == -2) {
                    if (menuSub == 2) statScroll = 0;
                    if (menuSub == 3) upgScroll = 0;
                    menuSub = 0;
                    break;
                }
                if (menuSub == 0 && !MenuPlayBusy()) {
                    if (navAct == MN_PLAY)
                        MenuPlayBegin();
                    else if (navAct >= 0) {
                        if (navAct == MN_SETTINGS) menuSub = 6;
                        else if (navAct == MN_MUSIC) menuSub = 7;
                        else if (navAct == MN_FUTURE) menuSub = 5;
                        else if (navAct == MN_TUTORIAL) { TutorialBookReset(); menuSub = 1; }
                        else if (navAct == MN_STATS) menuSub = 2;
                        else if (navAct == MN_SHOP) menuSub = 3;
                        else if (navAct == MN_ACHIEVE) menuSub = 4;
                        else if (navAct == MN_CHAL) { menuSub = 8; MenuNavExit(); }
                        break;
                    } else if (!MenuNavIsOn() && ActionPressedKey(ACT_CONFIRM))
                        MenuPlayBegin();
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
                } else if (!MenuPlayBusy()) {
                    Rectangle mr[MN_COUNT];
                    GetMainMenuRects(mr);
                    if (CheckCollisionPointRec(mp, mr[MN_SETTINGS])) { menuSub = 6; break; }
                    if (CheckCollisionPointRec(mp, mr[MN_MUSIC]))    { menuSub = 7; break; }
                    if (CheckCollisionPointRec(mp, mr[MN_FUTURE]))   { menuSub = 5; break; }
                    if (CheckCollisionPointRec(mp, mr[MN_TUTORIAL])) { TutorialBookReset(); menuSub = 1; break; }
                    if (CheckCollisionPointRec(mp, mr[MN_STATS]))    { menuSub = 2; break; }
                    if (CheckCollisionPointRec(mp, mr[MN_SHOP]))     { menuSub = 3; break; }
                    if (CheckCollisionPointRec(mp, mr[MN_ACHIEVE]))  { menuSub = 4; break; }
                    if (CheckCollisionPointRec(mp, mr[MN_CHAL]))     { menuSub = 8; MenuNavExit(); break; }
                    if (CheckCollisionPointRec(mp, mr[MN_PLAY]))       MenuPlayBegin();
                }
            }
            if(menuSub==8&&ActionPressedKey(ACT_CONFIRM))
                ChallengeHandleMenuConfirm();
            if (MenuPlayShouldStart()&&menuSub==0) {
                unsigned selectedSeed=0;
                int selectedRun=ChallengeTakeRunStart(&selectedSeed);
                if(!selectedRun)ChallengeClearRunMode();
                roundn=1; normn=0; totcoi=0; tottrs=0; kills_game=0; pinvn=0; inv_sel=-1; coins_round=0;
                coinRush = false;
                if(selectedRun)run_seed=selectedSeed;
                else StartNewRunSeed();
                if(selectedRun==2)ResetSeededArtifactSequence();
                GenRound(); gs = GS_PLAY; StatsInc(&global_games_played);
                if(selectedRun==2)ChallengeSaveSeedRunProgress();
                MenuNavExit();
            }
#endif
            break;
        }

        case GS_PLAY: {
            if(ChallengeRunMode()!=0)ChallengeTickRun(dt);
#ifdef DEBUG_ROOMS
            if (UpdateDebugRoomPanel()) break;
#endif
            Rectangle pauseBtn = GetPauseButtonRect();

            if (ActionPressed(ACT_PAUSE)) { gs = GS_PAUSE; break; }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                PointInRect(GetMousePosition(), pauseBtn)) {
                gs = GS_PAUSE;
                break;
            }

#ifdef DEBUG_ROOMS
            // Dev shortcut: cycle room types in place.
            // Order: NORMAL → BOSS → TRAP → DARK → LAVA → COIN RUSH → …
            if (IsKeyPressed(KEY_F9)) {
                int step = debugMapScenario >= 0 ? debugMapScenario : (int)roomKind;
                if (step < 0 || step > DEBUG_PORTAL_ROOM) step = 0;
                forceRoom = (step + 1) % (DEBUG_PORTAL_ROOM + 1);
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
            UpdateLockedObjects();
            UpdPlayerFall(dt);
            if (gs != GS_PLAY) {
                if ((gs == GS_NEXT || gs == GS_DEAD) && ActionPressed(ACT_CONFIRM))
                    overlayConfirmQueued = true;
                break;
            }
            bool falling = (pfall > 0);

            if (!falling) {
                if (ActionPressed(ACT_USE_1)) UseItm(0);
                if (ActionPressed(ACT_USE_2)) UseItm(1);
                if (ActionPressed(ACT_USE_3)) UseItm(2);
                if (ActionPressed(ACT_USE_4)) UseItm(3);
                if (ActionPressed(ACT_USE_5)) UseItm(4);

                if (mouse_item_select) {
                    if (pinvn <= 0) inv_sel = -1;
                    else if (inv_sel >= pinvn) inv_sel = pinvn - 1;

                    Vector2 mp = GetMousePosition();
                    int nslots = GetInventorySlotCount();
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                        !PointInRect(mp, pauseBtn)) {
                        for (int i = 0; i < nslots; i++) {
                            if (PointInRect(mp, GetInventorySlotRect(i))) {
                                if (i < pinvn) inv_sel = (inv_sel == i) ? -1 : i;
                                else           inv_sel = -1;
                                break;
                            }
                        }
                    }
                    float wheel = GetMouseWheelMove();
                    if (pinvn > 0 && wheel != 0.0f) {
                        if (inv_sel < 0) inv_sel = (wheel > 0.0f) ? pinvn - 1 : 0;
                        else if (wheel > 0.0f) inv_sel = (inv_sel + pinvn - 1) % pinvn;
                        else                   inv_sel = (inv_sel + 1) % pinvn;
                    }
                    if (inv_sel >= 0 && ActionPressed(ACT_USE_SELECTED))
                        UseItm(inv_sel);
                } else {
                    inv_sel = -1;
                }
            }

            if (pspd > 0) pspd -= dt;
            if (pwlk > 0) pwlk -= dt;
            if (pgold > 0) { pgold -= dt; if (pgold < 0) pgold = 0; }
            if (pshield && pshield_dur < 1e8f) {
                pshield_dur -= dt;
                if (pshield_dur <= 0) { pshield = false; pshield_dur = 0; }
            }
            if (bannerT > 0) bannerT -= dt;
            if (ArrowLanesPresent()) {
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
                    if      (ActionDown(ACT_MOVE_UP))    { TryMove(0,-1); moved=true; }
                    else if (ActionDown(ACT_MOVE_DOWN))  { TryMove(0,+1); moved=true; }
                    else if (ActionDown(ACT_MOVE_LEFT))  { TryMove(-1,0); moved=true; }
                    else if (ActionDown(ACT_MOVE_RIGHT)) { TryMove(+1,0); moved=true; }
                    if (moved) pmt = 0;
                }

                UpdPlayerHazards();   // e.g. a blink trap firing underfoot
                if (gs != GS_PLAY) {
                    if ((gs == GS_NEXT || gs == GS_DEAD) && ActionPressed(ACT_CONFIRM))
                        overlayConfirmQueued = true;
                    break;
                }
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
            if (gs != GS_PLAY) {   // the lava front may have caught us
                if ((gs == GS_NEXT || gs == GS_DEAD) && ActionPressed(ACT_CONFIRM))
                    overlayConfirmQueued = true;
                break;
            }
            UpdEnes(dt);
            UpdBoss(dt);
            UpdPars(dt);

            cam.target.x += (WX(px) - cam.target.x) * dt * cam_smooth;
            cam.target.y += (WY(py) - cam.target.y) * dt * cam_smooth;

            // Treasure/portal can happen mid-frame (TryMove). Remember a confirm
            // tap so the overlay can fire it after the minimum delay.
            if ((gs == GS_NEXT || gs == GS_DEAD) && ActionPressed(ACT_CONFIRM))
                overlayConfirmQueued = true;
            break;
        }

        case GS_PAUSE: {
            if (PauseSettingsOpen()) UpdateMenuNav();
            if (PauseSettingsOpen()) {
                if (IsKeyPressed(KEY_ESCAPE)) {
                    if (SettingsCapturingKey()) SettingsCancelCapture();
                    else if (SettingsDeleteMode()) SettingsCancelDelete();
                    else if (MenuNavIsOn()) MenuNavExit();
                    else PauseSettingsSet(false);
                }
                if (MenuNavActivatePressed() == -2) PauseSettingsSet(false);
                if (ActionPressed(ACT_MENU_BACK) && !SettingsCapturingKey())
                    PauseSettingsSet(false);
            } else if (PauseLeaveConfirmOpen()) {
                Rectangle cPanel, confirmBtn, cancelBtn;
                GetPauseLeaveConfirmLayout(&cPanel, &confirmBtn, &cancelBtn);
                if (IsKeyPressed(KEY_ESCAPE) || ActionPressed(ACT_PAUSE))
                    PauseLeaveConfirmSet(false);
                else if (ActionPressed(ACT_CONFIRM)) {
                    PauseLeaveConfirmSet(false);
                    PauseSettingsSet(false);
                    ChallengeClearRunMode(); gs = GS_MENU; menuSub = 0;
                }
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    Vector2 mp = GetMousePosition();
                    if (PointInRect(mp, confirmBtn)) {
                        PauseLeaveConfirmSet(false);
                        PauseSettingsSet(false);
                        ChallengeClearRunMode(); gs = GS_MENU; menuSub = 0;
                    } else if (PointInRect(mp, cancelBtn) || !PointInRect(mp, cPanel)) {
                        PauseLeaveConfirmSet(false);
                    }
                }
            } else {
                Rectangle resumeBtn, settingsBtn, menuBtn;
                GetPauseMenuLayout(NULL, &resumeBtn, &settingsBtn, &menuBtn);

                if (ActionPressed(ACT_PAUSE)) {
                    PauseLeaveConfirmSet(false);
                    PauseSettingsSet(false);
                    gs = GS_PLAY;
                }
                if (ActionPressed(ACT_MENU_BACK)) {
                    if (exit_warning) PauseLeaveConfirmSet(true);
                    else { PauseSettingsSet(false); ChallengeClearRunMode(); gs = GS_MENU; menuSub = 0; }
                }
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    Vector2 mp = GetMousePosition();
                    if (PointInRect(mp, resumeBtn)) {
                        PauseLeaveConfirmSet(false);
                        PauseSettingsSet(false);
                        gs = GS_PLAY;
                    }
                    else if (PointInRect(mp, settingsBtn)) PauseSettingsSet(true);
                    else if (PointInRect(mp, menuBtn)) {
                        if (exit_warning) PauseLeaveConfirmSet(true);
                        else { PauseSettingsSet(false); ChallengeClearRunMode(); gs = GS_MENU; menuSub = 0; }
                    }
                }
            }

            UpdPars(dt);
            break;
        }

        case GS_DEAD:
        case GS_NEXT:
            if(gs==GS_NEXT&&ChallengeRunActive()&&roundn>=CHALLENGE_FLOORS)
                ChallengeRecordCompletion();
            sttime += dt;
            UpdPars(dt);
            if (ActionPressed(ACT_CONFIRM)) overlayConfirmQueued = true;
            {
                float lock = (gs == GS_NEXT) ? 0.20f : 0.45f;
                if (sttime >= lock) {
                    if (gs == GS_DEAD) {
                        if (overlayConfirmQueued) {
                            overlayConfirmQueued = false;
                            if(ChallengeRunMode()==2)ChallengeSaveSeedRunProgress();
                            roundn=1;normn=0;totcoi=0;tottrs=0;kills_game=0;pinvn=0;inv_sel=-1;coins_round=0; coinRush=false;
                            StatsInc(&global_games_played);
                            if(ChallengeRunMode()!=2)StartNewRunSeed();
                            else ResetSeededArtifactSequence();
                            ChallengeRestartRun();GenRound();gs=GS_PLAY;
                        }
                        if (ActionPressed(ACT_MENU_BACK)) { overlayConfirmQueued = false; ChallengeClearRunMode(); gs = GS_MENU; menuSub = 0; }
                    } else if (overlayConfirmQueued) {
                        overlayConfirmQueued = false;
                        if(ChallengeRunActive() && roundn>=CHALLENGE_FLOORS){
                            ChallengeClearRunMode();
                            gs=GS_MENU;menuSub=8;MenuNavExit();
                            break;
                        }
                        // Only completed NORMAL rounds densify walls/traps (normn).
                        // Map size itself tracks roundn.
                        if (roomKind == RK_NORMAL) normn++;
                        roundn++; coins_round=0; GenRound(); gs=GS_PLAY;
                        // "Best round reached" tracks the round just STARTED,
                        // not merely the last one completed — so it updates
                        // here too, not only on death/treasure/portal.
                        UpdateAndSaveStats();
                        if(ChallengeRunMode()==2)ChallengeSaveSeedRunProgress();
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
            ChallengeDrawRunOverlay();

            if (gs == GS_PAUSE) DrawPauseMenu();
            if (gs == GS_DEAD || gs == GS_NEXT) DrawOverlay(t);
        }

        // FPS overlay: top-right in menu, just below the HUD bar in gameplay.
        if (show_fps)
            DrawFPS(GetScreenWidth() - 90, (gs == GS_MENU) ? 36 : 82);

#ifdef DEBUG_ROOMS
        if (gs == GS_PLAY) DrawDebugRoomPanel();
#endif
        EndDrawing();
    }

    ChallengeSaveSeedRunProgress();
    UpdateAndSaveStats();
    SaveSettings();
    ShutdownMapAssets();
    ShutdownArtifactAssets();
    ShutdownSfx();
    ShutdownGameAudio();
    CloseWindow();
    return 0;
}
