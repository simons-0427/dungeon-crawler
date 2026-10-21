// game_audio.c — background music: random track selection, crossfade, silence gaps.
//
// Track layout:
//   index 0          = menu music (loops)
//   indices 1..N-1   = gameplay tracks (non-looping, played in random order)
//
// Transition model per gameplay track:
//   TS_WAITING  → count down silence gap, then pick random track
//   TS_FADE_IN  → play at 0→full volume over FADE_IN_DUR seconds
//   TS_PLAYING  → full volume; watch for FADE_OUT_DUR seconds before end
//   TS_FADE_OUT → fade to 0 over FADE_OUT_DUR seconds, then go to TS_WAITING
//
// On GS_PAUSE : pause stream (resume on GS_PLAY)
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
    MUSIC_COUNT
} MusicTrack;

static const char *MUSIC_PATHS[MUSIC_COUNT] = {
    "assets/music/safe_haven.mp3",
    "assets/music/dangerzone.mp3",
    "assets/music/treasure_hunter.mp3",
    "assets/music/neon_void.mp3"
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

static Music      gMusic[MUSIC_COUNT];
static bool       gLoaded[MUSIC_COUNT];
static bool       gAudioReady  = false;
static int        gCurrent     = -1;      // -1 = none
static TrackState gState       = TS_WAITING;
static float      gFadeTimer   = 0.0f;
static float      gWaitTimer   = 0.0f;
static float      gWaitDur     = 0.0f;   // 0 = start immediately

static char  gNPName[64] = "";
static float gNPTimer    = 0.0f;

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
        float v = (i == MUSIC_MENU)
            ? EffectiveVol()
            : (EffectiveVol() * (i == gCurrent ? FadeFactor() : 0.0f));
        if (v < 0.0f) v = 0.0f;
        SetMusicVolume(gMusic[i], v);
    }
}

// Pick a random loaded gameplay track that isn't the one currently playing.
static int PickRandomTrack(void) {
    int pool[MUSIC_COUNT], n = 0;
    for (int i = 1; i < MUSIC_COUNT; i++)
        if (gLoaded[i] && i != gCurrent) pool[n++] = i;
    if (n == 0) {
        // Only one track exists — allow repeat
        for (int i = 1; i < MUSIC_COUNT; i++)
            if (gLoaded[i]) pool[n++] = i;
    }
    return (n > 0) ? pool[GetRandomValue(0, n - 1)] : -1;
}

// Set the Now-Playing banner from a track index.
static void ShowNowPlaying(int idx) {
    if (idx < 0 || idx >= MUSIC_COUNT) { gNPTimer = 0.0f; return; }
    const char *path  = MUSIC_PATHS[idx];
    const char *slash = strrchr(path, '/');
    const char *base  = slash ? slash + 1 : path;
    strncpy(gNPName, base, sizeof(gNPName) - 1);
    gNPName[sizeof(gNPName) - 1] = '\0';
    char *dot = strrchr(gNPName, '.');
    if (dot) *dot = '\0';
    // underscore → space, capitalize each word
    bool newWord = true;
    for (char *c = gNPName; *c; c++) {
        if (*c == '_') { *c = ' '; newWord = true; }
        else if (newWord && *c >= 'a' && *c <= 'z') { *c -= 32; newWord = false; }
        else newWord = false;
    }
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
    SetMusicVolume(gMusic[gCurrent], 0.0f);
    PlayMusicStream(gMusic[gCurrent]);
    ShowNowPlaying(idx);
}

// ─── Public API ───────────────────────────────────────────────────────────────
void InitGameAudio(void) {
    InitAudioDevice();
    gAudioReady = IsAudioDeviceReady();
    if (!gAudioReady) { TraceLog(LOG_WARNING, "AUDIO: device init failed"); return; }
    for (int i = 0; i < MUSIC_COUNT; i++) {
        gMusic[i]  = LoadMusicAsset(MUSIC_PATHS[i]);
        gLoaded[i] = IsMusicValid(gMusic[i]);
        gMusic[i].looping = (i == MUSIC_MENU);
    }
    ApplyVolumes();
}

