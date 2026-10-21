// game_sfx.c — one-shot sound effects.
// Expects InitGameAudio() (and thus InitAudioDevice()) to have been called first.
// Drop .wav files into assets/sfx/ matching the names below.
#include "game_sfx.h"
#include "globals.h"
#include "raylib.h"
#include <stdio.h>

// Base names without extension — loader tries .wav then .mp3 automatically.
static const char *SFX_NAMES[SFX_COUNT] = {
    "coin_pickup",
    "item_pickup",
    "speed",
    "shield_activate",
    "shield_trigger",
    "sword",
    "ghostwalk",
    "bomb",
    "death_enemy",
    "death_trap",
    "treasure",
    "portal",
    "arrows",
    "boss_spawn",
    "boss_aim",
    "boss_charge",
    "boss_fire",
    "boss_adds",
    "boss_damage",
    "boss_defeat",
};

static Sound gSfx[SFX_COUNT];
static bool  gSfxLoaded[SFX_COUNT];

// Try to resolve "assets/sfx/<name>.<ext>", then the same relative to the exe dir.
static bool TryLoad(const char *name, const char *ext, Sound *out) {
    char rel[256], full[512];
    snprintf(rel, sizeof(rel), "assets/sfx/%s.%s", name, ext);
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
        if (TryLoad(SFX_NAMES[i], "wav", &gSfx[i]) ||
            TryLoad(SFX_NAMES[i], "ogg", &gSfx[i]) ||
            TryLoad(SFX_NAMES[i], "mp3", &gSfx[i])) {
            gSfxLoaded[i] = true;
        } else {
            gSfxLoaded[i] = false;
            TraceLog(LOG_WARNING, "SFX: missing assets/sfx/%s (.wav/.ogg/.mp3)", SFX_NAMES[i]);
        }
    }
}

void PlaySfx(SfxId id) {
    if (id < 0 || id >= SFX_COUNT) return;
    if (!gSfxLoaded[id] || audio_mute) return;
    float v = vol_master * vol_sfx;
    if (v <= 0.0f) return;
    SetSoundVolume(gSfx[id], v);
    PlaySound(gSfx[id]);
}

void ShutdownSfx(void) {
    for (int i = 0; i < SFX_COUNT; i++)
        if (gSfxLoaded[i]) { UnloadSound(gSfx[i]); gSfxLoaded[i] = false; }
}
