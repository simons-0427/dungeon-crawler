// main.c â€” window/init, the top-level state machine & game loop, and cleanup.
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
#include "game_console_win32.h"
#include <stdio.h>           // snprintf
#include <stdlib.h>          // abs
#include <string.h>          // memset
#include "mobile_input.h"
#include "mobile_platform.h"
#include "android_safe_area.h"
#include <time.h>            // time
#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#include "../Web/web_platform.h"
#endif
#ifdef _WIN32
void SetWindowsTaskbarIcon(void *windowHandle);
#endif

// Movement and actions go through ActionDown/ActionPressed so release builds
// honour extra keybinds. DEBUG_ROOMS keeps the hardcoded WASD / 1-5 / F11 set.

static bool exitRequested=false;
static bool mobileReady=false;
void AndroidGameOnPause(void) {
    if(!mobileReady)return;
    if(gs==GS_PLAY)gs=GS_PAUSE;
    PauseGameAudio(); ChallengeSaveSeedRunProgress();
    UpdateAndSaveStats(); SaveShop(); SaveSettings();
}
void AndroidGameOnResume(void) { if(mobileReady)ResumeMenuAudio(); }
static bool overlayConfirmQueued = false;
static void GameFrame(void);

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    MobilePrepareStorage();
    // Both executables use the project root for relative assets and save paths.
#ifdef DEBUG_ROOMS
    char projectDirectory[1024];
    snprintf(projectDirectory,sizeof(projectDirectory),"%s..",GetApplicationDirectory());
    if(!ChangeDirectory(projectDirectory))return 1;
#elif !defined(PLATFORM_WEB) && !defined(PLATFORM_ANDROID)
    if(!ChangeDirectory(GetApplicationDirectory()))return 1;
#endif
    GameConsoleInit();
#ifdef DEBUG_ROOMS
    if (!DirectoryExists("Debug/saves")) MakeDirectory("Debug/saves");
#else
    if (!DirectoryExists("saves")) MakeDirectory("saves");
#endif
    SetRandomSeed((unsigned)time(NULL));
#ifdef DEBUG_ROOMS
    LoadDebugRoomSettings();
#endif
    LoadSettings();  // file only â€” window geometry is applied after InitWindow
    GameConsoleSetLogDetail(console_log_detail);
    if (show_windows_console && !GameConsoleSetEnabled(true)) show_windows_console = false;
    int win_w = SW, win_h = SH;
    GetPersistedWindowState(NULL, NULL, NULL, &win_w, &win_h);
    // Keep the window out of the taskbar until its icon is installed.
#if defined(PLATFORM_WEB) || defined(PLATFORM_ANDROID)
    // A standard GLES framebuffer also works on drivers whose window MSAA
    // resolves only the clear colour. Keep native rendering device-independent.
    SetConfigFlags(0);
#else
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIDDEN);
#endif
    InitWindow(win_w, win_h, "Dungeon Crawler");
#if !defined(_WIN32) && !defined(PLATFORM_WEB) && !defined(PLATFORM_ANDROID)
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
                // Give Windows native-sized icons for the title bar and taskbar.
                // A single 1024px icon can fail to show in the taskbar.
                const int iconSizes[] = { 16, 24, 32, 48, 64, 256 };
                Image icons[sizeof(iconSizes)/sizeof(iconSizes[0])] = { 0 };
                int iconCount = 0;
                for (int i = 0; i < (int)(sizeof(iconSizes)/sizeof(iconSizes[0])); i++) {
                    Image scaled = ImageCopy(flat);
                    if (!scaled.data) continue;
                    ImageResize(&scaled, iconSizes[i], iconSizes[i]);
                    if (scaled.data) icons[iconCount++] = scaled;
                }
                if (iconCount > 0) SetWindowIcons(icons, iconCount);
                for (int i = 0; i < iconCount; i++) UnloadImage(icons[i]);
                UnloadImage(flat);
                UnloadImage(appIcon);
            }
        }
    }
#elif defined(_WIN32)
    SetWindowsTaskbarIcon(GetWindowHandle());
#endif
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
#if !defined(PLATFORM_WEB) && !defined(PLATFORM_ANDROID)
    ApplyPersistedWindowState();
#endif
    LoadStats();
    ChallengeLoadData();
    LoadShop();

#ifdef PLATFORM_WEB
    WebApplyStarterProfile();
