// game_audio.c — background music: random track selection, crossfade, silence gaps.
//
// Track layout:
//   index 0..N-1     = selectable soundtrack tracks
//   menu selection   = loops; random gameplay rotation uses indices 1..N-1
//   fixed gameplay   = may select any loaded track, including index 0
//
// Transition model per gameplay track:
//   TS_WAITING  → count down silence gap, then pick random track
//   TS_FADE_IN  → play at 0→full volume over FADE_IN_DUR seconds
//   TS_PLAYING  → full volume; watch for FADE_OUT_DUR seconds before end
//   TS_FADE_OUT → fade to 0 over FADE_OUT_DUR seconds, then go to TS_WAITING
//
// On GS_PAUSE : always silent; gameplay music starts fresh after resuming.
// On GS_DEAD  : stop stream, reset to TS_WAITING (fresh track on next game)
// On GS_NEXT  : do nothing — music continues through the round transition

#include "game_audio.h"
#include "globals.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>

// ─── Track index constants ────────────────────────────────────────────────────
typedef enum {
    MUSIC_MENU = 0,
    MUSIC_DANGERZONE,
    MUSIC_TREASURE_HUNTER,
    MUSIC_NEON_VOID,
    MUSIC_SAFE_HAVEN,
    MUSIC_INSERT_COIN,
    MUSIC_PIXEL_PANIC,
    MUSIC_OVERACHIEVERS_OVERTUNE,
    MUSIC_NEON_PULSE,
    MUSIC_COUNT = MUSIC_TRACK_COUNT
} MusicTrack;

static const char *MUSIC_PATHS[MUSIC_COUNT] = {
    "assets/music/dungeon_crawler.mp3",
    "assets/music/dangerzone.mp3",
    "assets/music/treasure_hunter.mp3",
    "assets/music/neon_void.mp3",
    "assets/music/safe_haven.mp3",
    "assets/music/insert_coin.mp3",
    "assets/music/pixel_panic.mp3",
    "assets/music/overachievers_overtune.mp3",
    "assets/music/neon_pulse.mp3"
};

static const char *MUSIC_NAMES[MUSIC_COUNT] = {
    "Dungeon Crawler",
    "Dangerzone",
    "Treasure Hunter",
    "Neon Void",
    "Safe Haven",
    "Insert Coin",
    "Pixel Panic",
    "Overachievers Overtune",
    "Neon Pulse"
};

static const MusicAccess MUSIC_ACCESS[MUSIC_COUNT] = {
    MUSIC_ACCESS_INCLUDED, MUSIC_ACCESS_INCLUDED, MUSIC_ACCESS_INCLUDED, MUSIC_ACCESS_INCLUDED,
    MUSIC_ACCESS_INCLUDED, MUSIC_ACCESS_PURCHASE, MUSIC_ACCESS_PURCHASE, MUSIC_ACCESS_ACHIEVEMENT,
    MUSIC_ACCESS_PURCHASE
};

static const int MUSIC_PRICES[MUSIC_COUNT] = {
    0, 0, 0, 0, 0, 750, 1250, 0, 1500
};

static const char *MUSIC_ACCESS_LABELS[MUSIC_COUNT] = {
    "INCLUDED", "INCLUDED", "INCLUDED", "INCLUDED",
    "INCLUDED", "BUY WITH COINS", "BUY WITH COINS", "OVERACHIEVER TIER 6",
    "BUY WITH COINS"
};

// ─── Crossfade / gap constants ────────────────────────────────────────────────
static const float FADE_IN_DUR   = 3.0f;   // seconds to reach full volume
static const float FADE_OUT_DUR  = 5.0f;   // seconds before track end to start fading
static const float WAIT_MIN      = 5.0f;   // min silence gap between tracks
static const float WAIT_MAX      = 10.0f;  // max silence gap

// ─── Now-Playing display constants ───────────────────────────────────────────
static const float NP_DUR  = 4.0f;   // total display time in seconds
static const float NP_FADE = 0.5f;   // fade-in / fade-out duration

// ─── Module state ────────────────────────────────────────────────────────────
typedef enum { TS_WAITING, TS_FADE_IN, TS_PLAYING, TS_FADE_OUT } TrackState;
typedef enum { AC_NONE, AC_MENU, AC_GAMEPLAY, AC_PAUSE } AudioContext;