void UpdateGameAudio(GState state, float dt) {
    if (!gAudioReady) return;

    // Tick Now-Playing timer every frame regardless of state.
    if (gNPTimer > 0.0f) { gNPTimer -= dt; if (gNPTimer < 0.0f) gNPTimer = 0.0f; }

    // ── MENU ─────────────────────────────────────────────────────────────────
    if (state == GS_MENU) {
        if (gCurrent != MUSIC_MENU) {
            StopCurrent();
            if (gLoaded[MUSIC_MENU]) {
                gCurrent = MUSIC_MENU;
                gState   = TS_PLAYING;
                PlayMusicStream(gMusic[MUSIC_MENU]);
            }
        }
        if (gCurrent == MUSIC_MENU && gLoaded[MUSIC_MENU])
            UpdateMusicStream(gMusic[MUSIC_MENU]);
        ApplyVolumes();
        return;
    }

    // ── PAUSE — freeze gameplay audio ─────────────────────────────────────────
    if (state == GS_PAUSE) {
        if (gCurrent > 0 && gLoaded[gCurrent] && IsMusicStreamPlaying(gMusic[gCurrent]))
            PauseMusicStream(gMusic[gCurrent]);
        ApplyVolumes();
        return;
    }

    // ── DEAD — stop and reset so next game starts fresh ───────────────────────
    if (state == GS_DEAD) {
        if (gCurrent > 0 && gLoaded[gCurrent]) StopMusicStream(gMusic[gCurrent]);
        gCurrent   = -1;
        gState     = TS_WAITING;
        gWaitTimer = 0.0f;
        gWaitDur   = 0.0f;   // immediate track start on next game
        ApplyVolumes();
        return;
    }

    // ── GS_PLAY  /  GS_NEXT — run gameplay audio ──────────────────────────────
    // Coming from menu: stop menu track, init wait-state.
    if (gCurrent == MUSIC_MENU) {
        if (gLoaded[MUSIC_MENU]) StopMusicStream(gMusic[MUSIC_MENU]);
        gCurrent   = -1;
        gState     = TS_WAITING;
        gWaitTimer = 0.0f;
        gWaitDur   = 0.0f;
    }

    // Coming back from pause: resume the stream.
    if (gCurrent > 0 && gLoaded[gCurrent] && !IsMusicStreamPlaying(gMusic[gCurrent]))
        ResumeMusicStream(gMusic[gCurrent]);

    // ── Gameplay state machine ─────────────────────────────────────────────────
    switch (gState) {

    case TS_WAITING:
        gWaitTimer += dt;
        if (gWaitTimer >= gWaitDur) {
            int next = PickRandomTrack();
            if (next >= 0) BeginTrack(next);
            else { gWaitTimer = 0.0f; gWaitDur = WAIT_MIN; }  // no tracks — retry
        }
        break;

    case TS_FADE_IN:
        if (gCurrent >= 0 && gLoaded[gCurrent]) UpdateMusicStream(gMusic[gCurrent]);
        gFadeTimer += dt;
        if (gFadeTimer >= FADE_IN_DUR) { gFadeTimer = 0.0f; gState = TS_PLAYING; }
        break;

    case TS_PLAYING:
        if (gCurrent >= 0 && gLoaded[gCurrent]) {
            UpdateMusicStream(gMusic[gCurrent]);
            float len    = GetMusicTimeLength(gMusic[gCurrent]);
            float played = GetMusicTimePlayed(gMusic[gCurrent]);
            if (len > FADE_OUT_DUR && played >= len - FADE_OUT_DUR) {
                gState     = TS_FADE_OUT;
                gFadeTimer = 0.0f;
            }
        }
        break;

    case TS_FADE_OUT:
        if (gCurrent >= 0 && gLoaded[gCurrent]) UpdateMusicStream(gMusic[gCurrent]);
        gFadeTimer += dt;
        if (gFadeTimer >= FADE_OUT_DUR) {
            if (gCurrent >= 0 && gLoaded[gCurrent]) StopMusicStream(gMusic[gCurrent]);
            gCurrent   = -1;
            gState     = TS_WAITING;
            gWaitTimer = 0.0f;
            gWaitDur   = WAIT_MIN + GetRandomValue(0, (int)((WAIT_MAX - WAIT_MIN) * 100)) / 100.0f;
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
}