#endif
    SetTargetFPS(60);

    gs = GS_MENU; roundn = 1; totcoi = 0; tottrs = 0;
    eneon = true; pinvn = 0; inv_sel = -1;
    memset(pars, 0, sizeof(pars));
    InitGameAudio();
    InitSfx();
    mobileReady=true;
 #ifndef PLATFORM_WEB
    ClearWindowState(FLAG_WINDOW_HIDDEN);
 #endif

#ifdef PLATFORM_WEB
    emscripten_set_main_loop(GameFrame, 0, 1);
#else
    while (!WindowShouldClose() && !exitRequested) GameFrame();
    ChallengeSaveSeedRunProgress();
    StatsFinishCurrentRoom();
    UpdateAndSaveStats();
    SaveSettings();
    ShutdownMapAssets();
    ShutdownArtifactAssets();
    ShutdownSfx();
    ShutdownGameAudio();
    CloseWindow();
    GameConsoleShutdown();
#endif
    return 0;
}

static void GameFrame(void) {
        if (GameConsoleUpdate()) { show_windows_console = false; SaveSettings(); }
        float dt = GetFrameTime();
        float t  = GetTime();

        MobileInputUpdate(gs==GS_PLAY,mobile_control_mode);
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
            UpdateMenuPlayAnim();
            UpdateMenuNav();
            if (MobileBackPressed()) {
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
                exitRequested=true;
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
            if(menuSub!=0 && MobileTouchPressed(GetMenuBackButtonRect())) {
                if(menuSub==2)statScroll=0;if(menuSub==3)upgScroll=0; menuSub=0; break;
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

                Vector2 mp = GetMousePosition();

                if (menuSub != 0) {
                    Rectangle backR=GetMenuBackButtonRect();
                    if(CheckCollisionPointRec(mp,backR)){menuSub=0;break;}
                    Rectangle panelR=GetMenuSubPanelRect();
                    if (menuSub != 1 && !CheckCollisionPointRec(mp, panelR)) { if (menuSub == 2) statScroll = 0; if (menuSub == 3) upgScroll = 0; menuSub=0; break; }
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
                StatsRunStarted();
                GenRound(); gs = GS_PLAY;
                StatsInc(&global_games_played);
                if(selectedRun==2)ChallengeSaveSeedRunProgress();
                MenuNavExit();
            }
            break;
        }

        case GS_PLAY: {
            StatsTickRoom(dt);
            if(ChallengeRunMode()!=0)ChallengeTickRun(dt);
#ifdef DEBUG_ROOMS
            if (UpdateDebugRoomPanel()) break;
#endif
            Rectangle pauseBtn = GetPauseButtonRect();

            if (MobileBackPressed() || ActionPressed(ACT_PAUSE)) { gs = GS_PAUSE; break; }
            if ((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                PointInRect(GetMousePosition(), pauseBtn)) || MobileTouchPressed(pauseBtn)) {
                gs = GS_PAUSE;
                break;
            }

#ifdef DEBUG_ROOMS
            // Dev shortcut: cycle room types in place.
            // Order: NORMAL â†’ BOSS â†’ TRAP â†’ DARK â†’ LAVA â†’ COIN RUSH â†’ â€¦
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
                for(int i=0;i<GetInventorySlotCount();i++)
                    if(MobileTouchPressed(GetInventorySlotRect(i)))UseItm(i);
                if (ActionPressed(ACT_USE_1)) UseItm(0);
                if (ActionPressed(ACT_USE_2)) UseItm(1);
                if (ActionPressed(ACT_USE_3)) UseItm(2);
                if (ActionPressed(ACT_USE_4)) UseItm(3);
                if (ActionPressed(ACT_USE_5)) UseItm(4);

                inv_sel = -1;
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
                    int beforeX=px,beforeY=py,beforeRound=roundn;
                    RoomKind beforeKind=roomKind;
                    int beforePart=CanEnterArtifactVaultRoom();
                    Vector2 touchDirection={0};
                    if(MobileGetMoveVector(&touchDirection,NULL)) {
                        TryMove((int)touchDirection.x,(int)touchDirection.y);moved=true;
                    } else if (ActionDown(ACT_MOVE_UP))    { TryMove(0,-1); moved=true; }
                    else if (ActionDown(ACT_MOVE_DOWN))  { TryMove(0,+1); moved=true; }
                    else if (ActionDown(ACT_MOVE_LEFT))  { TryMove(-1,0); moved=true; }
                    else if (ActionDown(ACT_MOVE_RIGHT)) { TryMove(+1,0); moved=true; }
                    if(moved&&roundn==beforeRound&&roomKind==beforeKind&&
                       CanEnterArtifactVaultRoom()==beforePart&&
                       abs(px-beforeX)+abs(py-beforeY)==1)StatsTileWalked();
                    if (moved) pmt = 0;
                }

                UpdPlayerHazards();   // e.g. a blink trap firing underfoot
                if (gs != GS_PLAY) {
                    if ((gs == GS_NEXT || gs == GS_DEAD) && ActionPressed(ACT_CONFIRM))
                        overlayConfirmQueued = true;
                    break;
                }
            }

            // â”€â”€ Coin Magnet perk â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
                    StatsCoinEarned(STATS_COIN_GROUND,val);
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
                if (MobileBackPressed()) {
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
                if (MobileBackPressed() || ActionPressed(ACT_PAUSE))
                    PauseLeaveConfirmSet(false);
                else if (ActionPressed(ACT_CONFIRM)) {
                    PauseLeaveConfirmSet(false);
                    PauseSettingsSet(false);
                    StatsFinishCurrentRoom(); ChallengeClearRunMode(); gs = GS_MENU; menuSub = 0;
                }
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    Vector2 mp = GetMousePosition();
                    if (PointInRect(mp, confirmBtn)) {
                        PauseLeaveConfirmSet(false);
                        PauseSettingsSet(false);
                        StatsFinishCurrentRoom(); ChallengeClearRunMode(); gs = GS_MENU; menuSub = 0;
                    } else if (PointInRect(mp, cancelBtn) || !PointInRect(mp, cPanel)) {
                        PauseLeaveConfirmSet(false);
                    }
                }
            } else {
                Rectangle resumeBtn, settingsBtn, menuBtn;
                GetPauseMenuLayout(NULL, &resumeBtn, &settingsBtn, &menuBtn);

                if (MobileBackPressed() || ActionPressed(ACT_PAUSE)) {
                    PauseLeaveConfirmSet(false);
                    PauseSettingsSet(false);
                    gs = GS_PLAY;
                }
                if (ActionPressed(ACT_MENU_BACK)) {
                    if (exit_warning) PauseLeaveConfirmSet(true);
                    else { PauseSettingsSet(false); StatsFinishCurrentRoom(); ChallengeClearRunMode(); gs = GS_MENU; menuSub = 0; }
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
                        else { PauseSettingsSet(false); StatsFinishCurrentRoom(); ChallengeClearRunMode(); gs = GS_MENU; menuSub = 0; }
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
            Rectangle primaryBtn,secondaryBtn;
            GetOverlayButtonLayout(&primaryBtn,&secondaryBtn);
            if(MobileTouchPressed(primaryBtn))overlayConfirmQueued=true;
            if(MobileBackPressed() || MobileTouchPressed(secondaryBtn)) {
                overlayConfirmQueued=false;StatsFinishCurrentRoom();ChallengeClearRunMode();gs=GS_MENU;menuSub=0;break;
            }
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
                            ChallengeRestartRun();StatsRunStarted();GenRound();gs=GS_PLAY;
                        }
                        if (ActionPressed(ACT_MENU_BACK)) { overlayConfirmQueued = false; StatsFinishCurrentRoom(); ChallengeClearRunMode(); gs = GS_MENU; menuSub = 0; }
                    } else if (overlayConfirmQueued) {
                        overlayConfirmQueued = false;
                        if(ChallengeRunActive() && roundn>=CHALLENGE_FLOORS){
                            StatsFinishCurrentRoom(); ChallengeClearRunMode();
                            gs=GS_MENU;menuSub=8;MenuNavExit();
                            break;
                        }
                        // Only completed NORMAL rounds densify walls/traps (normn).
                        // Map size itself tracks roundn.
                        if (roomKind == RK_NORMAL) normn++;
                        roundn++; coins_round=0; GenRound(); gs=GS_PLAY;
                        // "Best round reached" tracks the round just STARTED,
                        // not merely the last one completed â€” so it updates
                        // here too, not only on death/treasure/portal.
                        UpdateAndSaveStats();
                        if(ChallengeRunMode()==2)ChallengeSaveSeedRunProgress();
                    }
                }
            }
            break;
        }

        static int touchMenuContext=-999;
        int currentTouchContext=gs==GS_MENU?menuSub:-1;
        if(currentTouchContext!=touchMenuContext){MobileCancelMenuTap();touchMenuContext=currentTouchContext;}
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
            if(gs==GS_PLAY)MobileDrawStick();
            int bossSafeTop=0;AndroidGetSafeInsets(NULL,&bossSafeTop,NULL,NULL);
            BeginMode2D((Camera2D){.offset={0,bossSafeTop+20},.zoom=1});
            DrawBossBar();
            EndMode2D();
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