static Music      gMusic[MUSIC_COUNT];
static bool       gLoaded[MUSIC_COUNT];
static bool       gAudioReady  = false;
static int        gCurrent     = -1;      // -1 = none
static TrackState gState       = TS_WAITING;
static AudioContext gContext   = AC_NONE;
static float      gFadeTimer   = 0.0f;
static float      gWaitTimer   = 0.0f;
static float      gWaitDur     = 0.0f;   // 0 = start immediately

static char  gNPName[64] = "";
static float gNPTimer    = 0.0f;

static unsigned int GetUnlockedMusicMask(void);

// ─── Now-Playing API ─────────────────────────────────────────────────────────
const char *GetNowPlayingName(void) { return (gNPTimer > 0.0f) ? gNPName : NULL; }

float GetNowPlayingAlpha(void) {
    if (gNPTimer <= 0.0f) return 0.0f;
    float fi = (NP_DUR - gNPTimer) / NP_FADE; if (fi > 1.0f) fi = 1.0f;
    float fo = gNPTimer          / NP_FADE; if (fo > 1.0f) fo = 1.0f;
    return fi * fo;
}

// ─── Private helpers ─────────────────────────────────────────────────────────
static Music LoadMusicAsset(const char *rel) {
    char full[512];
    const char *path = rel;
    if (!FileExists(path)) {
        snprintf(full, sizeof(full), "%s%s", GetApplicationDirectory(), rel);
        path = full;
    }
    Music m = LoadMusicStream(path);
    if (!IsMusicValid(m)) TraceLog(LOG_WARNING, "AUDIO: failed to load %s", path);
    return m;
}

static float EffectiveVol(void) {
    return audio_mute ? 0.0f : (vol_master * vol_music);
}

static float FadeFactor(void) {
    if (gState == TS_FADE_IN)  return gFadeTimer / FADE_IN_DUR;
    if (gState == TS_FADE_OUT) return 1.0f - gFadeTimer / FADE_OUT_DUR;
    return 1.0f;
}

static void ApplyVolumes(void) {
    if (!gAudioReady) return;
    SetMasterVolume(audio_mute ? 0.0f : vol_master);
    for (int i = 0; i < MUSIC_COUNT; i++) {
        if (!gLoaded[i]) continue;
        float v = (i == gCurrent) ? (EffectiveVol() * FadeFactor()) : 0.0f;
        if (v < 0.0f) v = 0.0f;
        SetMusicVolume(gMusic[i], v);
    }
}

// Pick one loaded track from a bitmask. When possible, avoid repeating the
// currently playing track so multi-track rotations feel like real playlists.
static int PickTrackFromMask(unsigned int mask, bool avoidCurrent) {
    int pool[MUSIC_COUNT], n = 0;
    for (int i = 0; i < MUSIC_COUNT; i++) {
        if ((mask & (1u << i)) && gLoaded[i] &&
            (!avoidCurrent || i != gCurrent))
            pool[n++] = i;
    }
    if (n == 0 && avoidCurrent) return PickTrackFromMask(mask, false);
    return (n > 0) ? pool[GetRandomValue(0, n - 1)] : -1;
}

static int PickGameplayTrack(void) {
    unsigned int available = GetUnlockedMusicMask();
    if (music_gameplay_tracks != 0u)
        return PickTrackFromMask(music_gameplay_tracks & available, true);
    return -1;
}

// Set the Now-Playing banner from a track index.
static void ShowNowPlaying(int idx) {
    if (idx < 0 || idx >= MUSIC_COUNT) { gNPTimer = 0.0f; return; }
    strncpy(gNPName, MUSIC_NAMES[idx], sizeof(gNPName) - 1);
    gNPName[sizeof(gNPName) - 1] = '\0';
    // underscore → space, capitalize each word
    gNPTimer = NP_DUR;
}

static void StopCurrent(void) {
    if (gCurrent >= 0 && gLoaded[gCurrent]) StopMusicStream(gMusic[gCurrent]);
    gCurrent   = -1;
    gState     = TS_WAITING;
    gFadeTimer = 0.0f;
}

