// game_sfx.c — one-shot sound effects.
// Expects InitGameAudio() (and thus InitAudioDevice()) to have been called first.
// Drop .wav files into assets/sfx/ matching the names below.
#include "game_sfx.h"
#include "globals.h"
#include "raylib.h"
#include <stdio.h>

// Relative base paths without extensions; loader tries .wav, .ogg, then .mp3.
static const char *SFX_PATHS[SFX_COUNT] = {
    "assets/sfx/coin_pickup",
    "assets/sfx/item_pickup",
    "assets/sfx/speed",
    "assets/sfx/shield_activate",
    "assets/sfx/shield_trigger",
    "assets/sfx/sword",
    "assets/sfx/ghostwalk",
    "assets/sfx/bomb",
    "assets/sfx/death_enemy",
    "assets/sfx/death_trap",
    "assets/sfx/treasure",
    "assets/sfx/portal",
    "assets/sfx/arrows",
    "assets/sfx/boss_spawn",
    "assets/sfx/boss_aim",
    "assets/sfx/boss_charge",
    "assets/sfx/boss_fire",
    "assets/sfx/boss_adds",
    "assets/sfx/boss_damage",
    "assets/sfx/boss_defeat",
    "assets/sfx/titlescreen-door-open",
    "assets/sfx/titlescreen-woosh",
    "assets/sfx/key-pickup",
    "assets/sfx/artifact",
    "assets/sfx/side_room_gate",
    "assets/sfx/locked_object",
    "assets/sfx/coin_pile",
};

static Sound gSfx[SFX_COUNT];
static bool  gSfxLoaded[SFX_COUNT];
static float gArtifactRampTime = -1.0f;
static float gArtifactRampBaseVolume = 0.0f;
static float gArtifactDuration = 0.0f;

#define ARTIFACT_START_GAIN 0.70f
#define ARTIFACT_END_GAIN 0.35f
#define ARTIFACT_FADE_IN_SECONDS 0.10f
#define ARTIFACT_FADE_OUT_SECONDS 0.30f

// Try to resolve "assets/sfx/<name>.<ext>", then the same relative to the exe dir.
static bool TryLoad(const char *basePath, const char *ext, Sound *out) {
    char rel[512], full[768];
    snprintf(rel, sizeof(rel), "%s.%s", basePath, ext);
    const char *path = rel;
    if (!FileExists(path)) {
        snprintf(full, sizeof(full), "%s%s", GetApplicationDirectory(), rel);
        path = full;
    }
    if (!FileExists(path)) return false;
    *out = LoadSound(path);
    return IsSoundValid(*out);
}

void InitSfx(void) {
    for (int i = 0; i < SFX_COUNT; i++) {
        if (TryLoad(SFX_PATHS[i], "wav", &gSfx[i]) ||
            TryLoad(SFX_PATHS[i], "ogg", &gSfx[i]) ||
            TryLoad(SFX_PATHS[i], "mp3", &gSfx[i])) {
            gSfxLoaded[i] = true;
        } else {
            gSfxLoaded[i] = false;
            TraceLog(LOG_WARNING, "SFX: missing %s (.wav/.ogg/.mp3)", SFX_PATHS[i]);
        }
    }
}

static bool SfxIsLoud(SfxId id) {
    switch (id) {
        case SFX_BOMB:
        case SFX_SWORD:
        case SFX_DEATH_ENEMY:
        case SFX_DEATH_TRAP:
        case SFX_BOSS_SPAWN:
        case SFX_BOSS_CHARGE:
        case SFX_BOSS_FIRE:
        case SFX_BOSS_DAMAGE:
        case SFX_BOSS_DEFEAT:
            return true;
        default:
            return false;
    }
}

void PlaySfx(SfxId id) {
    if (id < 0 || id >= SFX_COUNT) return;
    if (!gSfxLoaded[id] || audio_mute) return;
    float v = vol_master * vol_sfx;
    if (!loud_sfx && SfxIsLoud(id)) v *= 0.35f;
    if (id == SFX_KEY_PICKUP) {
        v *= 1.75f;
        if (v > 1.0f) v = 1.0f;
    }
    if (v <= 0.0f) return;
    if (id == SFX_ARTIFACT_PICKUP) {
        // The master gain is already applied by SetMasterVolume in game_audio.c.
        // Keep this rare pickup clear above the music without ignoring SFX volume.
        gArtifactRampBaseVolume = vol_sfx * 1.25f;
        if (gArtifactRampBaseVolume > 1.0f) gArtifactRampBaseVolume = 1.0f;
        gArtifactRampTime = 0.0f;
        gArtifactDuration = 0.0f;
        if (gSfx[id].stream.sampleRate > 0) {
            gArtifactDuration = (float)gSfx[id].frameCount / (float)gSfx[id].stream.sampleRate;
        }
        if (gArtifactDuration < 0.60f) gArtifactDuration = 0.60f;
        SetSoundVolume(gSfx[id], gArtifactRampBaseVolume * ARTIFACT_START_GAIN);
        PlaySound(gSfx[id]);
        return;
    }
    SetSoundVolume(gSfx[id], v);
    PlaySound(gSfx[id]);
}

void UpdateSfx(float dt) {
    if (gArtifactRampTime < 0.0f || !gSfxLoaded[SFX_ARTIFACT_PICKUP]) return;
    if (dt < 0.0f) dt = 0.0f;
    gArtifactRampTime += dt;
    if (gArtifactRampTime >= gArtifactDuration) {
        gArtifactRampTime = -1.0f;
        return;
    }
    float fadeIn = gArtifactRampTime / ARTIFACT_FADE_IN_SECONDS;
    if (fadeIn > 1.0f) fadeIn = 1.0f;
    float fadeOut = (gArtifactDuration - gArtifactRampTime) / ARTIFACT_FADE_OUT_SECONDS;
    if (fadeOut > 1.0f) fadeOut = 1.0f;
    if (fadeOut < 0.0f) fadeOut = 0.0f;
    float fadeInGain = ARTIFACT_START_GAIN + (1.0f - ARTIFACT_START_GAIN) * fadeIn;
    float fadeOutGain = ARTIFACT_END_GAIN + (1.0f - ARTIFACT_END_GAIN) * fadeOut;
    float gain = (fadeInGain < fadeOutGain) ? fadeInGain : fadeOutGain;
    SetSoundVolume(gSfx[SFX_ARTIFACT_PICKUP], gArtifactRampBaseVolume * gain);
}

void ShutdownSfx(void) {
    gArtifactRampTime = -1.0f;
    gArtifactDuration = 0.0f;
    for (int i = 0; i < SFX_COUNT; i++)
        if (gSfxLoaded[i]) { UnloadSound(gSfx[i]); gSfxLoaded[i] = false; }
}