static void BeginTrack(int idx) {
    if (idx < 0 || !gLoaded[idx]) { gState = TS_WAITING; gWaitTimer = 0.0f; gWaitDur = WAIT_MIN; return; }
    if (gCurrent >= 0 && gLoaded[gCurrent]) StopMusicStream(gMusic[gCurrent]);
    gCurrent   = idx;
    gState     = TS_FADE_IN;
    gFadeTimer = 0.0f;
    gMusic[gCurrent].looping = false;
    SetMusicVolume(gMusic[gCurrent], 0.0f);
    PlayMusicStream(gMusic[gCurrent]);
    ShowNowPlaying(idx);
}

static void BeginLoopingTrack(int idx) {
    StopCurrent();
    if (idx < 0 || idx >= MUSIC_COUNT || !gLoaded[idx]) return;
    gCurrent   = idx;
    gState     = TS_PLAYING;
    gFadeTimer = 0.0f;
    gMusic[gCurrent].looping = true;
    SetMusicVolume(gMusic[gCurrent], EffectiveVol());
    PlayMusicStream(gMusic[gCurrent]);
    ShowNowPlaying(idx);
}

int GetMusicTrackCount(void) { return MUSIC_COUNT; }

const char *GetMusicTrackName(int index) {
    if (index < 0 || index >= MUSIC_COUNT) return "Unknown";
    return MUSIC_NAMES[index];
}

MusicAccess GetMusicTrackAccess(int index) {
    if (index < 0 || index >= MUSIC_COUNT) return MUSIC_ACCESS_INCLUDED;
    return MUSIC_ACCESS[index];
}

int GetMusicTrackPrice(int index) {
    if (index < 0 || index >= MUSIC_COUNT) return 0;
    return MUSIC_PRICES[index];
}

const char *GetMusicTrackAccessLabel(int index) {
    if (index < 0 || index >= MUSIC_COUNT) return "INCLUDED";
    return MUSIC_ACCESS_LABELS[index];
}

bool IsMusicTrackUnlocked(int index) {
    if (index < 0 || index >= MUSIC_COUNT) return false;
    if (MUSIC_ACCESS[index] == MUSIC_ACCESS_INCLUDED) return true;
    return music_owned[index] != 0;
}

static unsigned int GetUnlockedMusicMask(void) {
    unsigned int mask = 0u;
    for (int i = 0; i < MUSIC_COUNT; i++)
        if (IsMusicTrackUnlocked(i)) mask |= (1u << i);
    return mask;
}

// ─── Public API ───────────────────────────────────────────────────────────────
void InitGameAudio(void) {
    InitAudioDevice();
    gAudioReady = IsAudioDeviceReady();
    if (!gAudioReady) { TraceLog(LOG_WARNING, "AUDIO: device init failed"); return; }
    for (int i = 0; i < MUSIC_COUNT; i++) {
        gMusic[i]  = LoadMusicAsset(MUSIC_PATHS[i]);
        gLoaded[i] = IsMusicValid(gMusic[i]);
        gMusic[i].looping = false;
    }
    ApplyVolumes();
}

void UpdateGameAudio(GState state, float dt) {
    if (!gAudioReady) return;

    // Tick Now-Playing timer every frame regardless of state.
    if (gNPTimer > 0.0f) {
        gNPTimer -= dt;
        if (gNPTimer < 0.0f) gNPTimer = 0.0f;
    }

    // MENU: one selected track loops; multiple selected tracks form a small
    // playlist and advance automatically while the menu stays open.
    if (state == GS_MENU) {
        unsigned int allMask = GetUnlockedMusicMask();
        unsigned int selectedMask = music_menu_tracks & allMask;
        bool multiple = selectedMask != 0u &&
                        (selectedMask & (selectedMask - 1u)) != 0u;

        if (selectedMask == 0u) {
            if (gContext != AC_MENU || gCurrent >= 0) StopCurrent();
            gContext = AC_MENU;
        } else {
            bool currentSelected = gCurrent >= 0 && gCurrent < MUSIC_COUNT &&
                                   (selectedMask & (1u << gCurrent));
            bool modeMismatch = currentSelected &&
                                (gMusic[gCurrent].looping != !multiple);
            if (gContext != AC_MENU || !currentSelected || modeMismatch) {
                int next = PickTrackFromMask(selectedMask, true);
                if (next >= 0) {
                    if (multiple) BeginTrack(next);
                    else         BeginLoopingTrack(next);
                } else {
                    StopCurrent();
                }
                gContext = AC_MENU;
            } else if (gCurrent >= 0 && gLoaded[gCurrent]) {
                UpdateMusicStream(gMusic[gCurrent]);
                if (multiple) {
                    if (gState == TS_FADE_IN) {
                        gFadeTimer += dt;
                        if (gFadeTimer >= FADE_IN_DUR) {
                            gFadeTimer = 0.0f;
                            gState = TS_PLAYING;
                        }
                    } else if (gState == TS_PLAYING) {
                        float len = GetMusicTimeLength(gMusic[gCurrent]);
                        float played = GetMusicTimePlayed(gMusic[gCurrent]);
                        if (len > 0.0f &&
                            (played >= len - 0.05f ||
                             !IsMusicStreamPlaying(gMusic[gCurrent]))) {
                            int next = PickTrackFromMask(selectedMask, true);
                            if (next >= 0) BeginTrack(next);
                        }
                    }
                }
            }
        }
        ApplyVolumes();
        return;
    }

    // PAUSE: the pause menu is always silent. Stop the stream immediately and
    // keep it silent for every frame spent on the overlay.
    if (state == GS_PAUSE) {
        StopCurrent();
        gContext = AC_PAUSE;
        ApplyVolumes();
        return;
    }

    // DEAD: stop and reset so the next game starts fresh.
    if (state == GS_DEAD) {
        StopCurrent();
        gContext   = AC_NONE;
        gWaitTimer = 0.0f;
        gWaitDur   = 0.0f;
        ApplyVolumes();
        return;
    }

    // GS_PLAY and GS_NEXT use the gameplay rotation.
    // Returning from pause starts the gameplay rotation cleanly again.
    if (gContext == AC_PAUSE) {
        StopCurrent();
        gContext   = AC_NONE;
        gWaitTimer = 0.0f;
        gWaitDur   = 0.0f;
    }

    // Coming from the menu or the silent pause overlay.
    if (gContext != AC_GAMEPLAY) {
        StopCurrent();
        gContext   = AC_GAMEPLAY;
        gWaitTimer = 0.0f;
        gWaitDur   = 0.0f;
    }

    switch (gState) {
    case TS_WAITING:
        gWaitTimer += dt;
        if (gWaitTimer >= gWaitDur) {
            int next = PickGameplayTrack();
            if (next >= 0) BeginTrack(next);
            else {
                gWaitTimer = 0.0f;
                gWaitDur = WAIT_MIN;
            }
        }
        break;

    case TS_FADE_IN:
        if (gCurrent >= 0 && gLoaded[gCurrent])
            UpdateMusicStream(gMusic[gCurrent]);
        gFadeTimer += dt;
        if (gFadeTimer >= FADE_IN_DUR) {
            gFadeTimer = 0.0f;
            gState = TS_PLAYING;
        }
        break;

    case TS_PLAYING:
        if (gCurrent >= 0 && gLoaded[gCurrent]) {
            UpdateMusicStream(gMusic[gCurrent]);
            float len = GetMusicTimeLength(gMusic[gCurrent]);
            float played = GetMusicTimePlayed(gMusic[gCurrent]);
            if (len > FADE_OUT_DUR && played >= len - FADE_OUT_DUR) {
                gState = TS_FADE_OUT;
                gFadeTimer = 0.0f;
            }
        }
        break;

    case TS_FADE_OUT:
        if (gCurrent >= 0 && gLoaded[gCurrent])
            UpdateMusicStream(gMusic[gCurrent]);
        gFadeTimer += dt;
        if (gFadeTimer >= FADE_OUT_DUR) {
            if (gCurrent >= 0 && gLoaded[gCurrent])
                StopMusicStream(gMusic[gCurrent]);
            gCurrent = -1;
            gState = TS_WAITING;
            gWaitTimer = 0.0f;
            gWaitDur = WAIT_MIN +
                       GetRandomValue(0, (int)((WAIT_MAX - WAIT_MIN) * 100)) / 100.0f;
        }
        break;
    }

    ApplyVolumes();
}

void ShutdownGameAudio(void) {
    if (!gAudioReady) return;
    StopCurrent();
    for (int i = 0; i < MUSIC_COUNT; i++)
        if (gLoaded[i]) { UnloadMusicStream(gMusic[i]); gLoaded[i] = false; }
    CloseAudioDevice();
    gAudioReady = false;
    gContext = AC_NONE;
}
